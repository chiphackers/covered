
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
#include "signal.h"
#include "expr.h"
#include "vector.h"
#include "module.h"
#include "db.h"
#include "link.h"
#include "parser_misc.h"
#include "statement.h"
#include "static.h"
#include "util.h"

extern char user_msg[USER_MSG_LENGTH];
extern int  delay_expr_type;

int ignore_mode = 0;
int param_mode  = 0;

exp_link* param_exp_head = NULL;
exp_link* param_exp_tail = NULL;

/* Uncomment these lines to turn debugging on */
/* #define YYDEBUG 1 */
#define YYERROR_VERBOSE 1
/* int yydebug = 1; */
%}

%union {
  char*           text;
  int	          integer;
  vector*         number;
  double          realtime;
  signal*         sig;
  expression*     expr;
  statement*      state;
  static_expr*    statexp;
  vector_width*   vecwidth; 
  str_link*       strlink;
  exp_link*       explink;
  case_statement* case_stmt;
};

%token <text>   HIDENTIFIER IDENTIFIER
%token <text>   PATHPULSE_IDENTIFIER
%token <number> NUMBER
%token <realtime> REALTIME
%token STRING PORTNAME SYSTEM_IDENTIFIER IGNORE
%token UNUSED_HIDENTIFIER UNUSED_IDENTIFIER
%token UNUSED_PATHPULSE_IDENTIFIER
%token UNUSED_NUMBER
%token UNUSED_REALTIME
%token UNUSED_STRING UNUSED_PORTNAME UNUSED_SYSTEM_IDENTIFIER
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_RS K_SG
%token K_PO_POS K_PO_NEG
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

%type <integer>   net_type
%type <vecwidth>  range_opt range
%type <statexp>   static_expr static_expr_primary
%type <text>      identifier port_reference port port_opt port_reference_list
%type <text>      list_of_ports list_of_ports_opt
%type <expr>      expr_primary expression_list expression
%type <expr>      event_control event_expression_list event_expression
%type <text>      udp_port_list
%type <text>      lpvalue lavalue
%type <expr>      delay_value delay_value_simple
%type <text>      range_or_type_opt
%type <text>      defparam_assign_list defparam_assign
%type <text>      gate_instance
%type <text>      localparam_assign_list localparam_assign
%type <strlink>   register_variable_list list_of_variables
%type <strlink>   net_decl_assigns gate_instance_list
%type <text>      register_variable net_decl_assign
%type <state>     statement statement_list statement_opt 
%type <state>     for_statement fork_statement while_statement named_begin_end_block if_statement_error
%type <case_stmt> case_items case_item
%type <expr>      delay1 delay3 delay3_opt

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
      db_end_module();
    }
  | module_start K_endmodule
    {
      db_end_module();
    }
  | K_module IGNORE I_endmodule
  | K_macromodule IGNORE I_endmodule
  ;

module_start
  : K_module IDENTIFIER { ignore_mode++; } list_of_ports_opt { ignore_mode--; } ';'
    {
      db_add_module( $2, @1.text );
    }
  | K_macromodule IDENTIFIER { ignore_mode++; } list_of_ports_opt { ignore_mode--; } ';'
    {
      db_add_module( $2, @1.text );
    }
  ;

list_of_ports_opt
  : '(' list_of_ports ')'
    {
      $$ = 0;
    }
  |
    {
      $$ = 0;
    }
  ;

list_of_ports
  : list_of_ports ',' port_opt
  | port_opt
  ;

port_opt
  : port
  |
    {
      $$ = 0;
    }
  ;

  /* Coverage tools should not find port information that interesting.  We will
     handle it for parsing purposes, but ignore its information. */

port
  : port_reference
  | PORTNAME '(' { ignore_mode++; } port_reference { ignore_mode--; } ')'
    {
      $$ = 0;
    }
  | '{' { ignore_mode++; } port_reference_list { ignore_mode--; } '}'
    {
      $$ = 0;
    }
  | PORTNAME '(' '{' { ignore_mode--; } port_reference_list { ignore_mode++; } '}' ')'
    {
      $$ = 0;
    }
  ;

port_reference
  : UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | UNUSED_IDENTIFIER '[' static_expr ':' static_expr ']'
    {
      $$ = NULL;
    }
  | UNUSED_IDENTIFIER '[' static_expr ']'
    {
      $$ = NULL;
    }
  | UNUSED_IDENTIFIER '[' error ']'
    { 
      $$ = NULL;
    }
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
      tmp = static_expr_gen_unary( $2, EXP_OP_UINV, @1.first_line );
      $$ = tmp;
    }
  | '&' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UAND, @1.first_line );
      $$ = tmp;
    }
  | '!' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNOT, @1.first_line );
      $$ = tmp;
    }
  | '|' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UOR, @1.first_line );
      $$ = tmp;
    }
  | '^' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UXOR, @1.first_line );
      $$ = tmp;
    }
  | K_NAND static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNAND, @1.first_line );
      $$ = tmp;
    }
  | K_NOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNOR, @1.first_line );
      $$ = tmp;
    }
  | K_NXOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      tmp = static_expr_gen_unary( $2, EXP_OP_UNXOR, @1.first_line );
      $$ = tmp;
    }
  | static_expr '^' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_XOR, @1.first_line );
      $$ = tmp;
    }
  | static_expr '*' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MULTIPLY, @1.first_line );
      $$ = tmp;
    }
  | static_expr '/' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_DIVIDE, @1.first_line );
      $$ = tmp;
    }
  | static_expr '%' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MOD, @1.first_line );
      $$ = tmp;
    }
  | static_expr '+' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_ADD, @1.first_line );
      $$ = tmp;
    }
  | static_expr '-' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_SUBTRACT, @1.first_line );
      $$ = tmp;
    }
  | static_expr '&' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_AND, @1.first_line );
      $$ = tmp;
    }
  | static_expr '|' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_OR, @1.first_line );
      $$ = tmp;
    }
  | static_expr K_NOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NOR, @1.first_line );
      $$ = tmp;
    }
  | static_expr K_NAND static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NAND, @1.first_line );
      $$ = tmp;
    }
  | static_expr K_NXOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NXOR, @1.first_line );
      $$ = tmp;
    }
  ;

static_expr_primary
  : NUMBER
    {
      static_expr* tmp;
      if( ignore_mode == 0 ) {
        tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
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
  | IDENTIFIER
    {
      static_expr* tmp;
      if( ignore_mode == 0 ) {
        tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
        tmp->num = -1;
        tmp->exp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, $1 );
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
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
      } else {
        $$ = NULL;
      }
    }
  | '-' expr_primary %prec UNARY_PREC
    {
      if( ignore_mode == 0 ) {
        $$ = $2;
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
      } else {
        $$ = NULL;
      }
    }
  | '~' expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UINV, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '&' expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UAND, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '!' expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UNOT, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '|' expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UOR, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '^' expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UXOR, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NAND expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UNAND, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NOR expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UNOR, @1.first_line, NULL );
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NXOR expr_primary %prec UNARY_PREC
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        if( $2 == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "%s:%d: Expression signal not declared", 
                    @1.text,
                    @1.first_line );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
        tmp = db_create_expression( $2, NULL, EXP_OP_UNXOR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_XOR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_MULTIPLY, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_DIVIDE, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_MOD, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_ADD, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_SUBTRACT, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_AND, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_OR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_NAND, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_NOR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_NXOR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_LT, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_GT, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_LSHIFT, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_RSHIFT, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_EQ, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_CEQ, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_LE, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_GE, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_NE, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_CNE, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_LOR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_LAND, @1.first_line, NULL );
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
        csel = db_create_expression( $5,   $3, EXP_OP_COND_SEL, @1.first_line, NULL );
        cond = db_create_expression( csel, $1, EXP_OP_COND,     @1.first_line, NULL );
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
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
      vector_dealloc( tmp->value );
      tmp->value = $1;
      /* Calculate TRUE/FALSE-ness of NUMBER now */
      //switch( expression_bit_value( tmp ) ) {
      //  case 0 :  tmp->suppl = tmp->suppl | (0x1 << SUPPL_LSB_FALSE) | (0x1 << SUPPL_LSB_EVAL_F);  break;
      //  case 1 :  tmp->suppl = tmp->suppl | (0x1 << SUPPL_LSB_TRUE)  | (0x1 << SUPPL_LSB_EVAL_T);  break;
      //  default:  break;
      //}
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
      $$ = NULL;
    }
  | UNUSED_STRING
    {
      $$ = NULL;
    }
  | identifier
    {
      expression* tmp;
      if( ignore_mode == 0 ) {
        tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, $1 );
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
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        tmp = db_create_expression( NULL, $3, EXP_OP_SBIT_SEL, @1.first_line, $1 );
        $$  = tmp;
        free_safe( $1 );
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | identifier '[' expression ':' expression ']'
    {		  
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) && ($5 != NULL) ) {
        tmp = db_create_expression( $5, $3, EXP_OP_MBIT_SEL, @1.first_line, $1 );
        $$  = tmp;
        free_safe( $1 );
      } else {
        expression_dealloc( $3, FALSE );
        expression_dealloc( $5, FALSE );
        $$ = NULL;
      }
    }
  | identifier '[' expression K_PO_POS static_expr ']'
    {
      snprintf( user_msg, USER_MSG_LENGTH, "K_PO_POS expressions not supported at this time, line: %d", @1.first_line );
      print_output( user_msg, WARNING );
      expression_dealloc( $3, FALSE );
      static_expr_dealloc( $5, TRUE );
      free_safe( $1 );
    }
  | identifier '[' expression K_PO_NEG static_expr ']'
    {
      snprintf( user_msg, USER_MSG_LENGTH, "K_PO_POS expressions not supported at this time, line: %d", @1.first_line );
      print_output( user_msg, WARNING );
      expression_dealloc( $3, FALSE );
      static_expr_dealloc( $5, TRUE );
      free_safe( $1 );
    }
  | identifier '(' expression_list ')'
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $3, FALSE );
        free_safe( $1 );
      }
      $$ = NULL;
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
        tmp = db_create_expression( $2, NULL, EXP_OP_CONCAT, @1.first_line, NULL );
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
        tmp = db_create_expression( $4, $2, EXP_OP_EXPAND, @1.first_line, NULL );
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
            tmp = db_create_expression( $3, $1, EXP_OP_LIST, @1.first_line, NULL );
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
  | HIDENTIFIER
    {
      $$ = $1;
    }
  | UNUSED_HIDENTIFIER
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
  : K_table
      udp_entry_list
    K_endtable
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
  : net_type range_opt list_of_variables ';'
    {
      str_link*     tmp  = $3;
      str_link*     curr = tmp;
      if( ($1 == 1) && ($2 != NULL) ) {
        /* Creating signal(s) */
        while( curr != NULL ) {
          db_add_signal( curr->str, $2->left, $2->right );
          curr = curr->next;
        }
      }
      str_link_delete_list( $3 );
      if( $2 != NULL ) {
        static_expr_dealloc( $2->left,  FALSE );
        static_expr_dealloc( $2->right, FALSE );
        free_safe( $2 );
      }
    }
  | net_type range_opt net_decl_assigns ';'
    {
      str_link* tmp  = $3;
      str_link* curr = tmp;
      if( ($1 == 1) && ($2 != NULL) ) {
        /* Create signal(s) */
        while( curr != NULL ) {
          db_add_signal( curr->str, $2->left, $2->right );
          curr = curr->next;
        }
        /* What to do about assignments? */
      }
      str_link_delete_list( $3 );
      if( $2 != NULL ) {
        static_expr_dealloc( $2->left, FALSE );
        static_expr_dealloc( $2->right, FALSE );
        free_safe( $2 );
      }
    }
  | net_type drive_strength net_decl_assigns ';'
    {
      str_link*   tmp  = $3;
      str_link*   curr = tmp;
      static_expr left;
      static_expr right;
      if( $1 == 1 ) {
        /* Create signal(s) */
        left.num  = 1;
        right.num = 0;
        while( curr != NULL ) {
          db_add_signal( curr->str, &left, &right );
          curr = curr->next;
        }
      }
      str_link_delete_list( $3 );
    }
  | K_trireg charge_strength_opt range_opt delay3_opt list_of_variables ';'
    {
      str_link* tmp  = $5;
      str_link* curr = tmp;
      while( curr != NULL ) {
        db_add_signal( curr->str, $3->left, $3->right );
        curr = curr->next;
      }
      str_link_delete_list( $5 );
      static_expr_dealloc( $3->left, FALSE );
      static_expr_dealloc( $3->right, FALSE );
      free_safe( $3 );
    }
  | port_type range_opt list_of_variables ';'
    {
      /* Create signal -- implicitly this is a wire which may not be explicitly declared */
      str_link* tmp  = $3;
      str_link* curr = tmp;
      while( curr != NULL ) {
        db_add_signal( curr->str, $2->left, $2->right );
        curr = curr->next;
      }
      str_link_delete_list( $3 );
      static_expr_dealloc( $2->left, FALSE );
      static_expr_dealloc( $2->right, FALSE );
      free_safe( $2 );
    }
  | port_type range_opt error ';'
    {
      if( $2 != NULL ) {
        free_safe( $2 );
      }
      VLerror( "Invalid variable list in port declaration" );
    }
  | block_item_decl
  | K_defparam defparam_assign_list ';'
    {
      snprintf( user_msg, USER_MSG_LENGTH, "Defparam found but not used, file: %s, line: %d.  Please use -P option to specify",
                @1.text, @1.first_line );
      print_output( user_msg, FATAL );
    }
  | K_event list_of_variables ';'
    {
      str_link_delete_list( $2 );
    }
  /* Handles instantiations of modules and user-defined primitives. */
  | IDENTIFIER parameter_value_opt gate_instance_list ';'
    {
      str_link* tmp  = $3;
      str_link* curr = tmp;
      exp_link* ecurr;
      while( curr != NULL ) {
        db_add_instance( curr->str, $1 );
        ecurr = param_exp_head;
        while( ecurr != NULL ){
          db_add_override_param( curr->str, ecurr->exp );
          ecurr = ecurr->next;
        }
        curr = curr->next;
      }
      str_link_delete_list( tmp );
      exp_link_delete_list( param_exp_head, FALSE );
      param_exp_head = NULL;
      param_exp_tail = NULL;
    }
  | K_assign drive_strength_opt { ignore_mode++; } delay3_opt { ignore_mode--; } assign_list ';'
	| K_always statement
    {
      statement* stmt = $2;
      if( stmt != NULL ) {
        db_statement_connect( stmt, stmt );
        db_statement_set_stop( stmt, stmt, TRUE );
        stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
        db_add_statement( stmt, stmt );
      }
    }
  | K_initial { ignore_mode++; } statement { ignore_mode--; }
    {
      /*
      statement* stmt = $2;
      db_statement_set_stop( stmt, NULL, FALSE );
      stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
      db_add_statement( stmt, stmt );
      */
    }
  | K_task { ignore_mode++; } UNUSED_IDENTIFIER ';'
      task_item_list_opt statement_opt
    K_endtask { ignore_mode--; }
  | K_function { ignore_mode++; } range_or_type_opt UNUSED_IDENTIFIER ';'
      function_item_list statement
    K_endfunction { ignore_mode--; }
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
  | K_begin ':' { ignore_mode++; } named_begin_end_block { ignore_mode--; } K_end
    {
      $$ = NULL;
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
  | K_TRIGGER { ignore_mode++; } UNUSED_IDENTIFIER ';' { ignore_mode--; }
    {
      $$ = NULL;
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
        c_expr->suppl = c_expr->suppl | (0x1 << SUPPL_LSB_ROOT);
        while( c_stmt != NULL ) {
          if( c_stmt->expr != NULL ) {
            expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASE, c_stmt->line, NULL );
          } else {
            expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, c_stmt->line, NULL );
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
          statement_dealloc_recursive( c_stmt->stmt );
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
        c_expr->suppl = c_expr->suppl | (0x1 << SUPPL_LSB_ROOT);
        while( c_stmt != NULL ) {
          if( c_stmt->expr != NULL ) {
            expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEX, c_stmt->line, NULL );
          } else {
            expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, c_stmt->line, NULL );
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
          statement_dealloc_recursive( c_stmt->stmt );
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
        c_expr->suppl = c_expr->suppl | (0x1 << SUPPL_LSB_ROOT);
        while( c_stmt != NULL ) {
          if( c_stmt->expr != NULL ) {
            expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEZ, c_stmt->line, NULL );
          } else {
            expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, c_stmt->line, NULL );
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
          statement_dealloc_recursive( c_stmt->stmt );
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
      statement* stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        stmt = db_create_statement( $3 );
        db_add_expression( $3 );
        db_connect_statement_true( stmt, $5 );
        db_statement_set_stop( $5, NULL, FALSE );
        $$ = stmt;
      } else {
        statement_dealloc_recursive( $5 );
        $$ = NULL;
      }
    }
  | K_if '(' expression ')' statement_opt K_else statement_opt
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        stmt = db_create_statement( $3 );
        db_add_expression( $3 );
        db_connect_statement_true( stmt, $5 );
        db_connect_statement_false( stmt, $7 );
        db_statement_set_stop( $5, NULL, FALSE );
        $$ = stmt;
      } else {
        statement_dealloc_recursive( $5 );
        statement_dealloc_recursive( $7 );
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
        statement_dealloc_recursive( $2 );
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
        statement_dealloc_recursive( $2 );
        $$ = NULL;
      }
    }
  | lpvalue '=' expression ';'
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        stmt = db_create_statement( $3 );
        db_add_expression( $3 );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | lpvalue K_LE expression ';'
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        stmt = db_create_statement( $3 );
        db_add_expression( $3 );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | lpvalue '=' delay1 expression ';'
    {
      statement* stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        stmt = db_create_statement( $4 );
        db_add_expression( $4 );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | lpvalue K_LE delay1 expression ';'
    {
      statement* stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        stmt = db_create_statement( $4 );
        db_add_expression( $4 );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | lpvalue '=' event_control expression ';'
    {
      statement* stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        stmt = db_create_statement( $4 );
        db_add_expression( $4 );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | lpvalue K_LE event_control expression ';'
    {
      statement* stmt;
      expression_dealloc( $3, FALSE );
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        stmt = db_create_statement( $4 );
        db_add_expression( $4 );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | lpvalue '=' K_repeat { ignore_mode++; } '(' expression ')' event_control expression ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | lpvalue K_LE K_repeat { ignore_mode++; } '(' expression ')' event_control expression ';' { ignore_mode--; }
    {
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
        statement_dealloc_recursive( $5 );
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
  | identifier '(' { ignore_mode++; } expression_list { ignore_mode--; } ')' ';'
    {
      if( ignore_mode == 0 ) {
        free_safe( $1 );
      }
      $$ = NULL;
    }
  | identifier ';'
    {
      if( ignore_mode == 0 ) {
        free_safe( $1 );
      }
      $$ = NULL;
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
  : UNUSED_IDENTIFIER block_item_decls_opt statement_list
    {
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
          $$ = $2;
        } else {
          if( $2 != NULL ) {
            db_statement_connect( $1, $2 );
          }
          $$ = $1;
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
  | identifier ignore_more '[' static_expr ']' ignore_less
  | identifier ignore_more '[' static_expr ':' static_expr ']' ignore_less
  | '{' ignore_more expression_list ignore_less '}'
    {
      $$ = 0;
    }
  ;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
  : identifier
  | identifier ignore_more '[' static_expr ']' ignore_less
  | identifier ignore_more '[' static_expr ':' static_expr ']' ignore_less
  | '{' ignore_more expression_list ignore_less '}'
    {
      $$ = 0;
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
          db_add_signal( curr->str, $2->left, $2->right );
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
          db_add_signal( curr->str, &left, &right );
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
          db_add_signal( curr->str, $3->left, $3->right );
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
          db_add_signal( curr->str, &left, &right );
          curr = curr->next;
        }
        str_link_delete_list( tmp );
      }
    }
  | K_integer register_variable_list ';'
    {
      if( ignore_mode == 0 ) {
        str_link_delete_list( $2 );
      }
    }
  | K_time register_variable_list ';'
    {
      if( ignore_mode == 0 ) {
        str_link_delete_list( $2 );
      }
    }
  | K_real list_of_variables ';'
    {
      if( ignore_mode == 0 ) {
        str_link_delete_list( $2 );
      }
    }
  | K_realtime list_of_variables ';'
    {
      if( ignore_mode == 0 ) {
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
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
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
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
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
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
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
        vec = vector_create( 32, 0, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL ); 
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $2, tmp, EXP_OP_DELAY, @1.first_line, NULL );
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
        vec = vector_create( 32, 0, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
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
        vec = vector_create( 32, 0, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $2, tmp, EXP_OP_DELAY, @1.first_line, NULL );
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
        vec = vector_create( 32, 0, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
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
        vec = vector_create( 32, 0, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
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
        vec = vector_create( 32, 0, TRUE );
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
        vector_from_int( vec, 0xffffffff );
        assert( tmp->value->value == NULL );
        free_safe( tmp->value );
        tmp->value = vec;
        exp = db_create_expression( $5, tmp, EXP_OP_DELAY, @1.first_line, NULL );
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
      if( ignore_mode == 0 ) {
        if( se->exp == NULL ) {
          tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
          vector_init( tmp->value, (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_SIZE( 32 ) ), 32, 0 );  
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
          print_output( user_msg, WARNING );
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
        if( se->exp == NULL ) {
          tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
          vector_init( tmp->value, (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_SIZE( 32 ) ), 32, 0 );  
          vector_from_int( tmp->value, se->num );
        } else {
          tmp = se->exp;
          free_safe( se );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

delay_value_simple
  : NUMBER
    {
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, @1.first_line, NULL );
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
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, $1 );
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
      statement* stmt;
      if( $3 != NULL ) {
        stmt = db_create_statement( $3 );
        stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
        stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_STOP);
        stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_CONTINUOUS);
        /* Statement will be looped back to itself */
        db_connect_statement_true( stmt, stmt );
        db_connect_statement_false( stmt, stmt );
        db_add_expression( $3 );
        db_add_statement( stmt, stmt );
      }
    }
  ;

range_opt
  : range
    {
      $$ = $1;
    }
  |
    {
      vector_width* tmp;
      static_expr*  left;
      static_expr*  right;
      if( ignore_mode == 0 ) {
        left = (static_expr*)malloc_safe( sizeof( static_expr ) );
        left->exp = NULL;
        left->num = 0;
        right = (static_expr*)malloc_safe( sizeof( static_expr ) );
        right->exp = NULL;
        right->num = 0;
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ) );
        tmp->left  = left;
        tmp->right = right;
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
        tmp = (vector_width*)malloc_safe( sizeof( vector_width ) );
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
        if( tmp->left != NULL ) {
          free_safe( tmp->left );
        }
        if( tmp->right != NULL ) {
          free_safe( tmp->right );
        }
        free_safe( tmp );
      }
      $$ = NULL;
    }
  | K_integer  { $$ = NULL; }
  | K_real     { $$ = NULL; }
  | K_realtime { $$ = NULL; }
  | K_time     { $$ = NULL; }
  |            { $$ = NULL; }
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
      if( $1 != NULL ) {
        free_safe( $1 );
      }
      $$ = NULL;
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
      if( ignore_mode == 0 ) {
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
        tmp = (str_link*)malloc( sizeof( str_link ) );
        tmp->str  = $3;
        tmp->next = $1;
        $$ = tmp;
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
  | K_input { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
    {
      if( ignore_mode == 0 ) {
        str_link_delete_list( $4 );
        if( $3 != NULL ) {
          free_safe( $3 );
        }
      }
    }
  | K_output { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
    {
      if( ignore_mode == 0 ) {
        str_link_delete_list( $4 );
        if( $3 != NULL ) {
          free_safe( $3 );
        }
      }
    }
  | K_inout { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
    {
      if( ignore_mode == 0 ) {
        str_link_delete_list( $4 );
        if( $3 != NULL ) {
          free_safe( $3 );
        }
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
  | net_decl_assign
    {
      str_link* tmp;
      if( ignore_mode == 0 ) {
        tmp = (str_link*)malloc( sizeof( str_link) );
        tmp->str  = $1;
        tmp->next = NULL;
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

net_decl_assign
  : IDENTIFIER '=' expression
    {
      statement* stmt;
      if( ignore_mode == 0 ) {
        if( $3 != NULL ) {
          stmt = db_create_statement( $3 );
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_STOP);
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_CONTINUOUS);
          /* Statement will be looped back to itself */
          db_connect_statement_true( stmt, stmt );
          db_connect_statement_false( stmt, stmt );
          db_add_expression( $3 );
          db_add_statement( stmt, stmt );
        }
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER '=' expression
    {
      $$ = NULL;
    } 
  | delay1 IDENTIFIER '=' expression
    {
      statement* stmt;
      if( ignore_mode == 0 ) {
        if( $4 != NULL ) {
          stmt = db_create_statement( $4 );
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_STOP);
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_CONTINUOUS);
          /* Statement will be looped back to itself */
          db_connect_statement_true( stmt, stmt );
          db_connect_statement_false( stmt, stmt );
          db_add_expression( $4 );
          db_add_statement( stmt, stmt );
        }
        $$ = $2;
      } else {
        $$ = NULL;
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
      signal*     sig;
      expression* tmp;
      if( ignore_mode == 0 ) {
        sig = db_find_signal( $2 );
        if( sig != NULL ) {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_EOR, @1.first_line, NULL );
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
        tmp = db_create_expression( $3, $1, EXP_OP_EOR, @1.first_line, NULL );
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
        tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, @1.first_line, NULL );
        expression_create_value( tmp1, 1, 0, FALSE );
        tmp2 = db_create_expression( $2, tmp1, EXP_OP_PEDGE, @1.first_line, NULL );
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
        tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, @1.first_line, NULL );
        expression_create_value( tmp1, 1, 0, FALSE );
        tmp2 = db_create_expression( $2, tmp1, EXP_OP_NEDGE, @1.first_line, NULL );
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
        tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, @1.first_line, NULL );
        tmp2 = db_create_expression( expr, tmp1, EXP_OP_AEDGE, @1.first_line, NULL );
        expression_create_value( tmp1, expr->value->width, 0, FALSE );
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
  | K_output
  | K_inout
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
  : PORTNAME '(' expression ')'
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $3, FALSE );
      }
    }
  | UNUSED_PORTNAME '(' expression ')'
  | PORTNAME '(' ')'
  | UNUSED_PORTNAME '(' ')'
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

  /* A function_item_list only lists the input/output/inout
     declarations. The integer and reg declarations are handled in
     place, so are not listed. The list builder needs to account for
     the possibility that the various parts may be NULL. */
function_item_list
  : function_item
  | function_item_list function_item
  ;

function_item
  : K_input { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
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
  : IDENTIFIER '=' { param_mode++; } expression { param_mode--; }
    {
      db_add_declared_param( $1, $4 );
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
  : PORTNAME '(' { ignore_mode++; } expression { ignore_mode--; } ')'
  | UNUSED_PORTNAME '(' expression ')'
  | PORTNAME '(' error ')'
  | UNUSED_PORTNAME '(' error ')'
  | PORTNAME '(' ')'
  | UNUSED_PORTNAME '(' ')'
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

