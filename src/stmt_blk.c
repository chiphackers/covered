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
 \author   Trevor Williams  (phase1geo@gmail.com)
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
#include "gen_item.h"
#include "obfuscate.h"


/*! Pointer to head of statement block list to remove */
stmt_link* rm_stmt_head    = NULL;

/*! Pointer to tail of statement block list to remove */
stmt_link* rm_stmt_tail    = NULL;

extern func_unit* curr_funit;


/*!
 \param stmt  Pointer to statement in a statement block that needs to be removed

 Adds the statement block containing the specified statement to the list of statement
 blocks to remove after parsing, binding and race condition checking has occurred.
*/
void stmt_blk_add_to_remove_list( statement* stmt ) {

  func_unit* funit;  /* Pointer to functional unit containing this statement */

  assert( stmt != NULL );

#ifndef VPI_ONLY
  if( !generate_remove_stmt( stmt ) ) {
#endif

    /* If this is a head statement, don't bother looking this up again */
    if( ESUPPL_IS_STMT_HEAD( stmt->exp->suppl ) == 0 ) {

      /* Find the functional unit that contains this statement */
      funit = funit_find_by_id( stmt->exp->id );
      assert( funit != NULL );

      /* Find the head statement of the statement block that contains this statement */
      stmt = statement_find_head_statement( stmt, funit->stmt_head );

    }

    /* If this statement has not been added to the removal list already, do so now */
    if( stmt_link_find( stmt->exp->id, rm_stmt_head ) == NULL ) {
      stmt_link_add_tail( stmt, &rm_stmt_head, &rm_stmt_tail );
    }

#ifndef VPI_ONLY
  }
#endif

}

/*!
 Iterates through rm_stmt list, deallocating each statement block in that list.
 This function is only called once after the parsing, binding and race condition
 checking phases have completed.
*/
void stmt_blk_remove() {

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
    if( (curr_funit->type == FUNIT_FUNCTION)  || (curr_funit->type == FUNIT_TASK)  || (curr_funit->type == FUNIT_NAMED_BLOCK) ||
        (curr_funit->type == FUNIT_AFUNCTION) || (curr_funit->type == FUNIT_ATASK) || (curr_funit->type == FUNIT_ANAMED_BLOCK) ) {
      curr_funit->type = FUNIT_NO_SCORE;
      db_remove_stmt_blks_calling_statement( stmt );
    }
    /* Deallocate the statement block now */
    statement_dealloc_recursive( stmt );
  }

}

/*
 $Log$
 Revision 1.12  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.11  2007/03/16 22:33:46  phase1geo
 One more fix that helps diagnostics like always1 and still fixes exclude3.
 Regressions are still not working correctly yet, though.

 Revision 1.10  2007/03/16 21:41:10  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.9  2006/10/09 17:54:19  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.8  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.7  2006/09/05 21:00:45  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.6  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.5  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.4  2006/08/06 04:36:20  phase1geo
 Fixing bugs 1533896 and 1533827.  Also added -rI option that will ignore
 the race condition check altogether (has not been verified to this point, however).

 Revision 1.3  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.2  2005/11/29 19:04:48  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.1  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

*/

