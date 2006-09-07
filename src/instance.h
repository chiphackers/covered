#ifndef __INSTANCE_H__
#define __INSTANCE_H__

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
 \file     instance.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/11/2002
 \brief    Contains functions for handling functional unit instances.
*/

#include "defines.h"


/*! \brief Creates a new instance with the given information */
funit_inst* instance_create( func_unit* funit, char* inst_name, vector_width* range );

/*! \brief Displays the current state of the instance tree */
void instance_display_tree( funit_inst* root );

/*! \brief Builds full hierarchy from leaf node to root. */
void instance_gen_scope( char* scope, funit_inst* leaf );

/*! \brief Finds specified scope in functional unit instance tree. */
funit_inst* instance_find_scope( funit_inst* root, char* scope );

/*! \brief Returns instance that points to specified functional unit for each instance. */
funit_inst* instance_find_by_funit( funit_inst* root, func_unit* funit, int* ignore );

/*! \brief Adds an instance to the specified parent instance */
funit_inst* instance_add_child( funit_inst* inst, func_unit* child, char* name, vector_width* range, bool resolve );

/*! \brief Adds new instance to specified instance tree during parse. */
bool instance_parse_add( funit_inst** root, func_unit* parent, func_unit* child, char* inst_name, vector_width* range, bool resolve );

/*! \brief Resolves all instance arrays. */
void instance_resolve( funit_inst* root );

/*! \brief Adds new instance to specified instance tree during CDD read. */
bool instance_read_add( funit_inst** root, char* parent, func_unit* child, char* inst_name );

/*! \brief Displays contents of functional unit instance tree to specified file. */
void instance_db_write( funit_inst* root, FILE* file, char* scope, bool parse_mode, bool report_save );

/*! \brief Removes all statement blocks that contain expressions that call the given statement */
void instance_remove_stmt_blks_calling_stmt( funit_inst* root, statement* stmt );

/*! \brief Removes specified instance from tree. */
void instance_dealloc( funit_inst* root, char* scope );


/*
 $Log$
 Revision 1.18  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.17  2006/07/21 20:12:46  phase1geo
 Fixing code to get generated instances and generated array of instances to
 work.  Added diagnostics to verify correct functionality.  Full regression
 passes.

 Revision 1.16  2006/07/18 13:37:47  phase1geo
 Fixing compile issues.

 Revision 1.15  2006/07/12 22:16:18  phase1geo
 Fixing hierarchical referencing for instance arrays.  Also attempted to fix
 a problem found with unary1; however, the generated report coverage information
 does not look correct at this time.  Checkpointing what I have done for now.

 Revision 1.14  2006/07/11 04:59:08  phase1geo
 Reworking the way that instances are being generated.  This is to fix a bug and
 pave the way for generate loops for instances.  Code not working at this point
 and may cause serious problems for regression runs.

 Revision 1.13  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.12  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.11  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.10  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.9  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.8  2002/10/31 23:13:52  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.7  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.6  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.5  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.4  2002/07/18 05:50:45  phase1geo
 Fixes should be just about complete for instance depth problems now.  Diagnostics
 to help verify instance handling are added to regression.  Full regression passes.

 Revision 1.3  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.
*/

#endif

