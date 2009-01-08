#ifndef __CODEGEN_H__
#define __CODEGEN_H__

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
 \file     codegen.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/11/2002
 \brief    Contains functions for generating Verilog code from expression trees.
*/

#include "defines.h"


/*! \brief Creates Verilog code string from specified expression tree. */
void codegen_gen_expr(
            expression*   expr,
            func_unit*    funit,
  /*@out@*/ char***       code,
  /*@out@*/ unsigned int* code_depth
);

/*! \brief Creates Verilog code string from specified expression tree that is guaranteed to be one line. */
char* codegen_gen_expr_one_line(
  expression*  expr,
  func_unit*   funit,
  bool         inline_exp
);


/*
 $Log$
 Revision 1.15  2009/01/07 23:40:46  phase1geo
 Updates to support intermediate expression substitution.  Not done yet.  Checkpointing.

 Revision 1.14  2008/12/13 00:17:28  phase1geo
 Fixing more regression bugs.  Updated some original tests to make them comparable to the inlined method output.
 Checkpointing.

 Revision 1.13  2008/08/18 23:07:25  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.10.6.1  2008/07/10 22:43:50  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.11  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.10  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.9  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.8  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.7  2003/12/12 17:16:25  phase1geo
 Changing code generator to output logic based on user supplied format.  Full
 regression fails at this point due to mismatching report files.

 Revision 1.6  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:13:21  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/10/24 23:19:38  phase1geo
 Making some fixes to report output.  Fixing bugs.  Added long_exp1.v diagnostic
 to regression suite which finds a current bug in the report underlining
 functionality.  Need to look into this.

 Revision 1.2  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

#endif
