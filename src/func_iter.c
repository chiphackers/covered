/*
 Copyright (c) 2006 Trevor Williams

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
#include "iter.h"
#include "util.h"


/*!
 Displays the given function iterator to standard output.
*/
void func_iter_display(
  func_iter* fi  /*!< Pointer to functional unit iterator to display */
) { PROFILE(FUNC_ITER_DISPLAY);

  int i;  /* Loop iterator */

  printf( "Functional unit iterator (scopes: %u, si_num: %u):\n", fi->scopes, fi->si_num );

  if( fi->sis != NULL ) {
    for( i=0; i<fi->si_num; i++ ) {
      if( fi->sis[i] != NULL ) {
        stmt_iter si;
        stmt_iter_copy( &si, fi->sis[i] );
        stmt_iter_reverse( &si );
        while( si.curr != NULL ) stmt_iter_next( &si );
        stmt_iter_reverse( &si );
        stmt_link_display( si.curr );
      }
    }
  }

  if( fi->sigs != NULL ) {
    for( i=0; i<fi->sig_num; i++ ) {
      if( fi->sigs[i] != NULL ) {
        printf( "  Name: %s\n", fi->sigs[i]->sig->name );
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

  stmt_iter* tmp;  /* Temporary statement iterator */
  int        i;    /* Loop iterator */

  assert( fi != NULL );
  assert( fi->si_num > 0 );

  tmp = fi->sis[0];

  /*
   If the statement iterator at the top of the list is NULL, shift all valid statement iterators
   towards the top and store this statement iterator after them 
  */
  if( tmp->curr == NULL ) {

    for( i=0; i<(fi->si_num - 1); i++ ) {
      fi->sis[i] = fi->sis[i+1];
    }
    fi->sis[i] = tmp;
    (fi->si_num)--;

  /* Otherwise, re-sort them based on line number and then column number (if there is a tie). */
  } else {

    i = 0;
    while( (i < (fi->si_num - 1)) &&
           ((tmp->curr->stmt->exp->line > fi->sis[i+1]->curr->stmt->exp->line) ||
            ((tmp->curr->stmt->exp->line == fi->sis[i+1]->curr->stmt->exp->line) &&
             ((tmp->curr->stmt->exp->col & 0xffff) > (fi->sis[i+1]->curr->stmt->exp->col & 0xffff)))) ) {
      fi->sis[i] = fi->sis[i+1];
      i++;
    }
    fi->sis[i] = tmp;

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
static void func_iter_add_stmt_iters(
  func_iter* fi,        /*!< Pointer to functional unit iterator to populate */
  func_unit* funit,     /*!< Pointer to current functional unit */
  bool       use_tail,  /*!< If TRUE, starts at the statement tail; otherwise, starts at the statement head */
  bool       inc_all    /*!< If TRUE, uses all statements in the functional unit (even in named scopes) */
) { PROFILE(FUNC_ITER_ADD_STMT_ITERS);

  funit_link* child;   /* Pointer to child functional unit */
  func_unit*  parent;  /* Pointer to parent module of this functional unit */
  int         i;       /* Loop iterator */

  /* First, shift all current statement iterators down by one */
  for( i=(fi->scopes - 2); i>=0; i-- ) {
    fi->sis[i+1] = fi->sis[i];
  }

  /* Now allocate a new statement iterator at position 0 and point it at the current functional unit statement list */
  fi->sis[0] = (stmt_iter*)malloc_safe( sizeof( stmt_iter ) );

  /* Clear the added bits in the statements */
  if( use_tail ) {
    stmt_iter_reset( fi->sis[0], funit->stmt_tail );
  } else {
    stmt_iter_reset( fi->sis[0], funit->stmt_head );
  }
  stmt_iter_find_head( fi->sis[0], FALSE );

  /* Increment the si_num */
  (fi->si_num)++;

  /* Now sort the new statement iterator */
  func_iter_sort( fi );

  /* Get the parent module */
  parent = funit_get_curr_module( funit );

  /* Now traverse down all of the child functional units doing the same */
  child = parent->tf_head;
  while( child != NULL ) {
    if( (inc_all || funit_is_unnamed( child->funit )) && (child->funit->parent == funit) ) {
      func_iter_add_stmt_iters( fi, child->funit, use_tail, inc_all );
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
  fi->sigs[fi->sig_num] = funit->sig_head;
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
  func_iter* fi,        /*!< Pointer to functional unit iterator to initializes */
  func_unit* funit,     /*!< Pointer to main functional unit to create iterator for (must be named) */
  bool       stmts,     /*!< Set to TRUE if we want statements to be included in the iterator */
  bool       sigs,      /*!< Set to TRUE if we want signals to be included in the iterator */
  bool       use_tail,  /*!< Set to TRUE to use statement tail (in reporting mode); otherwise, use head */
  bool       inc_all    /*!< Set to TRUE to use all statements/signals including those in named scopes */
) { PROFILE(FUNC_ITER_INIT);

  assert( fi != NULL );
  assert( funit != NULL );

  /* Count the number of scopes that are within the functional unit iterator */
  fi->scopes  = func_iter_count_scopes( funit, inc_all );
  fi->sis     = NULL;
  fi->sigs    = NULL;
  fi->sig_num = 0;

  /* Add statement iterators */
  if( stmts ) {
    fi->sis    = (stmt_iter**)malloc_safe( sizeof( stmt_iter* ) * fi->scopes );
    fi->si_num = 0;
    func_iter_add_stmt_iters( fi, funit, use_tail, inc_all );
  }

  /* Add signal lists */
  if( sigs ) {
    fi->sigs      = (sig_link**)malloc_safe( sizeof( sig_link* ) * fi->scopes );
    fi->sig_num   = 0;
    func_iter_add_sig_links( fi, funit, inc_all );
    fi->sig_num   = 0;
    fi->curr_sigl = fi->sigs[0];
  }

  PROFILE_END;

}

/*!
 Clears the added bits of all statements in the statement link array.
*/
void func_iter_reset(
  func_iter* fi  /*!< Pointer to functional unit to reset */
) { PROFILE(FUNC_ITER_RESET);

  unsigned int i;
  stmt_iter    si;

  for( i=0; i<fi->si_num; i++ ) {
    stmt_iter_copy( &si, fi->sis[i] );
    while( si.curr != NULL ) {
      si.curr->stmt->suppl.part.added = 0;
      stmt_iter_next( &si );
    }
    stmt_iter_copy( &si, fi->sis[i] );
    stmt_iter_reverse( &si );
    if( si.curr != NULL ) {
      while( si.curr != NULL ) {
        si.curr->stmt->suppl.part.added = 0;
        stmt_iter_next( &si );
      }
    }
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

  if( fi->si_num == 0 ) {

    stmt = NULL;

  } else {

    assert( fi->sis[0]->curr != NULL );

    /* Get the statement at the head of the sorted list */
    stmt = fi->sis[0]->curr->stmt;

    /* Go to the next statement in the current statement list */
    stmt_iter_get_next_in_order( fi->sis[0] );

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

  if( fi->curr_sigl != NULL ) {

    sig           = fi->curr_sigl->sig;
    fi->curr_sigl = fi->curr_sigl->next;

  } else {

    do {
      fi->sig_num++;
    } while( (fi->sig_num < fi->scopes) && (fi->sigs[fi->sig_num] == NULL) );

    if( fi->sig_num < fi->scopes ) {
      sig           = fi->sigs[fi->sig_num]->sig;
      fi->curr_sigl = fi->sigs[fi->sig_num]->next;
    } else {
      sig           = NULL;
      fi->curr_sigl = NULL;
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
    if( fi->sis != NULL ) {

      /* Deallocate all statement iterators */
      for( i=0; i<fi->scopes; i++ ) {
        free_safe( fi->sis[i], sizeof( stmt_iter ) );
      }

      /* Deallocate array of statement iterators */
      free_safe( fi->sis, (sizeof( stmt_iter* ) * fi->scopes) );

      fi->sis = NULL;

    }

    /* Deallocate signal array */
    if( fi->sigs != NULL ) {

      /* Deallocate array of signals */
      free_safe( fi->sigs, (sizeof( sig_link* ) * fi->scopes) );

      fi->sigs = NULL;

    }

  }

  PROFILE_END;
  
}

/*
 $Log$
 Revision 1.19  2008/12/05 04:39:14  phase1geo
 Checkpointing.  Updating regressions.

 Revision 1.18  2008/12/05 00:22:41  phase1geo
 More work completed on code coverage generator.  Currently working on bug in
 statement finder.  Checkpointing.

 Revision 1.17  2008/12/04 14:19:50  phase1geo
 Fixing bug in code generator.  Checkpointing.

 Revision 1.16  2008/12/04 07:14:38  phase1geo
 Doing more work on the code generator to handle combination logic output.
 Still more coding and debugging work to do here.  Need to clear the added
 bit in the statement lists to get current code working correctly.  Checkpointing.

 Revision 1.15  2008/11/29 04:27:07  phase1geo
 More work on inlined coverage code insertion.  Net assigns and procedural assigns
 seem to be working at a most basic level.  Currently, I have an issue that I need
 to solve where non-head statements are being ignored.  Checkpointing.

 Revision 1.14  2008/11/27 00:24:44  phase1geo
 Fixing problems with previous version of generator.  Things work as expected at this point.
 Checkpointing.

 Revision 1.13  2008/08/23 20:00:29  phase1geo
 Full fix for bug 2054686.  Also cleaned up Cver regressions.

 Revision 1.12  2008/08/22 20:56:35  phase1geo
 Starting to make updates for proper unnamed scope report handling (fix for bug 2054686).
 Not complete yet.  Also making updates to documentation.  Checkpointing.

 Revision 1.11  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.7.4.1  2008/07/10 22:43:51  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.9  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.8  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.7  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.6  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.5  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.4  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.3  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.2  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.1  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

*/

