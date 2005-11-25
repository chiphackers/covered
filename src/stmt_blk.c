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

  func_unit* funit;  /* Functional unit containing the specified statement */
  int        i;      /* Loop iterator */

  assert( stmt != NULL );

  /* Find the functional unit that contains this statement */
  funit = funit_find_by_id( stmt->exp->id );

  assert( funit != NULL );

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
*/

