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
 \file     stmt_blk.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/23/2005
*/


#include <assert.h>

#include "defines.h"
#include "stmt_blk.h"
#include "link.h"
#include "func_unit.h"
#include "statement.h"
#include "db.h"
#include "expr.h"


/*! Pointer to head of statement block list to remove */
stmt_link* rm_stmt_head    = NULL;

/*! Pointer to tail of statement block list to remove */
stmt_link* rm_stmt_tail    = NULL;

/*! Array containing statement IDs that have been listed for removal */
int*       rm_stmt_ids;

/*! Size indicator of rm_stmt_ids array */
int        rm_stmt_id_size = 0;

extern func_unit* curr_funit;


/*!
 \param stmt  Pointer to statement in a statement block that needs to be removed

 Adds the statement block containing the specified statement to the list of statement
 blocks to remove after parsing, binding and race condition checking has occurred.
*/
void stmt_blk_add_to_remove_list( statement* stmt ) {

  func_unit* funit;     /* Functional unit containing the specified statement */
  int        i;         /* Loop iterator */
  exp_link*  exp_head;  /* Head of expression list containing expressions that call this statement */
  exp_link*  expl;      /* Pointer to current expression link being examined */
  statement* tmp_stmt;  /* Temporary pointer to root statement */

  assert( stmt != NULL );

  /* Find the functional unit that contains this statement */
  funit = funit_find_by_id( stmt->exp->id );

  assert( funit != NULL );

  /*
   If we are removing the statement contained in a task or function, we need to remove all statement
   blocks that contain expressions that call this task or function.
  */
  if( (funit->type == FUNIT_FUNCTION) || (funit->type == FUNIT_TASK) ) {
    // printf( "Searching for all expressions that call %s...\n", funit->name );
    if( (exp_head = db_get_exprs_with_statement( stmt )) != NULL ) {
      expl = exp_head;
      while( expl != NULL ) {
        if( (tmp_stmt = expression_get_root_statement( expl->exp )) != NULL ) {
          stmt_blk_add_to_remove_list( tmp_stmt );
        }
        expl = expl->next;
      } 
      exp_link_delete_list( exp_head, FALSE );
    }
  }

  /* Find the head statement of the statement block that contains this statement */
  stmt = statement_find_head_statement( stmt, funit->stmt_head );

  assert( stmt != NULL );

  /* If this statement has not been added to the removal list already, do so now */
  i = 0;
  while( (i < rm_stmt_id_size) && (rm_stmt_ids[i] != stmt->exp->id) ) i++;

  if( i == rm_stmt_id_size ) {
    stmt_link_add_tail( stmt, &rm_stmt_head, &rm_stmt_tail );
    rm_stmt_ids = (int*)realloc( rm_stmt_ids, (sizeof( int ) * (rm_stmt_id_size + 1)) );
    rm_stmt_ids[rm_stmt_id_size] = stmt->exp->id;
    rm_stmt_id_size++;
  }

}

/*!
 Iterates through rm_stmt list, deallocating each statement block in that list, deallocating
 the rm_stmt list itself and the rm_stmt_ids array.  This function is only called once after
 the parsing, binding and race condition checking phases have completed.
*/
void stmt_blk_remove() {

  statement* stmt;  /* Temporary pointer to current statement to deallocate */

  /* Remove all statement blocks */
  while( rm_stmt_head != NULL ) {
    stmt = rm_stmt_head->stmt;
    stmt_link_unlink( stmt, &rm_stmt_head, &rm_stmt_tail );
    curr_funit = funit_find_by_id( stmt->exp->id );
    assert( curr_funit != NULL );
    statement_dealloc_recursive( stmt );
  }

  /* Now deallocate the entire rm_stmt_ids array */
  free_safe( rm_stmt_ids );
  rm_stmt_id_size = 0;

}

/*
 $Log$
 Revision 1.2  2005/11/29 19:04:48  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.1  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

*/

