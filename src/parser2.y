
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

#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "link.h"
#include "obfuscate.h"
#include "parser_misc.h"
#include "statement.h"
#include "static.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"

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
  int             integer;
  vector*         num;
  double          realtime;
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

%token <text>     IDENTIFIER SYSTEM_IDENTIFIER C_IDENTIFIER SIMPLE_IDENTIFIER ESCAPED_IDENTIFIER
%token <typdef>   TYPEDEF_IDENTIFIER
%token <text>     PATHPULSE_IDENTIFIER
%token <num>      NUMBER
%token <realtime> REALTIME
%token <num>      STRING
%token IGNORE
%token UNUSED_IDENTIFIER UNUSED_C_IDENTIFIER UNUSED_SIMPLE_IDENTIFIER UNUSED_ESCAPED_IDENTIFIER
%token UNUSED_PATHPULSE_IDENTIFIER
%token UNUSED_NUMBER
%token UNUSED_REALTIME
%token UNUSED_STRING UNUSED_SYSTEM_IDENTIFIER
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_RS K_SG K_PP K_PS K_PEQ K_PNE K_RSS K_LSS
%token K_ADD_A K_SUB_A K_MLT_A K_DIV_A K_MOD_A K_AND_A K_OR_A K_XOR_A K_LS_A K_RS_A K_ALS_A K_ARS_A K_INC K_DEC K_POW
%token K_PO_POS K_PO_NEG K_STARP K_PSTAR
%token K_LOR K_LAND K_NAND K_NOR K_NXOR K_TRIGGER
%token K_TU_S K_TU_MS K_TU_US K_TU_NS K_TU_PS K_TU_FS K_TU_STEP
%token K_INCDIR
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
%token K_pull0 K_pull1 K_pulldown K_pullup K_rcmos K_shortreal K_real K_realtime
%token K_reg K_release K_repeat
%token K_rnmos K_rpmos K_rtran K_rtranif0 K_rtranif1
%token K_signed K_small K_specify
%token K_specparam K_strong0 K_strong1 K_supply0 K_supply1 K_table K_task
%token K_time K_tran K_tranif0 K_tranif1 K_tri K_tri0 K_tri1 K_triand
%token K_trior K_trireg K_vectored K_wait K_wand K_weak0 K_weak1
%token K_while K_wire
%token K_wor K_xnor K_xor
%token K_Shold K_Speriod K_Srecovery K_Ssetup K_Swidth K_Ssetuphold K_Sremoval K_Srecrem K_Sskew K_Stimeskew K_Sfullskew K_Snochange
%token K_automatic K_cell K_use K_library K_config K_endconfig K_design K_liblist K_instance
%token K_showcancelled K_noshowcancelled K_pulsestyle_onevent K_pulsestyle_ondetect

%token K_bit K_byte K_logic K_shortint K_int K_longint K_unsigned
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
%token K_DPI

%token K_TAND
%right '?' ':'
%left K_LOR
%left K_LAND
%left '|'
%left '^' K_NXOR K_NOR
%left '&' K_NAND
%left K_EQ K_NE K_CEQ K_CNE
%left K_GE K_LE '<' '>'
%left K_LS K_RS K_RSS K_LSS
%left '+' '-'
%left '*' '/' '%'
%left UNARY_PREC

/* to resolve dangling else ambiguity: */
%nonassoc K_else

%%

  /* SOURCE TEXT */
main
  : source_text
  | library_text
  |
  ;

  /* Library source text - CURRENTLY NOT SUPPORTED - TBD */

library_text
  : library_descriptions
  ;

library_description
  : library_declaration
  | include_statement
  | config_declaration
  ;

library_descriptions
  : library_description
  | library_descriptions library_description
  ;

library_declaration
  : K_library identifier file_path_specs { lex_start_library_options(); } incdir_opt { lex_end_library_options(); }
  ;

file_path_spec
  : STRING        { free_safe( $1 ); }
  | UNUSED_STRING
  ;

file_path_specs
  : file_path_spec
  | file_path_specs ',' file_path_spec
  ;

include_statement
  : K_include file_path_spec ; 
  ;

incdir_opt
  : K_INCDIR file_path_specs
  |
  ;

  /* Configuration source text - CURRENTLY NOT SUPPORTED - TBD */

config_declaration
  : K_config identifier ';' design_statement config_rule_statements_opt K_endconfig postfix_identifier_opt
  ;

design_statement
  : K_design inst_names_opt ';'
  ;

config_rule_statement
  : default_clause liblist_clause
  | inst_clause liblist_clause
  | inst_clause use_clause
  | cell_clause liblist_clause
  | cell_clause use_clause
  ;

config_rule_statements
  : config_rule_statement
  | config_rule_statements config_rule_statement
  ;

config_rule_statements_opt
  : config_rule_statements
  |
  ;

default_clause
  : K_default
  ;

inst_clause
  : K_instance inst_name
  ;

inst_name
  : identifier instance_identifiers_opt
  ;

inst_names
  : inst_name
  | inst_names inst_name
  ;

inst_names_opt
  : inst_names
  |
  ;

instance_identifiers
  : '.' identifier
  | instance_identifiers '.' identifier
  ;

instance_identifiers_opt
  : instance_identifiers
  |
  ;

cell_clause
  : K_cell inst_name
  ;

liblist_clause
  : K_liblist identifiers_opt
  ;

use_clause
  : K_use inst_name
  | K_use inst_name ':' K_config
  ;

  /* Configuration source text */

source_text
  : timeunits_declaration descriptions
  | descriptions
  ;

description
  : module_declaration
  | udp_declaration
  | interface_declaration
  | program_declaration
  | package_declaration
  | attribute_instances_opt package_item
  | attribute_instances_opt bind_directive
  ;

descriptions
  : description
  | descriptions description
  ;

module_nonansi_header
  : attribute_instances_opt module_keyword lifetime_opt identifier parameter_port_list_opt list_of_ports ';'
  ;

module_ansi_header
  : attribute_instances_opt module_keyword lifetime_opt identifier parameter_port_list_opt list_of_port_declarations_opt ';'
  ;

module_declaration
  : module_nonansi_header timeunits_declaration_opt module_items_opt K_endmodule postfix_identifier_opt
  | module_ansi_header timeunits_declaration_opt non_port_module_items_opt K_endmodule postfix_identifier_opt
  | attribute_instances_opt module_keyword lifetime_opt identifier '(' K_PS ')' ';' timeunits_declaration_opt module_items_opt K_endmodule postfix_identifier_opt
  | K_extern module_nonansi_header
  | K_extern module_ansi_header
  ;

module_keyword
  : K_module
  | K_macromodule
  ;

interface_nonansi_header
  : attribute_instances_opt K_interface lifetime_opt identifier parameter_port_list_opt list_of_ports ';'
  ;

interface_ansi_header
  : attribute_instances_opt K_interface lifetime_opt identifier parameter_port_list_opt list_of_port_declarations_opt ';' 
  ;

interface_declaration
  : interface_nonansi_header timeunits_declaration_opt interface_items_opt K_endinterface postfix_identifier_opt
  | interface_ansi_header timeunits_declaration_opt non_port_interface_items_opt K_endinterface postfix_identifier_opt
  | attribute_instances_opt K_interface identifier '(' K_PS ')' ';' timeunits_declaration_opt interface_items_opt K_endinterface postfix_identifier_opt
  | K_extern interface_nonansi_header
  | K_extern interface_ansi_header
  ;

program_nonansi_header
  : attribute_instances_opt K_program lifetime_opt identifier parameter_port_list_opt list_of_ports ';'
  ;

program_ansi_header
  : attribute_instances_opt K_program lifetime_opt identifier parameter_port_list_opt list_of_port_declarations_opt ';'
  ;

program_declaration
  : program_nonansi_header timeunits_declaration_opt program_items_opt K_endprogram postfix_identifier_opt
  | program_ansi_header timeunits_declaration_opt non_port_program_items_opt K_endprogram postfix_identifier_opt
  | attribute_instances_opt K_program identifier '(' K_PS ')' ';' timeunits_declaration_opt program_items_opt K_endprogram postfix_identifier_opt
  | K_extern program_nonansi_header
  | K_extern program_ansi_header
  ;

class_declaration
  : K_class lifetime_opt identifier parameter_port_list_opt class_extension_opt ';' class_items_opt K_endclass postfix_identifier_opt
  | K_virtual K_class lifetime_opt identifier parameter_port_list_opt class_extension_opt ';' class_items_opt K_endclass postfix_identifier_opt
  ;

class_extension_opt
  : K_extends class_type list_of_arguments_opt
  |
  ;

package_declaration
  : attribute_instances_opt K_package identifier ';' timeunits_declaration_opt package_attrib_items_opt K_endpackage postfix_identifier_opt
  ;

package_attrib_item
  : attribute_instances_opt package_item
  ;

package_attrib_items
  : package_attrib_item
  | package_attrib_items package_attrib_items
  ;

package_attrib_items_opt
  : package_attrib_items
  |
  ;

timeunits_declaration
  : K_timeunit time_literal ';'
  | K_timeprecision time_literal ';'
  | K_timeunit time_literal ';' K_timeprecision time_literal ';'
  | K_timeprecision time_literal ';' K_timeunit time_literal ';'
  ;

timeunits_declaration_opt
  : timeunits_declaration
  |
  ;

  /* Module parameters and ports */
parameter_port_list
  : '#' '(' list_of_param_assignments parameter_port_list_declarations_opt ')'
  | '#' '(' parameter_port_declaration parameter_port_list_declarations_opt ')'

parameter_port_list_opt
  : parameter_port_list
  |
  ;

parameter_port_declaration
  : parameter_declaration
  | data_type list_of_param_assignments
  | K_type list_of_type_assignments
  ;

parameter_port_list_declaration
  : ',' parameter_port_declaration
  ;

parameter_port_list_declarations
  : parameter_port_list_declaration
  | parameter_port_list_declarations parameter_port_list_declaration
  ;

parameter_port_list_declarations_opt
  : parameter_port_list_declarations
  |
  ;

list_of_ports
  : '(' port port_lists_opt ')'
  ;

port_list
  : ',' port
  ;

port_lists
  : port_list
  | port_lists port_list
  ;

port_lists_opt
  : port_lists
  |
  ;

list_of_port_declarations
  : '(' port_declarations_opt ')'
  ;

list_of_port_declarations_opt
  : list_of_port_declarations
  |
  ;

port_declarations_opt
  : attribute_instances_opt ansi_port_declaration ansi_port_declarations_opt
  |
  ;

port_declaration
  : attribute_instances_opt inout_declaration
  | attribute_instances_opt input_declaration
  | attribute_instances_opt output_declaration
  | attribute_instances_opt ref_declaration
  | attribute_instances_opt interface_port_declaration
  ;

port
  : port_expression_opt
  | '.' identifier '(' port_expression_opt ')'

port_expression
  : port_reference
  | '{' port_reference port_reference_list_opt '}'
  ;

port_expression_opt
  : port_expression
  |
  ;

port_reference
  : identifier constant_select
  ;

port_reference_list
  : ',' port_reference
  | port_reference_list ',' port_reference
  ;

port_reference_list_opt
  : port_reference_list
  |
  ;

port_direction
  : K_input
  | K_output
  | K_inout
  | K_ref
  ;

port_direction_opt
  : port_direction
  |
  ;

net_port_header
  : port_direction_opt port_type
  ;

variable_port_header
  : port_direction_opt data_type
  ;

interface_port_header
  : identifier
  | identifier '.' identifier
  | K_interface
  | K_interface '.' identifier
  ;

ansi_port_declaration
  : identifier unpacked_dimensions_opt
  | net_port_header identifier unpacked_dimensions_opt
  | interface_port_header identifier unpacked_dimensions_opt
  | identifier variable_dimension
  | variable_port_header identifier variable_dimension
  | identifier variable_dimension '=' constant_expression
  | variable_port_header identifier variable_dimension '=' constant_expression
  | '.' identifier '(' expression_opt ')'
  | net_port_header '.' identifier '(' expression_opt ')'
  | variable_port_header '.' identifier '(' expression_opt ')'
  ;

ansi_port_declarations
  : ansi_port_declaration
  | ansi_port_declarations ansi_port_declaration
  ;

ansi_port_declarations_opt
  : ansi_port_declarations
  |
  ;

  /* Module items */
module_common_item
  : module_or_generate_item_declaration
  | interface_or_program_or_module_instantiation
  | concurrent_assertion_item
  | bind_directive
  | continuous_assign
  | net_alias
  | initial_construct
  | final_construct
  | always_construct
  ;

module_item
  : port_declaration ';'
  | non_port_module_item
  ;

module_items
  : module_item
  | module_items module_item
  ;

module_items_opt
  : module_items
  |
  ;

module_or_generate_item
  : attribute_instances_opt parameter_override
  | attribute_instances_opt gate_instantiation
  | attribute_instances_opt udp_instantiation
  | attribute_instances_opt module_common_item
  ;

module_or_generate_item_declaration
  : package_or_generate_item_declaration
  | genvar_declaration
  | clocking_declaration
  | K_default K_clocking identifier ';'
  ;

non_port_module_item
  : generated_module_instantiation
  | module_or_generate_item
  | specify_block
  | attribute_instances_opt specparam_declaration
  | program_declaration
  | module_declaration
  | timeunits_declaration 
  ;

non_port_module_items
  : non_port_module_item
  | non_port_module_items non_port_module_item
  ;

non_port_module_items_opt
  : non_port_module_items
  |
  ;

parameter_override
  : K_defparam list_of_defparam_assignments ';'
  ;

bind_directive
  : K_bind hierarchical_identifier constant_select bind_instantiation ';'
  ;

bind_instantiation
  : interface_or_program_or_module_instantiation 
  ;

  /* Interface items */
interface_or_generate_item
  : attribute_instances_opt module_common_item
  | attribute_instances_opt modport_declaration
  | attribute_instances_opt extern_tf_declaration
  ;

extern_tf_declaration
  : K_extern method_prototype ';'
  | K_extern K_forkjoin task_prototype ';'
  ;

interface_item
  : port_declaration ';'
  | non_port_interface_item
  ;

interface_items
  : interface_item
  | interface_items interface_item
  ;

interface_items_opt
  : interface_items
  |
  ;

non_port_interface_item
  : generated_interface_instantiation
  | attribute_instances_opt specparam_declaration
  | interface_or_generate_item
  | program_declaration
  | interface_declaration
  | timeunits_declaration 
  ;

non_port_interface_items
  : non_port_interface_item
  | non_port_interface_items non_port_interface_item
  ;

non_port_interface_items_opt
  : non_port_interface_items
  |
  ;

  /* Program items */
program_item
  : port_declaration ';'
  | non_port_program_item
  ;

program_items
  : program_item
  | program_items program_item
  ;

program_items_opt
  : program_items
  |
  ;

non_port_program_item
  : attribute_instances_opt continuous_assign
  | attribute_instances_opt module_or_generate_item_declaration
  | attribute_instances_opt specparam_declaration
  | attribute_instances_opt initial_construct
  | attribute_instances_opt concurrent_assertion_item
  | attribute_instances_opt timeunits_declaration 
  ;

non_port_program_items
  : non_port_program_item
  | non_port_program_items non_port_program_item
  ;

non_port_program_items_opt
  : non_port_program_items
  |
  ;

  /* Class items */
class_item
  : attribute_instances_opt class_property
  | attribute_instances_opt class_method
  | attribute_instances_opt class_constraint
  | attribute_instances_opt type_declaration
  | attribute_instances_opt class_declaration
  | attribute_instances_opt timeunits_declaration
  ;

class_items
  : class_item
  | class_items class_item
  ;

class_items_opt
  : class_items
  |
  ;

class_property
  : property_qualifiers_opt data_declaration
  | K_const class_item_qualifiers_opt data_type identifier ';'
  | K_const class_item_qualifiers_opt data_type identifier '=' constant_expression ';'
  ;

class_method
  : method_qualifiers_opt task_declaration
  | method_qualifiers_opt function_declaration
  | K_extern method_qualifiers_opt method_prototype ';'
  | method_qualifier_opt class_constructor_declaration
  | K_extern method_qualifiers_opt class_constructor_prototype
  ;

class_constructor_prototype
  : K_function K_new '(' tf_port_list_opt ')' ';'
  ;

class_constraint
  : constraint_prototype
  | constraint_declaration
  ;

class_item_qualifier
  : K_static
  | K_protected
  | K_local
  ;

class_item_qualifiers
  : class_item_qualifier
  | class_item_qualifiers class_item_qualifier
  ;

class_item_qualifiers_opt
  : class_item_qualifiers
  |
  ;

property_qualifier
  : K_rand
  | K_randc
  | class_item_qualifier
  ;

property_qualifiers
  : property_qualifier
  | property_qualifiers property_qualifier
  ;

property_qualifiers_opt
  : property_qualifiers
  |
  ;

method_qualifier
  : K_virtual
  | class_item_qualifier 
  ;

method_qualifier_opt
  : method_qualifier
  ;

method_qualifiers
  : method_qualifier
  | method_qualifiers method_qualifier
  ;

method_qualifiers_opt
  : method_qualifiers
  |
  ;

method_prototype
  : task_prototype ';'
  | function_prototype ';'
  ;

class_constructor_declaration
  : K_function class_scope_opt K_new tf_port_list_opt ';' block_item_declarations_opt
    function_statement_or_nulls_opt K_endfunction postfix_new_opt
  | K_function class_scope_opt K_new tf_port_list_opt ';' block_item_declarations_opt K_super '.' K_new list_of_arguments_opt ';'
    function_statement_or_nulls_opt K_endfunction postfix_new_opt
  ;

  /* Constraints */
constraint_declaration
  : K_static K_constraint identifier constraint_block
  | K_constraint identifier constraint_block
  ;

constraint_block
  : '{' constraint_block_items_opt '}'
  ;

constraint_block_item
  : K_solve identifier_list K_before identifier_list ';'
  | constraint_expression
  ;

constraint_block_items
  : constraint_block_item
  | constraint_block_items constraint_block_item
  ;

constraint_block_items_opt
  : constraint_block_items
  |
  ;

constraint_expression
  : expression_or_dist ';'
  | expression K_TRIGGER constraint_set
  | K_if '(' expression ')' constraint_set
  | K_if '(' expression ')' constraint_set K_else constraint_set
  | K_foreach '(' identifier loop_variables_opt ')' constraint_set
  ;

constraint_expressions
  : constraint_expression
  | constraint_expressions constraint_expression
  ;

constraint_expressions_opt
  : constraint_expressions
  |
  ;

constraint_set
  : constraint_expression
  | '{' constraint_expressions_opt '}'
  ;

dist_list
  : dist_item dist_items_opt
  ;

dist_item
  : value_range dist_weight_opt
  ;

dist_items
  : ',' dist_item
  | dist_items ',' dist_item
  ;

dist_items_opt
  : dist_items
  |
  ;

dist_weight
  : ':' '=' expression
  | ':' '/' expression
  ;

dist_weight_opt
  : dist_weight
  |
  ;

constraint_prototype
  : K_static K_constraint identifier ';'
  | K_constraint identifier ;
  ;

extern_constraint_declaration
  : K_static K_constraint class_scope identifier constraint_block
  | K_constraint class_scope identifier constraint_block
  ;

identifier_list
  : identifier identifiers_opt
  ;

  /* Package items */

package_item
  : package_or_generate_item_declaration
  | specparam_declaration
  | anonymous_program
  | timeunits_declaration
  ;

package_or_generate_item_declaration
  : net_declaration
  | data_declaration
  | task_declaration
  | function_declaration
  | dpi_import_export
  | extern_constraint_declaration
  | class_declaration
  | class_constructor_declaration
  | parameter_declaration ';'
  | local_parameter_declaration
  | covergroup_declaration
  | overload_declaration
  | concurrent_assertion_item_declaration
  |
  ;

anonymous_program
  : K_program ';' anonymous_program_items_opt K_endprogram
  ;

anonymous_program_item
  : task_declaration
  | function_declaration
  | class_declaration
  | covergroup_declaration
  | class_constructor_declaration
  ; 

anonymous_program_items
  : anonymous_program_item
  | anonymous_program_items anonymous_program_item
  ;

anonymous_program_items_opt
  : anonymous_program_items
  |
  ;

  /* DECLARATIONS */

  /* Declaration Types */

  /* Module parameter declarations */
local_parameter_declaration
  : K_localparam data_type_or_implicit list_of_param_assignments ';'
  ;

parameter_declaration
  : K_parameter data_type_or_implicit list_of_param_assignments
  | K_parameter K_type list_of_type_assignments
  ;

specparam_declaration
  : K_specparam packed_dimension_opt list_of_specparam_assignments ';'
  ;

  /* Port declarations */
inout_declaration
  : K_inout port_type list_of_port_identifiers
  ;

input_declaration
  : K_input port_type list_of_port_identifiers
  | K_input data_type list_of_variable_identifiers
  ;

output_declaration
  : K_output port_type list_of_port_identifiers
  | K_output data_type list_of_variable_port_identifiers
  ;

interface_port_declaration
  : identifier list_of_interface_identifiers
  | identifier '.' identifier list_of_interface_identifiers
  ;

ref_declaration
  : K_ref data_type list_of_port_identifiers
  ;

  /* Type declarations */

data_declaration
  : K_const lifetime_opt variable_declaration
  | variable_declaration
  | lifetime variable_declaration
  | type_declaration
  | package_import_declaration
  | virtual_interface_declaration
  ;

data_declarations
  : data_declaration
  | data_declarations data_declaration
  ;

data_declarations_opt
  : data_declarations
  |
  ;

package_import_declaration
  : K_import package_import_item package_import_items_opt ';'

package_import_item
  : identifier ':' ':' identifier
  | identifier ':' ':' '*'
  ;

package_import_items
  : ',' package_import_item
  | package_import_items ',' package_import_item
  ;

package_import_items_opt
  : package_import_items
  |
  ;

genvar_declaration
  : K_genvar list_of_genvar_identifiers ';'
  ;

net_declaration
  : net_type_or_trireg drive_or_charge_strength_opt vectored_or_scalared_opt signing_opt packed_dimensions_opt delay3_opt list_of_net_decl_assignments ';'
  ;

drive_or_charge_strength_opt
  : drive_strength
  | charge_strength
  |
  ;

vectored_or_scalared_opt
  : K_vectored
  | K_scalared
  |
  ;

type_declaration
  : K_typedef data_type identifier variable_dimension ';'
  | K_typedef identifier '.' identifier identifier ';'
  | K_typedef esuc_opt identifier ';'
  ;

esuc_opt
  : K_enum
  | K_struct
  | K_union
  | K_class
  |
  ;

variable_declaration
  : data_type list_of_variable_decl_assignments ';'
  ;

lifetime
  : K_static
  | K_automatic
  ;

lifetime_opt
  : lifetime
  |
  ;

  /* Declaration data types */

  /* Net and variable types */

casting_type
  : simple_type
  | NUMBER
  | UNUSED_NUMBER
  | signing 
  ;

data_type
  : integer_vector_type signing_opt packed_dimensions_opt
  | integer_atom_type signing_opt
  | non_integer_type
  | struct_union packed_opt '{' struct_union_member struct_union_members_opt '}' packed_dimensions_opt
  | K_enum enum_base_type_opt '{' enum_name_declaration enum_name_declarations_opt '}'
  | STRING
  | UNUSED_STRING
  | K_chandle
  | K_virtual K_interface identifier
  | K_virtual identifier
  | identifier packed_dimensions_opt
  | class_scope identifier packed_dimensions_opt
  | package_scope identifier packed_dimensions_opt
  | class_type
  | K_event
  | ps_identifier
  ;

data_type_list
  : ',' data_type
  | data_type_list ',' data_type
  ;

data_type_list_opt
  : data_type_list
  |
  ;

packed_opt
  : K_packed signing_opt
  |
  ;

data_type_or_implicit
  : data_type
  | signing_opt packed_dimensions_opt
  ;

enum_base_type
  : integer_atom_type signing_opt
  | integer_vector_type signing_opt packed_dimension_opt
  | identifier packed_dimension_opt
  ;

enum_base_type_opt
  : enum_base_type
  |
  ;

enum_name_declaration
  : identifier
  | identifier '[' NUMBER ']'
  | identifier '[' UNUSED_NUMBER ']'
  | identifier '[' NUMBER ':' NUMBER ']'
  | identifier '[' UNUSED_NUMBER ':' UNUSED_NUMBER ']'
  | identifier '=' constant_expression
  | identifier '[' NUMBER ']' '=' constant_expression
  | identifier '[' UNUSED_NUMBER ']' '=' constant_expression
  | identifier '[' NUMBER ':' NUMBER ']' '=' constant_expression
  | identifier '[' UNUSED_NUMBER ':' UNUSED_NUMBER ']' '=' constant_expression
  ;

enum_name_declarations
  : ',' enum_name_declaration
  | enum_name_declarations ',' enum_name_declaration
  ;

enum_name_declarations_opt
  : enum_name_declarations
  |
  ;

class_scope
  : class_type ':' ':'

class_scope_opt
  : class_scope
  |
  ;

class_type
  : ps_identifier parameter_value_assignment_opt class_identifier_and_param_value_assignments_opt
  ;

class_identifier_and_param_value_assignments
  : ':' ':' identifier parameter_value_assignment_opt
  | class_identifier_and_param_value_assignments ':' ':' identifier parameter_value_assignment_opt
  ;

class_identifier_and_param_value_assignments_opt
  : class_identifier_and_param_value_assignments
  |
  ;

integer_type
  : integer_vector_type
  | integer_atom_type
  ;

integer_atom_type
  : K_byte
  | K_shortint
  | K_int
  | K_longint
  | K_integer
  | K_time
  ;

integer_vector_type
  : K_bit
  | K_logic
  | K_reg
  ;

non_integer_type
  : K_shortreal
  | K_real
  | K_realtime
  ;

net_type
  : K_supply0
  | K_supply1
  | K_tri
  | K_triand
  | K_trior
  | K_tri0
  | K_tri1
  | K_wire
  | K_wand
  | K_wor
  ;

port_type
  : net_type_or_trireg_opt signing_opt packed_dimensions_opt
  ;

net_type_or_trireg
  : net_type
  | K_trireg
  ;

net_type_or_trireg_opt
  : net_type_or_trireg
  |
  ;

signing
  : K_signed
  | K_unsigned
  ;

signing_opt
  : signing
  |
  ;

simple_type
  : integer_type
  | non_integer_type
  | ps_identifier
  ;

struct_union_member
  : attribute_instances_opt data_type_or_void list_of_variable_identifiers ';'
  ;

struct_union_members
  : struct_union_member
  | struct_union_members struct_union_member
  ;

struct_union_members_opt
  : struct_union_members
  |
  ;

data_type_or_void
  : data_type
  | K_void
  ;

struct_union
  : K_struct
  | K_union
  | K_union K_tagged
  ;

  /* Strengths */

drive_strength
  : '(' strength0 ',' strength1 ')'
  | '(' strength1 ',' strength0 ')'
  | '(' strength0 ',' K_highz1 ')'
  | '(' strength1 ',' K_highz0 ')'
  | '(' K_highz0 ',' strength1 ')'
  | '(' K_highz1 ',' strength0 ')'
  ;

drive_strength_opt
  : drive_strength
  |
  ;

strength0
  : K_supply0
  | K_strong0
  | K_pull0
  | K_weak0
  ;

strength1
  : K_supply1
  | K_strong1
  | K_pull1
  | K_weak1
  ;

charge_strength
  : '(' K_small ')'
  | '(' K_medium ')'
  | '(' K_large ')'
  ;

  /* Delays */

delay3
  : '#' delay_value
  | '#' '(' mintypmax_expression ')'
  | '#' '(' mintypmax_expression ',' mintypmax_expression ')'
  | '#' '(' mintypmax_expression ',' mintypmax_expression ',' mintypmax_expression ')'
  ;

delay3_opt
  : delay3
  |
  ;

delay2
  : '#' delay_value
  | '#' '(' mintypmax_expression ')'
  | '#' '(' mintypmax_expression ',' mintypmax_expression ')'
  ;

delay2_opt
  : delay2
  |
  ;

delay_value
  : NUMBER
  | UNUSED_NUMBER
  | REALTIME
  | UNUSED_REALTIME
  | ps_identifier
  | time_literal
  ;

  /* Declaration lists */

list_of_defparam_assignments
  : defparam_assignment defparam_assignments_list_opt
  ;

defparam_assignments_list
  : ',' defparam_assignment
  | defparam_assignments_list ',' defparam_assignment
  ;

defparam_assignments_list_opt
  : defparam_assignments_list
  |
  ;

list_of_genvar_identifiers
  : identifier genvar_identifier_list_opt
  ;

genvar_identifier_list
  : ',' identifier
  | genvar_identifier_list ',' identifier
  ;

genvar_identifier_list_opt
  : genvar_identifier_list
  |
  ;

list_of_interface_identifiers
  : identifier unpacked_dimensions_opt interface_identifiers_list_opt
  ;

interface_identifiers_list
  : ',' identifier unpacked_dimensions_opt
  | interface_identifiers_list ',' identifier unpacked_dimensions_opt
  ;

interface_identifiers_list_opt
  : interface_identifiers_list
  |
  ;

list_of_net_decl_assignments
  : net_decl_assignment net_decl_assignment_list_opt
  ;

net_decl_assignment_list
  : ',' net_decl_assignment
  | net_decl_assignment_list ',' net_decl_assignment
  ;

net_decl_assignment_list_opt
  : net_decl_assignment_list
  |
  ;

list_of_param_assignments
  : param_assignment param_assignment_list_opt
  ;

param_assignment_list
  : ',' param_assignment
  | param_assignment_list ',' param_assignment
  ;

param_assignment_list_opt
  : param_assignment_list
  |
  ;

list_of_port_identifiers
  : identifier unpacked_dimensions_opt port_identifiers_list_opt
  ;

port_identifiers_list
  : ',' identifier unpacked_dimensions_opt
  | port_identifiers_list ',' identifier unpacked_dimensions_opt
  ;

port_identifiers_list_opt
  : port_identifiers_list
  |
  ;

list_of_udp_port_identifiers
  : identifier udp_port_identifiers_list_opt
  ;

udp_port_identifiers_list
  : ',' identifier
  | udp_port_identifiers_list ',' identifier
  ;

udp_port_identifiers_list_opt
  : udp_port_identifiers_list
  |
  ;

list_of_specparam_assignments
  : specparam_assignment specparam_assignments_list_opt
  ;

specparam_assignments_list
  : ',' specparam_assignment
  | specparam_assignments_list ',' specparam_assignment
  ;

specparam_assignments_list_opt
  : specparam_assignments_list
  |
  ;

list_of_tf_variable_identifiers
  : identifier variable_dimension tf_variable_identifiers_list_opt
  | identifier variable_dimension '=' expression tf_variable_identifiers_list_opt
  ;

tf_variable_identifiers_list
  : ',' identifier variable_dimension
  | ',' identifier variable_dimension '=' expression
  | tf_variable_identifiers_list ',' identifier variable_dimension
  | tf_variable_identifiers_list ',' identifier variable_dimension '=' expression
  ;

tf_variable_identifiers_list_opt
  : tf_variable_identifiers_list
  |
  ;

list_of_type_assignments
  : type_assignment type_assignments_list_opt

type_assignments_list
  : ',' type_assignment
  | type_assignments_list ',' type_assignment
  ;

type_assignments_list_opt
  : type_assignments_list
  |
  ;

list_of_variable_decl_assignments
  : variable_decl_assignment variable_decl_assignments_list_opt
  ;

variable_decl_assignments_list
  : ',' variable_decl_assignment
  | variable_decl_assignments_list ',' variable_decl_assignment
  ;

variable_decl_assignments_list_opt
  : variable_decl_assignments_list
  |
  ;

list_of_variable_identifiers
  : identifier variable_dimension variable_identifiers_list_opt
  ;

variable_identifiers_list
  : ',' identifier variable_dimension
  | variable_identifiers_list ',' identifier variable_dimension
  ;

variable_identifiers_list_opt
  : variable_identifiers_list
  |
  ;

list_of_variable_port_identifiers
  : identifier variable_dimension variable_port_identifiers_list_opt
  | identifier variable_dimension '=' constant_expression variable_port_identifiers_list_opt
  ;

variable_port_identifiers_list
  : ',' identifier variable_dimension
  | ',' identifier variable_dimension '=' constant_expression
  | variable_port_identifiers_list ',' identifier variable_dimension
  | variable_port_identifiers_list ',' identifier variable_dimension '=' constant_expression
  ;

variable_port_identifiers_list_opt
  : variable_port_identifiers_list
  |
  ;

list_of_virtual_interface_decl
  : identifier virtual_interface_decl_list_opt
  | identifier '=' identifier virtual_interface_decl_list_opt
  ;

virtual_interface_decl_list
  : ',' identifier
  | ',' identifier '=' identifier
  | virtual_interface_decl_list ',' identifier
  | virtual_interface_decl_list ',' identifier '=' identifier
  ;

virtual_interface_decl_list_opt
  : virtual_interface_decl_list
  |
  ;

  /* Declaration assignments */

defparam_assignment
  : hierarchical_identifier '=' constant_mintypmax_expression
  ;

net_decl_assignment
  : identifier unpacked_dimensions_opt
  | identifier unpacked_dimensions_opt '=' expression
  ;

param_assignment
  : identifier unpacked_dimensions_opt '=' constant_param_expression
  ;

specparam_assignment
  : identifier '=' constant_mintypmax_expression
  | pulse_control_specparam
  ;

type_assignment
  : identifier '=' data_type 
  ;

pulse_control_specparam
  : PATHPULSE_IDENTIFIER '=' '(' reject_limit_value ')' ';'
  | UNUSED_PATHPULSE_IDENTIFIER '=' '(' reject_limit_value ')' ';'
  | PATHPULSE_IDENTIFIER '=' '(' reject_limit_value ',' error_limit_value ')' ';'
  | UNUSED_PATHPULSE_IDENTIFIER '=' '(' reject_limit_value ',' error_limit_value ')' ';'
  | PATHPULSE_IDENTIFIER specify_input_terminal_descriptor '$' specify_output_terminal_descriptor '=' '(' reject_limit_value ')' ';'
  | UNUSED_PATHPULSE_IDENTIFIER specify_input_terminal_descriptor '$' specify_output_terminal_descriptor '=' '(' reject_limit_value ')' ';'
  | PATHPULSE_IDENTIFIER specify_input_terminal_descriptor '$' specify_output_terminal_descriptor '=' '(' reject_limit_value ',' error_limit_value ')' ';'
  | UNUSED_PATHPULSE_IDENTIFIER specify_input_terminal_descriptor '$' specify_output_terminal_descriptor '=' '(' reject_limit_value ',' error_limit_value ')' ';'
  ;

error_limit_value
  : limit_value
  ;

reject_limit_value
  : limit_value
  ;

limit_value
  : constant_mintypmax_expression
  ;

variable_decl_assignment
  : identifier variable_dimension
  | identifier variable_dimension '=' expression
  | identifier '[' ']'
  | identifier '[' ']' '=' dynamic_array_new
  | identifier
  | identifier '=' class_new
  | '=' K_new list_of_arguments_opt
  | identifier '=' K_new list_of_arguments_opt
  ;

class_new
  : K_new list_of_arguments_opt
  | K_new expression
  ;

dynamic_array_new
  : K_new '[' expression ']'
  | K_new '[' expression ']' '(' expression ')'
  ;

  /* Declaration ranges */
unpacked_dimension
  : '[' constant_range ']'
  | '[' constant_expression ']'
  ;

unpacked_dimensions
  : unpacked_dimension
  | unpacked_dimensions unpacked_dimension
  ;

unpacked_dimensions_opt
  : unpacked_dimensions
  |
  ;

packed_dimension
  : '[' constant_range ']'
  | unsized_dimension
  ;

packed_dimension_opt
  : packed_dimension
  |
  ;

packed_dimensions
  : packed_dimension
  | packed_dimensions packed_dimension
  ;

packed_dimensions_opt
  : packed_dimensions
  |
  ;

associative_dimension
  : '[' data_type ']'
  | '[' '*' ']'
  ;

variable_dimension
  : sized_or_unsized_dimensions_opt
  | associative_dimension
  | queue_dimension
  ;

queue_dimension
  : '[' '$' ']'
  | '[' '$' ':' constant_expression ']'
  ;

unsized_dimension
  : '[' ']'
  ;

sized_or_unsized_dimension
  : unpacked_dimension
  | unsized_dimension
  ;

sized_or_unsized_dimensions
  : sized_or_unsized_dimension
  | sized_or_unsized_dimensions sized_or_unsized_dimension
  ;

sized_or_unsized_dimensions_opt
  : sized_or_unsized_dimensions
  |
  ;

  /* Function declarations */
function_data_type
  : data_type
  | K_void
  ;

function_data_type_opt
  : function_data_type
  |
  ;

function_data_type_or_implicit
  : function_data_type
  | signing_opt packed_dimensions_opt
  ;

function_declaration
  : K_function lifetime_opt function_body_declaration
  ;

function_body_declaration
  : function_data_type_or_implicit identifier ';' tf_item_declarations_opt function_statement_or_nulls_opt K_endfunction postfix_identifier_opt
  | function_data_type_or_implicit identifier '.' identifier ';' tf_item_declarations_opt function_statement_or_nulls_opt K_endfunction postfix_identifier_opt
  | function_data_type_or_implicit class_scope identifier ';' tf_item_declarations_opt function_statement_or_nulls_opt K_endfunction postfix_identifier_opt
  | function_data_type_or_implicit identifier '(' tf_port_list_opt ')' ';' block_item_declarations_opt function_statement_or_nulls_opt K_endfunction postfix_identifier_opt
  | function_data_type_or_implicit identifier '.' identifier '(' tf_port_list_opt ')' ';' block_item_declarations_opt function_statement_or_nulls_opt K_endfunction postfix_identifier_opt
  | function_data_type_or_implicit class_scope identifier '(' tf_port_list_opt ')' ';' block_item_declarations_opt function_statement_or_nulls_opt K_endfunction postfix_identifier_opt
  ;

function_prototype
  : K_function function_data_type identifier '(' tf_port_list_opt ')'
  ;

dpi_import_export
  : K_import lex_start_import_export K_DPI lex_end_import_export dpi_function_import_property_opt dpi_function_proto ';'
  | K_import lex_start_import_export K_DPI lex_end_import_export dpi_function_import_property_opt c_identifier '=' dpi_function_proto ';'
  | K_import lex_start_import_export K_DPI lex_end_import_export dpi_task_import_property dpi_task_proto ';'
  | K_import lex_start_import_export K_DPI lex_end_import_export dpi_task_import_property c_identifier '=' dpi_task_proto ';'
  | K_export lex_start_import_export K_DPI lex_end_import_export K_function identifier ';'
  | K_export lex_start_import_export K_DPI lex_end_import_export c_identifier '=' K_function identifier ';'
  | K_export lex_start_import_export K_DPI lex_end_import_export K_task identifier ';'
  | K_export lex_start_import_export K_DPI lex_end_import_export c_identifier '=' K_task identifier ';'
  ;

dpi_function_import_property
  : K_context
  | K_pure
  ;

dpi_function_import_property_opt
  : dpi_function_import_property
  |
  ;

dpi_task_import_property
  : K_context
  ;

dpi_function_proto
  : function_prototype
  ;

dpi_task_proto
  : task_prototype
  ;

  /* Task declarations */
task_declaration
  : K_task lifetime_opt task_body_declaration
  ;

task_body_declaration
  : identifier ';' tf_item_declarations_opt statement_or_nulls_opt K_endtask postfix_identifier_opt
  | identifier '.' identifier ';' tf_item_declarations_opt statement_or_nulls_opt K_endtask postfix_identifier_opt
  | class_scope identifier ';' tf_item_declarations_opt statement_or_nulls_opt K_endtask postfix_identifier_opt
  | identifier '(' tf_port_list_opt ')' ';' block_item_declarations_opt statement_or_nulls_opt K_endtask postfix_identifier_opt
  | identifier '.' identifier '(' tf_port_list_opt ')' ';' block_item_declarations_opt statement_or_nulls_opt K_endtask postfix_identifier_opt
  | class_scope identifier '(' tf_port_list_opt ')' ';' block_item_declarations_opt statement_or_nulls_opt K_endtask postfix_identifier_opt
  ;

tf_item_declaration
  : block_item_declaration
  | tf_port_declaration
  ;

tf_item_declarations
  : tf_item_declaration
  | tf_item_declarations tf_item_declaration
  ;

tf_item_declarations_opt
  : tf_item_declarations
  |
  ;

tf_port_list
  : tf_port_item tf_port_items_opt
  ;

tf_port_list_opt
  : tf_port_list
  |
  ;

tf_port_item
  : attribute_instances_opt tf_port_direction_opt data_type_or_implicit identifier variable_dimension
  | attribute_instances_opt tf_port_direction_opt data_type_or_implicit identifier variable_dimension '=' expression
  ;

tf_port_items
  : tf_port_item
  | tf_port_items tf_port_item
  ;

tf_port_items_opt
  : tf_port_items
  |
  ;

tf_port_direction
  : port_direction
  | K_const K_ref
  ;

tf_port_direction_opt
  : tf_port_direction
  |
  ;

tf_port_declaration
  : attribute_instances_opt tf_port_direction data_type_or_implicit list_of_tf_variable_identifiers ';'
  ;

task_prototype
  : K_task identifier '(' tf_port_list_opt ')'
  ;

  /* Block item declarations */
block_item_declaration
  : attribute_instances_opt data_declaration
  | attribute_instances_opt local_parameter_declaration
  | attribute_instances_opt parameter_declaration ';'
  | attribute_instances_opt overload_declaration
  ;

block_item_declarations
  : block_item_declaration
  | block_item_declarations block_item_declaration
  ;

block_item_declarations_opt
  : block_item_declarations
  |
  ;

overload_declaration
  : K_bind overload_operator K_function data_type identifier '(' overload_proto_formals ')' ';'
  ;

overload_operator
  : '+'
  | K_INC
  | '-'
  | K_DEC
  | '*'
  | K_POW
  | '/'
  | '%'
  | K_EQ
  | K_NE
  | '<'
  | K_LE
  | '>'
  | K_GE
  | '='
  ;

overload_proto_formals
  : data_type data_type_list_opt
  ;

  /* Interface declarations */
virtual_interface_declaration
  : K_virtual identifier list_of_virtual_interface_decl ';'
  | K_virtual K_interface identifier list_of_virtual_interface_decl ';'
  ;

modport_declaration
  : K_modport modport_item modport_item_list_opt ';'
  ;

modport_item
  : identifier '(' modport_ports_declaration modport_ports_declaration_list_opt ')'
  ;

modport_item_list
  : ',' modport_item
  | modport_item_list ',' modport_item
  ;

modport_item_list_opt
  : modport_item_list
  |
  ;

modport_ports_declaration
  : attribute_instances_opt modport_simple_ports_declaration
  | attribute_instances_opt modport_hierarchical_ports_declaration
  | attribute_instances_opt modport_tf_ports_declaration
  | attribute_instances_opt modport_clocking_declaration
  ;

modport_ports_declaration_list
  : ',' modport_ports_declaration
  | modport_ports_declaration_list ',' modport_ports_declaration
  ;

modport_ports_declaration_list_opt
  : modport_ports_declaration_list
  |
  ;

modport_clocking_declaration
  : K_clocking identifier
  ;

modport_simple_ports_declaration
  : port_direction modport_simple_port modport_simple_port_list_opt
  ;

modport_simple_port
  : identifier
  | '.' identifier '(' expression_opt ')'
  ;

modport_simple_port_list
  : ',' modport_simple_port
  | modport_simple_port_list ',' modport_simple_port
  ;

modport_simple_port_list_opt
  : modport_simple_port_list
  |
  ;

modport_hierarchical_ports_declaration
  : identifier '.' identifier
  | identifier '[' constant_expression ']' '.' identifier
  ;

modport_tf_ports_declaration
  : import_export modport_tf_port modport_tf_port_list_opt
  ;

modport_tf_port
  : method_prototype
  | identifier
  ;

modport_tf_port_list
  : ',' modport_tf_port
  | modport_tf_port_list ',' modport_tf_port
  ;

modport_tf_port_list_opt
  : modport_tf_port_list
  |
  ;

import_export
  : K_import
  | K_export
  ;

  /* Assertion declarations */

concurrent_assertion_item
  : concurrent_assertion_statement
  | identifier ':' concurrent_assertion_statement
  ;

concurrent_assertion_statement
  : assert_property_statement
  | assume_property_statement
  | cover_property_statement
  ;

assert_property_statement
  : K_assert K_property '(' property_spec ')' action_block
  ;

assume_property_statement
  : K_assume K_property '(' property_spec ')' ';'
  ;

cover_property_statement
  : K_cover K_property '(' property_spec ')' statement_or_null
  ;

expect_property_statement
  : K_expect '(' property_spec ')' action_block
  ;

property_instance
  : ps_identifier actual_arg_lister_opt
  ;

concurrent_assertion_item_declaration
  : property_declaration
  | sequence_declaration
  ;

property_declaration
  : K_property identifier list_of_formals_opt ';' assertion_variable_declarations_opt property_spec ';' K_endproperty postfix_identifier_opt
  ;

property_spec
  : clocking_event_opt property_expr
  | clocking_event_opt K_disable K_iff '(' expression_or_dist ')' property_expr
  ;

property_expr
  : sequence_expr
  | '(' property_expr ')'
  | K_not property_expr
  | property_expr K_or property_expr
  | property_expr K_and property_expr
  | sequence_expr '|' K_TRIGGER property_expr
  | sequence_expr '|' K_EG property_expr
  | K_if '(' expression_or_dist ')' property_expr
  | K_if '(' expression_or_dist ')' property_expr K_else property_expr
  | property_instance
  | clocking_event property_expr
  ;

sequence_declaration
  : K_sequence identifier list_of_formals_opt ';' assertion_variable_declarations_opt sequence_expr ';' K_endsequence postfix_identifier_opt
  ;

sequence_expr
  : cycle_delay_range sequence_expr cycle_delay_range_sequence_exprs_opt
  | sequence_expr cycle_delay_range sequence_expr cycle_delay_range_sequence_exprs_opt
  | expression_or_dist boolean_abbrev_opt
  | '(' expression_or_dist sequence_match_item_list_opt ')' boolean_abbrev_opt
  | sequence_instance sequence_abbrev_opt
  | '(' sequence_expr sequence_match_item_list_opt ')' sequence_abbrev_opt
  | sequence_expr K_and sequence_expr
  | sequence_expr K_intersect sequence_expr
  | sequence_expr K_or sequence_expr
  | K_first_match '(' sequence_expr sequence_match_item_list_opt ')'
  | expression_or_dist K_throughout sequence_expr
  | sequence_expr K_within sequence_expr
  | clocking_event sequence_expr
  ;

cycle_delay_range_sequence_exprs
  : cycle_delay_range sequence_expr
  | cycle_delay_range_sequence_exprs cycle_delay_range sequence_expr
  ;

cycle_delay_range_sequence_exprs_opt
  : cycle_delay_range_sequence_exprs
  |
  ;

cycle_delay_range
  : K_PP NUMBER
  | K_PP UNUSED_NUMBER
  | K_PP identifier
  | K_PP '(' constant_expression ')'
  | K_PP '[' cycle_delay_const_range_expression ']'
  ;

sequence_method_call
  : sequence_instance '.' identifier
  ;

sequence_match_item
  : operator_assignment
  | inc_or_dec_expression
  | subroutine_call
  ;

sequence_match_item_list
  : ',' sequence_match_item
  | sequence_match_item_list ',' sequence_match_item
  ;

sequence_match_item_list_opt
  : sequence_match_item_list
  |
  ;

sequence_instance
  : ps_identifier actual_arg_lister_opt
  ;

formal_list_item
  : identifier
  | identifier '=' actual_arg_expr
  ;

formal_list_item_list
  : ',' formal_list_item
  | formal_list_item_list ',' formal_list_item
  ;

formal_list_item_list_opt
  : formal_list_item_list
  |
  ;

list_of_formals
  : formal_list_item formal_list_item_list_opt
  ;

actual_arg_list
  : actual_arg_expr actual_arg_expr_list_opt
  | '.' identifier '(' actual_arg_expr ')' identifier_actual_arg_expr_list_opt
  ;

actual_arg_expr
  : event_expression
  | '$'
  ;

actual_arg_expr_list
  : ',' actual_arg_expr
  | actual_arg_expr_list ',' actual_arg_expr
  ;

actual_arg_expr_list_opt
  : actual_arg_expr_list
  |
  ;

identifier_actual_arg_expr_list
  : ',' '.' identifier '(' actual_arg_expr ')'
  | identifier_actual_arg_expr_list ',' '.' identifier '(' actual_arg_expr ')'
  ;

identifier_actual_arg_expr_list_opt
  : identifier_actual_arg_expr_list
  |
  ;

boolean_abbrev
  : consecutive_repetition
  | non_consecutive_repetition
  | goto_repetition
  ;

boolean_abbrev_opt
  : boolean_abbrev
  |
  ;

sequence_abbrev
  : consecutive_repetition
  ;

sequence_abbrev_opt
  : sequence_abbrev
  |
  ;

consecutive_repetition
  : '[' '*' const_or_range_expression ']'
  ;

non_consecutive_repetition
  : '[' '=' const_or_range_expression ']'
  ;

goto_repetition
  : '[' K_TRIGGER const_or_range_expression ']'
  ;

const_or_range_expression
  : constant_expression
  | cycle_delay_const_range_expression
  ;

cycle_delay_const_range_expression
  : constant_expression ':' constant_expression
  | constant_expression ':' '$'
  ;

expression_or_dist
  : expression
  | expression K_dist '{' dist_list '}'
  ;

assertion_variable_declaration
  : data_type list_of_variable_identifiers ';'
  ;

assertion_variable_declarations
  : assertion_variable_declaration
  | assertion_variable_declarations assertion_variable_declaration
  ;

assertion_variable_declarations_opt
  : assertion_variable_declarations
  |
  ;

  /* Covergroup declarations */

covergroup_declaration
  : K_covergroup identifier tf_port_list_opt coverage_event_opt ';' coverage_spec_or_options_opt K_endgroup postfix_identifier_opt
  ;

coverage_spec_or_option
  : attribute_instances_opt coverage_spec
  | attribute_instances_opt coverage_option ';'
  ;

coverage_spec_or_options
  : coverage_spec_or_option ';'
  | coverage_spec_or_options coverage_spec_or_option ';'
  ;

coverage_spec_or_options_opt
  : coverage_spec_or_options
  |
  ;

coverage_option
  : K_optionDOT identifier '=' expression
  | K_type_optionDOT identifier '=' expression
  ;

coverage_spec
  : cover_point
  | cover_cross
  ;

coverage_event
  : clocking_event
  | '@' '@' '(' block_event_expression ')'
  ;

coverage_event_opt
  : coverage_event
  |
  ;

block_event_expression
  : block_event_expression K_or block_event_expression
  | K_begin hierarchical_btf_identifier
  | K_end hierarchical_btf_identifier
  ;

hierarchical_btf_identifier
  : hierarchical_identifier
  | hierarchical_identifier class_scope_opt identifier
  ;

cover_point
  : identifier_opt K_coverpoint expression iff_expression_opt bins_or_empty
  ;

iff_expression_opt
  : K_iff '(' expression ')'
  |
  ;

bins_or_empty
  : '{' attribute_instances_opt bins_or_options_list_opt '}'
  |
  ;

bins_or_options
  : coverage_option
  | bins_keyword identifier '=' '{' range_list '}' iff_expression_opt
  | bins_keyword identifier '[' expression_opt ']' '=' '{' range_list '}' iff_expression_opt
  | bins_keyword identifier '=' trans_list iff_expression_opt
  | bins_keyword identifier '[' ']' '=' trans_list iff_expression_opt
  | K_wildcard bins_keyword identifier '=' '{' range_list '}' iff_expression_opt
  | K_wildcard bins_keyword identifier '[' expression_opt ']' '=' '{' range_list '}' iff_expression_opt
  | K_wildcard bins_keyword identifier '=' trans_list iff_expression_opt
  | K_wildcard bins_keyword identifier '[' ']' '=' trans_list iff_expression_opt
  | bins_keyword identifier '=' K_default iff_expression_opt
  | bins_keyword identifier '[' expression_opt ']' '=' K_default iff_expression_opt
  | bins_keyword identifier '=' K_default K_sequence iff_expression_opt
  ;

bins_or_options_list
  : bins_or_options ';'
  | bins_or_options_list bins_or_options ';'
  ;

bins_or_options_list_opt
  : bins_or_options_list
  |
  ;

bins_keyword
  : K_bins
  | K_illegal_bins
  | K_ignore_bins
  ;

range_list
  : value_range value_range_list_opt

value_range_list
  : ',' value_range
  | value_range_list ',' value_range
  ;

value_range_list_opt
  : value_range_list
  |
  ;

trans_list
  : '(' trans_set ')' trans_set_list_opt
  ;

trans_set_list
  : ',' '(' trans_set ')'
  | trans_set_list ',' '(' trans_set ')'
  ;

trans_set_list_opt
  : trans_set_list
  |
  ;

trans_set
  : trans_range_list K_EG trans_range_list trans_range_lists_opt
  ;

trans_range_lists
  : K_EG trans_range_list
  | trans_range_lists K_EG trans_range_list
  ;

trans_range_lists_opt
  : trans_range_lists
  |
  ;

trans_range_list
  : trans_item
  | trans_item '[' '*' repeat_range ']'
  | trans_item '[' K_TRIGGER repeat_range ']'
  | trans_item '[' '=' repeat_range ']'
  ;

trans_item
  : range_list
  ;

repeat_range
  : expression
  | expression ':' expression
  ;

cover_cross
  : identifier_opt K_cross list_of_coverpoints iff_expression_opt select_bins_or_empty
  ;

list_of_coverpoints
  : cross_item ',' cross_item cross_item_list_opt
  ;

cross_item
  : identifier
  ;

cross_item_list
  : ',' cross_item
  | cross_item_list ',' cross_item
  ;

cross_item_list_opt
  : cross_item_list
  |
  ;

select_bins_or_empty
  : '{' bins_selection_or_option_list_opt '}'
  |
  ;

bins_selection_or_option
  : attribute_instances_opt coverage_option
  | attribute_instances_opt bins_selection
  ;

bins_selection_or_option_list
  : bins_selection_or_option ';'
  | bins_selection_or_option_list bins_selection_or_option ';'
  ;

bins_selection_or_option_list_opt
  : bins_selection_or_option_list
  |
  ;

bins_selection
  : bins_keyword identifier '=' select_expression iff_expression_opt
  ;

select_expression
  : select_condition
  | '!' select_condition
  | select_expression K_LAND select_expression
  | select_expression K_LOR select_expression
  | '(' select_expression ')' 
  ;

select_condition
  : K_binsof '(' bins_expression ')'
  | K_binsof '(' bins_expression ')' K_intersect open_range_lists_opt
  ;

bins_expression
  : identifier
  | identifier '.' identifier
  ;

open_range_list
  : open_value_range open_value_range_list_opt
  ;

open_range_lists
  : ',' open_range_list
  | open_range_lists ',' open_range_list
  ;

open_range_lists_opt
  : open_range_lists
  |
  ;

open_value_range
  : value_range
  ;

open_value_range_list
  : ',' open_value_range
  | open_value_range_list ',' open_value_range
  ;

open_value_range_list_opt
  : open_value_range_list
  |
  ;

  /* Primitive instances */

  /* Primitive instantiation and instances */

gate_instantiation
  : cmos_switchtype delay3_opt cmos_switch_instance cmos_switch_instance_list_opt ';'
  | enable_gatetype drive_strength_opt delay3_opt enable_gate_instance enable_gate_instance_list_opt ';'
  | mos_switchtype delay3_opt mos_switch_instance mos_switch_instance_list_opt ';'
  | n_input_gatetype drive_strength_opt delay2_opt n_input_gate_instance n_input_gate_instance_list_opt ';'
  | n_output_gatetype drive_strength_opt delay2_opt n_output_gate_instance n_output_gate_instance_list_opt ';'
  | pass_en_switchtype delay2_opt pass_enable_switch_instance pass_enable_switch_instance_list_opt ';'
  | pass_switchtype pass_switch_instance pass_switch_instance_list_opt ';'
  | K_pulldown pulldown_strength_opt pull_gate_instance pull_gate_instance_list_opt ';'
  | K_pullup pullup_strength_opt pull_gate_instance pull_gate_instance_list_opt ';'
  ;

cmos_switch_instance
  : name_of_instance_opt '(' output_terminal ',' input_terminal ',' ncontrol_terminal ',' pcontrol_terminal ')'
  ;

cmos_switch_instance_list
  : ',' cmos_switch_instance
  | cmos_switch_instance_list ',' cmos_switch_instance
  ;

cmos_switch_instance_list_opt
  : cmos_switch_instance_list
  |
  ;

enable_gate_instance
  : name_of_instance_opt '(' output_terminal ',' input_terminal ',' enable_terminal ')'
  ;

enable_gate_instance_list
  : ',' enable_gate_instance
  | enable_gate_instance_list ',' enable_gate_instance
  ;

enable_gate_instance_list_opt
  : enable_gate_instance_list
  |
  ;

mos_switch_instance
  : name_of_instance_opt '(' output_terminal ',' input_terminal ',' enable_terminal ')'
  ;

mos_switch_instance_list
  : ',' mos_switch_instance
  | mos_switch_instance_list ',' mos_switch_instance
  ;

mos_switch_instance_list_opt
  : mos_switch_instance_list
  |
  ;

n_input_gate_instance
  : name_of_instance_opt '(' output_terminal ',' input_terminal input_terminal_list_opt ')'
  ;

n_input_gate_instance_list
  : ',' n_input_gate_instance
  | n_input_gate_instance_list ',' n_input_gate_instance
  ;

n_input_gate_instance_list_opt
  : n_input_gate_instance_list
  |
  ;

n_output_gate_instance
  : name_of_instance_opt '(' output_terminal output_terminal_list_opt ',' input_terminal ')'
  ;

n_output_gate_instance_list
  : ',' n_output_gate_instance
  | n_output_gate_instance_list ',' n_output_gate_instance
  ;

n_output_gate_instance_list_opt
  : n_output_gate_instance_list
  |
  ;

pass_switch_instance
  : name_of_instance_opt '(' inout_terminal ',' inout_terminal ')'
  ;

pass_switch_instance_list
  : ',' pass_switch_instance
  | pass_switch_instance_list ',' pass_switch_instance
  ;

pass_switch_instance_list_opt
  : pass_switch_instance_list
  |
  ;

pass_enable_switch_instance
  : name_of_instance_opt '(' inout_terminal ',' inout_terminal ',' enable_terminal ')'
  ;

pass_enable_switch_instance_list
  : ',' pass_enable_switch_instance
  | pass_enable_switch_instance_list ',' pass_enable_switch_instance
  ;

pass_enable_switch_instance_list_opt
  : pass_enable_switch_instance_list
  |
  ;

pull_gate_instance
  : name_of_instance_opt '(' output_terminal ')'
  ;

pull_gate_instance_list
  : ',' pull_gate_instance
  | pull_gate_instance_list ',' pull_gate_instance
  ;

pull_gate_instance_list_opt
  : pull_gate_instance_list
  |
  ;

  /* Primitive strengths */

pulldown_strength
  : '(' strength0 ',' strength1 ')'
  | '(' strength1 ',' strength0 ')'
  | '(' strength0 ')'
  ;

pulldown_strength_opt
  : pulldown_strength
  |
  ;

pullup_strength
  : '(' strength0 ',' strength1 ')'
  | '(' strength1 ',' strength0 ')'
  | '(' strength1 ')'
  ;

pullup_strength_opt
  : pullup_strength
  |
  ;

  /* Primitive terminals */

enable_terminal
  : expression
  ;

inout_terminal
  : net_lvalue
  ;

input_terminal
  : expression
  ;

input_terminal_list
  : ',' input_terminal
  | input_terminal_list ',' input_terminal
  ;

input_terminal_list_opt
  : input_terminal_list
  |
  ;

ncontrol_terminal
  : expression
  ;

output_terminal
  : net_lvalue
  ;

output_terminal_list
  : ',' output_terminal
  | output_terminal_list ',' output_terminal
  ;

output_terminal_list_opt
  : output_terminal_list
  |
  ;

pcontrol_terminal
  : expression
  ;

  /* Primitive gate and switch types */

cmos_switchtype
  : K_cmos
  | K_rcmos
  ;

enable_gatetype
  : K_bufif0
  | K_bufif1
  | K_notif0
  | K_notif1
  ;

mos_switchtype
  : K_nmos
  | K_pmos
  | K_rnmos
  | K_rpmos
  ;

n_input_gatetype
  : K_and
  | K_nand
  | K_or
  | K_nor
  | K_xor
  | K_xnor
  ;

n_output_gatetype
  : K_buf
  | K_not
  ;

pass_en_switchtype
  : K_tranif0
  | K_tranif1
  | K_rtranif1
  | K_rtranif0
  ;

pass_switchtype
  : K_tran
  | K_rtran
  ;

  /* Module, interface and generated instantiation */

  /* Instantiation */

  /* Module instantiation */

parameter_value_assignment
  : '#' '(' list_of_parameter_assignments ')'
  ;

parameter_value_assignment_opt
  : parameter_value_assignment
  |
  ;

list_of_parameter_assignments
  : ordered_parameter_assignment ordered_parameter_assignment_list_opt
  | named_parameter_assignment named_parameter_assignment_list_opt
  ;

ordered_parameter_assignment
  : param_expression
  ;

ordered_parameter_assignment_list
  : ',' ordered_parameter_assignment
  | ordered_parameter_assignment_list ',' ordered_parameter_assignment
  ;

ordered_parameter_assignment_list_opt
  : ordered_parameter_assignment_list
  |
  ;

named_parameter_assignment
  : '.' identifier '(' param_expression_opt ')'
  ;

named_parameter_assignment_list
  : ',' named_parameter_assignment
  | named_parameter_assignment_list ',' named_parameter_assignment
  ;

named_parameter_assignment_list_opt
  : named_parameter_assignment_list
  |
  ;

hierarchical_instance
  : name_of_instance '(' list_of_port_connections_opt ')'
  ;

hierarchical_instance_list
  : ',' hierarchical_instance
  | hierarchical_instance_list ',' hierarchical_instance
  ;

hierarchical_instance_list_opt
  : hierarchical_instance_list
  |
  ;

name_of_instance
  : identifier unpacked_dimensions_opt
  ;

name_of_instance_opt
  : name_of_instance
  |
  ;

list_of_port_connections
  : ordered_port_connection ordered_port_connection_list_opt
  | named_port_connection named_port_connection_list_opt
  ;

list_of_port_connections_opt
  : list_of_port_connections
  |
  ;

ordered_port_connection
  : attribute_instances_opt expression_opt
  ;

ordered_port_connection_list
  : ',' ordered_port_connection
  | ordered_port_connection_list ',' ordered_port_connection
  ;

ordered_port_connection_list_opt
  : ordered_port_connection_list
  |
  ;

named_port_connection
  : attribute_instances_opt '.' identifier
  : attribute_instances_opt '.' identifier '(' expression_opt ')'
  | attribute_instances_opt K_PS
  ;

named_port_connection_list
  : ',' named_port_connection
  | named_port_connection_list ',' named_port_connection
  ;

named_port_connection_list_opt
  : named_port_connection_list
  |
  ;

  /* Interface/Program/Module instantiation */

interface_or_program_or_module_instantiation
  : identifier parameter_value_assignment_opt hierarchical_instance hierarchical_instance_list_opt ';'
  ;

  /* Generated instantiation */

  /* Generated module instantiation */

generated_module_instantiation
  : K_generate generate_module_items_opt K_endgenerate
  ;

generate_module_item
  : generate_module_conditional_statement
  | generate_module_case_statement
  | generate_module_loop_statement
  | generate_module_block
  | identifier ':' generate_module_block
  | module_or_generate_item
  ;

generate_module_items
  : generate_module_item
  | generate_module_items generate_module_item
  ;

generate_module_items_opt
  : generate_module_items
  |
  ;

generate_module_conditional_statement
  : K_if '(' constant_expression ')' generate_module_item
  | K_if '(' constant_expression ')' generate_module_item K_else generate_module_item
  ;

generate_module_case_statement
  : K_case '(' constant_expression ')' genvar_module_case_item genvar_module_case_items_opt K_endcase
  ;

genvar_module_case_item
  : constant_expression constant_expression_list_opt ':' generate_module_item
  | K_default generate_module_item
  | K_default ':' generate_module_item
  ;

genvar_module_case_items
  : genvar_module_case_item
  | genvar_module_case_items genvar_module_case_item
  ;

genvar_module_case_items_opt
  : genvar_module_case_items
  |
  ;

generate_module_loop_statement
  : K_for '(' genvar_decl_assignment ';' constant_expression ';' genvar_assignment ')' generate_module_named_block
  ;

genvar_assignment
  : identifier assignment_operator constant_expression
  | inc_or_dec_operator identifier
  | identifier inc_or_dec_operator
  ;

genvar_decl_assignment
  : identifier '=' constant_expression
  | K_genvar identifier '=' constant_expression
  ;

generate_module_named_block
  : K_begin ':' identifier generate_module_items_opt K_end postfix_identifier_opt
  | identifier ':' generate_module_block
  ;

generate_module_block
  : K_begin generate_module_items_opt K_end postfix_identifier_opt
  | K_begin ':' identifier generate_module_items_opt K_end postfix_identifier_opt
  ;

  /* Generated interface instantiation */

generated_interface_instantiation
  : K_generate generate_interface_items_opt K_endgenerate 
  ;

generate_interface_item
  : generate_interface_conditional_statement
  | generate_interface_case_statement
  | generate_interface_loop_statement
  | generate_interface_block
  | identifier ':' generate_interface_block
  | interface_or_generate_item
  ;

generate_interface_items
  : generate_interface_item
  | generate_interface_items generate_interface_item
  ;

generate_interface_items_opt
  : generate_interface_items
  |
  ;

generate_interface_conditional_statement
  : K_if '(' constant_expression ')' generate_interface_item 
  | K_if '(' constant_expression ')' generate_interface_item K_else generate_interface_item
  ;

generate_interface_case_statement
  : K_case '(' constant_expression ')' genvar_interface_case_item genvar_interface_case_items_opt K_endcase
  ;

genvar_interface_case_item
  : constant_expression constant_expression_list_opt ':' generate_interface_item
  | K_default generate_interface_item
  | K_default ':' generate_interface_item
  ;

genvar_interface_case_items
  : genvar_interface_case_item
  | genvar_interface_case_items genvar_interface_case_item
  ;

genvar_interface_case_items_opt
  : genvar_interface_case_items
  |
  ;

generate_interface_loop_statement
  : K_for '(' genvar_decl_assignment ';' constant_expression ';' genvar_assignment ')' generate_interface_named_block
  ;

generate_interface_named_block
  : K_begin ':' identifier generate_interface_items_opt K_end postfix_identifier_opt
  | identifier ':' generate_interface_block
  ;

generate_interface_block 
  : K_begin generate_interface_items_opt K_end postfix_identifier_opt
  | K_begin ':' identifier generate_interface_items_opt K_end postfix_identifier_opt
  ;

  /* UDP declaration and instantiation */
 
  /* UDP declaration */

udp_nonansi_declaration
  : attribute_instances_opt K_primitive identifier '(' udp_port_list ')' ';'
  ;

udp_ansi_declaration
  : attribute_instances_opt K_primitive identifier '(' udp_declaration_port_list ')' ';' 
  ;

udp_declaration
  : udp_nonansi_declaration udp_port_declaration udp_port_declarations_opt udp_body K_endprimitive postfix_identifier_opt
  | udp_ansi_declaration udp_body K_endprimitive postfix_identifier_opt
  | K_extern udp_nonansi_declaration
  | K_extern udp_ansi_declaration
  | attribute_instances_opt K_primitive identifier '(' K_PS ')' ';' udp_port_declarations_opt udp_body K_endprimitive postfix_identifier_opt
  ;

  /* UDP ports */

udp_port_list
  : identifier ',' identifier identifier_opt_list
  ;

udp_declaration_port_list
  : udp_output_declaration ',' udp_input_declaration udp_input_declaration_list_opt
  ;

udp_port_declaration
  : udp_output_declaration ';'
  | udp_input_declaration ';'
  | udp_reg_declaration ';'
  ;

udp_port_declarations
  : udp_port_declaration
  | udp_port_declarations udp_port_declaration
  ;

udp_port_declarations_opt
  : udp_port_declarations
  |
  ;

udp_output_declaration
  : attribute_instances_opt K_output identifier
  | attribute_instances_opt K_output K_reg identifier
  | attribute_instances_opt K_output K_reg identifier '=' constant_expression
  ;

udp_input_declaration
  : attribute_instances_opt K_input list_of_udp_port_identifiers
  ;

udp_input_declaration_list
  : ',' udp_input_declaration
  | udp_input_declaration_list ',' udp_input_declaration
  ;

udp_input_declaration_list_opt
  : udp_input_declaration_list
  |
  ;

udp_reg_declaration
  : attribute_instances_opt K_reg identifier
  ;

  /* UDP body */

udp_body
  : combinational_body
  | sequential_body
  ;

combinational_body
  : K_table { lex_start_udp_table(); } combinational_entry combinational_entries_opt { lex_end_udp_table(); } K_endtable

combinational_entry
  : level_input_list ':' output_symbol ';'
  ;

combinational_entries
  : combinational_entry
  | combinational_entries combinational_entry
  ;

combinational_entries_opt
  : combinational_entries
  |
  ;

sequential_body
  : K_table { lex_start_udp_table(); } sequential_entry sequential_entries_opt { lex_end_udp_table(); } K_endtable
  | udp_initial_statement K_table { lex_start_udp_table(); } sequential_entry sequential_entries_opt { lex_end_udp_table(); } K_endtable
  ;

udp_initial_statement
  : K_initial identifier '=' init_val ';'
  ;

init_val
  : NUMBER
  | UNUSED_NUMBER
  ;

sequential_entry
  : seq_input_list ':' current_state ':' next_state ';'
  ;

sequential_entries
  : sequential_entry
  | sequential_entries sequential_entry
  ;

sequential_entries_opt
  : sequential_entries
  |
  ;

seq_input_list
  : level_input_list 
  | edge_input_list
  ;

level_input_list
  : level_symbol level_symbols_opt
  ;

edge_input_list
  : level_symbols_opt edge_indicator level_symbols_opt
  ;

edge_indicator
  : '(' level_symbol level_symbol ')' 
  | edge_symbol
  ;

current_state
  : level_symbol
  ;

next_state
  : output_symbol 
  | '-'
  ;

output_symbol
  : '0'
  | '1'
  | 'x'
  ;

level_symbol
  : '0'
  | '1'
  | 'x'
  | '?'
  | 'b'
  ;

level_symbols
  : level_symbol
  | level_symbols level_symbol
  ;

level_symbols_opt
  : level_symbols
  |
  ;

edge_symbol
  : 'r'
  | 'f'
  | 'p'
  | 'n'
  | '*'
  ;

  /* UDP instantiation */

udp_instantiation
  : identifier drive_strength_opt delay2_opt udp_instance udp_instance_list_opt ';'
  ;

udp_instance
  : name_of_instance_opt '(' output_terminal ',' input_terminal input_terminal_list_opt ')'
  ;

udp_instance_list
  : ',' udp_instance
  | udp_instance_list ',' udp_instance
  ;

udp_instance_list_opt
  : udp_instance_list
  |
  ;

  /* Behavioral statements */

  /* Continuous assignment and net alias statements */

continuous_assign
  : K_assign drive_strength_opt delay3_opt list_of_net_assignments ';'
  | K_assign delay_control_opt list_of_variable_assignments ';'
  ;

list_of_net_assignments
  : net_assignment net_assignment_list_opt
  ;

net_assignment_list
  : ',' net_assignment
  | net_assignment_list ',' net_assignment
  ;

net_assignment_list_opt
  : net_assignment_list
  |
  ;

list_of_variable_assignments
  : variable_assignment variable_assignment_list_opt

net_alias
  : K_alias net_lvalue '=' net_lvalue net_lvalue_assign_list_opt ';'
  ;

net_assignment
  : net_lvalue '=' expression
  ;

  /* Procedural blocks and assignments */

initial_construct
  : K_initial statement_or_null
  ;

final_construct
  : K_final function_statement
  ;

always_construct
  : always_keyword statement
  ;

always_keyword
  : K_always
  | K_always_comb
  | K_always_latch
  | K_always_ff  
  ;

blocking_assignment
  : variable_lvalue '=' delay_or_event_control expression
  | hierarchical_identifier '=' dynamic_array_new
  | hierarchical_identifier select '=' class_new
  | implicit_class_handle_dot hierarchical_identifier select '=' class_new
  | class_scope hierarchical_identifier select '=' class_new
  | package_scope hierarchical_identifier select '=' class_new
  | operator_assignment
  ;

operator_assignment
  : variable_lvalue assignment_operator expression
  ;

assignment_operator
  : '='
  | K_ADD_A
  | K_SUB_A
  | K_MLT_A
  | K_DIV_A
  | K_MOD_A
  | K_AND_A
  | K_OR_A
  | K_XOR_A
  | K_LS_A
  | K_RS_A
  | K_ALS_A
  | K_ARS_A
  ;

nonblocking_assignment
  : variable_lvalue K_LE delay_or_event_control_opt expression
  ;

procedural_continuous_assignment
  : K_assign variable_assignment
  | K_deassign variable_lvalue
  | K_force variable_assignment
  | K_force net_assignment
  | K_release variable_lvalue
  | K_release net_lvalue
  ;

variable_assignment
  : variable_lvalue '=' expression
  ;

variable_assignment_list
  : ',' variable_assignment
  | variable_assignment_list ',' variable_assignment
  ;

variable_assignment_list_opt
  : variable_assignment_list
  |
  ;

  /* Parallel and sequential blocks */

action_block
  : statement_or_null
  | K_else statement_or_null
  | statement K_else statement_or_null
  ;

seq_block
  : K_begin block_item_declarations_opt statement_or_nulls_opt K_end postfix_identifier_opt
  | K_begin ':' identifier block_item_declarations_opt statement_or_nulls_opt K_end postfix_identifier_opt

par_block
  : K_fork block_item_declarations_opt statement_or_nulls_opt join_keyword postfix_identifier_opt
  | K_fork ':' identifier block_item_declarations_opt statement_or_nulls_opt join_keyword postfix_identifier_opt
  ;

join_keyword
  : K_join
  | K_join_any
  | K_join_none
  ;

  /* Statements */

statement_or_null
  : statement
  | attribute_instances_opt ';'
  ;

statement_or_nulls
  : statement_or_null
  | statement_or_nulls statement_or_null
  ;

statement_or_nulls_opt
  : statement_or_nulls
  |
  ;

statement
  : attribute_instances_opt statement_item 
  | identifier ':' attribute_instances_opt statement_item 
  ;

statement_item
  : blocking_assignment ';'
  | nonblocking_assignment ';'
  | procedural_continuous_assignment ';'
  | case_statement
  | conditional_statement
  | inc_or_dec_expression ';'
  | subroutine_call_statement
  | disable_statement
  | event_trigger
  | loop_statement
  | jump_statement
  | par_block
  | procedural_timing_control_statement
  | seq_block
  | wait_statement
  | procedural_assertion_statement
  | clocking_drive ';'
  | randsequence_statement
  | randcase_statement
  | expect_property_statement
  ;

function_statement
  : statement
  ;

function_statement_or_null
  : function_statement
  | attribute_instances_opt ';'
  ;

function_statement_or_nulls
  : function_statement_or_null
  | function_statement_or_nulls function_statement_or_null
  ;

function_statement_or_nulls_opt
  : function_statement_or_nulls
  |
  ;

variable_identifier_list
  : identifier identifiers_opt
  ;

  /* Timing control statements */

procedural_timing_control_statement
  : procedural_timing_control statement_or_null
  ;

delay_or_event_control
  : delay_control
  | event_control
  | K_repeat '(' expression ')' event_control
  ;

delay_or_event_control_opt
  : delay_or_event_control
  |
  ;

delay_control
  : '#' delay_value
  | '#' '(' mintypmax_expression ')' 
  ;

delay_control_opt
  : delay_control
  |
  ;

event_control
  : '@' hierarchical_identifier
  | '@' '(' event_expression ')'
  | '@' '*'
  | '@' sequence_instance
  ;

event_expression
  : edge_identifier_opt expression
  | edge_identifier_opt expression K_iff expression
  | sequence_instance
  | sequence_instance K_iff expression
  | event_expression K_or event_expression
  | event_expression ',' event_expression
  ;

procedural_timing_control
  : delay_control
  | event_control
  | cycle_delay
  ;

jump_statement
  : K_return expression_opt ';'
  | K_break ';'
  | K_continue ';'
  ;

wait_statement
  : K_wait '(' expression ')' statement_or_null
  | K_wait K_fork ';'
  | K_wait_order '(' hierarchical_identifier hierarchical_identifier_list_opt ')' action_block
  ;

event_trigger
  : K_TRIGGER hierarchical_identifier ';'
  | K_TRIGGER '>' delay_or_event_control_opt hierarchical_identifier ';'

disable_statement
  : K_disable hierarchical_identifier ';'
  | K_disable K_fork ';'

  /* Conditional statements */

conditional_statement
  : K_if '(' cond_predicate ')' statement_or_null
  | K_if '(' cond_predicate ')' statement_or_null K_else statement_or_null
  | unique_priority_if_statement
  ;

unique_priority_if_statement
  : unique_priority_opt K_if '(' cond_predicate ')' statement_or_null else_if_list_opt
  | unique_priority_opt K_if '(' cond_predicate ')' statement_or_null else_if_list_opt K_else statement_or_null
  ;

else_if_list
  : K_else K_if '(' cond_predicate ')' statement_or_null
  | else_if_list K_else K_if '(' cond_predicate ')' statement_or_null
  ;

else_if_list_opt
  : else_if_list
  |
  ;

unique_priority
  : K_unique
  | K_priority
  ;

unique_priority_opt
  : unique_priority
  |
  ;

cond_predicate
  : expression_or_cond_pattern expression_or_cond_pattern_tand_list_opt
  ;

expression_or_cond_pattern
  : expression
  | cond_pattern
  ;

expression_or_cond_pattern_tand_list
  : K_TAND expression_or_cond_pattern
  | expression_or_cond_pattern_tand_list K_TAND expression_or_cond_pattern
  ;

expression_or_cond_pattern_tand_list_opt
  : expression_or_cond_pattern_tand_list
  |
  ;

cond_pattern
  : expression K_matches pattern
  ;

  /* Case statements */

case_statement
  : unique_priority_opt case_keyword '(' expression ')' case_item case_items_opt K_endcase
  | unique_priority_opt case_keyword '(' expression ')' K_matches case_pattern_item case_pattern_items_opt K_endcase
  ;

case_keyword
  : K_case
  | K_casez
  | K_casex
  ;

case_item
  : expression expression_list_opt ':' statement_or_null
  | K_default statement_or_null
  | K_default ':' statement_or_null
  ;

case_items
  : case_item
  | case_items case_item
  ;

case_items_opt
  :  case_items
  |
  ;

case_pattern_item 
  : pattern ':' statement_or_null
  | pattern K_TAND expression ':' statement_or_null
  | K_default statement_or_null
  | K_default ':' statement_or_null
  ;

case_pattern_items
  : case_pattern_item
  | case_pattern_items case_pattern_item
  ;

case_pattern_items_opt
  : case_pattern_items
  |
  ;

randcase_statement
  : K_randcase randcase_item randcase_items_opt K_endcase
  ;

randcase_item
  : expression ':' statement_or_null
  ;

randcase_items
  : randcase_item
  | randcase_items randcase_item
  ;

randcase_items_opt
  : randcase_items
  |
  ;

  /* Patterns */

pattern
  : '.' identifier
  | K_PS
  | constant_expression
  | K_tagged identifier pattern_opt
  | '{' pattern pattern_list_opt '}'
  | '{' identifier ':' pattern member_identifier_pattern_list_opt '}'
  ;

pattern_opt
  : pattern
  |
  ;

pattern_list
  : ',' pattern
  | pattern_list ',' pattern
  ;

pattern_list_opt
  : pattern_list
  |
  ;

member_identifier_pattern_list
  : ',' identifier ':' pattern
  | member_identifier_pattern_list ',' identifier ':' pattern
  ;

member_identifier_pattern_list_opt
  : member_identifier_pattern_list
  |
  ;

  /* Looping statements */

loop_statement
  : K_forever statement_or_null
  | K_repeat '(' expression ')' statement_or_null
  | K_while '(' expression ')' statement_or_null
  | K_for '(' for_initialization ';' expression ';' for_step ')' statement_or_null
  | K_do statement_or_null K_while '(' expression ')' ';'
  | K_foreach '(' identifier loop_variables_opt ')' statement
  ;

for_initialization
  : list_of_variable_assignments
  | for_variable_declaration for_variable_declaration_list_opt
  ;

for_variable_declaration
  : data_type identifier '=' expression variable_expression_assign_list_opt
  ;

for_variable_declaration_list
  : ',' for_variable_declaration
  | for_variable_declaration_list ',' for_variable_declaration
  ;

for_variable_declaration_list_opt
  : for_variable_declaration_list
  |
  ;

variable_expression_assign_list
  : ',' identifier '=' expression
  | variable_expression_assign_list ',' identifier '=' expression
  ;

variable_expression_assign_list_opt
  : variable_expression_assign_list
  |
  ;

for_step
  : for_step_assignment for_step_assignment_list_opt

for_step_assignment
  : operator_assignment
  | inc_or_dec_expression
  ;

for_step_assignment_list
  : ',' for_step_assignment
  | for_step_assignment_list ',' for_step_assignment
  ;

for_step_assignment_list_opt
  : for_step_assignment_list
  |
  ;

loop_variables
  : identifier_opt identifier_opt_list_opt
  ;

loop_variables_opt
  : loop_variables
  |
  ;

  /* Subroutine call statements */

subroutine_call_statement
  : subroutine_call ';'
  | K_void '\'' '(' function_subroutine_call ')' ';'
  ;

  /* Assertion statements */

procedural_assertion_statement
  : concurrent_assertion_statement
  | immediate_assert_statement
  ;

immediate_assert_statement
  : K_assert '(' expression ')' action_block
  ;

  /* Clocking block */

clocking_declaration
  : K_clocking identifier_opt clocking_event ';'
  | K_default K_clocking identifier_opt clocking_event ';' clocking_items_opt K_endclocking postfix_identifier_opt
  ;

clocking_event
  : '@' identifier
  | '@' '(' event_expression ')'
  ;

clocking_event_opt
  : clocking_event
  |
  ;

clocking_item
  : K_default default_skew ';'
  | clocking_direction list_of_clocking_decl_assign ';'
  | attribute_instances_opt concurrent_assertion_item_declaration
  ;

clocking_items
  : clocking_item
  | clocking_items clocking_item
  ;

clocking_items_opt
  : clocking_items
  |
  ;

default_skew
  : K_input clocking_skew
  | K_output clocking_skew
  | K_input clocking_skew K_output clocking_skew
  ;

clocking_direction
  : K_input clocking_skew_opt
  | K_output clocking_skew_opt
  | K_input clocking_skew_opt K_output clocking_skew_opt
  | K_inout
  ;

list_of_clocking_decl_assign
  : clocking_decl_assign clocking_decl_assign_list_opt
  ;

clocking_decl_assign
  : identifier
  | identifier '=' hierarchical_identifier
  ;

clocking_decl_assign_list
  : ',' clocking_decl_assign
  | clocking_decl_assign_list ',' clocking_decl_assign
  ;

clocking_decl_assign_list_opt
  : clocking_decl_assign_list
  |
  ;

clocking_skew
  : edge_identifier delay_control_opt
  | delay_control 
  ;

clocking_skew_opt
  : clocking_skew
  |
  ;

clocking_drive
  : clockvar_expression K_LE cycle_delay_opt expression
  | cycle_delay clockvar_expression K_LE expression
  ;

cycle_delay
  : K_PP NUMBER
  | K_PP UNUSED_NUMBER
  | K_PP identifier
  | K_PP '(' expression ')'
  ;

cycle_delay_opt
  : cycle_delay
  |
  ;

clockvar
  : hierarchical_identifier
  ;

clockvar_expression
  : clockvar select
  ;

  /* Randsequence */

randsequence_statement
  : K_randsequence '(' identifier_opt ')' production productions_opt K_endsequence
  ;

production
  : function_data_type_opt identifier tf_port_lister_opt ':' rs_rule rs_rule_list_opt ';'
  ;

productions
  : production
  | productions production
  ;

productions_opt
  : productions
  |
  ;

rs_rule
  : rs_production_list
  | rs_production_list ':' '=' expression rs_code_block_opt
  ;

rs_rule_list
  : '|' rs_rule
  | rs_rule_list '|' rs_rule
  ;

rs_rule_list_opt
  : rs_rule_list
  |
  ;

rs_production_list
  : rs_prod rs_prods_opt
  | K_rand K_join production_item production_item production_items_opt
  | K_rand K_join '(' expression ')' production_item production_item production_items_opt

rs_code_block
  : '{' data_declarations_opt statement_or_nulls_opt '}'

rs_code_block_opt
  : rs_code_block
  |
  ;

rs_prod
  : production_item
  | rs_code_block
  | rs_if_else
  | rs_repeat
  | rs_case
  ;

rs_prods
  : rs_prod
  | rs_prods rs_prod
  ;

rs_prods_opt
  : rs_prods
  |
  ;

production_item
  : identifier list_of_arguments_opt
  ;

production_items
  : production_item
  | production_items production_item
  ;

production_items_opt
  : production_items
  |
  ;

rs_if_else
  : K_if '(' expression ')' production_item 
  | K_if '(' expression ')' production_item K_else production_item
  ;

rs_repeat
  : K_repeat '(' expression ')' production_item
  ;

rs_case
  : K_case '(' expression ')' rs_case_item rs_case_items_opt K_endcase
  ;

rs_case_item
  : expression expression_list_opt ':' production_item ';'
  | K_default production_item ';'
  | K_default ':' production_item ';'
  ;

rs_case_items
  : rs_case_item
  | rs_case_items rs_case_item
  ;

rs_case_items_opt
  : rs_case_items
  |
  ;

  /* Specify section */

  /* Specify block declaration */

specify_block
  : K_specify specify_items_opt K_endspecify
  ;

specify_item
  : specparam_declaration
  | pulsestyle_declaration
  | showcancelled_declaration
  | path_declaration
  | system_timing_check
  ;

specify_items
  : specify_item
  | specify_items specify_item
  ;

specify_items_opt
  : specify_items
  |
  ;

pulsestyle_declaration
  : K_pulsestyle_onevent list_of_path_outputs ';'
  | K_pulsestyle_ondetect list_of_path_outputs ';'
  ;

showcancelled_declaration
  : K_showcancelled list_of_path_outputs ';'
  | K_noshowcancelled list_of_path_outputs ';'
  ;

  /* Specify path declarations */

path_declaration
  : simple_path_declaration ';'
  | edge_sensitive_path_declaration ';'
  | state_dependent_path_declaration ';'
  ;

simple_path_declaration
  : parallel_path_description '=' path_delay_value
  | full_path_description '=' path_delay_value
  ;

parallel_path_description
  : '(' specify_input_terminal_descriptor polarity_operator_opt K_EG specify_output_terminal_descriptor ')'
  ;

full_path_description
  : '(' list_of_path_inputs polarity_operator_opt K_SG list_of_path_outputs ')'
  ;

list_of_path_inputs
  : specify_input_terminal_descriptor specify_input_terminal_descriptor_list_opt
  ;

list_of_path_outputs
  : specify_output_terminal_descriptor specify_output_terminal_descriptor_list_opt
  ;

  /* Specify block terminals */

specify_input_terminal_descriptor
  : input_identifier
  | input_identifier '[' constant_range_expression ']'
  ;

specify_input_terminal_descriptor_list
  : ',' specify_input_terminal_descriptor
  | specify_input_terminal_descriptor_list ',' specify_input_terminal_descriptor
  ;

specify_input_terminal_descriptor_list_opt
  : specify_input_terminal_descriptor_list
  |
  ;

specify_output_terminal_descriptor
  : output_identifier
  | output_identifier '[' constant_range_expression ']'
  ;

specify_output_terminal_descriptor_list
  : ',' specify_output_terminal_descriptor
  | specify_output_terminal_descriptor_list ',' specify_output_terminal_descriptor
  ;

specify_output_terminal_descriptor_list_opt
  : specify_output_terminal_descriptor_list
  |
  ;

input_identifier
  : identifier
  | identifier '.' identifier
  ;

output_identifier
  : identifier
  | identifier '.' identifier
  ;

  /* Specify path delays */

path_delay_value
  : list_of_path_delay_expressions
  | '(' list_of_path_delay_expressions ')'
  ;

list_of_path_delay_expressions
  : path_delay_expression
  | path_delay_expression ',' path_delay_expression
  | path_delay_expression ',' path_delay_expression ',' path_delay_expression
  | path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression
  | path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression ',' path_delay_expression
  ;

path_delay_expression
  : constant_mintypmax_expression
  ;

edge_sensitive_path_declaration
  : parallel_edge_sensitive_path_description '=' path_delay_value
  | full_edge_sensitive_path_description '=' path_delay_value
  ;

parallel_edge_sensitive_path_description
  : '(' edge_identifier_opt specify_input_terminal_descriptor K_EG '(' specify_output_terminal_descriptor polarity_operator_opt ':' data_source_expression ')' ')'
  ;

full_edge_sensitive_path_description
  : '(' edge_identifier_opt list_of_path_inputs K_SG '(' list_of_path_outputs polarity_operator_opt ':' data_source_expression ')' ')'
  ;

data_source_expression
  : expression
  ;

edge_identifier
  : K_posedge
  | K_negedge
  ;

edge_identifier_opt
  : edge_identifier
  |
  ;

state_dependent_path_declaration
  : K_if '(' module_path_expression ')' simple_path_declaration
  | K_if '(' module_path_expression ')' edge_sensitive_path_declaration
  | K_ifnone simple_path_declaration
  ;

polarity_operator
  : '+'
  | '-'
  ;

polarity_operator_opt
  : polarity_operator
  |
  ;

  /* System timing checks */

  /* System timing check commands */

system_timing_check
  : Ssetup_timing_check
  | Shold_timing_check
  | Ssetuphold_timing_check
  | Srecovery_timing_check
  | Sremoval_timing_check
  | Srecrem_timing_check
  | Sskew_timing_check
  | Stimeskew_timing_check
  | Sfullskew_timing_check
  | Speriod_timing_check
  | Swidth_timing_check
  | Snochange_timing_check
  ;

Ssetup_timing_check
  : K_Ssetup '(' data_event ',' reference_event ',' timing_check_limit ')' ';'
  | K_Ssetup '(' data_event ',' reference_event ',' timing_check_limit ',' notifier_opt ')' ';'
  ;

Shold_timing_check
  : K_Shold '(' reference_event ',' data_event ',' timing_check_limit ')' ';'
  | K_Shold '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ')' ';'
  ;

Ssetuphold_timing_check
  : K_Ssetuphold '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ')' ';'
  | K_Ssetuphold '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ')' ';'
  | K_Ssetuphold '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ')' ';'
  | K_Ssetuphold '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ',' checktime_condition_opt ')' ';'
  | K_Ssetuphold '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ',' checktime_condition_opt ',' delayed_reference_opt ')' ';'
  | K_Ssetuphold '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ',' checktime_condition_opt ',' delayed_reference_opt ',' delayed_data_opt ')' ';'
  ;

Srecovery_timing_check
  : K_Srecovery '(' reference_event ',' data_event ',' timing_check_limit ')' ';'
  | K_Srecovery '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ')' ';'
  ;

Sremoval_timing_check
  : K_Sremoval '(' reference_event ',' data_event ',' timing_check_limit ')' ';'
  | K_Sremoval '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ')' ';'
  ;

Srecrem_timing_check
  : K_Srecrem '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ')' ';'
  | K_Srecrem '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ')' ';'
  | K_Srecrem '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ')' ';'
  | K_Srecrem '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ',' checktime_condition_opt ')' ';'
  | K_Srecrem '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ',' checktime_condition_opt ',' delayed_reference_opt ')' ';'
  | K_Srecrem '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' stamptime_condition_opt ',' checktime_condition_opt ',' delayed_reference_opt ',' delayed_data_opt ')' ';'
  ;

Sskew_timing_check
  : K_Sskew '(' reference_event ',' data_event ',' timing_check_limit ')' ';'
  | K_Sskew '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ')' ';'
  ;

Stimeskew_timing_check 
  : K_Stimeskew '(' reference_event ',' data_event ',' timing_check_limit ')' ';' 
  | K_Stimeskew '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ')' ';' 
  | K_Stimeskew '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ',' event_based_flag_opt ')' ';' 
  | K_Stimeskew '(' reference_event ',' data_event ',' timing_check_limit ',' notifier_opt ',' event_based_flag_opt ',' remain_active_flag_opt ')' ';' 
  ;

Sfullskew_timing_check
  : K_Sfullskew '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ')' ';'
  | K_Sfullskew '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ')' ';'
  | K_Sfullskew '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' event_based_flag_opt ')' ';'
  | K_Sfullskew '(' reference_event ',' data_event ',' timing_check_limit ',' timing_check_limit ',' notifier_opt ',' event_based_flag_opt ',' remain_active_flag_opt ')' ';'
  ;

Speriod_timing_check
  : K_Speriod '(' controlled_reference_event ',' timing_check_limit ')' ';'
  | K_Speriod '(' controlled_reference_event ',' timing_check_limit ',' notifier_opt ')' ';'
  ;

Swidth_timing_check
  : K_Swidth '(' controlled_reference_event ',' timing_check_limit ',' threshold ')' ';'
  | K_Swidth '(' controlled_reference_event ',' timing_check_limit ',' threshold ',' notifier_opt ')' ';'
  ;

Snochange_timing_check
  : K_Snochange '(' reference_event ',' data_event ',' start_edge_offset ',' end_edge_offset ')' ';'
  | K_Snochange '(' reference_event ',' data_event ',' start_edge_offset ',' end_edge_offset ',' notifier_opt ')' ';'
  ;

  /* System timing check command arguments */

checktime_condition
  : mintypmax_expression
  ;

checktime_condition_opt
  : checktime_condition
  |
  ;

controlled_reference_event
  : controlled_timing_check_event
  ;

data_event
  : timing_check_event
  ;

delayed_data
  : identifier
  | identifier '[' constant_mintypmax_expression ']'
  ;

delayed_data_opt
  : delayed_data
  |
  ;

delayed_reference
  : identifier
  | identifier '[' constant_mintypmax_expression ']'
  ;

delayed_reference_opt
  : delayed_reference
  |
  ;

end_edge_offset
  : mintypmax_expression
  ;

event_based_flag
  : constant_expression
  ;

event_based_flag_opt
  : event_based_flag
  |
  ;

notifier
  : identifier
  ;

notifier_opt
  : notifier
  |
  ;

reference_event
  : timing_check_event
  ;

remain_active_flag
  : constant_mintypmax_expression
  ;

remain_active_flag_opt
  : remain_active_flag
  |
  ;

stamptime_condition
  : mintypmax_expression
  ;

stamptime_condition_opt
  : stamptime_condition
  |
  ;

start_edge_offset
  : mintypmax_expression
  ;

threshold
  : constant_expression
  ;

timing_check_limit
  : expression
  ;

  /* System timing check event definitions */

timing_check_event
  : timing_check_event_control_opt specify_terminal_descriptor
  | timing_check_event_control_opt specify_terminal_descriptor K_TAND timing_check_condition
  ;

controlled_timing_check_event
  : timing_check_event_control specify_terminal_descriptor
  | timing_check_event_control specify_terminal_descriptor K_TAND timing_check_condition
  ;

timing_check_event_control
  : K_posedge
  | K_negedge
  | edge_control_specifier
  ;

timing_check_event_control_opt
  : timing_check_event_control
  |
  ;

specify_terminal_descriptor
  : specify_input_terminal_descriptor
  | specify_output_terminal_descriptor
  ;

edge_control_specifier
  : K_edge
  | K_edge { lex_start_udp_table(); } edge_descriptor { lex_end_udp_table(); } edge_descriptor_list_opt
  ;

edge_descriptor
  : '0' '1'
  | '1' '0'
  | z_or_x zero_or_one
  | zero_or_one z_or_x
  ;

edge_descriptor_list
  : ',' edge_descriptor
  | edge_descriptor_list ',' edge_descriptor
  ;

edge_descriptor_list_opt
  : edge_descriptor_list
  |
  ;

zero_or_one
  : '0'
  | '1'
  ;

z_or_x
  : 'x'
  | 'z'
  ;

timing_check_condition
  : scalar_timing_check_condition
  | '(' scalar_timing_check_condition ')'
  ;

scalar_timing_check_condition
  : expression
  | '~' expression
  | expression K_EQ scalar_constant
  | expression K_CEQ scalar_constant
  | expression K_NE scalar_constant
  | expression K_CNE scalar_constant
  ;

scalar_constant
  : NUMBER
  | UNUSED_NUMBER
  ;

  /* Expressions */

  /* Concatenations */

concatenation
  : '{' expression expression_list_opt '}'
  | '{' struct_or_array_member_label ':' expression struct_or_array_member_label_expression_list_opt '}'
  ;

struct_or_array_member_label_expression_list
  : ',' struct_or_array_member_label ':' expression
  | struct_or_array_member_label_expression_list ',' struct_or_array_member_label ':' expression
  ;

struct_or_array_member_label_expression_list_opt
  : struct_or_array_member_label_expression_list
  |
  ;

constant_concatenation
  : '{' constant_expression constant_expression_list_opt '}'
  | '{' struct_or_array_member_label ':' constant_expression struct_or_array_member_label_constant_expression_list_opt '}'
  ;

struct_or_array_member_label
  : K_default
  | constant_expression { /* identifier only valid for structs */ }
  ;

struct_or_array_member_label_constant_expression_list
  : ',' struct_or_array_member_label ':' constant_expression
  | struct_or_array_member_label_constant_expression_list ',' struct_or_array_member_label ':' constant_expression
  ;

struct_or_array_member_label_constant_expression_list_opt
  : struct_or_array_member_label_constant_expression_list
  |
  ;

constant_multiple_concatenation
  : '{' constant_expression constant_concatenation '}'
  ;

module_path_concatenation
  : '{' module_path_expression module_path_expression_list_opt '}'
  ;

module_path_multiple_concatenation
  : '{' constant_expression module_path_concatenation '}'
  ;

multiple_concatenation
  : '{' expression concatenation '}'
  ;

streaming_expression
  : '{' stream_operator slice_size stream_concatenation '}'
  | '{' stream_operator stream_concatenation '}'
  ;

stream_operator
  : K_LS
  | K_RS
  ;

slice_size
  : ps_identifier
  | constant_expression
  ;

stream_concatenation
  : '{' stream_expression stream_expression_list_opt '}'
  ;

stream_expression
  : expression
  | expression K_with array_range_expression_opt
  ;

stream_expression_list
  : ',' stream_expression
  | stream_expression_list ',' stream_expression
  ;

stream_expression_list_opt
  : stream_expression_list
  |
  ;

array_range_expression
  : expression
  | expression ':' expression
  | expression K_PO_POS expression
  | expression K_PO_NEG expression
  ;

array_range_expression_opt
  : array_range_expression
  |
  ;

empty_queue
  : '{' '}'
  ;

  /* Subroutine calls */

constant_function_call
  : function_subroutine_call
  ;

tf_call
  : ps_or_hierarchical_identifier attribute_instances_opt list_of_arguments_opt
  ;

system_tf_call
  : system_tf_identifier list_of_arguments_opt
  ;

subroutine_call
  : tf_call
  | system_tf_call
  | method_call
  | randomize_call
  ;

function_subroutine_call
  : subroutine_call
  ;

list_of_arguments
  : expression_opt expression_opt_list_opt identifier_expression_opt_list_opt
  | '.' identifier '(' expression_opt ')' identifier_expression_opt_list_opt
  ;

expression_opt_list
  : ',' expression_opt
  | expression_opt_list ',' expression_opt
  ;

expression_opt_list_opt
  : expression_opt_list
  |
  ;

identifier_expression_opt_list
  : ',' '.' identifier '(' expression_opt ')'
  | identifier_expression_opt_list ',' '.' identifier '(' expression_opt ')'
  ;

identifier_expression_opt_list_opt
  : identifier_expression_opt_list
  |
  ;

method_call
  : method_call_root_dot method_call_body
  ;

method_call_body
  : identifier attribute_instances_opt list_of_arguments_opt
  | built_in_method_call
  ;

built_in_method_call
  : array_manipulation_call
  | randomize_call
  ;

array_manipulation_call
  : array_method_name attribute_instances_opt list_of_arguments_opt
  | array_method_name attribute_instances_opt list_of_arguments_opt K_with '(' expression ')'
  ;

randomize_call
  : K_randomize attribute_instances_opt
  | K_randomize attribute_instances_opt '(' ')'
  | K_randomize attribute_instances_opt '(' variable_identifier_list ')'
  | K_randomize attribute_instances_opt '(' K_null ')'
  | K_randomize attribute_instances_opt K_with constraint_block
  | K_randomize attribute_instances_opt '(' ')' K_with constraint_block
  | K_randomize attribute_instances_opt '(' variable_identifier_list ')' K_with constraint_block
  | K_randomize attribute_instances_opt '(' K_null ')' K_with constraint_block
  ;

method_call_root_dot
  : expression '.'
  | implicit_class_handle_dot
  ;

array_method_name
  : identifier
  | K_unique
  | K_and
  | K_or
  | K_xor
  ;

  /* Expressions */

inc_or_dec_expression
  : inc_or_dec_operator attribute_instances_opt variable_lvalue
  | variable_lvalue attribute_instances_opt inc_or_dec_operator 
  ;

conditional_expression
  : cond_predicate '?' attribute_instances_opt expression ':' expression
  ;

constant_expression
  : constant_primary
  | unary_operator attribute_instances_opt constant_primary
  | constant_expression binary_operator attribute_instances_opt constant_expression
  | constant_expression '?' attribute_instances_opt constant_expression ':' constant_expression
  ;

constant_expression_list
  : ',' constant_expression
  | constant_expression_list ',' constant_expression
  ;

constant_expression_list_opt
  : constant_expression_list
  |
  ;

constant_mintypmax_expression
  : constant_expression
  | constant_expression ':' constant_expression ':' constant_expression
  ;

constant_param_expression
  : constant_mintypmax_expression
  | data_type
  | '$'
  ;

param_expression
  : mintypmax_expression
  | data_type
  ;

param_expression_opt
  : param_expression
  |
  ;

constant_range_expression
  : constant_expression
  | constant_part_select_range
  ;

constant_part_select_range
  : constant_range
  | constant_indexed_range
  ;

constant_range
  : constant_expression ':' constant_expression
  ;

constant_indexed_range
  : constant_expression K_PO_POS constant_expression
  | constant_expression K_PO_NEG constant_expression
  ;

expression
  : primary
  | unary_operator attribute_instances_opt primary
  | inc_or_dec_expression
  | '(' operator_assignment ')'
  | expression binary_operator attribute_instances_opt expression
  | conditional_expression
  | inside_expression
  | tagged_union_expression
  ;

expression_opt
  : expression
  |
  ;

expression_list
  : ',' expression
  | expression_list ',' expression
  ;

expression_list_opt
  : expression_list
  |
  ;

tagged_union_expression
  : K_tagged identifier expression_opt
  ;

inside_expression
  : expression K_inside open_range_lists_opt
  ;

value_range
  : expression
  | '[' expression ':' expression ']'
  ;

mintypmax_expression
  : expression
  | expression ':' expression ':' expression
  ;

module_path_conditional_expression
  : module_path_expression '?' attribute_instances_opt module_path_expression ':' module_path_expression
  ;

module_path_expression
  : module_path_primary
  | unary_module_path_operator attribute_instances_opt module_path_primary
  | module_path_expression binary_module_path_operator attribute_instances_opt module_path_expression
  | module_path_conditional_expression
  ;

module_path_expression_list
  : ',' module_path_expression
  | module_path_expression_list ',' module_path_expression
  ;

module_path_expression_list_opt
  : module_path_expression_list
  |
  ;

module_path_mintypmax_expression
  : module_path_expression
  | module_path_expression ':' module_path_expression ':' module_path_expression
  ;

part_select_range
  : constant_range
  | indexed_range
  ;

indexed_range
  : expression K_PO_POS constant_expression
  | expression K_PO_NEG constant_expression
  ;

  /* Primaries */

constant_primary
  : primary_literal
  | ps_identifier
  | identifier
  | package_scope identifier
  | class_scope identifier
  | constant_concatenation
  | constant_multiple_concatenation
  | constant_function_call
  | '(' constant_mintypmax_expression ')'
  | constant_cast
  ;

module_path_primary
  : number
  | identifier
  | module_path_concatenation
  | module_path_multiple_concatenation
  | function_subroutine_call
  | '(' module_path_mintypmax_expression ')'
  ;

primary
  : primary_literal
  | hierarchical_identifier select
  | implicit_class_handle_dot hierarchical_identifier select
  | class_scope hierarchical_identifier select
  | package_scope hierarchical_identifier select
  | empty_queue
  | concatenation
  | multiple_concatenation
  | function_subroutine_call
  | '(' mintypmax_expression ')'
  | cast
  | streaming_expression
  | sequence_method_call
  | '$'
  | K_null
  ;

time_literal
  : NUMBER { lex_start_time_unit(); } time_unit { lex_end_time_unit(); }
  | UNUSED_NUMBER { lex_start_time_unit(); } time_unit { lex_end_time_unit(); }
  | NUMBER '.' NUMBER { lex_start_time_unit(); } time_unit { lex_end_time_unit(); }
  | UNUSED_NUMBER '.' UNUSED_NUMBER { lex_start_time_unit(); } time_unit { lex_end_time_unit(); }
  ;

time_unit
  : K_TU_S
  | K_TU_MS
  | K_TU_US
  | K_TU_NS
  | K_TU_PS
  | K_TU_FS
  | K_TU_STEP
  ;

implicit_class_handle_dot
  : K_this '.'
  | K_super '.'
  | K_this '.' K_super '.'
  ;

select
  : select_expression_list_opt
  | select_expression_list_opt '[' part_select_range ']'
  ;

select_expression_list
  : '[' expression ']'
  | select_expression_list '[' expression ']'
  ;

select_expression_list_opt
  : select_expression_list
  |
  ;

constant_select
  : select_constant_expression_list_opt
  | select_constant_expression_list_opt '[' constant_part_select_range ']'
  ;

select_constant_expression_list
  : '[' constant_expression ']'
  | select_constant_expression_list '[' constant_expression ']'
  ;

select_constant_expression_list_opt
  : select_constant_expression_list
  |
  ;

primary_literal
  : number
  | time_literal
  | string_literal
  ;

constant_cast
  : casting_type '\'' '(' constant_expression ')'
  | casting_type '\'' constant_concatenation
  | casting_type '\'' constant_multiple_concatenation
  ;

cast
  : casting_type '\'' '(' expression ')'
  | casting_type '\'' concatenation
  | casting_type '\'' multiple_concatenation
  ;

  /* Expression left-side values */

net_lvalue
  : ps_or_hierarchical_identifier constant_select
  | '{' net_lvalue net_lvalue_list_opt '}'
  ;

net_lvalue_list
  : ',' net_lvalue
  | net_lvalue_list ',' net_lvalue
  ;

net_lvalue_list_opt
  : net_lvalue_list
  |
  ;

variable_lvalue
  : hierarchical_identifier select
  | implicit_class_handle_dot hierarchical_identifier select
  | package_scope hierarchical_identifier select
  | '{' variable_lvalue variable_lvalue_list_opt '}'
  ;

variable_lvalue_list
  : ',' variable_lvalue
  | variable_lvalue_list ',' variable_lvalue
  ;

variable_lvalue_list_opt
  : variable_lvalue_list
  |
  ;

  /* Operators */

unary_operator
  : '+'
  | '-'
  | '!'
  | '~'
  | '&'
  | K_NAND
  | '|'
  | K_NOR
  | '^'
  | K_NXOR
  ;

binary_operator
  : '+'
  | '-'
  | '*'
  | '/'
  | '%'
  | K_EQ
  | K_NE
  | K_CEQ
  | K_CNE
  | K_PEQ
  | K_PNE
  | K_LAND
  | K_LOR
  | K_POW
  | '<'
  | K_LE
  | '>'
  | K_GE
  | '&'
  | '|'
  | '^'
  | K_NXOR
  | K_LS
  | K_RS
  | K_LSS
  | K_RSS
  ;

inc_or_dec_operator
  : K_INC
  | K_DEC
  ;

unary_module_path_operator
  : '!'
  | '~'
  | '&'
  | K_NAND
  | '|'
  | K_NOR
  | '^'
  | K_NXOR
  ;

binary_module_path_operator
  : K_EQ
  | K_NE
  | K_LAND
  | K_LOR
  | '&'
  | '|'
  | '^'
  | K_NXOR
  ;

  /* Numbers */

number
  : NUMBER
  | UNUSED_NUMBER
  | REALTIME
  | UNUSED_REALTIME
  ;

  /* Strings */

string_literal
  : STRING
  | UNUSED_STRING
  ;

  /* General */

  /* Attributes */

attribute_instance
  : K_PSTAR attr_spec attr_spec_list_opt K_STARP

attribute_instances
  : attribute_instance
  | attribute_instances attribute_instance
  ;

attribute_instances_opt
  : attribute_instances
  |
  ;

attr_spec
  : identifier
  | identifier '=' constant_expression
  ;

attr_spec_list
  : ',' attr_spec
  | attr_spec_list ',' attr_spec
  ;

attr_spec_list_opt
  : attr_spec_list
  |
  ;

  /* Identifiers */

c_identifier
  : C_IDENTIFIER
  | UNUSED_C_IDENTIFIER
  ; /* ::= [ a - zA - Z_ ] { [ a - zA - Z0 - 9_ ] } */

escaped_identifier
  : ESCAPED_IDENTIFIER
  | UNUSED_ESCAPED_IDENTIFIER
  ; /* '\' {any_ASCII_character_except_white_space} white_space */

hierarchical_identifier
  : hierarchical_identifier_tail
  | K_Sroot '.' hierarchical_identifier_tail
  ;

hierarchical_identifier_tail
  : identifier
  | identifier select_constant_expression_list_opt '.' hierarchical_identifier_list
  ;

hierarchical_identifier_list
  : ',' hierarchical_identifier
  | hierarchical_identifier_list ',' hierarchical_identifier
  ;

hierarchical_identifier_list_opt
  : hierarchical_identifier_list
  |
  ;

identifier
  : simple_identifier
  | escaped_identifier
  ;

identifier_opt
  : identifier
  |
  ;

identifier_opt_list
  : ',' identifier_opt
  | identifier_opt_list ',' identifier_opt
  ;

identifier_opt_list_opt
  : identifier_opt_list
  |
  ;

package_scope
  : identifier ':' ':'
  | K_Sunit ':' ':'
  ;

ps_identifier
  : identifier
  | package_scope identifier
  ;

ps_or_hierarchical_identifier
  : ps_identifier
  | hierarchical_identifier
  ;

simple_identifier
  : SIMPLE_IDENTIFIER
  | UNUSED_SIMPLE_IDENTIFIER
  ; /* ::= [ a - zA - Z_ ] { [ a - zA - Z0 - 9_$ ] } */

system_tf_identifier
  : SYSTEM_IDENTIFIER
  | UNUSED_SYSTEM_IDENTIFIER
  ;

  /* Miscellaneous rules not found specifically in the spec */

postfix_identifier_opt
  : ':' identifier
  |
  ;

postfix_new_opt
  : ':' K_new
  |
  ;

net_lvalue_assign_list
  : '=' net_lvalue
  | net_lvalue_assign_list '=' net_lvalue
  ;

net_lvalue_assign_list_opt
  : net_lvalue_assign_list
  |
  ;

list_of_arguments_opt
  : '(' list_of_arguments ')'
  |
  ;

list_of_formals_opt
  : '(' list_of_formals ')'
  |
  ;

tf_port_lister_opt
  : '(' tf_port_list_opt ')'
  |
  ;

actual_arg_lister_opt
  : '(' actual_arg_list ')'
  |
  ;

identifiers
  : identifier
  | identifiers identifier
  ;

identifiers_opt
  : identifiers
  |
  ;

lex_start_import_export
  : { lex_start_import_export(); }
  ;

lex_end_import_export
  : { lex_end_import_export(); }
  ;
