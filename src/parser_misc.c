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
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/19/2001
*/

#include <stdio.h>

#include "defines.h"
#include "parser_misc.h"
#include "util.h"
#include "static.h"
#include "link.h"
#include "obfuscate.h"


#ifndef VPI_ONLY
extern char       user_msg[USER_MSG_LENGTH];
extern sig_range  curr_prange;
extern sig_range  curr_urange;
extern func_unit* curr_funit;
extern str_link*  gen_mod_head;
extern int        flag_global_generation;


/*!
 Counts the number of errors found during the parsing process.
*/
unsigned error_count = 0;

/*!
 Counts the number of warnings found during the parsing process.
*/
static unsigned warn_count = 0;

/*!
 \param msg  String containing error message to display to user.

 Outputs specified error message string to standard output and increments
 error count.
*/
void VLerror(
  char* msg
) { PROFILE(VLERROR);

  unsigned int rv;

  error_count++;
  
  rv = snprintf( user_msg, USER_MSG_LENGTH, "%s,", msg );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, FATAL, __FILE__, __LINE__ );
  rv = snprintf( user_msg, USER_MSG_LENGTH, "File: %s, Line: %u, Column: %u",
                 obf_file( yylloc.text ), yylloc.first_line, yylloc.first_column );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );

}

/*!
 \param msg  String containing warning message to display to user.

 Outputs specified warning message string to standard output and increments
 warning count.
*/
void VLwarn(
  char* msg
) { PROFILE(VLWARN);

  unsigned int rv;

  warn_count++;
  
  rv = snprintf( user_msg, USER_MSG_LENGTH, "%s,", msg );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );
  rv = snprintf( user_msg, USER_MSG_LENGTH, "File: %s, Line: %u, Column: %u",
                 obf_file( yylloc.text ), yylloc.first_line, yylloc.first_column );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );

}

/*!
 Called by parser when file wrapping is required.
*/
int VLwrap() {

  return -1;

}
#endif /* VPI_ONLY */

/*!
 \param range   Pointer to signal range to deallocate
 \param rm_ptr  If TRUE, deallocates the pointer to the given range

 Deallocates all allocated memory within associated signal range variable, but does
 not deallocate the pointer itself (unless rm_ptr is set to TRUE).
*/
void parser_dealloc_sig_range(
  sig_range* range,
  bool       rm_ptr
) { PROFILE(PARSER_DEALLOC_SIG_RANGE);

  int i;  /* Loop iterator */

  for( i=0; i<range->dim_num; i++ ) {
    static_expr_dealloc( range->dim[i].left,  FALSE );
    static_expr_dealloc( range->dim[i].right, FALSE );
  }

  if( i > 0 ) {
    free_safe( range->dim, (sizeof( vector_width ) * range->dim_num) );
    range->dim     = NULL;
    range->dim_num = 0;
  }

  /* Clear the clear bit */
  range->clear = FALSE;

  /* Deallocate pointer itself, if specified to do so */
  if( rm_ptr ) {
    free_safe( range, sizeof( sig_range ) );
  }

}

#ifndef VPI_ONLY
/*!
 \param packed  Specifies if curr_prange (TRUE) or curr_urange (FALSE) should be copied.

 Creates a copy of the curr_range variable for stored usage.
*/
sig_range* parser_copy_curr_range(
  bool packed
) { PROFILE(PARSER_COPY_CURR_RANGE);

  sig_range* crange;  /* Pointer to curr_range variable to copy */
  sig_range* range;   /* Copy of the curr_range variable */
  int        i;       /* Loop iterator */

  /* Get the correct global range */
  crange = packed ? &curr_prange : &curr_urange;

  /* Allocate memory for the new range */
  range = (sig_range*)malloc_safe( sizeof( sig_range ) );

  /* Set curr_range */
  range->dim_num = crange->dim_num;
  if( crange->dim_num > 0 ) {
    range->dim = (vector_width*)malloc_safe( sizeof( vector_width ) * crange->dim_num );
    for( i=0; i<crange->dim_num; i++ ) {
      range->dim[i].left       = (static_expr*)malloc_safe( sizeof( static_expr ) );
      range->dim[i].left->num  = crange->dim[i].left->num;
      range->dim[i].left->exp  = crange->dim[i].left->exp;
      range->dim[i].right      = (static_expr*)malloc_safe( sizeof( static_expr ) );
      range->dim[i].right->num = crange->dim[i].right->num;
      range->dim[i].right->exp = crange->dim[i].right->exp;
      range->dim[i].implicit   = FALSE;
    }
  }

  return( range );

}

/*!
 \param range   Pointer to signal vector range
 \param packed  Specifies if curr_prange (TRUE) or curr_urange (FALSE) should be updated.

 Copies specifies static expressions to the specified current range.  Primarily used for
 copying typedef'ed ranges to the current range.
*/
void parser_copy_range_to_curr_range(
  sig_range* range,
  bool       packed
) { PROFILE(PARSER_COPY_RANGE_TO_CURR_RANGE);

  sig_range* crange = packed ? &curr_prange : &curr_urange;  /* Pointer to curr_Xrange to use */
  int        i;                                              /* Loop iterator */

  /* Deallocate any memory currently associated with the curr_range variable */
  parser_dealloc_sig_range( crange, FALSE );

  /* Set curr_range */
  crange->dim_num = range->dim_num;
  if( range->dim_num > 0 ) {
    crange->dim = (vector_width*)malloc_safe( sizeof( vector_width ) * range->dim_num );
    for( i=0; i<range->dim_num; i++ ) {
      crange->dim[i].left       = (static_expr*)malloc_safe( sizeof( static_expr ) );
      crange->dim[i].left->num  = range->dim[i].left->num;
      crange->dim[i].left->exp  = range->dim[i].left->exp;
      crange->dim[i].right      = (static_expr*)malloc_safe( sizeof( static_expr ) );
      crange->dim[i].right->num = range->dim[i].right->num;
      crange->dim[i].right->exp = range->dim[i].right->exp;
      crange->dim[i].implicit   = FALSE;
    }
  }

}

/*!
 \param left    Pointer to static expression of expression/value on the left side of the colon.
 \param right   Pointer to static expression of expression/value on the right side of the colon.
 \param packed  If TRUE, adds a packed dimension; otherwise, adds an unpacked dimension.

 Deallocates and sets the curr_range variable from static expressions
*/
void parser_explicitly_set_curr_range(
  static_expr* left,
  static_expr* right,
  bool         packed
) { PROFILE(PARSER_EXPLICITLY_SET_CURR_RANGE);

  sig_range* crange;  /* Pointer to curr_Xrange to change */

  /* Get a pointer to the correct signal range to use */
  crange = packed ? &curr_prange : &curr_urange;

  /* Clear current range, if specified */
  if( crange->clear ) {
    parser_dealloc_sig_range( crange, FALSE );
  }

  /* Now rebuild current range, adding in the new range */
  crange->dim_num++;
  crange->dim = (vector_width*)realloc_safe( crange->dim, (sizeof( vector_width ) * (crange->dim_num - 1)), (sizeof( vector_width ) * crange->dim_num) );
  crange->dim[crange->dim_num - 1].left     = left;
  crange->dim[crange->dim_num - 1].right    = right;
  crange->dim[crange->dim_num - 1].implicit = FALSE;

}

/*!
 \param left_num   Integer value of left expression
 \param right_num  Integer value of right expression
 \param packed     If TRUE, adds a packed dimension; otherwise, adds an unpacked dimension.

 Deallocates and sets the curr_range variable from known integer values.
*/
void parser_implicitly_set_curr_range(
  int  left_num,
  int  right_num,
  bool packed
) { PROFILE(PARSER_IMPLICITLY_SET_CURR_RANGE);

  sig_range* crange;  /* Pointer to curr_Xrange to modify */

  /* Get a pointer to the curr_Xrange to modify */
  crange = packed ? &curr_prange : &curr_urange;

  /* Clear current range, if specified */
  if( crange->clear ) {
    parser_dealloc_sig_range( crange, FALSE );
  }

  /* Now rebuild current range, adding in the new range */
  crange->dim_num++;
  crange->dim = (vector_width*)realloc_safe( crange->dim, (sizeof( vector_width ) * (crange->dim_num - 1)), (sizeof( vector_width ) * crange->dim_num) );
  crange->dim[crange->dim_num - 1].left       = (static_expr*)malloc_safe( sizeof( static_expr ) );
  crange->dim[crange->dim_num - 1].left->num  = left_num;
  crange->dim[crange->dim_num - 1].left->exp  = NULL;
  crange->dim[crange->dim_num - 1].right      = (static_expr*)malloc_safe( sizeof( static_expr ) );
  crange->dim[crange->dim_num - 1].right->num = right_num;
  crange->dim[crange->dim_num - 1].right->exp = NULL;
  crange->dim[crange->dim_num - 1].implicit   = TRUE;

}

/*!
 \param gen  Generation value to check

 \return Returns TRUE if the given gen value (see \ref generations for legal values) is less than
         or equal to the generation value specified for the current functional unit (or globally).
*/
bool parser_check_generation(
  int gen
) { PROFILE(PARSER_CHECK_GENERATION);

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
#endif /* VPI_ONLY */


/*
 $Log$
 Revision 1.23  2008/03/17 22:02:32  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.22  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.21  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.20  2008/01/16 23:10:32  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.19  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.18  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.17  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.16  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.15  2006/10/25 22:35:41  phase1geo
 Starting to update testsuite to verify VPI mode as well.  Fixing runtime
 issues with vpi.c.  Also updated VL_error output format for easier readability.
 Fixing bug in parser for statement blocks that do not contain RHS signals for
 implicit event expressions.  Updating regressions for these changes.

 Revision 1.14  2006/10/09 17:54:19  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.13  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.12  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.11  2006/08/31 22:32:18  phase1geo
 Things are in a state of flux at the moment.  I have added proper parsing support
 for assertions, properties and sequences.  Also added partial support for the $root
 space (though I need to work on figuring out how to handle it in terms of the
 instance tree) and all that goes along with that.  Add parsing support with an
 error message for multi-dimensional array declarations.  Regressions should not be
 expected to run correctly at the moment.

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

