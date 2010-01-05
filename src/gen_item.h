#ifndef __GEN_ITEM_H__
#define __GEN_ITEM_H__

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
 \file     gen_item.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     7/10/2006
 \brief    Contains functions for handling generate items.
*/


#include <stdio.h>

#include "defines.h"


/*@-exportlocal@*/
/*! \brief Displays the specified generate item to standard output */
void gen_item_display(
  gen_item* gi
);
/*@=exportlocal@*/

/*! \brief Displays an entire generate block */
void gen_item_display_block(
  gen_item* root
);

/*! \brief Searches for a generate item in the generate block of root that matches gi */
gen_item* gen_item_find(
  gen_item* root,
  gen_item* gi
);

/*! \brief Searches for an expression in the generate list that calls the given statement */
void gen_item_remove_if_contains_expr_calling_stmt(
  gen_item*  gi,
  statement* stmt
);

/*! \brief Returns TRUE if the specified variable name contains a generate variable within it */
bool gen_item_varname_contains_genvar(
  char* name
);

/*! \brief Returns the actual signal name specified by the given signal name which references a
           generated hierarchy */
char* gen_item_calc_signal_name(
  const char* name,
  func_unit*  funit,
  int         line,
  bool        no_genvars
);

/*! \brief Creates a generate item for an expression */
gen_item* gen_item_create_expr(
  expression* expr
);

/*! \brief Creates a generate item for a signal */
gen_item* gen_item_create_sig(
  vsignal* sig
);

/*! \brief Creates a generate item for a statement */
gen_item* gen_item_create_stmt(
  statement* stmt
);

/*! \brief Creates a generate item for an instance */
gen_item* gen_item_create_inst(
  funit_inst* inst
);

/*! \brief Creates a generate item for a namespace */
gen_item* gen_item_create_tfn(
  funit_inst* inst
);

/*! \brief Creates a generate item for a binding */
gen_item* gen_item_create_bind(
  const char* name,
  expression* expr
);

/*! \brief Resizes all statements and signals in the given generate item block */
void gen_item_resize_stmts_and_sigs(
  gen_item*  gi,
  func_unit* funit
);

/*! \brief Assigns unique signal ID or expression IDs to all expressions for specified statement block */
void gen_item_assign_ids(
  gen_item*  gi,
  func_unit* funit
);

/*! \brief Outputs the current generate item to the given output file if it matches the type specified */
void gen_item_db_write(
  gen_item* gi,
  uint32    type,
  FILE*     file
);

/*! \brief Outputs the entire expression tree from the given generate statement */
void gen_item_db_write_expr_tree(
  gen_item* gi,
  FILE*     file
);

/*! \brief Connects a generate item block to a new generate item */
bool gen_item_connect(
  gen_item* gi1,
  gen_item* gi2,
  int       conn_id,
  bool      stop_on_null
);

/*! \brief Checks generate item and if it is a bind, adds it to binding pool and returns TRUE */
void gen_item_bind(
  gen_item* gi
);

/*! \brief Resolves all generate items in the given instance */
void generate_resolve_inst(
  funit_inst* inst
);

/*! \brief Removes the given generate statement from the correct instance */
bool generate_remove_stmt(
  statement* stmt
);

/*! \brief Searches the generate statements within the given functional unit for a statement that matches the given positional information. */
statement* generate_find_stmt_by_position(
  func_unit*   funit,
  unsigned int first_line,
  unsigned int first_col
);

/*! \brief Searches the generate tasks/functions/named blocks within the given functional unit for a functional unit that matches the given positional information. */
func_unit* generate_find_tfn_by_position(
  func_unit*   funit,
  unsigned int first_line,
  unsigned int first_col
);

/*! \brief Deallocates all associated memory for the given generate item */
void gen_item_dealloc(
  gen_item* gi,
  bool      rm_elem
);

#endif

