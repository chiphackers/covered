
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
 \file     parser.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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

#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
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
 Pointer to head of parameter override list.
*/
param_oride* param_oride_head = NULL;

/*!
 Pointer to tail of parameter override list.
*/
param_oride* param_oride_tail = NULL;

/*!
 Pointer to head of signal list
*/
sig_link* dummy_head    = NULL;

/*!
 Pointer to tail of signal list
*/
sig_link* dummy_tail    = NULL;


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
  exp_link*       explink;
  case_statement* case_stmt;
  attr_param*     attr_parm;
  exp_bind*       expbind;
  port_info*      portinfo;
  gen_item*       gitem;
  case_gitem*     case_gi;
  typedef_item*   typdef;
  func_unit*      funit;
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
%type <state>     statement statement_list statement_or_null 
%type <state>     if_statement_error
%type <state>     passign for_initialization
%type <state>     expression_assignment_list
%type <strlink>   gate_instance gate_instance_list list_of_names
%type <funit>     begin_end_block fork_statement inc_for_depth
%type <attr_parm> attribute attribute_list
%type <portinfo>  port_declaration list_of_port_declarations
%type <gitem>     generate_item generate_item_list generate_item_list_opt
%type <case_stmt> case_items case_item
%type <case_gi>   generate_case_items generate_case_item
%type <optype>    static_unary_op unary_op syscall_wo_parms_op syscall_w_parms_op syscall_w_parms_op_64 syscall_w_parms_op_32
%type <optype>    pre_op_and_assign_op post_op_and_assign_op op_and_assign_op

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
%destructor { free_safe( $$, (strlen( $$ ) + 1) ); } IDENTIFIER PATHPULSE_IDENTIFIER identifier begin_end_id

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
  : K_PSTAR
    {
      if( parse_mode ) {
        parser_check_pstar();
      }
    }
    attribute_list K_STARP
    {
      if( parse_mode ) {
        parser_check_attribute( $3 );
      }
    }
  | K_PSTAR
    {
      if( parse_mode ) {
        parser_check_pstar();
      }
    }
    K_STARP
  |
  ;

attribute_list
  : attribute_list ',' attribute
    {
      if( parse_mode ) {
        $$ = parser_append_to_attr_list( $1, $3 );
      } else {
        $$ = NULL;
      }
    } 
  | attribute
    {
      if( parse_mode ) {
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  ;

attribute
  : IDENTIFIER
    {
      if( parse_mode ) {
        $$ = parser_create_attr( $1, NULL );
      } else {
        $$ = NULL;
      }
    }
  | IDENTIFIER '=' {attr_mode++;} expression {attr_mode--;}
    {
      if( parse_mode ) {
        $$ = parser_create_attr( $1, $4 );
      } else {
        $$ = NULL;
      }
    }
  ;

source_file 
  : description 
  | source_file description
  ;

description
  : module
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | udp_primitive
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')'
    {
      free_safe( $3, (strlen( $3 ) + 1) );
      vector_dealloc( $5.vec );
      vector_dealloc( $7.vec );
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | typedef_decl
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | net_type signed_opt range_opt list_of_variables ';'
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | net_type signed_opt range_opt
    net_decl_assigns ';'
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | net_type drive_strength
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 == 1) ) {
          parser_implicitly_set_curr_range( 0, 0, TRUE );
        }
        curr_signed = FALSE;
      }
    }
    net_decl_assigns ';'
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | K_trireg charge_strength_opt range_opt delay3_opt
    {
      curr_mba      = FALSE;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECL_REG;
    }
    list_of_variables ';'
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | gatetype gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $2 );
      } else {
        generator_flush_all();
      }
    }
  | gatetype delay3 gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      } else {
        generator_flush_all();
      }
    }
  | gatetype drive_strength gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      } else {
        generator_flush_all();
      }
    }
  | gatetype drive_strength delay3 gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $4 );
      } else {
        generator_flush_all();
      }
    }
  | K_pullup gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $2 );
      } else {
        generator_flush_all();
      }
    }
  | K_pulldown gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $2 );
      } else {
        generator_flush_all();
      }
    }
  | K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $5 );
      } else {
        generator_flush_all();
      }
    }
  | K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $5 );
      } else {
        generator_flush_all();
      }
    }
  | block_item_decl
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | K_event
    {
      if( parse_mode ) {
        curr_mba      = TRUE;
        curr_signed   = FALSE;
        curr_sig_type = SSUPPL_TYPE_EVENT;
        curr_handled  = TRUE;
        parser_implicitly_set_curr_range( 0, 0, TRUE );
      }
    }
    list_of_variables ';'
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | K_task automatic_opt IDENTIFIER ';'
    {
      if( parse_mode ) {
        parser_create_task_decl( $2, $3, @3.text, @3.first_line );
      }
    }
    task_item_list_opt statement_or_null
    {
      parser_create_task_body( $7, @7.first_line, @7.first_column, @7.last_column );
    }
    K_endtask
    {
      if( parse_mode ) {
        parser_end_task_function( @9.first_line );
      } else {
        generator_flush_all();
      }
    }
  | K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    {
      if( parse_mode ) {
        parser_create_function_decl( $2, $5, @5.text, @5.first_line, @5.first_column );
      }
    }
    function_item_list statement
    {
      parser_create_function_body( $9 );
    }
    K_endfunction
    {
      if( parse_mode ) {
        parser_end_task_function( @11.first_line );
      } else {
        generator_flush_all();
      }
    }
  | error ';'
    {
      VLerror( "Invalid $root item" );
    }
  | K_function error K_endfunction
    {
      VLerror( "Syntax error in function description" );
    }
  | enumeration list_of_names ';'
    {
      if( parse_mode ) {
        str_link* strl = $2;
        while( strl != NULL ) {
          db_add_signal( strl->str, SSUPPL_TYPE_DECL_REG, &curr_prange, NULL, curr_signed, FALSE, strl->suppl, strl->suppl2, TRUE );
          strl = strl->next;
        }
        str_link_delete_list( $2 );
      } else {
        generator_flush_all();
      }
    }
  ;

module
  : attribute_list_opt module_start IDENTIFIER 
    {
      if( parse_mode ) {
        db_add_module( $3, @2.text, @2.first_line );
      } else {
        db_find_and_set_curr_funit( $3, FUNIT_MODULE );
        generator_init_funit( db_get_curr_funit() );
      }
      free_safe( $3, (strlen( $3 ) + 1) );
    }
    module_parameter_port_list_opt
    module_port_list_opt ';'
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
    module_item_list_opt K_endmodule
    {
      if( parse_mode ) {
        db_end_module( @10.first_line );
      }
    }
  | attribute_list_opt K_module IGNORE I_endmodule
  | attribute_list_opt K_macromodule IGNORE I_endmodule
  ;

module_start
  : K_module
  | K_macromodule
  ;

module_parameter_port_list_opt
  : '#'
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Pre-module parameter declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
          ignore_mode++;
        }
      }
    }
    '(' module_parameter_port_list ')'
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          ignore_mode--;
        }
      }
    }
  |
  ;

module_parameter_port_list
  : K_parameter signed_opt range_opt parameter_assign
    {
      curr_prange.clear = TRUE;
    }
  | module_parameter_port_list ',' parameter_assign
  | module_parameter_port_list ',' K_parameter signed_opt range_opt parameter_assign
    {
      curr_prange.clear = TRUE;
    }
  ;

module_port_list_opt
  : '(' list_of_ports ')'
  | '(' list_of_port_declarations ')'
    {
      if( parse_mode ) {
        if( $2 != NULL ) {
          parser_dealloc_sig_range( $2->prange, TRUE );
          parser_dealloc_sig_range( $2->urange, TRUE );
          free_safe( $2, sizeof( port_info ) );
        }
      }
    }
  |
  ;

  /* Verilog-2001 syntax for declaring all ports within module port list */
list_of_port_declarations
  : port_declaration
    {
      $$ = $1;
    }
  | list_of_port_declarations ',' port_declaration
    {
      if( parse_mode ) {
        if( $1 != NULL ) {
          parser_dealloc_sig_range( $1->prange, TRUE );
          parser_dealloc_sig_range( $1->urange, TRUE );
          free_safe( $1, sizeof( port_info ) );
        }
        $$ = $3;
      } else {
        $$ = NULL;
      }
    }
  | list_of_port_declarations ',' IDENTIFIER
    {
      if( parse_mode ) {
        if( $1 != NULL ) {
          db_add_signal( $3, $1->type, $1->prange, $1->urange, curr_signed, curr_mba, @3.first_line, @3.first_column, TRUE );
        }
        free_safe( $3, (strlen( $3 ) + 1) );
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  ;

  /* Handles Verilog-2001 port of type:  input wire [m:l] <list>; */
port_declaration
  : attribute_list_opt port_type net_type_sign_range_opt IDENTIFIER
    { 
      if( parse_mode ) {
        $$ = parser_create_inline_port( $4, curr_sig_type, @4.first_line, @4.first_column );
      } else {
        $$ = NULL;
      }
    }
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER
    {
      if( parse_mode ) {
        $$ = parser_create_inline_port( $6, SSUPPL_TYPE_OUTPUT_REG, @6.first_line, @6.first_column );
      } else {
        $$ = NULL;
      }
    }
  /* We just need to parse the static register assignment as this signal will get its value from the dumpfile */
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER '=' ignore_more static_expr ignore_less
    {
      if( parse_mode ) {
        $$ = parser_create_inline_port( $6, SSUPPL_TYPE_OUTPUT_REG, @6.first_line, @6.first_column );
      } else {
        $$ = NULL;
      }
    }
  | attribute_list_opt port_type net_type_sign_range_opt error
    {
      if( parse_mode ) {
        $$ = parser_handle_inline_port_error();
      } else {
        $$ = NULL;
      }
    }
  | attribute_list_opt K_output var_type signed_opt range_opt error
    {
      if( parse_mode ) {
        $$ = parser_handle_inline_port_error();
      } else {
        $$ = NULL;
      }
    }

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
  | '.' IDENTIFIER '(' port_reference ')'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | '{' port_reference_list '}'
  | '.' IDENTIFIER '(' '{' port_reference_list '}' ')'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  ;

port_reference
  : IDENTIFIER
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER '[' ignore_more static_expr ':' static_expr ignore_less ']'
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER '[' ignore_more static_expr ignore_less ']'
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER '[' error ']'
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  ;

port_reference_list
  : port_reference
  | port_reference_list ',' port_reference
  ;

number
  : DEC_NUMBER
    {
      if( parse_mode ) {
        $$ = parser_create_simple_number( $1 );
      }
    }
  | BASE_NUMBER
    { 
      if( parse_mode ) {
        $$ = parser_create_simple_number( $1 );
      }
    }
  | DEC_NUMBER BASE_NUMBER
    {
      if( parse_mode ) {
        $$ = parser_create_complex_number( $1, $2 );
      }
    }
  ;

static_expr_port_list
  : static_expr_port_list ',' static_expr
    {
      if( parse_mode ) {
        $$ = parser_append_se_port_list( $1, $3, @1.first_line, @1.first_column, @3.first_line, @3.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | static_expr
    {
      if( parse_mode ) {
        $$ = parser_create_se_port_list( $1, @1.first_line, @1.first_column, @1.last_column );
      } else {
        $$ = NULL;
      }
    }
  ;

static_unary_op
  : '~'    { $$ = EXP_OP_UINV;  }
  | '!'    { $$ = EXP_OP_UNOT;  }
  | '&'    { $$ = EXP_OP_UAND;  }
  | '|'    { $$ = EXP_OP_UOR;   }
  | '^'    { $$ = EXP_OP_UXOR;  }
  | K_NAND { $$ = EXP_OP_UNAND; }
  | K_NOR  { $$ = EXP_OP_UNOR;  }
  | K_NXOR { $$ = EXP_OP_UNXOR; }
  ;

static_expr
  : static_expr_primary
    {
      if( parse_mode ) {
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  | '+' static_expr_primary %prec UNARY_PREC
    {
      if( parse_mode ) {
        static_expr* tmp = $2;
        if( tmp != NULL ) {
          if( tmp->exp == NULL ) {
            tmp->num = 0 + tmp->num;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '-' static_expr_primary %prec UNARY_PREC
    {
      if( parse_mode ) {
        static_expr* tmp = $2;
        if( tmp != NULL ) {
          if( tmp->exp == NULL ) {
            tmp->num = 0 - tmp->num;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | static_unary_op static_expr_primary %prec UNARY_PREC
    {
      if( parse_mode ) {
        $$ = parser_create_unary_se( $2, $1, @1.first_line, @1.first_column, @1.last_column );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '^' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_XOR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '*' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_MULTIPLY, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '/' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_DIVIDE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '%' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_MOD, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '+' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_ADD, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '-' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_SUBTRACT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '&' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_AND, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '|' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_OR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_NOR static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_NOR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_NAND static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_NAND, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_NXOR static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_NXOR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_LS static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_LSHIFT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_RS static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_RSHIFT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_GE static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_GE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_LE static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_LE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_EQ static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_EQ, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '>' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_GT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr '<' static_expr
    {
      if( parse_mode ) {
        $$ = static_expr_gen( $3, $1, EXP_OP_LT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      } else {
        $$ = NULL;
      }
    }
  | static_expr K_POW static_expr
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Exponential power operation found in block that is specified to not allow Verilog-2001 syntax" );
          static_expr_dealloc( $1, TRUE );
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        } else {
          $$ = static_expr_gen( $3, $1, EXP_OP_EXPONENT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        }
      } else {
        $$ = NULL;
      }
    }
  ;

static_expr_primary
  : number
    { PROFILE(PARSER_STATIC_EXPR_PRIMARY_A);
      if( parse_mode ) {
        static_expr* tmp;
        if( (ignore_mode == 0) && ($1.vec != NULL) ) {
          tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
          if( vector_is_unknown( $1.vec ) ) {
            Try {
              tmp->exp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
              tmp->exp->suppl.part.base = $1.base;
            } Catch_anonymous {
              error_count++;
            }
            vector_dealloc( tmp->exp->value );
            tmp->exp->value = $1.vec;
          } else {
            tmp->num = vector_to_int( $1.vec );
            tmp->exp = NULL;
            vector_dealloc( $1.vec );
          }
          $$ = tmp;
        } else {
          vector_dealloc( $1.vec );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | REALTIME
    {
      if( parse_mode ) {
        static_expr* tmp;
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
          Try {
            tmp->exp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          vector_dealloc( tmp->exp->value );
          tmp->exp->value = $1;
          $$ = tmp;
        } else {
          vector_dealloc( $1 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | IDENTIFIER
    { PROFILE(PARSER_STATIC_EXPR_PRIMARY_B);
      if( parse_mode ) {
        static_expr* tmp;
        if( ignore_mode == 0 ) {
          tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
          tmp->num = -1;
          Try {
            tmp->exp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          } Catch_anonymous {
            error_count++;
          }
          $$ = tmp;
        } else {
          $$ = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | '(' static_expr ')'
    {
      if( parse_mode ) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | IDENTIFIER '(' static_expr_port_list ')'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $$ = static_expr_gen( NULL, $3, EXP_OP_FUNC_CALL, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
        } else {
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER '[' static_expr ']'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) { 
          $$ = static_expr_gen( NULL, $3, EXP_OP_SBIT_SEL, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
        } else {
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | S_ignore
    {
      if( parse_mode ) {
        stmt_blk_specify_removal_reason( LOGIC_RM_SYSFUNC, @1.text, @1.first_line, __FILE__, __LINE__ );
      }
      $$ = NULL;
    }
  | S_allow
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_se( EXP_OP_NOOP, 0, 0, 0 );
      } else {
        $$ = NULL;
      }
    }
  | S_random
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_se( EXP_OP_SRANDOM, @1.first_line, @1.first_column, @1.last_column );
      } else {
        $$ = NULL;
      }
    }
  | S_urandom
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_se( EXP_OP_SURANDOM, @1.first_line, @1.first_column, @1.last_column );
      } else {
        $$ = NULL;
      }
    }
  ;

unary_op
  : '~'     { $$ = EXP_OP_UINV;   }
  | '!'     { $$ = EXP_OP_UNOT;   }
  | '&'     { $$ = EXP_OP_UAND;   }
  | '|'     { $$ = EXP_OP_UOR;    }
  | '^'     { $$ = EXP_OP_UXOR;   }
  | K_NAND  { $$ = EXP_OP_UNAND;  }
  | K_NOR   { $$ = EXP_OP_UNOR;   }
  | K_NXOR  { $$ = EXP_OP_UNXOR;  }
  ;

expression
  : expr_primary
    {
      $$ = $1;
    }
  | '+' expr_primary %prec UNARY_PREC
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $2->value->suppl.part.is_signed = 1;
          $$ = $2;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '-' expr_primary %prec UNARY_PREC
    {
      if( parse_mode ) {
        if( ($$ = parser_create_unary_exp( $2, EXP_OP_NEGATE, @1.first_line, @1.first_column, @1.last_column )) != NULL ) {
          $$->value->suppl.part.is_signed = 1;
        }
      } else {
        $$ = NULL;
      }
    }
  | unary_op expr_primary %prec UNARY_PREC
    {
      if( parse_mode ) {
        $$ = parser_create_unary_exp( $2, $1, @1.first_line, @1.first_column, @1.last_column );
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
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_XOR, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '*' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_MULTIPLY, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '/' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_DIVIDE, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '%' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_MOD, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '+' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_ADD, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '-' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_SUBTRACT, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '&' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_AND, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '|' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_OR, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_NAND expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_NAND, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_NOR expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_NOR, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_NXOR expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_NXOR, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '<' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_LT, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression '>' expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_GT, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_LS expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_LSHIFT, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_RS expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_RSHIFT, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_EQ expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_EQ, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_CEQ expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_CEQ, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_LE expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_LE, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_GE expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_GE, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_NE expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_NE, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_CNE expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_CNE, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_LOR expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_LOR, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_LAND expression
    {
      if( parse_mode ) {
        $$ = parser_create_binary_exp( $1, $3, EXP_OP_LAND, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | expression K_POW expression
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Exponential power operator found in block that is specified to not allow Verilog-2001 syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          $$ = parser_create_binary_exp( $1, $3, EXP_OP_EXPONENT, @1.first_line, @1.first_column, @3.last_column );
        }
      } else {
        $$ = NULL;
      }
    }
  | expression K_LSS expression
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Arithmetic left shift operation found in block that is specified to not allow Verilog-2001 syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          $$ = parser_create_binary_exp( $1, $3, EXP_OP_ALSHIFT, @1.first_line, @1.first_column, @3.last_column );
        }
      } else {
        $$ = NULL;
      }
    }
  | expression K_RSS expression
    {
      if( parse_mode ) {
        if( !parser_check_generation( GENERATION_2001 ) ) {
          VLerror( "Arithmetic right shift operation found in block that is specified to not allow Verilog-2001 syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          $$ = parser_create_binary_exp( $1, $3, EXP_OP_ARSHIFT, @1.first_line, @1.first_column, @3.last_column );
        }
      } else {
        $$ = NULL;
      }
    }
  | expression '?' expression ':' expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($5 != NULL) ) {
          Try {
            expression* csel = db_create_expression( $5, $3, EXP_OP_COND_SEL, lhs_mode, $3->line, @1.first_column, (@3.last_column - 1), NULL );
            $$ = db_create_expression( csel, $1, EXP_OP_COND, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $5, FALSE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

syscall_wo_parms_op
  : S_random          { $$ = EXP_OP_SRANDOM;  }
  | S_urandom         { $$ = EXP_OP_SURANDOM; }
  | S_time            { $$ = EXP_OP_STIME;    }
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
  ;

syscall_w_parms_op_64
  : S_bitstoreal      { $$ = EXP_OP_SB2R;   }
  | S_itor            { $$ = EXP_OP_SI2R;   }
  ;

syscall_w_parms_op_32
  : S_bitstoshortreal { $$ = EXP_OP_SB2SR;  }
  ;

pre_op_and_assign_op
  : K_INC             { $$ = EXP_OP_PINC; }
  | K_DEC             { $$ = EXP_OP_PDEC; }
  ;

post_op_and_assign_op
  : K_INC             { $$ = EXP_OP_IINC; }
  | K_DEC             { $$ = EXP_OP_IDEC; }
  ;

expr_primary
  : number
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1.vec != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            $$->suppl.part.base = $1.base;
            vector_dealloc( $$->value );
            $$->value = $1.vec;
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          vector_dealloc( $1.vec );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | REALTIME
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            vector_dealloc( $$->value );
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
  | STRING
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            $$->suppl.part.base = $1.base;
            vector_dealloc( $$->value );
            $$->value = $1.vec;
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          vector_dealloc( $1.vec );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | identifier
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | identifier post_op_and_assign_op
    {
      if( parse_mode ) {
        $$ = parser_create_op_and_assign_exp( $1, $2, @1.first_line, @1.first_column, @1.last_column, @2.last_column );
      } else {
        $$ = NULL;
      }
    }
  | pre_op_and_assign_op identifier
    {
      if( parse_mode ) {
        $$ = parser_create_op_and_assign_exp( $2, $1, @1.first_line, @1.first_column, @1.last_column, @2.last_column );
      } else {
        $$ = NULL;
      }
    }
  | S_ignore
    {
      if( parse_mode ) {
        stmt_blk_specify_removal_reason( LOGIC_RM_SYSFUNC, @1.text, @1.first_line, __FILE__, __LINE__ );
      }
      $$ = NULL;
    }
  | S_allow
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_exp( EXP_OP_NOOP, 0, 0, 0 );
      } else {
        $$ = NULL;
      }
    }
  | syscall_wo_parms_op
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_exp( $1, @1.first_line, @1.first_column, @1.last_column );
      } else {
        $$ = NULL;
      }
    }
  | identifier index_expr
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($2 != NULL) ) {
          db_bind_expr_tree( $2, $1 );
          $2->line = @1.first_line;
          $2->col  = ((@1.first_column & 0xffff) << 16) | ($2->col & 0xffff);
          $$ = $2;
        } else {
          expression_dealloc( $2, FALSE );
          $$ = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | identifier index_expr post_op_and_assign_op
    {
      if( parse_mode ) {
        $$ = parser_create_op_and_assign_w_dim_exp( $1, $3, $2, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | pre_op_and_assign_op identifier index_expr
    {
      if( parse_mode ) {
        $$ = parser_create_op_and_assign_w_dim_exp( $2, $1, $3, @1.first_line, @1.first_column, @3.last_column );
      } else {
        $$ = NULL;
      }
    }
  | identifier '(' expression_port_list ')'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL ) ) {
          Try {
            $$ = db_create_expression( NULL, $3, EXP_OP_FUNC_CALL, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | S_ignore '(' ignore_more expression_port_list ignore_less ')'
    {
      if( parse_mode ) {
        stmt_blk_specify_removal_reason( LOGIC_RM_SYSFUNC, @1.text, @1.first_line, __FILE__, __LINE__ );
      }
      $$ = NULL;
    }
  | S_allow '(' ignore_more expression_port_list ignore_less ')'
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_exp( EXP_OP_NOOP, 0, 0, 0 );
      } else {
        $$ = NULL;
      }
    }
  | syscall_w_parms_op '(' expression_systask_list ')'
    {
      if( parse_mode ) {
        $$ = parser_create_syscall_w_params_exp( $1, $3, @1.first_line, @1.first_column, @4.last_column );
      } else {
        $$ = NULL;
      }
    }
  | syscall_w_parms_op_64 '(' expression_systask_list ')'
    {
      if( parse_mode ) {
        if( ($$ = parser_create_syscall_w_params_exp( $1, $3, @1.first_line, @1.first_column, @4.last_column )) != NULL ) {
          $$->value->suppl.part.data_type = VDATA_R64;
        }
      } else {
        $$ = NULL;
      }
    }
  | syscall_w_parms_op_32 '(' expression_systask_list ')'
    {
      if( parse_mode ) {
        if( ($$ = parser_create_syscall_w_params_exp( $1, $3, @1.first_line, @1.first_column, @4.last_column )) != NULL ) {
          $$->value->suppl.part.data_type = VDATA_R32;
        }
      } else {
        $$ = NULL;
      }
    }
  | '(' expression ')'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $2->suppl.part.parenthesis = 1;
          $$ = $2;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | '{' expression_list '}'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, NULL, EXP_OP_CONCAT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
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
  | '{' expression '{' expression_list '}' '}'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
          Try {
            $$ = db_create_expression( $4, $2, EXP_OP_EXPAND, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
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
  ;

  /* Expression lists are used in concatenations and parameter overrides */
expression_list
  : expression_list ',' expression
    { PROFILE(PARSER_EXPRESSION_LIST_A);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( param_mode == 0 ) {
            if( $3 != NULL ) {
              Try {
                $$ = db_create_expression( $3, $1, EXP_OP_LIST, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
              } Catch_anonymous {
                error_count++;
                $$ = NULL;
              }
            } else {
              $$ = $1;
            }
          } else {
            if( $3 != NULL ) {
              param_oride* po = (param_oride*)malloc_safe( sizeof( param_oride ) );
              po->name = NULL;
              po->expr = $3;
              po->next = NULL;
              if( param_oride_head == NULL ) {
                param_oride_head = param_oride_tail = po;
              } else {
                param_oride_tail->next = po;
                param_oride_tail       = po;
              }
            }
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      PROFILE_END;
    }
  | expression
    { PROFILE(PARSER_EXPRESSION_LIST_B);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( param_mode == 0 ) {
            $$ = $1;
          } else {
            if( $1 != NULL ) {
              param_oride* po = (param_oride*)malloc_safe( sizeof( param_oride ) );
              po->name = NULL;
              po->expr = $1;
              po->next = NULL;
              if( param_oride_head == NULL ) {
                param_oride_head = param_oride_tail = po;
              } else {
                param_oride_tail->next = po;
                param_oride_tail       = po;
              }
            }
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
      PROFILE_END;
    }
  |
    { PROFILE(PARSER_EXPRESSION_LIST_C);
      if( parse_mode ) {
        if( (ignore_mode == 0) && (param_mode == 1) ) {
          param_oride* po = (param_oride*)malloc_safe( sizeof( param_oride ) );
          po->name = NULL;
          po->expr = NULL;
          po->next = NULL;
          if( param_oride_head == NULL ) {
            param_oride_head = param_oride_tail = po;
          } else {
            param_oride_tail->next = po;
            param_oride_tail       = po;
          }
        }
        $$ = NULL;
      } else {
        $$ = NULL;
      }
      PROFILE_END;
    }
  | expression_list ','
    { PROFILE(PARSER_EXPRESSION_LIST_D);
      if( parse_mode ) {
        if( (ignore_mode == 0) && (param_mode == 1) ) {
          param_oride* po = (param_oride*)malloc_safe( sizeof( param_oride ) );
          po->name = NULL;
          po->expr = NULL;
          po->next = NULL;
          if( param_oride_head == NULL ) {
            param_oride_head = param_oride_tail = po;
          } else {
            param_oride_tail->next = po;
            param_oride_tail       = po;
          }
        }
        $$ = $1;
      } else {
        $$ = NULL;
      }
      PROFILE_END;
    }
  ;

  /* Expression port lists are used in function/task calls */
expression_port_list
  : expression_port_list ',' expression
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $3 != NULL ) {
            Try {
              expression* tmp = db_create_expression( $3, NULL, EXP_OP_PASSIGN, lhs_mode, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
              $$ = db_create_expression( tmp, $1, EXP_OP_PLIST, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
            } Catch_anonymous {
              error_count++;
              $$ = NULL;
            }
          } else {
            $$ = $1;
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
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_expression( $1, NULL, EXP_OP_PASSIGN, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
    }
  ;

  /* Expression systask lists are used in system task calls */
expression_systask_list
  : expression_systask_list ',' expression
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $3 != NULL ) {
            Try {
              expression* tmp = db_create_expression( $3, NULL, EXP_OP_SASSIGN, lhs_mode, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
              $$ = db_create_expression( tmp, $1, EXP_OP_PLIST, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( $3, FALSE );
              error_count++;
              $$ = NULL;
            }
          } else {
            $$ = $1;
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
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_expression( $1, NULL, EXP_OP_SASSIGN, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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

begin_end_id
  : ':' IDENTIFIER
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $$ = $2;
        } else {
          free_safe( $2, (strlen( $2 ) + 1) );
          $$ = NULL;
        }
      } else {
        generator_flush_work_code();
        free_safe( $2, (strlen( $2 ) + 1) );
        $$ = NULL;
      }
    }
  |
    {
      if( parse_mode ) {
        $$ = db_create_unnamed_scope();
      } else {
        $$ = NULL;
      }
    }
  ;

identifier
  : IDENTIFIER
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          $$ = $1;
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
        free_safe( $1, (strlen( $1 ) + 1) );
        $$ = NULL;
      }
    }
  | identifier '.' IDENTIFIER
    { PROFILE(PARSER_IDENTIFIER_A);
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          unsigned int len = strlen( $1 ) + strlen( $3 ) + 2;
          char*        str = (char*)malloc_safe( len );
          unsigned int rv  = snprintf( str, len, "%s.%s", $1, $3 );
          assert( rv < len );
          $$ = str;
        } else {
          $$ = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  ;

list_of_variables
  : IDENTIFIER
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_add_signal( $1, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @1.first_line, @1.first_column, curr_handled );
        }
      }
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER { curr_packed = FALSE; } range
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          curr_packed = TRUE;
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "Unpacked array specified for net type in block that was specified to not allow SystemVerilog syntax" );
          } else {
            db_add_signal( $1, curr_sig_type, &curr_prange, &curr_urange, curr_signed, curr_mba, @1.first_line, @1.first_column, curr_handled );
          }
        }
      }
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | list_of_variables ',' IDENTIFIER
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_add_signal( $3, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @3.first_line, @3.first_column, curr_handled );
        }
      }
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  | list_of_variables ',' IDENTIFIER range
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "Unpacked array specified for net type in block that was specified to not allow SystemVerilog syntax" );
          } else {
            db_add_signal( $3, curr_sig_type, &curr_prange, &curr_urange, curr_signed, curr_mba, @3.first_line, @3.first_column, curr_handled );
          }
        }
      }
      free_safe( $3, (strlen( $3 ) + 1) );
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
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          /* We will treat primitives like regular modules */
          db_add_module( $2, @1.text, @1.first_line );
          db_end_module( @10.first_line );
        }
      }
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | K_primitive IGNORE K_endprimitive
  ;

udp_port_list
  : IDENTIFIER
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | udp_port_list ',' IDENTIFIER
    {
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  ;

udp_port_decls
  : udp_port_decl
  | udp_port_decls udp_port_decl
  ;

udp_port_decl
  : K_input ignore_more list_of_variables ignore_less ';'
  | K_output IDENTIFIER ';'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | K_reg IDENTIFIER ';'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
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
      free_safe( $2, (strlen( $2 ) + 1) );
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
              expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
              expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
              expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
              if( generate_varname == NULL ) {
                generate_varname = $1;
              } else {
                free_safe( $1, (strlen( $1 ) + 1) );
              }
            } Catch_anonymous {
              error_count++;
            }
            $$ = expr;
          } else {
            free_safe( $1, (strlen( $1 ) + 1) );
            $$ = NULL;
          }
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        }
      } else {
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
              expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
              expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
              expr = db_create_expression( expr, expl, $2, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
              if( generate_varname == NULL ) {
                generate_varname = $1;
              } else {
                free_safe( $1, (strlen( $1 ) + 1) );
              }
            } Catch_anonymous {
              error_count++;
            }
            $$ = expr;
          } else {
            free_safe( $1, (strlen( $1 ) + 1) );
            $$ = NULL;
          }
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
          static_expr_dealloc( $3, TRUE );
          $$ = NULL;
        }
      } else {
        free_safe( $1, (strlen( $1 ) + 1) );
        $$ = NULL;
      }
    }
  | IDENTIFIER post_op_and_assign_op
    {
      if( parse_mode ) {
        expression* expr = NULL;
        if( ignore_mode == 0 ) {
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
            expr = db_create_expression( NULL, expr, $2, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
            if( generate_varname == NULL ) {
              generate_varname = $1;
            } else {
              free_safe( $1, (strlen( $1 ) + 1) );
            }
          } Catch_anonymous {
            error_count++;
          }
          $$ = expr;
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
        free_safe( $1, (strlen( $1 ) + 1) );
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

  /* Similar to module_item but is more restrictive */
generate_item
  : module_item
    {
      if( parse_mode ) {
        $$ = db_get_curr_gen_block();
      } else {
        $$ = NULL;
      }
    }
  | K_begin generate_item_list_opt K_end
    {
      if( parse_mode ) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | K_begin ':' IDENTIFIER
    {
      if( parse_mode ) {
        generate_expr_mode++;
        if( ignore_mode == 0 ) {
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $3, @3.text, @3.first_line ) ) {
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
      }
      free_safe( $3, (strlen( $3 ) + 1) );
    }
    generate_item_list_opt K_end
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @6.first_line );
        } else {
          ignore_mode--;
        }
        db_gen_item_connect_true( save_gi_tail->gi, $5 );
        $$ = save_gi_tail->gi;
        gitem_link_remove( save_gi_tail->gi, &save_gi_head, &save_gi_tail );
      } else {
        $$ = NULL;
      }
    }
  | K_for
    {
      if( parse_mode ) {
        generate_expr_mode++;
      }
    }
    '(' generate_passign ';' static_expr ';' generate_passign ')' K_begin ':' IDENTIFIER
    {
      if( parse_mode ) {
        generate_for_mode++;
        if( ignore_mode == 0 ) {
          Try {
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $12, @12.text, @12.first_line ) ) {
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
      }
      free_safe( $12, (strlen( $12 ) + 1) );
    }
    generate_item_list_opt K_end
    {
      if( parse_mode ) {
        gen_item*   gi1;
        gen_item*   gi2;
        gen_item*   gi3;
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @15.first_line );
        } else {
          ignore_mode--;
        }
        generate_for_mode--;
        generate_expr_mode++;
        if( (ignore_mode == 0) && ($4 != NULL) && ($6 != NULL) && ($8 != NULL) && ($14 != NULL) ) {
          block_depth++;
          /* Create first statement */
          db_add_expression( $4 );
          gi1 = db_get_curr_gen_block();
          /* Create second statement and attach it to the first statement */
          db_add_expression( db_create_expr_from_static( $6, @6.first_line, @6.first_column, (@6.last_column - 1)) );
          gi2 = db_get_curr_gen_block();
          db_gen_item_connect( gi1, gi2 );
          /* Add genvar to the new scope */
          /* Connect next_true of gi2 to the new scope */
          db_gen_item_connect_true( gi2, save_gi_tail->gi );
          /* Connect the generate block to the new scope */
          db_gen_item_connect_true( save_gi_tail->gi, $14 );
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
        $$ = NULL;
      }
    }
  | K_if inc_gen_expr_mode '(' static_expr ')' dec_gen_expr_mode inc_block_depth generate_item dec_block_depth %prec less_than_K_else
    {
      if( parse_mode ) {
        expression* expr;
        gen_item*   gi1;
        if( (ignore_mode == 0) && ($4 != NULL) && ($8 != NULL) ) {
          generate_expr_mode++;
          expr = db_create_expr_from_static( $4, @4.first_line, @4.first_column, (@4.last_column - 1) );
          Try {
            expr = db_create_expression( expr, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          db_add_expression( expr );
          gi1 = db_get_curr_gen_block();
          db_gen_item_connect_true( gi1, $8 );
          generate_expr_mode--;
          $$ = gi1;
        } else {
          static_expr_dealloc( $4, TRUE );
          /* TBD - Deallocate generate block */
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | K_if inc_gen_expr_mode '(' static_expr ')' dec_gen_expr_mode inc_block_depth generate_item dec_block_depth K_else generate_item
    {
      if( parse_mode ) {
        expression* expr;
        gen_item*   gi1;
        if( (ignore_mode == 0) && ($4 != NULL) && ($8 != NULL) && ($11 != NULL) ) {
          generate_expr_mode++;
          expr = db_create_expr_from_static( $4, @4.first_line, @4.first_column, (@4.last_column - 1) );
          Try {
            expr = db_create_expression( expr, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          db_add_expression( expr );
          gi1 = db_get_curr_gen_block();
          db_gen_item_connect_true( gi1, $8 );
          db_gen_item_connect_false( gi1, $11 );
          generate_expr_mode--;
          $$ = gi1;
        } else {
          static_expr_dealloc( $4, TRUE );
          gen_item_dealloc( db_get_curr_gen_block(), TRUE );
          gen_item_dealloc( $8, TRUE );
          gen_item_dealloc( $11, TRUE );
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | K_case inc_gen_expr_mode '(' static_expr ')' inc_block_depth generate_case_items dec_block_depth dec_gen_expr_mode K_endcase
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
          c_expr = db_create_expr_from_static( $4, @4.first_line, @4.first_column, (@4.last_column - 1) );
          while( c_stmt != NULL ) {
            Try {
              if( c_stmt->expr != NULL ) {
                expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASE, lhs_mode, c_stmt->line, 0, 0, NULL );
              } else {
                expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
              }
            } Catch_anonymous {
              error_count++;
              break;
            }
            db_add_expression( expr );
            stmt = db_get_curr_gen_block();
            db_gen_item_connect_true( stmt, c_stmt->gi );
            db_gen_item_connect_false( stmt, last_stmt );
            if( stmt != NULL ) {
              last_stmt = stmt;
            }
            tc_stmt   = c_stmt;
            c_stmt    = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_gitem ) );
          }
          if( stmt != NULL ) {
            stmt->elem.expr->suppl.part.owned = 1;
          }
          generate_expr_mode--;
          $$ = stmt;
        } else {
          static_expr_dealloc( $4, FALSE );
          while( c_stmt != NULL ) {
            expression_dealloc( c_stmt->expr, FALSE );
            // db_remove_statement( c_stmt->stmt );
            /* statement_dealloc_recursive( c_stmt->stmt ); */
            tc_stmt = c_stmt;
            c_stmt  = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_gitem ) );
          }
          $$ = NULL;
        }
      } else {
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
          cstmt->gi = $4;
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
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  | module_item
    {
      if( !parse_mode ) {
        generator_flush_all();
      }
    }
  ;

module_item
  : attribute_list_opt
    net_type signed_opt range_opt list_of_variables ';'
  | attribute_list_opt
    net_type signed_opt range_opt net_decl_assigns ';'
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
        /* TBD - Need to add code generation stuff here */
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
  | attribute_list_opt port_type range_opt
    {
      if( parse_mode ) {
        curr_mba     = FALSE;
        curr_signed  = FALSE;
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
      }
    }
  | attribute_list_opt port_type range_opt error ';'
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
      }
    }
  | attribute_list_opt gatetype gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      }
    }
  | attribute_list_opt gatetype delay3 gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $4 );
      }
    }
  | attribute_list_opt gatetype drive_strength gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $4 );
      }
    }
  | attribute_list_opt gatetype drive_strength delay3 gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $5 );
      }
    }
  | attribute_list_opt K_pullup gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      }
    }
  | attribute_list_opt K_pulldown gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $3 );
      }
    }
  | attribute_list_opt K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $6 );
      }
    }
  | attribute_list_opt K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      if( parse_mode ) {
        str_link_delete_list( $6 );
      }
    }
  | block_item_decl
  | attribute_list_opt K_defparam defparam_assign_list ';'
    {
      if( parse_mode ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Defparam found but not used, file: %s, line: %u.  Please use -P option to specify",
                                    obf_file( @1.text ), @1.first_line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
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
      }
    }
    list_of_variables ';'
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
              (void)db_add_instance( curr->str, $2, FUNIT_MODULE, curr->range );
              curr = curr->next;
            }
          } Catch_anonymous {
            error_count++;
          }
          str_link_delete_list( tmp );
          param_oride_head = NULL;
          param_oride_tail = NULL;
          free_safe( $2, (strlen( $2 ) + 1) );
        } else {
          free_safe( $2, (strlen( $2 ) + 1) );
        }
      }
    }
  | attribute_list_opt
    K_assign drive_strength_opt { ignore_mode++; } delay3_opt { ignore_mode--; } assign_list ';'
    {
      if( !parse_mode ) {
//        generator_insert_comb_cov();
      }
    }
  | attribute_list_opt
    K_always inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( stmt != NULL ) {
          if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
            stmt->suppl.part.head = 1;
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      } else {
        generator_flush_hold_code();
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
        generator_flush_work_code();
        generator_add_to_hold_code( " begin" );
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
                slist = db_create_expression( slist, NULL, EXP_OP_ALWAYS_COMB, lhs_mode, @2.first_line, @2.first_column, (@2.last_column - 1), NULL );
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
                  db_add_statement( stmt, stmt );
                } else {
                  db_remove_statement( stmt );
                }
              }
            }
          }
        }
      } else {
        generator_flush_work_code();
        generator_add_to_hold_code( " end" );
        generator_flush_hold_code();
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
        generator_flush_work_code();
        generator_add_to_hold_code( " begin" );
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
                slist = db_create_expression( slist, NULL, EXP_OP_ALWAYS_LATCH, lhs_mode, @2.first_line, @2.first_column, (@2.last_column - 1), NULL );
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
                  db_add_statement( stmt, stmt );
                } else {
                  db_remove_statement( stmt );
                }
              }
            }
          }
        }
      } else {
        generator_flush_work_code();
        generator_add_to_hold_code( " end" );
        generator_flush_hold_code();
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
        generator_flush_work_code();
        generator_add_to_hold_code( " begin" );
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
              db_add_statement( stmt, stmt );
            } else {
              db_remove_statement( stmt );
            }
          }
        }
      } else {
        generator_flush_work_code();
        generator_add_to_hold_code( " end" );
        generator_flush_hold_code();
      }
    }
  | attribute_list_opt
    K_initial inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( stmt != NULL ) {
          if( (info_suppl.part.excl_init == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
            stmt->suppl.part.head = 1;
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      } else {
        generator_flush_hold_code();
      }
    }
  | attribute_list_opt
    K_final inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        statement* stmt = $4;
        if( stmt != NULL ) {
          if( (info_suppl.part.excl_final == 0) && (stmt->exp->op != EXP_OP_NOOP) ) {
            stmt->suppl.part.head  = 1;
            stmt->suppl.part.final = 1;
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      } else {
        generator_flush_hold_code();
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
            if( !db_add_function_task_namedblock( ($3 ? FUNIT_ATASK : FUNIT_TASK), $4, @4.text, @4.first_line ) ) {
              ignore_mode++;
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        }
        free_safe( $4, (strlen( $4 ) + 1) );
        generate_top_mode--;
      }
    }
    task_item_list_opt statement_or_null
    {
      if( parse_mode ) {
        statement* stmt = $8;
        if( ignore_mode == 0 ) {
          if( stmt == NULL ) {
            stmt = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @8.first_line, @8.first_column, (@8.last_column - 1), NULL ) );
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
        generator_flush_hold_code();
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
            if( db_add_function_task_namedblock( ($3 ? FUNIT_AFUNCTION : FUNIT_FUNCTION), $6, @6.text, @6.first_line ) ) {
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
        free_safe( $6, (strlen( $6 ) + 1) );
      }
    }
    function_item_list statement
    {
      if( parse_mode ) {
        statement* stmt = $10;
        if( (ignore_mode == 0) && (stmt != NULL) ) {
          stmt->suppl.part.head      = 1;
          stmt->suppl.part.is_called = 1;
          db_add_statement( stmt, stmt );
        }
      }
    }
    K_endfunction
    {
      if( parse_mode ) {
        generate_top_mode++;
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @12.first_line );
        } else {
          ignore_mode--;
        }
      } else {
        generator_flush_hold_code();
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
      }
    }
  | attribute_list_opt
    typedef_decl
  /* SystemVerilog assertion - we don't currently support these and I don't want to worry about how to parse them either */
  | attribute_list_opt
    IDENTIFIER ':' K_assert ';'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  /* SystemVerilog property - we don't currently support these but crudely parse them */
  | attribute_list_opt
    K_property K_endproperty

  /* SystemVerilog sequence - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_sequence K_endsequence
  /* SystemVerilog program block - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_program K_endprogram
  | KK_attribute '(' IDENTIFIER ',' STRING ',' STRING ')' ';'
    {
      free_safe( $3, (strlen( $3 ) + 1) );
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
            tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.first_column, (@2.last_column - 1), $2 );
          } Catch_anonymous {
            error_count++;
          }
          Try {
            tmp = db_create_expression( $4, tmp, EXP_OP_BASSIGN, FALSE, @2.first_line, @2.first_column, (@4.last_column - 1), NULL );
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
        $$ = NULL;
      }
      free_safe( $2, (strlen( $2 ) + 1) );
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
            tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @4.first_line, @4.first_column, (@4.last_column - 1), $4 );
          } Catch_anonymous {
            error_count++;
          }
          Try {
            tmp = db_create_expression( $6, tmp, EXP_OP_BASSIGN, FALSE, @4.first_line, @4.first_column, (@6.last_column - 1), NULL );
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
        $$ = NULL;
      }
      free_safe( $4, (strlen( $4 ) + 1) );
    }
  ;

passign
  : lpvalue '=' expression
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $3, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, TRUE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_ADD_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_SUB_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_MLT_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_DIV_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_MOD_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_AND_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_OR_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_XOR_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_LS_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_RS_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_ALS_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( $3, $1, EXP_OP_ARS_A, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( NULL, $1, EXP_OP_PINC, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
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
              tmp = db_create_expression( NULL, $1, EXP_OP_PDEC, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, FALSE, @1.first_line, @1.first_column );
        $$ = NULL;
      }
    }
  ;

statement
  : K_assign { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_flush_work_code();
      }
      $$ = NULL;
    }
  | K_deassign { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_flush_work_code();
      }
      $$ = NULL;
    }
  | K_force { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_flush_work_code();
      }
      $$ = NULL;
    }
  | K_release { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      if( !parse_mode ) {
        generator_flush_work_code();
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
            exp  = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
            exp  = db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            stmt = db_create_statement( exp );
            $$   = stmt;
          }
        }
      } else {
        generator_flush_hold_code();
        $$ = NULL;
      }
    }
  | K_fork inc_fork_depth fork_statement K_join
    { PROFILE(PARSER_STATEMENT_FORK_A);
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        char        back[4096];
        char        rest[4096];
        if( ignore_mode == 0 ) {
          db_end_function_task_namedblock( @4.first_line );
          if( $3 != NULL ) {
            scope_extract_back( $3->name, back, rest );
            exp  = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            exp->elem.funit      = $3;
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
        generator_flush_work_code();
        $$ = NULL;  /* TBD - Need to do something here */
      }
    }
  | K_disable identifier ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          expression* exp = NULL;
          Try {
            exp = db_create_expression( NULL, NULL, EXP_OP_DISABLE, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
          } Catch_anonymous {
            error_count++;
          }
          $$ = db_create_statement( exp );
        } else {
          $$ = NULL;
        } 
      } else {
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
        $$ = NULL;
      }
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | K_TRIGGER IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          expression* exp = NULL;
          Try {
            exp = db_create_expression( NULL, NULL, EXP_OP_TRIGGER, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
          } Catch_anonymous {
            error_count++;
          }
          $$ = db_create_statement( exp );
        } else {
          $$ = NULL;
        }
      } else {
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
        $$ = NULL;
      }
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | K_forever inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        if( $3 != NULL ) {
          expression* expr;
          statement*  stmt;
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_FOREVER, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          stmt = db_create_statement( expr );
          if( db_statement_connect( $3, $3 ) ) {
            db_connect_statement_false( stmt, $3 );
            $$ = stmt;
          } else {
            db_remove_statement( stmt );
            db_remove_statement( $3 );
            $$ = NULL;
          }
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | K_repeat '(' expression ')' inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) && ($6 != NULL) ) {
          vector*     vec  = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          expression* expr = NULL;
          statement*  stmt;
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            (void)vector_from_int( vec, 0x0 );
            assert( expr->value->value.ul == NULL );
            free_safe( expr->value, sizeof( vector ) );
            expr->value = vec;
          } Catch_anonymous {
            error_count++;
          }
          Try {
            expr = db_create_expression( $3, expr, EXP_OP_REPEAT, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, $6 );
          stmt->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for all statements connecting to stmt */
          assert( db_statement_connect( $6, stmt ) );
          $$ = stmt;
        } else {
          expression_dealloc( $3, FALSE );
          db_remove_statement( $6 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_case '(' expression ')' inc_block_depth case_items dec_block_depth K_endcase
    {
      if( parse_mode ) {
        expression*     expr      = NULL;
        expression*     c_expr    = $4;
        statement*      stmt      = NULL;
        statement*      last_stmt = NULL;
        case_statement* c_stmt    = $7;
        case_statement* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          while( c_stmt != NULL ) {
            Try {
              if( c_stmt->expr != NULL ) {
                expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASE, lhs_mode, c_stmt->line, 0, 0, NULL );
              } else {
                expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
              }
            } Catch_anonymous {
              error_count++;
            }
            stmt = db_create_statement( expr );
            db_connect_statement_true( stmt, c_stmt->stmt );
            db_connect_statement_false( stmt, last_stmt );
            if( stmt != NULL ) {
              last_stmt = stmt;
            }
            tc_stmt   = c_stmt;
            c_stmt    = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_statement ) );
          }
          if( stmt != NULL ) {
            stmt->exp->suppl.part.owned = 1;
          }
          $$ = stmt;
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
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_casex '(' expression ')' inc_block_depth case_items dec_block_depth K_endcase
    {
      if( parse_mode ) {
        expression*     expr      = NULL;
        expression*     c_expr    = $4;
        statement*      stmt      = NULL;
        statement*      last_stmt = NULL;
        case_statement* c_stmt    = $7;
        case_statement* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          while( c_stmt != NULL ) {
            Try {
              if( c_stmt->expr != NULL ) {
                expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEX, lhs_mode, c_stmt->line, 0, 0, NULL );
              } else {
                expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
              }
            } Catch_anonymous {
              error_count++;
            }
            stmt = db_create_statement( expr );
            db_connect_statement_true( stmt, c_stmt->stmt );
            db_connect_statement_false( stmt, last_stmt );
            if( stmt != NULL ) {
              last_stmt = stmt;
            }
            tc_stmt   = c_stmt;
            c_stmt    = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_statement ) );
          }
          if( stmt != NULL ) {
            stmt->exp->suppl.part.owned = 1;
          }
          $$ = stmt;
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
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_casez '(' expression ')' inc_block_depth case_items dec_block_depth K_endcase
    {
      if( parse_mode ) {
        expression*     expr      = NULL;
        expression*     c_expr    = $4;
        statement*      stmt      = NULL;
        statement*      last_stmt = NULL;
        case_statement* c_stmt    = $7;
        case_statement* tc_stmt;
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          while( c_stmt != NULL ) {
            Try {
              if( c_stmt->expr != NULL ) {
                expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEZ, lhs_mode, c_stmt->line, 0, 0, NULL );
              } else {
                expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
              }
            } Catch_anonymous {
              error_count++;
            }
            stmt = db_create_statement( expr );
            db_connect_statement_true( stmt, c_stmt->stmt );
            db_connect_statement_false( stmt, last_stmt );
            if( stmt != NULL ) {
              last_stmt = stmt;
            }
            tc_stmt   = c_stmt;
            c_stmt    = c_stmt->prev;
            free_safe( tc_stmt, sizeof( case_statement ) );
          }
          if( stmt != NULL ) {
            stmt->exp->suppl.part.owned = 1;
          }
          $$ = stmt;
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
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_case '(' expression ')' inc_block_depth error dec_block_depth K_endcase
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          expression_dealloc( $4, FALSE );
        }
      }
      VLerror( "Illegal case expression" );
      $$ = NULL;
    }
  | cond_specifier_opt K_casex '(' expression ')' inc_block_depth error dec_block_depth K_endcase
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          expression_dealloc( $4, FALSE );
        }
      }
      VLerror( "Illegal casex expression" );
      $$ = NULL;
    }
  | cond_specifier_opt K_casez '(' expression ')' inc_block_depth error dec_block_depth K_endcase
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          expression_dealloc( $4, FALSE );
        }
      }
      VLerror( "Illegal casez expression" );
      $$ = NULL;
    }
  | cond_specifier_opt K_if '(' expression ')' inc_block_depth statement_or_null dec_block_depth %prec less_than_K_else
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          expression* tmp = NULL;
          Try {
            tmp = db_create_expression( $4, NULL, EXP_OP_IF, FALSE, @2.first_line, @2.first_column, (@5.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          if( ($$ = db_create_statement( tmp )) != NULL ) {
            db_connect_statement_true( $$, $7 );
          }
        } else {
          db_remove_statement( $7 );
          $$ = NULL;
        }
      } else {
        generator_insert_line_cov( @2.first_line, @2.first_column );
        generator_flush_work_code();
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_if '(' expression ')' inc_block_depth statement_or_null dec_block_depth K_else inc_block_depth statement_or_null dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          expression* tmp = NULL;
          Try {
            tmp = db_create_expression( $4, NULL, EXP_OP_IF, FALSE, @2.first_line, @2.first_column, (@5.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          if( ($$ = db_create_statement( tmp )) != NULL ) {
            db_connect_statement_true( $$, $7 );
            db_connect_statement_false( $$, $11 );
          }
        } else {
          db_remove_statement( $7 );
          db_remove_statement( $11 );
          $$ = NULL;
        }
      } else {
        generator_insert_line_cov( @2.first_line, @2.first_column );
        generator_flush_work_code();
        $$ = NULL;  /* TBD */
      }
    }
  | cond_specifier_opt K_if '(' error ')' { ignore_mode++; } if_statement_error { ignore_mode--; }
    {
      VLerror( "Illegal conditional if expression" );
      $$ = NULL;
    }
  | K_for inc_for_depth '(' for_initialization ';' expression ';' passign ')' statement dec_for_depth
    { PROFILE(PARSER_STATEMENT_FOR_A);
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        statement*  stmt1 = $4;
        statement*  stmt2;
        statement*  stmt3 = $8;
        statement*  stmt4 = $10;
        char        back[4096];
        char        rest[4096];
        if( (ignore_mode == 0) && ($4 != NULL) && ($6 != NULL) && ($8 != NULL) && ($10 != NULL) ) {
          block_depth++;
          stmt2 = db_create_statement( $6 );
          assert( db_statement_connect( stmt1, stmt2 ) );
          db_connect_statement_true( stmt2, stmt4 );
          assert( db_statement_connect( stmt4, stmt3 ) );
          stmt2->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for the stmt3 */
          assert( db_statement_connect( stmt3, stmt2 ) );
          block_depth--;
          stmt1 = db_parallelize_statement( stmt1 );
          stmt1->suppl.part.head      = 1;
          stmt1->suppl.part.is_called = 1;
          db_add_statement( stmt1, stmt1 );
        } else {
          db_remove_statement( stmt1 );
          expression_dealloc( $6, FALSE );
          db_remove_statement( stmt3 );
          db_remove_statement( stmt4 );
          stmt1 = NULL;
        }
        db_end_function_task_namedblock( @11.first_line );
        if( (stmt1 != NULL) && ($2 != NULL) ) {
          scope_extract_back( $2->name, back, rest );
          exp = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          exp->elem.funit      = $2;
          exp->suppl.part.type = ETYPE_FUNIT;
          exp->name            = strdup_safe( back );
          stmt = db_create_statement( exp );
          $$ = stmt;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | K_for inc_for_depth '(' for_initialization ';' expression ';' error ')' statement dec_for_depth
    {
      if( parse_mode ) {
        db_remove_statement( $4 );
        expression_dealloc( $6, FALSE );
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
  | K_while '(' expression ')' inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) && ($6 != NULL) ) {
          expression* expr = NULL;
          Try {
            expr = db_create_expression( $3, NULL, EXP_OP_WHILE, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
          } Catch_anonymous {
            error_count++;
          }
          if( ($$ = db_create_statement( expr )) != NULL ) {
            db_connect_statement_true( $$, $6 );
            $$->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for all statements connecting to stmt */
            assert( db_statement_connect( $6, $$ ) );
          }
        } else {
          expression_dealloc( $3, FALSE );
          db_remove_statement( $6 );
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | K_while '(' error ')' inc_block_depth statement dec_block_depth
    {
      if( parse_mode ) {
        db_remove_statement( $6 );
      }
      $$ = NULL;
    }
  | K_do inc_block_depth statement dec_block_depth K_while '(' expression ')' ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) && ($7 != NULL) ) {
          expression* expr = NULL;
          statement*  stmt;
          Try {
            expr = db_create_expression( $7, NULL, EXP_OP_WHILE, FALSE, @5.first_line, @5.first_column, (@8.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
  | delay1
    {
      if( !parse_mode ) {
        generator_add_to_work_code( ";" );
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
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
        generator_flush_work_code();
        $$ = NULL;  /* TBD */
      }
    }
  | event_control
    {
      if( !parse_mode ) {
        generator_add_to_work_code( ";" );
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
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
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | '@' '*'
    {
      if( !parse_mode ) {
        generator_add_to_work_code( ";" );
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
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
                expr = db_create_expression( expr, NULL, EXP_OP_SLIST, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL ); 
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
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | passign ';'
    {
      if( parse_mode ) {
        $$ = $1;
      } else {
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
        $$ = NULL;  /* TBD */
      }
    }
  | lpvalue K_LE expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $3, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( FALSE, TRUE, @1.first_line, @1.first_column );
        generator_insert_line_cov( @1.first_line, @1.first_column );
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | lpvalue '=' delay1 expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $3, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.first_column, (@4.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE delay1 expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
  | lpvalue '=' event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $3, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.first_column, (@4.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
  | lpvalue '=' K_repeat '(' expression ')' event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($5 != NULL) && ($7 != NULL) && ($8 != NULL) ) {
          vector* vec = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          Try {
            expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            (void)vector_from_int( vec, 0x0 );
            assert( tmp->value->value.ul == NULL );
            free_safe( tmp->value, sizeof( vector ) );
            tmp->value = vec;
            tmp = db_create_expression( $5, tmp, EXP_OP_REPEAT, FALSE, @3.first_line, @3.first_column, (@6.last_column - 1), NULL );
            tmp = db_create_expression( $7, tmp, EXP_OP_RPT_DLY, FALSE, @3.first_line, @3.first_column, (@7.last_column - 1), NULL );
            tmp = db_create_expression( $8, tmp, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.first_column, (@8.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE K_repeat '(' expression ')' event_control expression ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($8 != NULL) ) {
          Try {
            expression* tmp = db_create_expression( $8, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
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
        $$ = NULL;  /* TBD */
      }
    }
  | K_wait '(' expression ')' inc_block_depth statement_or_null dec_block_depth
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          expression* exp = NULL;
          Try {
            exp = db_create_expression( $3, NULL, EXP_OP_WAIT, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
        generator_flush_work_code();
        $$ = NULL;  /* TBD */
      }
    }
  | S_ignore '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( !parse_mode ) {
        generator_flush_work_code();
      }
      $$ = NULL;
    }
  | S_allow '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, 0, 0, 0, NULL ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | S_finish '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SFINISH, FALSE, 0, 0, 0, NULL ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | S_stop '(' ignore_more expression_systask_list ignore_less ')' ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SSTOP, FALSE, 0, 0, 0, NULL ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | S_srandom '(' expression_systask_list ')' ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($3 != NULL) ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, $3, EXP_OP_SSRANDOM, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL ) );
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
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | S_ignore ';'
    {
      $$ = NULL;
      if( !parse_mode ) {
        generator_flush_work_code();
      }
    }
  | S_allow ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, 0, 0, 0, NULL ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | S_finish ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SFINISH, FALSE, 0, 0, 0, NULL ) );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
      } else {
        generator_flush_work_code();
        $$ = NULL;
      }
    }
  | S_stop ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          Try {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SSTOP, FALSE, 0, 0, 0, NULL ) );
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
            expression* exp = db_create_expression( NULL, $3, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.first_column, (@5.last_column - 1), $1 );
            $$ = db_create_statement( exp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
          free_safe( $1, (strlen( $1 ) + 1) );
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | identifier ';'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            expression* exp = db_create_expression( NULL, NULL, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $1 );
            $$ = db_create_statement( exp );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
          free_safe( $1, (strlen( $1 ) + 1) );
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
   /* Immediate SystemVerilog assertions are parsed but not performed -- we will not exclude a block that contains one */
  | K_assert ';'
    {
      if( parse_mode ) {
        expression* exp;
        statement*  stmt;
        if( ignore_mode == 0 ) {
          exp  = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL );
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
            expression* exp = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $1, @1.text, @1.first_line ) ) {
              ignore_mode++;
            }
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
        } else {
          ignore_mode++;
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
              expr = db_create_expression( NULL, NULL, EXP_OP_JOIN, FALSE, @4.first_line, @4.first_column, (@4.last_column - 1), NULL );
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
          free_safe( $1, (strlen( $1 ) + 1) );
        } else {
          if( $3 && db_is_unnamed_scope( $1 ) ) {
            ignore_mode--;
          }
          free_safe( $1, (strlen( $1 ) + 1) );
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
            if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $1, @1.text, @1.first_line ) ) {
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
        free_safe( $1, (strlen( $1 ) + 1) );
        generate_top_mode++;
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | begin_end_id
    {
      if( parse_mode ) {
        free_safe( $1, (strlen( $1 ) + 1) );
        ignore_mode++;
        $$ = NULL;
      } else {
        $$ = NULL;  /* TBD */
      }
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
      $$ = $1;
    }
  /* This rule is not in the LRM but seems to be supported by several simulators */
  | statement_list ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          if( $1 == NULL ) {
            $$ = NULL;
          } else {
            statement* stmt = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @2.first_line, @2.first_column, (@2.last_column - 1), NULL ) );
            if( !db_statement_connect( $1, stmt ) ) {
              db_remove_statement( stmt );
            }
            $$ = $1;
          }
        }
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  /* This rule is not in the LRM but seems to be supported by several simulators */
  | ';'
    {
      if( parse_mode ) {
        $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL ) );
      } else {
        $$ = NULL;  /* TBD */
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
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$  = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | identifier start_lhs index_expr end_lhs
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          db_bind_expr_tree( $3, $1 );
          $3->line = @1.first_line;
          $3->col  = ((@1.first_column & 0xffff) << 16) | ($3->col & 0xffff);
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = $3;
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
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
            $$ = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
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
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          } Catch_anonymous {
            error_count++;
            $$ = NULL;
          }
        } else {
          $$  = NULL;
        }
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | identifier start_lhs index_expr end_lhs
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          db_bind_expr_tree( $3, $1 );
          $3->line = @1.first_line;
          $3->col  = ((@1.first_column & 0xffff) << 16) | ($3->col & 0xffff);
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = $3;
        } else {
          free_safe( $1, (strlen( $1 ) + 1) );
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
            $$ = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
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
  | block_item_decls block_item_decl
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
        curr_mba      = TRUE;
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
  : expression_list ':' statement_or_null
    { PROFILE(PARSER_CASE_ITEM_A);
      if( parse_mode ) {
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
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | K_default ':' statement_or_null
    { PROFILE(PARSER_CASE_ITEM_B);
      if( parse_mode ) {
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
      } else {
        $$ = NULL;  /* TBD */
      }
    }
  | K_default statement_or_null
    { PROFILE(PARSER_CASE_ITEM_C);
      if( parse_mode ) {
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
      } else {
        $$ = NULL;  /* TBD */
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

delay1
  : '#' delay_value_simple
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( $2, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
            $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
            $$ = db_create_expression( $2, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
            $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
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
            $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), NULL );
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
            $$ = db_create_expression( $5, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
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
              $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
                                        obf_file( @1.text ),
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
                $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
        free_safe( $1, (strlen( $1 ) + 1) );
      } else {
        $$ = NULL;
      }
    }
  | REALTIME
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
              $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
            tmp = db_create_expression( $3, $1, EXP_OP_ASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( TRUE, TRUE, @1.first_line, @1.first_column );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
                exp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
                exp = db_create_expression( $3, exp, EXP_OP_RASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
              tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
              tmp = db_create_expression( $3, tmp, EXP_OP_DASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
        generator_insert_comb_cov( TRUE, TRUE, @1.first_line, @1.first_column );
      }
      free_safe( $1, (strlen( $1 ) + 1) );
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
              tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.first_column, (@2.last_column - 1), $2 );
              tmp = db_create_expression( $4, tmp, EXP_OP_DASSIGN, FALSE, @2.first_line, @2.first_column, (@4.last_column - 1), NULL );
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
        generator_insert_comb_cov( TRUE, TRUE, @1.first_line, @1.first_column );
      }
      free_safe( $2, (sizeof( $2 ) + 1) );
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
            $$ = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
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
      free_safe( $2, (strlen( $2 ) + 1) );
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
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            $$ = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
              $$ = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
            $$ = db_create_expression( $2, NULL, EXP_OP_PEDGE, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
            $$ = db_create_expression( $2, NULL, EXP_OP_NEDGE, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
            $$ = db_create_expression( $1, NULL, EXP_OP_AEDGE, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
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
        free_safe( $1, (strlen( $1 ) + 1) );
      }
    }
  ;

 /* Parameter override */
parameter_value_opt
  : '#' '(' { param_mode++; } expression_list { param_mode--; } ')'
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
      free_safe( $2, (strlen( $2 ) + 1) );
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
  : '.' IDENTIFIER '(' expression ')'
    { PROFILE(PARSER_PARAMETER_VALUE_BYNAME_A);
      if( parse_mode ) {
        param_oride* po;
        if( ignore_mode == 0 ) {
          if( !parser_check_generation( GENERATION_2001 ) ) {
            VLerror( "Explicit in-line parameter passing syntax found in block that is specified to not allow Verilog-2001 syntax" );
            free_safe( $2, (strlen( $2 ) + 1) );
            expression_dealloc( $4, FALSE );
          } else {
            po = (param_oride*)malloc_safe( sizeof( param_oride ) );
            po->name = $2;
            po->expr = $4;
            po->next = NULL;
            if( param_oride_head == NULL ) {
              param_oride_head = param_oride_tail = po;
            } else {
              param_oride_tail->next = po;
              param_oride_tail       = po;
            }
          }
        } else {
          free_safe( $2, (strlen( $2 ) + 1) );
        }
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
      free_safe( $2, (strlen( $2 ) + 1) );
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
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
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
            free_safe( $1, (strlen( $1 ) + 1) );
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
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
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
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
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
            free_safe( $1, (strlen( $1 ) + 1) );
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
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
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
  | function_item_list function_item
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
  : IDENTIFIER '=' expression
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          /* If the size was not set by the user, the left number will be set to 0 but we need to change this to 31 */
          assert( curr_prange.dim != NULL );
          if( curr_prange.dim[0].implicit ) {
            db_add_declared_param( curr_signed, NULL, NULL, $1, $3, FALSE );
          } else {
            db_add_declared_param( curr_signed, curr_prange.dim[0].left, curr_prange.dim[0].right, $1, $3, FALSE );
            curr_prange.exp_dealloc = FALSE;
          }
        }
      }
      free_safe( $1, (strlen( $1 ) + 1) );
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
          /* If the size was not set by the user, the left number will be set to 0 but we need to change this to 31 */
          assert( curr_prange.dim != NULL );
          if( curr_prange.dim[0].implicit ) {
            db_add_declared_param( curr_signed, NULL, NULL, $1, $3, TRUE );
          } else {
            db_add_declared_param( curr_signed, curr_prange.dim[0].left, curr_prange.dim[0].right, $1, $3, TRUE );
            curr_prange.exp_dealloc = FALSE;
          }
        }
      }
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  ;

port_name_list
  : port_name_list ',' port_name
  | port_name
  ;

port_name
  : '.' IDENTIFIER '(' ignore_more expression ignore_less ')'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | '.' IDENTIFIER '(' error ')'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | '.' IDENTIFIER '(' ')'
    {
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | '.' IDENTIFIER
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Implicit .name port list item found in block that is specified to not allow SystemVerilog syntax" );
      }
      free_safe( $2, (strlen( $2 ) + 1) );
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
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER '=' expression ':' expression ':' expression
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | PATHPULSE_IDENTIFIER '=' expression
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | PATHPULSE_IDENTIFIER '=' '(' expression ',' expression ')'
    {
      free_safe( $1, (strlen( $1 ) + 1) );
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
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | IDENTIFIER '[' expr_primary ']'
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  | specify_path_identifiers ',' IDENTIFIER
    {
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  | specify_path_identifiers ',' IDENTIFIER '[' expr_primary ']'
    {
      free_safe( $3, (strlen( $3 ) + 1) );
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
      free_safe( $2, (strlen( $2 ) + 1) );
    }
  | spec_notifier ',' 
  | spec_notifier ',' identifier
    {
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  | IDENTIFIER
    {
      free_safe( $1, (strlen( $1 ) + 1) );
    }
  ;

specify_edge_path_decl
  : specify_edge_path '=' '(' specify_delay_value_list ')'
  | specify_edge_path '=' delay_value_simple
  ;

specify_edge_path
  : '(' K_posedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
      free_safe( $6, (strlen( $6 ) + 1) );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
      free_safe( $6, (strlen( $6 ) + 1) );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
      free_safe( $6, (strlen( $6 ) + 1) );
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
      free_safe( $6, (strlen( $6 ) + 1) );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
      free_safe( $1, (strlen( $1 ) + 1) );
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
          free_safe( $1, (strlen( $1 ) + 1) );
          $$ = NULL;
        }
      } else {
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
          strl->str    = NULL;
          strl->suppl  = @3.first_line;
          strl->suppl2 = @3.first_column;
          strl->next   = NULL;
          while( strt->next != NULL ) strt = strt->next;
          strt->next = strl;
          $$ = $1; 
        } else {
          str_link_delete_list( $1 );
          free_safe( $3, (strlen( $3 ) + 1) );
          $$ = NULL;
        }
      } else {
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
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  | K_typedef net_type_sign_range_opt IDENTIFIER ';'
    {
      if( parse_mode ) {
        if( ignore_mode == 0 ) {
          assert( curr_prange.dim != NULL );
          db_add_typedef( $3, curr_signed, curr_handled, TRUE, parser_copy_curr_range( TRUE ), parser_copy_curr_range( FALSE ) );
        }
      }
      free_safe( $3, (strlen( $3 ) + 1) );
    }
  ;

single_index_expr
  : '[' expression ']'
    {
      if( parse_mode ) {
        if( (ignore_mode == 0) && ($2 != NULL) ) {
          Try {
            $$ = db_create_expression( NULL, $2, EXP_OP_SBIT_SEL, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
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
            $$ = db_create_expression( $4, $2, EXP_OP_MBIT_SEL, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
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
                expression* tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
                vector_dealloc( tmp->value );
                tmp->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
                (void)vector_from_int( tmp->value, $4->num );
                $$ = db_create_expression( tmp, $2, EXP_OP_MBIT_POS, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
              } Catch_anonymous {
                error_count++;
                $$ = NULL;
              }
            } else {
              Try {
                $$ = db_create_expression( $4->exp, $2, EXP_OP_MBIT_POS, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
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
                tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
              } Catch_anonymous {
                error_count++;
              }
              vector_dealloc( tmp->value );
              tmp->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
              (void)vector_from_int( tmp->value, $4->num );
              Try {
                tmp = db_create_expression( tmp, $2, EXP_OP_MBIT_NEG, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
              } Catch_anonymous {
                error_count++;
              }
            } else {
              Try {
                tmp = db_create_expression( $4->exp, $2, EXP_OP_MBIT_NEG, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
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
            $$ = db_create_expression( $2, $1, EXP_OP_DIM, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
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
      if( parse_mode ) {
        fork_depth++;
        fork_block_depth = (int*)realloc_safe( fork_block_depth, (sizeof( int ) * fork_depth), ((fork_depth + 1) * sizeof( int )) );
        fork_block_depth[fork_depth] = block_depth;
      }
    }
  ;

dec_fork_depth
  :
    {
      if( parse_mode ) {
        fork_depth--;
        fork_block_depth = (int*)realloc_safe( fork_block_depth, (sizeof( int ) * (fork_depth + 2)), ((fork_depth + 1) * sizeof( int )) );
      }
    }
  ;

inc_block_depth
  :
    {
      if( parse_mode ) {
        block_depth++;
      } else {
        generator_flush_work_code();
        generator_add_to_hold_code( " begin" );
      }
    }
  ;

dec_block_depth
  :
    {
      if( parse_mode ) {
        block_depth--;
      } else {
        generator_flush_work_code();
        generator_add_to_hold_code( " end" );
      }
    }
  ;

inc_for_depth
  :
    {
      if( parse_mode ) {
        char* scope      = NULL;
        func_unit* funit = db_get_curr_funit();
        if( ignore_mode == 0 ) {
          scope = db_create_unnamed_scope();
          Try {
            assert( db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, scope, funit->filename, 0 ) );
          } Catch_anonymous {
            error_count++;
            ignore_mode++;
          }
          free_safe( scope, (strlen( scope ) + 1) );
        }
        block_depth++;
        for_mode++;
        $$ = db_get_curr_funit();
      } else {
        $$ = NULL;
      }
    }
  ;

dec_for_depth
  :
    {
      if( parse_mode ) {
        block_depth--;
        for_mode--;
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
