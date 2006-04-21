#ifndef __STATIC_H__
#define __STATIC_H__

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
 \file     static.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/02/2002
 \brief    Contains functions for handling creation of static expressions.
*/

#include "defines.h"


/*! \brief Calculates new values for unary static expressions and returns the new static expression. */
static_expr* static_expr_gen_unary( static_expr* stexp, int op, int line, int first, int last );

/*! \brief Calculates new values for static expression and returns the new static expression. */
static_expr* static_expr_gen( static_expr* right, static_expr* left, int op, int line, int first, int last, char* func_name );

/*! \brief Calculates LSB, width and endianness for specified left/right pair for vector (used before parameter resolve). */
void static_expr_calc_lsb_and_width_pre( static_expr* left, static_expr* right, int* width, int* lsb, int* big_endian );

/*! \brief Calculates LSB, width and endianness for specified left/right pair for vector (used after parameter resolve). */
void static_expr_calc_lsb_and_width_post( static_expr* left, static_expr* right, int* width, int* lsb, int* big_endian );

/*! \brief Deallocates static_expr memory from heap. */
void static_expr_dealloc( static_expr* stexp, bool rm_exp );


/*
 $Log$
 Revision 1.9.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.9  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.8  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.7  2004/04/19 04:54:56  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.6  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:14:29  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.2  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.1  2002/10/02 18:55:29  phase1geo
 Adding static.c and static.h files for handling static expressions found by
 the parser.  Initial versions which compile but have not been tested.
*/

#endif

