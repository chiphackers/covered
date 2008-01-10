#ifndef __FUNC_UNIT_H__
#define __FUNC_UNIT_H__

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
 \file     func_unit.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/7/2001
 \brief    Contains functions for handling functional units.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Creates new functional unit from heap and initializes structure. */
func_unit* funit_create();

/*! \brief Returns the parent module of the given functional unit. */
func_unit* funit_get_curr_module( func_unit* funit );

/*! \brief Returns the parent module of the given functional unit (returning a const version). */
const func_unit* funit_get_curr_module_safe( const func_unit* funit );

/*! \brief Returns the parent function of the given functional unit (if there is one) */
func_unit* funit_get_curr_function( func_unit* funit );

/*! \brief Returns the parent task of the given functional unit (if there is one) */
func_unit* funit_get_curr_task( func_unit* funit );

/*! \brief Returns the number of input, output and inout ports in the specified functional unit */
int funit_get_port_count( func_unit* funit );

/*! \brief Finds specified module parameter given the current functional unit and its scope */
mod_parm* funit_find_param( char* name, func_unit* funit );

/*! \brief Finds specified signal given in the current functional unit */
vsignal* funit_find_signal( char* name, func_unit* funit );

/*! \brief Finds all expressions that call the given statement */
void funit_remove_stmt_blks_calling_stmt( func_unit* funit, statement* stmt );

/*! \brief Generates the internally used task/function/named-block name for the specified functional unit */
char* funit_gen_task_function_namedblock_name( char* orig_name, func_unit* parent );

/*! \brief Sizes all elements for the current functional unit from the given instance */
void funit_size_elements( func_unit* funit, funit_inst* inst, bool gen_all, bool alloc_exprs );

/*! \brief Writes contents of provided functional unit to specified output. */
bool funit_db_write( func_unit* funit, char* scope, FILE* file, funit_inst* inst, bool report_save );

/*! \brief Read contents of current line from specified file, creates functional unit
           and adds to functional unit list. */
bool funit_db_read( func_unit* funit, /*@out@*/char* scope, char** line );

/*! \brief Reads and merges two functional units into base functional unit. */
bool funit_db_merge( func_unit* base, FILE* file, bool same );

/*! \brief Flattens the functional unit name by removing all unnamed scope portions */
/*@shared@*/ char* funit_flatten_name( func_unit* funit );

/*! \brief Finds the functional unit that contains the given statement/expression ID */
func_unit* funit_find_by_id( int id );

/*! \brief Returns TRUE if the given functional unit does not contain any input, output or inout ports. */
bool funit_is_top_module( func_unit* funit );

/*! \brief Returns TRUE if the given functional unit is an unnamed scope. */
bool funit_is_unnamed( func_unit* funit );

/*! \brief Returns TRUE if the specified "parent" functional unit is a parent of the "child" functional unit */
bool funit_is_unnamed_child_of( func_unit* parent, func_unit* child );

/*! \brief Returns TRUE if the specified "parent" functional unit is a parent of the "child" functional unit */
bool funit_is_child_of( func_unit* parent, func_unit* child );

/*! \brief Displays signals stored in this functional unit. */
void funit_display_signals( func_unit* funit );

/*! \brief Displays expressions stored in this functional unit. */
void funit_display_expressions( func_unit* funit );

/*! \brief Deallocates functional unit element from heap. */
void funit_dealloc( func_unit* funit );


/*
 $Log$
 Revision 1.31  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.30  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.29  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.28  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.27  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.26  2007/04/18 22:35:02  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.25  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.24  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.23  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.22  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.21  2007/03/19 20:30:31  phase1geo
 More fixes to report command for instance flattening.  This seems to be
 working now as far as I can tell.  Regressions still have about 8 diagnostics
 failing with report errors.  Checkpointing.

 Revision 1.20  2007/03/19 03:30:16  phase1geo
 More fixes to instance flattening algorithm.  Still much more work to do here.
 Checkpointing.

 Revision 1.19  2007/03/16 21:41:09  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.18  2006/12/19 05:23:39  phase1geo
 Added initial code for handling instance flattening for unnamed scopes.  This
 is partially working at this point but still needs some debugging.  Checkpointing.

 Revision 1.17  2006/11/03 23:36:36  phase1geo
 Fixing bug 1590104.  Updating regressions per this change.

 Revision 1.16  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.15  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.14  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.13  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.12  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.11  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.10  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.9  2006/01/23 03:53:30  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.8  2006/01/20 19:15:23  phase1geo
 Fixed bug to properly handle the scoping of parameters when parameters are created/used
 in non-module functional units.  Added param10*.v diagnostics to regression suite to
 verify the behavior is correct now.

 Revision 1.7  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.6  2005/12/01 21:11:16  phase1geo
 Adding more error checking diagnostics into regression suite.  Full regression
 passes.

 Revision 1.5  2005/12/01 20:49:02  phase1geo
 Adding nested_block3 to verify nested named blocks in tasks.  Fixed named block
 usage to be FUNC_CALL or TASK_CALL -like based on its placement.

 Revision 1.4  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.3  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.2  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.1  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.10  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.9  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.8  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.7  2002/10/31 23:14:00  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.4  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.3  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

