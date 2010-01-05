#ifndef __PARSER_FUNC_H__
#define __PARSER_FUNC_H__

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
 \file    parser_func.h
 \author  Trevor Williams  (phase1geo@gmail.com)
 \date    12/1/2008
 \brief   Contains functionality dealing with the Verilog parser.
*/

#include "defines.h"


/*! \brief Checks the (* ... *) syntax. */
void parser_check_pstar();

/*! \brief Parses and checks attributes. */
void parser_check_attribute(
  attr_param* ap
);
    
/*! \brief Appends an attribute to an attribute list. */
attr_param* parser_append_to_attr_list(
  attr_param* ap_list,
  attr_param* attr
);

/*! \brief Creates a new attribute parameter structure. */
attr_param* parser_create_attr(
  char*       name,
  expression* expr
);

/*! \brief Creates a task declaration (scope). */
void parser_create_task_decl(
  bool         automatic,
  char*        name,
  char*        orig_fname,
  char*        incl_fname,
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Creates the body of the current task. */
void parser_create_task_body(      
  statement*   stmt,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates a function declaration (scope). */
void parser_create_function_decl(
  bool         automatic,
  char*        name,
  char*        orig_fname,
  char*        incl_fname,
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Creates the body of the current function. */
void parser_create_function_body(
  statement* stmt
);

/*! \brief Completes the instantiation of the current function/task. */
void parser_end_task_function(
  unsigned int last_line
);

/*! \brief Creates an inline port (V2001 version). */
port_info* parser_create_inline_port(
  char*        name,
  int          sig_type,
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Handles inline port errors. */
port_info* parser_handle_inline_port_error();

/*! \brief Creates a simple number. */
const_value parser_create_simple_number(
  char* str_num
);

/*! \brief Creates a complex number. */
const_value parser_create_complex_number(
  char* str_num1,
  char* str_num2
);

/*! \brief Creates a port out of a static expression and appends it to the given static expression list. */
static_expr* parser_append_se_port_list(
  static_expr* sel,
  static_expr* se,
  unsigned int sel_first_line,
  unsigned int sel_ppfline,
  unsigned int sel_pplline,
  unsigned int sel_first_column,
  unsigned int se_first_line,
  unsigned int se_ppfline,
  unsigned int se_pplline,
  unsigned int se_first_column,
  unsigned int se_last_column
);

/*! \brief Creates a port out of a static expression. */
static_expr* parser_create_se_port_list(
  static_expr* se,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates a unary static expression. */
static_expr* parser_create_unary_se(
  static_expr* se,
  exp_op_type  op,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates a system call static expression. */
static_expr* parser_create_syscall_se(
  exp_op_type  op,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates a unary expression. */
expression* parser_create_unary_exp(
  expression*  exp,
  exp_op_type  op,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates a binary expression. */
expression* parser_create_binary_exp(
  expression* lexp,
  expression* rexp,
  exp_op_type op,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates an op-and-assign expression. */
expression* parser_create_op_and_assign_exp(
  char*        name,
  exp_op_type  op,
  unsigned int first_line1,
  unsigned int ppfline1,
  unsigned int pplline1,
  unsigned int first_column1,
  unsigned int last_column1,
  unsigned int last_column2
);

/*! \brief Creates an expression for a system call. */
expression* parser_create_syscall_exp(
  exp_op_type  op,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates an expression for a system call with a parameter list. */
expression* parser_create_syscall_w_params_exp(
  exp_op_type  op,
  expression*  plist,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Creates an op-and-assign expression. */  
expression* parser_create_op_and_assign_w_dim_exp(
  char*        name,
  exp_op_type  op,
  expression*  dim_exp,
  unsigned int first_line,
  unsigned int ppfline,
  unsigned int pplline,
  unsigned int first_column,
  unsigned int last_column
);

/*! \brief Adds the specified case statement item to the statement tree. */
void parser_handle_case_statement(
  exp_op_type  case_op,
  expression*  cs_expr,
  expression*  c_expr,
  statement*   cs_stmt,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  statement**  last_stmt
);

/*! \brief Adds each expression within an expression list as a cast statement item. */
void parser_handle_case_statement_list(
  exp_op_type  case_op,
  expression*  cs_expr,
  expression*  c_expr,
  statement*   cs_stmt,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  statement**  last_stmt
);

/*! \brief Addes the given expression as a case statement item. */
void parser_handle_generate_case_statement(
  expression*  cs_expr,
  expression*  c_expr,
  gen_item*    gi,
  unsigned int line,
  gen_item**   last_gi
);

/*! \brief Parses an expression tree list structure, adding each expression within the list as an individual case expression. */
void parser_handle_generate_case_statement_list(
  expression*  cs_expr,
  expression*  c_expr,
  gen_item*    gi,
  unsigned int line,
  gen_item**   last_gi
);

#endif

