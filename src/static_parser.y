
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
 \file     static_parser.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/23/2006
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "db.h"
#include "obfuscate.h"
#include "parser_misc.h"
#include "static.h"
#include "util.h"

extern int  SElex();
extern void reset_static_lexer( char* str );
extern char user_msg[USER_MSG_LENGTH];

int SEerror( char* str );

/*! Pointer to value of the current static expression string */
static int  se_value;

/*! Set to the current filename being parsed */
func_unit*  se_funit;

/*! Set to the current line number being parsed */
int         se_lineno;

/*! Specifies if generate variables are allowed in the parsed expression */
bool        se_no_gvars;


#define YYERROR_VERBOSE 1

/* Uncomment these lines to turn debugging on */
/*
#define YYDEBUG 1
int yydebug = 1; 
*/

%}

%union {
  char* text;
  int   number;
};

%token <text>     IDENTIFIER
%token <number>   NUMBER
%token <realtime> REALTIME
%token            K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_LSS K_RS K_RSS K_SG K_EG
%token            K_NAND K_NOR K_NXOR

%type <number>    static_expr static_expr_primary static_expr_port_list

%left '|'
%left '^' K_NXOR K_NOR
%left '&' K_NAND
%left K_EQ K_NE K_CEQ K_CNE
%left K_GE K_LE '<' '>'
%left K_LS K_RS K_LSS K_RSS
%left '+' '-'
%left '*' '/' '%'
%left UNARY_PREC

%%

main
  : static_expr
    {
      se_value = $1;
    }
		
static_expr_port_list
  : static_expr_port_list ',' static_expr
    {
      $$ = 0;  /* TBD */
    }
  | static_expr
    {
      $$ = 0;  /* TBD */
    }
  ;

static_expr
  : static_expr_primary
    {
      $$ = $1;
    }
  | '+' static_expr_primary %prec UNARY_PREC
    {
      $$ = 0 + $2;
    }
  | '-' static_expr_primary %prec UNARY_PREC
    {
      $$ = 0 - $2;
    }
  | '~' static_expr_primary %prec UNARY_PREC
    {
      $$ = ~$2;
    }
  | '&' static_expr_primary %prec UNARY_PREC
    {
      int val = $2 & 0x1;
      int i;
      for( i=1; i<(SIZEOF_INT * 8); i++ ) {
        val &= (($2 >> i) & 0x1);
      }
      $$ = val;
    }
  | '!' static_expr_primary %prec UNARY_PREC
    {
      $$ = ($2 == 0) ? 1 : 0;
    }
  | '|' static_expr_primary %prec UNARY_PREC
    {
      int val = $2 & 0x1;
      int i;
      for( i=1; i<(SIZEOF_INT * 8); i++ ) {
        val |= (($2 >> i) & 0x1);
      }
      $$ = val;
    }
  | '^' static_expr_primary %prec UNARY_PREC
    {
      int val = $2 & 0x1;
      int i;
      for( i=1; i<(SIZEOF_INT * 8); i++ ) {
        val ^= (($2 >> i) & 0x1);
      }
      $$ = val;
    }
  | K_NAND static_expr_primary %prec UNARY_PREC
    {
      int val = $2 & 0x1;
      int i;
      for( i=1; i<(SIZEOF_INT * 8); i++ ) {
        val &= (($2 >> i) & 0x1);
      }
      $$ = (val == 0) ? 1 : 0;
    }
  | K_NOR static_expr_primary %prec UNARY_PREC
    {
      int val = $2 & 0x1;
      int i;
      for( i=1; i<(SIZEOF_INT * 8); i++ ) {
        val |= (($2 >> i) & 0x1);
      }
      $$ = (val == 0) ? 1 : 0;
    }
  | K_NXOR static_expr_primary %prec UNARY_PREC
    {
      int val = $2 & 0x1;
      int i;
      for( i=1; i<(SIZEOF_INT * 8); i++ ) {
        val ^= (($2 >> i) & 0x1);
      }
      $$ = (val == 0) ? 1 : 0;
    }
  | static_expr '^' static_expr
    {
      $$ = $1 ^ $3;
    }
  | static_expr '*' static_expr
    {
      $$ = $1 * $3;
    }
  | static_expr '/' static_expr
    {
      $$ = $1 / $3;
    }
  | static_expr '%' static_expr
    {
      $$ = $1 % $3;
    }
  | static_expr '+' static_expr
    {
      $$ = $1 + $3;
    }
  | static_expr '-' static_expr
    {
      $$ = $1 - $3;
    }
  | static_expr '*' '*' static_expr
    {
      int value = 1;
      int i;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        SEerror( "Exponential power operation found in block that is specified to not allow Verilog-2001 syntax" );
      } else {
        for( i=0; i<$4; i++ ) {
          value *= $1;
        }
      }
      $$ = value;
    }
  | static_expr '&' static_expr
    {
      $$ = $1 & $3;
    }
  | static_expr '|' static_expr
    {
      $$ = $1 | $3;
    }
  | static_expr K_NOR static_expr
    {
      $$ = ~($1 | $3);
    }
  | static_expr K_NAND static_expr
    {
      $$ = ~($1 & $3);
    }
  | static_expr K_NXOR static_expr
    {
      $$ = ~($1 ^ $3);
    }
  | static_expr K_LS static_expr
    {
      $$ = $1 << $3;
    }
  | static_expr K_RS static_expr
    {
      $$ = $1 >> $3;
    }
  | static_expr K_GE static_expr
    {
      $$ = ($1 >= $3);
    }
  | static_expr K_LE static_expr
    {
      $$ = ($1 <= $3);
    }
  | static_expr K_EQ static_expr
    {
      $$ = ($1 == $3);
    }
  | static_expr K_NE static_expr
    {
      $$ = ($1 != $3);
    }
  | static_expr '>' static_expr
    {
      $$ = ($1 > $3);
    }
  | static_expr '<' static_expr
    {
      $$ = ($1 < $3);
    }
  ;

static_expr_primary
  : NUMBER
    {
      $$ = $1;
    }
  | REALTIME
    {
      SEerror( "Realtime value found in constant expression which is not currently supported" );
      $$ = 0;
    }
  | '(' static_expr ')'
    {
      $$ = $2;
    }
  | IDENTIFIER '(' static_expr_port_list ')'
    {
      SEerror( "Static function call used in constant expression which is not currently supported" );
    }
  ;

%%

/*!
 \param str         Pointer to string to parse as a static expression.
 \param fname       Filename containing this expression
 \param lineno      Line number containing this expression
 \param no_genvars  Specifies if generate variables can exist in the given expression string

 \return Returns the value of the given expression string.

 Parses given static expression string and returns its calculated integer value.
*/
int parse_static_expr( char* str, func_unit* funit, int lineno, bool no_genvars ) {

  /* Set the global values */
  se_funit    = funit;
  se_lineno   = lineno;
  se_no_gvars = no_genvars;

  /* Reset the lexer with the given string */
  reset_static_lexer( str );

  /* Call the parser */
  if( SEparse() != 0 ) {
    exit( 1 );
  }

  return( se_value );

}

/*!
 \param str  String to output to standard error

 \return Returns 0

 Displays an error message from the static expression parser and exits execution.
*/
int SEerror( char* str ) {

  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d", str, obf_file( se_funit->name ), se_lineno );
  print_output( user_msg, FATAL, __FILE__, __LINE__ );
  exit( 1 );

  return( 0 );

}
