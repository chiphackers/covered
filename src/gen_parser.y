
%{
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
 \file     gen_parser.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/23/2010
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
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "generator.h"
#include "link.h"
#include "obfuscate.h"
#include "parser_func.h"
#include "parser_misc.h"
#include "profiler.h"
#include "statement.h"
#include "static.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"

extern char   user_msg[USER_MSG_LENGTH];
extern int    delay_expr_type;
extern int    stmt_conn_id;
extern int    gi_conn_id;
extern isuppl info_suppl;
extern char*  file_version;
extern int    curr_sig_type;

/* Functions from lexer */
extern void lex_start_udp_table();
extern void lex_end_udp_table();

/* TBD - We won't be needing these later */
extern int ignore_mode;
extern sig_range curr_prange;
extern sig_range curr_urange;
extern bool curr_signed;
extern bool lhs_mode;


/*!
 Specifies whether we are running to parse (TRUE) or to generate inline coverage code (FALSE).
*/
bool parse_mode = TRUE;

/*!
 If set to a value > 0, specifies that we are parsing a parameter expression
*/
int param_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing an attribute
*/
int attr_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing the control block of a for loop.
*/
int for_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing a generate block
*/
int generate_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing the top-most level of structures in a generate block.
*/
int  generate_top_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing the body of a generate loop
*/
int  generate_for_mode = 0;

/*!
 If set to a value > 0, specifies that we are creating generate expressions
*/
int  generate_expr_mode = 0;

/*!
 Points to current generate variable name.
*/
char* generate_varname = NULL;

/*!
 Array storing the depth that a given fork block is at.
*/
int* fork_block_depth;

/*!
 Current fork depth position in the fork_block_depth array.
*/
int  fork_depth  = -1;

/*!
 Specifies the current block depth (for embedded blocks).
*/
int  block_depth = 0; 

/*!
 Set to TRUE if we are currently parsing a static expression.
*/
bool in_static_expr = FALSE;


/*!
 Pointer to head of parameter override list.
*/
param_oride* param_oride_head = NULL;

/*!
 Pointer to tail of parameter override list.
*/
param_oride* param_oride_tail = NULL;

/*!
 Specifies if the current variable list is handled by Covered or not.
*/
bool curr_handled = TRUE;

/*!
 Specifies if current signal must be assigned or not (by Covered).
*/
bool curr_mba = FALSE;

/*!
 Specifies if current range should be flagged as packed or not.
*/
bool curr_packed = TRUE;

/*!
 Pointer to head of stack structure that stores the contents of generate
 items that we want to refer to later.
*/
gitem_link* save_gi_head = NULL;

/*!
 Pointer to tail of stack structure that stores the contents of generate
 items that we want to refer to later.
*/
gitem_link* save_gi_tail = NULL;

/*!
 Generate block index.
*/
int generate_block_index = 0;

/*!
 Starting line number of last parsed FOR keyword.
*/
unsigned int for_start_line = 0;

/*!
 Starting column of last parsed FOR keyword.
*/
unsigned int for_start_col = 0;

/*!
 Macro to free a text variable type.  Sets the pointer to NULL so that the pointer is re-deallocated.
 x = pointer to the variable.
*/
#define FREE_TEXT(x)  free_safe( x, (strlen( x ) + 1) );  x = NULL;

#define YYERROR_VERBOSE 1

/* Uncomment these lines to turn debugging on */
/*
#define YYDEBUG 1
int yydebug = 1; 
*/

/* Recent version of bison expect that the user supply a
   YYLLOC_DEFAULT macro that makes up a yylloc value from existing
   values. I need to supply an explicit version to account for the
   orig_fname and incl_fname fields, that otherwise won't be copied. */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
          (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
          (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
          (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
          (Current).orig_fname   = YYRHSLOC(Rhs, 1).orig_fname;         \
          (Current).incl_fname   = YYRHSLOC(Rhs, 1).incl_fname;         \
          (Current).ppfline      = YYRHSLOC(Rhs, 1).ppfline;            \
          (Current).pplline      = YYRHSLOC(Rhs, N).pplline;            \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC(Rhs, 0).last_line;                                 \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC(Rhs, 0).last_column;                               \
          (Current).orig_fname   = YYRHSLOC(Rhs, 0).orig_fname;         \
          (Current).incl_fname   = YYRHSLOC(Rhs, 0).incl_fname;         \
          (Current).ppfline      = YYRHSLOC(Rhs, 0).ppfline;            \
          (Current).pplline      = YYRHSLOC(Rhs, 0).pplline;            \
        }                                                               \
    while (0)

%}

%union {
  bool            logical;
  char*           text;
  int	          integer;
  const_value     num;
  exp_op_type     optype;
  vector*         realtime;
  vsignal*        sig;
  expression*     expr;
  statement*      state;
  static_expr*    statexp;
  str_link*       strlink;
  case_statement* case_stmt;
  attr_param*     attr_parm;
  exp_bind*       expbind;
  port_info*      portinfo;
  gen_item*       gitem;
  case_gitem*     case_gi;
  typedef_item*   typdef;
  func_unit*      funit;
  stmt_pair       stmtpair;
  gitem_pair      gitempair;
};

%token <text>     IDENTIFIER
%token <typdef>   TYPEDEF_IDENTIFIER
%token <text>     PATHPULSE_IDENTIFIER
%token <text>     DEC_NUMBER BASE_NUMBER
%token <realtime> REALTIME
%token <num>      STRING
%token IGNORE
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_LSS K_RS K_RSS K_SG
%token K_ADD_A K_SUB_A K_MLT_A K_DIV_A K_MOD_A K_AND_A K_OR_A K_XOR_A K_LS_A K_RS_A K_ALS_A K_ARS_A K_INC K_DEC K_POW
%token K_PO_POS K_PO_NEG K_STARP K_PSTAR
%token K_LOR K_LAND K_NAND K_NOR K_NXOR K_TRIGGER
%token K_always K_and K_assign K_begin K_buf K_bufif0 K_bufif1 K_case
%token K_casex K_casez K_cmos K_deassign K_default K_defparam K_disable
%token K_edge K_else K_end K_endcase K_endfunction K_endmodule I_endmodule
%token K_endprimitive K_endspecify K_endtable K_endtask K_event K_for
%token K_force K_forever K_fork K_function K_highz0 K_highz1 K_if
%token K_generate K_endgenerate K_genvar
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
%token S_user S_ignore S_allow S_finish S_stop S_time S_random S_srandom S_dumpfile S_urandom S_urandom_range
%token S_realtobits S_bitstoreal S_rtoi S_itor S_shortrealtobits S_bitstoshortreal S_testargs S_valargs
%token S_signed S_unsigned S_clog2

%token K_automatic K_cell K_use K_library K_config K_endconfig K_design K_liblist K_instance
%token K_showcancelled K_noshowcancelled K_pulsestyle_onevent K_pulsestyle_ondetect

%token K_TU_S K_TU_MS K_TU_US K_TU_NS K_TU_PS K_TU_FS K_TU_STEP
%token K_INCDIR K_DPI
%token K_PP K_PS K_PEQ K_PNE
%token K_bit K_byte K_char K_logic K_shortint K_int K_longint K_unsigned K_shortreal
%token K_unique K_priority K_do
%token K_always_comb K_always_latch K_always_ff
%token K_typedef K_type K_enum K_union K_struct K_packed
%token K_assert K_assume K_property K_endproperty K_cover K_sequence K_endsequence K_expect
%token K_program K_endprogram K_final K_void K_return K_continue K_break K_extern K_interface K_endinterface
%token K_class K_endclass K_extends K_package K_endpackage K_timeunit K_timeprecision K_ref K_bind K_const
%token K_new K_static K_protected K_local K_rand K_randc K_randcase K_constraint K_import K_export K_scalared K_chandle
%token K_context K_pure K_modport K_clocking K_iff K_intersect K_first_match K_throughout K_with K_within
%token K_dist K_covergroup K_endgroup K_coverpoint K_optionDOT K_type_optionDOT K_wildcard
%token K_bins K_illegal_bins K_ignore_bins K_cross K_binsof K_alias K_join_any K_join_none K_matches
%token K_tagged K_foreach K_randsequence K_ifnone K_randomize K_null K_inside K_super K_this K_wait_order
%token K_include K_Sroot K_Sunit K_endclocking K_virtual K_before K_forkjoin K_solve

%token KK_attribute

%type <num>       number
%type <logical>   automatic_opt block_item_decls_opt
%type <integer>   net_type net_type_sign_range_opt var_type data_type_opt
%type <text>      identifier begin_end_id
%type <statexp>   static_expr static_expr_primary static_expr_port_list
%type <expr>      expr_primary expression_list expression expression_port_list expression_systask_list
%type <expr>      lavalue lpvalue
%type <expr>      event_control event_expression_list event_expression
%type <expr>      delay_value delay_value_simple
%type <expr>      delay1 delay3 delay3_opt
%type <expr>      generate_passign index_expr single_index_expr
%type <state>     statement statement_list statement_or_null for_condition
%type <state>     if_statement_error
%type <state>     passign for_initialization
%type <state>     expression_assignment_list
%type <strlink>   gate_instance gate_instance_list list_of_names
%type <funit>     begin_end_block fork_statement inc_for_depth
%type <attr_parm> attribute attribute_list
%type <portinfo>  port_declaration list_of_port_declarations
%type <gitem>     generate_item generate_item_list generate_item_list_opt
%type <case_stmt> case_items case_item case_body
%type <case_gi>   generate_case_items generate_case_item
%type <optype>    static_unary_op unary_op syscall_wo_parms_op syscall_w_parms_op syscall_w_parms_op_64 syscall_w_parms_op_32
%type <optype>    pre_op_and_assign_op post_op_and_assign_op op_and_assign_op
%type <stmtpair>  if_body
%type <gitempair> gen_if_body

%token K_TAND
%right '?' ':'
%left K_LOR
%left K_LAND
%left '|'
%left '^' K_NXOR K_NOR
%left '&' K_NAND
%left K_EQ K_NE K_CEQ K_CNE
%left K_GE K_LE '<' '>'
%left K_LS K_RS K_LSS K_RSS
%left '+' '-'
%left '*' '/' '%' K_POW
%left UNARY_PREC

/* to resolve dangling else ambiguity: */
%nonassoc less_than_K_else
%nonassoc K_else

/* Make sure that string elements are deallocated if parser errors occur */
%destructor { FREE_TEXT( $$ ); } IDENTIFIER PATHPULSE_IDENTIFIER DEC_NUMBER BASE_NUMBER identifier begin_end_id

%%

  /* A degenerate source file can be completely empty. */
main 
  : source_file 
    {
      generator_output( $1 );
    }
  |
  ;

  /* Verilog-2001 supports attribute lists, which can be attached to a
     variety of different objects. The syntax inside the (* *) is a
     comma separated list of names or names with assigned values. */
attribute_list_opt
  : K_PSTAR attribute_list K_STARP
    {
      $$ = generator_build( 3, strdup_safe( "(*" ), $2, strdup_safe( "*)" ) );
    }
  | K_PSTAR K_STARP
    {
      $$ = strdup_safe( "(* *)" );
    }
  |
    {
      $$ = NULL;
    }
  ;

attribute_list
  : attribute_list ',' attribute
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    } 
  | attribute
    {
      $$ = $1;
    }
  ;

attribute
  : IDENTIFIER
    {
      $$ = $1;
    }
  | IDENTIFIER '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  ;

source_file 
  : description 
    {
      $$ = $1;
    }
  | source_file description
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

description
  : module
    {
      $$ = $1;
    }
  | udp_primitive
    {
      $$ = $1;
    }
  | KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')'
    {
      $$ = generator_build( 7, strdup_safe( "$attribute(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, strdup_safe( ")" ) );
    }
  | typedef_decl
    {
      $$ = $1;
    }
  | net_type signed_opt range_opt list_of_variables ';'
    {
      $$ = generator_build( 6, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | net_type signed_opt range_opt
    net_decl_assigns ';'
    {
      $$ = generator_build( 6, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | net_type drive_strength
    net_decl_assigns ';'
    {
      $$ = generator_build( 5, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  | K_trireg charge_strength_opt range_opt delay3_opt
    list_of_variables ';'
    {
      $$ = generator_build( 7, strdup_safe( "trireg" ), $2, $3, $4, R5, strdup_safe( ";" ), "\n" );
    }
  | gatetype gate_instance_list ';'
    {
      $$ = generator_buildln( 3, $1, $2, strdup_safe( ";" ) );
    }
  | gatetype delay3 gate_instance_list ';'
    {
      $$ = generator_buildln( 4, $1, $2, $3, strdup_safe( ";" ) );
    }
  | gatetype drive_strength gate_instance_list ';'
    {
      $$ = generator_buildln( 4, $1, $2, $3, strdup_safe( ";" ) );
    }
  | gatetype drive_strength delay3 gate_instance_list ';'
    {
      $$ = generator_buildln( 5, $1, $2, $3, $4, strdup_safe( ";" ) );
    }
  | K_pullup gate_instance_list ';'
    {
      $$ = generator_buildln( 3, strdup_safe( "pullup" ), $2, strdup_safe( ";" ) );
    }
  | K_pulldown gate_instance_list ';'
    {
      $$ = generator_buildln( 3, strdup_safe( "pulldown" ), $2, strdup_safe( ";" ) );
    }
  | K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      $$ = generator_buildln( 5, strdup_safe( "pullup(" ), $3, strdup_safe( ")" ), $5, strdup_safe( ";" ) );
    }
  | K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      $$ = generator_buildln( 5, strdup_safe( "pulldown(" ), $3, strdup_safe( ")" ), $5, strdup_safe( ";" ) );
    }
  | block_item_decl
    {
      $$ = $1;
    }
  | K_event list_of_variables ';'
    {
      /* Events are handled as regs with the generator */
      $$ = generator_build( 4, strdup_safe( "reg" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_task automatic_opt IDENTIFIER ';'
    task_item_list_opt statement_or_null
    K_endtask
    {
      func_unit* funit = db_get_tfn_by_position( @3.first_line, @3.first_column );
      $$ = generator_build( 10, strdup_safe( "task" ), $2, $3, strdup_safe( ";" ), "\n", $5, generator_inst_id_reg( funit ), $6, strdup_safe( "endtask" ), "\n" );
    }
  | K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    function_item_list
    statement
    K_endfunction
    {
      func_unit* funit = db_get_tfn_by_position( @5.first_line, @5.first_column );
      /* If this is not an automatic function, place all intermediate signals within the function */
      if( generator_is_static_function( funit ) ) {
        $$ = generator_build( 12, strdup_safe( "function" ), $2, $3, $4, $5, strdup_safe( ";" ), "\n", $7,
                             (generator_is_static_function( funit ) ? generator_inst_id_reg( funit ) : NULL), $8, strdup_safe( "endfunction" ), "\n" );
      }
    }
  | error ';'
    {
      VLerror( "Invalid $root item" );
      FREE_TEXT( $1 );
      $$ = NULL;
    }
  | K_function error K_endfunction
    {
      VLerror( "Syntax error in function description" );
      FREE_TEXT( $2 );
      $$ = NULL;
    }
  | enumeration list_of_names ';'
    {
      $$ = generator_build( 4, $1, $2, strdup_safe( ";" ), "\n" );
    }
  ;

module
  : attribute_list_opt module_start IDENTIFIER 
    module_parameter_port_list_opt
    module_port_list_opt ';'
    module_item_list_opt K_endmodule
    {
      $$ = generator_build( 9, $1, $2, $3, $4, $5, strdup_safe( ";" ), $7, generator_inst_id_overrides(), strdup_safe( "endmodule" ) );
    }
  | attribute_list_opt K_module IGNORE I_endmodule
    {
      $$ = generator_build( 4, $1, strdup_safe( "module" ), $3, strdup_safe( "endmodule" ) );
    }
  | attribute_list_opt K_macromodule IGNORE I_endmodule
    {
      $$ = generator_build( 4, $1, strdup_safe( "macromodule" ), $3, strdup_safe( "endmodule" ) );
    }
  ;

module_start
  : K_module
    {
      $$ = strdup_safe( "module" );
    }
  | K_macromodule
    {
      $$ = strdup_safe( "macromodule" );
    }
  ;

module_parameter_port_list_opt
  : '#' '(' module_parameter_port_list ')'
    {
      $$ = generator_build( 3, strdup_safe( "#(" ), $2, strdup_safe( ")" ) );
    }
  |
    {
      $$ = NULL;
    }
  ;

module_parameter_port_list
  : K_parameter signed_opt range_opt parameter_assign
    {
      $$ = generator_build( 4, strdup_safe( "parameter" ), $2, $3, $4 );
    }
  | module_parameter_port_list ',' parameter_assign
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | module_parameter_port_list ',' K_parameter signed_opt range_opt parameter_assign
    {
      $$ = generator_build( 6, $1, strdup_safe( "," ), strdup_safe( "parameter" ), $4, $5, $6 );
    }
  ;

module_port_list_opt
  : '(' list_of_ports ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ")" ) );
    }
  | '(' list_of_port_declarations ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ")" ) );
    }
  |
    {
      $$ = NULL;
    }
  ;

  /* Verilog-2001 syntax for declaring all ports within module port list */
list_of_port_declarations
  : port_declaration
    {
      $$ = $1;
    }
  | list_of_port_declarations ',' port_declaration
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | list_of_port_declarations ',' IDENTIFIER
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

  /* Handles Verilog-2001 port of type:  input wire [m:l] <list>; */
port_declaration
  : attribute_list_opt port_type net_type_sign_range_opt IDENTIFIER
    {
      $$ = generator_build( 4, $1, $2, $3, $4 );
    }
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER
    {
      $$ = generator_build( 6, $1, $2, $3, $4, $5, $6 );
    }
  /* We just need to parse the static register assignment as this signal will get its value from the dumpfile */
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER '=' static_expr
    {
      $$ = generator_build( 8, $1, $2, $3, $4, $5, $6, strdup_safe( "=" ), $8 );
    }
  | attribute_list_opt port_type net_type_sign_range_opt error
    {
      FREE_TEXT( generator_build( 4, $1, $2, $3, $4 ) );
      $$ = NULL;
    }
  | attribute_list_opt K_output var_type signed_opt range_opt error
    {
      FREE_TEXT( generator_build( 6, $1, $2, $3, $4, $5, $6 ) );
      $$ = NULL;
    }
  ;

list_of_ports
  : list_of_ports ',' port_opt
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | port_opt
    {
      $$ = $1;
    }
  ;

port_opt
  : port
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

  /* Coverage tools should not find port information that interesting.  We will
     handle it for parsing purposes, but ignore its information. */
port
  : port_reference
    {
      $$ = $1;
    }
  | '.' IDENTIFIER '(' port_reference ')'
    {
      $$ = generator_build( 5, strdup_safe( "." ), $1, strdup_safe( "(" ), $4, strdup_safe( ")" ) );
    }
  | '{' port_reference_list '}'
    {
      $$ = generator_build( 3, strdup_safe( "{" ), $2, strdup_safe( "}" ) );
    }
  | '.' IDENTIFIER '(' '{' port_reference_list '}' ')'
    {
      $$ = generator_build( 5, strdup_safe( "." ), $2, strdup_safe( "({" ), $5, strdup_safe( "})" ) );
    }
  ;

port_reference
  : IDENTIFIER
    {
      $$ = $1;
    }
  | IDENTIFIER '[' static_expr ':' static_expr ']'
    {
      $$ = generator_build( 6, $1, strdup_safe( "[" ), $3, strdup_safe( ":" ), $5, strdup_safe( "]" ) ); 
    }
  | IDENTIFIER '[' static_expr ']'
    {
      $$ = generator_build( 4, $1, strdup_safe( "[" ), $3, strdup_safe( "]" ) );
    }
  | IDENTIFIER '[' error ']'
    {
      FREE_TEXT( $1 );
      $$ = NULL;
    }
  ;

port_reference_list
  : port_reference
    {
      $$ = $1;
    }
  | port_reference_list ',' port_reference
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

number
  : DEC_NUMBER
    {
      $$ = $1;
    }
  | BASE_NUMBER
    { 
      $$ = $1;
    }
  | DEC_NUMBER BASE_NUMBER
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

static_expr_port_list
  : static_expr_port_list ',' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | static_expr
    {
      $$ = $1;
    }
  ;

static_unary_op
  : '~'    { $$ = strdup_safe( "~" );  }
  | '!'    { $$ = strdup_safe( "!" );  }
  | '&'    { $$ = strdup_safe( "&" );  }
  | '|'    { $$ = strdup_safe( "|" );  }
  | '^'    { $$ = strdup_safe( "^" );  }
  | K_NAND { $$ = strdup_safe( "~&" ); }
  | K_NOR  { $$ = strdup_safe( "~|" ); }
  | K_NXOR { $$ = strdup_safe( "~^" ); }
  ;

static_expr
  : static_expr_primary
    {
      $$ = $1;
    }
  | '+' static_expr_primary %prec UNARY_PREC
    {
      $$ = generator_build( 2, strdup_safe( "+" ), $2 );
    }
  | '-' static_expr_primary %prec UNARY_PREC
    {
      $$ = generator_build( 2, strdup_safe( "-" ), $2 );
    }
  | static_unary_op static_expr_primary %prec UNARY_PREC
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | static_expr '^' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "^" ), $3 );
    }
  | static_expr '*' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "*" ), $3 );
    }
  | static_expr '/' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "/" ), $3 );
    }
  | static_expr '%' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "%" ), $3 );
    }
  | static_expr '+' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "+" ), $3 );
    }
  | static_expr '-' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "-" ), $3 );
    }
  | static_expr '&' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "&" ), $3 );
    }
  | static_expr '|' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "|" ), $3 );
    }
  | static_expr K_NOR static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "~|" ), $3 );
    }
  | static_expr K_NAND static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "~&" ), $3 );
    }
  | static_expr K_NXOR static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "~^" ), $3 );
    }
  | static_expr K_LS static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "<<" ), $3 );
    }
  | static_expr K_RS static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( ">>" ), $3 );
    }
  | static_expr K_GE static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( ">=" ), $3 );
    }
  | static_expr K_LE static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "<=" ), $3 );
    }
  | static_expr K_EQ static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "==" ), $3 );
    }
  | static_expr K_NE static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "!=" ), $3 );
    }
  | static_expr '>' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( ">" ), $3 );
    }
  | static_expr '<' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "<" ), $3 );
    }
  | static_expr K_LAND static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "&&" ), $3 );
    }
  | static_expr K_LOR static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "||" ), $3 );
    }
  | static_expr K_POW static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "**" ), $3 );
    }
  | static_expr '?' static_expr ':' static_expr
    {
      $$ = generator_build( 5, $1, strdup_safe( "?" ), $3, strdup_safe( ":" ), $5 );
    }
  ;

static_expr_primary
  : number
    {
      $$ = $1;
    }
  | REALTIME
    {
      $$ = $1;
    }
  | IDENTIFIER
    {
      $$ = $1;
    }
  | '(' static_expr ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ")" ) );
    }
  | IDENTIFIER '(' static_expr_port_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" ) );
    }
  | IDENTIFIER '[' static_expr ']'
    {
      $$ = generator_build( 4, $1, strdup_safe( "[" ), $3, strdup_safe( "]" ) );
    }
  | SYSCALL
    {
      $$ = $1;
    }
  | S_clog2 '(' static_expr ')'
    {
      $$ = generator_build( 3, strdup_safe( "$clog2(" ), $3, strdup_safe( ")" ) );
    }
  ;

unary_op
  : '~'     { $$ = strdup_safe( "~" );  }
  | '!'     { $$ = strdup_safe( "!" );  }
  | '&'     { $$ = strdup_safe( "&" );  }
  | '|'     { $$ = strdup_safe( "|" );  }
  | '^'     { $$ = strdup_safe( "^" );  }
  | K_NAND  { $$ = strdup_safe( "~&" ); }
  | K_NOR   { $$ = strdup_safe( "~|" ); }
  | K_NXOR  { $$ = strdup_safe( "~^" ); }
  ;

expression
  : expr_primary
    {
      $$ = $1;
    }
  | '+' expr_primary %prec UNARY_PREC
    {
      $$ = generator_build( 2, strdup_safe( "+" ), $2 );
    }
  | '-' expr_primary %prec UNARY_PREC
    {
      $$ = generator_build( 2, strdup_safe( "-" ), $2 );
    }
  | unary_op expr_primary %prec UNARY_PREC
    {
      $$ = generator_build( 2, $1, $2 );
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
      $$ = generator_build( 3, $1, strdup_safe( "^" ), $3 );
    }
  | expression '*' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "*" ), $3 );
    }
  | expression '/' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "/" ), $3 );
    }
  | expression '%' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "%" ), $3 );
    }
  | expression '+' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "+" ), $3 );
    }
  | expression '-' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "-" ), $3 );
    }
  | expression '&' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "&" ), $3 );
    }
  | expression '|' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "|" ), $3 );
    }
  | expression K_NAND expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "~&" ), $3 );
    }
  | expression K_NOR expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "~|" ), $3 );
    }
  | expression K_NXOR expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "~^" ), $3 );
    }
  | expression '<' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "<" ), $3 );
    }
  | expression '>' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( ">" ), $3 );
    }
  | expression K_LS expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "<<" ), $3 );
    }
  | expression K_RS expression
    {
      $$ = generator_build( 3, $1, strdup_safe( ">>" ), $3 );
    }
  | expression K_EQ expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "==" ), $3 );
    }
  | expression K_CEQ expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "===" ), $3 );
    }
  | expression K_LE expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "<=" ), $3 );
    }
  | expression K_GE expression
    {
      $$ = generator_build( 3, $1, strdup_safe( ">=" ), $3 );
    }
  | expression K_NE expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "!=" ), $3 );
    }
  | expression K_CNE expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "!==" ), $3 );
    }
  | expression K_LOR expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "||" ), $3 );
    }
  | expression K_LAND expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "&&" ), $3 );
    }
  | expression K_POW expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "**" ), $3 );
    }
  | expression K_LSS expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "<<<" ), $3 );
    }
  | expression K_RSS expression
    {
      $$ = generator_build( 3, $1, strdup_safe( ">>>" ), $3 );
    }
  | expression '?' expression ':' expression
    {
      $$ = generator_build( 5, $1, strdup_safe( "?" ), $3, strdup_safe( ":" ), $5 );
    }
  ;

syscall_w_parms_op
  : S_random          { $$ = EXP_OP_SRANDOM;      }
  | S_urandom         { $$ = EXP_OP_SURANDOM;     }
  | S_urandom_range   { $$ = EXP_OP_SURAND_RANGE; }
  | S_realtobits      { $$ = EXP_OP_SR2B;         }
  | S_rtoi            { $$ = EXP_OP_SR2I;         }
  | S_shortrealtobits { $$ = EXP_OP_SSR2B;        }
  | S_testargs        { $$ = EXP_OP_STESTARGS;    }
  | S_valargs         { $$ = EXP_OP_SVALARGS;     }
  | S_signed          { $$ = EXP_OP_SSIGNED;      }
  | S_unsigned        { $$ = EXP_OP_SUNSIGNED;    }
  ;

syscall_w_parms_op_64
  : S_bitstoreal      { $$ = EXP_OP_SB2R;   }
  | S_itor            { $$ = EXP_OP_SI2R;   }
  ;

syscall_w_parms_op_32
  : S_bitstoshortreal { $$ = EXP_OP_SB2SR;  }
  ;

pre_op_and_assign_op
  : K_INC             { $$ = strdup_safe( "++" ); }
  | K_DEC             { $$ = strdup_safe( "--" ); }
  ;

post_op_and_assign_op
  : K_INC             { $$ = strdup_safe( "++" ); }
  | K_DEC             { $$ = strdup_safe( "--" ); }
  ;

expr_primary
  : number
    {
      $$ = $1;
    }
  | REALTIME
    {
      $$ = $1;
    }
  | STRING
    {
      $$ = $1;
    }
  | identifier
    {
      $$ = $1;
    }
  | identifier post_op_and_assign_op
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | pre_op_and_assign_op identifier
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | SYSCALL
    {
      $$ = $1;
    }
  | identifier index_expr
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | identifier index_expr post_op_and_assign_op
    {
      $$ = generator_build( 3, $1, $2, $3 );
    }
  | pre_op_and_assign_op identifier index_expr
    {
      $$ = generator_build( 3, $1, $2, $3 );
    }
  | identifier '(' expression_port_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" );
    }
  | SYSCALL '(' expression_port_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" ) );
    }
  | '(' expression ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ")" ) );
    }
  | '{' expression_list '}'
    {
      $$ = generator_build( 3, strdup_safe( "{" ), $2, strdup_safe( "}" ) );
    }
  | '{' expression '{' expression_list '}' '}'
    {
      $$ = generator_build( 5, strdup_safe( "{" ), $2, strdup_safe( "{" ), $4, strdup_safe( "}}" ) );
    }
  ;

  /* Expression lists are used in concatenations and parameter overrides */
expression_list
  : expression_list ',' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
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
      $$ = generator_build( 2, $1, strdup_safe( "," ) );
    }
  ;

  /* Expression port lists are used in function/task calls */
expression_port_list
  : expression_port_list ',' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | expression
    {
      $$ = $1;
    }
  ;

  /* Expression systask lists are used in system task calls */
expression_systask_list
  : expression_systask_list ',' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | expression
    {
      $$ = $1;
    }
  ;

begin_end_id
  : ':' IDENTIFIER
    {
      $$ = generator_build( 2, strdup_safe( ":" ), $2 );
    }
  |
    {
      $$ = NULL;
    }
  ;

identifier
  : IDENTIFIER
    {
      $$ = $1;
    }
  | identifier '.' IDENTIFIER
    {
      $$ = generator_build( $1, strdup_safe( "." ), $3 );
    }
  ;

list_of_variables
  : IDENTIFIER
    {
      $$ = $1;
    }
  | IDENTIFIER range
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | list_of_variables ',' IDENTIFIER
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | list_of_variables ',' IDENTIFIER range
    {
      $$ = generator_build( 4, $1, strdup_safe( "," ), $3, $4 );
    }
  ;

udp_primitive
  : K_primitive IDENTIFIER '(' udp_port_list ')' ';'
      udp_port_decls
      udp_init_opt
      udp_body
    K_endprimitive
    {
      $$ = generator_build( 11, strdup_safe( "primitive" ), $2, strdup_safe( "(" ), $4, strdup_safe( ");" ), "\n", $7, $8, $9, strdup_safe( "endprimitive" ), "\n" );
    }
  | K_primitive IGNORE K_endprimitive
    {
      $$ = generator_build( 4, strdup_safe( "primitive" ), $2, strdup_safe( "endprimitive" ), "\n" );
    }
  ;

udp_port_list
  : IDENTIFIER
    {
      $$ = $1;
    }
  | udp_port_list ',' IDENTIFIER
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

  /* START_HERE */
udp_port_decls
  : udp_port_decl
  | udp_port_decls udp_port_decl
  ;

udp_port_decl
  : K_input ignore_more list_of_variables ignore_less ';'
  | K_output IDENTIFIER ';'
    {
      FREE_TEXT( $2 );
    }
  | K_reg IDENTIFIER ';'
    {
      FREE_TEXT( $2 );
    }
  ;

udp_init_opt
  : udp_initial
  |
  ;

udp_initial
  : K_initial IDENTIFIER '=' number ';'
    {
      if( parse_mode ) {
        vector_dealloc( $4.vec );
      }
      FREE_TEXT( $2 );
    }
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

op_and_assign_op
  : K_ADD_A  { $$ = EXP_OP_ADD_A; }
  | K_SUB_A  { $$ = EXP_OP_SUB_A; }
  | K_MLT_A  { $$ = EXP_OP_MLT_A; }
  | K_DIV_A  { $$ = EXP_OP_DIV_A; }
  | K_MOD_A  { $$ = EXP_OP_MOD_A; }
  | K_AND_A  { $$ = EXP_OP_AND_A; }
  | K_OR_A   { $$ = EXP_OP_OR_A;  }
  | K_XOR_A  { $$ = EXP_OP_XOR_A; }
  | K_LS_A   { $$ = EXP_OP_LS_A;  }
  | K_RS_A   { $$ = EXP_OP_RS_A;  }
  | K_ALS_A  { $$ = EXP_OP_ALS_A; }
  | K_ARS_A  { $$ = EXP_OP_ARS_A; }
  ;

 /* This statement type can be found in FOR statements in generate blocks */
generate_passign
  : IDENTIFIER '=' static_expr
    {
      if( parse_mode ) {
        expression* expr = NULL;
        expression* expl = NULL;
        if( ignore_mode == 0 ) {
          if( $3 != NULL ) {
            Try {
              expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
              expr = db_create_expr_from_static( $3, @3.first_line, @3.ppfline, @3.pplline, @3.first_column, (@3.last_column - 1) );
              expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              if( generate_varname == NULL ) {
                generate_varname = $1;
              } else {
                FREE_TEXT( $1 );
              }
            } Catch_anonymous {
              error_count++;
            }
            $$ = expr;
          } else {
            FREE_TEXT( $1 );
            $$ = NULL;
          }
        } else {
          FREE_TEXT( $1 );
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | IDENTIFIER op_and_assign_op static_expr
    {
      if( parse_mode ) {
        expression* expr = NULL;
        expression* expl = NULL;
        if( ignore_mode == 0 ) {
          if( $3 != NULL ) {
            Try {
              expr = db_create_expr_from_static( $3, @3.first_line, @3.ppfline, @3.pplline, @3.first_column, (@3.last_column - 1) );
              expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
              expr = db_create_expression( expr, expl, $2, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              if( generate_varname == NULL ) {
                generate_varname = $1;
              } else {
                FREE_TEXT( $1 );
              }
            } Catch_anonymous {
              error_count++;
            }
            $$ = expr;
          } else {
            FREE_TEXT( $1 );
            $$ = NULL;
          }
        } else {
          FREE_TEXT( $1 );
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | IDENTIFIER post_op_and_assign_op
    {
      if( parse_mode ) {
        expression* expr = NULL;
        if( ignore_mode == 0 ) {
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
            expr = db_create_expression( NULL, expr, $2, FALSE, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
            if( generate_varname == NULL ) {
              generate_varname = $1;
            } else {
              FREE_TEXT( $1 );
            }
          } Catch_anonymous {
            error_count++;
          }
          $$ = expr;
        } else {
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  ;

  /* Generate support */
generate_item_list_opt
  : generate_item_list
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

generate_item_list
  : generate_item_list generate_item
    {
      if( parse_mode ) {
        if( $1 == NULL ) {
          $$ = $2;
        } else if( $2 == NULL ) {
          $$ = $1;
        } else {
          db_gen_item_connect( $1, $2 );
          $$ = $1;
        }
      } else {
        $$ = NULL;
      }
    }
  | generate_item
    {
      $$ = $1;
    }
  ;

start_gen_block
  :
    {
      if( !parse_mode ) {
        generator_add_cov_to_work_code( " begin" );
        generator_flush_work_code;
      }
    }
  ;

end_gen_block
  :
    {
      if( !parse_mode ) {
        generator_add_to_hold_code( " end ", __FILE__, __LINE__ );
      }
    }
  ;

gen_if_body
  : generate_item K_else generate_item
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $$.gitem1 = $1;
          $$.gitem2 = $3;
        }
      } else {
        $$.gitem1 = NULL;
      }
    }
  | generate_item %prec less_than_K_else
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $$.gitem1 = $1;
          $$.gitem2 = NULL;
        }
      } else {
        $$.gitem1 = NULL;
      }
    }
  ;

  /* Similar to module_item but is more restrictive */
generate_item
  : module_item
    {
      if( parse_mode ) {
        $$ = db_get_curr_gen_block();
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_begin start_gen_block
    {
      if( parse_mode ) {
        generate_expr_mode++;
        if( ignore_mode == 0 ) {
          char* name = db_create_unnamed_scope();
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, name, @1.orig_fname, @1.incl_fname, @1.first_line, @1.first_column ) ) {
              ignore_mode++;
            } else {
              gen_item* gi = db_get_curr_gen_block();
              assert( gi != NULL );
              gitem_link_add( gi, &save_gi_head, &save_gi_tail );
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
          free_safe( name, (strlen( name ) + 1) );
        }
        generate_expr_mode--;
      } else {
        char         str[50];
        char*        back;
        char*        rest;
        unsigned int rv;
        func_unit*   funit = db_get_tfn_by_position( @1.first_line, @1.first_column );
        assert( funit != NULL );
        back = strdup_safe( funit->name );
        rest = strdup_safe( funit->name );
        scope_extract_back( funit->name, back, rest );
        rv = snprintf( str, 50, " : %s", back );
        assert( rv < 50 );
        generator_add_to_hold_code( str, __FILE__, __LINE__ );
        free_safe( back, (strlen( funit->name ) + 1) );
        free_safe( rest, (strlen( funit->name ) + 1) );
        generator_push_funit( funit );
        generator_insert_inst_id_reg( db_get_curr_funit() );
        generator_push_reg_insert();
      }
    }
    generate_item_list_opt end_gen_block K_end
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @6.first_line );
        } else {
          ignore_mode--;
        }
        db_gen_item_connect_true( save_gi_tail->gi, $4 );
        $$ = save_gi_tail->gi;
        gitem_link_remove( save_gi_tail->gi, &save_gi_head, &save_gi_tail );
      } else {
        generator_pop_funit();
        generator_pop_reg_insert();
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_begin start_gen_block ':' IDENTIFIER
    {
      if( parse_mode ) {
        generate_expr_mode++;
        if( ignore_mode == 0 ) {
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $4, @4.orig_fname, @4.incl_fname, @4.first_line, @4.first_column ) ) {
              ignore_mode++;
            } else {
              gen_item* gi = db_get_curr_gen_block();
              assert( gi != NULL );
              gitem_link_add( gi, &save_gi_head, &save_gi_tail );
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        }
        generate_expr_mode--;
      } else {
        func_unit* funit = db_get_tfn_by_position( @4.first_line, @4.first_column );
        assert( funit != NULL );
        generator_push_funit( funit );
        generator_insert_inst_id_reg( funit );
        generator_flush_work_code;
        generator_push_reg_insert();
      }
      FREE_TEXT( $4 );
    }
    generate_item_list_opt end_gen_block K_end
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @8.first_line );
        } else {
          ignore_mode--;
        }
        db_gen_item_connect_true( save_gi_tail->gi, $6 );
        $$ = save_gi_tail->gi;
        gitem_link_remove( save_gi_tail->gi, &save_gi_head, &save_gi_tail );
      } else {
        generator_pop_funit();
        generator_pop_reg_insert();
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_for
    {
      if( parse_mode ) {
        generate_expr_mode++;
      }
    }
    '(' generate_passign ';' static_expr ';' generate_passign ')'
    K_begin start_gen_block ':' IDENTIFIER
    {
      if( parse_mode ) {
        generate_for_mode++;
        if( ignore_mode == 0 ) {
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $13, @13.orig_fname, @13.incl_fname, @13.first_line, @13.first_column ) ) {
              ignore_mode++;
            } else {
              gen_item* gi = db_get_curr_gen_block();
              assert( gi != NULL );
              gi->varname = generate_varname;
              generate_varname = NULL;
              gitem_link_add( gi, &save_gi_head, &save_gi_tail );
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        }
        generate_expr_mode--;
      } else {
        func_unit* funit = db_get_tfn_by_position( @13.first_line, @13.first_column );
        assert( funit != NULL );
        generator_push_funit( funit );
        generator_flush_work_code;
        generator_insert_inst_id_reg( funit );
        generator_push_reg_insert();
      }
      FREE_TEXT( $13 );
    }
    generate_item_list_opt end_gen_block K_end
    {
      if( parse_mode ) {
        gen_item*   gi1;
        gen_item*   gi2;
        gen_item*   gi3;
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @17.first_line );
        } else {
          ignore_mode--;
        }
        generate_for_mode--;
        generate_expr_mode++;
        if( (ignore_mode == 0) && ($4 != NULL) && ($6 != NULL) && ($8 != NULL) && ($15 != NULL) ) {
          block_depth++;
          /* Create first statement */
          db_add_expression( $4 );
          gi1 = db_get_curr_gen_block();
          /* Create second statement and attach it to the first statement */
          db_add_expression( db_create_expr_from_static( $6, @6.first_line, @6.ppfline, @6.pplline, @6.first_column, (@6.last_column - 1)) );
          gi2 = db_get_curr_gen_block();
          db_gen_item_connect( gi1, gi2 );
          /* Add genvar to the new scope */
          /* Connect next_true of gi2 to the new scope */
          db_gen_item_connect_true( gi2, save_gi_tail->gi );
          /* Connect the generate block to the new scope */
          db_gen_item_connect_true( save_gi_tail->gi, $15 );
          /* Create third statement and attach it to the generate block */
          db_add_expression( $8 );
          gi3 = db_get_curr_gen_block();
          db_gen_item_connect_false( save_gi_tail->gi, gi3 );
          gitem_link_remove( save_gi_tail->gi, &save_gi_head, &save_gi_tail );
          gi2->suppl.part.conn_id = gi_conn_id;
          db_gen_item_connect( gi3, gi2 );
          block_depth--;
          $$ = gi1;
        } else {
          expression_dealloc( $4, FALSE );
          static_expr_dealloc( $6, TRUE );
          expression_dealloc( $8, TRUE );
          /* TBD - Deallocate generate block */
          $$ = NULL;
        }
        generate_expr_mode--;
      } else {
        generator_pop_funit();
        generator_pop_reg_insert();
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_if inc_gen_expr_mode '(' static_expr ')' dec_gen_expr_mode gen_if_body
    {
      if( parse_mode ) {
        expression* expr;
        gen_item*   gi1;
        if( (ignore_mode == 0) && ($4 != NULL) && ($7.gitem1 != NULL) ) {
          generate_expr_mode++;
          expr = db_create_expr_from_static( $4, @4.first_line, @4.ppfline, @4.pplline, @4.first_column, (@4.last_column - 1) );
          Try {
            expr = db_create_expression( expr, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          db_add_expression( expr );
          gi1 = db_get_curr_gen_block();
          db_gen_item_connect_true( gi1, $7.gitem1 );
          if( $7.gitem2 != NULL ) {
            db_gen_item_connect_false( gi1, $7.gitem2 );
          }
          generate_expr_mode--;
          $$ = gi1;
        } else {
          static_expr_dealloc( $4, TRUE );
          gen_item_dealloc( $7.gitem1, TRUE );
          gen_item_dealloc( $7.gitem2, TRUE );
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_case inc_gen_expr_mode '(' static_expr ')' inc_block_depth_only generate_case_items dec_block_depth_only dec_gen_expr_mode K_endcase
    { 
      if( parse_mode ) {
        expression* expr      = NULL;
        expression* c_expr;
        gen_item*   stmt      = NULL;
        gen_item*   last_stmt = NULL;
        case_gitem* c_stmt    = $7;
        case_gitem* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          generate_expr_mode++;
          c_expr = db_create_expr_from_static( $4, @4.first_line, @4.ppfline, @4.pplline, @4.first_column, (@4.last_column - 1) );
          while( c_stmt != NULL ) {
            Try {
              if( (c_stmt->expr != NULL) && (c_stmt->expr->op == EXP_OP_LIST) ) {
                parser_handle_generate_case_statement_list( c_stmt->expr, c_expr, c_stmt->gi, c_stmt->line, &last_stmt );
              } else {
                parser_handle_generate_case_statement( c_stmt->expr, c_expr, c_stmt->gi, c_stmt->line, &last_stmt );
              }
            } Catch_anonymous {
              error_count++;
              break;
            }
            tc_stmt   = c_stmt;
            c_stmt    = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_gitem ) );
          }
          if( last_stmt != NULL ) {
            last_stmt->elem.expr->suppl.part.owned = 1;
          }
          generate_expr_mode--;
          $$ = last_stmt;
        } else {
          static_expr_dealloc( $4, FALSE );
          while( c_stmt != NULL ) {
            expression_dealloc( c_stmt->expr, FALSE );
            tc_stmt = c_stmt;
            c_stmt  = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_gitem ) );
          }
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  ;

generate_case_item
  : expression_list ':' dec_gen_expr_mode generate_item inc_gen_expr_mode
    { PROFILE(PARSER_GENERATE_CASE_ITEM_A);
      if( parse_mode ) {
        case_gitem* cstmt;
        if( ignore_mode == 0 ) {
          cstmt = (case_gitem*)malloc_safe( sizeof( case_gitem ) );
          cstmt->prev = NULL;
          cstmt->expr = $1;
          cstmt->gi   = $4;
          cstmt->line = @1.first_line;
          $$ = cstmt;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | K_default ':' dec_gen_expr_mode generate_item inc_gen_expr_mode
    { PROFILE(PARSER_GENERATE_CASE_ITEM_B);
      if( parse_mode ) {
        case_gitem* cstmt;
        if( ignore_mode == 0 ) {
          cstmt = (case_gitem*)malloc_safe( sizeof( case_gitem ) );
          cstmt->prev = NULL;
          cstmt->expr = NULL;
          cstmt->gi   = $4;
          cstmt->line = @1.first_line;
          $$ = cstmt;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | K_default dec_gen_expr_mode generate_item inc_gen_expr_mode
    { PROFILE(PARSER_GENERATE_CASE_ITEM_C);
      if( parse_mode ) {
        case_gitem* cstmt;
        if( ignore_mode == 0 ) {
          cstmt = (case_gitem*)malloc_safe( sizeof( case_gitem ) );
          cstmt->prev = NULL;
          cstmt->expr = NULL;
          cstmt->gi   = $3;
          cstmt->line = @1.first_line;
          $$ = cstmt;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | error ignore_more dec_gen_expr_mode ':' generate_item ignore_less inc_gen_expr_mode
    {
      VLerror( "Illegal generate case expression" );
    }
  ;

generate_case_items
  : generate_case_items generate_case_item
    {
      if( parse_mode ) {
        case_gitem* list = $1;
        case_gitem* curr = $2;
        if( ignore_mode == 0 ) {
          curr->prev = list;
          $$ = curr;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | generate_case_item
    {
      $$ = $1;
    }
  ;

  /* This is the start of a module body */
module_item_list_opt
  : module_item_list
  |
  ;

module_item_list
  : module_item_list module_item
  | module_item
  ;

module_item
  : attribute_list_opt
    net_type signed_opt range_opt list_of_variables ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    net_type signed_opt range_opt net_decl_assigns ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    net_type drive_strength
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 == 1) ) {
          parser_implicitly_set_curr_range( 0, 0, TRUE );
        }
        curr_signed = FALSE;
      }
    }
    net_decl_assigns ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    TYPEDEF_IDENTIFIER
    {
      if( parse_mode ) {
        curr_mba     = FALSE;
        curr_signed  = $2->is_signed;
        curr_handled = $2->is_handled;
        parser_copy_range_to_curr_range( $2->prange, TRUE );
        parser_copy_range_to_curr_range( $2->urange, FALSE );
      }
    }
    register_variable_list ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt port_type signed_opt range_opt
    {
      if( parse_mode ) {
        curr_mba     = FALSE;
        curr_handled = TRUE;
        if( generate_top_mode > 0 ) {
          ignore_mode++;
          VLerror( "Port declaration not allowed within a generate block" );
        }
      }
    }
    list_of_variables ';'
    {
      if( parse_mode ) {
        if( generate_top_mode > 0 ) {
          ignore_mode--;
        }
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  /* Handles Verilog-2001 port of type:  input wire [m:l] <list>; */
  | attribute_list_opt port_type net_type signed_opt range_opt
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "ANSI-C port declaration used in a block that is specified to not allow Verilog-2001 syntax" );
          ignore_mode++;
        } else {
          curr_mba     = FALSE;
          curr_handled = TRUE;
          if( generate_top_mode > 0 ) {
            ignore_mode++;
            VLerror( "Port declaration not allowed within a generate block" );
          }
        }
      }
    }
    list_of_variables ';'
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) || (generate_top_mode > 0) ) {
          ignore_mode--;
        }
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt K_output var_type signed_opt range_opt
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "ANSI-C port declaration used in a block that is specified to not allow Verilog-2001 syntax" );
          ignore_mode++;
        } else {
          curr_mba      = FALSE;
          curr_handled  = TRUE;
          curr_sig_type = SSUPPL_TYPE_OUTPUT_REG;
          if( generate_top_mode > 0 ) {
            ignore_mode++;
            VLerror( "Port declaration not allowed within a generate block" );
          }
        }
      }
    }
    list_of_variables ';'
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) || (generate_top_mode > 0) ) {
          ignore_mode--;
        }
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt port_type signed_opt range_opt error ';'
    {
      if( parse_mode ) {
        if( generate_top_mode > 0 ) {
          VLerror( "Port declaration not allowed within a generate block" );
        } else {
          VLerror( "Invalid variable list in port declaration" );
        }
      }
    }
  | attribute_list_opt K_trireg charge_strength_opt range_opt delay3_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_signed   = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        if( generate_top_mode > 0 ) {
          ignore_mode++;
          VLerror( "Port declaration not allowed within a generate block" );
        }
      }
    }
    list_of_variables ';'
    {
      if( parse_mode ) {
        if( generate_top_mode > 0 ) {
          ignore_mode--;
        }
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt gatetype gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt gatetype delay3 gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $4 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt gatetype drive_strength gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $4 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt gatetype drive_strength delay3 gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $5 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt K_pullup gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt K_pulldown gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $6 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $6 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | block_item_decl
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt K_defparam defparam_assign_list ';'
    {
      if( parse_mode ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Defparam found but not used, file: %s, line: %u.  Please use -P option to specify",
                                    obf_file( @1.orig_fname ), @1.first_line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
      } else {
        generator_flush_all;
      }
    }
  | attribute_list_opt K_event
    {
      if( parse_mode ) {
        curr_mba      = TRUE;
        curr_signed   = FALSE;
        curr_sig_type = SSUPPL_TYPE_EVENT;
        curr_handled  = TRUE;
        parser_implicitly_set_curr_range( 0, 0, TRUE );
      } else {
        generator_handle_event_type( @2.ppfline, @2.first_column );
      }
    }
    list_of_variables ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  /* Handles instantiations of modules and user-defined primitives. */
  | attribute_list_opt IDENTIFIER parameter_value_opt gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link*    tmp  = $4;
        str_link*    curr = tmp;
        param_oride* po;
        if( ignore_mode == 0 ) {
          Try {
            while( curr != NULL ) {
              while( param_oride_head != NULL ){
                po               = param_oride_head;
                param_oride_head = po->next;
                db_add_override_param( curr->str, po->expr, po->name );
                if( po->name != NULL ) {
                  free_safe( po->name, (strlen( po->name ) + 1) );
                }
                free_safe( po, sizeof( param_oride ) );
              }
              (void)db_add_instance( curr->str, $2, FUNIT_MODULE, @2.ppfline, @2.first_column, curr->range );
              curr = curr->next;
            }
          } Catch_anonymous {
            error_count++;
          }
          str_link_delete_list( tmp );
          param_oride_head = NULL;
          param_oride_tail = NULL;
        }
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
      FREE_TEXT( $2 );
    }
  | attribute_list_opt
    K_assign drive_strength_opt { ignore_mode++; } delay3_opt { ignore_mode--; } assign_list ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    K_always statement
    {
      if( parse_mode ) {
        statement* stmt = $3;
        if( stmt != NULL ) {
          if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
            stmt->suppl.part.head = 1;
            if( info_suppl.part.inlined && info_suppl.part.scored_line ) {
              stmt->exp->exec_num = 1;
            }
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      } else {
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_always_comb
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "always_comb syntax found in block that is specified to not allow SystemVerilog syntax" );
          ignore_mode++;
        }
      } else {
        if( !info_suppl.part.verilator ) {
          generator_add_cov_to_work_code( " begin" );
          block_depth++;
        }
        generator_flush_work_code;
        if( !info_suppl.part.verilator ) {
          (void)generator_insert_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE );
          (void)generator_insert_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE );
        }
      }
    }
    statement
    {
      if( parse_mode ) {
        statement*  stmt;
        expression* slist;
        if( !parser_check_generation( GENERATION_SV ) ) {
          ignore_mode--;
        } else {
          if( $4 != NULL ) {
            if( (slist = db_create_sensitivity_list( $4 )) == NULL ) {
              VLerror( "Empty implicit event expression for the specified always_comb statement" );
            } else {
              Try {
                slist = db_create_expression( slist, NULL, EXP_OP_ALWAYS_COMB, lhs_mode, @2.first_line, @2.ppfline, @2.pplline, @2.first_column, (@2.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
              }
              stmt = db_create_statement( slist );
              if( !db_statement_connect( stmt, $4 ) ) {
                db_remove_statement( stmt );
                db_remove_statement( $4 );
              } else {
                if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
                  stmt->suppl.part.head = 1;
                  if( info_suppl.part.inlined && info_suppl.part.scored_line ) {
                    stmt->exp->exec_num = 1;
                  }
                  db_add_statement( stmt, stmt );
                } else {
                  db_remove_statement( stmt );
                }
              }
            }
          }
        }
      } else {
        if( !info_suppl.part.verilator ) {
          generator_add_cov_to_work_code( " end " );
          block_depth--;
        }
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_always_latch
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "always_latch syntax found in block that is specified to not allow SystemVerilog syntax" );
          ignore_mode++;
        }
      } else {
        generator_add_cov_to_work_code( " begin" );
        block_depth++;
        generator_flush_work_code;
        if( !info_suppl.part.verilator ) {
          (void)generator_insert_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE );
          (void)generator_insert_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE );
        }
      }
    }
    statement
    {
      if( parse_mode ) {
        statement*  stmt;
        expression* slist;
        if( !parser_check_generation( GENERATION_SV ) ) {
          ignore_mode--;
        } else {
          if( $4 != NULL ) {
            if( (slist = db_create_sensitivity_list( $4 )) == NULL ) {
              VLerror( "Empty implicit event expression for the specified always_latch statement" );
            } else {
              Try {
                slist = db_create_expression( slist, NULL, EXP_OP_ALWAYS_LATCH, lhs_mode, @2.first_line, @2.ppfline, @2.pplline, @2.first_column, (@2.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
              }
              stmt  = db_create_statement( slist );
              if( !db_statement_connect( stmt, $4 ) ) {
                db_remove_statement( stmt );
                db_remove_statement( $4 );
              } else {
                if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
                  stmt->suppl.part.head = 1;
                  if( info_suppl.part.inlined && info_suppl.part.scored_line ) {
                    stmt->exp->exec_num = 1;
                  }
                  db_add_statement( stmt, stmt );
                } else {
                  db_remove_statement( stmt );
                }
              }
            }
          }
        }
      } else {
        generator_add_cov_to_work_code( " end " );
        block_depth--;
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_always_ff
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "always_ff syntax found in block that is specified to not allow SystemVerilog syntax" );
          ignore_mode++;
        }
      } else {
        if( !info_suppl.part.verilator ) {
          generator_add_cov_to_work_code( " begin" );
          block_depth++;
        }
        generator_flush_work_code;
        if( !info_suppl.part.verilator ) {
          (void)generator_insert_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE );
          (void)generator_insert_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE );
        }
      }
    }
    statement
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( !parser_check_generation( GENERATION_SV ) ) {
          ignore_mode--;
        } else {
          if( stmt != NULL ) {
            if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
              stmt->suppl.part.head = 1;
              if( info_suppl.part.inlined && info_suppl.part.scored_line ) {
                stmt->exp->exec_num = 1;
              }
              db_add_statement( stmt, stmt );
            } else {
              db_remove_statement( stmt );
            }
          }
        }
      } else {
        if( !info_suppl.part.verilator ) {
          generator_add_cov_to_work_code( " end " );
          block_depth--;
        }
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_initial
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
    statement
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( stmt != NULL ) {
          if( (info_suppl.part.excl_init == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
            stmt->suppl.part.head = 1;
            if( info_suppl.part.inlined && info_suppl.part.scored_line ) {
              stmt->exp->exec_num = 1;
            }
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      } else {
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_final
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
    statement
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( stmt != NULL ) {
          if( (info_suppl.part.excl_final == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
            stmt->suppl.part.head  = 1;
            stmt->suppl.part.final = 1;
            if( info_suppl.part.inlined && info_suppl.part.scored_line ) {
              stmt->exp->exec_num = 1;
            }
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      } else {
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_task automatic_opt IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( generate_for_mode > 0 ) {
          VLerror( "Task definition not allowed within a generate loop" );
          ignore_mode++;
        }
        if( ignore_mode == 0 ) {
          Try {
            if( !db_add_function_task_namedblock( ($3 ? FUNIT_ATASK : FUNIT_TASK), $4, @4.orig_fname, @4.incl_fname, @4.first_line, @4.first_column ) ) {
              ignore_mode++;
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        }
        generate_top_mode--;
      }
      FREE_TEXT( $4 );
    }
    task_item_list_opt statement_or_null
    {
      if( parse_mode ) {
        statement* stmt = $8;
        if( ignore_mode == 0 ) {
          if( stmt == NULL ) {
            stmt = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @8.first_line, @8.ppfline, @8.pplline, @8.first_column, (@8.last_column - 1), NULL, in_static_expr ) );
          }
          stmt->suppl.part.head      = 1;
          stmt->suppl.part.is_called = 1;
          db_add_statement( stmt, stmt );
        }
      }
    }
    K_endtask
    {
      if( parse_mode ) {
        generate_top_mode++;
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @10.first_line );
        } else {
          ignore_mode--;
        }
      } else {
        generator_flush_work_code;
      }
    }
  | attribute_list_opt
    K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( generate_for_mode > 0 ) {
          VLerror( "Function definition not allowed within a generate loop" );
          ignore_mode++;
        }
        if( ignore_mode == 0 ) {
          Try {
            if( db_add_function_task_namedblock( ($3 ? FUNIT_AFUNCTION : FUNIT_FUNCTION), $6, @6.orig_fname, @6.incl_fname, @6.first_line, @6.first_column ) ) {
              generate_top_mode--;
              db_add_signal( $6, curr_sig_type, &curr_prange, NULL, curr_signed, FALSE, @6.first_line, @6.first_column, TRUE );
              generate_top_mode++;
            } else {
              ignore_mode++;
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        }
        generate_top_mode--;
      }
      FREE_TEXT( $6 );
    }
    function_item_list
    {
      if( !parse_mode ) {
        func_unit* funit;
        if( ((funit = db_get_tfn_by_position( @6.first_line, @6.first_column )) != NULL) && generator_is_static_function( funit ) ) {
          generator_push_funit( funit );
          generator_insert_inst_id_reg( funit );
          generator_push_reg_insert();
        }
        generator_add_to_hold_code( " begin", __FILE__, __LINE__ );
        block_depth++;
      }
    }
    statement
    {
      if( parse_mode ) {
        statement* stmt = $11;
        if( (ignore_mode == 0) && (stmt != NULL) ) {
          stmt->suppl.part.head      = 1;
          stmt->suppl.part.is_called = 1;
          db_add_statement( stmt, stmt );
        }
      } else {
        generator_add_to_hold_code( " end ", __FILE__, __LINE__ );
        block_depth--;
      }
    }
    K_endfunction
    {
      if( parse_mode ) {
        generate_top_mode++;
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @13.first_line );
        } else {
          ignore_mode--;
        }
      } else {
        func_unit* funit;
        if( ((funit = db_get_tfn_by_position( @6.first_line, @6.first_column )) != NULL) && generator_is_static_function( funit ) ) {
          generator_pop_reg_insert();
          generator_pop_funit();
        }
        generator_flush_work_code;
      }
    }
  | K_generate
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Generate syntax found in block that is specified to not allow Verilog-2001 syntax" );
          ignore_mode++;
        } else {
          if( generate_mode == 0 ) {
            generate_mode = 1;
            generate_top_mode = 1;
          } else {
            VLerror( "Found generate keyword inside of a generate block" );
          }
        }
      } else {
        generator_flush_work_code;
        generate_mode = 1;
      }
    }
    generate_item_list_opt
    K_endgenerate
    {
      if( parse_mode ) {
        generate_mode = 0;
        generate_top_mode = 0;
        if( !parser_check_generation( GENERATION_2001 ) ) {
          ignore_mode--;
        } else {
          db_add_gen_item_block( $3 );
        }
      } else {
        generate_mode = 0;
      }
    }
  | K_genvar
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Genvar syntax found in block that is specified to not allow Verilog-2001 syntax" );
          ignore_mode++;
        } else {
          curr_signed   = FALSE;
          curr_mba      = TRUE;
          curr_handled  = TRUE;
          curr_sig_type = SSUPPL_TYPE_GENVAR;
          parser_implicitly_set_curr_range( 31, 0, TRUE );
        }
      }
    }
    list_of_variables ';'
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          ignore_mode--;
        }
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    K_specify
    {
      if( parse_mode ) {
        if( generate_top_mode > 0 ) {
          VLerror( "Specify block not allowed within a generate block" );
        }
      }
    }
    ignore_more specify_item_list ignore_less K_endspecify
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    K_specify
    {
      if( parse_mode ) {
        if( generate_top_mode > 0 ) {
          VLerror( "Specify block not allowed within a generate block" );
        }
      }
    }
    K_endspecify
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    K_specify
    {
      if( parse_mode ) {
        if( generate_top_mode > 0 ) {
          VLerror( "Specify block not allowed within a generate block" );
        }
      }
    }
    error K_endspecify
  | error ';'
    {
      VLerror( "Invalid module item.  Did you forget an initial or always?" );
    }
  | attribute_list_opt
    K_assign error '=' { ignore_mode++; } expression { ignore_mode--; } ';'
    {
      VLerror( "Syntax error in left side of continuous assignment" );
    }
  | attribute_list_opt
    K_assign error ';'
    {
      VLerror( "Syntax error in continuous assignment" );
    }
  | attribute_list_opt
    K_function error K_endfunction
    {
      if( generate_for_mode > 0 ) {
        VLerror( "Function definition not allowed within generate block" );
      }
      VLerror( "Syntax error in function description" );
    }
  | attribute_list_opt
    enumeration list_of_names ';'
    {
      if( parse_mode ) {
        str_link* strl = $3;
        while( strl != NULL ) {
          db_add_signal( strl->str, SSUPPL_TYPE_DECL_REG, &curr_prange, NULL, curr_signed, FALSE, strl->suppl, strl->suppl2, TRUE );
          strl = strl->next;
        }
        str_link_delete_list( $3 );
      } else {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | attribute_list_opt
    typedef_decl
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  /* SystemVerilog assertion - we don't currently support these and I don't want to worry about how to parse them either */
  | attribute_list_opt
    IDENTIFIER ':' K_assert ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
      FREE_TEXT( $2 );
    }
  /* SystemVerilog property - we don't currently support these but crudely parse them */
  | attribute_list_opt
    K_property K_endproperty
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  /* SystemVerilog sequence - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_sequence K_endsequence
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  /* SystemVerilog program block - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_program K_endprogram
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
    }
  | KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')' ';'
    {
      if( !parse_mode ) {
        if( generate_mode ) {
          generator_flush_work_code;
        } else {
          generator_flush_all;
        }
      }
      FREE_TEXT( $3 );
      vector_dealloc( $5.vec );
      vector_dealloc( $7.vec );
    } 
  | KK_attribute '(' error ')' ';'
    {
      VLerror( "Syntax error in $attribute parameter list" );
    }
  ;

for_initialization
  : expression_assignment_list
    {
      $$ = $1;
    }
  ;

for_condition
  : expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          $1->suppl.part.for_cntrl = 2;
          $$ = db_create_statement( $1 );
        } else {
          $$ = NULL;
        }
      } else {
        $$ = generator_insert_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE );
      }
    }
  ;

  /* TBD - In the SystemVerilog BNF, there are more options available for this rule -- but we don't current support them */
data_type
  : integer_vector_type signed_opt range_opt
  | integer_atom_type signed_opt
  | struct_union '{' struct_union_member_list '}' range_opt
  | struct_union K_packed signed_opt '{' struct_union_member_list '}' range_opt
  | K_event
    {
      if( parse_mode ) {
        curr_mba      = TRUE;
        curr_signed   = FALSE;
        curr_sig_type = SSUPPL_TYPE_EVENT;
        curr_handled  = TRUE;
        parser_implicitly_set_curr_range( 0, 0, TRUE );
      } else {
        generator_handle_event_type( @1.ppfline, @1.first_column );
      }
    }
  ;

data_type_opt
  : data_type
    {
      $$ = 1;
    }
  |
    {
      $$ = 0;
    }
  ;

  /* TBD - Not sure what this should return at this point */
struct_union
  : K_struct
  | K_union
  | K_union K_tagged
  ;

struct_union_member_list
  : attribute_list_opt data_type_or_void list_of_variables ';'
  | struct_union_member_list ',' attribute_list_opt data_type_or_void list_of_variables ';'
  |
  ;

data_type_or_void
  : data_type
  | K_void 
  ;

integer_vector_type
  : K_bit
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
      }
    }
  | K_logic
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
      }
    }
  | K_reg
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
      }
    }
  ;

integer_atom_type
  : K_byte
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 7, 0, TRUE );
      }
    }
  | K_char
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 7, 0, TRUE );
      }
    }
  | K_shortint
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 15, 0, TRUE );
      }
    }
  | K_int
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
  | K_integer
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
  | K_longint
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 63, 0, TRUE );
      }
    }
  | K_time
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 63, 0, TRUE );
      }
    }
  ;

expression_assignment_list
  : data_type_opt IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        expression* tmp = NULL;
        statement*  stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          Try {
            if( ($1 == 1) && !parser_check_generation( GENERATION_SV ) ) {
              VLerror( "Variables declared in FOR initialization block that is specified to not allow SystemVerilog syntax" );
            } else if( ($1 == 1) || (db_find_signal( $2, TRUE ) == NULL) ) {
              db_add_signal( $2, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @2.first_line, @2.first_column, TRUE );
            }
            tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.ppfline, @2.pplline, @2.first_column, (@2.last_column - 1), $2, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          Try {
            tmp = db_create_expression( $4, tmp, EXP_OP_BASSIGN, FALSE, @2.first_line, @2.ppfline, @4.pplline, @2.first_column, (@4.last_column - 1), NULL, in_static_expr );
            tmp->suppl.part.for_cntrl = 1;
          } Catch_anonymous {
            error_count++;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        } else {
          expression_dealloc( $4, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_line_cov( @2.ppfline, ((@4.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@4.last_column - 1), TRUE );
        $$ = NULL;
      }
      FREE_TEXT( $2 );
    }
  | expression_assignment_list ',' data_type_opt IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        expression* tmp = NULL;
        statement*  stmt;
        if( (ignore_mode == 0) && ($1 != NULL) && ($6 != NULL) ) {
          Try {
            if( ($3 == 1) && !parser_check_generation( GENERATION_SV ) ) {
              VLerror( "Variables declared in FOR initialization block that is specified to not allow SystemVerilog syntax" );
            } else if( ($3 == 1) || (db_find_signal( $4, TRUE ) == NULL) ) {
              db_add_signal( $4, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @4.first_line, @4.first_column, TRUE );
            }
            tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @4.first_line, @4.ppfline, @4.pplline, @4.first_column, (@4.last_column - 1), $4, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          Try {
            tmp = db_create_expression( $6, tmp, EXP_OP_BASSIGN, FALSE, @4.first_line, @4.ppfline, @6.pplline, @4.first_column, (@6.last_column - 1), NULL, in_static_expr );
            tmp->suppl.part.for_cntrl = 1;
          } Catch_anonymous {
            error_count++;
          }
          stmt = db_create_statement( tmp );
          if( !db_statement_connect( $1, stmt ) ) {
            db_remove_statement( stmt );
          }
        } else {
          expression_dealloc( $6, FALSE );
        }
        $$ = $1;
      } else {
        (void)generator_insert_line_cov( @4.ppfline, ((@6.last_line - @4.first_line) + @4.ppfline), @4.first_column, (@6.last_column - 1), TRUE );
        $$ = NULL;
      }
      FREE_TEXT( $4 );
    }
  ;

passign
  : lpvalue '=' expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $3, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
            tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_ADD_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "+= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_ADD_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_SUB_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "-= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_SUB_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_MLT_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "*= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_MLT_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_DIV_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "/= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_DIV_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_MOD_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "%= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_MOD_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_AND_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "&= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_AND_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_OR_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "|= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_OR_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_XOR_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "^= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_XOR_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              expression_dealloc( $3, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_LS_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "<<= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_LS_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              expression_dealloc( $3, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_RS_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( ">>= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_RS_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              expression_dealloc( $3, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_ALS_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "<<<= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_ALS_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              expression_dealloc( $3, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_ARS_A expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( ">>>= operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( $3, $1, EXP_OP_ARS_A, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              expression_dealloc( $3, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_INC
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "Increment (++) operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( NULL, $1, EXP_OP_PINC, FALSE, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  | lpvalue K_DEC
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "Decrement (--) operation found in block that is specified to not allow SystemVerilog syntax" );
            expression_dealloc( $1, FALSE );
            $$ = NULL;
          } else {
            expression* tmp = NULL;
            Try {
              tmp = db_create_expression( NULL, $1, EXP_OP_PDEC, FALSE, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
              tmp->suppl.part.for_cntrl = (for_mode > 0) ? 3 : 0;
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              error_count++;
            }
            $$ = db_create_statement( tmp );
          }
        } else {
          expression_dealloc( $1, FALSE );
          $$ = NULL;
        }
      } else {
        if( for_mode == 0 ) {
          if( block_depth == 0 ) {
            generator_prepend_to_work_code( " begin " );
            generator_flush_work_code;
          }
          block_depth++;
          (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
          $$ = NULL;
        } else {
          $$ = generator_find_statement( @1.ppfline, @1.first_column );
        }
      }
    }
  ;

if_body
  : inc_block_depth statement_or_null dec_block_depth %prec less_than_K_else
    {
      $$.stmt1 = $2;
      $$.stmt2 = NULL;
      if( !parse_mode ) {
        generator_hold_last_token();
      }
    }
  | inc_block_depth statement_or_null dec_block_depth K_else inc_block_depth statement_or_null dec_block_depth
    {
      $$.stmt1 = $2;
      $$.stmt2 = $6;
    }
  ;

statement
  : K_assign { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
      }
      $$ = NULL;
    }
  | K_deassign { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
      }
      $$ = NULL;
    }
  | K_force { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
      }
      $$ = NULL;
    }
  | K_release { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
      }
      $$ = NULL;
    }
  | K_begin inc_block_depth begin_end_block dec_block_depth K_end
    { PROFILE(PARSER_STATEMENT_BEGIN_A);
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        char        back[4096];
        char        rest[4096];
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @5.first_line );
          if( $3 != NULL ) {
            scope_extract_back( $3->name, back, rest );
            exp  = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            exp->elem.funit      = $3;
            exp->suppl.part.type = ETYPE_FUNIT;
            exp->name            = strdup_safe( back );
            stmt = db_create_statement( exp );
            $$   = stmt;
          } else {
            $$ = NULL;
          }
        } else {
          if( ignore_mode > 0 ) {
            ignore_mode--;
          }
          if( ignore_mode == 0 ) {
            /* If there is no body to the begin..end block, replace the block with a NOOP */
            exp  = db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            stmt = db_create_statement( exp );
            $$   = stmt;
          }
        }
      } else {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_fork
    {
      if( !parse_mode ) {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
      }
    }
    inc_fork_depth fork_statement K_join
    { PROFILE(PARSER_STATEMENT_FORK_A);
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        char        back[4096];
        char        rest[4096];
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @5.first_line );
          if( $4 != NULL ) {
            scope_extract_back( $4->name, back, rest );
            exp  = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            exp->elem.funit      = $4;
            exp->suppl.part.type = ETYPE_FUNIT;
            exp->name            = strdup_safe( back );
            stmt = db_create_statement( exp );
            $$   = stmt;
          } else {
            ignore_mode--;
            $$ = NULL;
          }
        } else {
          ignore_mode--;
          $$ = NULL;
        }
      } else {
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_disable identifier ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          expression* exp = NULL;
          Try {
            exp = db_create_expression( NULL, NULL, EXP_OP_DISABLE, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@2.last_column - 1), $2, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          $$ = db_create_statement( exp );
        } else {
          $$ = NULL;
        } 
      } else {
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE );
        }
        if( (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {
          generator_prepend_to_work_code( " begin " );
          generator_add_cov_to_work_code( " end " );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
      FREE_TEXT( $2 );
    }
  | K_TRIGGER IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          expression* exp = NULL;
          Try {
            exp = db_create_expression( NULL, NULL, EXP_OP_TRIGGER, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@2.last_column - 1), $2, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          $$ = db_create_statement( exp );
        } else {
          $$ = NULL;
        }
      } else {
        generator_handle_event_trigger( $2, @1.ppfline, @1.first_column, ((@2.last_line - @1.first_line) + @1.ppfline), (@2.last_column - 1) );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE );
        }
        if( (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {
          generator_prepend_to_work_code( " begin " );
          generator_add_cov_to_work_code( " end " );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
      FREE_TEXT( $2 );
    }
  | K_forever
    {
      if( !parse_mode ) {
        generator_begin_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
      }
    }
    inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        if( $4 != NULL ) {
          expression* expr;
          statement*  stmt;
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_FOREVER, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          stmt = db_create_statement( expr );
          if( db_statement_connect( $4, $4 ) ) {
            db_connect_statement_false( stmt, $4 );
            $$ = stmt;
          } else {
            db_remove_statement( stmt );
            db_remove_statement( $4 );
            $$ = NULL;
          }
        }
      } else {
        generator_end_parallel_statement( @1.first_line, @1.first_column );
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_repeat '(' expression ')'
    {
      if( !parse_mode ) {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        generator_flush_work_code;
      }
    }
    inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) && ($7 != NULL) ) {
          vector*     vec  = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          expression* expr = NULL;
          statement*  stmt;
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            (void)vector_from_int( vec, 0x0 );
            assert( expr->value->value.ul == NULL );
            free_safe( expr->value, sizeof( vector ) );
            expr->value = vec;
          } Catch_anonymous {
            error_count++;
          }
          Try {
            expr = db_create_expression( $3, expr, EXP_OP_REPEAT, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, $7 );
          stmt->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for all statements connecting to stmt */
          assert( db_statement_connect( $7, stmt ) );
          $$ = stmt;
        } else {
          expression_dealloc( $3, FALSE );
          db_remove_statement( $7 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_case '(' expression ')'
    {
      if( !parse_mode ) {
        generator_insert_case_comb_cov( @4.ppfline, @4.first_column );
      }
    }
    case_body K_endcase
    {
      if( parse_mode ) {
        expression*     expr      = NULL;
        expression*     c_expr    = $4;
        statement*      stmt      = NULL;
        statement*      last_stmt = NULL;
        case_statement* c_stmt    = $7;
        case_statement* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          if( c_stmt == NULL ) {
            VLerror( "Illegal case expression" );
            expression_dealloc( c_expr, FALSE );
            $$ = NULL;
          } else {
            while( c_stmt != NULL ) {
              Try {
                if( (c_stmt->expr != NULL) && (c_stmt->expr->op == EXP_OP_LIST) ) {
                  parser_handle_case_statement_list( EXP_OP_CASE, c_stmt->expr, c_expr, c_stmt->stmt, c_stmt->line, c_stmt->ppfline, c_stmt->pplline, &last_stmt );
                } else {
                  parser_handle_case_statement( EXP_OP_CASE, c_stmt->expr, c_expr, c_stmt->stmt, c_stmt->line, c_stmt->ppfline, c_stmt->pplline, &last_stmt );
                }
              } Catch_anonymous {
                error_count++;
              }
              tc_stmt   = c_stmt;
              c_stmt    = c_stmt->prev;
              free_safe( tc_stmt, sizeof( case_statement ) );
            }
            if( last_stmt != NULL ) {
              last_stmt->exp->suppl.part.owned = 1;
            }
            $$ = last_stmt;
          }
        } else {
          expression_dealloc( $4, FALSE );
          while( c_stmt != NULL ) {
            expression_dealloc( c_stmt->expr, FALSE );
            db_remove_statement( c_stmt->stmt );
            tc_stmt = c_stmt;
            c_stmt  = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_statement ) );
          }
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_casex '(' expression ')'
    {
      if( !parse_mode ) {
        generator_insert_case_comb_cov( @4.ppfline, @4.first_column );
      }
    }
    case_body K_endcase
    {
      if( parse_mode ) {
        expression*     expr      = NULL;
        expression*     c_expr    = $4;
        statement*      stmt      = NULL;
        statement*      last_stmt = NULL;
        case_statement* c_stmt    = $7;
        case_statement* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          if( c_stmt == NULL ) {
            VLerror( "Illegal casex expression" );
            expression_dealloc( c_expr, FALSE );
            $$ = NULL;
          } else {
            while( c_stmt != NULL ) {
              Try {
                if( (c_stmt->expr != NULL) && (c_stmt->expr->op == EXP_OP_LIST) ) {
                  parser_handle_case_statement_list( EXP_OP_CASEX, c_stmt->expr, c_expr, c_stmt->stmt, c_stmt->line, c_stmt->ppfline, c_stmt->pplline, &last_stmt );
                } else {
                  parser_handle_case_statement( EXP_OP_CASEX, c_stmt->expr, c_expr, c_stmt->stmt, c_stmt->line, c_stmt->ppfline, c_stmt->pplline, &last_stmt );
                }
              } Catch_anonymous {
                error_count++;
              }
              tc_stmt   = c_stmt;
              c_stmt    = c_stmt->prev;
              free_safe( tc_stmt, sizeof( case_statement ) );
            }
            if( last_stmt != NULL ) {
              last_stmt->exp->suppl.part.owned = 1;
            }
            $$ = last_stmt;
          }
        } else {
          expression_dealloc( $4, FALSE );
          while( c_stmt != NULL ) {
            expression_dealloc( c_stmt->expr, FALSE );
            db_remove_statement( c_stmt->stmt );
            tc_stmt = c_stmt;
            c_stmt  = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_statement ) );
          }
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_casez '(' expression ')'
    {
      if( !parse_mode ) {
        generator_insert_case_comb_cov( @4.ppfline, @4.first_column );
      }
    }
    case_body K_endcase
    {
      if( parse_mode ) {
        expression*     expr      = NULL;
        expression*     c_expr    = $4;
        statement*      stmt      = NULL;
        statement*      last_stmt = NULL;
        case_statement* c_stmt    = $7;
        case_statement* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          if( c_stmt == NULL ) {
            VLerror( "Illegal casez expression" );
            expression_dealloc( c_expr, FALSE );
            $$ = NULL;
          } else {
            while( c_stmt != NULL ) {
              Try {
                if( (c_stmt->expr != NULL) && (c_stmt->expr->op == EXP_OP_LIST) ) {
                  parser_handle_case_statement_list( EXP_OP_CASEZ, c_stmt->expr, c_expr, c_stmt->stmt, c_stmt->line, c_stmt->ppfline, c_stmt->pplline, &last_stmt );
                } else {
                  parser_handle_case_statement( EXP_OP_CASEZ, c_stmt->expr, c_expr, c_stmt->stmt, c_stmt->line, c_stmt->ppfline, c_stmt->pplline, &last_stmt );
                }
              } Catch_anonymous {
                error_count++;
              }
              tc_stmt   = c_stmt;
              c_stmt    = c_stmt->prev;
              free_safe( tc_stmt, sizeof( case_statement ) );
            }
            if( last_stmt != NULL ) {
              last_stmt->exp->suppl.part.owned = 1;
            }
            $$ = last_stmt;
          }
        } else {
          expression_dealloc( $4, FALSE );
          while( c_stmt != NULL ) {
            expression_dealloc( c_stmt->expr, FALSE );
            db_remove_statement( c_stmt->stmt );
            tc_stmt = c_stmt;
            c_stmt  = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_statement ) );
          }
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_if '(' expression ')'
    {
      if( !parse_mode ) {
        (void)generator_insert_comb_cov( @2.ppfline, @2.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @2.ppfline, ((@5.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@5.last_column - 1), TRUE );
        }
      }
    }
    if_body
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          expression* tmp = NULL;
          Try {
            tmp = db_create_expression( $4, NULL, EXP_OP_IF, FALSE, @2.first_line, @2.ppfline, @5.pplline, @2.first_column, (@5.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          if( ($$ = db_create_statement( tmp )) != NULL ) {
            db_connect_statement_true( $$, $7.stmt1 );
            if( $7.stmt2 != NULL ) {
              db_connect_statement_false( $$, $7.stmt2 );
            }
          }
        } else {
          db_remove_statement( $7.stmt1 );
          db_remove_statement( $7.stmt2 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_if '(' error ')' { ignore_mode++; } if_statement_error { ignore_mode--; }
    {
      VLerror( "Illegal conditional if expression" );
      $$ = NULL;
    }
  | K_for inc_for_depth '(' for_initialization ';' for_condition ';' passign ')'
    {
      for_mode--;
      if( !parse_mode ) {
        generator_add_cov_to_work_code( " begin" );
        block_depth++;
        generator_flush_work_code;
        generator_insert_comb_cov_with_stmt( $6, FALSE, TRUE );
      }
    }
    statement dec_for_depth
    { PROFILE(PARSER_STATEMENT_FOR_A);
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        statement*  stmt1 = $4;
        statement*  stmt3 = $8;
        statement*  stmt4 = $11;
        char        back[4096];
        char        rest[4096];
        if( (ignore_mode == 0) && ($4 != NULL) && ($6 != NULL) && ($8 != NULL) && ($11 != NULL) ) {
          block_depth++;
          assert( db_statement_connect( stmt1, $6 ) );
          db_connect_statement_true( $6, stmt4 );
          assert( db_statement_connect( stmt4, stmt3 ) );
          $6->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for the stmt3 */
          assert( db_statement_connect( stmt3, $6 ) );
          block_depth--;
          stmt1 = db_parallelize_statement( stmt1 );
          stmt1->suppl.part.head      = 1;
          stmt1->suppl.part.is_called = 1;
          db_add_statement( stmt1, stmt1 );
        } else {
          db_remove_statement( stmt1 );
          db_remove_statement( $6 );
          db_remove_statement( stmt3 );
          db_remove_statement( stmt4 );
          stmt1 = NULL;
        }
        db_end_function_task_namedblock( @12.first_line );
        if( (stmt1 != NULL) && ($2 != NULL) ) {
          scope_extract_back( $2->name, back, rest );
          exp = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
          exp->elem.funit      = $2;
          exp->suppl.part.type = ETYPE_FUNIT;
          exp->name            = strdup_safe( back );
          stmt = db_create_statement( exp );
          $$ = stmt;
        } else {
          $$ = NULL;
        }
      } else {
        generator_insert_line_cov_with_stmt( $8, TRUE );
        generator_insert_comb_cov_with_stmt( $8, FALSE, TRUE );
        generator_add_cov_to_work_code( " end " );
        block_depth--;
        generator_flush_work_code;
        generator_insert_comb_cov_with_stmt( $6, FALSE, FALSE );
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_for inc_for_depth '(' for_initialization ';' for_condition ';' error ')' statement dec_for_depth
    {
      if( parse_mode ) {
        db_remove_statement( $4 );
        db_remove_statement( $6 );
        db_remove_statement( $10 );
        db_end_function_task_namedblock( @11.first_line );
      }
      $$ = NULL;
    }
  | K_for inc_for_depth '(' for_initialization ';' error ';' passign ')' statement dec_for_depth
    {
      if( parse_mode ) {
        db_remove_statement( $4 );
        db_remove_statement( $8 );
        db_remove_statement( $10 );
        db_end_function_task_namedblock( @11.first_line );
      }
      $$ = NULL;
    }
  | K_for inc_for_depth '(' error ')' statement dec_for_depth
    {
      if( parse_mode ) {
        db_remove_statement( $6 );
        db_end_function_task_namedblock( @7.first_line );
      }
      $$ = NULL;
    }
  | K_while '(' expression ')'
    {
      if( !parse_mode ) {
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, TRUE );
        generator_flush_work_code;
      }
    }
    inc_block_depth statement
    {
      if( !parse_mode ) {
        (void)generator_insert_comb_cov_from_stmt_stack();
        generator_flush_work_code;
      }
    }
    dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) && ($7 != NULL) ) {
          expression* expr = NULL;
          Try {
            expr = db_create_expression( $3, NULL, EXP_OP_WHILE, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          if( ($$ = db_create_statement( expr )) != NULL ) {
            db_connect_statement_true( $$, $7 );
            $$->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for all statements connecting to stmt */
            assert( db_statement_connect( $7, $$ ) );
          }
        } else {
          expression_dealloc( $3, FALSE );
          db_remove_statement( $7 );
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_while '(' error ')' inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        db_remove_statement( $6 );
      }
      $$ = NULL;
    }
  | K_do inc_block_depth statement dec_block_depth_flush K_while '(' expression ')' ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) && ($7 != NULL) ) {
          expression* expr = NULL;
          statement*  stmt;
          Try {
            expr = db_create_expression( $7, NULL, EXP_OP_WHILE, FALSE, @5.first_line, @5.ppfline, @8.pplline, @5.first_column, (@8.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          if( (stmt = db_create_statement( expr )) != NULL ) {
            assert( db_statement_connect( $3, stmt ) );
            db_connect_statement_true( stmt, $3 );
            stmt->suppl.part.stop_true = 1;  /* Set STOP bit for the TRUE path */
          }
          $$ = $3;
        } else {
          expression_dealloc( $7, FALSE );
          db_remove_statement( $3 );
          $$ = NULL;
        }
      } else {
        generator_prepend_to_work_code( " end " );
        (void)generator_insert_line_cov( @5.ppfline, ((@8.last_line - @5.first_line) + @5.ppfline), @5.first_column, (@8.last_column - 1), TRUE );
        (void)generator_insert_comb_cov( @5.ppfline, @5.first_column, FALSE, TRUE, FALSE );
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | delay1
    {
      if( !parse_mode ) {
        if( block_depth > 0 ) {
          generator_add_cov_to_work_code( ";" );
          (void)generator_insert_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE );
        }
        generator_add_cov_to_work_code( " begin " );
        block_depth++;
        generator_flush_work_code;
      }
    }
    statement_or_null
    {
      if( parse_mode ) {
        statement* stmt;
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          stmt = db_create_statement( $1 );
          if( $3 != NULL ) {
            if( !db_statement_connect( stmt, $3 ) ) {
              db_remove_statement( stmt );
              db_remove_statement( $3 );
              stmt = NULL;
            }
          }
          $$ = stmt;
        } else {
          db_remove_statement( $3 );
          $$ = NULL;
        }
      } else {
        generator_add_to_hold_code( " end ", __FILE__, __LINE__ );
        block_depth--;
        // generator_flush_work_code;
        $$ = NULL;
      }
    }
  | event_control
    {
      if( !parse_mode ) {
        if( block_depth > 0 ) {
          generator_add_cov_to_work_code( ";" );
          (void)generator_insert_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE );
        }
        generator_add_cov_to_work_code( " begin " );
        block_depth++;
        generator_flush_work_code;
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
        generator_flush_work_code;
      }
    }
    statement_or_null
    {
      if( parse_mode ) {
        statement* stmt;
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          stmt = db_create_statement( $1 );
          if( $3 != NULL ) {
            if( !db_statement_connect( stmt, $3 ) ) {
              db_remove_statement( stmt );
              db_remove_statement( $3 );
              stmt = NULL;
            }
          }
          $$ = stmt;
        } else {
          db_remove_statement( $3 );
          $$ = NULL;
        }
      } else {
        generator_add_to_hold_code( " end ", __FILE__, __LINE__ );
        block_depth--;
        $$ = NULL;
      }
    }
  | '@' '*'
    {
      if( !parse_mode ) {
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE );
        }
        generator_add_cov_to_work_code( " begin" );
        block_depth++;
        generator_flush_work_code;
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
        generator_flush_work_code;
      }
    }
    statement_or_null
    {
      if( parse_mode ) {
        expression* expr;
        statement*  stmt;
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Wildcard combinational logic sensitivity list found in block that is specified to not allow Verilog-2001 syntax" );
          db_remove_statement( $4 );
          $$ = NULL;
        } else {
          if( (ignore_mode == 0) && ($4 != NULL) ) {
            if( (expr = db_create_sensitivity_list( $4 )) == NULL ) {
              VLerror( "Empty implicit event expression for the specified statement" );
              db_remove_statement( $4 );
              $$ = NULL;
            } else {
              Try {
                expr = db_create_expression( expr, NULL, EXP_OP_SLIST, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr ); 
              } Catch_anonymous {
                error_count++;
              }
              stmt = db_create_statement( expr );
              if( !db_statement_connect( stmt, $4 ) ) {
                db_remove_statement( stmt );
                db_remove_statement( $4 );
                stmt = NULL;
              }
              $$ = stmt;
            }
          } else {
            db_remove_statement( $4 );
            $$ = NULL;
          }
        }
      } else {
        generator_add_cov_to_work_code( " end " );
        block_depth--;
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | passign ';'
    {
      if( parse_mode ) {
        $$ = $1;
      } else {
        block_depth--;
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE );
        } else {
          generator_add_cov_to_work_code( " end " );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | lpvalue K_LE expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $3, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@3.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@3.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | lpvalue '=' delay1 expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $3, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.ppfline, @4.pplline, @3.first_column, (@4.last_column - 1), NULL, in_static_expr );
            tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
            $$  = db_create_statement( tmp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE delay1 expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $4, FALSE );
            error_count++;
            $$ = NULL;
          }
          expression_dealloc( $3, FALSE );
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | lpvalue '=' event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $3, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.ppfline, @4.pplline, @3.first_column, (@4.last_column - 1), NULL, in_static_expr );
            tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $4, FALSE );
            error_count++;
            $$ = NULL;
          }
          expression_dealloc( $3, FALSE );
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | lpvalue '=' K_repeat '(' expression ')' event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($5 != NULL) && ($7 != NULL) && ($8 != NULL) ) {
          vector* vec = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          Try {
            expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            (void)vector_from_int( vec, 0x0 );
            assert( tmp->value->value.ul == NULL );
            free_safe( tmp->value, sizeof( vector ) );
            tmp->value = vec;
            tmp = db_create_expression( $5, tmp, EXP_OP_REPEAT, FALSE, @3.first_line, @3.ppfline, @6.pplline, @3.first_column, (@6.last_column - 1), NULL, in_static_expr );
            tmp = db_create_expression( $7, tmp, EXP_OP_RPT_DLY, FALSE, @3.first_line, @3.ppfline, @7.pplline, @3.first_column, (@7.last_column - 1), NULL, in_static_expr );
            tmp = db_create_expression( $8, tmp, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.ppfline, @8.pplline, @3.first_column, (@8.last_column - 1), NULL, in_static_expr );
            tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.ppfline, @8.pplline, @1.first_column, (@8.last_column - 1), NULL, in_static_expr );
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $5, FALSE );
          expression_dealloc( $7, FALSE );
          expression_dealloc( $8, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@8.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@8.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE K_repeat '(' expression ')' event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($8 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $8, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.ppfline, @8.pplline, @1.first_column, (@8.last_column - 1), NULL, in_static_expr );
            $$ = db_create_statement( tmp );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $8, FALSE );
            error_count++;
            $$ = NULL;
          }
          expression_dealloc( $5, FALSE );
          expression_dealloc( $7, FALSE );
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $5, FALSE );
          expression_dealloc( $7, FALSE );
          expression_dealloc( $8, FALSE );
          $$ = NULL;
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@8.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@8.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | K_wait '(' expression ')'
    {
      if( !parse_mode ) {
        statement* stmt = generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE );
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        generator_add_cov_to_work_code( " begin" );
        generator_flush_work_code;
        generator_insert_event_comb_cov( stmt->exp, stmt->funit, TRUE );
        generator_insert_comb_cov_with_stmt( stmt, TRUE, FALSE );
        generator_flush_work_code;
      }
      block_depth++;
    }
    statement_or_null dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          expression* exp = NULL;
          Try {
            exp = db_create_expression( $3, NULL, EXP_OP_WAIT, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          $$ = db_create_statement( exp );
          if( $6 != NULL ) {
            if( !db_statement_connect( $$, $6 ) ) {
              db_remove_statement( $$ );
              db_remove_statement( $6 );
              $$ = NULL;
            }
          }
        } else {
          db_remove_statement( $6 );
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_ignore '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
      $$ = NULL;
    }
  | S_allow '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, 0, 0, 0, 0, 0, NULL, in_static_expr ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_finish '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SFINISH, FALSE, 0, 0, 0, 0, 0, NULL, in_static_expr ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_stop '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SSTOP, FALSE, 0, 0, 0, 0, 0, NULL, in_static_expr ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_srandom '(' expression_systask_list ')' ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, $3, EXP_OP_SSRANDOM, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr ) );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_ignore ';'
    {
      $$ = NULL;
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
  | S_allow ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, 0, 0, 0, 0, 0, NULL, in_static_expr ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_finish ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SFINISH, FALSE, 0, 0, 0, 0, 0, NULL, in_static_expr ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  | S_stop ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SSTOP, FALSE, 0, 0, 0, 0, 0, NULL, in_static_expr ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | identifier '(' expression_port_list ')' ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            expression* exp = db_create_expression( NULL, $3, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), $1, in_static_expr );
            $$ = db_create_statement( exp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE );
        }
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE );
        generator_flush_work_code;
        $$ = NULL;
      }
      FREE_TEXT( $1 );
    }
  | identifier ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            expression* exp = db_create_expression( NULL, NULL, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
            $$ = db_create_statement( exp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        if( block_depth > 0 ) {
          (void)generator_insert_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE );
        }
        generator_flush_work_code;
        $$ = NULL;
      }
      FREE_TEXT( $1 );
    }
   /* Immediate SystemVerilog assertions are parsed but not performed -- we will not exclude a block that contains one */
  | K_assert ';'
    {
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        if( ignore_mode == 0 ) {
          exp  = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, 0, 0, NULL, in_static_expr );
          stmt = db_create_statement( exp );
          $$   = stmt;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
   /* Inline SystemVerilog assertion -- parsed, not performed and we will not exclude a block that contains one */
  | IDENTIFIER ':' K_assert ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            expression* exp = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, 0, 0, NULL, in_static_expr );
            $$ = db_create_statement( exp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
      FREE_TEXT( $1 );
    }
  | error ';'
    {
      VLerror( "Illegal statement" );
      $$ = NULL;
    }
  ;

fork_statement
  : begin_end_id
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $1, @1.orig_fname, @1.incl_fname, @1.first_line, @1.first_column ) ) {
              ignore_mode++;
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        } else {
          ignore_mode++;
        }
      } else {
        func_unit* funit;
        if( (funit = db_get_tfn_by_position( @1.first_line, @1.first_column )) != NULL ) {
          generator_hold_last_token();
          if( $1 == NULL ) {
            char         str[50];
            char*        back;
            char*        rest;
            unsigned int rv;
            back = strdup_safe( funit->name );
            rest = strdup_safe( funit->name );
            scope_extract_back( funit->name, back, rest );
            rv = snprintf( str, 50, " : %s", back );
            assert( rv < 50 );
            generator_add_cov_to_work_code( str );
            generator_flush_work_code;
            free_safe( back, (strlen( funit->name ) + 1) );
            free_safe( rest, (strlen( funit->name ) + 1) );
          } else {
            FREE_TEXT( $1 );
          }
          generator_insert_inst_id_reg( funit );
          generator_flush_held_token;
          // generator_flush_work_code;
        }
      }
    }
    block_item_decls_opt statement_list dec_fork_depth
    {
      if( parse_mode ) {
        if( $3 && db_is_unnamed_scope( $1 ) && !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "Net/variables declared in unnamed fork...join block that is specified to not allow SystemVerilog syntax" );
          ignore_mode++;
        }
        if( ignore_mode == 0 ) {
          if( $4 != NULL ) {
            expression* expr = NULL;
            statement*  stmt;
            Try {
              expr = db_create_expression( NULL, NULL, EXP_OP_JOIN, FALSE, @4.first_line, @4.ppfline, @4.pplline, @4.first_column, (@4.last_column - 1), NULL, in_static_expr );
            } Catch_anonymous {
              error_count++;
            }
            if( ((stmt = db_create_statement( expr )) != NULL) && db_statement_connect( $4, stmt ) ) {
              stmt = $4;
              stmt->suppl.part.head      = 1;
              stmt->suppl.part.is_called = 1;
              db_add_statement( stmt, stmt );
              $$ = db_get_curr_funit();
            } else {
              db_remove_statement( $4 );
              db_remove_statement( stmt );
              $$ = NULL;
            }
          } else {
            $$ = NULL;
          }
          FREE_TEXT( $1 );
        } else {
          if( $3 && db_is_unnamed_scope( $1 ) ) {
            ignore_mode--;
          }
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  |
    {
      $$ = NULL;
    }
  ;

begin_end_block
  : begin_end_id
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $1, @1.orig_fname, @1.incl_fname, @1.first_line, @1.first_column ) ) {
              ignore_mode++;
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        } else {
          ignore_mode++;
        }
        generate_top_mode--;
      } else {
        func_unit* funit;
        if( (funit = db_get_tfn_by_position( @1.first_line, @1.first_column )) != NULL ) {
          generator_hold_last_token();
          if( $1 == NULL ) {
            char         str[50];
            char*        back;
            char*        rest;
            unsigned int rv;
            back = strdup_safe( funit->name );
            rest = strdup_safe( funit->name );
            scope_extract_back( funit->name, back, rest );
            rv = snprintf( str, 50, " : %s", back );
            assert( rv < 50 );
            generator_add_to_hold_code( str, __FILE__, __LINE__ );
            free_safe( back, (strlen( funit->name ) + 1) );
            free_safe( rest, (strlen( funit->name ) + 1) );
          } else {
            FREE_TEXT( $1 );
          }
          generator_insert_inst_id_reg( funit );
          generator_flush_work_code;
        }
      }
    }
    block_item_decls_opt statement_list
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( $3 && db_is_unnamed_scope( $1 ) && !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "Net/variables declared in unnamed begin...end block that is specified to not allow SystemVerilog syntax" );
          ignore_mode++;
        }
        if( ignore_mode == 0 ) {
          if( stmt != NULL ) {
            stmt->suppl.part.head      = 1;
            stmt->suppl.part.is_called = 1;
            db_add_statement( stmt, stmt );
            $$ = db_get_curr_funit();
          } else {
            $$ = NULL;
          }
        } else {
          if( $3 && db_is_unnamed_scope( $1 ) ) {
            ignore_mode--;
          }
          $$ = NULL;
        }
        generate_top_mode++;
      } else {
        $$ = NULL;
      }
      FREE_TEXT( $1 );
    }
  | begin_end_id
    {
      if( parse_mode ) {
        ignore_mode++;
        $$ = NULL;
      } else {
        $$ = NULL;
      }
      FREE_TEXT( $1 );
    }
  ;

if_statement_error
  : statement_or_null %prec less_than_K_else
    {
      $$ = NULL;
    }
  | statement_or_null K_else statement_or_null
    {
      $$ = NULL;
    }
  ;

statement_list
  : statement_list statement
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $1 == NULL ) {
            db_remove_statement( $2 );
            $$ = NULL;
          } else {
            if( $2 != NULL ) {
              if( !db_statement_connect( $1, $2 ) ) {
                db_remove_statement( $2 );
              }
              $$ = $1;
            } else {
              db_remove_statement( $1 );
              $$ = NULL;
            }
          }
        }
      } else {
        $$ = NULL;
      }
    }
  | statement
    {
      if( parse_mode ) {
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  /* This rule is not in the LRM but seems to be supported by several simulators */
  | statement_list ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $1 == NULL ) {
            $$ = NULL;
          } else {
            statement* stmt = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @2.first_line, @2.ppfline, @2.pplline, @2.first_column, (@2.last_column - 1), NULL, in_static_expr ) );
            if( !db_statement_connect( $1, stmt ) ) {
              db_remove_statement( stmt );
            }
            $$ = $1;
          }
        }
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  /* This rule is not in the LRM but seems to be supported by several simulators */
  | ';'
    {
      if( parse_mode ) {
        $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr ) );
      } else {
        generator_flush_work_code;
        $$ = NULL;
      }
    }
  ;

statement_or_null
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
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$  = NULL;
        }
        FREE_TEXT( $1 );
      } else {
        $$ = NULL;
      }
    }
  | identifier start_lhs index_expr end_lhs
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          db_bind_expr_tree( $3, $1 );
          $3->line           = @1.first_line;
          $3->col.part.first = @1.first_column;
          FREE_TEXT( $1 );
          $$ = $3;
        } else {
          FREE_TEXT( $1 );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '{' start_lhs expression_list end_lhs '}'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$  = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
  : identifier
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$  = NULL;
        }
        FREE_TEXT( $1 );
      } else {
        $$ = NULL;
      }
    }
  | identifier start_lhs index_expr end_lhs
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          db_bind_expr_tree( $3, $1 );
          $3->line           = @1.first_line;
          $3->col.part.first = @1.first_column;
          FREE_TEXT( $1 );
          $$ = $3;
        } else {
          FREE_TEXT( $1 );
          expression_dealloc( $3, FALSE );
          $$  = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '{' start_lhs expression_list end_lhs '}'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$  = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

block_item_decls_opt
  : block_item_decls { $$ = TRUE; }
  | { $$ = FALSE; }
  ;

block_item_decls
  : block_item_decl
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
  | block_item_decls block_item_decl
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
  ;

  /* The block_item_decl is used in function definitions, task
     definitions, module definitions and named blocks. Wherever a new
     scope is entered, the source may declare new registers and
     integers. This rule matches those declarations. The containing
     rule has presumably set up the scope. */
block_item_decl
  : attribute_list_opt K_reg signed_opt range_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_bit signed_opt range_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_logic signed_opt range_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_char unsigned_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 7, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_byte unsigned_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 7, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_shortint unsigned_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 15, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_integer unsigned_opt
    {
      if( parse_mode ) {
        curr_signed   = TRUE;
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_int unsigned_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_longint unsigned_opt
    {
      if( parse_mode ) {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 63, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_time
    {
      if( parse_mode ) {
        curr_signed   = FALSE;
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REG;
        parser_implicitly_set_curr_range( 63, 0, TRUE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt TYPEDEF_IDENTIFIER ';'
    {
      if( parse_mode ) {
        curr_signed  = $2->is_signed;
        curr_mba     = TRUE;
        curr_handled = $2->is_handled;
        parser_copy_range_to_curr_range( $2->prange, TRUE );
        parser_copy_range_to_curr_range( $2->urange, FALSE );
      }
    }
    register_variable_list ';'
  | attribute_list_opt K_real
    {
      if( parse_mode ) {
        int real_size = sizeof( double ) * 8;
        curr_signed   = TRUE;
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REAL;
        parser_implicitly_set_curr_range( (real_size - 1), 0, TRUE );
      }
    }
    list_of_variables ';'
  | attribute_list_opt K_realtime
    {
      if( parse_mode ) {
        int real_size = sizeof( double ) * 8;
        curr_signed   = TRUE;
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_REAL;
        parser_implicitly_set_curr_range( (real_size - 1), 0, TRUE );
      }
    }
    list_of_variables ';'
  | attribute_list_opt K_shortreal
    {
      if( parse_mode ) {
        int real_size = sizeof( float ) * 8;
        curr_signed   = TRUE;
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_DECL_SREAL;
        parser_implicitly_set_curr_range( (real_size - 1), 0, TRUE );
      }
    }
  | attribute_list_opt K_parameter parameter_assign_decl ';'
  | attribute_list_opt K_localparam
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Localparam syntax found in block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      }
    }
    localparam_assign_decl ';'
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        ignore_mode--;
      }
    }
  | attribute_list_opt K_reg error ';'
    {
      VLerror( "Syntax error in reg variable list" );
    }
  | attribute_list_opt K_bit error ';'
    {
      VLerror( "Syntax error in bit variable list" );
    }
  | attribute_list_opt K_byte error ';'
    {
      VLerror( "Syntax error in byte variable list" );
    }
  | attribute_list_opt K_logic error ';'
    {
      VLerror( "Syntax error in logic variable list" );
    }
  | attribute_list_opt K_char error ';'
    {
      VLerror( "Syntax error in char variable list" );
    }
  | attribute_list_opt K_shortint error ';'
    {
      VLerror( "Syntax error in shortint variable list" );
    }
  | attribute_list_opt K_integer error ';'
    {
      VLerror( "Syntax error in integer variable list" );
    }
  | attribute_list_opt K_int error ';'
    {
      VLerror( "Syntax error in int variable list" );
    }
  | attribute_list_opt K_longint error ';'
    {
      VLerror( "Syntax error in longint variable list" );
    }
  | attribute_list_opt K_time error ';'
    {
      VLerror( "Syntax error in time variable list" );
    }
  | attribute_list_opt K_real error ';'
    {
      VLerror( "Syntax error in real variable list" );
    }
  | attribute_list_opt K_realtime error ';'
    {
      VLerror( "Syntax error in realtime variable list" );
    }
  | attribute_list_opt K_parameter error ';'
    {
      VLerror( "Syntax error in parameter variable list" );
    }
  | attribute_list_opt K_localparam
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Localparam syntax found in block that is specified to not allow Verilog-2001 syntax" );
      }
    }
    error ';'
    {
      if( parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Syntax error in localparam list" );
      }
    }
  ;	

case_item
  : expression_list ':'
    {
      if( !parse_mode ) {
        generator_add_cov_to_work_code( " begin" );
        block_depth++;
        generator_flush_work_code;
      }
    }
    statement_or_null
    { PROFILE(PARSER_CASE_ITEM_A);
      if( parse_mode ) {
        case_statement* cstmt;
        if( ignore_mode == 0 ) {
          cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
          cstmt->prev    = NULL;
          cstmt->expr    = $1;
          cstmt->stmt    = $4;
          cstmt->line    = @1.first_line;
          cstmt->ppfline = @1.ppfline;
          cstmt->pplline = @1.pplline;
          cstmt->fcol    = @1.first_column;
          cstmt->lcol    = @1.last_column;
          $$ = cstmt;
        } else {
          $$ = NULL;
        }
      } else {
        generator_add_cov_to_work_code( " end " );
        block_depth--;
        $$ = NULL;
      }
    }
  | K_default ':'
    {
      if( !parse_mode ) {
        generator_add_cov_to_work_code( " begin" );
        block_depth++;
        generator_flush_work_code;
      }
    }
    statement_or_null
    { PROFILE(PARSER_CASE_ITEM_B);
      if( parse_mode ) {
        case_statement* cstmt;
        if( ignore_mode == 0 ) {
          cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
          cstmt->prev    = NULL;
          cstmt->expr    = NULL;
          cstmt->stmt    = $4;
          cstmt->line    = @1.first_line;
          cstmt->ppfline = @1.ppfline;
          cstmt->pplline = @1.pplline;
          cstmt->fcol    = @1.first_column;
          cstmt->lcol    = @1.last_column;
          $$ = cstmt;
        } else {
          $$ = NULL;
        }
      } else {
        generator_add_cov_to_work_code( " end " );
        block_depth--;
        $$ = NULL;
      }
    }
  | K_default 
    {
      if( !parse_mode ) {
        generator_add_cov_to_work_code( " begin" );
        block_depth++;
        generator_flush_work_code;
      }
    }
    statement_or_null
    { PROFILE(PARSER_CASE_ITEM_C);
      if( parse_mode ) {
        case_statement* cstmt;
        if( ignore_mode == 0 ) {
          cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
          cstmt->prev    = NULL;
          cstmt->expr    = NULL;
          cstmt->stmt    = $3;
          cstmt->line    = @1.first_line;
          cstmt->ppfline = @1.ppfline;
          cstmt->pplline = @1.pplline;
          cstmt->fcol    = @1.first_column;
          cstmt->lcol    = @1.last_column;
          $$ = cstmt;
        } else {
          $$ = NULL;
        }
      } else {
        generator_add_cov_to_work_code( " end " );
        block_depth--;
        $$ = NULL;
      }
    }
  | error { ignore_mode++; } ':' statement_or_null { ignore_mode--; }
    {
      VLerror( "Illegal case expression" );
    }
  ;

case_items
  : case_items case_item
    {
      if( parse_mode ) {
        case_statement* list = $1;
        case_statement* curr = $2;
        if( ignore_mode == 0 ) {
          curr->prev = list;
          $$ = curr;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | case_item
    {
      if( parse_mode ) {
        $$ = $1;
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  ;

case_body
  : inc_block_depth_only case_items dec_block_depth_only
    {
      $$ = $2;
    }
  | inc_block_depth_only error dec_block_depth_only
    {
      $$ = NULL;
    }
  ;

delay1
  : '#' delay_value_simple
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $2, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ')'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

delay3
  : '#' delay_value_simple
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $2, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ')'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.ppfline, @4.pplline, @1.first_column, (@4.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ',' delay_value ')'
    {
      if( parse_mode ) {
        expression_dealloc( $5, FALSE );
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.ppfline, @6.pplline, @1.first_column, (@6.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ',' delay_value ',' delay_value ')'
    {
      if( parse_mode ) {
        expression_dealloc( $3, FALSE );
        expression_dealloc( $7, FALSE );
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_expression( $5, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.ppfline, @8.pplline, @1.first_column, (@8.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $5, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $5, FALSE );
          $$ = NULL;
        }
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
    { PROFILE(PARSER_DELAY_VALUE_A);
      if( parse_mode ) {
        static_expr* se = $1;
        if( (ignore_mode == 0) && (se != NULL) ) {
          if( se->exp == NULL ) {
            Try {
              $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
              vector_dealloc( $$->value );
              $$->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
              (void)vector_from_int( $$->value, se->num );
            } Catch_anonymous {
              error_count++;
              $$ = NULL;
            }
            static_expr_dealloc( se, TRUE );
          } else {
            $$ = se->exp;
            static_expr_dealloc( se, FALSE );
          }
        } else {
          $$ = NULL;
          static_expr_dealloc( se, TRUE );
        }
      } else {
        $$ = NULL;
      }
      PROFILE_END;
    }
  | static_expr ':' static_expr ':' static_expr
    { PROFILE(PARSER_DELAY_VALUE_B);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          static_expr* se = NULL;
          if( delay_expr_type == DELAY_EXPR_DEFAULT ) {
            unsigned int rv = snprintf( user_msg,
                                        USER_MSG_LENGTH,
                                        "Delay expression type for min:typ:max not specified, using default of 'typ', file %s, line %u",
                                        obf_file( @1.orig_fname ),
                                        @1.first_line );
            assert( rv < USER_MSG_LENGTH );
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
              Try {
                $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
                vector_dealloc( $$->value );
                $$->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
                (void)vector_from_int( $$->value, se->num );
              } Catch_anonymous {
                error_count++;
                $$ = NULL;
              }
              static_expr_dealloc( se, TRUE );
            } else {
              $$ = se->exp;
              static_expr_dealloc( se, FALSE );
            }
          } else {
            $$ = NULL;
            static_expr_dealloc( se, TRUE );
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      PROFILE_END;
    }
  ;

delay_value_simple
  : DEC_NUMBER
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            int   base;
            char* num = $1;
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            assert( $$->value->value.ul == NULL );
            free_safe( $$->value, sizeof( vector ) );
            vector_from_string( &num, FALSE, &($$->value), &base );
            $$->suppl.part.base = base;
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
        FREE_TEXT( $1 );
      } else {
        $$ = NULL;
      }
    }
  | REALTIME
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
            assert( $$->value->value.r64 == NULL );
            free_safe( $$->value, sizeof( vector ) );
            $$->value = $1;
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          vector_dealloc( $1 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | IDENTIFIER
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
              $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      FREE_TEXT( $1 );
    }
  ;

assign_list
  : assign_list ',' assign
  | assign
  ;

assign
  : lavalue '=' expression
    {
      if( parse_mode ) {
        if( ($1 != NULL) && ($3 != NULL) && (info_suppl.part.excl_assign == 0) ) {
          expression* tmp = NULL;
          statement*  stmt;
          Try {
            tmp = db_create_expression( $3, $1, EXP_OP_ASSIGN, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
          if( (stmt = db_create_statement( tmp )) != NULL ) {
            stmt->suppl.part.head       = 1;
            stmt->suppl.part.stop_true  = 1;
            stmt->suppl.part.stop_false = 1;
            stmt->suppl.part.cont       = 1;
            /* Statement will be looped back to itself */
            db_connect_statement_true( stmt, stmt );
            db_connect_statement_false( stmt, stmt );
            db_add_statement( stmt, stmt );
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, TRUE, TRUE, FALSE );
      }
    }
  ;

range_opt
  : range
  |
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          parser_implicitly_set_curr_range( 0, 0, curr_packed );
        }
      }
    }
  ;

range
  : '[' static_expr ':' static_expr ']'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          parser_explicitly_set_curr_range( $2, $4, curr_packed );
        }
      }
    }
  | range '[' static_expr ':' static_expr ']'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          parser_explicitly_set_curr_range( $3, $5, curr_packed );
        }
      }
    }
  ;

range_or_type_opt
  : range      
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_sig_type = SSUPPL_TYPE_IMPLICIT;
        }
      }
    }
  | K_integer
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_sig_type = SSUPPL_TYPE_IMPLICIT;
          parser_implicitly_set_curr_range( 31, 0, curr_packed );
        }
      }
    }
  | K_real
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_sig_type = SSUPPL_TYPE_IMPLICIT_REAL;
          parser_implicitly_set_curr_range( 63, 0, curr_packed );
        }
      }
    }
  | K_realtime
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_sig_type = SSUPPL_TYPE_IMPLICIT_REAL;
          parser_implicitly_set_curr_range( 63, 0, curr_packed );
        }
      }
    }
  | K_time
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_sig_type = SSUPPL_TYPE_IMPLICIT;
          parser_implicitly_set_curr_range( 63, 0, curr_packed );
        }
      }
    }
  |
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_sig_type = SSUPPL_TYPE_IMPLICIT;
          parser_implicitly_set_curr_range( 0, 0, curr_packed );
        }
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
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_add_signal( $1, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @1.first_line, @1.first_column, TRUE );
        }
      }
      FREE_TEXT( $1 );
    }
  | IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( !parser_check_generation( GENERATION_2001 ) ) {
            VLerror( "Register declaration with initialization found in block that is specified to not allow Verilog-2001 syntax" );
            expression_dealloc( $3, FALSE );
          } else {
            db_add_signal( $1, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @1.first_line, @1.first_column, TRUE );
            if( $3 != NULL ) {
              expression* exp = NULL;
              statement*  stmt;
              Try {
                exp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
                exp = db_create_expression( $3, exp, EXP_OP_RASSIGN, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
              }
              if( (stmt = db_create_statement( exp )) != NULL ) {
                stmt->suppl.part.head       = 1;
                stmt->suppl.part.stop_true  = 1;
                stmt->suppl.part.stop_false = 1;
                db_add_statement( stmt, stmt );
              }
            }
          }
        }
      }
      FREE_TEXT( $1 );
    }
  | IDENTIFIER { curr_packed = FALSE; } range
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          /* Unpacked dimensions are now handled */
          curr_packed = TRUE;
          db_add_signal( $1, SSUPPL_TYPE_MEM, &curr_prange, &curr_urange, curr_signed, TRUE, @1.first_line, @1.first_column, TRUE );
        }
      }
      FREE_TEXT( $1 );
    }
  ;

register_variable_list
  : register_variable
  | register_variable_list ',' register_variable
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
  | port_type range_opt
    {
      curr_signed  = FALSE;
      curr_mba     = FALSE;
      curr_handled = TRUE;
    }
    list_of_variables ';'
  ;

automatic_opt
  : K_automatic { $$ = TRUE; }
  |             { $$ = FALSE; }
  ;

signed_opt
  : K_signed   { curr_signed = TRUE;  }
  | K_unsigned { curr_signed = FALSE; }
  |            { curr_signed = FALSE; }
  ;

unsigned_opt
  : K_unsigned { curr_signed = FALSE; }
  | K_signed   { curr_signed = TRUE;  }
  |            { curr_signed = TRUE;  }
  ;

  /*
   The net_type_sign_range_opt is only used in port lists so don't set the curr_sig_type field
   as this will already be filled in by the port_type rule.
  */
net_type_sign_range_opt
  : K_wire signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_tri signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_tri1 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_supply0 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_wand signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_triand signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_tri0 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_supply1 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_wor signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_trior signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_logic signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | TYPEDEF_IDENTIFIER
    {
      if( parse_mode ) {
        curr_mba     = FALSE;
        curr_signed  = $1->is_signed;
        curr_handled = $1->is_handled;
        parser_copy_range_to_curr_range( $1->prange, TRUE );
        parser_copy_range_to_curr_range( $1->urange, FALSE );
      }
      $$ = 1;
    }
  | signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 0;
    }
  ;

net_type
  : K_wire
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_tri
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_tri1
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_supply0
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_wand
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_triand
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_tri0
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_supply1
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_wor
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_trior
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECL_NET;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  ;

var_type
  : K_reg     { $$ = 1; }
  ;

net_decl_assigns
  : net_decl_assigns ',' net_decl_assign
  | net_decl_assign
  ;

net_decl_assign
  : IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          db_add_signal( $1, SSUPPL_TYPE_DECL_NET, &curr_prange, NULL, curr_signed, FALSE, @1.first_line, @1.first_column, TRUE );
          if( (info_suppl.part.excl_assign == 0) && ($3 != NULL) ) {
            expression* tmp = NULL;
            statement*  stmt;
            Try {
              tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), $1, in_static_expr );
              tmp = db_create_expression( $3, tmp, EXP_OP_DASSIGN, FALSE, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
            } Catch_anonymous {
              error_count++;
            }
            if( (stmt = db_create_statement( tmp )) != NULL ) {
              stmt->suppl.part.head       = 1;
              stmt->suppl.part.stop_true  = 1;
              stmt->suppl.part.stop_false = 1;
              stmt->suppl.part.cont       = 1;
              /* Statement will be looped back to itself */
              db_connect_statement_true( stmt, stmt );
              db_connect_statement_false( stmt, stmt );
              db_add_statement( stmt, stmt );
            }
          } else {
            expression_dealloc( $3, FALSE );
          }
        } else {
          expression_dealloc( $3, FALSE );
        }
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, TRUE, TRUE, FALSE );
      }
      FREE_TEXT( $1 );
    }
  | delay1 IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          db_add_signal( $2, SSUPPL_TYPE_DECL_NET, &curr_prange, NULL, FALSE, FALSE, @2.first_line, @2.first_column, TRUE );
          if( (info_suppl.part.excl_assign == 0) && ($4 != NULL) ) {
            expression* tmp = NULL;
            statement*  stmt;
            Try {
              tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.ppfline, @2.pplline, @2.first_column, (@2.last_column - 1), $2, in_static_expr );
              tmp = db_create_expression( $4, tmp, EXP_OP_DASSIGN, FALSE, @2.first_line, @2.ppfline, @4.pplline, @2.first_column, (@4.last_column - 1), NULL, in_static_expr );
            } Catch_anonymous {
              error_count++;
            }
            if( (stmt = db_create_statement( tmp )) != NULL ) {
              stmt->suppl.part.head       = 1;
              stmt->suppl.part.stop_true  = 1;
              stmt->suppl.part.stop_false = 1;
              stmt->suppl.part.cont       = 1;
              /* Statement will be looped back to itself */
              db_connect_statement_true( stmt, stmt );
              db_connect_statement_false( stmt, stmt );
              db_add_statement( stmt, stmt );
            }
          } else {
            expression_dealloc( $4, FALSE );
          }
        } else {
          expression_dealloc( $4, FALSE );
        }
        expression_dealloc( $1, FALSE );
      } else {
        (void)generator_insert_comb_cov( @1.ppfline, @1.first_column, TRUE, TRUE, FALSE );
      }
      FREE_TEXT( $2 );
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
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), $2, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      FREE_TEXT( $2 );
    }
  | '@' '(' event_expression_list ')'
    {
      @$.first_line   = @3.first_line;
      @$.last_line    = @3.last_line;
      @$.first_column = @3.first_column;
      @$.last_column  = @3.last_column;
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
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      } else {
        @$.last_line = @3.last_line;
        $$ = NULL;
      }
    }
  | event_expression_list ',' event_expression
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Comma-separated combinational logic sensitivity list found in block that is specified to not allow Verilog-2001 syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
            Try {
              $$ = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
            } Catch_anonymous {
              error_count++;
              $$ = NULL;
            }
          } else {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            $$ = NULL;
          }
        }
      } else {
        $$ = NULL;
      }
    }
  ;

event_expression
  : K_posedge expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, NULL, EXP_OP_PEDGE, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | K_negedge expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, NULL, EXP_OP_NEDGE, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL ) ) {
          Try {
            $$ = db_create_expression( $1, NULL, EXP_OP_AEDGE, lhs_mode, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
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
  : K_input  { curr_sig_type = SSUPPL_TYPE_INPUT_NET;  }
  | K_output { curr_sig_type = SSUPPL_TYPE_OUTPUT_NET; }
  | K_inout  { curr_sig_type = SSUPPL_TYPE_INOUT_NET;  }
  ;

defparam_assign_list
  : defparam_assign
  | range defparam_assign
  | defparam_assign_list ',' defparam_assign
  ;

defparam_assign
  : identifier '=' expression
    {
      if( parse_mode ) {
        expression_dealloc( $3, FALSE );
        FREE_TEXT( $1 );
      }
    }
  ;

 /* Parameter override */
parameter_value_opt
  : '#' '(' { param_mode++; in_static_expr = TRUE; } expression_list { param_mode--; in_static_expr = FALSE; } ')'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          expression_dealloc( $4, FALSE );
        }
      }
    }
  | '#' '(' parameter_value_byname_list ')'
  | '#' DEC_NUMBER
    {
      FREE_TEXT( $2 );
    }
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
  : '.' IDENTIFIER '(' { in_static_expr = TRUE; } expression { in_static_expr = FALSE; } ')'
    { PROFILE(PARSER_PARAMETER_VALUE_BYNAME_A);
      if( parse_mode ) {
        param_oride* po;
        if( ignore_mode == 0 ) {
          if( !parser_check_generation( GENERATION_2001 ) ) {
            VLerror( "Explicit in-line parameter passing syntax found in block that is specified to not allow Verilog-2001 syntax" );
            FREE_TEXT( $2 );
            expression_dealloc( $5, FALSE );
          } else {
            po = (param_oride*)malloc_safe( sizeof( param_oride ) );
            po->name = $2;
            po->expr = $5;
            po->next = NULL;
            if( param_oride_head == NULL ) {
              param_oride_head = param_oride_tail = po;
            } else {
              param_oride_tail->next = po;
              param_oride_tail       = po;
            }
          }
        } else {
          FREE_TEXT( $2 );
        }
      } else {
        FREE_TEXT( $2 );
      }
    }
  | '.' IDENTIFIER '(' ')'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( !parser_check_generation( GENERATION_2001 ) ) {
            VLerror( "Explicit in-line parameter passing syntax found in block that is specified to not allow Verilog-2001 syntax" );
          }
        }
      }
      FREE_TEXT( $2 );
    }
  ;

gate_instance_list
  : gate_instance_list ',' gate_instance
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          $3->next = $1;
          $$ = $3;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | gate_instance
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $$ = $1;
        } else {
          $$ = NULL;
        }
      }
    }
  ;

  /* A gate_instance is a module instantiation or a built in part
     type. In any case, the gate has a set of connections to ports. */
gate_instance
  : IDENTIFIER '(' ignore_more expression_list ignore_less ')'
    { PROFILE(PARSER_GATE_INSTANCE_A);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          str_link* tmp;
          tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
          tmp->str   = $1;
          tmp->str2  = NULL;
          tmp->range = NULL;
          tmp->next  = NULL;
          $$ = tmp;
        } else {
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | IDENTIFIER range '(' ignore_more expression_list ignore_less ')'
    { PROFILE(PARSER_GATE_INSTANCE_B);
      if( parse_mode ) {
        curr_prange.clear       = TRUE;
        curr_prange.exp_dealloc = FALSE;
        if( ignore_mode == 0 ) {
          str_link* tmp;
          if( !parser_check_generation( GENERATION_2001 ) ) {
            VLerror( "Arrayed instantiation syntax found in block that is specified to not allow Verilog-2001 syntax" );
            FREE_TEXT( $1 );
            $$ = NULL;
          } else {
            tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
            tmp->str   = $1;
            tmp->str2  = NULL;
            tmp->range = curr_prange.dim;
            tmp->next  = NULL;
            $$ = tmp;
          }
        } else {
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | IDENTIFIER '(' port_name_list ')'
    { PROFILE(PARSER_GATE_INSTANCE_C);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          str_link* tmp;
          tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
          tmp->str   = $1;
          tmp->str2  = NULL;
          tmp->range = NULL;
          tmp->next  = NULL;
          $$ = tmp;
        } else {
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | IDENTIFIER range '(' port_name_list ')'
    { PROFILE(PARSER_GATE_INSTANCE_D);
      if( parse_mode ) {
        curr_prange.clear       = TRUE;
        curr_prange.exp_dealloc = FALSE;
        if( ignore_mode == 0 ) {
          str_link* tmp;
          if( !parser_check_generation( GENERATION_2001 ) ) {
            VLerror( "Arrayed instantiation syntax found in block that is specified to not allow Verilog-2001 syntax" );
            FREE_TEXT( $1 );
            $$ = NULL;
          } else {
            tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
            tmp->str   = $1;
            tmp->str2  = NULL;
            tmp->range = curr_prange.dim;
            tmp->next  = NULL;
            $$ = tmp;
          }
        } else {
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | '(' ignore_more expression_list ignore_less ')'
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
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
  | function_item_list function_item
    {
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
  ;

function_item
  : K_input signed_opt range_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_INPUT_NET;
    }
    list_of_variables ';'
  | block_item_decl
  ;

parameter_assign_decl
  : signed_opt range_opt parameter_assign_list
  ;

parameter_assign_list
  : parameter_assign
  | parameter_assign_list ',' parameter_assign
  ;

parameter_assign
  : IDENTIFIER '=' { in_static_expr = TRUE; } expression { in_static_expr = FALSE; }
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $4 != NULL ) {
            /* If the size was not set by the user, the left number will be set to 0 but we need to change this to 31 */
            assert( curr_prange.dim != NULL );
            if( curr_prange.dim[0].implicit ) {
              db_add_declared_param( curr_signed, NULL, NULL, $1, $4, FALSE );
            } else {
              db_add_declared_param( curr_signed, curr_prange.dim[0].left, curr_prange.dim[0].right, $1, $4, FALSE );
              curr_prange.exp_dealloc = FALSE;
            }
          } else {
            VLerror( "Parameter assignment contains unsupported constructs on the right-hand side" );
          }
        }
      }
      FREE_TEXT( $1 );
    }
  ;

localparam_assign_decl
  : signed_opt range_opt localparam_assign_list
  ;

localparam_assign_list
  : localparam_assign
  | localparam_assign_list ',' localparam_assign
  ;

localparam_assign
  : IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $3 != NULL ) {
            /* If the size was not set by the user, the left number will be set to 0 but we need to change this to 31 */
            assert( curr_prange.dim != NULL );
            if( curr_prange.dim[0].implicit ) {
              db_add_declared_param( curr_signed, NULL, NULL, $1, $3, TRUE );
            } else {
              db_add_declared_param( curr_signed, curr_prange.dim[0].left, curr_prange.dim[0].right, $1, $3, TRUE );
              curr_prange.exp_dealloc = FALSE;
            }
          } else {
            VLerror( "Localparam assignment contains unsupported constructs on the right-hand side" );
          }
        }
      }
      FREE_TEXT( $1 );
    }
  ;

port_name_list
  : port_name_list ',' port_name
  | port_name
  ;

port_name
  : '.' IDENTIFIER '(' ignore_more expression ignore_less ')'
    {
      FREE_TEXT( $2 );
    }
  | '.' IDENTIFIER '(' error ')'
    {
      FREE_TEXT( $2 );
    }
  | '.' IDENTIFIER '(' ')'
    {
      FREE_TEXT( $2 );
    }
  | '.' IDENTIFIER
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Implicit .name port list item found in block that is specified to not allow SystemVerilog syntax" );
      }
      FREE_TEXT( $2 );
    }
  | K_PS
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Implicit .* port list item found in block that is specified to not allow SystemVerilog syntax" );
      }
    }
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
  : IDENTIFIER '=' expression
    {
      FREE_TEXT( $1 );
    }
  | IDENTIFIER '=' expression ':' expression ':' expression
    {
      FREE_TEXT( $1 );
    }
  | PATHPULSE_IDENTIFIER '=' expression
    {
      FREE_TEXT( $1 );
    }
  | PATHPULSE_IDENTIFIER '=' '(' expression ',' expression ')'
    {
      FREE_TEXT( $1 );
    }
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
  : IDENTIFIER
    {
      FREE_TEXT( $1 );
    }
  | IDENTIFIER '[' expr_primary ']'
    {
      FREE_TEXT( $1 );
    }
  | specify_path_identifiers ',' IDENTIFIER
    {
      FREE_TEXT( $3 );
    }
  | specify_path_identifiers ',' IDENTIFIER '[' expr_primary ']'
    {
      FREE_TEXT( $3 );
    }
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
    {
      FREE_TEXT( $2 );
    }
  | spec_notifier ',' 
  | spec_notifier ',' identifier
    {
      FREE_TEXT( $3 );
    }
  | IDENTIFIER
    {
      FREE_TEXT( $1 );
    }
  ;

specify_edge_path_decl
  : specify_edge_path '=' '(' specify_delay_value_list ')'
  | specify_edge_path '=' delay_value_simple
  ;

specify_edge_path
  : '(' K_posedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
      FREE_TEXT( $6 );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
      FREE_TEXT( $6 );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
      FREE_TEXT( $6 );
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
      FREE_TEXT( $6 );
    }
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

 /*
  These are SystemVerilog constructs that are optionally placed before if and case statements.  Covered parses
  them but otherwise ignores them (we will not bother displaying the warning messages).
 */
cond_specifier_opt
  : K_unique
  | K_priority
  |
  ;

 /* SystemVerilog enumeration syntax */
enumeration
  : K_enum enum_var_type_range_opt '{' enum_variable_list '}'
    {
      if( parse_mode ) {
        db_end_enum_list();
      }
    }
  ;

 /* List of valid enumeration variable types */
enum_var_type_range_opt
  : TYPEDEF_IDENTIFIER range_opt
    {
      if( parse_mode ) {
        assert( curr_prange.dim != NULL );
        if( !$1->is_sizeable && !curr_prange.dim[0].implicit ) { 
          VLerror( "Packed dimensions are not allowed for this type" );
        } else {
          if( $1->is_sizeable && !curr_prange.dim[0].implicit ) {
            /* TBD - Need to multiply the size of the typedef with the size of the curr_range */
          } else {
            parser_copy_range_to_curr_range( $1->prange, TRUE );
          }
        }
      }
    }
  | K_reg range_opt
  | K_logic range_opt
  | K_int
    {
      if( parse_mode ) {
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
  | K_integer
    {
      if( parse_mode ) {
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
  | K_shortint
    {
      if( parse_mode ) {
        parser_implicitly_set_curr_range( 15, 0, TRUE );
      }
    }
  | K_longint
    {
      if( parse_mode ) {
        parser_implicitly_set_curr_range( 63, 0, TRUE );
      }
    }
  | K_byte
    {
      if( parse_mode ) {
        parser_implicitly_set_curr_range( 7, 0, TRUE );
      }
    }
  | range_opt
    {
      if( parse_mode ) {
        assert( curr_prange.dim != NULL );
        if( curr_prange.dim[0].implicit ) {
          parser_implicitly_set_curr_range( 31, 0, TRUE );
        }
      }
    }
  ;

 /* This is a lot like a register_variable but assigns proper value to variables with no assignment */
enum_variable
  : IDENTIFIER
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_add_signal( $1, SSUPPL_TYPE_ENUM, &curr_prange, NULL, curr_signed, FALSE, @1.first_line, @1.first_column, TRUE );
          Try {
            db_add_enum( db_find_signal( $1, FALSE ), NULL );
          } Catch_anonymous {
            error_count++;
          }
        }
      }
      FREE_TEXT( $1 );
    }
  | IDENTIFIER '=' static_expr
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_add_signal( $1, SSUPPL_TYPE_ENUM, &curr_prange, NULL, curr_signed, FALSE, @1.first_line, @1.first_column, TRUE );
          Try {
            db_add_enum( db_find_signal( $1, FALSE ), $3 );
          } Catch_anonymous {
            error_count++;
          }
        }
      }
      FREE_TEXT( $1 );
    }
  ;

enum_variable_list
  : enum_variable_list ',' enum_variable
  | enum_variable
  ;

list_of_names
  : IDENTIFIER
    { PROFILE(PARSER_LIST_OF_NAMES_A);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          str_link* strl = (str_link*)malloc_safe( sizeof( str_link ) );
          strl->str    = $1;
          strl->str2   = NULL;
          strl->suppl  = @1.first_line;
          strl->suppl2 = @1.first_column;
          strl->next   = NULL;
          $$ = strl;
        } else {
          FREE_TEXT( $1 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $1 );
        $$ = NULL;
      }
    }
  | list_of_names ',' IDENTIFIER
    { PROFILE(PARSER_LIST_OF_NAMES_B);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          str_link* strl = (str_link*)malloc_safe( sizeof( str_link ) );
          str_link* strt = $1;
          strl->str    = $3;
          strl->str2   = NULL;
          strl->suppl  = @3.first_line;
          strl->suppl2 = @3.first_column;
          strl->next   = NULL;
          while( strt->next != NULL ) strt = strt->next;
          strt->next = strl;
          $$ = $1; 
        } else {
          str_link_delete_list( $1 );
          FREE_TEXT( $3 );
          $$ = NULL;
        }
      } else {
        FREE_TEXT( $3 );
        $$ = NULL;
      }
    }
  ;

 /* Handles typedef declarations (both in module and $root space) */
typedef_decl
  : K_typedef enumeration IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          assert( curr_prange.dim != NULL );
          db_add_typedef( $3, curr_signed, curr_handled, TRUE, parser_copy_curr_range( TRUE ), parser_copy_curr_range( FALSE ) );
        }
      }
      FREE_TEXT( $3 );
    }
  | K_typedef net_type_sign_range_opt IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          assert( curr_prange.dim != NULL );
          db_add_typedef( $3, curr_signed, curr_handled, TRUE, parser_copy_curr_range( TRUE ), parser_copy_curr_range( FALSE ) );
        }
      }
      FREE_TEXT( $3 );
    }
  ;

single_index_expr
  : '[' expression ']'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, $2, EXP_OP_SBIT_SEL, lhs_mode, @1.first_line, @1.ppfline, @3.pplline, @1.first_column, (@3.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
        } else {
          expression_dealloc( $2, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '[' expression ':' expression ']'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
          Try {
            $$ = db_create_expression( $4, $2, EXP_OP_MBIT_SEL, lhs_mode, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
        } else {
          expression_dealloc( $2, FALSE );
          expression_dealloc( $4, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '[' expression K_PO_POS static_expr ']'
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Indexed vector part select found in block that is specified to not allow Verilog-2001 syntax" );
          expression_dealloc( $2, FALSE );
          static_expr_dealloc( $4, FALSE );
          $$ = NULL;
        } else {
          if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
            if( $4->exp == NULL ) {
              Try {
                expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
                vector_dealloc( tmp->value );
                tmp->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
                (void)vector_from_int( tmp->value, $4->num );
                $$ = db_create_expression( tmp, $2, EXP_OP_MBIT_POS, lhs_mode, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
                $$ = NULL;
              }
            } else {
              Try {
                $$ = db_create_expression( $4->exp, $2, EXP_OP_MBIT_POS, lhs_mode, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
                $$ = NULL;
              }
            }
            free_safe( $4, sizeof( static_expr ) );
          } else {
            expression_dealloc( $2, FALSE );
            static_expr_dealloc( $4, FALSE );
            $$ = NULL;
          }
        }
      } else {
        $$ = NULL;
      }
    }
  | '[' expression K_PO_NEG static_expr ']'
    {
      if( parse_mode ) {
        expression* tmp = NULL;
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Indexed vector part select found in block that is specified to not allow Verilog-2001 syntax" );
          expression_dealloc( $2, FALSE );
          static_expr_dealloc( $4, FALSE );
          $$ = NULL;
        } else {
          if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
            if( $4->exp == NULL ) {
              Try {
                tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.ppfline, @1.pplline, @1.first_column, (@1.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
              }
              vector_dealloc( tmp->value );
              tmp->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
              (void)vector_from_int( tmp->value, $4->num );
              Try {
                tmp = db_create_expression( tmp, $2, EXP_OP_MBIT_NEG, lhs_mode, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
              }
            } else {
              Try {
                tmp = db_create_expression( $4->exp, $2, EXP_OP_MBIT_NEG, lhs_mode, @1.first_line, @1.ppfline, @5.pplline, @1.first_column, (@5.last_column - 1), NULL, in_static_expr );
              } Catch_anonymous {
                error_count++;
              }
            }
            free_safe( $4, sizeof( static_expr ) );
            $$ = tmp;
          } else {
            expression_dealloc( $2, FALSE );
            static_expr_dealloc( $4, FALSE );
            $$ = NULL;
          }
        }
      } else {
        $$ = NULL;
      }
    }
  ;

index_expr
  : index_expr single_index_expr
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, $1, EXP_OP_DIM, lhs_mode, @1.first_line, @1.ppfline, @2.pplline, @1.first_column, (@2.last_column - 1), NULL, in_static_expr );
          } Catch_anonymous {
            error_count++;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $2, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | single_index_expr
    {
      $$ = $1;
    }
  ;

ignore_more
  :
    {
      if( parse_mode ) {
        ignore_mode++;
      }
    }
  ;
  
ignore_less
  :
    {
      if( parse_mode ) {
        ignore_mode--;
      }
    }
  ;

start_lhs
  :
    {
      if( parse_mode ) {
        lhs_mode = TRUE;
      }
    }
  ;

end_lhs
  :
    {
      if( parse_mode ) {
        lhs_mode = FALSE;
      }
    }
  ;

inc_fork_depth
  :
    {
      fork_depth++;
      fork_block_depth = (int*)realloc_safe( fork_block_depth, (sizeof( int ) * fork_depth), ((fork_depth + 1) * sizeof( int )) );
      fork_block_depth[fork_depth] = block_depth;
    }
  ;

dec_fork_depth
  :
    {
      fork_depth--;
      fork_block_depth = (int*)realloc_safe( fork_block_depth, (sizeof( int ) * (fork_depth + 2)), ((fork_depth + 1) * sizeof( int )) );
    }
  ;

inc_block_depth
  :
    {
      block_depth++;
      if( !parse_mode ) {
        generator_add_cov_to_work_code( " begin" );
        generator_flush_work_code;
      }
    }
  ;

dec_block_depth
  :
    {
      block_depth--;
      if( !parse_mode ) {
        generator_add_to_hold_code( " end ", __FILE__, __LINE__ );
      }
    }
  ;

inc_block_depth_only
  :
    {
      block_depth++;
    }
  ;

dec_block_depth_only
  :
    {
      block_depth--;
    }
  ;

dec_block_depth_flush
  :
    {
      block_depth--;
      if( !parse_mode ) {
        generator_flush_work_code;
      }
    }
  ;

inc_for_depth
  :
    {
      char* scope;
      for_mode++;
      scope = db_create_unnamed_scope();
      if( parse_mode ) {
        func_unit* funit = db_get_curr_funit();
        if( ignore_mode == 0 ) {
          Try {
            assert( db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, scope, funit->orig_fname, funit->incl_fname, for_start_line, for_start_col ) );
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        }
        block_depth++;
        $$ = db_get_curr_funit();
      } else {
        func_unit*   funit = db_get_tfn_by_position( for_start_line, for_start_col );
        char         str[50];
        char*        back;
        char*        rest;
        unsigned int rv;
        assert( funit != NULL );
        back = strdup_safe( funit->name );
        rest = strdup_safe( funit->name );
        scope_extract_back( funit->name, back, rest );
        rv = snprintf( str, 50, " begin : %s", back );
        assert( rv < 50 );
        generator_add_to_hold_code( str, __FILE__, __LINE__ );
        free_safe( back, (strlen( funit->name ) + 1) );
        free_safe( rest, (strlen( funit->name ) + 1) );
        generator_insert_inst_id_reg( funit );
        block_depth++;
        $$ = NULL;
      }
      free_safe( scope, (strlen( scope ) + 1) );
    }
  ;

dec_for_depth
  :
    {
      block_depth--;
      if( !parse_mode ) {
        generator_add_to_hold_code( " end ", __FILE__, __LINE__ );      
      }
    }
  ;

inc_gen_expr_mode
  :
    {
      if( parse_mode ) {
        generate_expr_mode++;
      }
    }
  ;

dec_gen_expr_mode
  : 
    {
      if( parse_mode ) {
        generate_expr_mode--;
      }
    }
  ;