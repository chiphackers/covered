#ifndef __PARSE_MISC_H__
#define __PARSE_MISC_H__

/*!
 \file     parser_misc.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/19/2001
 \brief    Contains miscellaneous functions declarations and defines used by parser.
*/

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
  char*    pptext;
};

#define YYLTYPE struct vlltype
extern YYLTYPE yylloc;

/*
 * Interface into the lexical analyzer. ...
 */
extern int  VLlex();
extern void VLerror( char* msg );
#define yywarn VLwarn
extern void VLwarn( char* msg );

extern unsigned error_count, warn_count;

#endif

