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
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
 \brief    Contains functions for handling functional units.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Initializes all values of functional_unit. */
void funit_init( func_unit* mod );

/*! \brief Creates new functional unit from heap and initializes structure. */
func_unit* funit_create();

/*! \brief Returns the parent module of the given functional unit. */
func_unit* funit_get_curr_module( func_unit* funit );

/*! \brief Returns the parent function of the given functional unit (if there is one) */
func_unit* funit_get_curr_function( func_unit* funit );

/*! \brief Returns the number of input, output and inout ports in the specified functional unit */
int funit_get_port_count( func_unit* funit );

/*! \brief Finds specified module parameter given the current functional unit and its scope */
mod_parm* funit_find_param( char* name, func_unit* funit );

/*! \brief Generates the internally used task/function/named-block name for the specified functional unit */
char* funit_gen_task_function_namedblock_name( char* orig_name, func_unit* parent );

/*! \brief Writes contents of provided functional unit to specified output. */
bool funit_db_write( func_unit* funit, char* scope, FILE* file, funit_inst* inst );

/*! \brief Read contents of current line from specified file, creates functional unit
           and adds to functional unit list. */
bool funit_db_read( func_unit* funit, char* scope, char** line );

/*! \brief Reads and merges two functional units into base functional unit. */
bool funit_db_merge( func_unit* base, FILE* file, bool same );

/*! \brief Reads and replaces original functional unit with contents of new functional unit. */
bool funit_db_replace( func_unit* base, FILE* file );

/*! \brief Finds the functional unit that contains the given statement/expression ID */
func_unit* funit_find_by_id( int id );

/*! \brief Displays signals stored in this functional unit. */
void funit_display_signals( func_unit* funit );

/*! \brief Displays expressions stored in this functional unit. */
void funit_display_expressions( func_unit* funit );

/*! \brief Deallocates functional unit element contents only from heap. */
void funit_clean( func_unit* funit );

/*! \brief Deallocates functional unit element from heap. */
void funit_dealloc( func_unit* funit );


/*
 $Log$
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

