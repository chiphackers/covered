/*
 Copyright (c) 2006-2010 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     func_iter.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/2/2007
*/

#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "func_iter.h"
#include "func_unit.h"
#include "util.h"


/*!
 Displays the given function iterator to standard output.
*/
void func_iter_display(
  func_iter* fi  /*!< Pointer to functional unit iterator to display */
) { PROFILE(FUNC_ITER_DISPLAY);

  int i;  /* Loop iterator */

  printf( "Functional unit iterator (scopes: %u, sl_num: %u):\n", fi->scopes, fi->sl_num );

  if( fi->sls != NULL ) {
    for( i=0; i<fi->sl_num; i++ ) {
      if( fi->sls[i] != NULL ) {
        stmt_link_display( fi->sls[i] );
      }
    }
  }

  if( fi->sigs != NULL ) {
    for( i=0; i<fi->sig_num; i++ ) {
      if( fi->sigs[i] != NULL ) {
        sig_link_display( fi->sigs[i], fi->sig_size[i] );
      }
    }
  }

  PROFILE_END;

}

/*!
 Performs a bubble sort of the first element such that the first line is in location 0 of the sis array.
*/
static void func_iter_sort(
  func_iter* fi  /*!< Pointer to functional unit iterator to sort */
) { PROFILE(FUNC_ITER_SORT);

  stmt_link* tmp;  /* Temporary statement link */
  int        i;    /* Loop iterator */

  assert( fi != NULL );
  assert( fi->sl_num > 0 );

  tmp = fi->sls[0];

  /*
   If the statement iterator at the top of the list is NULL, shift all valid statement iterators
   towards the top and store this statement iterator after them 
  */
  if( tmp == NULL ) {

    for( i=0; i<(fi->sl_num - 1); i++ ) {
      fi->sls[i] = fi->sls[i+1];
    }
    fi->sls[i] = tmp;
    (fi->sl_num)--;

  /* Otherwise, re-sort them based on line number and then column number (if there is a tie). */
  } else {

    i = 0;
    while( (i < (fi->sl_num - 1)) &&
           ((tmp->stmt->exp->ppfline > fi->sls[i+1]->stmt->exp->ppfline) ||
            ((tmp->stmt->exp->ppfline == fi->sls[i+1]->stmt->exp->ppfline) &&
             (tmp->stmt->exp->col.part.last > fi->sls[i+1]->stmt->exp->col.part.last))) ) {
      fi->sls[i] = fi->sls[i+1];
      i++;
    }
    fi->sls[i] = tmp;

  }

  PROFILE_END;

}

/*!
 \return Returns the number of statement iterators found in all of the unnamed functional units
         within a named functional unit.
*/
static int func_iter_count_scopes(
  func_unit* funit,   /*!< Pointer to current functional unit being examined */
  bool       inc_all  /*!< If set to TRUE, counts all scopes (even named scopes) */
) { PROFILE(FUNC_ITER_COUNT_STMT_ITERS);

  int         count = 1;  /* Number of statement iterators within this functional unit */
  funit_link* child;      /* Pointer to child functional unit */
  func_unit*  parent;     /* Parent module of this functional unit */

  assert( funit != NULL );

  /* Get parent module */
  parent = funit_get_curr_module( funit );

  /* Iterate through children functional units, counting all of the unnamed scopes */
  child = parent->tf_head;
  while( child != NULL ) {
    if( (inc_all || funit_is_unnamed( child->funit )) && (child->funit->parent == funit) ) {
      count += func_iter_count_scopes( child->funit, inc_all );
    }
    child = child->next;
  }

  PROFILE_END;

  return( count );

}

/*!
 Recursively iterates through functional units, adding their statement iterators to the func_iter structure's array.
*/
static void func_iter_add_stmt_links(
  func_iter* fi,      /*!< Pointer to functional unit iterator to populate */
  func_unit* funit,   /*!< Pointer to current functional unit */
  bool       inc_all  /*!< If TRUE, uses all statements in the functional unit (even in named scopes) */
) { PROFILE(FUNC_ITER_ADD_STMT_ITERS);

  funit_link* child;   /* Pointer to child functional unit */
  func_unit*  parent;  /* Pointer to parent module of this functional unit */
  int         i;       /* Loop iterator */

  /* First, shift all current statement iterators down by one */
  for( i=(fi->scopes - 2); i>=0; i-- ) {
    fi->sls[i+1] = fi->sls[i];
  }

  /* Set the sls pointer to the head of the functional unit statement list */
  fi->sls[0] = funit->stmt_head;

  /* Increment the si_num */
  (fi->sl_num)++;

  /* Now sort the new statement iterator */
  func_iter_sort( fi );

  /* Get the parent module */
  parent = funit_get_curr_module( funit );

  /* Now traverse down all of the child functional units doing the same */
  child = parent->tf_head;
  while( child != NULL ) {
    if( (inc_all || funit_is_unnamed( child->funit )) && (child->funit->parent == funit) ) {
      func_iter_add_stmt_links( fi, child->funit, inc_all );
    }
    child = child->next;
  }

  PROFILE_END;

}

/*!
 Recursively iterates through the functional units, adding their signal link pointers to the func_iter structure's array.
*/
static void func_iter_add_sig_links(
  func_iter* fi,      /*!< Pointer to functional unit iterator to populate */
  func_unit* funit,   /*!< Pointer to current functional unit */
  bool       inc_all  /*!< If set to TRUE, includes all scopes */
) { PROFILE(FUNC_ITER_ADD_SIG_LINKS);

  funit_link* child;   /* Pointer to child functional unit */
  func_unit*  parent;  /* Pointer to parent module of this functional unit */

  /* Add the pointer */
  fi->sigs[fi->sig_num]     = funit->sigs;
  fi->sig_size[fi->sig_num] = funit->sig_size;
  fi->sig_num++;

  /* Get the parent module */
  parent = funit_get_curr_module( funit );

  /* Now traverse down all of the child functional units doing the same */
  child = parent->tf_head;
  while( child != NULL ) {
    if( (inc_all || funit_is_unnamed( child->funit )) && (child->funit->parent == funit) ) {
      func_iter_add_sig_links( fi, child->funit, inc_all );
    }
    child = child->next;
  }

  PROFILE_END;

}

/*!
 Initializes the contents of the functional unit iterator structure.  This should be called before
 the functional unit iterator structure is populated with either statement iterator or signal
 information.
*/
void func_iter_init(
  func_iter* fi,      /*!< Pointer to functional unit iterator to initializes */
  func_unit* funit,   /*!< Pointer to main functional unit to create iterator for (must be named) */
  bool       stmts,   /*!< Set to TRUE if we want statements to be included in the iterator */
  bool       sigs,    /*!< Set to TRUE if we want signals to be included in the iterator */
  bool       inc_all  /*!< Set to TRUE to use all statements/signals including those in named scopes */
) { PROFILE(FUNC_ITER_INIT);

  assert( fi != NULL );
  assert( funit != NULL );

  /* Count the number of scopes that are within the functional unit iterator */
  fi->scopes   = func_iter_count_scopes( funit, inc_all );
  fi->sls      = NULL;
  fi->sigs     = NULL;
  fi->sig_size = NULL;
  fi->sig_num  = 0;

  /* Add statement iterators */
  if( stmts ) {
    fi->sls    = (stmt_link**)malloc_safe( sizeof( stmt_link* ) * fi->scopes );
    fi->sl_num = 0;
    func_iter_add_stmt_links( fi, funit, inc_all );
  }

  /* Add signal lists */
  if( sigs ) {
    fi->sigs     = (vsignal***)malloc_safe( sizeof( vsignal** ) * fi->scopes );
    fi->sig_size = (int*)malloc_safe( sizeof( int ) * fi->scopes );
    fi->sig_num  = 0;
    func_iter_add_sig_links( fi, funit, inc_all );
    fi->sig_num  = 0;
    fi->curr_sig = 0;
  }

  PROFILE_END;

}

/*!
 \return Returns pointer to next statement in line order (or NULL if there are no more
         statements in the given functional unit)
*/
statement* func_iter_get_next_statement(
  func_iter* fi  /*!< Pointer to functional unit iterator to use */
) { PROFILE(FUNC_ITER_GET_NEXT_STATEMENT);

  statement* stmt;  /* Pointer to next statement in line order */

  assert( fi != NULL );

  if( fi->sl_num == 0 ) {

    stmt = NULL;

  } else {

    assert( fi->sls[0] != NULL );

    /* Get the statement at the head of the sorted list */
    stmt = fi->sls[0]->stmt;

    /* Go to the next statement in the current statement list */
    fi->sls[0] = fi->sls[0]->next;

    /* Resort */
    func_iter_sort( fi );

  }

  PROFILE_END;

  return( stmt );

}

/*!
 \return Returns pointer to next signal in order (or NULL if there are no more signals in the
         given functional unit)
*/
vsignal* func_iter_get_next_signal(
  func_iter* fi  /*!< Pointer to functional unit iterator to use */
) { PROFILE(FUNC_ITER_GET_NEXT_SIGNAL);

  vsignal* sig;

  assert( fi != NULL );

  if( fi->curr_sig < fi->sig_size[fi->sig_num] ) {

    sig = fi->sigs[fi->sig_num][fi->curr_sig];
    fi->curr_sig++;

  } else {

    do {
      fi->sig_num++;
    } while( (fi->sig_num < fi->scopes) && (fi->sigs[fi->sig_num] == NULL) );

    if( fi->sig_num < fi->scopes ) {
      sig          = fi->sigs[fi->sig_num][0];
      fi->curr_sig = 1;
    } else {
      sig          = NULL;
      fi->curr_sig = 0;
    }

  }

  PROFILE_END;

  return( sig );

}

/*!
 Deallocates all allocated memory for the given functional unit iterator.
*/
void func_iter_dealloc(
  func_iter* fi  /*!< Pointer to functional unit iterator to deallocate */
) { PROFILE(FUNC_ITER_DEALLOC);

  int i;  /* Loop iterator */
  
  if( fi != NULL ) {

    /* Deallocate statement iterators */
    free_safe( fi->sls, (sizeof( stmt_link* ) * fi->scopes) );
    fi->sls = NULL;

    /* Deallocate signal arrays */
    free_safe( fi->sigs,     (sizeof( vsignal** ) * fi->scopes) );
    free_safe( fi->sig_size, (sizeof( unsigned int ) * fi->scopes) );

    fi->sigs     = NULL;
    fi->sig_size = NULL;

  }

  PROFILE_END;
  
}

