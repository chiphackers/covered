#ifndef __VCD_PARSER_MISC_H__
#define __VCD_PARSER_MISC_H__

/*!
 \file     vcd_parser_misc.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2001
 \brief    Contains definitions and other header information for VCD parser and lexer.
*/

#include "vector.h"

/*!
 * The vcdltype supports the passing of detailed source file location
 * information between the lexical analyzer and the parser. Defining
 * YYLTYPE compels the lexor to use this type and not something other.
 */
struct vcdltype {
  unsigned first_line;
  unsigned first_column;
  unsigned last_line;
  unsigned last_column;
  char*    text;
};

/*!
 Allows lexer to analyze and send back information about vector selects.
*/
struct sigwidth_s {
  int width;
  int lsb;
};

typedef struct sigwidth_s sigwidth;

struct valchange_s {
  vector* vec;
  char*   sym;
};

typedef struct valchange_s valchange;

#define YYLTYPE struct vcdltype
extern YYLTYPE yylloc;

/*
 * Interface into the lexical analyzer. ...
 */
extern int  VCDlex();
extern void VCDerror( char* msg );
#define yywarn VCDwarn
extern void VCDwarn( char* msg );

#endif

