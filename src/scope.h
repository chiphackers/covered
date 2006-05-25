#ifndef __SCOPE_H__
#define __SCOPE_H__

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
 \file     scope.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/10/2005
 \brief    Contains functions for looking up structures for the given scope
*/


#include "defines.h"


/*! \brief  Finds the functional unit that corresponds to the given scope relative to the current
            functional unit */
func_unit* scope_find_funit_from_scope( char* scope, func_unit* curr_funit );

/*! \brief  Find the given signal in the provided scope */
bool scope_find_param( char* name, func_unit* curr_funit, mod_parm** found_parm, func_unit** found_funit, int line );

/*! \brief  Find the given signal in the provided scope */
bool scope_find_signal( char* name, func_unit* curr_funit, vsignal** found_sig, func_unit** found_funit, int line );

/*! \brief  Finds the given task or function in the provided scope. */
bool scope_find_task_function_namedblock( char* name, int type, func_unit* curr_funit, func_unit** found_funit, int line );

/*! \brief  Finds the parent functional unit of the functional unit with the given scope */
func_unit* scope_get_parent_funit( char* scope );

/*! \brief  Finds the parent module of the functional unit with the given scope */
func_unit* scope_get_parent_module( char* scope );


/*
 $Log$
 Revision 1.7.8.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.7  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.6  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.5  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.4  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.3  2005/11/16 05:41:31  phase1geo
 Fixing implicit signal creation in binding functions.

 Revision 1.2  2005/11/11 22:53:40  phase1geo
 Updated bind process to allow binding of structures from different hierarchies.
 Added task port signals to get added.

 Revision 1.1  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.
*/

#endif

