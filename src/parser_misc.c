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
 \file     parser_misc.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/19/2001
*/

#include <stdio.h>

#include "defines.h"
#include "parser_misc.h"
#include "util.h"
#include "static.h"
#include "link.h"
#include "obfuscate.h"


extern char          user_msg[USER_MSG_LENGTH];
extern vector_width* curr_range;
extern func_unit*    curr_funit;
extern str_link*     gen_mod_head;
extern int           flag_global_generation;


/*!
 Counts the number of errors found during the parsing process.
*/
unsigned error_count = 0;

/*!
 Counts the number of warnings found during the parsing process.
*/
unsigned warn_count  = 0;

/*!
 \param msg  String containing error message to display to user.

 Outputs specified error message string to standard output and increments
 error count.
*/
void VLerror( char* msg ) {

  error_count += 1;
  
  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d, col: %d",
            msg, obf_file( yylloc.text ), yylloc.first_line, yylloc.first_column );
  print_output( user_msg, FATAL, __FILE__, __LINE__ );

}

/*!
 \param msg  String containing warning message to display to user.

 Outputs specified warning message string to standard output and increments
 warning count.
*/
void VLwarn( char* msg ) {

  warn_count += 1;
  
  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d, col: %d",
            msg, obf_file( yylloc.text ), yylloc.first_line, yylloc.first_column );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );

}

/*!
 Called by parser when file wrapping is required.
*/
int VLwrap() {

  return -1;

}

/*!
 Deallocates all memory associated with the curr_range global variable.
*/
void parser_dealloc_curr_range() {

  if( curr_range != NULL ) {
    static_expr_dealloc( curr_range->left,  FALSE );
    static_expr_dealloc( curr_range->right, FALSE );
    free_safe( curr_range );
  }

}

/*!
 Creates a copy of the curr_range variable for stored usage.
*/
vector_width* parser_copy_curr_range() {

  vector_width* vw;  /* Copy of the curr_range variable */

  assert( curr_range != NULL );

  vw             = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );

  vw->left       = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
  vw->left->num  = curr_range->left->num;
  vw->left->exp  = curr_range->left->exp;

  vw->right      = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
  vw->right->num = curr_range->right->num;
  vw->right->exp = curr_range->right->exp;

  return( vw );

}

/*!
 \param left   Pointer to left static_expression to copy to current range
 \param right  Pointer to right static_expression to copy to current range

 Copies specifies static expressions to the current range.  Primarily used for
 copying typedef'ed ranges to the current range.
*/
void parser_copy_se_to_curr_range( static_expr* left, static_expr* right ) {

  /* Deallocate any memory currently associated with the curr_range variable */
  parser_dealloc_curr_range();

  /* Allocate and set curr_range */
  curr_range             = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
  curr_range->left       = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
  curr_range->left->num  = left->num;
  curr_range->left->exp  = left->exp;
  curr_range->right      = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
  curr_range->right->num = right->num;
  curr_range->right->exp = right->exp;
  curr_range->implicit   = FALSE;

}

/*!
 \param left   Pointer to static expression of expression/value on the left side of the colon.
 \param right  Pointer to static expression of expression/value on the right side of the colon.

 Deallocates and sets the curr_range variable from static expressions
*/
void parser_explicitly_set_curr_range( static_expr* left, static_expr* right ) {

  /* Deallocate any memory currently associated with the curr_range variable */
  parser_dealloc_curr_range();

  /* Allocate and set curr_range */
  curr_range           = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
  curr_range->left     = left;
  curr_range->right    = right;
  curr_range->implicit = FALSE;

}

/*!
 \param left_num   Integer value of left expression
 \param right_num  Integer value of right expression

 Deallocates and sets the curr_range variable from known integer values.
*/
void parser_implicitly_set_curr_range( int left_num, int right_num ) {

  /* Deallocate any memory currently associated with the curr_range variable */
  parser_dealloc_curr_range();

  /* Allocate and set curr_range */
  curr_range             = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
  curr_range->left       = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
  curr_range->left->num  = left_num;
  curr_range->left->exp  = NULL;
  curr_range->right      = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
  curr_range->right->num = right_num;
  curr_range->right->exp = NULL;
  curr_range->implicit   = TRUE;

}

/*!
 \param gen  Generation value to check

 \return Returns TRUE if the given gen value (see \ref generations for legal values) is less than
         or equal to the generation value specified for the current functional unit (or globally).
*/
bool parser_check_generation( int gen ) {

  bool      retval;    /* Return value for this function */
  str_link* strl;      /* Pointer to the str_link found to match the given mod_name */

  /* Search the generation module list to see if the specified module name has been set there */
  if( (curr_funit != NULL) && ((strl = str_link_find( curr_funit->name, gen_mod_head )) != NULL) ) {

    /* The user has specified a generation value for this module so check it against this */
    retval = (gen <= strl->suppl);

  } else {

    retval = (gen <= flag_global_generation);

  }

  return( retval );

}


/*
 $Log$
 Revision 1.10  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.9  2006/07/15 22:07:14  phase1geo
 Added all code to parser to check generation value to decide if a piece of
 syntax is allowable by the parser or not.  This code compiles and has been
 proven to not break regressions; however, none if it has been tested at this
 point.  Many regression tests to follow...

 Revision 1.8  2006/07/10 22:36:37  phase1geo
 Getting parser to parse generate blocks appropriately.  I believe this is
 accurate now.  Also added the beginnings of gen_item.c which is meant to
 hold functions which handle generate items.  This is really just a brainstorm
 at this point -- I still don't have a definite course of action on how to
 properly handle generate blocks.

 Revision 1.7  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.6  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.5  2006/02/01 19:58:28  phase1geo
 More updates to allow parsing of various parameter formats.  At this point
 I believe full parameter support is functional.  Regression has been updated
 which now completely passes.  A few new diagnostics have been added to the
 testsuite to verify additional functionality that is supported.

 Revision 1.4  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.3  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.2  2002/12/01 06:37:52  phase1geo
 Adding appropriate error output and causing score command to proper exit
 if parser errors are found.

*/

