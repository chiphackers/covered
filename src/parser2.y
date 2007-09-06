
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
  vector*         number;
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

%token <text>     IDENTIFIER SYSTEM_IDENTIFIER
%token <typdef>   TYPEDEF_IDENTIFIER
%token <text>     PATHPULSE_IDENTIFIER
%token <number>   NUMBER
%token <realtime> REALTIME
%token <number>   STRING
%token IGNORE
%token UNUSED_IDENTIFIER
%token UNUSED_PATHPULSE_IDENTIFIER
%token UNUSED_NUMBER
%token UNUSED_REALTIME
%token UNUSED_STRING UNUSED_SYSTEM_IDENTIFIER
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_LSS K_RS K_RSS K_SG
%token K_ADD_A K_SUB_A K_MLT_A K_DIV_A K_MOD_A K_AND_A K_OR_A K_XOR_A K_LS_A K_RS_A K_ALS_A K_ARS_A K_INC K_DEC
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
%token K_automatic K_cell K_use K_library K_config K_endconfig K_design K_liblist K_instance
%token K_showcancelled K_noshowcancelled K_pulsestyle_onevent K_pulsestyle_ondetect

%token K_bool K_bit K_byte K_logic K_char K_shortint K_int K_longint K_unsigned
%token K_unique K_priority K_do
%token K_always_comb K_always_latch K_always_ff
%token K_typedef K_type K_enum K_union K_struct K_packed
%token K_assert K_property K_endproperty K_cover K_sequence K_endsequence
%token K_program K_endprogram K_final K_void K_return K_continue K_break K_extern K_interface K_endinterface
%token K_class K_endclass K_extends K_package K_endpackage K_timeunit K_timeprecision K_ref K_bind K_const
%token K_new K_static K_protected K_local K_rand K_randc K_constraint K_import K_scalared K_chandle

%token KK_attribute

%type <logical>   automatic_opt block_item_decls_opt
%type <integer>   net_type net_type_sign_range_opt var_type
%type <text>      identifier begin_end_id
%type <text>      udp_port_list
%type <text>      defparam_assign_list defparam_assign
%type <statexp>   static_expr static_expr_primary static_expr_port_list
%type <expr>      expr_primary expression_list expression expression_port_list
%type <expr>      lavalue lpvalue
%type <expr>      event_control event_expression_list event_expression
%type <expr>      delay_value delay_value_simple
%type <expr>      delay1 delay3 delay3_opt
%type <expr>      generate_passign index_expr single_index_expr
%type <state>     statement statement_list statement_opt
%type <state>     if_statement_error
%type <state>     passign
%type <strlink>   gate_instance gate_instance_list list_of_names
%type <funit>     begin_end_block fork_statement
%type <attr_parm> attribute attribute_list
%type <portinfo>  port_declaration list_of_port_declarations
%type <gitem>     generate_item generate_item_list generate_item_list_opt
%type <case_stmt> case_items case_item
%type <case_gi>   generate_case_items generate_case_item

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
%left '*' '/' '%'
%left UNARY_PREC

/* to resolve dangling else ambiguity: */
%nonassoc less_than_K_else
%nonassoc K_else

%%

  /* SOURCE TEXT */

  /* Library source text - CURRENTLY NOT SUPPORTED - TBD */
library_text
  : library_descriptions
  |
  ;

library_descriptions
  : library_declaration
  | include_statement
  | config_declaration
  |
  ;

library_declaration
  : K_library library_identifier file_path_specs incdir_opt

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

incdir_opt
  : K_incdiropt file_path_specs
  |
  ;

  /* Configuration source text - CURRENTLY NOT SUPPORTED - TBD */
config_declaration
  : K_config config_identifier ';'
    design_statement
    config_rule_statements
    K_endconfig postfix_identifier_opt

design_statement
  : K_design library_identifiers_opt

config_rule_statement
  : default_clause liblist_clause
  | inst_clause liblist_clause
  | inst_clause use_clause
  | cell_clause liblist_clause
  | cell_clause use_clause
  |
  ;

default_clause
  : K_default
  ;

inst_clause
  : instance inst_name
  ;

inst_name
  : topmodule_identifier { . instance_identifier }

cell_clause
  : K_cell library_identifier
  ;

liblist_clause
  : K_liblist identifiers_opt
  ;

use_clause
  : K_use library_identifier
  | K_use library_identifier ':' K_config
  ;

library_identifier
  : identifier
  | identifier '.' identifier
  ;

library_identifiers
  : library_identifier
  | library_identifiers library_identifier
  ;

library_identifiers_opt
  : library_identifiers
  |
  ;

  /* Configuration source text */
source_text
  : timeunits_declaration_opt descriptions_opt

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

descriptions_opt
  : descriptions
  |
  ;

module_nonansi_header
  : attribute_instances_opt module_keyword lifetime_opt module_identifier parameter_port_list_opt list_of_ports ';'
  ;

module_ansi_header
  : attribute_instances_opt module_keyword lifetime_opt module_identifier parameter_port_list_opt list_of_port_declarations_opt ';'
  ;

module_declaration
  : module_nonansi_header timeunits_declaration_opt module_items_opt K_endmodule postfix_identifier_opt
  | module_ansi_header timeunits_declaration_opt non_port_module_items_opt K_endmodule postfix_identifier_opt
  | attribute_instances_opt module_keyword lifetime_opt module_identifier '(' '.' '*' ')' ';' timeunits_declaration_opt module_items_opt K_endmodule postfix_identifier_opt
  | K_extern module_nonansi_header
  | K_extern module_ansi_header
  ;

module_keyword
  : K_module
  | K_macromodule
  ;

interface_nonansi_header
  : attribute_instances_opt K_interface lifetime_opt interface_identifier parameter_port_list_opt list_of_ports ';'
  ;

interface_ansi_header
  : attribute_instances_opt K_interface lifetime_opt interface_identifier parameter_port_list_opt list_of_port_declarations_opt ';' 
  ;

interface_declaration
  : interface_nonansi_header timeunits_declaration_opt interface_items_opt K_endinterface postfix_identifier_opt
  | interface_ansi_header timeunits_declaration_opt non_port_interface_items_opt K_endinterface postfix_identifier_opt
  | attribute_instances_opt K_interface interface_identifier '(' '.' '*' ')' ';' timeunits_declaration_opt interface_items_opt K_endinterface postfix_identifier_opt
  | K_extern interface_nonansi_header
  | K_extern interface_ansi_header
  ;

program_nonansi_header
  : attribute_instances_opt K_program lifetime_opt program_identifier parameter_port_list_opt list_of_ports ';'
  ;

program_ansi_header
  : attribute_instances_opt K_program lifetime_opt program_identifier parameter_port_list_opt list_of_port_declarations_opt ';'
  ;

program_declaration
  : program_nonansi_header timeunits_declaration_opt program_items_opt K_endprogram postfix_identifier_opt
  | program_ansi_header timeunits_declaration_opt non_port_program_items_opt K_endprogram postfix_identifier_opt
  | attribute_instances_opt K_program program_identifier '(' '.' '*' ')' ';' timeunits_declaration_opt program_items_opt K_endprogram postfix_identifier_opt
  | K_extern program_nonansi_header
  | K_extern program_ansi_header
  ;

class_declaration
  : virtual_opt K_class lifetime_opt class_identifier parameter_port_list_opt class_extension_opt ';' class_items_opt K_endclass postfix_identifier_opt
  ;

class_extension_opt
  : K_extends class_type list_of_arguments_opt
  |
  ;

package_declaration
  : attribute_instances_opt K_package package_identifier ';' timeunits_declaration_opt package_attrib_items_opt K_endpackage postfix_identifier_opt
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

parameter_port_declaration
  : parameter_declaration
  | data_type list_of_param_assignments
  | type list_of_type_assignments
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
  | '.' port_identifier '(' port_expression_opt ')'

port_expression
  : port_reference
  | { port_reference { , port_reference } }

port_expression_opt
  : port_expression
  |
  ;

port_reference
  : port_identifier constant_select
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

variable_port_header_opt
  : variable_port_header
  |
  ;

interface_port_header
  : interface_identifier
  | interface_identifier '.' modport_identifier
  | K_interface
  | K_interface '.' modport_identifier
  ;

ansi_port_declaration
  : net_port_header_or_interface_port_header_opt port_identifier unpacked_dimensions_opt
  | variable_port_header_opt port_identifier variable_dimension
  | variable_port_header_opt port_identifier variable_dimension '=' constant_expression
  | net_port_header_or_interface_port_header_opt '.' port_identifier '(' expression_opt ')'
  ;

net_port_header_or_interface_port_header_opt
  : net_port_header
  | interface_port_header
  |
  ;

  /* Module items */
module_common_item
  : module_or_generate_item_declaration
  | interface_instantiation
  | program_instantiation
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

module_or_generate_item
  : attribute_instances_opt parameter_override
  | attribute_instances_opt gate_instantiation
  | attribute_instances_opt udp_instantiation
  | attribute_instances_opt module_instantiation
  | attribute_instances_opt module_common_item
  ;

module_or_generate_item_declaration
  : package_or_generate_item_declaration
  | genvar_declaration
  | clocking_declaration
  | K_default clocking clocking_identifier ';'
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

parameter_override
  : K_defparam list_of_defparam_assignments ';'
  ;

bind_directive
  : K_bind hierarchical_identifier constant_select bind_instantiation ';'
  ;

bind_instantiation
  : program_instantiation
  | module_instantiation
  | interface_instantiation 
  ;

  /* Interface items */
interface_or_generate_item
  : attribute_instances_opt module_common_item
  | attribute_instances_opt modport_declaration
  | attribute_instances_opt extern_tf_declaration
  ;

extern_tf_declaration
  : K_extern method_prototype ';'
  | K_extern forkjoin task_prototype ';'
  ;

interface_item
  : port_declaration ';'
  | non_port_interface_item
  ;

non_port_interface_item
  : generated_interface_instantiation
  | attribute_instances_opt specparam_declaration
  | interface_or_generate_item
  | program_declaration
  | interface_declaration
  | timeunits_declaration 
  ;

  /* Program items */
program_item
  : port_declaration ';'
  | non_port_program_item
  ;

non_port_program_item
  : attribute_instances_opt continuous_assign
  | attribute_instances_opt module_or_generate_item_declaration
  | attribute_instances_opt specparam_declaration
  | attribute_instances_opt initial_construct
  | attribute_instances_opt concurrent_assertion_item
  | attribute_instances_opt timeunits_declaration 
  ;

  /* Class items */
class_item
  : attribute_instances_opt class_property
  | attribute_instances_opt class_method
  | attribute_instances_opt class_constraint
  | attribute_instances_opt type_declaration
  | attribute_instances_opt class_declaration
  | attribute_instances_opt timeunits_declaration
  |
  ;

class_property
  : property_qualifiers_opt data_declaration
  | K_const class_item_qualifiers_opt data_type const_identifier ';'
  | K_const class_item_qualifiers_opt data_type const_identifier '=' constant_expression ';'
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

property_qualifier
  : K_rand
  | K_randc
  | class_item_qualifier
  ;

method_qualifier
  : K_virtual
  | class_item_qualifier 
  ;

method_qualifier
  : K_virtual
  | class_item_qualifier
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
  : K_function class_scope_opt K_new tf_port_list_opt ';'
    block_item_declarations_opt super_opt
    function_statement_or_nulls_opt
    K_endfunction postfix_new_opt
  ;

super_opt
  : K_super '.' K_new list_of_arguments_opt ';'
  |
  ;

  /* Constraints */
constraint_declaration
  : K_static K_constraint constraint_identifier constraint_block
  | K_constraint constraint_identifier constraint_block
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
  | K_foreach '(' array_identifier loop_variables_opt ')' constraint_set
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
  : K_static K_constraint constraint_identifier ';'
  | K_constraint constraint_identifier ;
  ;

extern_constraint_declaration
  : K_static K_constraint class_scope constraint_identifier constraint_block
  | K_constraint class_scope constraint_identifier constraint_block
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
  |
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
  : interface_identifier list_of_interface_identifiers
  | interface_identifier '.' modport_identifier list_of_interface_identifiers
  ;

ref_declaration
  : K_ref data_type list_of_port_identifiers
  ;

  /* Type declarations */

data_declaration
  : K_const lifetime_opt variable_declaration
  | lifetime_opt variable_declaration
  | type_declaration
  | package_import_declaration
  | virtual_interface_declaration
  ;

package_import_declaration
  : K_import package_import_item { , package_import_item } ;

package_import_item
  : package_identifier ':' ':' identifier
  | package_identifier ':' ':' '*'
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
  : K_typedef data_type type_identifier variable_dimension ';'
  | K_typedef interface_instance_identifier '.' type_identifier type_identifier ';'
  | K_typedef esuc_opt type_identifier ';'
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
  | size
  | signing 
  ;

data_type
  : integer_vector_type signing_opt packed_dimensions_opt
  | integer_atom_type signing_opt
  | non_integer_type
  | struct_union packed_opt '{' struct_union_member struct_union_members_opt '}' packed_dimensions_opt
  | K_enum enum_base_type_opt '{' enum_name_declaration enum_name_declarations_opt '}'
  | K_string
  | K_chandle
  | K_virtual K_interface interface_identifier
  | K_virtual interface_identifier
  | type_identifier packed_dimensions_opt
  | class_scope type_identifier packed_dimensions_opt
  | package_scope type_identifier packed_dimensions_opt
  | class_type
  | K_event
  | ps_covergroup_identifier
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
  | type_identifier packed_dimension_opt
  ;

enum_name_declaration
  : enum_identifier
  | enum_identifier '[' integral_number ']'
  | enum_identifier '[' integral_number ':' integral_number ']'
  | enum_identifier '=' constant_expression
  | enum_identifier '[' integral_number ']' '=' constant_expression
  | enum_identifier '[' integral_number ':' integral_number ']' '=' constant_expression
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

class_type
  : ps_class_identifier parameter_value_assignment_opt class_identifier_and_param_value_assignments_opt
  ;

class_identifier_and_param_value_assignments
  : ':' ':' class_identifier parameter_value_assignment_opt
  | class_identifier_and_param_value_assignments ':' ':' class_identifier parameter_value_assignment_opt
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

signing
  : K_signed
  | K_unsigned
  ;

simple_type
  : integer_type
  | non_integer_type
  | ps_type_identifier
  ;

struct_union_member
  : attribute_instances_opt data_type_or_void list_of_variable_identifiers ';'
  ;

data_type_or_void
  : data_type
  | K_void
  ;

struct_union
  : K_struct
  | K_union tagged_opt

  /* Strengths */

drive_strength
  : '(' strength0 ',' strength1 ')'
  | '(' strength1 ',' strength0 ')'
  | '(' strength0 ',' highz1 ')'
  | '(' strength1 ',' highz0 ')'
  | '(' highz0 ',' strength1 ')'
  | '(' highz1 ',' strength0 ')'
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
  | '#' ( mintypmax_expression ')'
  | '#' ( mintypmax_expression ',' mintypmax_expression ')'
  ;

delay_value
  : unsigned_number
  | real_number
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
  : genvar_identifier { , genvar_identifier } 

genvar_identifiers_list
  : ',' genvar_identifier
  | genvar_identifiers_list ',' genvar_identifier
  ;

genvar_identifiers_list_opt
  : genvar_identifiers_list
  |
  ;

list_of_interface_identifiers
  : interface_identifier unpacked_dimensions_opt interface_identifiers_list_opt
  ;

interface_identifiers_list
  : ',' interface_identifier unpacked_dimensions_opt
  | interface_identifiers_list ',' interface_identifier unpacked_dimensions_opt
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
  : port_identifier unpacked_dimensions_opt port_identifiers_list_opt
  ;

port_identifiers_list
  : ',' port_identifier unpacked_dimensions_opt
  | port_identifiers_list ',' port_identifier unpacked_dimensions_opt
  ;

port_identifiers_list_opt
  : port_identifiers_list
  |
  ;

list_of_udp_port_identifiers ::= port_identifier { , port_identifier }

list_of_specparam_assignments ::= specparam_assignment { , specparam_assignment }

list_of_tf_variable_identifiers ::= port_identifier variable_dimension [ = expression ]
{ , port_identifier variable_dimension [ = expression ] }

list_of_type_assignments ::= type_assignment { , type_assignment }

list_of_variable_decl_assignments ::= variable_decl_assignment { , variable_decl_assignment }

list_of_variable_identifiers ::= variable_identifier variable_dimension
{ , variable_identifier variable_dimension }

list_of_variable_port_identifiers ::= port_identifier variable_dimension [ = constant_expression ]
{ , port_identifier variable_dimension [ = constant_expression ] }

list_of_virtual_interface_decl ::=
variable_identifier [ = interface_instance_identifier ]
{ , variable_identifier [ = interface_instance_identifier ] }
Declaration assignments

defparam_assignment ::= hierarchical_parameter_identifier = constant_mintypmax_expression

net_decl_assignment ::= net_identifier { unpacked_dimension } [ = expression ]

param_assignment ::= parameter_identifier { unpacked_dimension } = constant_param_expression

specparam_assignment ::=
specparam_identifier = constant_mintypmax_expression
| pulse_control_specparam

type_assignment ::= type_identifier = data_type 




  /* Gets created later on */

postfix_identifier_opt
  : ':' identifier
  |
  ;

postfix_new_opt
  : ':' K_new
  |
  ;

list_of_arguments_opt
  : '(' list_of_arguments ')'
  |
  ;

tf_port_lister_opt
  : '(' tf_port_list_opt ')'
  |
  ;

identifiers
  : ',' identifier
  | identifiers ',' identifier
  ;

identifiers_opt
  : identifiers
  |
  ;
