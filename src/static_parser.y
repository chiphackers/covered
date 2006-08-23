
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
#include "static.h"
#include "util.h"
#include "obfuscate.h"

extern void reset_static_lexer( char* str );

extern char user_msg[USER_MSG_LENGTH];

/*! Pointer to found static expression in given string */
static static_expr* se;

/*!
 If set to TRUE, specifies that we are currently parsing syntax on the left-hand-side of an
 assignment.
*/
static bool       se_lhs_mode;

/*! Set to the current filename being parsed */
func_unit* se_funit;

/*! Set to the current line number being parsed */
static int        se_lineno;


#define YYERROR_VERBOSE 1

/* Uncomment these lines to turn debugging on */
/*
#define YYDEBUG 1
int yydebug = 1; 
*/

%}

%union {
  char*        text;
  vector*      number;
  static_expr* statexp;
};

%token <text>     IDENTIFIER
%token <number>   NUMBER
%token <realtime> REALTIME
%token            K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_LSS K_RS K_RSS K_SG K_EG
%token            K_NAND K_NOR K_NXOR

%type <statexp>   static_expr static_expr_primary static_expr_port_list

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
      se = $1;
    }
		
static_expr_port_list
  : static_expr_port_list ',' static_expr
    {
      static_expr* tmp = $3;
      if( $3 != NULL ) {
        tmp = static_expr_gen_unary( $3, EXP_OP_PASSIGN, se_lineno, @3.first_column, (@3.last_column - 1) );
        tmp = static_expr_gen( tmp, $1, EXP_OP_LIST, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        $$ = $1;
      }
    }
  | static_expr
    {
      static_expr* tmp = $1;
      if( $1 != NULL ) {
        tmp = static_expr_gen_unary( $1, EXP_OP_PASSIGN, se_lineno, @1.first_column, (@1.last_column - 1) );
      }
      $$ = tmp;
    }
  ;

static_expr
  : static_expr_primary
    {
      $$ = $1;
    }
  | '+' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp = $2;
      if( tmp != NULL ) {
        if( tmp->exp == NULL ) {
          tmp->num = 0 + tmp->num;
        }
      }
      $$ = tmp;
    }
  | '-' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp = $2;
      if( tmp != NULL ) {
        if( tmp->exp == NULL ) {
          tmp->num = 0 - tmp->num;
        }
      }
      $$ = tmp;
    }
  | '~' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UINV, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '&' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UAND, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '!' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNOT, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '|' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UOR, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '^' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UXOR, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | K_NAND static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNAND, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | K_NOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNOR, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | K_NXOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNXOR, se_lineno, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '^' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_XOR, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '*' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MULTIPLY, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '/' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_DIVIDE, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '%' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MOD, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '+' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_ADD, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '-' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_SUBTRACT, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '*' '*' static_expr
    {
      static_expr* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Exponential power operation found in block that is specified to not allow Verilog-2001 syntax" );
        static_expr_dealloc( $1, TRUE );
        static_expr_dealloc( $4, TRUE );
        $$ = NULL;
      } else {
        tmp = static_expr_gen( $4, $1, EXP_OP_EXPONENT, se_lineno, @1.first_column, (@4.last_column - 1), NULL );
        $$ = tmp;
      }
    }
  | static_expr '&' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_AND, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '|' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_OR, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NOR, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NAND static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NAND, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NXOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NXOR, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_LS static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_LSHIFT, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_RS static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_RSHIFT, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_GE static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_GE, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_LE static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_LE, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_EQ static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_EQ, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NE static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NE, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '>' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_GT, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '<' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_LT, se_lineno, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  ;

static_expr_primary
  : NUMBER
    {
      static_expr* tmp;
      tmp = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
      tmp->num = vector_to_int( $1 );
      tmp->exp = NULL;
      vector_dealloc( $1 );
      $$ = tmp;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | IDENTIFIER
    {
      static_expr* tmp;
      tmp = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
      tmp->num = -1;
      tmp->exp = db_create_expression( NULL, NULL, EXP_OP_SIG, se_lhs_mode, se_lineno, @1.first_column, (@1.last_column - 1), $1 );
      free_safe( $1 );
      $$ = tmp;
    }
  | '(' static_expr ')'
    {
      $$ = $2;
    }
  | IDENTIFIER '(' static_expr_port_list ')'
    {
      static_expr* tmp;
      tmp = static_expr_gen( NULL, $3, EXP_OP_FUNC_CALL, se_lineno, @1.first_column, (@4.last_column - 1), $1 );
      $$  = tmp;
      free_safe( $1 );
    }
  ;

%%

/*!
 \param str     Pointer to string to parse as a static expression.
 \param lhs     Specifies if this expression exists on the left-hand-side of an assignment expression
 \param fname   Filename containing this expression
 \param lineno  Line number containing this expression

 \return Returns an allocated static expression structure representing the given string
*/
static_expr* parse_static_expr( char* str, bool lhs, func_unit* funit, int lineno ) {

  /* Set the global values */
  se_lhs_mode = lhs;
  se_funit    = funit;
  se_lineno   = lineno;

  /* Reset the lexer with the given string */
  reset_static_lexer( str );

  return( se );

}

int SEerror( char* str ) {

  snprintf( user_msg, USER_MSG_LENGTH, "%s,   file: %s, line: %d", str, obf_file( se_funit->name ), se_lineno );
  print_output( user_msg, FATAL, __FILE__, __LINE__ );
  exit( 1 );

}
