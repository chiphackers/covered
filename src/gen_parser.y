
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
#include "generator.new.h"
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
extern int    curr_sig_type;

/* Functions from lexer */
extern void lex_start_udp_table();
extern void lex_end_udp_table();

/*!
 The current block depth.
*/
static int block_depth = 0;

/*!
 Array storing the depth that a given fork block is at.
*/
static int* fork_block_depth = NULL;

/*!
 Specifies if we are currently in a fork statement (> 0) or not.
*/
static int fork_depth = -1;

/*!
 \return Returns the name of the fork..join block that this statement represents (if it does).

 Checks to see if the current statement is a fork..join block, and if it is pushes
 the functional unit on the stack and returns the block name.
*/
static char* generator_handle_push_funit(
  unsigned int first_line,   /*!< First line of statement to check */
  unsigned int first_column  /*!< First column of statement to check */
) {

  char* cov_str = NULL;

  if( (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {
    func_unit* funit;
    if( (funit = db_get_tfn_by_position( first_line, first_column )) != NULL ) {
      char         str[50];
      char*        back;
      char*        rest;
      unsigned int rv;
      assert( funit != NULL );
      back = strdup_safe( funit->name );
      rest = strdup_safe( funit->name );
      scope_extract_back( funit->name, back, rest );
      rv = snprintf( str, 50, " : %s", back );
      assert( rv < 50 );
      free_safe( back, (strlen( funit->name ) + 1) );
      free_safe( rest, (strlen( funit->name ) + 1) );
      generator_push_funit( funit );
      cov_str = generator_build( 3, strdup_safe( "begin" ), strdup_safe( str ), "\n" );
    }
  }

  return( cov_str );

}

/*!
 \return Returns the "end" string if we are handling a fork..join block statement.

 Pops the current functional unit if we are handling a fork..join block.
*/
static char* generator_handle_pop_funit(
  unsigned int first_line,   /*!< First line of statement to check */
  unsigned int first_column  /*!< First column of statement to check */
) {

  char* cov_str = NULL;

  if( (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {
    if( db_get_tfn_by_position( first_line, first_column ) != NULL ) {
      generator_pop_funit();
      cov_str = generator_build( 2, strdup_safe( "end" ), "\n" );
    }
  }

  return( cov_str );

}


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
  char*    text;
  str_cov* text_cov;
};

%token <text> IDENTIFIER TYPEDEF_IDENTIFIER PATHPULSE_IDENTIFIER DEC_NUMBER BASE_NUMBER REALTIME STRING IGNORE SYSCALL
%token <text> K_assert
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
%token K_assume K_property K_endproperty K_cover K_sequence K_endsequence K_expect
%token K_program K_endprogram K_final K_void K_return K_continue K_break K_extern K_interface K_endinterface
%token K_class K_endclass K_extends K_package K_endpackage K_timeunit K_timeprecision K_ref K_bind K_const
%token K_new K_static K_protected K_local K_rand K_randc K_randcase K_constraint K_import K_export K_scalared K_chandle
%token K_context K_pure K_modport K_clocking K_iff K_intersect K_first_match K_throughout K_with K_within
%token K_dist K_covergroup K_endgroup K_coverpoint K_optionDOT K_type_optionDOT K_wildcard
%token K_bins K_illegal_bins K_ignore_bins K_cross K_binsof K_alias K_join_any K_join_none K_matches
%token K_tagged K_foreach K_randsequence K_ifnone K_randomize K_null K_inside K_super K_this K_wait_order
%token K_include K_Sroot K_Sunit K_endclocking K_virtual K_before K_forkjoin K_solve

%token KK_attribute

%type <text>     number
%type <text>     automatic_opt block_item_decls_opt
%type <text>     net_type net_type_sign_range_opt var_type data_type_opt
%type <text>     identifier begin_end_id
%type <text>     static_expr static_expr_primary static_expr_port_list
%type <text>     expr_primary expression_list expression expression_port_list expression_systask_list
%type <text>     lavalue lpvalue
%type <text>     event_control event_expression_list event_expression
%type <text>     delay_value delay_value_simple
%type <text>     delay1 delay3 delay3_opt
%type <text>     generate_passign index_expr single_index_expr
%type <text>     statement statement_list statement_or_null
%type <text>     if_statement_error for_condition
%type <text_cov> passign for_initialization expression_assignment_list
%type <text>     gate_instance gate_instance_list list_of_names
%type <text>     begin_end_block fork_statement
%type <text>     generate_item generate_item_list generate_item_list_opt
%type <text>     case_items case_item case_body
%type <text>     generate_case_items generate_case_item
%type <text>     static_unary_op unary_op
%type <text>     pre_op_and_assign_op post_op_and_assign_op op_and_assign_op
%type <text>     if_body
%type <text>     gen_if_body
%type <text>     attribute_list_opt attribute_list attribute source_file description module module_start module_parameter_port_list_opt module_parameter_port_list
%type <text>     module_port_list_opt list_of_port_declarations port_declaration udp_primitive typedef_decl signed_opt range_opt list_of_variables
%type <text>     net_decl_assigns drive_strength charge_strength_opt gatetype dr_strength1 dr_strength0 block_item_decl task_item_list_opt
%type <text>     range_or_type_opt function_item_list enumeration module_item_list_opt parameter_assign list_of_ports port_type port_opt port port_reference
%type <text>     port_reference_list range udp_port_list udp_port_decls udp_init_opt udp_body udp_port_decl udp_initial udp_entry_list
%type <text>     udp_comb_entry_list udp_sequ_entry_list udp_comb_entry udp_input_list udp_output_sym udp_input_sym udp_sequ_entry module_item module_item_list
%type <text>     register_variable_list defparam_assign_list parameter_value_opt drive_strength_opt assign_list specify_item_list struct_union
%type <text>     integer_vector_type data_type integer_atom_type struct_union_member_list data_type_or_void cond_specifier_opt block_item_decls unsigned_opt
%type <text>     parameter_assign_decl localparam_assign_decl assign register_variable task_item_list task_item net_decl_assign charge_strength defparam_assign
%type <text>     parameter_value_byname_list parameter_value_byname port_name_list function_item parameter_assign_list localparam_assign_list localparam_assign
%type <text>     port_name specify_item specparam_list specify_simple_path_decl specify_edge_path_decl spec_reference_event spec_notifier_opt specparam
%type <text>     specify_simple_path specify_delay_value_list specify_path_identifiers spec_polarity spec_notifier specify_edge_path polarity_operator
%type <text>     enum_var_type_range_opt enum_variable_list enum_variable

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
      generator_write_to_file( $1 );
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
      $$ = generator_build( 7, strdup_safe( "trireg" ), $2, $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | gatetype gate_instance_list ';'
    {
      $$ = generator_build( 4, $1, $2, strdup_safe( ";" ), "\n" );
    }
  | gatetype delay3 gate_instance_list ';'
    {
      $$ = generator_build( 4, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  | gatetype drive_strength gate_instance_list ';'
    {
      $$ = generator_build( 4, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  | gatetype drive_strength delay3 gate_instance_list ';'
    {
      $$ = generator_build( 5, $1, $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | K_pullup gate_instance_list ';'
    {
      $$ = generator_build( 3, strdup_safe( "pullup" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_pulldown gate_instance_list ';'
    {
      $$ = generator_build( 3, strdup_safe( "pulldown" ), $2, strdup_safe( ";" ), "\n" );
    }
  | K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      $$ = generator_build( 5, strdup_safe( "pullup(" ), $3, strdup_safe( ")" ), $5, strdup_safe( ";" ), "\n" );
    }
  | K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      $$ = generator_build( 5, strdup_safe( "pulldown(" ), $3, strdup_safe( ")" ), $5, strdup_safe( ";" ), "\n" );
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
    task_item_list_opt
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @3.first_line, @3.first_column )) != NULL ) {
        // generator_create_tmp_regs();
        generator_push_funit( funit );
      }
    }
    statement_or_null
    K_endtask
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @3.first_line, @3.first_column )) != NULL ) {
        generator_pop_funit();
      }
      $$ = generator_build( 10, strdup_safe( "task" ), $2, $3, strdup_safe( ";" ), "\n", $5, generator_inst_id_reg( funit ), $7, strdup_safe( "endtask" ), "\n" );
    }
  | K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    function_item_list
    {
      func_unit* funit;
      if( ((funit = db_get_tfn_by_position( @5.first_line, @5.first_column )) != NULL) && generator_is_static_function( funit ) ) {
        generator_push_funit( funit );
        generator_create_tmp_regs();
      }
    }
    statement
    K_endfunction
    {
      func_unit* funit = db_get_tfn_by_position( @5.first_line, @5.first_column );
      if( (funit != NULL) && generator_is_static_function( funit ) ) {
        generator_pop_funit( funit );
      }
      if( (strncmp( $9, "begin ", 6 ) != 0) && ($9[0] != ';') ) {
        $9 = generator_build( 5, strdup_safe( "begin" ), "\n", $9, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 12, strdup_safe( "function" ), $2, $3, $4, $5, strdup_safe( ";" ), "\n", $7,
                            (generator_is_static_function( funit ) ? generator_inst_id_reg( funit ) : NULL), $9, strdup_safe( "endfunction" ), "\n" );
    }
  | error ';'
    {
      GENerror( "Invalid $root item" );
      $$ = NULL;
    }
  | K_function error K_endfunction
    {
      GENerror( "Syntax error in function description" );
      $$ = NULL;
    }
  | enumeration list_of_names ';'
    {
      $$ = generator_build( 4, $1, $2, strdup_safe( ";" ), "\n" );
    }
  ;

module
  : attribute_list_opt module_start IDENTIFIER 
    {
      db_find_and_set_curr_funit( $3, FUNIT_MODULE );
      generator_push_funit( db_get_curr_funit() );
      generator_init_funit( db_get_curr_funit() );
    }
    module_parameter_port_list_opt
    module_port_list_opt ';'
    module_item_list_opt K_endmodule
    {
      generator_pop_funit();
      $$ = generator_build( 14, $1, $2, $3, $5, $6, strdup_safe( ";" ), "\n",
                            generator_inst_id_reg( db_get_curr_funit() ), generator_tmp_regs(), $8, generator_fsm_covs(),
                            generator_inst_id_overrides(), strdup_safe( "endmodule" ), "\n" );
    }
  | attribute_list_opt K_module IGNORE I_endmodule
    {
      $$ = generator_build( 5, $1, strdup_safe( "module" ), $3, strdup_safe( "endmodule" ), "\n" );
    }
  | attribute_list_opt K_macromodule IGNORE I_endmodule
    {
      $$ = generator_build( 5, $1, strdup_safe( "macromodule" ), $3, strdup_safe( "endmodule" ), "\n" );
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
      $$ = generator_build( 3, strdup_safe( "#(" ), $3, strdup_safe( ")" ) );
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
      $$ = generator_build( 6, $1, strdup_safe( "output" ), $3, $4, $5, $6 );
    }
  /* We just need to parse the static register assignment as this signal will get its value from the dumpfile */
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER '=' static_expr
    {
      $$ = generator_build( 8, $1, strdup_safe( "output" ), $3, $4, $5, $6, strdup_safe( "=" ), $8 );
    }
  | attribute_list_opt port_type net_type_sign_range_opt error
    {
      $$ = NULL;
    }
  | attribute_list_opt K_output var_type signed_opt range_opt error
    {
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
      $$ = generator_build( 5, strdup_safe( "." ), $2, strdup_safe( "(" ), $4, strdup_safe( ")" ) );
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
      int          len = strlen( $1 ) + strlen( $2 ) + 1;
      char*        str = (char*)malloc_safe( len );
      unsigned int rv  = snprintf( str, len, "%s%s", $1, $2 );
      assert( rv < len );
      FREE_TEXT( $1 );
      FREE_TEXT( $2 );
      $$ = str;
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
  | SYSCALL '(' static_expr ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" ) );
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
      GENerror( "Operand of signed positive + is not a primary expression" );
    }
  | '-' error %prec UNARY_PREC
    {
      GENerror( "Operand of signed negative - is not a primary expression" );
    }
  | '~' error %prec UNARY_PREC
    {
      GENerror( "Operand of unary ~ is not a primary expression" );
    }
  | '&' error %prec UNARY_PREC
    {
      GENerror( "Operand of reduction & is not a primary expression" );
    }
  | '!' error %prec UNARY_PREC
    {
      GENerror( "Operand of unary ! is not a primary expression" );
    }
  | '|' error %prec UNARY_PREC
    {
      GENerror( "Operand of reduction | is not a primary expression" );
    }
  | '^' error %prec UNARY_PREC
    {
      GENerror( "Operand of reduction ^ is not a primary expression" );
    }
  | K_NAND error %prec UNARY_PREC
    {
      GENerror( "Operand of reduction ~& is not a primary expression" );
    }
  | K_NOR error %prec UNARY_PREC
    {
      GENerror( "Operand of reduction ~| is not a primary expression" );
    }
  | K_NXOR error %prec UNARY_PREC
    {
      GENerror( "Operand of reduction ~^ is not a primary expression" );
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
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" ) );
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
      $$ = generator_build( 3, $1, strdup_safe( "." ), $3 );
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
      $$ = generator_build( 4, strdup_safe( "reg" ), $2, strdup_safe( ";" ), "\n" );
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
      $$ = generator_build( 4, strdup_safe( "table" ), $3, strdup_safe( "endtable" ), "\n" );
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
  | K_begin
    {
      func_unit* funit = db_get_tfn_by_position( @1.first_line, @1.first_column );
      assert( funit != NULL );
      generator_push_funit( funit );
      generator_create_tmp_regs();
    }
    generate_item_list_opt K_end
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
      generator_pop_funit();
      $$ = generator_build( 9, strdup_safe( "begin" ), strdup_safe( str ), "\n", generator_inst_id_reg( funit ), "\n", generator_tmp_regs(), $3, strdup_safe( "end" ), "\n" );
    }
  | K_begin ':' IDENTIFIER
    {
      func_unit* funit = db_get_tfn_by_position( @1.first_line, @1.first_column );
      assert( funit != NULL );
      generator_push_funit( funit );
      generator_create_tmp_regs();
    }
    generate_item_list_opt K_end
    {
      func_unit* funit = db_get_tfn_by_position( @3.first_line, @3.first_column );
      assert( funit != NULL );
      generator_pop_funit();
      $$ = generator_build( 9, strdup_safe( "begin : " ), $3, "\n", generator_inst_id_reg( funit ), "\n", generator_tmp_regs(), $5, strdup_safe( "end" ), "\n" );
    }
  | K_for '(' generate_passign ';' static_expr ';' generate_passign ')' K_begin ':' IDENTIFIER
    {
      func_unit* funit = db_get_tfn_by_position( @11.first_line, @11.first_column );
      assert( funit != NULL );
      generator_push_funit( funit );
      generator_create_tmp_regs();
    }
    generate_item_list_opt K_end
    {
      func_unit* funit = db_get_tfn_by_position( @11.first_line, @11.first_column );
      assert( funit != NULL );
      generator_pop_funit();
      $$ = generator_build( 17, strdup_safe( "for(" ), $3, strdup_safe( ";" ), $5, strdup_safe( ";" ), $7, strdup_safe( ")" ), "\n", strdup_safe( "begin : " ), $11, "\n",
                            generator_inst_id_reg( funit ), "\n", generator_tmp_regs(), $13, strdup_safe( "end" ), "\n" );
    }
  | K_if '(' static_expr ')' gen_if_body
    {
      $$ = generator_build( 4, strdup_safe( "if(" ), $3, strdup_safe( ")" ), $5 );
    }
  | K_case '(' static_expr ')' inc_block_depth generate_case_items dec_block_depth K_endcase
    { 
      $$ = generator_build( 6, strdup_safe( "case(" ), $3, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
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
      GENerror( "Illegal generate case expression" );
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
      GENerror( "Invalid variable list in port declaration" );
      $$ = NULL;
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
      $$ = generator_build( 5, $1, strdup_safe( "pullup" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_pulldown gate_instance_list ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "pulldown" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "pullup(" ), $4, strdup_safe( ")" ), $6, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "pulldown(" ), $4, strdup_safe( ")" ), $6, strdup_safe( ";" ), "\n" );
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
  | attribute_list_opt K_assign drive_strength_opt delay3_opt
    {
      generator_create_tmp_regs();
    }
    assign_list ';'
    {
      $$ = generator_build( 8, generator_tmp_regs(), $1, strdup_safe( "assign" ), $3, $4, $6, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_always statement
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 3, $1, strdup_safe( "always" ), $3 );
    }
  | attribute_list_opt K_always_comb statement
    {
      if( !info_suppl.part.verilator ) {
        $$ = generator_build( 8, $1, strdup_safe( "always_comb" ), strdup_safe( "begin" ),
                              generator_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE ),
                              generator_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE, TRUE ), $3, strdup_safe( "end" ), "\n" );
      } else {
        $$ = generator_build( 3, $1, strdup_safe( "always_comb" ), $3 );
      }
    }
  | attribute_list_opt K_always_latch statement
    {
      if( !info_suppl.part.verilator ) {
        $$ = generator_build( 8, $1, strdup_safe( "always_latch" ), strdup_safe( "begin" ),
                              generator_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE ),
                              generator_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE, TRUE ), $3, strdup_safe( "end" ), "\n" );
      } else {
        $$ = generator_build( 3, $1, strdup_safe( "always_latch" ), $3 );
      }
    }
  | attribute_list_opt K_always_ff statement
    {
      if( !info_suppl.part.verilator ) {
        $$ = generator_build( 8, $1, strdup_safe( "always_ff" ), strdup_safe( "begin" ),
                              generator_line_cov( @2.ppfline, ((@2.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@2.last_column - 1), TRUE ),
                              generator_comb_cov( @2.ppfline, @2.first_column, FALSE, FALSE, FALSE, TRUE ), $3, strdup_safe( "end" ), "\n" );
      } else {
        $$ = generator_build( 3, $1, strdup_safe( "always_ff" ), $3 );
      }
    }
  | attribute_list_opt K_initial statement
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 3, $1, strdup_safe( "initial" ), $3 );
    }
  | attribute_list_opt K_final statement
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 3, $1, strdup_safe( "final" ), $3 );
    }
  | attribute_list_opt K_task automatic_opt IDENTIFIER ';' task_item_list_opt
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @4.first_line, @4.first_column )) != NULL ) {
        generator_push_funit( funit );
        // generator_create_tmp_regs();
      }
    }
    statement_or_null K_endtask
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @4.first_line, @4.first_column )) != NULL ) {
        generator_pop_funit();
      }
      $$ = generator_build( 10, $1, strdup_safe( "task" ), $3, $4, strdup_safe( ";" ), "\n", $6, $8, strdup_safe( "endtask" ), "\n" );
    }
  | attribute_list_opt K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    function_item_list
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @6.first_line, @6.first_column )) != NULL ) {
        generator_push_funit( funit );
        generator_create_tmp_regs();
      }
    }
    statement
    K_endfunction
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @6.first_line, @6.first_column )) != NULL ) {
        generator_pop_funit();
        if( (strncmp( $10, "begin ", 6 ) != 0) && ($10[0] != ';') ) {
          $10 = generator_build( 5, strdup_safe( "begin" ), "\n", $10, strdup_safe( "end" ), "\n" );
        }
        $$ = generator_build( 14, strdup_safe( "function" ), $3, $4, $5, $6, strdup_safe( ";" ), "\n", $8,
                              (generator_is_static_function( funit ) ? generator_inst_id_reg( funit ) : NULL), "\n", generator_tmp_regs(), $10, strdup_safe( "endfunction" ), "\n" );
      } else {
        $$ = generator_build( 11, strdup_safe( "function" ), $3, $4, $5, $6, strdup_safe( ";" ), "\n", $8, $10, strdup_safe( "endfunction" ), "\n" );
      }
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
      GENerror( "Invalid specify syntax" );
      $$ = NULL;
    }
  | error ';'
    {
      GENerror( "Invalid module item.  Did you forget an initial or always?" );
      $$ = NULL;
    }
  | attribute_list_opt K_assign error '=' expression ';'
    {
      GENerror( "Syntax error in left side of continuous assignment" );
      $$ = NULL;
    }
  | attribute_list_opt K_assign error ';'
    {
      GENerror( "Syntax error in continuous assignment" );
      $$ = NULL;
    }
  | attribute_list_opt K_function error K_endfunction
    {
      GENerror( "Syntax error in function description" );
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
      $$ = generator_build( 8, strdup_safe( "$attribute(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, strdup_safe( ");" ), "\n" );
    } 
  | KK_attribute '(' error ')' ';'
    {
      GENerror( "Syntax error in $attribute parameter list" );
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
      $$ = $1;
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
      $$ = generator_build2( generator_line_cov( @2.ppfline, ((@4.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@4.last_column - 1), TRUE ),
                             generator_build( 4, $1, $2, strdup_safe( "=" ), $4 ) );
    }
  | expression_assignment_list ',' data_type_opt IDENTIFIER '=' expression
    {
      $$ = generator_build2( generator_build( 2, $1->cov, generator_line_cov( @4.ppfline, ((@6.last_line - @4.first_line) + @4.ppfline), @4.first_column, (@6.last_column - 1), TRUE ) ),
                             generator_build( 6, $1->str, strdup_safe( "," ), $3, $4, strdup_safe( "=" ), $6 ) );
      generator_destroy2( $1 );
    }
  ;

passign
  : lpvalue '=' expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "=" ), $3 ) );
    }
  | lpvalue K_ADD_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "+=" ), $3 ) );
    }
  | lpvalue K_SUB_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "-=" ), $3 ) );
    }
  | lpvalue K_MLT_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "*=" ), $3 ) );
    }
  | lpvalue K_DIV_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "/=" ), $3 ) );
    }
  | lpvalue K_MOD_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "%=" ), $3 ) );
    }
  | lpvalue K_AND_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "&=" ), $3 ) );
    }
  | lpvalue K_OR_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "|=" ), $3 ) );
    }
  | lpvalue K_XOR_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "^=" ), $3 ) );
    }
  | lpvalue K_LS_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "<<=" ), $3 ) );
    }
  | lpvalue K_RS_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( ">>=" ), $3 ) );
    }
  | lpvalue K_ALS_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( "<<<=" ), $3 ) );
    }
  | lpvalue K_ARS_A expression
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 3, $1, strdup_safe( ">>>=" ), $3 ) );
    }
  | lpvalue K_INC
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 2, $1, strdup_safe( "++" ) ) );
    }
  | lpvalue K_DEC
    {
      $$ = generator_build2( generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                             generator_build( 2, $1, strdup_safe( "--" ) ) );
    }
  ;

if_body
  : inc_block_depth statement_or_null dec_block_depth %prec less_than_K_else
    {
      if( (strncmp( $2, "begin ", 6 ) != 0) && ($2[0] != ';') ) {
        $2 = generator_build( 5, strdup_safe( "begin" ), "\n", $2, strdup_safe( "end" ), "\n" );
      }
      $$ = $2;
    }
  | inc_block_depth statement_or_null dec_block_depth K_else inc_block_depth statement_or_null dec_block_depth
    {
      if( (strncmp( $2, "begin ", 6 ) != 0) && ($2[0] != ';') ) {
        $2 = generator_build( 5, strdup_safe( "begin" ), "\n", $2, strdup_safe( "end" ), "\n" );
      }
      if( (strncmp( $6, "begin ", 6 ) != 0) && ($6[0] != ';') ) {
        $6 = generator_build( 5, strdup_safe( "begin" ), "\n", $6, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 3, $2, strdup_safe( "else" ), $6 );
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
  | K_begin inc_block_depth begin_end_block dec_block_depth K_end
    {
      $$ = generator_build( 4, strdup_safe( "begin" ), $3, strdup_safe( "end" ), "\n" );
    }
  | K_fork inc_fork_depth fork_statement dec_fork_depth K_join
    {
      $$ = generator_build( 4, strdup_safe( "fork" ), $3, strdup_safe( "join" ), "\n" );
    }
  | K_disable identifier ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 6, $$, generator_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE ),
                            strdup_safe( "disable" ), $2, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | K_TRIGGER IDENTIFIER ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 9, $$, generator_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE ),
                            $2, strdup_safe( "= (" ), strdup_safe( $2 ), strdup_safe( "=== 1'bx) ? 1'b0 : ~" ), strdup_safe( $2 ), strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | K_forever inc_block_depth statement dec_block_depth
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 2, strdup_safe( "forever" ), $3 );
    }
  | K_repeat '(' expression ')' inc_block_depth statement dec_block_depth
    {
      if( (strncmp( $6, "begin ", 6 ) != 0) && ($6[0] != ';') ) {
        $6 = generator_build( 5, strdup_safe( "begin" ), "\n", $6, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 7, $$, generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                            strdup_safe( "repeat(" ), $3, strdup_safe( ")" ), $6 );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | cond_specifier_opt K_case '(' expression ')' case_body K_endcase
    {
      $$ = generator_handle_push_funit( @2.first_line, @2.first_column );
      $$ = generator_build( 9, $$, generator_case_comb_cov( @4.ppfline, @4.first_column ),
                            $1, strdup_safe( "case(" ), $4, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @2.first_line, @2.first_column ) );
    }
  | cond_specifier_opt K_casex '(' expression ')' case_body K_endcase
    {
      $$ = generator_handle_push_funit( @2.first_line, @2.first_column );
      $$ = generator_build( 9, $$, generator_case_comb_cov( @4.ppfline, @4.first_column ),
                            $1, strdup_safe( "casex(" ), $4, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @2.first_line, @2.first_column ) );
    }
  | cond_specifier_opt K_casez '(' expression ')' case_body K_endcase
    {
      $$ = generator_handle_push_funit( @2.first_line, @2.first_column );
      $$ = generator_build( 9, $$, generator_case_comb_cov( @4.ppfline, @4.first_column ),
                            $1, strdup_safe( "casez(" ), $4, strdup_safe( ")" ), $6, strdup_safe( "endcase" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @2.first_line, @2.first_column ) );
    }
  | cond_specifier_opt K_if '(' expression ')' if_body
    {
      $$ = generator_handle_push_funit( @2.first_line, @2.first_column );
      $$ = generator_build( 8, $$, generator_line_cov( @2.ppfline, ((@5.last_line - @2.first_line) + @2.ppfline), @2.first_column, (@5.last_column - 1), TRUE ),
                            generator_comb_cov( @2.ppfline, @2.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "if(" ), $4, strdup_safe( ")" ), $6 );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @2.first_line, @2.first_column ) );
    }
  | cond_specifier_opt K_if '(' error ')' if_statement_error
    {
      GENerror( "Illegal conditional if expression" );
      $$ = NULL;
    }
  | K_for inc_for_depth '(' for_initialization ';' for_condition ';' passign ')' statement
    {
      char         str[50];
      char*        back;
      char*        rest;
      char*        for_cond_comb1 = generator_comb_cov( @6.ppfline, @6.first_column, FALSE, FALSE, FALSE, TRUE );
      char*        for_cond_comb2 = (for_cond_comb1 != NULL) ? strdup_safe( for_cond_comb1 ) : NULL;
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
      if( (strncmp( $10, "begin ", 6 ) != 0) && ($10[0] != ';') ) {
        $10 = generator_build( 5, strdup_safe( "begin" ), "\n", $10, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 23, strdup_safe( "begin" ), strdup_safe( str ), "\n", generator_inst_id_reg( funit ),
                            $4->cov,
                            generator_line_cov( @6.ppfline, ((@6.last_line - @6.first_line) + @6.ppfline), @6.first_column, (@6.last_column - 1), TRUE ),
                            strdup_safe( "for(" ), $4->str, strdup_safe( ";" ), $6, strdup_safe( ";" ), $8->str, strdup_safe( ") begin" ), "\n",
                            for_cond_comb1, $10,
                            generator_line_cov( @8.ppfline, ((@8.last_line - @8.first_line) + @8.ppfline), @8.first_column, (@8.last_column - 1), TRUE ),
                            $8->cov, strdup_safe( "end" ), "\n", for_cond_comb2, strdup_safe( "end" ), "\n" );
      generator_destroy2( $4 );
      generator_destroy2( $8 );
      generator_pop_funit();
    }
  | K_for inc_for_depth '(' for_initialization ';' for_condition ';' error ')' statement
    {
      $$ = NULL;
      generator_pop_funit();
    }
  | K_for inc_for_depth '(' for_initialization ';' error ';' passign ')' statement
    {
      $$ = NULL;
      generator_pop_funit();
    }
  | K_for inc_for_depth '(' error ')' statement
    {
      $$ = NULL;
      generator_pop_funit();
    }
  | K_while '(' expression ')' inc_block_depth statement dec_block_depth
    {
      if( (strncmp( $6, "begin ", 6 ) != 0) && ($6[0] != ';') ) {
        $6 = generator_build( 5, strdup_safe( "begin" ), "\n", $6, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 8, $$, generator_line_cov( @1.ppfline, ((@3.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@3.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, TRUE, TRUE ),
                            strdup_safe( "while(" ), $3, strdup_safe( ")" ), $6,
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, TRUE, FALSE ) );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | K_while '(' error ')' inc_block_depth statement dec_block_depth
    {
      $$ = NULL;
    }
  | K_do inc_block_depth statement dec_block_depth K_while '(' expression ')' ';'
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 9, $$, strdup_safe( "do" ), $3,
                            generator_line_cov( @6.ppfline, ((@6.last_line - @6.first_line) + @6.ppfline), @6.first_column, (@6.last_column - 1), TRUE ),
                            generator_comb_cov( @6.ppfline, @6.first_column, FALSE, TRUE, FALSE, TRUE ),
                            strdup_safe( "while(" ), $7, strdup_safe( ");" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | delay1 statement_or_null
    {
      if( (strncmp( $2, "begin ", 6 ) != 0) && ($2[0] != ';') ) {
        $2 = generator_build( 5, strdup_safe( "begin" ), "\n", $2, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 4, $$, generator_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE ),
                            $1, $2 );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | event_control statement_or_null
    {
      if( $2[0] == ';' ) {
        FREE_TEXT( $2 );
      }
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 9, $$, generator_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE ),
                            $1, strdup_safe( "begin" ), "\n", generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ), $2, strdup_safe( "end" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | '@' '*' statement_or_null
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 7, $$, generator_line_cov( @1.ppfline, ((@2.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@2.last_column - 1), TRUE ),
                            strdup_safe( "@* begin" ), "\n",
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                            $3, strdup_safe( "end" ) );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) ); 
    }
  | passign ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 6, $$, generator_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE ),
                            $1->cov, $1->str, strdup_safe( ";" ), "\n" );
      generator_destroy2( $1 );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | lpvalue K_LE expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 8, $$, generator_line_cov( @1.ppfline, ((@3.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@3.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "<=" ), $3, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | lpvalue '=' delay1 expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 9, $$, generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "=" ), $3, $4, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE delay1 expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 8, $$, generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "<=" ), $3, $4, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | lpvalue '=' event_control expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 9, $$,
                            generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "=" ), $3, $4, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE event_control expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 9, $$,
                            generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "<=" ), $3, $4, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | lpvalue '=' K_repeat '(' expression ')' event_control expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 11, $$,
                            generator_line_cov( @1.ppfline, ((@8.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@8.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "= repeat(" ), $5, strdup_safe( ")" ), $7, $8, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE K_repeat '(' expression ')' event_control expression ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 11, $$,
                            generator_line_cov( @1.ppfline, ((@8.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@8.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "<= repeat(" ), $5, strdup_safe( ")" ), $7, $8, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | K_wait '(' expression ')' statement_or_null
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 11, $$,
                            generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ),
                            strdup_safe( "wait(" ), $3, strdup_safe( ") begin" ), "\n",
                            generator_event_comb_cov( @1.ppfline, @1.first_column, TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, TRUE, FALSE, TRUE ), strdup_safe( "end" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | SYSCALL '(' expression_systask_list ')' ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "(" ), $3, strdup_safe( ");" ), "\n" );
    }
  | SYSCALL ';'
    {
      $$ = generator_build( 3, $1, strdup_safe( ";" ), "\n" );
    }
  | identifier '(' expression_port_list ')' ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 8, $$,
                            generator_line_cov( @1.ppfline, ((@4.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@4.last_column - 1), TRUE ),
                            generator_comb_cov( @1.ppfline, @1.first_column, FALSE, FALSE, FALSE, TRUE ),
                            $1, strdup_safe( "(" ), $3, strdup_safe( ");" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
  | identifier ';'
    {
      $$ = generator_handle_push_funit( @1.first_line, @1.first_column );
      $$ = generator_build( 5, $$,
                            generator_line_cov( @1.ppfline, ((@1.last_line - @1.first_line) + @1.ppfline), @1.first_column, (@1.last_column - 1), TRUE ),
                            $1, strdup_safe( ";" ), "\n" );
      $$ = generator_build( 2, $$, generator_handle_pop_funit( @1.first_line, @1.first_column ) );
    }
   /* Immediate SystemVerilog assertions are parsed but not performed -- we will not exclude a block that contains one */
  | K_assert ';'
    {
      $$ = generator_build( 3, $1, strdup_safe( ";" ), "\n" );
    }
   /* Inline SystemVerilog assertion -- parsed, not performed and we will not exclude a block that contains one */
  | IDENTIFIER ':' K_assert ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( ":" ), $3, strdup_safe( ";" ), "\n" );
    }
  | error ';'
    {
      GENerror( "Illegal statement" );
      $$ = NULL;
    }
  ;

fork_statement
  : begin_end_id block_item_decls_opt statement_list
    {
      func_unit* funit = db_get_tfn_by_position( @1.first_line, @1.first_column );
      assert( funit != NULL );
      printf( "In fork_statement, funit: %s\n", funit->name );
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
        $1 = generator_build( 1, strdup_safe( str ) );
        free_safe( back, (strlen( funit->name ) + 1) );
        free_safe( rest, (strlen( funit->name ) + 1) );
      }
      $$ = generator_build( 4, $1, generator_inst_id_reg( funit ), $2, $3 );
    }
  |
    {
      $$ = NULL;
    }
  ;

begin_end_block
  : begin_end_id
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @1.first_line, @1.first_column )) != NULL ) {
        generator_push_funit( funit );
      }
    }
    block_item_decls_opt statement_list
    {
      func_unit* funit;
      if( (funit = db_get_tfn_by_position( @1.first_line, @1.first_column )) != NULL ) {
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
          $1 = generator_build( 1, strdup_safe( str ) );
          free_safe( back, (strlen( funit->name ) + 1) );
          free_safe( rest, (strlen( funit->name ) + 1) );
        }
        generator_pop_funit();
      }
      $$ = generator_build( 5, $1, "\n", generator_inst_id_reg( funit ), $3, $4 );
    }
  | begin_end_id
    {
      $$ = $1;
    }
  ;

if_statement_error
  : statement_or_null %prec less_than_K_else
    {
      FREE_TEXT( $1 );
      $$ = NULL;
    }
  | statement_or_null K_else statement_or_null
    {
      FREE_TEXT( $1 );
      FREE_TEXT( $3 );
      $$ = NULL;
    }
  ;

statement_list
  : statement_list statement
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | statement
    {
      $$ = $1;
    }
  /* This rule is not in the LRM but seems to be supported by several simulators */
  | statement_list ';'
    {
      $$ = generator_build( 3, $1, strdup_safe( ";" ), "\n" );
    }
  /* This rule is not in the LRM but seems to be supported by several simulators */
  | ';'
    {
      $$ = generator_build( 2, strdup_safe( ";" ), "\n" );
    }
  ;

statement_or_null
  : statement
    {
      $$ = $1;
    }
  | ';'
    {
      $$ = generator_build( 2, strdup_safe( ";" ), "\n" );
    }
  ;

  /* An lpvalue is the expression that can go on the left side of a procedural assignment.
     This rule handles only procedural assignments. */
lpvalue
  : identifier
    {
      $$ = $1;
    }
  | identifier index_expr
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | '{' expression_list '}'
    {
      $$ = generator_build( 3, strdup_safe( "{" ), $2, strdup_safe( "}" ) );
    }
  ;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
  : identifier
    {
      $$ = $1;
    }
  | identifier index_expr
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | '{' expression_list '}'
    {
      $$ = generator_build( 3, strdup_safe( "{" ), $2, strdup_safe( "}" ) );
    }
  ;

block_item_decls_opt
  : block_item_decls
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

block_item_decls
  : block_item_decl
    {
      $$ = $1;
    }
  | block_item_decls block_item_decl
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

  /* The block_item_decl is used in function definitions, task
     definitions, module definitions and named blocks. Wherever a new
     scope is entered, the source may declare new registers and
     integers. This rule matches those declarations. The containing
     rule has presumably set up the scope. */
block_item_decl
  : attribute_list_opt K_reg signed_opt range_opt register_variable_list ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "reg" ), $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_bit signed_opt range_opt register_variable_list ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "bit" ), $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_logic signed_opt range_opt register_variable_list ';'
    {
      $$ = generator_build( 7, $1, strdup_safe( "logic" ), $3, $4, $5, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_char unsigned_opt register_variable_list ';'
    {
      $$ = generator_build( 6, $1, strdup_safe( "char" ), $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_byte unsigned_opt register_variable_list ';'
    {
      $$ = generator_build( 6, $1, strdup_safe( "byte" ), $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_shortint unsigned_opt register_variable_list ';'
    {
      $$ = generator_build( 6, $1, strdup_safe( "shortint" ), $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_integer unsigned_opt register_variable_list ';'
    {
      $$ = generator_build( 6, $1, strdup_safe( "integer" ), $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_int unsigned_opt register_variable_list ';'
    {
      $$ = generator_build( 6, $1, strdup_safe( "int" ), $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_longint unsigned_opt register_variable_list ';'
    {
      $$ = generator_build( 6, $1, strdup_safe( "longint" ), $3, $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_time register_variable_list ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "time" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt TYPEDEF_IDENTIFIER ';' register_variable_list ';'
    {
      $$ = generator_build( 5, $1, $2, strdup_safe( ";" ), $4, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_real list_of_variables ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "real" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_realtime list_of_variables ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "realtime" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_shortreal list_of_variables ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "shortreal" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_parameter parameter_assign_decl ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "parameter" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_localparam localparam_assign_decl ';'
    {
      $$ = generator_build( 5, $1, strdup_safe( "localparam" ), $3, strdup_safe( ";" ), "\n" );
    }
  | attribute_list_opt K_reg error ';'
    {
      GENerror( "Syntax error in reg variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_bit error ';'
    {
      GENerror( "Syntax error in bit variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_byte error ';'
    {
      GENerror( "Syntax error in byte variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_logic error ';'
    {
      GENerror( "Syntax error in logic variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_char error ';'
    {
      GENerror( "Syntax error in char variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_shortint error ';'
    {
      GENerror( "Syntax error in shortint variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_integer error ';'
    {
      GENerror( "Syntax error in integer variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_int error ';'
    {
      GENerror( "Syntax error in int variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_longint error ';'
    {
      GENerror( "Syntax error in longint variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_time error ';'
    {
      GENerror( "Syntax error in time variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_real error ';'
    {
      GENerror( "Syntax error in real variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_realtime error ';'
    {
      GENerror( "Syntax error in realtime variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_parameter error ';'
    {
      GENerror( "Syntax error in parameter variable list" );
      $$ = NULL;
    }
  | attribute_list_opt K_localparam error ';'
    {
      if( parser_check_generation( GENERATION_2001 ) ) {
        GENerror( "Syntax error in localparam list" );
      }
      $$ = NULL;
    }
  ;	

case_item
  : expression_list ':' statement_or_null
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 3, $1, strdup_safe( ":" ), $3 );
    }
  | K_default ':' statement_or_null
    {
      if( (strncmp( $3, "begin ", 6 ) != 0) && ($3[0] != ';') ) {
        $3 = generator_build( 5, strdup_safe( "begin" ), "\n", $3, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 2, strdup_safe( "default :" ), $3 );
    }
  | K_default statement_or_null
    {
      if( (strncmp( $2, "begin ", 6 ) != 0) && ($2[0] != ';') ) {
        $2 = generator_build( 5, strdup_safe( "begin" ), "\n", $2, strdup_safe( "end" ), "\n" );
      }
      $$ = generator_build( 2, strdup_safe( "default" ), $2 );
    }
  | error ':' statement_or_null
    {
      GENerror( "Illegal case expression" );
      $$ = NULL;
    }
  ;

case_items
  : case_items case_item
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | case_item
    {
      $$ = $1;
    }
  ;

case_body
  : inc_block_depth case_items dec_block_depth
    {
      $$ = $2;
    }
  | inc_block_depth error dec_block_depth
    {
      $$ = NULL;
    }
  ;

delay1
  : '#' delay_value_simple
    {
      $$ = generator_build( 2, strdup_safe( "#" ), $2 );
    }
  | '#' '(' delay_value ')'
    {
      $$ = generator_build( 3, strdup_safe( "#(" ), $3, strdup_safe( ")" ) );
    }
  ;

delay3
  : '#' delay_value_simple
    {
      $$ = generator_build( 2, strdup_safe( "#" ), $2 );
    }
  | '#' '(' delay_value ')'
    {
      $$ = generator_build( 3, strdup_safe( "#(" ), $3, strdup_safe( ")" ) );
    }
  | '#' '(' delay_value ',' delay_value ')'
    {
      $$ = generator_build( 5, strdup_safe( "#(" ), $3, strdup_safe( "," ), $5, strdup_safe( ")" ) );
    }
  | '#' '(' delay_value ',' delay_value ',' delay_value ')'
    {
      $$ = generator_build( 7, strdup_safe( "#(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, strdup_safe( ")" ) );
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
      $$ = $1;
    }
  | static_expr ':' static_expr ':' static_expr
    {
      $$ = generator_build( 5, $1, strdup_safe( ":" ), $3, strdup_safe( ":" ), $5 );
    }
  ;

delay_value_simple
  : DEC_NUMBER
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
  ;

assign_list
  : assign_list ',' assign
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | assign
    {
      $$ = $1;
    }
  ;

assign
  : lavalue '=' expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, TRUE, TRUE, FALSE, TRUE ), $1, strdup_safe( "=" ), $3 );
    }
  ;

range_opt
  : range
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

range
  : '[' static_expr ':' static_expr ']'
    {
      $$ = generator_build( 5, strdup_safe( "[" ), $2, strdup_safe( ":" ), $4, strdup_safe( "]" ) );
    }
  | range '[' static_expr ':' static_expr ']'
    {
      $$ = generator_build( 6, $1, strdup_safe( "[" ), $3, strdup_safe( ":" ), $5, strdup_safe( "]" ) );
    }
  ;

range_or_type_opt
  : range      
    {
      $$ = $1;
    }
  | K_integer
    {
      $$ = strdup_safe( "integer" );
    }
  | K_real
    {
      $$ = strdup_safe( "real" );
    }
  | K_realtime
    {
      $$ = strdup_safe( "realtime" );
    }
  | K_time
    {
      $$ = strdup_safe( "time" );
    }
  |
    {
      $$ = NULL;
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
  | IDENTIFIER '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  | IDENTIFIER range
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

register_variable_list
  : register_variable
    {
      $$ = $1;
    }
  | register_variable_list ',' register_variable
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

task_item_list_opt
  : task_item_list
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

task_item_list
  : task_item_list task_item
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | task_item
    {
      $$ = $1;
    }
  ;

task_item
  : block_item_decl
    {
      $$ = $1;
    }
  | port_type range_opt list_of_variables ';'
    {
      $$ = generator_build( 5, $1, $2, $3, strdup_safe( ";" ), "\n" );
    }
  ;

automatic_opt
  : K_automatic { $$ = strdup_safe( "automatic" ); }
  |             { $$ = NULL;                       }
  ;

signed_opt
  : K_signed   { $$ = strdup_safe( "signed" );   }
  | K_unsigned { $$ = strdup_safe( "unsigned" ); }
  |            { $$ = NULL;                      }
  ;

unsigned_opt
  : K_unsigned { $$ = strdup_safe( "unsigned" ); }
  | K_signed   { $$ = strdup_safe( "signed" );   }
  |            { $$ = NULL;  }
  ;

  /*
   The net_type_sign_range_opt is only used in port lists so don't set the curr_sig_type field
   as this will already be filled in by the port_type rule.
  */
net_type_sign_range_opt
  : K_wire signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "wire" ), $2, $3 );
    }
  | K_tri signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "tri" ), $2, $3 );
    }
  | K_tri1 signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "tri1" ), $2, $3 );
    }
  | K_supply0 signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "supply0" ), $2, $3 );
    }
  | K_wand signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "wand" ), $2, $3 );
    }
  | K_triand signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "triand" ), $2, $3 );
    }
  | K_tri0 signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "tri0" ), $2, $3 );
    }
  | K_supply1 signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "supply1" ), $2, $3 );
    }
  | K_wor signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "wor" ), $2, $3 );
    }
  | K_trior signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "trior" ), $2, $3 );
    }
  | K_logic signed_opt range_opt
    {
      $$ = generator_build( 3, strdup_safe( "logic" ), $2, $3 );
    }
  | TYPEDEF_IDENTIFIER
    {
      $$ = $1;
    }
  | signed_opt range_opt
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

net_type
  : K_wire    { $$ = strdup_safe( "wire" );    }
  | K_tri     { $$ = strdup_safe( "tri" );     }
  | K_tri1    { $$ = strdup_safe( "tri1" );    }
  | K_supply0 { $$ = strdup_safe( "supply0" ); }
  | K_wand    { $$ = strdup_safe( "wand" );    }
  | K_triand  { $$ = strdup_safe( "triand" );  }
  | K_tri0    { $$ = strdup_safe( "tri0" );    }
  | K_supply1 { $$ = strdup_safe( "supply1" ); }
  | K_wor     { $$ = strdup_safe( "wor" );     }
  | K_trior   { $$ = strdup_safe( "trior" );   }
  ;

var_type
  : K_reg     { $$ = strdup_safe( "reg" ); }
  ;

net_decl_assigns
  : net_decl_assigns ',' net_decl_assign
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | net_decl_assign
    {
      $$ = $1;
    }
  ;

  /* START_HERE */
net_decl_assign
  : IDENTIFIER '=' expression
    {
      $$ = generator_build( 4, generator_comb_cov( @1.ppfline, @1.first_column, TRUE, TRUE, FALSE, TRUE ),
                            $1, strdup_safe( "=" ), $3 ); 
    }
  | delay1 IDENTIFIER '=' expression
    {
      $$ = generator_build( 5, generator_comb_cov( @1.ppfline, @1.first_column, TRUE, TRUE, FALSE, TRUE ),
                            $1, $2, strdup_safe( "=" ), $4 );
    }
  ;

drive_strength_opt
  : drive_strength
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

drive_strength
  : '(' dr_strength0 ',' dr_strength1 ')'
    {
      $$ = generator_build( 5, strdup_safe( "(" ), $2, strdup_safe( "," ), $4, strdup_safe( ")" ) );
    }
  | '(' dr_strength1 ',' dr_strength0 ')'
    {
      $$ = generator_build( 5, strdup_safe( "(" ), $2, strdup_safe( "," ), $4, strdup_safe( ")" ) );
    }
  | '(' dr_strength0 ',' K_highz1 ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ", highz1 )" ) );
    }
  | '(' dr_strength1 ',' K_highz0 ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ", highz0 )" ) );
    }
  | '(' K_highz1 ',' dr_strength0 ')'
    {
      $$ = generator_build( 3, strdup_safe( "( highz1," ), $4, strdup_safe( ")" ) );
    }
  | '(' K_highz0 ',' dr_strength1 ')'
    {
      $$ = generator_build( 3, strdup_safe( "( highz0," ), $4, strdup_safe( ")" ) );
    }
  ;

dr_strength0
  : K_supply0 { $$ = strdup_safe( "supply0" ); }
  | K_strong0 { $$ = strdup_safe( "strong0" ); }
  | K_pull0   { $$ = strdup_safe( "pull0" );   }
  | K_weak0   { $$ = strdup_safe( "weak0" );   }
  ;

dr_strength1
  : K_supply1 { $$ = strdup_safe( "supply1" ); }
  | K_strong1 { $$ = strdup_safe( "strong1" ); }
  | K_pull1   { $$ = strdup_safe( "pull1" );   }
  | K_weak1   { $$ = strdup_safe( "weak1" );   }
  ;

event_control
  : '@' IDENTIFIER
    {
      $$ = generator_build( 2, strdup_safe( "@" ), $2 );
    }
  | '@' '(' event_expression_list ')'
    {
      @$.first_line   = @3.first_line;
      @$.last_line    = @3.last_line;
      @$.ppfline      = @3.ppfline;
      @$.pplline      = @3.pplline;
      @$.first_column = @3.first_column;
      @$.last_column  = @3.last_column;
      $$ = generator_build( 3, strdup_safe( "@(" ), $3, strdup_safe( ")" ) );
    }
  | '@' '(' error ')'
    {
      GENerror( "Illegal event control expression" );
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
      $$ = generator_build( 3, $1, strdup_safe( "or" ), $3 );
    }
  | event_expression_list ',' event_expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

event_expression
  : K_posedge expression
    {
      $$ = generator_build( 2, strdup_safe( "posedge" ), $2 );
    }
  | K_negedge expression
    {
      $$ = generator_build( 2, strdup_safe( "negedge" ), $2 );
    }
  | expression
    {
      $$ = $1;
    }
  ;
		
charge_strength_opt
  : charge_strength
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

charge_strength
  : '(' K_small ')'  { $$ = strdup_safe( "(small)" );  }
  | '(' K_medium ')' { $$ = strdup_safe( "(medium)" ); }
  | '(' K_large ')'  { $$ = strdup_safe( "(large)" );  }
  ;

port_type
  : K_input  { $$ = strdup_safe( "input" );  }
  | K_output { $$ = strdup_safe( "output" ); }
  | K_inout  { $$ = strdup_safe( "inout" );  }
  ;

defparam_assign_list
  : defparam_assign
    {
      $$ = $1;
    }
  | range defparam_assign
    {
      $$ = generator_build( 2, $1, $2 );
    }
  | defparam_assign_list ',' defparam_assign
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

defparam_assign
  : identifier '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  ;

 /* Parameter override */
parameter_value_opt
  : '#' '(' expression_list ')'
    {
      $$ = generator_build( 3, strdup_safe( "#(" ), $3, strdup_safe( ")" ) );
    }
  | '#' '(' parameter_value_byname_list ')'
    {
      $$ = generator_build( 3, strdup_safe( "#(" ), $3, strdup_safe( ")" ) );
    }
  | '#' DEC_NUMBER
    {
      $$ = generator_build( 2, strdup_safe( "#" ), $2 );
    }
  | '#' error
    {
      GENerror( "Syntax error in parameter value assignment list" );
      $$ = NULL;
    }
  |
    {
      $$ = NULL;
    }
  ;

parameter_value_byname_list
  : parameter_value_byname
    {
      $$ = $1;
    }
  | parameter_value_byname_list ',' parameter_value_byname
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

parameter_value_byname
  : '.' IDENTIFIER '(' expression ')'
    {
      $$ = generator_build( 5, strdup_safe( "." ), $2, strdup_safe( "(" ), $4, strdup_safe( ")" ) );
    }
  | '.' IDENTIFIER '(' ')'
    {
      $$ = generator_build( 3, strdup_safe( "." ), $2, strdup_safe( "()" ) );
    }
  ;

gate_instance_list
  : gate_instance_list ',' gate_instance
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | gate_instance
    {
      $$ = $1;
    }
  ;

  /* A gate_instance is a module instantiation or a built in part
     type. In any case, the gate has a set of connections to ports. */
gate_instance
  : IDENTIFIER '(' expression_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" ) );
    }
  | IDENTIFIER range '(' expression_list ')'
    {
      $$ = generator_build( 5, $1, $2, strdup_safe( "(" ), $4, strdup_safe( ")" ) );
    }
  | IDENTIFIER '(' port_name_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "(" ), $3, strdup_safe( ")" ) );
    }
  | IDENTIFIER range '(' port_name_list ')'
    {
      $$ = generator_build( 5, $1, $2, strdup_safe( "(" ), $4, strdup_safe( ")" ) );
    }
  | '(' expression_list ')'
    {
      $$ = generator_build( 3, strdup_safe( "(" ), $2, strdup_safe( ")" ) );
    }
  ;

gatetype
  : K_and       { $$ = strdup_safe( "and" );      }
  | K_nand      { $$ = strdup_safe( "nand" );     }
  | K_or        { $$ = strdup_safe( "or" );       }
  | K_nor       { $$ = strdup_safe( "nor" );      }
  | K_xor       { $$ = strdup_safe( "xor" );      }
  | K_xnor      { $$ = strdup_safe( "xnor" );     }
  | K_buf       { $$ = strdup_safe( "buf" );      }
  | K_bufif0    { $$ = strdup_safe( "bufif0" );   }
  | K_bufif1    { $$ = strdup_safe( "bufif1" );   }
  | K_not       { $$ = strdup_safe( "not" );      }
  | K_notif0    { $$ = strdup_safe( "notif0" );   }
  | K_notif1    { $$ = strdup_safe( "notif1" );   }
  | K_nmos      { $$ = strdup_safe( "nmos" );     }
  | K_rnmos     { $$ = strdup_safe( "rnmos" );    }
  | K_pmos      { $$ = strdup_safe( "pmos" );     }
  | K_rpmos     { $$ = strdup_safe( "rpmos" );    }
  | K_cmos      { $$ = strdup_safe( "cmos" );     }
  | K_rcmos     { $$ = strdup_safe( "rcmos" );    }
  | K_tran      { $$ = strdup_safe( "tran" );     }
  | K_rtran     { $$ = strdup_safe( "rtran" );    }
  | K_tranif0   { $$ = strdup_safe( "tranif0" );  }
  | K_tranif1   { $$ = strdup_safe( "tranif1" );  }
  | K_rtranif0  { $$ = strdup_safe( "rtranif0" ); }
  | K_rtranif1  { $$ = strdup_safe( "rtranif1" ); }
  ;

  /* A function_item_list only lists the input/output/inout
     declarations. The integer and reg declarations are handled in
     place, so are not listed. The list builder needs to account for
     the possibility that the various parts may be NULL. */
function_item_list
  : function_item
    {
      $$ = $1;
    }
  | function_item_list function_item
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

function_item
  : K_input signed_opt range_opt list_of_variables ';'
    {
      $$ = generator_build( 6, strdup_safe( "input" ), $2, $3, $4, strdup_safe( ";" ), "\n" );
    }
  | block_item_decl
    {
      $$ = $1;
    }
  ;

parameter_assign_decl
  : signed_opt range_opt parameter_assign_list
    {
      $$ = generator_build( 3, $1, $2, $3 );
    }
  ;

parameter_assign_list
  : parameter_assign
    {
      $$ = $1;
    }
  | parameter_assign_list ',' parameter_assign
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

parameter_assign
  : IDENTIFIER '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  ;

localparam_assign_decl
  : signed_opt range_opt localparam_assign_list
    {
      $$ = generator_build( 3, $1, $2, $3 );
    }
  ;

localparam_assign_list
  : localparam_assign
    {
      $$ = $1;
    }
  | localparam_assign_list ',' localparam_assign
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

localparam_assign
  : IDENTIFIER '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  ;

port_name_list
  : port_name_list ',' port_name
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | port_name
    {
      $$ = $1;
    }
  ;

port_name
  : '.' IDENTIFIER '(' expression ')'
    {
      $$ = generator_build( 5, strdup_safe( "." ), $2, strdup_safe( "(" ), $4, strdup_safe( ")" ) );
    }
  | '.' IDENTIFIER '(' error ')'
    {
      FREE_TEXT( $2 );
      $$ = NULL;
    }
  | '.' IDENTIFIER '(' ')'
    {
      $$ = generator_build( 3, strdup_safe( "." ), $2, strdup_safe( "()" ) );
    }
  | '.' IDENTIFIER
    {
      if( !parser_check_generation( GENERATION_SV ) ) {
        GENerror( "Implicit .name port list item found in block that is specified to not allow SystemVerilog syntax" );
        $$ = NULL;
      } else {
        $$ = generator_build( 2, strdup_safe( "." ), $2 );
      }
    }
  | K_PS
    {
      if( !parser_check_generation( GENERATION_SV ) ) {
        GENerror( "Implicit .* port list item found in block that is specified to not allow SystemVerilog syntax" );
        $$ = NULL;
      } else {
        $$ = strdup_safe( ".*" );
      }
    }
  ;

specify_item_list
  : specify_item
    {
      $$ = $1;
    }
  | specify_item_list specify_item
    {
      $$ = generator_build( 2, $1, $2 );
    }
  ;

specify_item
  : K_specparam specparam_list ';'
    {
      $$ = generator_build( 4, strdup_safe( "specparam" ), $2, strdup_safe( ";" ), "\n" );
    }
  | specify_simple_path_decl ';'
    {
      $$ = generator_build( 3, $1, strdup_safe( ";" ), "\n" );
    }
  | specify_edge_path_decl ';'
    {
      $$ = generator_build( 3, $1, strdup_safe( ";" ), "\n" );
    }
  | K_if '(' expression ')' specify_simple_path_decl ';'
    {
      $$ = generator_build( 6, strdup_safe( "if(" ), $3, strdup_safe( ")" ), $5, strdup_safe( ";" ), "\n" );
    }
  | K_if '(' expression ')' specify_edge_path_decl ';'
    {
      $$ = generator_build( 6, strdup_safe( "if(" ), $3, strdup_safe( ")" ), $5, strdup_safe( ";" ), "\n" );
    }
  | K_Shold '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
    {
      $$ = generator_build( 8, strdup_safe( "$hold(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, strdup_safe( ");" ), "\n" );
    }
  | K_Speriod '(' spec_reference_event ',' expression spec_notifier_opt ')' ';'
    {
      $$ = generator_build( 6, strdup_safe( "$period(" ), $3, strdup_safe( "," ), $5, strdup_safe( ");" ), "\n" );
    }
  | K_Srecovery '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
    {
      $$ = generator_build( 9, strdup_safe( "$recovery(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, $8, strdup_safe( ");" ), "\n" );
    }
  | K_Ssetup '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
    {
      $$ = generator_build( 9, strdup_safe( "$setup(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, $8, strdup_safe( ");" ), "\n" );
    }
  | K_Ssetuphold '(' spec_reference_event ',' spec_reference_event ',' expression ',' expression spec_notifier_opt ')' ';'
    {
      $$ = generator_build( 11, strdup_safe( "$setuphold(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, strdup_safe( "," ), $9, $10, strdup_safe( ");" ), "\n" );
    }
  | K_Swidth '(' spec_reference_event ',' expression ',' expression spec_notifier_opt ')' ';'
    {
      $$ = generator_build( 9, strdup_safe( "$width(" ), $3, strdup_safe( "," ), $5, strdup_safe( "," ), $7, $8, strdup_safe( ");" ), "\n" );
    }
  | K_Swidth '(' spec_reference_event ',' expression ')' ';'
    {
      $$ = generator_build( 6, strdup_safe( "$width(" ), $3, strdup_safe( "," ), $5, strdup_safe( ");" ), "\n" );
    }
  ;

specparam_list
  : specparam
    {
      $$ = $1;
    }
  | specparam_list ',' specparam
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  ;

specparam
  : IDENTIFIER '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  | IDENTIFIER '=' expression ':' expression ':' expression
    {
      $$ = generator_build( 7, $1, strdup_safe( "=" ), $3, strdup_safe( ":" ), $5, strdup_safe( ":" ), $7 );
    }
  | PATHPULSE_IDENTIFIER '=' expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  | PATHPULSE_IDENTIFIER '=' '(' expression ',' expression ')'
    {
      $$ = generator_build( 6, $1, strdup_safe( "= (" ), $4, strdup_safe( "," ), $6, strdup_safe( ")" ) );
    }
  ;
  
specify_simple_path_decl
  : specify_simple_path '=' '(' specify_delay_value_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "= (" ), $4, strdup_safe( ")" ) );
    }
  | specify_simple_path '=' delay_value_simple
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  | specify_simple_path '=' '(' error ')'
    {
      $$ = NULL;
    }
  ;

specify_simple_path
  : '(' specify_path_identifiers spec_polarity K_EG expression ')'
    {
      $$ = generator_build( 6, strdup_safe( "(" ), $2, $3, strdup_safe( "=>" ), $5, strdup_safe( ")" ) );
    }
  | '(' specify_path_identifiers spec_polarity K_SG expression ')'
    {
      $$ = generator_build( 6, strdup_safe( "(" ), $2, $3, strdup_safe( "*>" ), $5, strdup_safe( ")" ) );
    }
  | '(' error ')'
    {
      $$ = NULL;
    }
  ;

specify_path_identifiers
  : IDENTIFIER
    {
      $$ = $1;
    }
  | IDENTIFIER '[' expr_primary ']'
    {
      $$ = generator_build( 4, $1, strdup_safe( "[" ), $3, strdup_safe( "]" ) );
    }
  | specify_path_identifiers ',' IDENTIFIER
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | specify_path_identifiers ',' IDENTIFIER '[' expr_primary ']'
    {
      $$ = generator_build( 6, $1, strdup_safe( "," ), $3, strdup_safe( "[" ), $5, strdup_safe( "]" ) );
    }
  ;

spec_polarity
  : '+' { $$ = strdup_safe( "+" ); }
  | '-' { $$ = strdup_safe( "-" ); }
  |     { $$ = NULL;               }
  ;

spec_reference_event
  : K_posedge expression
    {
      $$ = generator_build( 2, strdup_safe( "posedge" ), $2 );
    }
  | K_negedge expression
    {
      $$ = generator_build( 2, strdup_safe( "negedge" ), $2 );
    }
  | K_posedge expr_primary K_TAND expression
    {
      $$ = generator_build( 4, strdup_safe( "posedge" ), $2, strdup_safe( "&&&" ), $4 );
    }
  | K_negedge expr_primary K_TAND expression
    {
      $$ = generator_build( 4, strdup_safe( "negedge" ), $2, strdup_safe( "&&&" ), $4 );
    }
  | expr_primary K_TAND expression
    {
      $$ = generator_build( 3, $1, strdup_safe( "&&&" ), $3 );
    }
  | expr_primary
    {
      $$ = $1;
    }
  ;

spec_notifier_opt
  :
    {
      $$ = NULL;
    }
  | spec_notifier
    {
      $$ = $1;
    }
  ;

spec_notifier
  : ','
    {
      $$ = strdup_safe( "," );
    }
  | ','  identifier
    {
      $$ = generator_build( 2, strdup_safe( "," ), $2 );
    }
  | spec_notifier ',' 
    {
      $$ = generator_build( 2, $1, strdup_safe( "," ) );
    }
  | spec_notifier ',' identifier
    {
      $$ = generator_build( 3, $1, strdup_safe( "," ), $3 );
    }
  | IDENTIFIER
    {
      $$ = $1;
    }
  ;

specify_edge_path_decl
  : specify_edge_path '=' '(' specify_delay_value_list ')'
    {
      $$ = generator_build( 4, $1, strdup_safe( "= (" ), $4, strdup_safe( ")" ) );
    }
  | specify_edge_path '=' delay_value_simple
    {
      $$ = generator_build( 3, $1, strdup_safe( "=" ), $3 );
    }
  ;

specify_edge_path
  : '(' K_posedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
      $$ = generator_build( 6, strdup_safe( "(posedge" ), $3, $4, strdup_safe( "=>" ), $6, strdup_safe( ")" ) );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
    {
      $$ = generator_build( 8, strdup_safe( "(posedge" ), $3, $4, strdup_safe( "=> (" ), $7, $8, $9, strdup_safe( ")" ) );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
      $$ = generator_build( 6, strdup_safe( "(posedge" ), $3, $4, strdup_safe( "*>" ), $6, strdup_safe( ")" ) );
    }
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
    {
      $$ = generator_build( 8, strdup_safe( "(posedge" ), $3, $4, strdup_safe( "*> (" ), $7, $8, $9, strdup_safe( "))" ) );
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG IDENTIFIER ')'
    {
      $$ = generator_build( 6, strdup_safe( "(negedge" ), $3, $4, strdup_safe( "=>" ), $6, strdup_safe( ")" ) );
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
    {
      $$ = generator_build( 8, strdup_safe( "(negedge" ), $3, $4, strdup_safe( "=> (" ), $7, $8, $9, strdup_safe( ")" ) );
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG IDENTIFIER ')'
    {
      $$ = generator_build( 6, strdup_safe( "(negedge" ), $3, $4, strdup_safe( "*>" ), $6, strdup_safe( ")" ) );
    }
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
    {
      $$ = generator_build( 8, strdup_safe( "(negedge" ), $3, $4, strdup_safe( "*> (" ), $7, $8, $9, strdup_safe( "))" ) );
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
      $$ = generator_build( 2, strdup_safe( "reg" ), $2 );
    }
  | K_logic range_opt
    {
      $$ = generator_build( 2, strdup_safe( "logic" ), $2 );
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

inc_block_depth
  :
    {
      block_depth++;
    }
  ;

dec_block_depth
  :
    {
      block_depth--;
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

inc_for_depth
  :
    {
      func_unit* funit = db_get_tfn_by_position( @$.first_line, (@$.first_column - 3) );
      assert( funit != NULL );
      generator_push_funit( funit );
    }
  ;
