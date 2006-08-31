#ifndef __PARSE_MISC_H__
#define __PARSE_MISC_H__

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
 \file     parser_misc.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/19/2001
 \brief    Contains miscellaneous functions declarations and defines used by parser.
*/

#include "defines.h"

/*!
 The vlltype supports the passing of detailed source file location
 information between the lexical analyzer and the parser. Defining
 YYLTYPE compels the lexor to use this type and not something other.
*/
struct vlltype {
  unsigned first_line;
  unsigned first_column;
  unsigned last_line;
  unsigned last_column;
  char*    text;
};

#define YYLTYPE struct vlltype

/* This for compatibility with new and older bison versions. */
#ifndef yylloc
# define yylloc VLlloc
#endif
extern YYLTYPE yylloc;

/*
 * Interface into the lexical analyzer. ...
 */
extern int  VLlex();
extern void VLerror( char* msg );
#define yywarn VLwarn
extern void VLwarn( char* msg );

extern unsigned error_count, warn_count;

/*! \brief Deallocates the curr_sig_width variable if it has been previously set */
void parser_dealloc_curr_range();

/*! \brief Creates a copy of the curr_range variable */
vector_width* parser_copy_curr_range();

/*! \brief Copies specifies static expressions to the current range */
void parser_copy_se_to_curr_range( static_expr* left, static_expr* right );

/*! \brief Deallocates and sets the curr_range variable from explicitly set values */
void parser_explicitly_set_curr_range( static_expr* left, static_expr* right );

/*! \brief Deallocates and sets the curr_range variable from implicitly set values */
void parser_implicitly_set_curr_range( int left_num, int right_num );

/*! \brief Checks the specified generation value to see if it holds in the specified module */
bool parser_check_generation( int gen );

/*
 $Log$
 Revision 1.8  2006/07/15 22:07:14  phase1geo
 Added all code to parser to check generation value to decide if a piece of
 syntax is allowable by the parser or not.  This code compiles and has been
 proven to not break regressions; however, none if it has been tested at this
 point.  Many regression tests to follow...

 Revision 1.7  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.6  2006/02/01 19:58:28  phase1geo
 More updates to allow parsing of various parameter formats.  At this point
 I believe full parameter support is functional.  Regression has been updated
 which now completely passes.  A few new diagnostics have been added to the
 testsuite to verify additional functionality that is supported.

*/

#endif

