#ifndef __DB_H__
#define __DB_H__

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
 \file     db.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/7/2001
 \brief    Contains functions for writing and reading contents of
           covered database file.
*/

#include "defines.h"


/*! \brief Creates a new database. */
db* db_create();

/*! \brief Deallocates all memory consumed by the database. */
void db_close();

/*! \brief Checks to see if the module specified by the -t option is the top-level module of the simulator. */
bool db_check_for_top_module();

/*! \brief Writes contents of expressions, functional units and vsignals to database file. */
void db_write(
  const char* file,
  bool        parse_mode,
  bool        issue_ids
);

/*! \brief Reads contents of database file and stores into internal lists. */
bool db_read(
  const char* file,
  int         read_mode
);

/*! \brief Assigns instance/functional unit IDs to all non-generated instances. */
void db_assign_ids();

/*! \brief Merges the current instance trees */
void db_merge_instance_trees();

/*! \brief After functional units have been read, merge the contents of the functional units (used in GUI only). */
void db_merge_funits();

/*! \brief Returns a scaled version of the given value to the timescale for the given functional unit. */
uint64 db_scale_to_precision(
  uint64     value,
  func_unit* funit
);

/*! \brief Sets the global timescale unit and precision variables. */
void db_set_timescale(
  int unit,
  int precision
);

/*! \brief Searches for and sets the current functional unit. */
void db_find_and_set_curr_funit(
  const char* name,
  int         type
);

/*! \brief Returns a pointer to the current functional unit. */
func_unit* db_get_curr_funit();

/*! \brief Returns a pointer to the functional unit that begins at the specified file position. */
func_unit* db_get_tfn_by_position(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Calculates and returns the size of the exclusion ID string */
unsigned int db_get_exclusion_id_size();

/*! \brief Allocates memory for and generates the exclusion identifier. */
char* db_gen_exclusion_id(
  char type,
  int  id
);

/*! \brief Creates a scope name for an unnamed scope.  Called only during parsing. */
char* db_create_unnamed_scope();

/*! \brief Returns TRUE if the given scope is an unnamed scope name; otherwise, returns FALSE. */
bool db_is_unnamed_scope(
  char* scope
);

/*! \brief Adds the given filename and version information to the database. */
void db_add_file_version(
  const char* file,
  const char* version
);

/*! \brief Outputs all needed signals in $dumpvars calls to the specified file. */
void db_output_dumpvars(
  FILE* vfile
);

/*! \brief Adds specified functional unit node to functional unit tree.  Called by parser. */
func_unit* db_add_instance(
  char*         scope,
  char*         name,
  int           type,
  unsigned int  ppfline,
  int           fcol,
  vector_width* range
);

/*! \brief Adds specified module to module list.  Called by parser. */
void db_add_module(
  char*        name,
  char*        orig_fname,
  char*        incl_fname,
  unsigned int start_line,
  unsigned int start_col
);

/*! \brief Adds specified task/function to functional unit list.  Called by parser. */
bool db_add_function_task_namedblock(
  int   type,
  char* name,
  char* orig_fname,
  char* incl_fname,
  int   start_line,
  int   start_column
);

/*! \brief Performs actions necessary when the end of a function/task/named-block is seen.  Called by parser. */
void db_end_function_task_namedblock(
  int end_line
);

/*! \brief Adds specified declared parameter to parameter list.  Called by parser. */
void db_add_declared_param(
  bool         is_signed,
  static_expr* msb,
  static_expr* lsb,
  char*        name,
  expression*  expr,
  bool         local
);

/*! \brief Adds specified override parameter to parameter list.  Called by parser. */
void db_add_override_param(
  char*       inst_name,
  expression* expr,
  char*       param_name
);

/*! \brief Adds specified defparam to parameter override list.  Called by parser. */
void db_add_defparam(
  char*       name,
  expression* expr
);

/*! \brief Adds specified vsignal to vsignal list.  Called by parser. */
void db_add_signal(
  char*      name,
  int        type,
  sig_range* prange,
  sig_range* urange,
  bool       is_signed,
  bool       mba,
  int        line,
  int        col,
  bool       handled
);

/*! \brief Creates statement block that acts like a fork join block from a standard statement block */
statement* db_add_fork_join(
  statement* stmt
);

/*! \brief Creates an enumerated list based on the given parameters */
void db_add_enum(
  vsignal*     enum_sig,
  static_expr* value
);

/*! \brief Called after all enumerated values for the current list have been added */
void db_end_enum_list();

/*! \brief Adds given typedefs to the database */
void db_add_typedef(
  const char* name,
  bool        is_signed,
  bool        is_handled,
  bool        is_sizable,
  sig_range*  prange,
  sig_range*  urange
);

/*! \brief Called when the endmodule keyword is parsed. */
void db_end_module(
  int end_line
);

/*! \brief Called when the endfunction or endtask keyword is parsed. */
void db_end_function_task(
  int end_line
);

/*! \brief Finds specified signal in functional unit and returns pointer to the signal structure.  Called by parser. */
vsignal* db_find_signal(
  char* name,
  bool  okay_if_not_found
);

/*! \brief Adds a generate block to the database.  Called by parser. */
void db_add_gen_item_block(
  gen_item* gi
);

/*! \brief Find specified generate item in the current functional unit.  Called by parser. */
gen_item* db_find_gen_item(
  gen_item* root,
  gen_item* gi
);

/*! \brief Finds specified typedef and returns TRUE if it is found */
typedef_item* db_find_typedef(
  const char* name
);

/*! \brief Returns a pointer to the current implicitly connected generate block.  Called by parser. */
gen_item* db_get_curr_gen_block();

/*! \brief Creates new expression from specified information.  Called by parser and db_add_expression. */
expression* db_create_expression(
  expression*  right,
  expression*  left,
  exp_op_type  op,
  bool         lhs,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  int          first,
  int          last,
  char*        sig_name,
  bool         in_static
);

/*! \brief Binds all necessary sub-expressions in the given tree to the given signal name */
void db_bind_expr_tree(
  expression* root,
  char*       sig_name
);

/*! \brief Creates an expression from the specified static expression */
expression* db_create_expr_from_static(
  static_expr* se,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  int          first_col,
  int          last_col
);

/*! \brief Adds specified expression to expression list.  Called by parser. */
void db_add_expression(
  expression* root
);

/*! \brief Creates an expression tree sensitivity list for the given statement block */
expression* db_create_sensitivity_list(
  statement* stmt
);

/*! \brief Checks specified statement for parallelization and if it must be, creates a parallel statement block */
statement* db_parallelize_statement(
  statement* stmt
);

/*! \brief Creates new statement expression from specified information.  Called by parser. */
statement* db_create_statement(
  expression* exp
);

/*! \brief Adds specified statement to current functional unit's statement list.  Called by parser. */
void db_add_statement(
  statement* stmt,
  statement* start
);

/*! \brief Removes specified statement from current functional unit. */
void db_remove_statement_from_current_funit(
  statement* stmt
);

/*! \brief Removes specified statement and associated expression from list and memory. */
void db_remove_statement(
  statement* stmt
);

/*! \brief Connects gi2 to the true path of gi1 */
void db_gen_item_connect_true(
  gen_item* gi1,
  gen_item* gi2
);

/*! \brief Connects gi2 to the false path of gi1 */
void db_gen_item_connect_false(
  gen_item* gi1,
  gen_item* gi2
);

/*! \brief Connects one generate item block to another. */
void db_gen_item_connect(
  gen_item* gi1,
  gen_item* gi2
);

/*! \brief Connects one statement block to another. */
bool db_statement_connect(
  statement* curr_stmt,
  statement* next_stmt
);

/*! \brief Connects true statement to specified statement. */
void db_connect_statement_true(
  statement* stmt,
  statement* exp_true
);

/*! \brief Connects false statement to specified statement. */
void db_connect_statement_false(
  statement* stmt,
  statement* exp_false
);

/*! \brief Allocates and initializes an attribute parameter. */
attr_param* db_create_attr_param(
  char*       name,
  expression* expr
);

/*! \brief Parses the specified attribute parameter list for Covered attributes */
void db_parse_attribute(
  attr_param* ap
);

/*! \brief Searches entire design for expressions that call the specified statement */
void db_remove_stmt_blks_calling_statement(
  statement* stmt
);

/*! \brief Synchronizes the curr_instance pointer to match the curr_inst_scope hierarchy */
void db_sync_curr_instance();

/*! \brief Sets current VCD scope to specified scope. */
void db_set_vcd_scope(
  const char* scope
);

/*! \brief Moves current VCD hierarchy up one level */
void db_vcd_upscope();

/*! \brief Adds symbol to signal specified by name. */
void db_assign_symbol(
  char*       name,
  const char* symbol,
  int         msb,
  int         lsb
);

/*! \brief Sets the found symbol value to specified character value.  Called by VCD lexer. */
void db_set_symbol_char(
  const char* sym,
  char        value
);

/*! \brief Sets the found symbol value to specified string value.  Called by VCD lexer. */
void db_set_symbol_string(
  const char* sym,
  const char* value
);

/*! \brief Performs a timestep for all signal changes during this timestep. */
bool db_do_timestep(
  uint64 time,
  bool   final
); 

/*! \brief Called after all signals are parsed from dumpfile.  Checks to see if dumpfile results were
           correct for the covered design. */
void db_check_dumpfile_scopes();

/*! \brief Called before simulation begins to open the CDD file for coverage. */
bool db_verilator_initialize(
  const char* cdd_name
);

/*! \brief Called after simulation has completed to write the database contents to the CDD. */
bool db_verilator_close(
  const char* cdd_name
);

/*!
 Gathers line coverage for the specified expression.
*/
bool db_add_line_coverage(
  uint32 inst_index,
  uint32 expr_index
);

#endif

