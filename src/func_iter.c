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
#include "func_iter.h"
#include "func_unit.h"
#include "util.h"


void func_iter_display( func_iter* fi ) {

  int i;  /* Loop iterator */

  printf( "Functional unit iterator:\n" );

  for( i=0; i<fi->si_num; i++ ) {
    printf( "  Line: %d\n", fi->sis[i]->curr->stmt->exp->line );
  }

}

/*!
 \param fi  Pointer to functional unit iterator to sort

 Performs a bubble sort of the first element such that the first line is in location 0 of the sis array.
*/
void func_iter_sort( func_iter* fi ) {

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

  /* Otherwise, resort them based on line number */
  } else {

    i = 0;
    while( (i < (fi->si_num - 1)) && (tmp->curr->stmt->exp->line > fi->sis[i+1]->curr->stmt->exp->line) ) {
      fi->sis[i] = fi->sis[i+1];
      i++;
    }
    fi->sis[i] = tmp;

  }

  // func_iter_display( fi );

}

/*!
 \param funit  Pointer to current functional unit being examined

 \return Returns the number of statement iterators found in all of the unnamed functional units
         within a named functional unit.
*/
int func_iter_count_stmt_iters( func_unit* funit ) {

  int         count = 1;  /* Number of statement iterators within this functional unit */
  funit_link* child;      /* Pointer to child functional unit */
  func_unit*  parent;     /* Parent module of this functional unit */

  assert( funit != NULL );

  /* Get parent module */
  parent = funit_get_curr_module( funit );

  /* Iterate through children functional units, counting all of the unnamed scopes */
  child = parent->tf_head;
  while( child != NULL ) {
    if( funit_is_unnamed( child->funit ) && (child->funit->parent == funit) ) {
      count += func_iter_count_stmt_iters( child->funit );
    }
    child = child->next;
  }

  return( count );

}

void func_iter_add_stmt_iters( func_iter* fi, func_unit* funit ) {

  funit_link* child;   /* Pointer to child functional unit */
  func_unit*  parent;  /* Pointer to parent module of this functional unit */
  int         i;       /* Loop iterator */

  /* First, shift all current statement iterators down by one */
  for( i=(fi->si_num - 1); i>=0; i-- ) {
    fi->sis[i+1] = fi->sis[i];
  }

  /* Now allocate a new statement iterator at position 0 and point it at the current functional unit statement list */
  fi->sis[0] = (stmt_iter*)malloc_safe( sizeof( stmt_iter ), __FILE__, __LINE__ );
  stmt_iter_reset( fi->sis[0], funit->stmt_tail );
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
    if( funit_is_unnamed( child->funit ) && (child->funit->parent == funit) ) {
      func_iter_add_stmt_iters( fi, child->funit );
    }
    child = child->next;
  }

}

/*!
 \param fi     Pointer to functional unit iterator structure to populate
 \param funit  Pointer to main functional unit to create iterator for (must be named)
*/
void func_iter_init( func_iter* fi, func_unit* funit ) {
  
  assert( fi != NULL );
  assert( funit != NULL );

  /* Allocate array of statement iterators for the current functional unit */
  fi->sis    = (stmt_iter**)malloc_safe( (sizeof( func_iter* ) * func_iter_count_stmt_iters( funit )), __FILE__, __LINE__ );
  fi->si_num = 0;

  /* Create statement iterators */
  func_iter_add_stmt_iters( fi, funit );
  
}

/*!
 \param fi  Pointer to functional unit iterator to use

 \return Returns pointer to next statement in line order (or NULL if there are no more
         statements in the given functional unit)
*/
statement* func_iter_get_next_statement( func_iter* fi ) {

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

  return( stmt );

}

/*!
 \param fi  Pointer to functional unit iterator to deallocate
 
 Deallocates all allocated memory for the given functional unit iterator.
*/
void func_iter_dealloc( func_iter* fi ) {

  int i;  /* Loop iterator */
  
  if( fi != NULL ) {

    /* Deallocate all statement iterators */
    for( i=0; i<(sizeof( fi->sis ) / sizeof( stmt_iter )); i++ ) {
      free_safe( fi->sis[i] );
    }

    /* Deallocate array of statement iterators */
    free_safe( fi->sis );

  }
  
}

/*
 $Log$
 Revision 1.2  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.1  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

*/

