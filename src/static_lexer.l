%{
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
 \file     static_lexer.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/23/2006
 \brief    Lexer for static expressions
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "defines.h"
//#include "parser_misc.h"
#include "static_parser.h"
#include "vector.h"
#include "scope.h"
#include "util.h"
#include "obfuscate.h"

#define yylval SElval
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

/* Redefine YY_INPUT so that we can parse a string and not a file */
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = seinput(b, ms))

static char* seinputptr;
static char* seinputlim;

extern func_unit* se_funit;
extern int        se_lineno;
extern bool       se_no_gvars;
extern char       user_msg[USER_MSG_LENGTH];

/*!
 Contains state before entering comment block.
*/
static int   comment_enter;
static int   yywrap();

%}

%option stack

%x LCOMMENT
%x CCOMMENT
%x PCOMMENT
%x CSTRING
%x DEFINE
%x INCLUDE
%x ERROR_LINE

WSPACE [ \t\b\f]+

%%

  /* Whitespace -- we ignore this */
[ \t\b\f\r]

  /* Multi-line comment */
"/*"           {
	comment_enter = YY_START;
	BEGIN( CCOMMENT );
 }
<CCOMMENT>.    {
	yymore();
 }
<CCOMMENT>"*/" {
	BEGIN( comment_enter );
 }

  /* Multi-character symbols */
"<<"   { return K_LS; }
">>"   { return K_RS; }
"<="   { return K_LE; }
">="   { return K_GE; }
"=="   { return K_EQ; }
"!="   { return K_NE; }
"~|"   { return K_NOR; }
"~^"   { return K_NXOR; }
"^~"   { return K_NXOR; }
"~&"   { return K_NAND; }

  /* Single character symbols */
[}{;:\[\],()#=.@&!?<>%|^~+*/-] {
	return yytext[0];
 }

  /* Generate variable value */
[a-zA-Z_][a-zA-Z0-9$_]* {
     vsignal*   gvar;
     func_unit* found_funit;
     if( !se_no_gvars ) {
       if( !scope_find_signal( yytext, se_funit, &gvar, &found_funit, 0 ) ) {
         snprintf( user_msg, USER_MSG_LENGTH, "Unable to find generate variable %s in module %s",
                   obf_sig( yytext ), obf_funit( se_funit->name ) );
         print_output( user_msg, FATAL, __FILE__, __LINE__ );
         exit( EXIT_FAILURE );
       }
       yylval.number = vector_to_int( gvar->value );
     } else {
       snprintf( user_msg, USER_MSG_LENGTH, "Generate variable found in constant expression outside of generate block, file: %s, line: %d",
                 obf_file( se_funit->filename ), se_lineno );
       print_output( user_msg, FATAL, __FILE__, __LINE__ );
       exit( EXIT_FAILURE );
     }
     return NUMBER;
 }

  /* Decimal number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[dD][ \t]*[0-9][0-9_]* {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Binary number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[bB][ \t]*[0-1xzXZ_\?]+ {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Octal number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[oO][ \t]*[0-7xzXZ_\?]+ {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Hexidecimal number - Size indicated */
[0-9][0-9_]*[ \t]*\'[sS]?[hH][ \t]*[0-9a-fA-FxzXZ_\?]+ {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Decimal number - unsized */
\'[sS]?[dD][ \t]*[0-9][0-9_]* {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Binary number - unsized */
\'[sS]?[bB][ \t]*[0-1xzXZ_\?]+ {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Octal number - unsized */
\'[sS]?[oO][ \t]*[0-7xzXZ_\?]+ {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Hexidecimal number - unsized */
\'[sS]?[hH][ \t]*[0-9a-fA-FxzXZ_\?]+ {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Unsized decimal number */
[0-9][0-9_]* {
     vector* vec = vector_from_string( &yytext, FALSE );
     assert( vec != NULL );
     yylval.number = vector_to_int( vec );
     vector_dealloc( vec );
     return NUMBER;
 }

  /* Real numbers -- we ignore this information */
[0-9][0-9_]*\.[0-9][0-9_]*([Ee][+-]?[0-9][0-9_]*)? { 
     return REALTIME;
 }

  /* Real numbers -- we ignore this information */
[0-9][0-9_]*[Ee][+-]?[0-9][0-9_]* { 
     return REALTIME;
 }

  /* Final catchall. something got lost or mishandled. */
. { fprintf( stderr, "%d: unmatched character (%c)\n", se_lineno, yytext[0] ); }

%%

/*!
 \param buf       Pointer to buffer to copy
 \param max_size  Maximum size of buffer

 \return Returns result of copy.

 Overloaded input() operator for lexer.  Allows us to do string parsing instead of
 file parsing.
*/
int seinput( char* buf, int max_size ) {

  int n;

  if( max_size < (seinputlim - seinputptr) ) {
    n = max_size;
  } else {
    n = (seinputlim - seinputptr);
  }

  if( n > 0 ) {
    memcpy( buf, seinputptr, n );
    seinputptr += n;
  }

  return( n );

}

/*!
 \return Returns 1
*/
static int yywrap() {

  return( 1 );

}

/*!
 \param str  String to parse.

 This function initializes the static parser to parse the given string.
*/
void reset_static_lexer( char* str ) {

  seinputptr = str;
  seinputlim = str + strlen( str );

}

