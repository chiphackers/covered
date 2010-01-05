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
 \file     stmt_blk.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/23/2005
*/


#include <assert.h>

#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "link.h"
#include "obfuscate.h"
#include "profiler.h"
#include "statement.h"
#include "stmt_blk.h"


/*! Pointer to head of statement block list to remove */
static stmt_link* rm_stmt_head    = NULL;

/*! Pointer to tail of statement block list to remove */
static stmt_link* rm_stmt_tail    = NULL;

/*! Array containing reasons for logic block removal */
static const char* logic_rm_msgs[LOGIC_RM_NUM] = {
  "it contains a real number (currently unsupported)",
  "it contains an unsupported system function",
  "it contains an unsupported system task"
};


extern func_unit* curr_funit;
extern char       user_msg[USER_MSG_LENGTH];


/*!
 Adds the statement block containing the specified statement to the list of statement
 blocks to remove after parsing, binding and race condition checking has occurred.
*/
void stmt_blk_add_to_remove_list(
  statement* stmt  /*!< Pointer to statement in a statement block that needs to be removed */
) { PROFILE(STMT_BLK_ADD_TO_REMOVE_LIST);

  func_unit* funit;  /* Pointer to functional unit containing this statement */

  assert( stmt != NULL );

#ifndef VPI_ONLY
  if( !generate_remove_stmt( stmt ) ) {
#endif

    /* If this is a head statement, don't bother looking this up again */
    if( stmt->suppl.part.head == 0 ) {

      /* Find the functional unit that contains this statement */
      funit = funit_find_by_id( stmt->exp->id );
      assert( funit != NULL );

      /* Find the head statement of the statement block that contains this statement */
      stmt = stmt->head;

    }

    /* If this statement has not been added to the removal list already, do so now */
    if( stmt_link_find( stmt->exp->id, rm_stmt_head ) == NULL ) {
      stmt_link_add( stmt, TRUE, &rm_stmt_head, &rm_stmt_tail );
    }

#ifndef VPI_ONLY
  }
#endif

  PROFILE_END;

}

/*!
 Iterates through rm_stmt list, deallocating each statement block in that list.
 This function is only called once after the parsing, binding and race condition
 checking phases have completed.
*/
void stmt_blk_remove() { PROFILE(STMT_BLK_REMOVE);

  statement* stmt;      /* Temporary pointer to current statement to deallocate */

  /* Remove all statement blocks */
  while( rm_stmt_head != NULL ) {
    stmt = rm_stmt_head->stmt;
    stmt_link_unlink( stmt, &rm_stmt_head, &rm_stmt_tail );
    curr_funit = funit_find_by_id( stmt->exp->id );
    assert( curr_funit != NULL );
    /*
     If we are removing the statement contained in a task, function or named block, we need to remove all statement
     blocks that contain expressions that call this task, function or named block.
    */
    if( (curr_funit->suppl.part.type == FUNIT_FUNCTION)  || (curr_funit->suppl.part.type == FUNIT_TASK)  || (curr_funit->suppl.part.type == FUNIT_NAMED_BLOCK) ||
        (curr_funit->suppl.part.type == FUNIT_AFUNCTION) || (curr_funit->suppl.part.type == FUNIT_ATASK) || (curr_funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {
      curr_funit->suppl.part.type = FUNIT_NO_SCORE;
      db_remove_stmt_blks_calling_statement( stmt );
    }
    /* Deallocate the statement block now */
    statement_dealloc_recursive( stmt, TRUE );
  }

  PROFILE_END;

}

/*!
 Outputs the reason why a logic block is being removed from coverage consideration.
*/
void stmt_blk_specify_removal_reason(
  logic_rm_type type,   /*!< Reason for removing the logic block */
  const char*   file,   /*!< Filename containing logic block being removed */
  int           line,   /*!< Line containing logic that is causing logic block removal */
  const char*   cfile,  /*!< File containing removal line */
  int           cline   /*!< Line containing removal line */
) { PROFILE(STMT_BLK_SPECIFY_REMOVAL_REASON);

  unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Removing logic block containing line %d in file %s because", line, file );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, WARNING, cfile, cline );
  print_output( logic_rm_msgs[type], WARNING_WRAP, cfile, cline );

  PROFILE_END;

}

