
%{
/*!
 \file     parser.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/14/2001
*/

#include <stdio.h>

#include "defines.h"
#include "signal.h"
#include "expr.h"
#include "module.h"
#include "db.h"
#include "link.h"
#include "parser_misc.h"

char err_msg[1000];

/* Uncomment these lines to turn debuggin on */
//#define YYDEBUG 1
//#define YYERROR_VERBOSE 1
//int yydebug = 1;
%}

%union {
  char*         text;
  int	        integer;
  vector*       number;
  double        realtime;
  signal*       sig;
  expression*   expr;
  signal_width* sigwidth; 
  str_link*     strlink;
};

%token <text>   HIDENTIFIER IDENTIFIER
%token <text>   PATHPULSE_IDENTIFIER
%token <number> NUMBER
%token <realtime> REALTIME
%token STRING PORTNAME SYSTEM_IDENTIFIER IGNORE
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

%type <integer>  net_type
%type <sigwidth> range_opt range
%type <integer>  static_expr static_expr_primary
%type <text>     identifier port_reference port port_opt port_reference_list
%type <text>     list_of_ports list_of_ports_opt
%type <expr>     expr_primary expression_list expression
%type <expr>     event_control event_expression_list event_expression
%type <text>     udp_port_list
%type <text>     lpvalue lavalue
%type <integer>  delay_value delay_value_simple
%type <text>     range_or_type_opt
%type <text>     defparam_assign_list defparam_assign
%type <text>     gate_instance
%type <text>     parameter_assign_list parameter_assign
%type <text>     localparam_assign_list localparam_assign
%type <strlink>  register_variable_list list_of_variables
%type <strlink>  net_decl_assigns gate_instance_list
%type <text>     register_variable net_decl_assign
%type <expr>     statement statement_list statement_opt

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
	| KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')'
	;

module
	: module_start IDENTIFIER list_of_ports_opt ';'
		{ 
		  db_add_module( $2, @1.text );
		}
	  module_item_list
	  K_endmodule
		{
		  db_end_module();
		}
	| module_start IDENTIFIER list_of_ports_opt ';'
		{
		  db_add_module( $2, @1.text );
		}
	  K_endmodule
		{
		  db_end_module();
		}
	| module_start IGNORE I_endmodule
	;

module_start
	: K_module
	| K_macromodule
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
	: port_opt
	| list_of_ports ',' port_opt
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
	| PORTNAME '(' port_reference ')'
		{
		  $$ = 0;
		}
	| '{' port_reference_list '}'
		{
		  $$ = 0;
		}
	| PORTNAME '(' '{' port_reference_list '}' ')'
		{
		  $$ = 0;
		}
	;

port_reference
	: IDENTIFIER
	| IDENTIFIER '[' static_expr ':' static_expr ']'
	| IDENTIFIER '[' static_expr ']'
	| IDENTIFIER '[' error ']'
		{
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
		  $$ = +$2;
		}
	| '-' static_expr_primary %prec UNARY_PREC
		{
		  $$ = -$2;
		}
        | '~' static_expr_primary %prec UNARY_PREC
		{
		  $$ = ~$2;
		}
        | '&' static_expr_primary %prec UNARY_PREC
		{
		  int tmp = $2;
		  int uop = tmp & 0x1;
		  int i;
		  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
		    uop = uop & ((tmp >> i) & 0x1);
		  }
		  $$ = uop;
		}
        | '!' static_expr_primary %prec UNARY_PREC
		{
		  $$ = ($2 == 0) ? 1 : 0;
		}
        | '|' static_expr_primary %prec UNARY_PREC
		{
		  int tmp = $2;
		  int uop = tmp & 0x1;
		  int i;
		  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
		    uop = uop | ((tmp >> i) & 0x1);
		  }
		  $$ = uop;
		}
        | '^' static_expr_primary %prec UNARY_PREC
		{
		  int tmp = $2;
		  int uop = uop & 0x1;
		  int i;
		  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
		    uop = uop ^ ((tmp >> i) & 0x1);
		  }
		  $$ = uop;
		}
        | K_NAND static_expr_primary %prec UNARY_PREC
		{
                  int tmp = $2;
                  int uop = tmp & 0x1;
                  int i;
                  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
                    uop = uop & ((tmp >> i) & 0x1);
                  }
                  $$ = (uop == 0) ? 1 : 0;
		}
        | K_NOR static_expr_primary %prec UNARY_PREC
		{
                  int tmp = $2;
                  int uop = tmp & 0x1;
                  int i;
                  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
                    uop = uop | ((tmp >> i) & 0x1);
                  }
                  $$ = (uop == 0) ? 1 : 0;
		}
        | K_NXOR static_expr_primary %prec UNARY_PREC
		{
                  int tmp = $2;
                  int uop = uop & 0x1;
                  int i;
                  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
                    uop = uop ^ ((tmp >> i) & 0x1);
                  }
                  $$ = (uop == 0) ? 1 : 0;
		}
        | '!' error %prec UNARY_PREC
                {
		  snprintf( err_msg, 1000, "%s:%d: error: Operand of unary ! is not a primary expression.\n",
				@1.text,
				@1.first_line
			  );
                  yyerror( err_msg );
                }
        | '^' error %prec UNARY_PREC
                {
		  snprintf( err_msg, 1000, "%s:%d: error: Operand of reduction ^ is not a primary expression.\n",
				@1.text,
				@1.first_line
			  );
		  yyerror( err_msg );
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
	;

static_expr_primary
	: NUMBER
		{
		  $$ = vector_to_int( $1 );
		  vector_dealloc( $1 );
		}
	| '(' static_expr ')'
		{
		  $$ = $2;
		}
	| error
		{
		  // yyerror( @1, "error: Static value expected for static expression." );
		}
	;

expression
	: expr_primary
		{
		  $$ = $1;
		}
	| '+' expr_primary %prec UNARY_PREC
		{
		  $$ = $2;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		}
	| '-' expr_primary %prec UNARY_PREC
		{
		  $$ = $2;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		}
	| '~' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UINV, @1.first_line, NULL );
		  $$ = tmp;
		}
	| '&' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UAND, @1.first_line, NULL );
		  $$ = tmp;
		}
	| '!' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UNOT, @1.first_line, NULL );
		  $$ = tmp;
		}
	| '|' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| '^' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UXOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| K_NAND expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UNAND, @1.first_line, NULL );
		  $$ = tmp;
		}
	| K_NOR expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UNOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| K_NXOR expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
		  if( $2 == NULL ) {
		    snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		    print_output( err_msg, FATAL );
		    exit( 1 );
		  }
		  tmp = db_create_expression( $2, NULL, EXP_OP_UNXOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| '!' error %prec UNARY_PREC
		{
		  // yyerror( @1, "error: Operand of unary ! is not a primary expression." );
		}
	| '^' error %prec UNARY_PREC
		{
		  // yyerror( @1, "error: Operand of reduction ^ is not a primary expression." );
		}
	| expression '^' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_XOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '*' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_MULTIPLY, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '/' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_DIVIDE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '%' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_MOD, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '+' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_ADD, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '-' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_SUBTRACT, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '&' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_AND, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '|' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_OR, @1.first_line, NULL );
		  $$ = tmp;
		}
        | expression K_NAND expression
                {
                  expression* tmp = db_create_expression( $3, $1, EXP_OP_NAND, @1.first_line, NULL );
                  $$ = tmp;
                }
	| expression K_NOR expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_NOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_NXOR expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_NXOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '<' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_LT, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '>' expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_GT, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_LS expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_LSHIFT, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_RS expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_RSHIFT, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_EQ expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_EQ, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_CEQ expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_CEQ, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_LE expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_LE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_GE expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_GE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_NE expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_NE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_CNE expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_CNE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_LOR expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_LOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression K_LAND expression
		{
		  expression* tmp = db_create_expression( $3, $1, EXP_OP_LAND, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression '?' expression ':' expression
		{
		  expression* tmp1 = db_create_expression( $3, $1, EXP_OP_COND_T, @1.first_line, NULL );
		  expression* tmp2 = db_create_expression( $5, $1, EXP_OP_COND_F, @1.first_line, NULL );
		  expression* tmp  = db_create_expression( tmp2, tmp1, EXP_OP_OR, @1.first_line, NULL );
		  $$ = tmp;
		}
	;

expr_primary
	: NUMBER
		{
		  expression* tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                  free_safe( tmp->value );
		  tmp->value = $1;
		  $$ = tmp;
		}
	| STRING
		{
		  $$ = 0;
		}
	| identifier
		{
                  expression* tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, $1 );
                  $$ = tmp;
		  free_safe( $1 );
		}
	| SYSTEM_IDENTIFIER
		{
		  $$ = 0;
		}
	| identifier '[' expression ']'
		{
		  expression* tmp = db_create_expression( $3, NULL, EXP_OP_SBIT_SEL, @1.first_line, $1 );
		  $$ = tmp;
		  free_safe( $1 );
		}
	| identifier '[' expression ':' expression ']'
		{		  
                  expression* tmp = db_create_expression( $5, $3, EXP_OP_MBIT_SEL, @1.first_line, $1 );
                  $$ = tmp;
                  free_safe( $1 );
		}
	| identifier '(' expression_list ')'
		{
		  $$ = 0;
		}
	| SYSTEM_IDENTIFIER '(' expression_list ')'
		{
		  $$ = 0;
		}
	| '(' expression ')'
		{
		  $$ = $2;
		}
	| '{' expression_list '}'
		{
		  $$ = $2;
		}
	| '{' expression '{' expression_list '}' '}'
		{
		  expression* tmp;
		  tmp = db_create_expression( $4, $2, EXP_OP_EXPAND, @1.first_line, NULL );
		  $$ = tmp;
		}
	;

  /* Expression lists are used in function/task calls and concatenations */
expression_list
	: expression_list ',' expression
		{
		  expression* tmp;
		  if( $3 != NULL ) {
		    tmp = db_create_expression( $3, $1, EXP_OP_CONCAT, @1.first_line, NULL );
		    $$ = tmp;
		  } else {
		    $$ = $1;
		  }
		}
	| expression
		{
		  $$ = $1;
		}
	|
		{
		  $$ = NULL;
		}
	| expression_list ','
		{
		  $$ = $1;
		}
	;

identifier
	: IDENTIFIER
		{
		  $$ = $1;
		}
	| HIDENTIFIER
		{
		  $$ = $1;
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
	| list_of_variables ',' IDENTIFIER
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $3;
		  tmp->next = $1;
		  $$ = tmp;
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
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
		  if( $1 == 1 ) {
		    /* Creating signal(s) */
		    while( curr != NULL ) {
		      db_add_signal( curr->str, $2->width, $2->lsb, 0 );
		      curr = curr->next;
		    }
		  }
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| net_type range_opt net_decl_assigns ';'
		{
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
		  if( $1 == 1 ) {
		    /* Create signal(s) */
		    while( curr != NULL ) {
                      db_add_signal( curr->str, $2->width, $2->lsb, 0 );
                      curr = curr->next;
                    }
		    /* What to do about assignments? */
		  }
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| net_type drive_strength net_decl_assigns ';'
		{
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
		  if( $1 == 1 ) {
		    /* Create signal(s) */
	            while( curr != NULL ) {
                      db_add_signal( curr->str, 1, 0, 0 );
                      curr = curr->next;
                    }
                    /* What to do about assignments? */
		  }
		  str_link_delete_list( $3 );
		}
	| K_trireg charge_strength_opt range_opt delay3_opt list_of_variables ';'
		{
		  /* Tri-state signals are not currently supported by covered */
		  str_link_delete_list( $5 );
		  free_safe( $3 );
		}
	| port_type range_opt list_of_variables ';'
		{
		  /* Create signal -- implicitly this is a wire which may not be explicitly declared */
		  str_link* tmp  = $3;
                  str_link* curr = tmp;
		  while( curr != NULL ) {
                    db_add_signal( curr->str, $2->width, $2->lsb, 0 );
                    curr = curr->next;
                  }
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| port_type range_opt error ';'
		{
		  free_safe( $2 );
		  // yyerror( @3, "error: Invalid variable list in port declaration.");
		  // if( $2 ) delete $2;
		  // yyerrok;
		}
	| block_item_decl
	| K_defparam defparam_assign_list ';'
	| K_event list_of_variables ';'
		{
		  str_link_delete_list( $2 );
		}

  /* Handles instantiations of modules and user-defined primitives. */
	| IDENTIFIER parameter_value_opt gate_instance_list ';'
		{
		  str_link* tmp = $3;
		  str_link* curr = tmp;
		  while( curr != NULL ) {
		    db_add_instance( curr->str, $1 );
		    curr = curr->next;
		  }
		  str_link_delete_list( $3 );
		}

	| K_assign drive_strength_opt delay3_opt assign_list ';'
		{
		  /* Create root expression */
		}
	| K_always statement
		{
                  /* Line will be uncommented when statement support is complete */
                  // db_add_expression( $2 );
		}
	| K_initial statement
		{
                  db_add_expression( $2 );
		}
	| K_task IDENTIFIER ';'
	    task_item_list_opt statement_opt
	  K_endtask
	| K_function range_or_type_opt IDENTIFIER ';'
	    function_item_list statement
	  K_endfunction
	| K_specify K_endspecify
	| K_specify error K_endspecify
	| error ';'
		{
		  // yyerror( @1, "error: invalid module item.  Did you forget an initial or always?" );
		  // yyerrok;
		}
	| K_assign error '=' expression ';'
		{
		  // yyerror( @1, "error: syntax error in left side of continuous assignment." );
		  // yyerrok;
		  expression_dealloc( $4, FALSE );
		}
	| K_assign error ';'
		{
		  // yyerror( @1, "error: syntax error in continuous assignment." );
		  // yyerrok;
		}
	| K_function error K_endfunction
		{
		  // yyerrok( @1, "error: I give up on this function definition" );
		  // yyerrok;
		}
	| KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')' ';'
	| KK_attribute '(' error ')' ';'
		{
		  // yyerror( @1, "error: Misformed $attribute parameter list.");
		}
	;

statement
	: K_assign lavalue '=' expression ';'
		{
		  db_add_expression( $4 );
                  $$ = NULL;
		}
	| K_deassign lavalue ';'
		{
                  $$ = NULL;
		}
	| K_force lavalue '=' expression ';'
		{
		  /* Don't handle forces at this time */
		  expression_dealloc( $4, FALSE );
                  $$ = NULL;
		}
	| K_release lavalue ';'
		{
                  $$ = NULL;
		}
	| K_begin statement_list K_end
		{
                  $$ = $2;
		}
	| K_begin ':' IDENTIFIER
	    block_item_decls_opt
	    statement_list
	  K_end
		{
                  $$ = $5;
		}
	| K_begin K_end
		{
                  $$ = NULL;
		}
	| K_begin ':' IDENTIFIER K_end
		{
                  $$ = NULL;
		}
	| K_begin error K_end
		{
		  yyerrok;
                  $$ = NULL;
		}
	| K_fork ':' IDENTIFIER
	    block_item_decls_opt
	    statement_list
	  K_join
		{
                  $$ = $5;
		}
	| K_fork K_join
		{
                  $$ = NULL;
		}
	| K_fork ':' IDENTIFIER K_join
		{
                  $$ = NULL;
		}
	| K_disable identifier ';'
		{
                  $$ = NULL;
		}
	| K_TRIGGER IDENTIFIER ';'
		{
                  $$ = NULL;
		}
	| K_forever statement
		{
                  expression* exp = db_create_statement( @1.first_line, $2 );
                  $$ = exp;
		}
	| K_fork statement_list K_join
		{
                  expression* exp = db_create_statement( @1.first_line, $2 );
                  $$ = exp;
		}
	| K_repeat '(' expression ')' statement
		{
                  expression* exp = db_create_statement( @1.first_line, $3 );
                  db_add_expression( $3 );
                  $$ = exp;
		}
	| K_case '(' expression ')' case_items K_endcase
		{
                  /* Not sure how to handle expressions quite yet */
                  $$ = NULL;
		}
	| K_casex '(' expression ')' case_items K_endcase
		{
                  /* Not sure how to handle expressions quite yet */
                  $$ = NULL;
		}
	| K_casez '(' expression ')' case_items K_endcase
		{
                  /* Not sure how to handle expressions quite yet */
                  $$ = NULL;
		}
	| K_case '(' expression ')' error K_endcase
		{
		  yyerrok;
                  $$ = NULL;
		}
	| K_casex '(' expression ')' error K_endcase
		{
		  yyerrok;
                  $$ = NULL;
		}
	| K_casez '(' expression ')' error K_endcase
		{
		  yyerrok;
                  $$ = NULL;
		}
	| K_if '(' expression ')' statement_opt %prec less_than_K_else
		{
                  expression* exp = db_create_statement( @1.first_line, $3 );
		  db_add_expression( $3 );
                  db_connect_statement_true( exp, $5 );
                  $$ = exp;
		}
	| K_if '(' expression ')' statement_opt K_else statement_opt
		{
                  expression* exp = db_create_statement( @1.first_line, $3 );
		  db_add_expression( $3 );
                  db_connect_statement_true( exp, $5 );
                  db_connect_statement_false( exp, $7 );
                  $$ = exp;
		}
	| K_if '(' error ')' statement_opt %prec less_than_K_else
		{
		  // yyerror( @1, "error: Malformed conditional expression." );
                  $$ = NULL;
		}
	| K_if '(' error ')' statement_opt K_else statement_opt 
		{
		  // yyerror( @1, "error: Malformed conditional expression." );
                  $$ = NULL;
		}
	| K_for '(' lpvalue '=' expression ';' expression ';' lpvalue '=' expression ')' statement
		{
                  expression* exp1 = db_create_statement( @1.first_line, $5 );
                  expression* exp2 = db_create_statement( @1.first_line, $7 );
                  expression* exp3 = db_create_statement( @1.first_line, $11 );
                  db_add_expression( $5 );
                  db_add_expression( $7 );
                  db_add_expression( $11 );
                  db_connect_statement_false( exp1, exp2 );
                  db_connect_statement_true( exp2, $13 );
                  /* Need to add loopback code */
                  $$ = exp1;
		}
	| K_for '(' lpvalue '=' expression ';' expression ';' error ')' statement
		{
		  expression_dealloc( $5, FALSE );
		  expression_dealloc( $7, FALSE );
		  // yyerror( @9, "error: Error in for loop step assignment." );
                  $$ = NULL;
		}
	| K_for '(' lpvalue '=' expression ';' error ';' lpvalue '=' expression ')' statement
		{
		  expression_dealloc( $5, FALSE );
		  expression_dealloc( $11, FALSE );
		  // yyerror( @7, "error: Error in for loop condition expression." );
                  $$ = NULL;
		}
	| K_for '(' error ')' statement
		{
		  // yyerror( @3, "error: Incomprehensible for loop." );
                  $$ = NULL;
		}
	| K_while '(' expression ')' statement
		{
                  expression* exp = db_create_statement( @1.first_line, $3 );
		  db_add_expression( $3 );
                  db_connect_statement_true( exp, $5 );
                  /* Need to add loopback code */
                  $$ = exp;
		}
	| K_while '(' error ')' statement
		{
		  // yyerror( @3, "error: Error in while loop condition." );
                  $$ = NULL;
		}
	| delay1 statement_opt
		{
                  printf( "In delay\n" );
                  $$ = NULL;
		}
	| event_control statement_opt
		{
                  $$ = NULL;
		}
	| lpvalue '=' expression ';'
		{
                  expression* exp = db_create_statement( @1.first_line, $3 );
                  printf( "In assignment\n" );
		  db_add_expression( $3 );
                  $$ = exp;
		}
	| lpvalue K_LE expression ';'
		{
		  /* Add root expression (non-blocking) */
                  expression* exp = db_create_statement( @1.first_line, $3 );
		  db_add_expression( $3 );
                  $$ = exp;
		}
	| lpvalue '=' delay1 expression ';'
		{
		  expression* exp = db_create_statement( @1.first_line, $4 );
		  db_add_expression( $4 );
                  $$ = exp;
		}
	| lpvalue K_LE delay1 expression ';'
		{
		  expression* exp = db_create_statement( @1.first_line, $4 );
		  db_add_expression( $4 );
                  $$ = exp;
		}
	| lpvalue '=' event_control expression ';'
		{
		  expression* exp = db_create_statement( @1.first_line, $4 );
		  db_add_expression( $4 );
                  $$ = exp;
		}
	| lpvalue '=' K_repeat '(' expression ')' event_control expression ';'
		{
                  expression* exp = db_create_statement( @1.first_line, $5 );
		  db_add_expression( $5 );
		  db_add_expression( $8 );
                  $$ = exp;
		}
	| lpvalue K_LE event_control expression ';'
		{
                  expression* exp = db_create_statement( @1.first_line, $4 );
		  db_add_expression( $4 );
                  $$ = exp;
		}
	| lpvalue K_LE K_repeat '(' expression ')' event_control expression ';'
		{
                  expression* exp = db_create_statement( @1.first_line, $5 );
		  db_add_expression( $5 );
		  db_add_expression( $8 );
		  $$ = exp;
		}
	| K_wait '(' expression ')' statement_opt
		{
                  expression* exp = db_create_statement( @1.first_line, $3 );
                  db_add_expression( $3 );
                  db_connect_statement_false( exp, $5 );
		  $$ = exp;
		}
	| SYSTEM_IDENTIFIER '(' expression_list ')' ';'
		{
                  printf( "In SYSTEM_IDENTIFIER\n" );
		  expression_dealloc( $3, FALSE );
                  $$ = NULL;
		}
	| SYSTEM_IDENTIFIER ';'
		{
                  $$ = NULL;
		}
	| identifier '(' expression_list ')' ';'
		{
		  expression_dealloc( $3, FALSE );
                  $$ = NULL;
		}
	| identifier ';'
		{
		  /* This is a task call */
                  $$ = NULL;
		}
	| error ';'
		{
		  // yyerror( @1, "error: Malformed statement." );
		  // yyerrok;
                  $$ = NULL;
		}
	;

statement_list
	: statement statement_list
                {
                  if( $2 != NULL ) {
                    db_connect_statement_false( $1, $2 );
                  }
                  if( $1 == NULL ) {
                    $$ = $2;
                  } else {
                    $$ = $1;
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
	| identifier '[' static_expr ']'
	| identifier '[' static_expr ':' static_expr ']'
	| '{' expression_list '}'
		{
		  $$ = 0;
		}
	;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
	: identifier
	| identifier '[' static_expr ']'
	| identifier range
		{
		  free_safe( $2 );
		}
	| '{' expression_list '}'
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
		  while( curr != NULL ) {
		    db_add_signal( curr->str, $2->width, $2->lsb, 0 );
		    curr = curr->next;
		  }
		  str_link_delete_list( tmp );
		  free_safe( $2 );
		}
	| K_reg register_variable_list ';'
		{
		  /* Create new signal */
		  str_link* tmp  = $2;
		  str_link* curr = tmp;
		  while( curr != NULL ) {
		    db_add_signal( curr->str, 1, 0, 0 );
		    curr = curr->next;
		  }
		  str_link_delete_list( tmp );
		}
	| K_reg K_signed range register_variable_list ';'
		{
		  /* Create new signal */
		  str_link* tmp  = $4;
                  str_link* curr = tmp;
                  while( curr != NULL ) {
                    db_add_signal( curr->str, $3->width, $3->lsb, 0 );
                    curr = curr->next;
                  }
                  str_link_delete_list( tmp );
		  free_safe( $3 );
		}
	| K_reg K_signed register_variable_list ';'
		{
		  /* Create new signal */
                  str_link* tmp  = $3;
                  str_link* curr = tmp;
                  while( curr != NULL ) {
                    db_add_signal( curr->str, 1, 0, 0 );
                    curr = curr->next;
                  }
                  str_link_delete_list( tmp );
		}
	| K_integer register_variable_list ';'
		{
		  str_link_delete_list( $2 );
		}
	| K_time register_variable_list ';'
		{
		  str_link_delete_list( $2 );
		}
	| K_real list_of_variables ';'
		{
		  str_link_delete_list( $2 );
		}
	| K_realtime list_of_variables ';'
		{
		  str_link_delete_list( $2 );
		}
	| K_parameter parameter_assign_list ';'
	| K_localparam localparam_assign_list ';'
	| K_reg error ';'
		{
		  // yyerror( @1, "error: syntax error in reg variable list." );
		  // yyerrok;
		}
 	;	

case_item
	: expression_list ':' statement_opt
		{
		  /* Create conditional */
		}
	| K_default ':' statement_opt
		{
		  /* Create conditional */
		}
	| K_default statement_opt
		{
		  /* Create conditional */
		}
	| error ':' statement_opt
		{
		  // yyerror( @1, "error: Incomprehensible case expression." );
		  // yyerrok;
		}
	;

case_items
	: case_items case_item
	| case_item
	;

delay1
	: '#' delay_value_simple
	| '#' '(' delay_value ')'
	;

delay3
	: '#' delay_value_simple
	| '#' '(' delay_value ')'
	| '#' '(' delay_value ',' delay_value ')'
	| '#' '(' delay_value ',' delay_value ',' delay_value ')'
	;

delay3_opt
	: delay3
	|
	;

delay_value
	: static_expr
	| static_expr ':' static_expr ':' static_expr
	;

delay_value_simple
	: NUMBER
		{
		  $$ = vector_to_int( $1 );
		  vector_dealloc( $1 );
		}
	| REALTIME
		{
		  $$ = 0;
		}
	| IDENTIFIER
		{
		  $$ = 0;
		}
	;

assign_list
	: assign_list ',' assign
	| assign
	;

assign
	: lavalue '=' expression
		{
		  db_add_expression( $3 );
		}
	;

range_opt
	: range
		{
		  $$ = $1;
		}
	|
		{
		  signal_width* tmp = (signal_width*)malloc( sizeof( signal_width ) );
		  tmp->width = 1;
		  tmp->lsb   = 0;
		  $$ = tmp;
		}
;

range
	: '[' static_expr ':' static_expr ']'
		{
		  signal_width* tmp = (signal_width*)malloc( sizeof( signal_width ) );
		  if( $2 >= $4 ) {
		    tmp->width = ($2 - $4) + 1;
		    tmp->lsb   = $4;
		  } else {
		    tmp->width = $4 - $2;
		    tmp->lsb   = $2;
		  }
		  $$ = tmp;
		}
	;

range_or_type_opt
	: range      { $$ = 0;  free_safe( $1 ); }
	| K_integer  { $$ = 0; }
	| K_real     { $$ = 0; }
	| K_realtime { $$ = 0; }
	| K_time     { $$ = 0; }
	|            { $$ = 0; }
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
	| IDENTIFIER '=' expression
		{
		  $$ = $1;
		}
	| IDENTIFIER '[' static_expr ':' static_expr ']'
		{
		  /* We don't support memory coverage */
		  $$ = 0;
		}
	;

register_variable_list
	: register_variable
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $1;
		  tmp->next = NULL;
		  $$ = tmp;
		}
	| register_variable_list ',' register_variable
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $3;
		  tmp->next = $1;
		  $$ = tmp;
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
	| K_input range_opt list_of_variables ';'
		{
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| K_output range_opt list_of_variables ';'
		{
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| K_inout range_opt list_of_variables ';'
		{
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	;

net_type
	: K_wire    { $$ = 1; }
	| K_tri     { $$ = 0; }
	| K_tri1    { $$ = 0; }
	| K_supply0 { $$ = 0; }
	| K_wand    { $$ = 0; }
	| K_triand  { $$ = 0; }
	| K_tri0    { $$ = 0; }
	| K_supply1 { $$ = 0; }
	| K_wor     { $$ = 0; }
	| K_trior   { $$ = 0; }
	;

net_decl_assigns
	: net_decl_assigns ',' net_decl_assign
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $3;
		  tmp->next = $1;
		  $$ = tmp;
		}
	| net_decl_assign
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link) );
		  tmp->str  = $1;
		  tmp->next = NULL;
		  $$ = tmp;
		}
	;

net_decl_assign
	: IDENTIFIER '=' expression
		{
		  /* Create root expression */
		  $$ = $1;
		}
	| delay1 IDENTIFIER '=' expression
		{
		  /* Create root expression */
		  $$ = $2;
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
		  signal*     sig = db_find_signal( $2 );
		  expression* tmp;
		  if( sig != NULL ) {
		    tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, NULL );
		    vector_dealloc( tmp->value );
		    tmp->value = sig->value;
		    $$ = tmp;
		  } else {
		    $$ = NULL;
		  }
		  free_safe( $2 );
		}
	| '@' '(' event_expression_list ')'
		{
		  $$ = $3;
		}
	| '@' '(' error ')'
		{
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
		  tmp = db_create_expression( $3, $1, EXP_OP_LOR, @1.first_line, NULL );
		  $$ = tmp;
		}
	| event_expression_list ',' event_expression
		{
		  expression_dealloc( $1, FALSE );
		  expression_dealloc( $3, FALSE );
		}
	;

event_expression
	: K_posedge expression
		{
		  expression* tmp;
		  tmp = db_create_expression( $2, NULL, EXP_OP_PEDGE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| K_negedge expression
		{
		  expression* tmp;
		  tmp = db_create_expression( $2, NULL, EXP_OP_NEDGE, @1.first_line, NULL );
		  $$ = tmp;
		}
	| expression
		{
		  expression* tmp;
		  tmp = db_create_expression( $1, NULL, EXP_OP_AEDGE, @1.first_line, NULL );
		  $$ = tmp;
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
		  expression_dealloc( $3, FALSE );
		}
	;

parameter_value_opt
	: '#' '(' expression_list ')'
		{
		  expression_dealloc( $3, FALSE );
		}
	| '#' '(' parameter_value_byname_list ')'
	| '#' NUMBER
	| '#' error
	|
	;

parameter_value_byname_list
	: parameter_value_byname
	| parameter_value_byname_list ',' parameter_value_byname
	;

parameter_value_byname
	: PORTNAME '(' expression ')'
		{
		  expression_dealloc( $3, FALSE );
		}
	| PORTNAME '(' ')'
	;

gate_instance_list
	: gate_instance_list ',' gate_instance
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $3;
		  tmp->next = $1;
		  $$ = tmp;
		}
	| gate_instance
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $1;
		  tmp->next = NULL;
		  $$ = tmp;
		}
	;

  /* A gate_instance is a module instantiation or a built in part
     type. In any case, the gate has a set of connections to ports. */
gate_instance
	: IDENTIFIER '(' expression_list ')'
		{
		  expression_dealloc( $3, FALSE );
		  $$ = $1;
		}
	| IDENTIFIER range '(' expression_list ')'
		{
		  expression_dealloc( $4, FALSE );
		  free_safe( $2 );
		  $$ = $1;
		}
	| '(' expression_list ')'
		{
		  expression_dealloc( $2, FALSE );
		}
	| IDENTIFIER '(' port_name_list ')'
		{
		  $$ = $1;
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
	: K_input range_opt list_of_variables ';'
		{
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| block_item_decl
	;

parameter_assign_list
	: parameter_assign
	| range parameter_assign
		{
		  $$ = 0;
		}
	| parameter_assign_list ',' parameter_assign

parameter_assign
	: IDENTIFIER '=' expression
		{
		  expression_dealloc( $3, FALSE );
		}
	;

localparam_assign_list
	: localparam_assign
	| range localparam_assign
		{
		  $$ = 0;
		}
	| localparam_assign_list ',' localparam_assign
	;

localparam_assign
	: IDENTIFIER '=' expression
		{
		  expression_dealloc( $3, FALSE );
		}
	;

port_name_list
	: port_name_list ',' port_name
	| port_name
	;

port_name
	: PORTNAME '(' expression ')'
		{
		  expression_dealloc( $3, FALSE );
		}
	| PORTNAME '(' error ')'
	| PORTNAME '(' ')'
	;

