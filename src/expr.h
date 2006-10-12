#ifndef __EXPR_H__
#define __EXPR_H__

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
 \file     expr.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains functions for handling expressions.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Creates an expression value and initializes it. */
void expression_create_value( expression* exp, int width, bool data );

/*! \brief Creates new expression. */
expression* expression_create( expression* right, expression* left, exp_op_type op, bool lhs, int id, int line, int first, int last, bool data );

/*! \brief Sets the specified expression value to the specified vector value. */
void expression_set_value( expression* exp, vsignal* sig );

/*! \brief Sets the signed bit for all appropriate parent expressions */
void expression_set_signed( expression* exp );

/*! \brief Recursively resizes specified expression tree leaf node. */
void expression_resize( expression* expr, bool recursive );

/*! \brief Returns expression ID of this expression. */
int expression_get_id( expression* expr, bool parse_mode );

/*! \brief Returns first line in this expression tree. */
expression* expression_get_first_line_expr( expression* expr );

/*! \brief Returns last line in this expression tree. */
expression* expression_get_last_line_expr( expression* expr );

/*! \brief Returns the current dimension of the given expression. */
int expression_get_curr_dimension( expression* expr );

/*! \brief Finds all RHS signals in given expression tree */
void expression_find_rhs_sigs( expression* expr, str_link** head, str_link** tail );

/*! \brief Finds all parameter expressions in the given expression tree */
void expression_find_params( expression* expr, exp_link** head, exp_link** tail );

/*! \brief Finds the expression in this expression tree with the specified underline id. */
expression* expression_find_uline_id( expression* expr, int ulid );

/*! \brief Returns TRUE if the specified expression exists within the given root expression tree */
bool expression_find_expr( expression* root, expression* expr );

/*! \brief Searches for an expression that calls the given statement */
bool expression_contains_expr_calling_stmt( expression* expr, statement* stmt );

/*! \brief Finds the root statement for the given expression */
statement* expression_get_root_statement( expression* exp );

/*! \brief Assigns each expression in the given tree a unique identifier */
void expression_assign_expr_ids( expression* root );

/*! \brief Writes this expression to the specified database file. */
void expression_db_write( expression* expr, FILE* file, bool parse_mode );

/*! \brief Writes the entire expression tree to the specified data file. */
void expression_db_write_tree( expression* root, FILE* file );

/*! \brief Reads current line of specified file and parses for expression information. */
bool expression_db_read( char** line, func_unit* curr_mod, bool eval );

/*! \brief Reads and merges two expressions and stores result in base expression. */
bool expression_db_merge( expression* base, char** line, bool same );

/*! \brief Returns user-readable name of specified expression operation. */
const char* expression_string_op( int op );

/*! \brief Returns user-readable version of the supplied expression. */
char* expression_string( expression* exp );

/*! \brief Displays the specified expression information. */
void expression_display( expression* expr );

/*! \brief Performs operation specified by parameter expression. */
bool expression_operate( expression* expr, thread* thr );

/*! \brief Performs recursive expression operation (parse mode only). */
void expression_operate_recursively( expression* expr, bool sizing );

/*! \brief Returns TRUE if specified expression is found to contain all static leaf expressions. */
bool expression_is_static_only( expression* expr );

/*! \brief Returns TRUE if specified expression is on the LHS of a blocking assignment operator. */
bool expression_is_assigned( expression* expr );

/*! \brief Returns TRUE if specified expression is a part of an bit select expression tree. */
bool expression_is_bit_select( expression* expr );

/*! \brief Returns TRUE if specified expression is in an RASSIGN expression tree. */
bool expression_is_in_rassign( expression* expr );

/*! \brief Returns TRUE if specified expression is the last select of a signal */
bool expression_is_last_select( expression* expr );

/*! \brief Sets the expression signal supplemental field assigned bit if the given expression is an RHS of an assignment */
void expression_set_assigned( expression* expr );

/*! \brief Performs blocking assignment assignment to variables. */
void expression_assign( expression* lhs, expression* rhs, int* lsb );

/*! \brief Deallocates memory used for expression. */
void expression_dealloc( expression* expr, bool exp_only );


/*
 $Log$
 Revision 1.51  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.50  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.49  2006/09/08 22:39:50  phase1geo
 Fixes for memory problems.

 Revision 1.48  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.47  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.46  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.45  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.44  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.43  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.42  2006/03/20 16:43:38  phase1geo
 Fixing code generator to properly display expressions based on lines.  Regression
 still needs to be updated for these changes.

 Revision 1.41  2006/02/06 22:48:34  phase1geo
 Several enhancements to GUI look and feel.  Fixed error in combinational logic
 window.

 Revision 1.40  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.39  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.38  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.37  2006/01/09 04:15:25  phase1geo
 Attempting to fix one last problem with latest changes.  Regression runs are
 currently running.  Checkpointing.

 Revision 1.36  2006/01/06 18:54:03  phase1geo
 Breaking up expression_operate function into individual functions for each
 expression operation.  Also storing additional information in a globally accessible,
 constant structure array to increase performance.  Updating full regression for these
 changes.  Full regression passes.

 Revision 1.35  2005/12/23 05:41:52  phase1geo
 Fixing several bugs in score command per bug report #1388339.  Fixed problem
 with race condition checker statement iterator to eliminate infinite looping (this
 was the problem in the original bug).  Also fixed expression assigment when static
 expressions are used in the LHS (caused an assertion failure).  Also fixed the race
 condition checker to properly pay attention to task calls, named blocks and fork
 statements to make sure that these are being handled correctly for race condition
 checking.  Fixed bug for signals that are on the LHS side of an assignment expression
 but is not being assigned (bit selects) so that these are NOT considered for race
 conditions.  Full regression is a bit broken now but the opened bug can now be closed.

 Revision 1.34  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.33  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.32  2005/11/17 23:35:16  phase1geo
 Blocking assignment is now working properly along with support for event expressions
 (currently only the original PEDGE, NEDGE, AEDGE and DELAY are supported but more
 can now follow).  Added new race4 diagnostic to verify that a register cannot be
 assigned from more than one location -- this works.  Regression fails at this point.

 Revision 1.31  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.30  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.29  2005/02/08 23:18:23  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.28  2004/10/22 21:40:30  phase1geo
 More incremental updates to improve efficiency in score command (though this
 change should not, in and of itself, improve efficiency).

 Revision 1.27  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.26  2004/04/19 04:54:56  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.25  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.24  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.23  2003/11/30 21:50:45  phase1geo
 Modifying line_collect_uncovered function to create array containing all physical
 lines (rather than just uncovered statement starting line values) for more
 accurate line coverage results for the GUI.  Added new long_exp2 diagnostic that
 is used to test this functionality.

 Revision 1.22  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.21  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.20  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.19  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.18  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.17  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.16  2002/10/31 23:13:47  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.15  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.14  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.13  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.12  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.11  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.10  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.9  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.8  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.7  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.6  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.5  2002/06/21 05:55:05  phase1geo
 Getting some codes ready for writing simulation engine.  We should be set
 now.

 Revision 1.4  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.3  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

#endif

