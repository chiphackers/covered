
%{
/*!
 \file     parser.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/14/2001
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

#include "defines.h"
#include "vsignal.h"
#include "expr.h"
#include "vector.h"
#include "func_unit.h"
#include "db.h"
#include "link.h"
#include "parser_misc.h"
#include "statement.h"
#include "static.h"
#include "util.h"

extern char user_msg[USER_MSG_LENGTH];
extern int  delay_expr_type;

/* Functions from lexer */
extern void lex_start_udp_table();
extern void lex_end_udp_table();

int  ignore_mode = 0;
int  param_mode  = 0;
int  attr_mode   = 0;
bool lhs_mode    = FALSE;

exp_link* param_exp_head = NULL;
exp_link* param_exp_tail = NULL;

sig_link* wait_sig_head = NULL;
sig_link* wait_sig_tail = NULL;
sig_link* dummy_head    = NULL;
sig_link* dummy_tail    = NULL;

vector_width* curr_sig_width = NULL;

#define YYERROR_VERBOSE 1

/* Uncomment these lines to turn debugging on */
/*
#define YYDEBUG 1
int yydebug = 1; 
*/

/* Recent version of bison expect that the user supply a
   YYLLOC_DEFAULT macro that makes up a yylloc value from existing
   values. I need to supply an explicit version to account for the
   text field, that otherwise won't be copied. */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
          (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
          (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
          (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
          (Current).text         = YYRHSLOC(Rhs, 1).text;               \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC(Rhs, 0).last_line;                                 \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC(Rhs, 0).last_column;                               \
          (Current).text         = YYRHSLOC(Rhs, 0).text;               \
        }                                                               \
    while (0)

%}

%union {
  char*           text;
  int	          integer;
  vector*         number;
  double          realtime;
  vsignal*        sig;
  expression*     expr;
  statement*      state;
  static_expr*    statexp;
  vector_width*   vecwidth; 
  str_link*       strlink;
  exp_link*       explink;
  case_statement* case_stmt;
  attr_param*     attr_parm;
  exp_bind*       expbind;
};

%token <text>     IDENTIFIER
%token <text>     PATHPULSE_IDENTIFIER
%token <number>   NUMBER
%token <realtime> REALTIME
%token <number>   STRING
%token SYSTEM_IDENTIFIER IGNORE
%token UNUSED_IDENTIFIER
%token UNUSED_PATHPULSE_IDENTIFIER
%token UNUSED_NUMBER
%token UNUSED_REALTIME
%token UNUSED_STRING UNUSED_SYSTEM_IDENTIFIER
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_RS K_SG K_EG
%token K_PO_POS K_PO_NEG K_STARP K_PSTAR
%token K_LOR K_LAND K_NAND K_NOR K_NXOR K_TRIGGER
%token K_always K_and K_assign K_begin K_buf K_bufif0 K_bufif1 K_case
%token K_casex K_casez K_cmos K_deassign K_default K_defparam K_disable
%token K_edge K_else K_end K_endcase K_endfunction K_endmodule I_endmodule
%token K_endprimitive K_endspecify K_endtable K_endtask K_event K_for
%token K_force K_forever K_fork K_function K_highz0 K_highz1 K_if
%token K_initial K_inout K_input K_integer K_join K_large K_localparam
%token K_macromodule
%token K_medium K_module K_nand K_negedge K_nmos K_nor K_not K_notif0
%token K_notif1 K_or K_output K_parameter K_pmos K_posedge K_primitive
%token K_pull0 K_pull1 K_pulldown K_pullup K_rcmos K_real K_realtime
%token K_reg K_release K_repeat
%token K_rnmos K_rpmos K_rtran K_rtranif0 K_rtranif1 K_scalered
%token K_signed K_small K_specify
%token K_specparam K_strong0 K_strong1 K_supply0 K_supply1 K_table K_task
%token K_time K_tran K_tranif0 K_tranif1 K_tri K_tri0 K_tri1 K_triand
%token K_trior K_trireg K_vectored K_wait K_wand K_weak0 K_weak1
%token K_while K_wire
%token K_wor K_xnor K_xor
%token K_Shold K_Speriod K_Srecovery K_Ssetup K_Swidth K_Ssetuphold

%token KK_attribute

%type <integer>   net_type port_type
%type <vecwidth>  range_opt range range_or_type_opt
%type <statexp>   static_expr static_expr_primary
%type <text>      identifier
%type <expr>      expr_primary expression_list expression
%type <expr>      lavalue lpvalue
%type <expr>      event_control event_expression_list event_expression
%type <text>      udp_port_list
%type <expr>      delay_value delay_value_simple
%type <text>      defparam_assign_list defparam_assign
%type <text>      gate_instance
%type <text>      localparam_assign_list localparam_assign
%type <strlink>   register_variable_list list_of_variables
%type <strlink>   gate_instance_list
%type <text>      register_variable
%type <state>     statement statement_list statement_opt 
%type <state>     for_statement fork_statement while_statement named_begin_end_block if_statement_error
%type <case_stmt> case_items case_item
%type <expr>      delay1 delay3 delay3_opt
%type <attr_parm> attribute attribute_list

%token K_TAND
%right '?' ':'
%left K_LOR
%left K_LAND
%left '|'
%left '^' K_NXOR K_NOR
%left '&' K_NAND
%left K_EQ K_NE K_CEQ K_CNE
%left K_GE K_LE '<' '>'
%left K_LS K_RS
%left '+' '-'
%left '*' '/' '%'
%left UNARY_PREC

/* to resolve dangling else ambiguity: */
%nonassoc less_than_K_else
%nonassoc K_else

%%

  /* A degenerate source file can be completely empty. */
main 
  : source_file 
  |
  ;

  /* Verilog-2001 supports attribute lists, which can be attached to a
     variety of different objects. The syntax inside the (* *) is a
     comma separated list of names or names with assigned values. */
attribute_list_opt
  : K_PSTAR attribute_list K_STARP
    {
      db_parse_attribute( $2 );
    }
  | K_PSTAR K_STARP
  |
  ;

attribute_list
  : attribute_list ',' attribute
    {
      $3->next  = $1;
      $1->prev  = $3;
      $3->index = $1->index + 1;
      $$ = $3;
    } 
  | attribute
    {
      $$ = $1;
    }
  ;

attribute
  : IDENTIFIER
    {
      attr_param* ap = db_create_attr_param( $1, NULL );
      free_safe( $1 );
      $$ = ap;
    }
  | IDENTIFIER '=' {attr_mode++;} expression {attr_mode--;}
    {
      attr_param* ap = db_create_attr_param( $1, $4 );
      free_safe( $1 );
      $$ = ap;
    }
  ;

source_file 
  : description 
  | source_file description
  ;

description
  : module
  | udp_primitive
  | KK_attribute { ignore_mode++; } '(' UNUSED_IDENTIFIER ',' UNUSED_STRING ',' UNUSED_STRING ')' { ignore_mode--; }
  ;

module
  : module_start module_item_list K_endmodule
    {
      db_end_module( @3.first_line );
    }
  | module_start K_endmodule
    {
      db_end_module( @2.first_line );
    }
  | attribute_list_opt K_module IGNORE I_endmodule
  | attribute_list_opt K_macromodule IGNORE I_endmodule
  ;

module_start
  : attribute_list_opt
    K_module IDENTIFIER { ignore_mode++; } list_of_ports_opt { ignore_mode--; } ';'
    {
      db_add_module( $3, @2.text, @2.first_line );
    }
  | attribute_list_opt
    K_macromodule IDENTIFIER { ignore_mode++; } list_of_ports_opt { ignore_mode--; } ';'
    {
      db_add_module( $3, @2.text, @2.first_line );
    }
  ;

list_of_ports_opt
  : '(' list_of_ports ')'
  |
  ;

list_of_ports
  : list_of_ports ',' port_opt
  | port_opt
  ;

port_opt
  : port
  |
  ;

  /* Coverage tools should not find port information that interesting.  We will
     handle it for parsing purposes, but ignore its information. */

port
  : port_reference
  | '.' IDENTIFIER '(' { ignore_mode++; } port_reference { ignore_mode--; } ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' port_reference ')'
  | '{' { ignore_mode++; } port_reference_list { ignore_mode--; } '}'
  | '.' IDENTIFIER '(' '{' { ignore_mode--; } port_reference_list { ignore_mode++; } '}' ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' '{' port_reference_list '}' ')'
  ;

port_reference
  : UNUSED_IDENTIFIER
  | UNUSED_IDENTIFIER '[' static_expr ':' static_expr ']'
  | UNUSED_IDENTIFIER '[' static_expr ']'
  | UNUSED_IDENTIFIER '[' error ']'
  ;

port_reference_list
  : port_reference
  | port_reference_list ',' port_reference
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
      tmp = static_expr_gen_unary( $2, EXP_OP_UINV, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '&' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UAND, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '!' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNOT, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '|' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | '^' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UXOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | K_NAND static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNAND, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | K_NOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | K_NXOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNXOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '^' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_XOR, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '*' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MULTIPLY, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '/' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_DIVIDE, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '%' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MOD, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '+' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_ADD, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '-' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_SUBTRACT, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '&' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_AND, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr '|' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_OR, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr K_NOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NOR, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr K_NAND static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NAND, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  | static_expr K_NXOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NXOR, @1.first_line, @1.first_column, (@3.last_column - 1) );
      $$ = tmp;
    }
  ;

static_expr_primary
  : NUMBER
    {
      static_expr* tmp;
      if( ignore_mode == 0 ) {
        tmp = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        tmp->num = vector_to_int( $1 );
        tmp->exp = NULL;
        vector_dealloc( $1 );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | UNUSED_NUMBER
    {
      $$ = NULL;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | UNUSED_REALTIME
    {
      $$ = NULL;
    }
  | IDENTIFIER
    {
      static_expr* tmp;
      if( ignore_mode == 0 ) {
        tmp = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        tmp->num = -1;
        tmp->exp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        free_safe( $1 );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | '(' static_expr ')'
    {
      $$ = $2;
    }
  ;

expression
  : expr_primary
    {
      $$ = $1;
    }
  | '+' expr_primary %prec UNARY_PREC
    {
      if( ignore_mode == 0 ) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | '-' expr_primary %prec UNARY_PREC
    {
      if( ignore_mode == 0 ) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | '~' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UINV, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '&' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UAND, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '!' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UNOT, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '|' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '^' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UXOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NAND expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UNAND, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NOR expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UNOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NXOR expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          tmp = db_create_expression( $2, NULL, EXP_OP_UNXOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '+' error %prec UNARY_PREC
    {
      VLerror( "Operand of signed positive + is not a primary expression" );
    }
  | '-' error %prec UNARY_PREC
    {
      VLerror( "Operand of signed negative - is not a primary expression" );
    }
  | '~' error %prec UNARY_PREC
    {
      VLerror( "Operand of unary ~ is not a primary expression" );
    }
  | '&' error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction & is not a primary expression" );
    }
  | '!' error %prec UNARY_PREC
    {
      VLerror( "Operand of unary ! is not a primary expression" );
    }
  | '|' error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction | is not a primary expression" );
    }
  | '^' error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ^ is not a primary expression" );
    }
  | K_NAND error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ~& is not a primary expression" );
    }
  | K_NOR error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ~| is not a primary expression" );
    }
  | K_NXOR error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ~^ is not a primary expression" );
    }
  | expression '^' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_XOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '*' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_MULTIPLY, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '/' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_DIVIDE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '%' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_MOD, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '+' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_ADD, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '-' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_SUBTRACT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '&' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_AND, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '|' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_OR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NAND expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_NAND, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NOR expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_NOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NXOR expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_NXOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '<' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_LT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '>' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_GT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LS expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_LSHIFT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_RS expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_RSHIFT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_EQ expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_EQ, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_CEQ expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_CEQ, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_LE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_GE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_GE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_NE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_CNE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_CNE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LOR expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_LOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LAND expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_LAND, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '?' expression ':' expression
    {
      expression* csel;
      expression* cond;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($5 != NULL) ) {
        csel = db_create_expression( $5,   $3, EXP_OP_COND_SEL, lhs_mode, $3->line, @1.first_column, (@3.last_column - 1), NULL );
        cond = db_create_expression( csel, $1, EXP_OP_COND,     lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        $$ = cond;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        expression_dealloc( $5, FALSE );
        $$ = NULL;
      }
    }
  ;

expr_primary
  : NUMBER
    {
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
      vector_dealloc( tmp->value );
      tmp->value = $1;
      $$ = tmp;
    }
  | UNUSED_NUMBER
    {
      $$ = NULL;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | UNUSED_REALTIME
    {
      $$ = NULL;
    }
  | STRING
    {
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
      vector_dealloc( tmp->value );
      tmp->value = $1;
      $$ = tmp;
    }
  | UNUSED_STRING
    {
      $$ = NULL;
    }
  | identifier
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        $$  = tmp;
        free_safe( $1 );
      } else {
        $$ = NULL;
      }
    }
  | SYSTEM_IDENTIFIER
    {
      $$ = NULL;
    }
  | UNUSED_SYSTEM_IDENTIFIER
    {
      $$ = NULL;
    }
  | identifier '[' expression ']'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( NULL, $3, EXP_OP_SBIT_SEL, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
        $$  = tmp;
        free_safe( $1 );
      } else {
        if( $1 != NULL ) {
          free_safe( $1 );
        }
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | identifier '[' expression ':' expression ']'
    {		  
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($5 != NULL) ) {
        tmp = db_create_expression( $5, $3, EXP_OP_MBIT_SEL, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), $1 );
        $$  = tmp;
        free_safe( $1 );
      } else {
        if( $1 != NULL ) {
          free_safe( $1 );
        }
        expression_dealloc( $3, FALSE );
        expression_dealloc( $5, FALSE );
        $$ = NULL;
      }
    }
  | identifier '[' expression K_PO_POS static_expr ']'
    {
      snprintf( user_msg, USER_MSG_LENGTH, "K_PO_POS expressions not supported at this time, line: %d", @1.first_line );
      print_output( user_msg, WARNING, __FILE__, __LINE__ );
      expression_dealloc( $3, FALSE );
      static_expr_dealloc( $5, TRUE );
      free_safe( $1 );
      $$ = NULL;
    }
  | identifier '[' expression K_PO_NEG static_expr ']'
    {
      snprintf( user_msg, USER_MSG_LENGTH, "K_PO_POS expressions not supported at this time, line: %d", @1.first_line );
      print_output( user_msg, WARNING, __FILE__, __LINE__ );
      expression_dealloc( $3, FALSE );
      static_expr_dealloc( $5, TRUE );
      free_safe( $1 );
      $$ = NULL;
    }
  | identifier '(' expression_list ')'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL ) ) {
        tmp = db_create_expression( NULL, $3, EXP_OP_FUNC_CALL, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
        $$  = tmp;
        free_safe( $1 );
      } else {
        if( $1 != NULL ) {
          free_safe( $1 );
        }
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | SYSTEM_IDENTIFIER '(' expression_list ')'
    {
      expression_dealloc( $3, FALSE );
      $$ = NULL;
    }
  | UNUSED_SYSTEM_IDENTIFIER '(' expression_list ')'
    {
      $$ = NULL;
    }
  | '(' expression ')'
    {
      if( ignore_mode == 0 ) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | '{' expression_list '}'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        tmp = db_create_expression( $2, NULL, EXP_OP_CONCAT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
    }
  | '{' expression '{' expression_list '}' '}'
    {
      expression*  tmp;
      if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
        tmp = db_create_expression( $4, $2, EXP_OP_EXPAND, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $2, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  ;

  /* Expression lists are used in function/task calls, concatenations, and parameter overrides */
expression_list
  : expression_list ',' expression
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( param_mode == 0 ) {
          if( $3 != NULL ) {
            tmp = db_create_expression( $3, $1, EXP_OP_LIST, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
            $$ = tmp;
          } else {
            $$ = $1;
          }
        } else {
          if( $3 != NULL ) {
            exp_link_add( $3, &param_exp_head, &param_exp_tail );
          }
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | expression
    {
      if( ignore_mode == 0 ) {
        if( param_mode == 0 ) {
          $$ = $1;
        } else {
          if( $1 != NULL ) {
            exp_link_add( $1, &param_exp_head, &param_exp_tail );
          }
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  |
    {
      $$ = NULL;
    }
  | expression_list ','
    {
      if( ignore_mode == 0 ) {
        if( param_mode == 0 ) {
          $$ = $1;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

identifier
  : IDENTIFIER
    {
      $$ = $1;
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | identifier '.' IDENTIFIER
    {
      /* Hierarchical references are going to be unsupported at the current time */
      if( $1 != NULL ) {
        free_safe( $1 );
      }
      free_safe( $3 );
      $$ = NULL;
    }
  | identifier '.' UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  ;

list_of_variables
  : IDENTIFIER
    {
      str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
      tmp->str  = $1;
      tmp->next = NULL;
      $$ = tmp;
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | list_of_variables ',' IDENTIFIER
    {
      str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
      tmp->str  = $3;
      tmp->next = $1;
      $$ = tmp;
    }
  | list_of_variables ',' UNUSED_IDENTIFIER
    {
      $$ = $1;
    }
  ;

  /* I don't know what to do with UDPs.  This is included to allow designs that contain
     UDPs to pass parsing. */
udp_primitive
  : K_primitive IDENTIFIER '(' udp_port_list ')' ';'
      udp_port_decls
      udp_init_opt
      udp_body
    K_endprimitive
    {
      /* We will treat primitives like regular modules */
      db_add_module( $2, @1.text, @1.first_line );
      db_end_module( @10.first_line );
    }
  | K_primitive IGNORE K_endprimitive
  ;

udp_port_list
  : IDENTIFIER
    {
      $$ = 0;
    }
  | udp_port_list ',' IDENTIFIER
  ;

udp_port_decls
  : udp_port_decl
  | udp_port_decls udp_port_decl
  ;

udp_port_decl
  : K_input list_of_variables ';'
    {
      str_link_delete_list( $2 );
    }
  | K_output IDENTIFIER ';'
  | K_reg IDENTIFIER ';'
  ;

udp_init_opt
  : udp_initial
  |
  ;

udp_initial
  : K_initial IDENTIFIER '=' NUMBER ';'
  ;

udp_body
  : K_table { lex_start_udp_table(); }
      udp_entry_list
    K_endtable { lex_end_udp_table(); }
  ;

udp_entry_list
  : udp_comb_entry_list
  | udp_sequ_entry_list
  ;

udp_comb_entry_list
  : udp_comb_entry
  | udp_comb_entry_list udp_comb_entry
  ;

udp_comb_entry
  : udp_input_list ':' udp_output_sym ';'
  ;

udp_input_list
  : udp_input_sym
  | udp_input_list udp_input_sym
  ;

udp_input_sym
  : '0'
  | '1'
  | 'x'
  | '?'
  | 'b'
  | '*'
  | '%'
  | 'f'
  | 'F'
  | 'l'
  | 'h'
  | 'B'
  | 'r'
  | 'R'
  | 'M'
  | 'n'
  | 'N'
  | 'p'
  | 'P'
  | 'Q'
  | '_'
  | '+'
  ;

udp_output_sym
  : '0'
  | '1'
  | 'x'
  | '-'
  ;

udp_sequ_entry_list
  : udp_sequ_entry
  | udp_sequ_entry_list udp_sequ_entry
  ;

udp_sequ_entry
  : udp_input_list ':' udp_input_sym ':' udp_output_sym ';'
  ;

  /* This is the start of a module body */
module_item_list
  : module_item_list module_item
  | module_item
  ;

module_item
  : attribute_list_opt
    net_type range_opt list_of_variables ';'
    {
      str_link*     tmp  = $4;
      str_link*     curr = tmp;
      if( ($2 == 1) && ($3 != NULL) ) {
        /* Creating signal(s) */
        while( curr != NULL ) {
          db_add_signal( curr->str, $3->left, $3->right, FALSE, FALSE );
          curr = curr->next;
        }
      }
      str_link_delete_list( $4 );
      if( $3 != NULL ) {
        static_expr_dealloc( $3->left,  FALSE );
        static_expr_dealloc( $3->right, FALSE );
        free_safe( $3 );
        curr_sig_width = NULL;
      }
    }
  | attribute_list_opt
    net_type range_opt net_decl_assigns ';'
    {
      if( curr_sig_width != NULL ) {
        static_expr_dealloc( curr_sig_width->left, FALSE );
        static_expr_dealloc( curr_sig_width->right, FALSE );
        free_safe( curr_sig_width );
        curr_sig_width = NULL;
      }
    }
  | attribute_list_opt
    net_type drive_strength
    {
      static_expr*  left;
      static_expr*  right;
      if( (ignore_mode == 0) && ($2 == 1) ) {
        left = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        left->exp = NULL;
        left->num = 0;
        right = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        right->exp = NULL;
        right->num = 0;
        curr_sig_width = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
        curr_sig_width->left  = left;
        curr_sig_width->right = right;
      }
    }
    net_decl_assigns ';'
    {
      if( curr_sig_width != NULL ) {
        static_expr_dealloc( curr_sig_width->left, FALSE );
        static_expr_dealloc( curr_sig_width->right, FALSE );
        free_safe( curr_sig_width );
        curr_sig_width = NULL;
      }
    }
  | K_trireg charge_strength_opt range_opt delay3_opt list_of_variables ';'
    {
      str_link* tmp  = $5;
      str_link* curr = tmp;
      while( curr != NULL ) {
        db_add_signal( curr->str, $3->left, $3->right, FALSE, FALSE );
        curr = curr->next;
      }
      str_link_delete_list( $5 );
      static_expr_dealloc( $3->left, FALSE );
      static_expr_dealloc( $3->right, FALSE );
      free_safe( $3 );
      curr_sig_width = NULL;
    }
  | port_type range_opt list_of_variables ';'
    {
      /* Create signal -- implicitly this is a wire which may not be explicitly declared */
      str_link* tmp  = $3;
      str_link* curr = tmp;
      while( curr != NULL ) {
        db_add_signal( curr->str, $2->left, $2->right, ($1 == 1), FALSE );
        curr = curr->next;
      }
      str_link_delete_list( $3 );
      static_expr_dealloc( $2->left, FALSE );
      static_expr_dealloc( $2->right, FALSE );
      free_safe( $2 );
      curr_sig_width = NULL;
    }
  /* Handles Verilog-2001 port of type:  input wire [m:l] <list>; */
  | port_type net_type range_opt list_of_variables ';'
    {
      /* Create signal -- implicitly this is a wire which may not be explicitly declared */
      str_link* tmp  = $4;
      str_link* curr = tmp;
      while( curr != NULL ) {
        db_add_signal( curr->str, $3->left, $3->right, ($1 == 1), FALSE );
        curr = curr->next;
      }
      str_link_delete_list( $4 );
      static_expr_dealloc( $3->left, FALSE );
      static_expr_dealloc( $3->right, FALSE );
      free_safe( $3 );
      curr_sig_width = NULL;
    }
  | port_type range_opt error ';'
    {
      if( $2 != NULL ) {
        free_safe( $2 );
        curr_sig_width = NULL;
      }
      VLerror( "Invalid variable list in port declaration" );
    }
  | attribute_list_opt gatetype gate_instance_list ';'
    {
      str_link_delete_list( $3 );
    }
  | attribute_list_opt gatetype delay3 gate_instance_list ';'
    {
      str_link_delete_list( $4 );
    }
  | attribute_list_opt gatetype drive_strength gate_instance_list ';'
    {
      str_link_delete_list( $4 );
    }
  | attribute_list_opt gatetype drive_strength delay3 gate_instance_list ';'
    {
      str_link_delete_list( $5 );
    }
  | K_pullup gate_instance_list ';'
    {
      str_link_delete_list( $2 );
    }
  | K_pulldown gate_instance_list ';'
    {
      str_link_delete_list( $2 );
    }
  | K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      str_link_delete_list( $5 );
    }
  | K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      str_link_delete_list( $5 );
    }
  | block_item_decl
  | K_defparam defparam_assign_list ';'
    {
      snprintf( user_msg, USER_MSG_LENGTH, "Defparam found but not used, file: %s, line: %d.  Please use -P option to specify",
                @1.text, @1.first_line );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
    }
  | K_event list_of_variables ';'
    {
      str_link*   curr = $2;
      static_expr left;
      static_expr right;
      if( ignore_mode == 0 ) {
        left.exp  = NULL;
        left.num  = 0;
        right.exp = NULL;
        right.num = 0;
        while( curr != NULL ) {
          db_add_signal( curr->str, &left, &right, FALSE, TRUE );
          curr = curr->next;
        }
        str_link_delete_list( $2 );
      }
    }
  /* Handles instantiations of modules and user-defined primitives. */
  | IDENTIFIER parameter_value_opt gate_instance_list ';'
    {
      str_link* tmp  = $3;
      str_link* curr = tmp;
      exp_link* ecurr;
      while( curr != NULL ) {
        ecurr = param_exp_head;
        while( ecurr != NULL ){
          db_add_override_param( curr->str, ecurr->exp );
          ecurr = ecurr->next;
        }
        db_add_instance( curr->str, $1, FUNIT_MODULE );
        curr = curr->next;
      }
      str_link_delete_list( tmp );
      exp_link_delete_list( param_exp_head, FALSE );
      param_exp_head = NULL;
      param_exp_tail = NULL;
    }
  | K_assign drive_strength_opt { ignore_mode++; } delay3_opt { ignore_mode--; } assign_list ';'
  | attribute_list_opt
    K_always statement
    {
      statement* stmt = $3;
      if( stmt != NULL ) {
        db_statement_connect( stmt, stmt );
        db_statement_set_stop( stmt, stmt, TRUE );
        stmt->exp->suppl.part.stmt_head = 1;
        db_add_statement( stmt, stmt );
      }
    }
  | attribute_list_opt
    K_initial statement
    {
      statement* stmt = $3;
      if( stmt != NULL ) {
        db_statement_set_stop( stmt, NULL, FALSE );
        stmt->exp->suppl.part.stmt_head = 1;
        db_add_statement( stmt, stmt );
      }
    }
  | K_task IDENTIFIER ';'
    {
      if( ignore_mode == 0 ) {
        db_add_function_task( FUNIT_TASK, $2, @2.text, @2.first_line );
      }
    }
    task_item_list_opt statement_opt
    {
      statement* stmt = $6;
      if( (ignore_mode == 0) && (stmt != NULL) ) {
        db_statement_set_stop( stmt, NULL, FALSE );
        stmt->exp->suppl.part.stmt_head = 1;
        db_add_statement( stmt, stmt );
      }
    }
    K_endtask
    {
      if( ignore_mode == 0 ) {
        db_end_function_task( @8.first_line );
      }
    }
  | K_function range_or_type_opt IDENTIFIER ';'
    {
      char tmp[256];
      if( ignore_mode == 0 ) {
        db_add_function_task( FUNIT_FUNCTION, $3, @3.text, @3.first_line );
        snprintf( tmp, 256, "%s", $3 );
        db_add_signal( tmp, $2->left, $2->right, FALSE, FALSE );
        static_expr_dealloc( $2->left, FALSE );
        static_expr_dealloc( $2->right, FALSE );
        free_safe( $2 );
        free_safe( $3 );
      }
    }
    function_item_list statement
    {
      statement* stmt = $7;
      if( (ignore_mode == 0) && (stmt != NULL) ) {
        db_statement_set_stop( stmt, NULL, FALSE );
        stmt->exp->suppl.part.stmt_head = 1;
        db_add_statement( stmt, stmt );
      }
    }
    K_endfunction
    {
      if( ignore_mode == 0 ) {
        db_end_function_task( @9.first_line );
      }
    }
  | K_specify ignore_more specify_item_list ignore_less K_endspecify
  | K_specify K_endspecify
  | K_specify error K_endspecify
  | error ';'
    {
      VLerror( "Invalid module item.  Did you forget an initial or always?" );
    }
  | K_assign error '=' { ignore_mode++; } expression { ignore_mode--; } ';'
    {
      VLerror( "Syntax error in left side of continuous assignment" );
    }
  | K_assign error ';'
    {
      VLerror( "Syntax error in continuous assignment" );
    }
  | K_function error K_endfunction
    {
      /* yyerror( @1, "error: I give up on this function definition" );
         yyerrok; */
    }
  | KK_attribute '(' { ignore_mode++; } UNUSED_IDENTIFIER ',' UNUSED_STRING ',' UNUSED_STRING { ignore_mode--; }')' ';'
  | KK_attribute '(' error ')' ';'
    {
      /* yyerror( @1, "error: Misformed $attribute parameter list."); */
    }
  ;

statement
  : K_assign { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_deassign { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_force { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_release { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_begin statement_list K_end
    {
      $$ = $2;
    }
  | K_begin ':' named_begin_end_block K_end
    {
      $$ = $3;
    }
  | K_begin K_end
    {
      $$ = NULL;
    }
  | K_begin error K_end
    {
      VLerror( "Illegal syntax in begin/end block" );
      $$ = NULL;
    }
  | K_fork { ignore_mode++; } fork_statement { ignore_mode--; } K_join
    {
      $$ = NULL;
    }
  | K_disable { ignore_mode++; } identifier ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_TRIGGER IDENTIFIER ';'
    {
      expression* expr;
      statement*  stmt;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        expr = db_create_expression( NULL, NULL, EXP_OP_TRIGGER, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
        db_add_expression( expr );
        stmt = db_create_statement( expr );
        $$   = stmt;
      } else {
        $$ = NULL;
      }
    }
  | K_forever { ignore_mode++; } statement { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_fork { ignore_mode++; } statement_list K_join { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_repeat { ignore_mode++; } '(' expression ')' statement { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_case '(' expression ')' case_items K_endcase
    {
      expression*     expr;
      expression*     c_expr    = $3;
      statement*      stmt      = NULL;
      statement*      last_stmt = NULL;
      case_statement* c_stmt    = $5;
      case_statement* tc_stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        c_expr->suppl.part.root = 1;
        while( c_stmt != NULL ) {
          if( c_stmt->expr != NULL ) {
            expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASE, lhs_mode, c_stmt->line, 0, 0, NULL );
          } else {
            expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
          }
          db_add_expression( expr );
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, c_stmt->stmt );
          db_connect_statement_false( stmt, last_stmt );
          db_statement_set_stop( c_stmt->stmt, NULL, FALSE );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = stmt;
      } else {
        expression_dealloc( $3, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          db_remove_statement( c_stmt->stmt );
          /* statement_dealloc_recursive( c_stmt->stmt ); */
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  | K_casex '(' expression ')' case_items K_endcase
    {
      expression*     expr;
      expression*     c_expr    = $3;
      statement*      stmt      = NULL;
      statement*      last_stmt = NULL;
      case_statement* c_stmt    = $5;
      case_statement* tc_stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        c_expr->suppl.part.root = 1;
        while( c_stmt != NULL ) {
          if( c_stmt->expr != NULL ) {
            expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEX, lhs_mode, c_stmt->line, 0, 0, NULL );
          } else {
            expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
          }
          db_add_expression( expr );
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, c_stmt->stmt );
          db_connect_statement_false( stmt, last_stmt );
          db_statement_set_stop( c_stmt->stmt, NULL, FALSE );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = stmt;
      } else {
        expression_dealloc( $3, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          db_remove_statement( c_stmt->stmt );
          /* statement_dealloc_recursive( c_stmt->stmt ); */
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  | K_casez '(' expression ')' case_items K_endcase
    {
      expression*     expr;
      expression*     c_expr    = $3;
      statement*      stmt      = NULL;
      statement*      last_stmt = NULL;
      case_statement* c_stmt    = $5;
      case_statement* tc_stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        c_expr->suppl.part.root = 1;
        while( c_stmt != NULL ) {
          if( c_stmt->expr != NULL ) {
            expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEZ, lhs_mode, c_stmt->line, 0, 0, NULL );
          } else {
            expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
          }
          db_add_expression( expr );
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, c_stmt->stmt );
          db_connect_statement_false( stmt, last_stmt );
          db_statement_set_stop( c_stmt->stmt, NULL, FALSE );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = stmt;
      } else {
        expression_dealloc( $3, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          db_remove_statement( c_stmt->stmt );
          /* statement_dealloc_recursive( c_stmt->stmt ); */
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  | K_case '(' expression ')' error K_endcase
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $3, FALSE );
      }
      VLerror( "Illegal case expression" );
      $$ = NULL;
    }
  | K_casex '(' expression ')' error K_endcase
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $3, FALSE );
      }
      VLerror( "Illegal casex expression" );
      $$ = NULL;
    }
  | K_casez '(' expression ')' error K_endcase
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $3, FALSE );
      }
      VLerror( "Illegal casez expression" );
      $$ = NULL;
    }
  | K_if '(' expression ')' statement_opt %prec less_than_K_else
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        tmp  = db_create_expression( $3, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $3->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        db_connect_statement_true( stmt, $5 );
        db_statement_set_stop( $5, NULL, FALSE );
        $$ = stmt;
      } else {
        db_remove_statement( $5 );
        $$ = NULL;
      }
    }
  | K_if '(' expression ')' statement_opt K_else statement_opt
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        tmp  = db_create_expression( $3, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $3->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        db_connect_statement_true( stmt, $5 );
        db_connect_statement_false( stmt, $7 );
        db_statement_set_stop( $5, NULL, FALSE );
        $$ = stmt;
      } else {
        db_remove_statement( $5 );
        db_remove_statement( $7 );
        $$ = NULL;
      }
    }
  | K_if '(' error ')' { ignore_mode++; } if_statement_error { ignore_mode--; }
    {
      VLerror( "Illegal conditional if expression" );
      $$ = NULL;
    }
  | K_for { ignore_mode++; } for_statement { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_while { ignore_mode++; } while_statement { ignore_mode--; }
    {
      $$ = NULL;
    }
  | delay1 statement_opt
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        stmt = db_create_statement( $1 );
        db_add_expression( $1 );
        if( $2 != NULL ) {
          db_statement_connect( stmt, $2 );
        }
        $$ = stmt;
      } else {
        db_remove_statement( $2 );
        $$ = NULL;
      }
    }
  | event_control statement_opt
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        stmt = db_create_statement( $1 );
        db_add_expression( $1 );
        if( $2 != NULL ) {
          db_statement_connect( stmt, $2 );
        }
        $$ = stmt;
      } else {
        db_remove_statement( $2 );
        $$ = NULL;
      }
    }
  | lpvalue '=' expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp  = db_create_expression( $3, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $3->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_LE expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp  = db_create_expression( $3, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $3->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue '=' delay1 expression ';'
    {
      expression* tmp;
      statement*  stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) ) {
        tmp  = db_create_expression( $4, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $4->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_LE delay1 expression ';'
    {
      expression* tmp;
      statement*  stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) ) {
        tmp  = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $4->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue '=' event_control expression ';'
    {
      expression* tmp;
      statement*  stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) ) {
        tmp  = db_create_expression( $4, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $4->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_LE event_control expression ';'
    {
      expression* tmp;
      statement*  stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) ) {
        tmp  = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $4->value;
        stmt = db_create_statement( tmp );
        db_add_expression( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue '=' K_repeat { ignore_mode++; } '(' expression ')' event_control expression ';' { ignore_mode--; }
    {
      expression_dealloc( $1, FALSE );
      $$ = NULL;
    }
  | lpvalue K_LE K_repeat { ignore_mode++; } '(' expression ')' event_control expression ';' { ignore_mode--; }
    {
      expression_dealloc( $1, FALSE );
      $$ = NULL;
    }
  | K_wait '(' expression ')' statement_opt
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        stmt = db_create_statement( $3 );
        db_add_expression( $3 );
        db_connect_statement_true( stmt, $5 );
        $$ = stmt;
      } else {
        db_remove_statement( $5 );
        $$ = NULL;
      }
    }
  | SYSTEM_IDENTIFIER { ignore_mode++; } '(' expression_list ')' ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | UNUSED_SYSTEM_IDENTIFIER '(' expression_list ')' ';'
    {
      $$ = NULL;
    }
  | SYSTEM_IDENTIFIER ';'
    {
      $$ = NULL;
    }
  | UNUSED_SYSTEM_IDENTIFIER ';'
    {
      $$ = NULL;
    }
  | identifier '(' expression_list ')' ';'
    {
      expression* exp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        exp  = db_create_expression( NULL, $3, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.first_column, (@5.last_line - 1), $1 );
        stmt = db_create_statement( exp );
        db_add_expression( exp );
        $$   = stmt;
        free_safe( $1 );
      } else {
        if( $1 != NULL ) {
          free_safe( $1 );
        }
        $$ = NULL;
      }
    }
  | identifier ';'
    {
      expression* exp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        exp  = db_create_expression( NULL, NULL, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.first_column, (@2.last_line - 1), $1 );
        stmt = db_create_statement( exp );
        db_add_expression( exp );
        $$   = stmt;
        free_safe( $1 );
      } else {
        if( $1 != NULL ) {
          free_safe( $1 );
        }
        $$ = NULL;
      }
    }
  | error ';'
    {
      VLerror( "Illegal statement" );
      $$ = NULL;
    }
  ;

for_statement
  : '(' lpvalue '=' expression ';' expression ';' lpvalue '=' expression ')' statement
    {
      $$ = NULL;
    }
  | '(' lpvalue '=' expression ';' expression ';' error ')' statement
    {
      $$ = NULL;
    }
  | '(' lpvalue '=' expression ';' error ';' lpvalue '=' expression ')' statement
    {
      $$ = NULL;
    }
  | '(' error ')' statement
    {
      $$ = NULL;
    }
  ;

fork_statement
  : ':' UNUSED_IDENTIFIER block_item_decls_opt statement_list
    {
      $$ = NULL;
    }
  | ':' UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  |
    {
      $$ = NULL;
    }
  ;

while_statement
  : '(' expression ')' statement
    {
      $$ = NULL;
    }
  | '(' error ')' statement
    {
      $$ = NULL;
    }
  ;

named_begin_end_block
  : IDENTIFIER ignore_more block_item_decls_opt ignore_less statement_list
    {
      if( $1 != NULL ) {
        free_safe( $1 );
      }
      $$ = $5;
    }
  | UNUSED_IDENTIFIER block_item_decls_opt statement_list
    {
      $$ = NULL;
    }
  | IDENTIFIER K_end 
    {
      if( $1 != NULL ) {
        free_safe( $1 );
      }
      $$ = NULL;
    }
  | UNUSED_IDENTIFIER K_end
    {
      $$ = NULL;
    }
  ;

if_statement_error
  : statement_opt %prec less_than_K_else
    {
      $$ = NULL;
    }
  | statement_opt K_else statement_opt
    {
      $$ = NULL;
    }
  ;

statement_list
  : statement_list statement
    {
      if( ignore_mode == 0 ) {
        if( $1 == NULL ) {
          db_remove_statement( $2 );
          $$ = NULL;
        } else {
          if( $2 != NULL ) {
            db_statement_connect( $1, $2 );
            $$ = $1;
          } else {
            db_remove_statement( $1 );
            $$ = NULL;
          }
        }
      }
    }
  | statement
    {
      $$ = $1;
    }
  ;

statement_opt
  : statement
    {
      $$ = $1;
    }
  | ';'
    {
      $$ = NULL;
    }
  ;

  /* An lpvalue is the expression that can go on the left side of a procedural assignment.
     This rule handles only procedural assignments. */
lpvalue
  : identifier
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        free_safe( $1 );
        $$  = tmp;
      } else {
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | identifier start_lhs '[' expression ']' end_lhs
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) ) {
        tmp = db_create_expression( NULL, $4, EXP_OP_SBIT_SEL, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), $1 );
        free_safe( $1 );
        $$  = tmp;
      } else {
        expression_dealloc( $4, FALSE );
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | identifier start_lhs '[' expression ':' expression ']' end_lhs
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) && ($6 != NULL) ) {
        tmp = db_create_expression( $6, $4, EXP_OP_MBIT_SEL, TRUE, @1.first_line, @1.first_column, (@7.last_column - 1), $1 );
        free_safe( $1 );
        $$  = tmp;
      } else {
        expression_dealloc( $4, FALSE );
        expression_dealloc( $6, FALSE );
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | '{' start_lhs expression_list end_lhs '}'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        $$  = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$  = NULL;
      }
    }
  ;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
  : identifier
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        free_safe( $1 );
        $$  = tmp;
      } else {
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | identifier start_lhs '[' expression ']' end_lhs
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) ) {
        tmp = db_create_expression( NULL, $4, EXP_OP_SBIT_SEL, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), $1 );
        free_safe( $1 );
        $$  = tmp;
      } else {
        expression_dealloc( $4, FALSE );
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | identifier start_lhs '[' expression ':' expression ']' end_lhs
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($4 != NULL) && ($6 != NULL) ) {
        tmp = db_create_expression( $6, $4, EXP_OP_MBIT_SEL, TRUE, @1.first_line, @1.first_column, (@7.last_column - 1), $1 );
        free_safe( $1 );
        $$  = tmp;
      } else {
        expression_dealloc( $4, FALSE );
        expression_dealloc( $6, FALSE );
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | '{' start_lhs expression_list end_lhs '}'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        $$  = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$  = NULL;
      }
    }
  ;

block_item_decls_opt
  : block_item_decls
  |
  ;

block_item_decls
  : block_item_decl
  | block_item_decls block_item_decl
  ;

  /* The block_item_decl is used in function definitions, task
     definitions, module definitions and named blocks. Wherever a new
     scope is entered, the source may declare new registers and
     integers. This rule matches those declarations. The containing
     rule has presumably set up the scope. */
block_item_decl
  : K_reg range register_variable_list ';'
    {
      /* Create new signal */
      str_link* tmp  = $3;
      str_link* curr = tmp;
      if( ignore_mode == 0 ) {
        while( curr != NULL ) {
          db_add_signal( curr->str, $2->left, $2->right, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( tmp );
        static_expr_dealloc( $2->left, FALSE );
        static_expr_dealloc( $2->right, FALSE );
        free_safe( $2 );
      }
    }
  | K_reg register_variable_list ';'
    {
      /* Create new signal */
      str_link*   tmp  = $2;
      str_link*   curr = tmp;
      static_expr left;
      static_expr right;
      if( ignore_mode == 0 ) {
        left.num  = 0;
        right.num = 0;
        left.exp  = NULL;
        right.exp = NULL;
        while( curr != NULL ) {
          db_add_signal( curr->str, &left, &right, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( tmp );
      }
    }
  | K_reg K_signed range register_variable_list ';'
    {
      /* Create new signal */
      str_link* tmp  = $4;
      str_link* curr = tmp;
      if( ignore_mode == 0 ) {
        while( curr != NULL ) {
          db_add_signal( curr->str, $3->left, $3->right, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( tmp );
        static_expr_dealloc( $3->left, FALSE );
        static_expr_dealloc( $3->right, FALSE );
        free_safe( $3 );
      }
    }
  | K_reg K_signed register_variable_list ';'
    {
      /* Create new signal */
      str_link*   tmp  = $3;
      str_link*   curr = tmp;
      static_expr left;
      static_expr right;
      if( ignore_mode == 0 ) {
        left.num  = 1;
        right.num = 0;
        left.exp  = NULL;
        right.exp = NULL;
        while( curr != NULL ) {
          db_add_signal( curr->str, &left, &right, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( tmp );
      }
    }
  | K_integer register_variable_list ';'
    {
      str_link* curr = $2;
      char      tmp[256];
      if( ignore_mode == 0 ) {
        while( curr != NULL ) {
          snprintf( tmp, 256, "!%s", curr->str );
          db_add_signal( tmp, NULL, NULL, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( $2 );
      }
    }
  | K_time register_variable_list ';'
    {
      str_link* curr = $2;
      char      tmp[256];
      if( ignore_mode == 0 ) {
        while( curr != NULL ) {
          snprintf( tmp, 256, "!%s", curr->str );
          db_add_signal( tmp, NULL, NULL, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( $2 );
      }
    }
  | K_real list_of_variables ';'
    {
      str_link* curr = $2;
      char      tmp[256];
      if( ignore_mode == 0 ) {
        while( curr != NULL ) {
          snprintf( tmp, 256, "!%s", curr->str );
          db_add_signal( tmp, NULL, NULL, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( $2 );
      }
    }
  | K_realtime list_of_variables ';'
    {
      str_link* curr = $2;
      char      tmp[256];
      if( ignore_mode == 0 ) {
        while( curr != NULL ) {
          snprintf( tmp, 256, "!%s", curr->str );
          db_add_signal( tmp, NULL, NULL, FALSE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( $2 );
      }
    }
  | K_parameter parameter_assign_list ';'
  | K_localparam localparam_assign_list ';'
  | K_reg error ';'
    {
      VLerror( "Syntax error in reg variable list" );
    }
  | K_parameter error ';'
    {
      VLerror( "Syntax error in parameter variable list" );
    }
  | K_localparam error ';'
    {
      VLerror( "Syntax error in localparam list" );
    }
  ;	

case_item
  : expression_list ':' statement_opt
    {
      case_statement* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ), __FILE__, __LINE__ );
        cstmt->prev = NULL;
        cstmt->expr = $1;
        cstmt->stmt = $3;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | K_default ':' statement_opt
    {
      case_statement* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ), __FILE__, __LINE__ );
        cstmt->prev = NULL;
        cstmt->expr = NULL;
        cstmt->stmt = $3;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | K_default statement_opt
    {
      case_statement* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ), __FILE__, __LINE__ );
        cstmt->prev = NULL;
        cstmt->expr = NULL;
        cstmt->stmt = $2;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | error { ignore_mode++; } ':' statement_opt { ignore_mode--; }
    {
      VLerror( "Illegal case expression" );
    }
  ;

case_items
  : case_items case_item
    {
      case_statement* list = $1;
      case_statement* curr = $2;
      if( ignore_mode == 0 ) {
        curr->prev = list;
        $$ = curr;
      } else {
        $$ = NULL;
      }
    }
  | case_item
    {
      $$ = $1;
    }
  ;

delay1
  : '#' delay_value_simple
    {
      vector*     vec;
      expression* exp; 
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        vec = vector_create( 32, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @2.first_column, (@2.last_column - 1), NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL ); 
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $2, tmp, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        $$  = exp;
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ')'
    {
      vector*     vec;
      expression* exp;
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        vec = vector_create( 32, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @3.first_column, (@3.last_column - 1), NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $3, tmp, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        $$  = exp;
      } else {
        $$ = NULL;
      }
    }
  ;

delay3
  : '#' delay_value_simple
    {
      vector*     vec;
      expression* exp; 
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        vec = vector_create( 32, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @2.first_column, (@2.last_column - 1), NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $2, tmp, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        $$  = exp;
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ')'
    {
      vector*     vec;
      expression* exp; 
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        vec = vector_create( 32, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @3.first_column, (@3.last_column - 1), NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $3, tmp, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        $$  = exp;
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ',' delay_value ')'
    {
      vector*     vec;
      expression* exp; 
      expression* tmp;
      expression_dealloc( $5, FALSE );
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        vec = vector_create( 32, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @3.first_column, (@5.last_column - 1), NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $3, tmp, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), NULL );
        $$  = exp;
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ',' delay_value ',' delay_value ')'
    {
      vector*     vec;
      expression* exp; 
      expression* tmp;
      expression_dealloc( $3, FALSE );
      expression_dealloc( $7, FALSE );
      if( ignore_mode == 0 ) {
        vec = vector_create( 32, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @3.first_column, (@7.last_column - 1), NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $5, tmp, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
        $$  = exp;
      } else {
        $$ = NULL;
      }
    }
  ;

delay3_opt
  : delay3
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

delay_value
  : static_expr
    {
      expression*  tmp;
      static_expr* se = $1;
      if( (ignore_mode == 0) && (se != NULL) ) {
        if( se->exp == NULL ) {
          tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          vector_init( tmp->value, (vec_data*)malloc_safe( (sizeof( vec_data ) * 32), __FILE__, __LINE__ ), 32 );  
          vector_from_int( tmp->value, se->num );
        } else {
          tmp = se->exp;
          static_expr_dealloc( se, FALSE );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | static_expr ':' static_expr ':' static_expr
    {
      expression*  tmp;
      static_expr* se = NULL;
      if( ignore_mode == 0 ) {
        if( delay_expr_type == DELAY_EXPR_DEFAULT ) {
          snprintf( user_msg,
                    USER_MSG_LENGTH,
                    "Delay expression type for min:typ:max not specified, using default of 'typ', file %s, line %d",
                    @1.text,
                    @1.first_line );
          print_output( user_msg, WARNING, __FILE__, __LINE__ );
        }
        switch( delay_expr_type ) {
          case DELAY_EXPR_MIN :
            se = $1;
            static_expr_dealloc( $3, TRUE );
            static_expr_dealloc( $5, TRUE );
            break;
          case DELAY_EXPR_DEFAULT :
          case DELAY_EXPR_TYP     :
            se = $3;
            static_expr_dealloc( $1, TRUE );
            static_expr_dealloc( $5, TRUE );
            break;
          case DELAY_EXPR_MAX :
            se = $5;
            static_expr_dealloc( $1, TRUE );
            static_expr_dealloc( $3, TRUE );
            break;
          default:
            assert( (delay_expr_type == DELAY_EXPR_MIN) ||
                    (delay_expr_type == DELAY_EXPR_TYP) ||
                    (delay_expr_type == DELAY_EXPR_MAX) );
            break;
        }
        if( se != NULL ) {
          if( se->exp == NULL ) {
            tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            vector_init( tmp->value, (vec_data*)malloc_safe( (sizeof( vec_data ) * 32), __FILE__, __LINE__ ), 32 );
            vector_from_int( tmp->value, se->num );
          } else {
            tmp = se->exp;
            free_safe( se );
          }
          $$ = tmp;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

delay_value_simple
  : NUMBER
    {
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
      assert( tmp->value->value == NULL );
      free_safe( tmp->value );
      tmp->value = $1;
      $$ = tmp;
    }
  | UNUSED_NUMBER
    {
      $$ = NULL;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | UNUSED_REALTIME
    {
      $$ = NULL;
    }
  | IDENTIFIER
    {
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
      $$ = tmp;
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  ;

assign_list
  : assign_list ',' assign
  | assign
  ;

assign
  : lavalue '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( ($1 != NULL) && ($3 != NULL) ) {
        tmp  = db_create_expression( $3, $1, EXP_OP_ASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        vector_dealloc( tmp->value );
        tmp->value = $3->value;
        stmt = db_create_statement( tmp );
        stmt->exp->suppl.part.stmt_head = 1;
        stmt->exp->suppl.part.stmt_stop = 1;
        stmt->exp->suppl.part.stmt_cont = 1;
        /* Statement will be looped back to itself */
        db_connect_statement_true( stmt, stmt );
        db_connect_statement_false( stmt, stmt );
        db_add_expression( tmp );
        db_add_statement( stmt, stmt );
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
      }
    }
  ;

range_opt
  : range
    {
      curr_sig_width = $1;
      $$ = $1;
    }
  |
    {
      vector_width* tmp;
      static_expr*  left;
      static_expr*  right;
      if( ignore_mode == 0 ) {
        left = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        left->exp = NULL;
        left->num = 0;
        right = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        right->exp = NULL;
        right->num = 0;
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
        tmp->left  = left;
        tmp->right = right;
        curr_sig_width = tmp;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

range
  : '[' static_expr ':' static_expr ']'
    {
      vector_width* tmp;
      if( ignore_mode == 0 ) {
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
        tmp->left  = $2;
        tmp->right = $4;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

range_or_type_opt
  : range      
    { 
      vector_width* tmp = $1;
      if( ignore_mode == 0 ) {
        $$ = tmp;
      } else {
        if( tmp->left != NULL ) {
          free_safe( tmp->left );
        } else {
          free_safe( tmp->right );
        }
        free_safe( tmp );
        $$ = NULL;
      }
    }
  | K_integer
    {
      vector_width* tmp;
      static_expr*  left;
      static_expr*  right;
      if( ignore_mode == 0 ) {
        left = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        left->exp = NULL;
        left->num = 31;
        right = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        right->exp = NULL;
        right->num = 0;
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
        tmp->left  = left;
        tmp->right = right;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_real     { $$ = NULL; }
  | K_realtime { $$ = NULL; }
  | K_time
    {
      vector_width* tmp;
      static_expr*  left;
      static_expr*  right;
      if( ignore_mode == 0 ) {
        left = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        left->exp = NULL;
        left->num = 63;
        right = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        right->exp = NULL;
        right->num = 0;
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
        tmp->left  = left;
        tmp->right = right;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  |
    {
      vector_width* tmp;
      static_expr*  left;
      static_expr*  right;
      if( ignore_mode == 0 ) {
        left = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        left->exp = NULL;
        left->num = 0;
        right = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
        right->exp = NULL;
        right->num = 0;
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
        tmp->left  = left;
        tmp->right = right;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

  /* The register_variable rule is matched only when I am parsing
     variables in a "reg" definition. I therefore know that I am
     creating registers and I do not need to let the containing rule
     handle it. The register variable list simply packs them together
     so that bit ranges can be assigned. */
register_variable
  : IDENTIFIER
    {
      $$ = $1;
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | IDENTIFIER '=' expression
    {
      $$ = $1;
    }
  | UNUSED_IDENTIFIER '=' expression
    {
      $$ = NULL;
    }
  | IDENTIFIER '[' static_expr ':' static_expr ']'
    {
      /* We don't support memory coverage */
      char* name;
      if( $1 != NULL ) {
        name = (char*)malloc_safe( (strlen( $1 ) + 2), __FILE__, __LINE__ );
        snprintf( name, (strlen( $1 ) + 2), "!%s", $1 );
        free_safe( $1 );
        $$ = name;
      } else {
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER '[' static_expr ':' static_expr ']'
    {
      $$ = NULL;
    }
  ;

register_variable_list
  : register_variable
    {
      str_link* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        tmp = (str_link*)malloc( sizeof( str_link ) );
        tmp->str  = $1;
        tmp->next = NULL;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | register_variable_list ',' register_variable
    {
      str_link* tmp;
      if( ignore_mode == 0 ) {
        if( $3 != NULL ) {
          tmp = (str_link*)malloc( sizeof( str_link ) );
          tmp->str  = $3;
          tmp->next = $1;
          $$ = tmp;
        } else {
          $$ = $1;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

task_item_list_opt
  : task_item_list
  |
  ;

task_item_list
  : task_item_list task_item
  | task_item
  ;

task_item
  : block_item_decl
  | port_type range_opt list_of_variables ';'
    {
      if( ignore_mode == 0 ) {
        /* Create signal -- implicitly this is a wire which may not be explicitly declared */
        str_link* tmp  = $3;
        str_link* curr = tmp;
        while( curr != NULL ) {
          db_add_signal( curr->str, $2->left, $2->right, TRUE, FALSE );
          curr = curr->next;
        }
        str_link_delete_list( $3 );
        static_expr_dealloc( $2->left, FALSE );
        static_expr_dealloc( $2->right, FALSE );
        free_safe( $2 );
        curr_sig_width = NULL;
      }
    }
  ;

net_type
  : K_wire    { $$ = 1; }
  | K_tri     { $$ = 1; }
  | K_tri1    { $$ = 1; }
  | K_supply0 { $$ = 1; }
  | K_wand    { $$ = 1; }
  | K_triand  { $$ = 1; }
  | K_tri0    { $$ = 1; }
  | K_supply1 { $$ = 1; }
  | K_wor     { $$ = 1; }
  | K_trior   { $$ = 1; }
  ;

net_decl_assigns
  : net_decl_assigns ',' net_decl_assign
  | net_decl_assign
  ;

net_decl_assign
  : IDENTIFIER '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && (curr_sig_width != NULL) ) {
        db_add_signal( $1, curr_sig_width->left, curr_sig_width->right, FALSE, FALSE );
        if( $3 != NULL ) {
          tmp  = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          tmp  = db_create_expression( $3, tmp, EXP_OP_DASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          stmt = db_create_statement( tmp );
          stmt->exp->suppl.part.stmt_head = 1;
          stmt->exp->suppl.part.stmt_stop = 1;
          stmt->exp->suppl.part.stmt_cont = 1;
          /* Statement will be looped back to itself */
          db_connect_statement_true( stmt, stmt );
          db_connect_statement_false( stmt, stmt );
          db_add_expression( tmp );
          db_add_statement( stmt, stmt );
        }
        free_safe( $1 );
      }
    }
  | UNUSED_IDENTIFIER '=' expression
  | delay1 IDENTIFIER '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($2 != NULL) && (curr_sig_width != NULL) ) {
        db_add_signal( $2, curr_sig_width->left, curr_sig_width->right, FALSE, FALSE );
        if( $4 != NULL ) {
          tmp  = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.first_column, (@2.last_column - 1), $2 );
          tmp  = db_create_expression( $4, tmp, EXP_OP_DASSIGN, FALSE, @2.first_line, @2.first_column, (@4.last_column - 1), NULL );
          stmt = db_create_statement( tmp );
          stmt->exp->suppl.part.stmt_head = 1;
          stmt->exp->suppl.part.stmt_stop = 1;
          stmt->exp->suppl.part.stmt_cont = 1;
          /* Statement will be looped back to itself */
          db_connect_statement_true( stmt, stmt );
          db_connect_statement_false( stmt, stmt );
          db_add_expression( tmp );
          db_add_statement( stmt, stmt );
        }
        free_safe( $2 );
      }
    }
  ;

drive_strength_opt
  : drive_strength
  |
  ;

drive_strength
  : '(' dr_strength0 ',' dr_strength1 ')'
  | '(' dr_strength1 ',' dr_strength0 ')'
  | '(' dr_strength0 ',' K_highz1 ')'
  | '(' dr_strength1 ',' K_highz0 ')'
  | '(' K_highz1 ',' dr_strength0 ')'
  | '(' K_highz0 ',' dr_strength1 ')'
  ;

dr_strength0
  : K_supply0
  | K_strong0
  | K_pull0
  | K_weak0
  ;

dr_strength1
  : K_supply1
  | K_strong1
  | K_pull1
  | K_weak1
  ;

event_control
  : '@' IDENTIFIER
    {
      vsignal*    sig;
      expression* tmp;
      if( ignore_mode == 0 ) {
        sig = db_find_signal( $2 );
        if( sig != NULL ) {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
          vector_dealloc( tmp->value );
          tmp->value = sig->value;
          $$ = tmp;
        } else {
          $$ = NULL;
        }
        free_safe( $2 );
      } else {
        $$ = NULL;
      }
    }
  | '@' UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | '@' '(' event_expression_list ')'
    {
      $$ = $3;
    }
  | '@' '(' error ')'
    {
      VLerror( "Illegal event control expression" );
      $$ = NULL;
    }
  ;

event_expression_list
  : event_expression
    {
      $$ = $1;
    }
  | event_expression_list K_or event_expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | event_expression_list ',' event_expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        tmp = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  ;

event_expression
  : K_posedge expression
    {
      expression* tmp1;
      expression* tmp2;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        /* Create 1-bit expression to hold last value of right expression */
        tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        expression_create_value( tmp1, 1, FALSE );
        tmp2 = db_create_expression( $2, tmp1, EXP_OP_PEDGE, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        $$ = tmp2;
      } else {
        $$ = NULL;
      }
    }
  | K_negedge expression
    {
      expression* tmp1;
      expression* tmp2;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        expression_create_value( tmp1, 1, FALSE );
        tmp2 = db_create_expression( $2, tmp1, EXP_OP_NEDGE, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        $$ = tmp2;
      } else {
        $$ = NULL;
      }
    }
  | expression
    {
      expression* tmp1;
      expression* tmp2;
      expression* expr = $1;
      if( (ignore_mode == 0) && ($1 != NULL ) ) {
        tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        tmp2 = db_create_expression( expr, tmp1, EXP_OP_AEDGE, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        expression_create_value( tmp1, expr->value->width, FALSE );
        $$ = tmp2;
      } else {
        $$ = NULL;
      }
    }
  ;
		
charge_strength_opt
  : charge_strength
  |
  ;

charge_strength
  : '(' K_small ')'
  | '(' K_medium ')'
  | '(' K_large ')'
  ;

port_type
  : K_input
    {
      $$ = 1;
    }
  | K_output
    {
      $$ = 0;
    }
  | K_inout
    {
      $$ = 1;
    }
  ;

defparam_assign_list
  : defparam_assign
    {
      $$ = 0;
    }
  | range defparam_assign
    {
      $$ = 0;
    }
  | defparam_assign_list ',' defparam_assign
  ;

defparam_assign
  : identifier '=' expression
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $3, FALSE );
        free_safe( $1 );
      }
    }
  ;

 /* Parameter override */
parameter_value_opt
  : '#' '(' { param_mode++; } expression_list { param_mode--; } ')'
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $4, FALSE );
      }
    }
  | '#' '(' parameter_value_byname_list ')'
  | '#' NUMBER
    {
      vector_dealloc( $2 );
    }
  | '#' UNUSED_NUMBER
  | '#' error
    {
      VLerror( "Syntax error in parameter value assignment list" );
    }
  |
  ;

parameter_value_byname_list
  : parameter_value_byname
  | parameter_value_byname_list ',' parameter_value_byname
  ;

parameter_value_byname
  : '.' IDENTIFIER '(' expression ')'
    {
      expression_dealloc( $4, FALSE );
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' expression ')'
  | '.' IDENTIFIER '(' ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' ')'
  ;

gate_instance_list
  : gate_instance_list ',' gate_instance
    {
      str_link* tmp;
      if( ignore_mode == 0 ) {
        tmp = (str_link*)malloc( sizeof( str_link ) );
        tmp->str  = $3;
        tmp->next = $1;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | gate_instance
    {
      str_link* tmp;
      if( ignore_mode == 0 ) {
        tmp = (str_link*)malloc( sizeof( str_link ) );
        tmp->str  = $1;
        tmp->next = NULL;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

  /* A gate_instance is a module instantiation or a built in part
     type. In any case, the gate has a set of connections to ports. */
gate_instance
  : IDENTIFIER '(' { ignore_mode++; } expression_list { ignore_mode--; } ')'
    {
      if( ignore_mode == 0 ) {
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER '(' expression_list ')'
    {
      $$ = NULL;
    }
  | IDENTIFIER { ignore_mode++; } range '(' expression_list { ignore_mode--; } ')'
    {
      if( ignore_mode == 0 ) {
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  | '(' { ignore_mode++; } expression_list { ignore_mode--; } ')'
    {
      $$ = NULL;
    }
  | IDENTIFIER '(' port_name_list ')'
    {
      $$ = $1;
    }
  | UNUSED_IDENTIFIER '(' port_name_list ')'
    {
      $$ = NULL;
    }
  ;

gatetype
  : K_and
  | K_nand
  | K_or
  | K_nor
  | K_xor
  | K_xnor
  | K_buf
  | K_bufif0
  | K_bufif1
  | K_not
  | K_notif0
  | K_notif1
  | K_nmos
  | K_rnmos
  | K_pmos
  | K_rpmos
  | K_cmos
  | K_rcmos
  | K_tran
  | K_rtran
  | K_tranif0
  | K_tranif1
  | K_rtranif0
  | K_rtranif1
  ;

  /* A function_item_list only lists the input/output/inout
     declarations. The integer and reg declarations are handled in
     place, so are not listed. The list builder needs to account for
     the possibility that the various parts may be NULL. */
function_item_list
  : function_item
  | function_item_list function_item
  ;

function_item
  : K_input range_opt list_of_variables ';'
    {
      /* Create signal -- implicitly this is a wire which may not be explicitly declared */
      str_link* tmp  = $3;
      str_link* curr = tmp;
      while( curr != NULL ) {
        db_add_signal( curr->str, $2->left, $2->right, TRUE, FALSE );
        curr = curr->next;
      }
      str_link_delete_list( $3 );
      static_expr_dealloc( $2->left, FALSE );
      static_expr_dealloc( $2->right, FALSE );
      free_safe( $2 );
      curr_sig_width = NULL;
    }
  | block_item_decl
  ;

parameter_assign_list
  : parameter_assign
  | range parameter_assign
    {
      free_safe( $1 );
    }
  | parameter_assign_list ',' parameter_assign
  ;

parameter_assign
  : IDENTIFIER '=' expression
    {
      db_add_declared_param( $1, $3 );
    }
  | UNUSED_IDENTIFIER '=' expression
  ;

localparam_assign_list
  : localparam_assign
    {
      $$ = NULL;
    }
  | range localparam_assign
    {
      if( ignore_mode == 0 ) {
        free_safe( $1 );
      }
      $$ = NULL;
    }
  | localparam_assign_list ',' localparam_assign
    {
      $$ = NULL;
    }
  ;

localparam_assign
  : IDENTIFIER '=' { ignore_mode++; } expression { ignore_mode--; }
    {
      $$ = $1;
    }
  | UNUSED_IDENTIFIER '=' expression
    {
      $$ = NULL;
    }
  ;

port_name_list
  : port_name_list ',' port_name
  | port_name
  ;

port_name
  : '.' IDENTIFIER '(' { ignore_mode++; } expression { ignore_mode--; } ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' expression ')'
  | '.' IDENTIFIER '(' error ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' error ')'
  | '.' IDENTIFIER '(' ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' ')'
  ;

specify_item_list
  : specify_item
  | specify_item_list specify_item
  ;

specify_item
  : K_specparam specparam_list ';'
  | specify_simple_path_decl ';'
  | specify_edge_path_decl ';'
  | K_if '(' expression ')' specify_simple_path_decl ';'
  | K_if '(' expression ')' specify_edge_path_decl ';'
  | K_Shold '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Speriod '(' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Srecovery '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Ssetup '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Ssetuphold '(' spec_reference_event ',' spec_reference_event ',' expression ',' expression spec_notifier_opt ')' ';'
  | K_Swidth '(' spec_reference_event ',' expression ',' expression spec_notifier_opt ')' ';'
  | K_Swidth '(' spec_reference_event ',' expression ')' ';'
  ;

specparam_list
  : specparam
  | specparam_list ',' specparam
  ;

specparam
  : UNUSED_IDENTIFIER '=' expression
  | UNUSED_IDENTIFIER '=' expression ':' expression ':' expression
  | UNUSED_PATHPULSE_IDENTIFIER '=' expression
  | UNUSED_PATHPULSE_IDENTIFIER '=' '(' expression ',' expression ')'
  ;
  
specify_simple_path_decl
  : specify_simple_path '=' '(' specify_delay_value_list ')'
  | specify_simple_path '=' delay_value_simple
  | specify_simple_path '=' '(' error ')'
  ;

specify_simple_path
  : '(' specify_path_identifiers spec_polarity K_EG expression ')'
  | '(' specify_path_identifiers spec_polarity K_SG expression ')'
  | '(' error ')'
  ;

specify_path_identifiers
  : UNUSED_IDENTIFIER
  | UNUSED_IDENTIFIER '[' expr_primary ']'
  | specify_path_identifiers ',' UNUSED_IDENTIFIER
  | specify_path_identifiers ',' UNUSED_IDENTIFIER '[' expr_primary ']'
  ;

spec_polarity
  : '+'
  | '-'
  |
  ;

spec_reference_event
  : K_posedge expression
  | K_negedge expression
  | K_posedge expr_primary K_TAND expression
  | K_negedge expr_primary K_TAND expression
  | expr_primary K_TAND expression
    {
      assert( $1 == NULL );
      assert( $3 == NULL );
    }
  | expr_primary
    {
      assert( $1 == NULL );
    }
  ;

spec_notifier_opt
  :
  | spec_notifier
  ;

spec_notifier
  : ','
  | ','  identifier
  | spec_notifier ',' 
  | spec_notifier ',' identifier
  | UNUSED_IDENTIFIER
  ;

specify_edge_path_decl
  : specify_edge_path '=' '(' specify_delay_value_list ')'
  | specify_edge_path '=' delay_value_simple
  ;

specify_edge_path
  : '(' K_posedge specify_path_identifiers spec_polarity K_EG UNUSED_IDENTIFIER ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG UNUSED_IDENTIFIER ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG UNUSED_IDENTIFIER ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG UNUSED_IDENTIFIER ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
  ;

polarity_operator
  : K_PO_POS
  | K_PO_NEG
  | ':'
  ;

specify_delay_value_list
  : delay_value
    {
      assert( $1 == NULL );
    }
  | specify_delay_value_list ',' delay_value
  ;

ignore_more
  :
    {
      ignore_mode++;
    }
  ;
  
ignore_less
  :
    {
      ignore_mode--;
    }
  ;

start_lhs
  :
    {
      lhs_mode = TRUE;
    }
  ;

end_lhs
  :
    {
      lhs_mode = FALSE;
    }
  ;
