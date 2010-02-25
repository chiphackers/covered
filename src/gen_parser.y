
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
    {
      generator_create_tmp_regs();
    }
    statement
    K_endfunction
    {
      func_unit* funit = db_get_tfn_by_position( @5.first_line, @5.first_column );
      $$ = generator_build( 12, strdup_safe( "function" ), $2, $3, $4, $5, strdup_safe( ";" ), "\n", $7,
                            (generator_is_static_function( funit ) ? generator_inst_id_reg( funit ) : NULL), $8, strdup_safe( "endfunction" ), "\n" );
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

udp_port_decls
  : udp_port_decl
    {
      $$ = $1;
    }
  | udp_port_decls udp_port_decl
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

udp_port_decl
  : K_input list_of_variables ';'
    {
      $$ = generator_build( 4, strdup_safe( "input" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_output IDENTIFIER ';'
    {
      $$ = generator_build( 4, strdup_safe( "output" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_reg IDENTIFIER ';'
    {
      $$ = generator_build( 4, strdup_safe( "reg", $2, strdup_safe( ";" ), "\n" );
    }
  ;

udp_init_opt
  : udp_initial
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

udp_initial
  : K_initial IDENTIFIER '=' number ';'
    {
      $$ = generator_build( 6, strdup_safe( "initial" ), $2, strdup_safe( "=" ), $4, strdup_safe( ";" ), "\n" );
    }
  ;

udp_body
  : K_table { lex_start_udp_table(); }
      udp_entry_list
    K_endtable { lex_end_udp_table(); }
    {
      $$ = generator_build( 4, $1, $3, $4, "\n" );
    }
  ;

udp_entry_list
  : udp_comb_entry_list
    {
      $$ = $1;
    }
  | udp_sequ_entry_list
    {
      $$ = $1;
    }
  ;

udp_comb_entry_list
  : udp_comb_entry
    {
      $$ = $1;
    }
  | udp_comb_entry_list udp_comb_entry
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

udp_comb_entry
  : udp_input_list ':' udp_output_sym ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( ":" ), $3, strdup_safe( ";" ), "\n" );
    }
  ;

udp_input_list
  : udp_input_sym
    {
      $$ = $1;
    }
  | udp_input_list udp_input_sym
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

udp_input_sym
  : '0' { $$ = strdup_safe( "0" ); }
  | '1' { $$ = strdup_safe( "1" ); }
  | 'x' { $$ = strdup_safe( "x" ); }
  | '?' { $$ = strdup_safe( "?" ); }
  | 'b' { $$ = strdup_safe( "b" ); }
  | '*' { $$ = strdup_safe( "*" ); }
  | '%' { $$ = strdup_safe( "%" ); }
  | 'f' { $$ = strdup_safe( "f" ); }
  | 'F' { $$ = strdup_safe( "F" ); }
  | 'l' { $$ = strdup_safe( "l" ); }
  | 'h' { $$ = strdup_safe( "h" ); }
  | 'B' { $$ = strdup_safe( "B" ); }
  | 'r' { $$ = strdup_safe( "r" ); }
  | 'R' { $$ = strdup_safe( "R" ); }
  | 'M' { $$ = strdup_safe( "M" ); }
  | 'n' { $$ = strdup_safe( "n" ); }
  | 'N' { $$ = strdup_safe( "N" ); }
  | 'p' { $$ = strdup_safe( "p" ); }
  | 'P' { $$ = strdup_safe( "P" ); }
  | 'Q' { $$ = strdup_safe( "Q" ); }
  | '_' { $$ = strdup_safe( "_" ); }
  | '+' { $$ = strdup_safe( "+" ); }
  ;

udp_output_sym
  : '0' { $$ = strdup_safe( "0" ); }
  | '1' { $$ = strdup_safe( "1" ); }
  | 'x' { $$ = strdup_safe( "x" ); }
  | '-' { $$ = strdup_safe( "-" ); }
  ;

udp_sequ_entry_list
  : udp_sequ_entry
    {
      $$ = $1;
    }
  | udp_sequ_entry_list udp_sequ_entry
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

udp_sequ_entry
  : udp_input_list ':' udp_input_sym ':' udp_output_sym ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( ":" ), $3, strdup_safe( ":" ), $5, strdup_safe( ";" ), "\n" );
    }
  ;

op_and_assign_op
  : K_ADD_A  { $$ = strdup_safe( "+=" ); }
  | K_SUB_A  { $$ = strdup_safe( "-=" ); }
  | K_MLT_A  { $$ = strdup_safe( "*=" ); }
  | K_DIV_A  { $$ = strdup_safe( "/=" ); }
  | K_MOD_A  { $$ = strdup_safe( "%=" ); }
  | K_AND_A  { $$ = strdup_safe( "&=" ); }
  | K_OR_A   { $$ = strdup_safe( "|=" );  }
  | K_XOR_A  { $$ = strdup_safe( "^=" ); }
  | K_LS_A   { $$ = strdup_safe( "<<=" );  }
  | K_RS_A   { $$ = strdup_safe( ">>=" );  }
  | K_ALS_A  { $$ = strdup_safe( "<<<=" ); }
  | K_ARS_A  { $$ = strdup_safe( ">>>=" ); }
  ;

 /* This statement type can be found in FOR statements in generate blocks */
generate_passign
  : IDENTIFIER '=' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  | IDENTIFIER op_and_assign_op static_expr
    {
      $$ = generator_build( 3, $1, $2, $3 );
    }
  | IDENTIFIER post_op_and_assign_op
    {
      $$ = generator_build( 2, $1, $2 );
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
      $$ = generator_build( 2, $1, $2 );
    }
  | generate_item
    {
      $$ = $1;
    }
  ;

gen_if_body
  : generate_item K_else generate_item
    {
      $$ = generator_build( 3, $1, strdup_safe( "else" ), $3 );
    }
  | generate_item %prec less_than_K_else
    {
      $$ = $1;
    }
  ;

  /* Similar to module_item but is more restrictive */
generate_item
  : module_item
    {
      $$ = $1;
    }
  | K_begin generate_item_list_opt end_gen_block K_end
    {
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
      free_safe( back, (strlen( funit->name ) + 1) );
      free_safe( rest, (strlen( funit->name ) + 1) );
      $$ = generator_build( 9, strdup_safe( "begin" ), strdup_safe( str ), "\n", generator_inst_id_reg( funit ), "\n", generator_temp_regs(), $2, strdup_safe( "end" ), "\n" );
    }
  | K_begin ':' IDENTIFIER generate_item_list_opt K_end
    {
      func_unit* funit = db_get_tfn_by_position( @3.first_line, @3.first_column );
      assert( funit != NULL );
      $$ = generator_build( 9, strdup_safe( "begin : " ), $3, "\n", generator_inst_id_reg( funit ), "\n", generator_temp_regs(), $4, strdup_safe( "end" ), "\n" );
    }
  | K_for '(' generate_passign ';' static_expr ';' generate_passign ')' K_begin ':' IDENTIFIER generate_item_list_opt K_end
    {
      func_unit* funit = db_get_tfn_by_position( @11.first_line, @11.first_column );
      assert( funit != NULL );
      $$ = generator_build( 17, strdup_safe( "for(" ), $3, strdup_safe( ";" ), $5, strdup_safe( ";" ), $7, strdup_safe( ")" ), "\n", strdup_safe( "begin : " ), $11, "\n",
                            generator_inst_id_reg( funit ), "\n", generator_temp_regs(), $13, strdup_safe( "end" ), "\n" );
    }
  | K_if '(' static_expr ')' gen_if_body
    {
      $$ = generator_build( 4, strdup_safe( "if(" ), $3, strdup_safe( ")" ), $5 );
    }
  | K_case '(' static_expr ')' generate_case_items K_endcase
    { 
      $$ = generator_build( 6, strdup_safe( "case(" ), $3, strdup_safe( ")" ), $5, strdup_safe( "endcase" ), "\n" );
    }
  ;

generate_case_item
  : expression_list ':' generate_item
    {
      $$ = generator_build( 3, $1, strdup_safe( ":" ), $3 );
    }
  | K_default ':' generate_item
    {
      $$ = generator_build( 2, strdup_safe( "default : " ), $3 );
    }
  | K_default generate_item
    {
      $$ = generator_build( 2, strdup_safe( "default" ), $2 );
    }
  | error ':' generate_item
    {
      VLerror( "Illegal generate case expression" );
      FREE_TEXT( $3 );
    }
  ;

generate_case_items
  : generate_case_items generate_case_item
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | generate_case_item
    {
      $$ = $1;
    }
  ;

  /* This is the start of a module body */
module_item_list_opt
  : module_item_list
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

module_item_list
  : module_item_list module_item
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | module_item
    {
      $$ = $1;
    }
  ;

module_item
  : attribute_list_opt
    net_type signed_opt range_opt list_of_variables ';'
    {
      $$ = generator_build( 7, $1, $2, $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    net_type signed_opt range_opt net_decl_assigns ';'
    {
      $$ = generator_build( 7, $1, $2, $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    net_type drive_strength net_decl_assigns ';'
    {
      $$ = generator_build( 6, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    TYPEDEF_IDENTIFIER register_variable_list ';'
    {
      $$ = generator_build( 5, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    port_type signed_opt range_opt list_of_variables ';'
    {
      $$ = generator_build( 7, $1, $2, $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    port_type net_type signed_opt range_opt list_of_variables ';'
    {
      $$ = generator_build( 8, $1, $2, $3, $4, $5, $6, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    K_output var_type signed_opt range_opt list_of_variables ';'
    {
      $$ = generator_build( 8, $1, strdup_safe( "output" ), $3, $4, $5, $6, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    port_type signed_opt range_opt error ';'
    {
      VLerror( "Invalid variable list in port declaration" );
      FREE_TEXT( generator_build( 5, $1, $2, $3, $4, $5 ) );
    }
  | attribute_list_opt
    K_trireg charge_strength_opt range_opt delay3_opt list_of_variables ';'
    {
      $$ = generator_build( 8, $1, strdup_safe( "trireg" ), $3, $4, $5, $6, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    gatetype gate_instance_list ';'
    {
      $$ = generator_build( 5, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt
    gatetype delay3 gate_instance_list ';'
    {
      $$ = generator_build( 6, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt gatetype drive_strength gate_instance_list ';'
    {
      $$ = generator_build( 6, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt gatetype drive_strength delay3 gate_instance_list ';'
    {
      $$ = generator_build( 7, $1, $2, $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_pullup gate_instance_list ';'
    {
    }
  | attribute_list_opt K_pulldown gate_instance_list ';'
    {
    }
  | attribute_list_opt K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
    }
  | attribute_list_opt K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
    }
  | block_item_decl
    {
      $$ = $1;
    }
  | attribute_list_opt K_defparam defparam_assign_list ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "defparam" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_event list_of_variables ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "reg" ), $3, strdup_safe( ";" ), "\n" );
    }
  /* Handles instantiations of modules and user-defined primitives. */
  | attribute_list_opt IDENTIFIER parameter_value_opt gate_instance_list ';'
    {
      $$ = generator_build( 6, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_assign drive_strength_opt delay3_opt assign_list ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "assign" ), $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_always statement
    {
      $$ = generator_build( 3, $1, strdup_safe( "always" ), $3 );
    }
  | attribute_list_opt K_always_comb statement
    {
      if( !info_suppl.part.verilator ) {
        $$ = generator_build( 8, $1, strdup_safe( "always_comb" ), strdup_safe( "begin" ),
                              generator_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE ),
                              generator_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE ), $3, strdup_safe( "end" ), "\n" );
      } else {
        $$ = generator_build( 3, $1, strdup_safe( "always_comb" ), $3 );
      }
    }
  | attribute_list_opt K_always_latch statement
    {
      if( !info_suppl.part.verilator ) {
        $$ = generator_build( 8, $1, strdup_safe( "always_latch" ), strdup_safe( "begin" ),
                              generator_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE ),
                              generator_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE ), $3, strdup_safe( "end" ), "\n" );
      } else {
        $$ = generator_build( 3, $1, strdup_safe( "always_latch" ), $3 );
      }
    }
  | attribute_list_opt K_always_ff statement
    {
      if( !info_suppl.part.verilator ) {
        $$ = generator_build( 8, $1, strdup_safe( "always_ff" ), strdup_safe( "begin" ),
                              generator_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE ),
                              generator_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE ), $3, strdup_safe( "end" ), "\n" );
      } else {
        $$ = generator_build( 3, $1, strdup_safe( "always_ff" ), $3 );
      }
    }
  | attribute_list_opt K_initial statement
    {
      $$ = generator_build( 3, $1, strdup_safe( "initial" ), $3 );
    }
  | attribute_list_opt K_final statement
    {
      $$ = generator_build( 3, $1, strdup_safe( "final" ), $3 );
    }
  | attribute_list_opt K_task automatic_opt IDENTIFIER ';' task_item_list_opt statement_or_null K_endtask
    {
      $$ = generator_build( 10, $1, strdup_safe( "task" ), $3, $4, strdup_safe( ";" ), "\n", $6, $7, strdup_safe( "endtask" ), "\n" );
    }
  | attribute_list_opt K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    function_item_list
    {
      generator_create_tmp_regs();
    }
    statement
    K_endfunction
    {
      func_unit* funit = db_get_tfn_by_position( @6.first_line, @6.first_column );
      $$ = generator_build( 15, strdup_safe( "function" ), $2, $3, $4, $5, $6, strdup_safe( ";" ), "\n", $8,
                            (generator_is_static_function( funit ) ? generator_inst_id_reg( funit ) : NULL), "\n", generator_tmp_regs(), $10, strdup_safe( "endfunction" ), "\n" );
    }
  | K_generate generate_item_list_opt K_endgenerate
    {
      $$ = generator_build( 4, strdup_safe( "generate" ), $2, strdup_safe( "endgenerate" ), "\n" );
    }
  | K_genvar list_of_variables ';'
    {
      $$ = generator_build( 4, strdup_safe( "genvar" ), $2, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_specify specify_item_list K_endspecify
    {
      $$ = generator_build( 5, $1, strdup_safe( "specify" ), $3, strdup_safe( "endspecify" ), "\n" );
    }
  | attribute_list_opt K_specify K_endspecify
    {
      $$ = generator_build( 3, $1, strdup_safe( "specify endspecify" ), "\n" );
    }
  | attribute_list_opt K_specify error K_endspecify
    {
      VLerror( "Invalid specify syntax" );
      $$ = NULL;
    }
  | error ';'
    {
      VLerror( "Invalid module item.  Did you forget an initial or always?" );
      $$ = NULL;
    }
  | attribute_list_opt K_assign error '=' expression ';'
    {
      VLerror( "Syntax error in left side of continuous assignment" );
      $$ = NULL;
    }
  | attribute_list_opt K_assign error ';'
    {
      VLerror( "Syntax error in continuous assignment" );
      $$ = NULL;
    }
  | attribute_list_opt K_function error K_endfunction
    {
      VLerror( "Syntax error in function description" );
      $$ = NULL;
    }
  | attribute_list_opt enumeration list_of_names ';'
    {
      $$ = generator_build( 4, $1, $2, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt typedef_decl
    {
      $$ = generator_build( 3, $1, $2, "\n" );
    }
  /* SystemVerilog assertion - we don't currently support these and I don't want to worry about how to parse them either */
  | attribute_list_opt IDENTIFIER ':' K_assert ';'
    {
      $$ = generator_build( 4, $1, $2, strdup_safe( ": assert;" ), "\n" );
    }
  /* SystemVerilog property - we don't currently support these but crudely parse them */
  | attribute_list_opt
    K_property K_endproperty
    {
      $$ = NULL;  /* TBD */
    }
  /* SystemVerilog sequence - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_sequence K_endsequence
    {
      $$ = NULL;  /* TBD */
    }
  /* SystemVerilog program block - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_program K_endprogram
    {
      $$ = NULL;  /* TBD */
    }
  | KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')' ';'
    {
      $$ = generator_build( strdup_safe( 8, "$attribute(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, strdup_safe( ");" ), "\n" );
    } 
  | KK_attribute '(' error ')' ';'
    {
      VLerror( "Syntax error in $attribute parameter list" );
      $$ = NULL;
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
      $$ = generator_build( 2, generator_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE ), $1 );
    }
  ;

  /* TBD - In the SystemVerilog BNF, there are more options available for this rule -- but we don't current support them */
data_type
  : integer_vector_type signed_opt range_opt
    {
      $$ = generator_build( 3, $1, $2, $3 );
    }
  | integer_atom_type signed_opt
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | struct_union '{' struct_union_member_list '}' range_opt
    {
      $$ = generator_build( 5, $1, strdup_safe( "{" ), $3, strdup_safe( "}" ), $5 );
    }
  | struct_union K_packed signed_opt '{' struct_union_member_list '}' range_opt
    {
      $$ = generator_build( 7, $1, strdup_safe( "packed" ), $3, strdup_safe( "{" ), $5, strdup_safe( "}" ), $7 );
    }
  | K_event
    {
      $$ = strdup_safe( "reg" );
    }
  ;

data_type_opt
  : data_type
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

  /* TBD - Not sure what this should return at this point */
struct_union
  : K_struct         { $$ = strdup_safe( "struct" );       }
  | K_union          { $$ = strdup_safe( "union" );        }
  | K_union K_tagged { $$ = strdup_safe( "union tagged" ); }
  ;

struct_union_member_list
  : attribute_list_opt data_type_or_void list_of_variables ';'
    {
      $$ = generator_build( 5, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  | struct_union_member_list ',' attribute_list_opt data_type_or_void list_of_variables ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "," ), $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  |
    {
      $$ = NULL;
    }
  ;

data_type_or_void
  : data_type
    {
      $$ = $1;
    }
  | K_void 
    {
      $$ = strdup_safe( "void" );
    }
  ;

integer_vector_type
  : K_bit   { $$ = strdup_safe( "bit" );   }
  | K_logic { $$ = strdup_safe( "logic" ); }
  | K_reg   { $$ = strdup_safe( "reg" );   }
  ;

integer_atom_type
  : K_byte     { $$ = strdup_safe( "byte" );     }
  | K_char     { $$ = strdup_safe( "char" );     }
  | K_shortint { $$ = strdup_safe( "shortint" ); }
  | K_int      { $$ = strdup_safe( "int" );      }
  | K_integer  { $$ = strdup_safe( "integer" );  }
  | K_longint  { $$ = strdup_safe( "longint" );  }
  | K_time     { $$ = strdup_safe( "time" );     }
  ;

expression_assignment_list
  : data_type_opt IDENTIFIER '=' expression
    {
      $$ = generator_build( 5, generator_line_cov( @2.ppfline, ((@4.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@4.last_column - 1), TRUE ),
                            $1, $2, strdup_safe( "=" ), $4 );
    }
  | expression_assignment_list ',' data_type_opt IDENTIFIER '=' expression
    {
      $$ = generator_build( 7, generator_line_cov( @4.ppfline, ((@6.last_line - @4.first_line) + @4.ppfline), @4.first_column, (@6.last_column - 1), TRUE ),
                            $1, strdup_safe( "," ), $3, $4, strdup_safe( "=" ), $6 );
    }
  ;

passign
  : lpvalue '=' expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE ),
                            $1, strdup_safe( "=" ), $3 );
    }
  | lpvalue K_ADD_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "+=" ), $3 );
    }
  | lpvalue K_SUB_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "-=" ), $3 );
    }
  | lpvalue K_MLT_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "*=" ), $3 );
    }
  | lpvalue K_DIV_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "/=" ), $3 );
    }
  | lpvalue K_MOD_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "%=" ), $3 );
    }
  | lpvalue K_AND_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "&=" ), $3 );
    }
  | lpvalue K_OR_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "|=" ), $3 );
    }
  | lpvalue K_XOR_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "^=" ), $3 );
    }
  | lpvalue K_LS_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "<<=" ), $3 );
    }
  | lpvalue K_RS_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( ">>=" ), $3 );
    }
  | lpvalue K_ALS_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "<<<=" ), $3 );
    }
  | lpvalue K_ARS_A expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( ">>>=" ), $3 );
    }
  | lpvalue K_INC
    {
      $$ = generator_build( 3, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "++" ) );
    }
  | lpvalue K_DEC
    {
      $$ = generator_build( 3, generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            $1, strdup_safe( "--" ) );
    }
  ;

if_body
  : statement_or_null %prec less_than_K_else
    {
      $$ = $1;
    }
  | statement_or_null K_else statement_or_null
    {
      $$ = generator_build( 3, $1, strdup_safe( "else" ), $3 );
    }
  ;

statement
  : K_assign lavalue '=' expression ';'
    {
      $$ = generator_build( 6, strdup_safe( "assign" ), $2, strdup_safe( "=" ), $4, strdup_safe( ";" ), "\n" );
    }
  | K_deassign lavalue ';'
    {
      $$ = generator_build( 4, strdup_safe( "deassign" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_force lavalue '=' expression ';'
    {
      $$ = generator_build( 6, strdup_safe( "force" ), $2, strdup_safe( "=" ), $4, strdup_safe( ";" ), "\n" );
    }
  | K_release lavalue ';'
    {
      $$ = generator_build( 4, strdup_safe( "release" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_begin begin_end_block K_end
    {
      $$ = generator_build( 4, strdup_safe( "begin" ), $2, strdup_safe( "end" ), "\n" );
    }
  | K_fork fork_statement K_join
    {
      $$ = generator_build( 4, strdup_safe( "fork" ), $2, strdup_safe( "join" ), "\n" );
    }
  | K_disable identifier ';'
    {
      $$ = generator_build( 5, generator_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE ),
                            strdup_safe( "disable" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_TRIGGER IDENTIFIER ';'
    {
      $$ = generator_build( 8, generator_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE ),
                            $1, strdup_safe( "= (" ), $1, strdup_safe( "=== 1'bx) ? 1'b0 : ~" ), $1, strdup_safe( ";" ), "\n" );
    }
  | K_forever statement
    {
      $$ = generator_build( 2, strdup_safe( "forever" ), $2 );
    }
  | K_repeat '(' expression ')' statement
    {
      $$ = generator_build( 6, generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE ),
                            strdup_safe( "repeat(" ), $3, strdup_safe( ")" ), $5 );
    }
  | cond_specifier_opt K_case '(' expression ')' case_body K_endcase
    {
      $$ = generator_build( 8, generator_case_comb_cov( @4.ppfline, @4.first_column ),
                            $1, strdup_safe( "case(" ), $4, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
    }
  | cond_specifier_opt K_casex '(' expression ')'
    {
      $$ = generator_build( 8, generator_case_comb_cov( @4.ppfline, @4.first_column ),
                            $1, strdup_safe( "casex(" ), $4, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
    }
  | cond_specifier_opt K_casez '(' expression ')'
    {
      $$ = generator_build( 8, generator_case_comb_cov( @4.ppfline, @4.first_column ),
                            $1, strdup_safe( "casez(" ), $4, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
    }
  | cond_specifier_opt K_if '(' expression ')' if_body
    {
      $$ = generator_build( 7, generator_line_cov( @2.ppfline, ((@5.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@5.last_column - 1), TRUE ),
                            generator_comb_cov( @2.ppfline, @2.first_column, FALSE, TRUE, FALSE ),
                            $1, strdup_safe( "if(" ), $4, strdup_safe( ")" ), $6 );
    }
  | cond_specifier_opt K_if '(' error ')' if_statement_error
    {
      VLerror( "Illegal conditional if expression" );
      $$ = NULL;
    }
  /* START_HERE */
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
    statement
    {
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
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
    {
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
    {
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
    {
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
    {
    }
  ;

polarity_operator
  : K_PO_POS { $$ = strdup_safe( "+:" ); }
  | K_PO_NEG { $$ = strdup_safe( "-:" ); }
  | ':'      { $$ = strdup_safe( ":" );  }
  ;

specify_delay_value_list
  : delay_value
    {
      $$ = $1;
    }
  | specify_delay_value_list ',' delay_value
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

 /*
  These are SystemVerilog constructs that are optionally placed before if and case statements.  Covered parses
  them but otherwise ignores them (we will not bother displaying the warning messages).
 */
cond_specifier_opt
  : K_unique   { $$ = strdup_safe( "unique" );   }
  | K_priority { $$ = strdup_safe( "priority" ); }
  |            { $$ = NULL;                      }
  ;

 /* SystemVerilog enumeration syntax */
enumeration
  : K_enum enum_var_type_range_opt '{' enum_variable_list '}'
    {
      $$ = generator_build( 5, strdup_safe( "enum" ), $2, strdup_safe( "{" ), $4, strdup_safe( "}" ) );
    }
  ;

 /* List of valid enumeration variable types */
enum_var_type_range_opt
  : TYPEDEF_IDENTIFIER range_opt
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | K_reg range_opt
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | K_logic range_opt
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | K_int      { $$ = strdup_safe( "int" );      }
  | K_integer  { $$ = strdup_safe( "integer" );  }
  | K_shortint { $$ = strdup_safe( "shortint" ); }
  | K_longint  { $$ = strdup_safe( "longing" );  }
  | K_byte     { $$ = strdup_safe( "byte" );     }
  | range_opt
    {
      $$ = $1;
    }
  ;

 /* This is a lot like a register_variable but assigns proper value to variables with no assignment */
enum_variable
  : IDENTIFIER
    {
      $$ = $1;
    }
  | IDENTIFIER '=' static_expr
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  ;

enum_variable_list
  : enum_variable_list ',' enum_variable
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | enum_variable
    {
      $$ = $1;
    }
  ;

list_of_names
  : IDENTIFIER
    {
      $$ = $1;
    }
  | list_of_names ',' IDENTIFIER
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

 /* Handles typedef declarations (both in module and $root space) */
typedef_decl
  : K_typedef enumeration IDENTIFIER ';'
    {
      $$ = generator_build( 5, strdup_safe( "typedef" ), $2, $3, strdup_safe( ";" ), "\n" );
    }
  | K_typedef net_type_sign_range_opt IDENTIFIER ';'
    {
      $$ = generator_build( 5, strdup_safe( "typedef" ), $2, $3, strdup_safe( ";" ), "\n" );
    }
  ;

single_index_expr
  : '[' expression ']'
    {
      $$ = generator_build( 3, strdup_safe( "[" ), $2, strdup_safe( "]" ) );
    }
  | '[' expression ':' expression ']'
    {
      $$ = generator_build( 5, strdup_safe( "[" ), $2, strdup_safe( ":" ), $4, strdup_safe( "]" ) );
    }
  | '[' expression K_PO_POS static_expr ']'
    {
      $$ = generator_build( 5, strdup_safe( "[" ), $2, strdup_safe( "+:" ), $4, strdup_safe( "]" ) );
    }
  | '[' expression K_PO_NEG static_expr ']'
    {
      $$ = generator_build( 5, strdup_safe( "[" ), $2, strdup_safe( "-:" ), $4, strdup_safe( "]" ) );
    }
  ;

index_expr
  : index_expr single_index_expr
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | single_index_expr
    {
      $$ = $1;
    }
  ;

