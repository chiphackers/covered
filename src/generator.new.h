#ifndef __GENERATOR_H__
#define __GENERATOR_H__

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
 \file     generator.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/25/2008
 \brief    Contains functions for generating verilog to simulate.
*/


#include "defines.h"

/*! Shortcut for the generator_flush_hold_code1 function call */
#define generator_flush_hold_code generator_flush_hold_code1( __FILE__, __LINE__ )

/*! Shortcut for the generator_flush_work_code1 function call */
#define generator_flush_work_code generator_flush_work_code1( __FILE__, __LINE__ )

/*! Shortcut for the generator_flush_all1 function call */
#define generator_flush_all generator_flush_all1( __FILE__, __LINE__ )

/*! Shortcut for the generator_flush_held_token call */
#define generator_flush_held_token generator_flush_held_token1( __FILE__, __LINE__ )

/*! Shortcut for the generator_add_to_work_code function for code coming from Covered */
#define generator_add_cov_to_work_code(x) generator_add_to_work_code(x, 0, 0, FALSE, __FILE__, __LINE__ )

/*! Shortcut for the generator_add_to_work_code function for code coming from original file */
#define generator_add_orig_to_work_code(x, y, z) generator_add_to_work_code(x, y, z, TRUE, __FILE__, __LINE__ )

/*! Shortcut for the generator_build function */
#define generator_build(x, ...) generator_build1( __FILE__, __LINE__, x, __VA_ARGS__ )


/*! \brief Outputs the current state of the code generator to standard output for debugging purposes. */
void generator_display();

/*! \brief Returns TRUE if the given functional unit is within a static function; otherwise, returns FALSE. */
bool generator_is_static_function(
  func_unit* funit
);

/*! \brief Replaces original code with the specified code string. */
void generator_replace(
  const char*  str,
  unsigned int first_line,
  unsigned int first_column,
  unsigned int last_line,
  unsigned int last_column
);

/*! \brief Allocates and initializes a new structure for the purposes of coverage register insertion. */
void generator_push_reg_insert();

/*! \brief Pops the head of the register insertion stack. */
void generator_pop_reg_insert();

/*! \brief Sets the given functional unit instance ID to the current instance. */
void generator_set_inst_id(
  func_unit* funit
);

/*! \brief Pushes the given functional unit onto the functional unit stack. */
void generator_push_funit(
  func_unit* funit
);

/*! \brief Pops the top of the functional unit stack and deallocates it. */
void generator_pop_funit();

/*! \brief Returns TRUE if the current expression will be calculated via an intermediate assignment. */
bool generator_expr_name_needed(
  expression* exp
);

/*! \brief Creates an inlined expression name (guanteed to be unique for a given expression) */
char* generator_create_expr_name(
  expression* exp
);

/*! \brief Generates Verilog containing coverage instrumentation */
void generator_output(
  const char* output_db
);

/*! \brief Initializes and resets the functional unit iterator. */
void generator_init_funit(
  func_unit* funit
);

/*!
 Prepends the given string to the working code list/buffer.
*/
void generator_prepend_to_work_code(
  const char* str
);

/*! \brief Adds the given string to the work code buffers */
void generator_add_to_work_code(
  const char*  str,
  unsigned int first_line,
  unsigned int first_column,
  bool         from_code,
  const char*  file,
  unsigned int line
);

/*! \brief Flushes the working code to the hold code */
void generator_flush_work_code1(
  const char*  file,
  unsigned int line
);

/*! \brief Adds the given string to the hold code buffers */
void generator_add_to_hold_code(
  const char*  str,
  const char*  file,
  unsigned int line
);

/*! \brief Outputs all held code to the output file. */
void generator_flush_hold_code1(
  const char*  file,
  unsigned int line
);

/*! \brief Flushes the working and holding code buffers. */
void generator_flush_all1(
  const char*  file,
  unsigned int line
);

/*! \brief Searches for the statement with matching line and column information. */
statement* generator_find_statement(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Creates line coverage string for the given statement. */
char* generator_line_cov_with_stmt(
  statement* stmt,
  bool       semicolon
);

/*! \brief Inserts line coverage information. */
char* generator_line_cov(
  unsigned int first_line,
  unsigned int last_line,
  unsigned int first_column,
  unsigned int last_column,
  bool         semicolon
);

/*! \brief Inserts event combinational coverage for a specific expression. */
char* generator_event_comb_cov(
  expression* exp,
  func_unit*  funit,
  bool        reg_needed
);

/*! \brief Inserts combinational logic coverage information. */
char* generator_comb_cov(
  unsigned int first_line,
  unsigned int first_column,
  bool         net,
  bool         use_right,
  bool         save_stmt,
  bool         reg_needed
);

/*! \brief Inserts combinational logic coverage information from the current top of the statement stack. */
char* generator_comb_cov_from_stmt_stack();

/*! \brief Inserts combinational logic coverage for the given statement. */
char* generator_comb_cov_with_stmt(
  statement* stmt,
  bool       use_right,
  bool       reg_needed
);

/*! \brief Inserts code for handling combinational logic coverage for case blocks. */
char* generator_case_comb_cov(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Inserts FSM coverage code into module */
char* generator_fsm_covs();

/*! \brief Changes event type to reg type if we are performing combinational logic coverage. */
void generator_handle_event_type(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Converts an event trigger statement to a register inversion statement. */
void generator_handle_event_trigger( 
  const char*  identifier,
  unsigned int first_line,
  unsigned int first_column,
  unsigned int last_line,
  unsigned int last_column
);

/*! \brief Removes the last token in the work buffer and holds onto it. */
void generator_hold_last_token();

/*! \brief Flushes the held token to the work buffer. */
void generator_flush_held_token1(
  const char* file,
  int         line
);

/*! \brief Generates the COVERED_INST_ID register into the work buffer at the current point. */
char* generator_inst_id_reg(
  func_unit* funit
);

/*! \brief Creates instance ID parameter override string */
char* generator_inst_id_overrides();

/*! \brief Handles statements that might be in a fork..join block */
void generator_begin_parallel_statement(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Ends a statement that might be in a fork..join block */
void generator_end_parallel_statement(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Generates a long string from an arbitrary number of strings */
char* generator_build1(
  const char* file,
  int         line,
  int         args,
  ...
);

/*! \brief Generates a code/coverage structure from the given strings */
str_cov* generator_build2(
  char* cov,
  char* str
);

/*! \brief Deallocates the given code/coverage structure */
void generator_destroy2(
  str_cov*
);

/*! \brief Returns the temporary register list to the calling function and pops it from the stack. */
char* generator_tmp_regs();

/*! \brief Adds a new temporary register string to the stack */
void generator_create_tmp_regs();

/*! \brief Wrapper around the VLerror handler for the generator parser */
void GENerror(
  char* str
);

/*! \brief Outputs the current string to the output file */
void generator_write_to_file(
  char* str
);

#endif

