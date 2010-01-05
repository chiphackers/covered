/*
 Copyright (c) 2006-2010 Trevor Williams

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
extern char         user_msg[USER_MSG_LENGTH];
extern sig_range    curr_prange;
extern sig_range    curr_urange;
extern func_unit*   curr_funit;
extern str_link*    gen_mod_head;
extern unsigned int flag_global_generation;


/*!
 Counts the number of errors found during the parsing process.
*/
unsigned error_count = 0;

/*!
 Counts the number of warnings found during the parsing process.
*/
static unsigned warn_count = 0;

/*!
 Outputs specified error message string to standard output and increments
 error count.
*/
void VLerror(
  char* msg  /*!< String containing error message to display to user */
) { PROFILE(VLERROR);

  unsigned int rv;

  error_count++;
  
  rv = snprintf( user_msg, USER_MSG_LENGTH, "%s,", msg );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, FATAL, __FILE__, __LINE__ );
  rv = snprintf( user_msg, USER_MSG_LENGTH, "File: %s, Line: %u, Column: %u",
                 obf_file( yylloc.orig_fname ), yylloc.first_line, yylloc.first_column );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );

  PROFILE_END;

}

/*!
 Outputs specified warning message string to standard output and increments
 warning count.
*/
void VLwarn(
  char* msg  /*!< String containing warning message to display to user */
) { PROFILE(VLWARN);

  unsigned int rv;

  warn_count++;
  
  rv = snprintf( user_msg, USER_MSG_LENGTH, "%s,", msg );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );
  rv = snprintf( user_msg, USER_MSG_LENGTH, "File: %s, Line: %u, Column: %u",
                 obf_file( yylloc.orig_fname ), yylloc.first_line, yylloc.first_column );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );

  PROFILE_END;

}

/*!
 Called by parser when file wrapping is required.
*/
int VLwrap() {

  return -1;

}
#endif /* VPI_ONLY */

/*!
 Deallocates all allocated memory within associated signal range variable, but does
 not deallocate the pointer itself (unless rm_ptr is set to TRUE).
*/
void parser_dealloc_sig_range(
  sig_range* range,  /*!< Pointer to signal range to deallocate */
  bool       rm_ptr  /*!< If TRUE, deallocates the pointer to the given range */
) { PROFILE(PARSER_DEALLOC_SIG_RANGE);

  int i;  /* Loop iterator */

  for( i=0; i<range->dim_num; i++ ) {
    static_expr_dealloc( range->dim[i].left,  range->exp_dealloc );
    static_expr_dealloc( range->dim[i].right, range->exp_dealloc );
  }

  if( i > 0 ) {
    free_safe( range->dim, (sizeof( vector_width ) * range->dim_num) );
    range->dim     = NULL;
    range->dim_num = 0;
  }

  /* Clear the clear bit */
  range->clear       = FALSE;

  /* Set the deallocation bit */
  range->exp_dealloc = TRUE;

  /* Deallocate pointer itself, if specified to do so */
  if( rm_ptr ) {
    free_safe( range, sizeof( sig_range ) );
  }

  PROFILE_END;

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
  range->clear       = crange->clear;
  range->exp_dealloc = crange->exp_dealloc;

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
  crange->clear       = range->clear;
  crange->exp_dealloc = range->exp_dealloc;

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
 \return Returns TRUE if the given gen value (see \ref generations for legal values) is less than
         or equal to the generation value specified for the current functional unit (or globally).
*/
bool parser_check_generation(
  unsigned int gen  /*!< Generation value to check */
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

  PROFILE_END;

  return( retval );

}
#endif /* VPI_ONLY */

