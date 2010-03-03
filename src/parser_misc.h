#ifndef __PARSE_MISC_H__
#define __PARSE_MISC_H__

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
 \file     parser_misc.h
 \author   Trevor Williams  (phase1geo@gmail.com)
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
  unsigned int first_line;
  unsigned int first_column;
  unsigned int last_line;
  unsigned int last_column;
  char*        orig_fname;
  char*        incl_fname;
  unsigned int ppfline;
  unsigned int pplline;
};

#define YYLTYPE struct vlltype

/* This for compatibility with new and older bison versions. */
#ifndef yylloc
#ifdef GENERATOR
#define yylloc GENlloc
#else
#define yylloc VLlloc
#endif
#endif
extern YYLTYPE yylloc;

/*
 * Interface into the lexical analyzer. ...
 */
extern int  VLlex();
extern void VLerror( char* msg );
#define yywarn VLwarn
/*@-exportlocal@*/
extern void VLwarn( char* msg );
/*@=exportlocal@*/

extern unsigned error_count;

/*! \brief Deallocates the curr_sig_width variable if it has been previously set */
void parser_dealloc_sig_range( sig_range* range, bool rm_ptr );

/*! \brief Creates a copy of the curr_range variable */
sig_range* parser_copy_curr_range( bool packed );

/*! \brief Copies specifies static expressions to the current range */
void parser_copy_range_to_curr_range( sig_range* range, bool packed );

/*! \brief Deallocates and sets the curr_range variable from explicitly set values */
void parser_explicitly_set_curr_range( static_expr* left, static_expr* right, bool packed );

/*! \brief Deallocates and sets the curr_range variable from implicitly set values */
void parser_implicitly_set_curr_range( int left_num, int right_num, bool packed );

/*! \brief Checks the specified generation value to see if it holds in the specified module */
bool parser_check_generation( unsigned int gen );

#endif

