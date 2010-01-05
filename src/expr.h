#ifndef __EXPR_H__
#define __EXPR_H__

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
 \file     expr.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/1/2001
 \brief    Contains functions for handling expressions.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Creates, initializes and adds a non-blocking assignment structure to the given expression's element pointer. */
void expression_create_nba(
  expression* expr,
  vsignal*    lhs_sig,
  vector*     rhs_vec
);

/*! \brief Returns a pointer to the non-blocking assignment expression if the given expression is an assignable expression on the LHS of a non-blocking assignment. */
expression* expression_is_nba_lhs(
  expression* exp
);

/*! \brief Creates new expression. */
expression* expression_create(
  /*@null@*/ expression*  right,
  /*@null@*/ expression*  left,
             exp_op_type  op,
             bool         lhs,
             int          id,
             unsigned int line,
             unsigned int ppfline,
             unsigned int pplline,
             unsigned int first,
             unsigned int last,
             bool         data
);

/*! \brief Sets the specified expression value to the specified vector value. */
void expression_set_value( expression* exp, vsignal* sig, func_unit* funit );

/*! \brief Sets the signed bit for all appropriate parent expressions */
void expression_set_signed( expression* exp );

/*! \brief Recursively resizes specified expression tree leaf node. */
void expression_resize( expression* expr, func_unit* funit, bool recursive, bool alloc );

/*! \brief Returns expression ID of this expression. */
int expression_get_id( expression* expr, bool parse_mode );

/*! \brief Returns first line in this expression tree. */
expression* expression_get_first_line_expr( expression* expr );

/*! \brief Returns last line in this expression tree. */
expression* expression_get_last_line_expr( expression* expr );

/*! \brief Returns the current dimension of the given expression. */
unsigned int expression_get_curr_dimension( expression* expr );

/*! \brief Finds all RHS signals in given expression tree */
void expression_find_rhs_sigs( expression* expr, str_link** head, str_link** tail );

/*! \brief Finds the expression in this expression tree with the specified underline id. */
expression* expression_find_uline_id( expression* expr, int ulid );

/*! \brief Returns TRUE if the specified expression exists within the given root expression tree */
bool expression_find_expr( expression* root, expression* expr );

/*! \brief Searches for an expression that calls the given statement */
bool expression_contains_expr_calling_stmt( expression* expr, statement* stmt );

/*! \brief Finds the root statement for the given expression */
statement* expression_get_root_statement( expression* exp );

/*! \brief Assigns each expression in the given tree a unique identifier */
void expression_assign_expr_ids( expression* root, func_unit* funit );

/*! \brief Writes this expression to the specified database file. */
void expression_db_write( expression* expr, FILE* file, bool parse_mode, bool ids_issued );

/*! \brief Writes the entire expression tree to the specified data file. */
void expression_db_write_tree( expression* root, FILE* file );

/*! \brief Reads current line of specified file and parses for expression information. */
void expression_db_read( char** line, /*@null@*/func_unit* curr_mod, bool eval );

/*! \brief Reads and merges two expressions and stores result in base expression. */
void expression_db_merge(
  expression* base,
  char**      line,
  bool        same
);

/*! \brief Merges two expressions into the base expression. */
void expression_merge(
  expression* base,
  expression* other
);

/*! \brief Returns user-readable name of specified expression operation. */
const char* expression_string_op(
  int op
);

/*! \brief Returns user-readable version of the supplied expression. */
char* expression_string(
  expression* exp
);

/*! \brief Displays the specified expression information. */
void expression_display(
  expression* expr
);

/*! \brief Performs operation specified by parameter expression. */
bool expression_operate(
  expression*     expr,
  thread*         thr,
  const sim_time* time
);

/*! \brief Performs recursive expression operation (parse mode only). */
void expression_operate_recursively(
  expression* expr,
  func_unit*  funit,
  bool        sizing
);

/*! \brief Sets line coverage for the given expression */
void expression_set_line_coverage(
  expression* exp
);

/*! \brief Assigns a value to an expression's coverage data from a dumpfile */
void expression_vcd_assign(
  expression* expr,
  char        action,
  const char* value
);

/*! \brief Returns TRUE if specified expression is found to contain all static leaf expressions. */
bool expression_is_static_only( expression* expr );

/*! \brief Returns TRUE if specified expression is a part of an bit select expression tree. */
bool expression_is_bit_select( expression* expr );

/*! \brief Returns TRUE if specified expression is in an RASSIGN expression tree. */
bool expression_is_in_rassign( expression* expr );

/*! \brief Returns TRUE if specified expression is the last select of a signal */
bool expression_is_last_select( expression* expr );

/*! \brief Returns the first expression of a memory dimensional select operation. */
expression* expression_get_first_select(
  expression* expr
);

/*! \brief Sets the expression signal supplemental field assigned bit if the given expression is an RHS of an assignment */
void expression_set_assigned( expression* expr );

/*! \brief Sets the left/right changed expression bits for each expression in the tree */
void expression_set_changed( expression* expr );

/*! \brief Deallocates memory used for expression. */
void expression_dealloc( expression* expr, bool exp_only );

#endif

