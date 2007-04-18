#ifndef __PARAM_H__
#define __PARAM_H__

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
 \file     param.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002
 \brief    Contains functions and structures necessary to handle parameters.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Searches specified module parameter list for matching parameter. */
mod_parm* mod_parm_find( char* name, mod_parm* parm );

/*! \brief Find specified expression and remove if found from module parameter expression lists. */
void mod_parm_find_expr_and_remove( expression* exp, mod_parm* parm );

/*! \brief Creates new module parameter and adds it to the specified list. */
mod_parm* mod_parm_add( char* scope, static_expr* msb, static_expr* lsb, bool is_signed,
                        expression* expr, int type, func_unit* funit, char* inst_name );

/*! \brief Outputs contents of module parameter list to standard output. */
void mod_parm_display( mod_parm* mparm );

/*! \brief Searches specified instance parameter list for matching parameter. */
inst_parm* inst_parm_find( char* name, inst_parm* parm );

/*! \brief Creates and adds new instance parameter to specified instance parameter list. */
inst_parm* inst_parm_add( char* name, char* inst_name, static_expr* msb, static_expr* lsb, bool is_signed,
                          vector* value, mod_parm* mparm, funit_inst* inst );

/*! \brief Creates a new instance parameter for a generate variable */
void inst_parm_add_genvar( vsignal* sig, funit_inst* inst );

/*! \brief Performs bind of signal and expressions for the given instance parameter. */
void inst_parm_bind( inst_parm* iparm );

/*! \brief Adds parameter override to defparam list. */
void defparam_add( char* scope, vector* expr );

/*! \brief Deallocates all memory associated with defparam storage from command-line */
void defparam_dealloc();

/*! \brief Sets the specified signal size according to the specified instance parameter */
void param_set_sig_size( vsignal* sig, inst_parm* icurr );

/*! \brief Evaluates parameter expression for the given instance. */
void param_expr_eval( expression* expr, funit_inst* inst );

/*! \brief Transforms a declared module parameter into an instance parameter. */
void param_resolve_declared( mod_parm* mparm, funit_inst* inst );

/*! \brief Transforms an override module parameter into an instance parameter. */
void param_resolve_override( mod_parm* oparm, funit_inst* inst );

/*! \brief Resolves all parameters for the specified instance. */
void param_resolve( funit_inst* inst );

/*! \brief Outputs specified instance parameter to specified output stream. */
void param_db_write( inst_parm* iparm, FILE* file, bool parse_mode );

/*! \brief Deallocates specified module parameter and possibly entire module parameter list. */
void mod_parm_dealloc( mod_parm* parm, bool recursive );

/*! \brief Deallocates specified instance parameter and possibly entire instance parameter list. */
void inst_parm_dealloc( inst_parm* parm, bool recursive );


/*
 $Log$
 Revision 1.26.2.1  2007/04/17 16:31:53  phase1geo
 Fixing bug 1698806 by rebinding a parameter signal to its list of expressions
 prior to writing the initial CDD file (elaboration phase).  Added param16
 diagnostic to regression suite to verify the fix.  Full regressions pass.

 Revision 1.26  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.25  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.24  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.23  2006/02/02 22:37:41  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.22  2006/02/01 19:58:28  phase1geo
 More updates to allow parsing of various parameter formats.  At this point
 I believe full parameter support is functional.  Regression has been updated
 which now completely passes.  A few new diagnostics have been added to the
 testsuite to verify additional functionality that is supported.

 Revision 1.21  2006/02/01 15:13:11  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.20  2006/01/24 23:24:38  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.19  2006/01/20 22:44:51  phase1geo
 Moving parameter resolution to post-bind stage to allow static functions to
 be considered.  Regression passes without static function testing.  Static
 function support still has some work to go.  Checkpointing.

 Revision 1.18  2006/01/20 19:15:23  phase1geo
 Fixed bug to properly handle the scoping of parameters when parameters are created/used
 in non-module functional units.  Added param10*.v diagnostics to regression suite to
 verify the behavior is correct now.

 Revision 1.17  2006/01/12 22:14:45  phase1geo
 Completed code for handling parameter value pass by name Verilog-2001 syntax.
 Added diagnostics to regression suite and updated regression files for this
 change.  Full regression now passes.

 Revision 1.16  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.15  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.14  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.13  2003/02/07 02:28:24  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.12  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.11  2002/10/31 23:14:12  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.10  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.9  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.8  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.7  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.6  2002/09/23 01:37:45  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.5  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.4  2002/09/21 04:11:32  phase1geo
 Completed phase 1 for adding in parameter support.  Main code is written
 that will create an instance parameter from a given module parameter in
 its entirety.  The next step will be to complete the module parameter
 creation code all the way to the parser.  Regression still passes and
 everything compiles at this point.

 Revision 1.3  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.2  2002/08/26 12:57:04  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.1  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.
*/

#endif

