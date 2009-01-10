#ifndef __GEN_ITEM_H__
#define __GEN_ITEM_H__

/*
 Copyright (c) 2006-2009 Trevor Williams

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
 \file     gen_item.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     7/10/2006
 \brief    Contains functions for handling generate items.
*/


#include <stdio.h>

#include "defines.h"


/*@-exportlocal@*/
/*! \brief Displays the specified generate item to standard output */
void gen_item_display( gen_item* gi );
/*@=exportlocal@*/

/*! \brief Displays an entire generate block */
void gen_item_display_block( gen_item* root );

/*! \brief Searches for a generate item in the generate block of root that matches gi */
gen_item* gen_item_find( gen_item* root, gen_item* gi );

/*! \brief Searches for an expression in the generate list that calls the given statement */
void gen_item_remove_if_contains_expr_calling_stmt( gen_item* gi, statement* stmt );

/*! \brief Returns TRUE if the specified variable name contains a generate variable within it */
bool gen_item_varname_contains_genvar( char* name );

/*! \brief Returns the actual signal name specified by the given signal name which references a
           generated hierarchy */
char* gen_item_calc_signal_name( const char* name, func_unit* funit, int line, bool no_genvars );

/*! \brief Creates a generate item for an expression */
gen_item* gen_item_create_expr( expression* expr );

/*! \brief Creates a generate item for a signal */
gen_item* gen_item_create_sig( vsignal* sig );

/*! \brief Creates a generate item for a statement */
gen_item* gen_item_create_stmt( statement* stmt );

/*! \brief Creates a generate item for an instance */
gen_item* gen_item_create_inst( funit_inst* inst );

/*! \brief Creates a generate item for a namespace */
gen_item* gen_item_create_tfn( funit_inst* inst );

/*! \brief Creates a generate item for a binding */
gen_item* gen_item_create_bind( const char* name, expression* expr );

/*! \brief Resizes all statements and signals in the given generate item block */
void gen_item_resize_stmts_and_sigs( gen_item* gi, func_unit* funit );

/*! \brief Assigns unique signal ID or expression IDs to all expressions for specified statement block */
void gen_item_assign_ids( gen_item* gi, func_unit* funit );

/*! \brief Outputs the current generate item to the given output file if it matches the type specified */
void gen_item_db_write( gen_item* gi, uint32 type, FILE* file );

/*! \brief Outputs the entire expression tree from the given generate statement */
void gen_item_db_write_expr_tree( gen_item* gi, FILE* file );

/*! \brief Connects a generate item block to a new generate item */
bool gen_item_connect( gen_item* gi1, gen_item* gi2, int conn_id );

/*! \brief Checks generate item and if it is a bind, adds it to binding pool and returns TRUE */
void gen_item_bind( gen_item* gi );

/*! \brief Resolves all generate items in the design */
void generate_resolve( funit_inst* inst );

/*! \brief Removes the given generate statement from the correct instance */
bool generate_remove_stmt( statement* stmt );

/*! \brief Searches the generate statements within the given functional unit for a statement that matches the given positional information. */
statement* generate_find_stmt_by_position(
  func_unit*   funit,
  unsigned int first_line,
  unsigned int first_col
);

/*! \brief Deallocates all associated memory for the given generate item */
void gen_item_dealloc( gen_item* gi, bool rm_elem );

/*
 $Log$
 Revision 1.28  2009/01/09 21:25:00  phase1geo
 More generate block fixes.  Updated all copyright information source code files
 for the year 2009.  Checkpointing.

 Revision 1.27  2008/08/27 23:06:00  phase1geo
 Starting to make updates for supporting command-line exclusions.  Signals now
 have a unique ID associated with them in the CDD file.  Checkpointing.

 Revision 1.26  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.23.6.1  2008/07/10 22:43:51  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.24  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.23  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.22  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.21  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.20  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.19  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.18  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.17  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.16  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.15  2006/09/05 21:00:45  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.14  2006/08/25 22:49:45  phase1geo
 Adding support for handling generated hierarchical names in signals that are outside
 of generate blocks.  Added support for op-and-assigns in generate for loops as well
 as normal for loops.  Added generate11.4 and for3 diagnostics to regression suite
 to verify this new behavior.  Full regressions have not been verified with these
 changes however.  Checkpointing.

 Revision 1.13  2006/08/14 04:19:56  phase1geo
 Fixing problem with generate11* diagnostics (generate variable used in
 signal name).  These tests pass now but full regression hasn't been verified
 at this point.

 Revision 1.12  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.11  2006/07/29 20:53:43  phase1geo
 Fixing some code related to generate statements; however, generate8.1 is still
 not completely working at this point.  Full regression passes for IV.

 Revision 1.10  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.9  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.8  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.7  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.6  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.5  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.4  2006/07/20 04:55:18  phase1geo
 More updates to support generate blocks.  We seem to be passing the parser
 stage now.  Getting segfaults in the generate_resolve code, presently.

 Revision 1.3  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.2  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.1  2006/07/10 22:37:14  phase1geo
 Missed the actual gen_item files in the last submission.

*/

#endif
