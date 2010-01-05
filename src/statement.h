#ifndef __STATEMENT_H__
#define __STATEMENT_H__

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
 \file     statement.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     5/1/2002
 \brief    Contains functions to create, manipulate and deallocate statements.
*/

#include "defines.h"


/*! \brief Creates new statement structure. */
statement* statement_create(
  expression*  exp,
  func_unit*   funit
);

/*! \brief Sizes all expressions for the given statement block */
void statement_size_elements(
  statement* stmt,
  func_unit* funit
);

/*! \brief Writes specified statement to the specified output file. */
void statement_db_write(
  statement* stmt,
  FILE*      ofile,
  bool       ids_issued
);

/*! \brief Writes specified statement tree to the specified output file. */
void statement_db_write_tree(
  statement* stmt,
  FILE*      ofile
);

/*! \brief Writes specified expression trees for given statement block to specified output file. */
void statement_db_write_expr_tree(
  statement* stmt,
  FILE*      ofile
);

/*! \brief Reads in statement line from specified string and stores statement in specified functional unit. */
void statement_db_read(
             char**     line,
  /*@null@*/ func_unit* curr_funit,
             int        read_mode
);

/*! \brief Assigns unique expression IDs to each expression in the given statement block. */
void statement_assign_expr_ids(
  statement* stmt,
  func_unit* funit
);

/*! \brief Connects statement sequence to next statement and sets stop bit. */
bool statement_connect(
  statement* curr_stmt,
  statement* next_stmt,
  int        conn_id
);

/*! \brief Calculates the last line of the specified statement tree. */
int statement_get_last_line(
  statement* stmt
);

/*! \brief Creates a list of all signals on the RHS of expressions in the given statement block */
void statement_find_rhs_sigs(
  statement* stmt,
  str_link** head,
  str_link** tail
);

/*! \brief Searches for statement with ID in the given statement block */
statement* statement_find_statement(
  statement* curr,
  int        id
);

/*! \brief Searches for statement with given positional information */
statement* statement_find_statement_by_position(
  statement*   curr,
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Searches the specified statement block for expression that calls the given stmt */
bool statement_contains_expr_calling_stmt(
  statement* curr,
  statement* stmt
);

/*! \brief Recursively traverses the entire statement block and adds the given statements to the specified statement list */
void statement_add_to_stmt_link(
  statement*  stmt,
  stmt_link** head,
  stmt_link** tail
);

/*! \brief Recursively deallocates specified statement tree. */
void statement_dealloc_recursive(
  statement* stmt,
  bool       rm_stmt_blks
);

/*! \brief Deallocates statement memory and associated expression tree from the heap. */
void statement_dealloc(
  statement* stmt
);

#endif

