#ifndef __PARSE_MISC_H__
#define __PARSE_MISC_H__

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

/*! \brief Deallocates and sets the curr_range variable from explicitly set values */
void parser_explicitly_set_curr_range( static_expr* left, static_expr* right );

/*! \brief Deallocates and sets the curr_range variable from implicitly set values */
void parser_implicitly_set_curr_range( int left_num, int right_num );

/*
 $Log$
*/

#endif

