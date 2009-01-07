#ifndef __GENERATOR_H__
#define __GENERATOR_H__

/*
 Copyright (c) 2006-2009 Trevor Williams

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

/*! Shortcut for the generator_add_to_work_code function for code coming from Covered */
#define generator_add_cov_to_work_code(x) generator_add_to_work_code(x, 0, 0, FALSE)

/*! Shortcut for the generator_add_to_work_code function for code coming from original file */
#define generator_add_orig_to_work_code(x, y, z) generator_add_to_work_code(x, y, z, TRUE)


/*! \brief Outputs the current state of the code generator to standard output for debugging purposes. */
void generator_display();

/*! \brief Replaces original code with the specified code string. */
void generator_replace(
  const char*  str,
  unsigned int first_line,
  unsigned int first_column,
  unsigned int last_line,
  unsigned int last_column
);

/*! \brief Returns TRUE if the current expression will be calculated via an intermediate assignment. */
bool generator_expr_name_needed(
  expression*  exp,   /*!< Pointer to expression to evaluate */
  unsigned int depth  /*!< Expression depth of the given expression */
);

/*! \brief Creates an inlined expression name (guanteed to be unique for a given expression) */
char* generator_create_expr_name(
  expression* exp
);

/*! \brief Generates Verilog containing coverage instrumentation */
void generator_output();

/*! \brief Initializes and resets the functional unit iterator. */
void generator_init_funit(
  func_unit* funit  /*!< Pointer to current functional unit */
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
  bool         from_code
);

/*! \brief Flushes the working code to the hold code */
void generator_flush_work_code1(
  const char*  file,
  unsigned int line
);

/*! \brief Adds the given string to the hold code buffers */
void generator_add_to_hold_code(
  const char* str
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

/*! \brief Inserts line coverage for the given statement. */
void generator_insert_line_cov_with_stmt(
  statement* stmt,
  bool       semicolon
);

/*! \brief Inserts line coverage information. */
statement* generator_insert_line_cov(
  unsigned int first_line,
  unsigned int last_line,
  unsigned int first_column,
  unsigned int last_column,
  bool         semicolon
);

/*! \brief Inserts event combinational coverage for a specific expression. */
void generator_insert_event_comb_cov(
  expression* exp,
  func_unit*  funit,
  bool        reg_needed
);

/*! \brief Inserts combinational logic coverage information. */
statement* generator_insert_comb_cov(
  unsigned int first_line,
  unsigned int first_column,
  bool         net,
  bool         use_right,
  bool         save_stmt
);

/*! \brief Inserts combinational logic coverage information from the current top of the statement stack. */
statement* generator_insert_comb_cov_from_stmt_stack();

/*! \brief Inserts combinational logic coverage for the given statement. */
void generator_insert_comb_cov_with_stmt(
  statement* stmt,
  bool       use_right,
  bool       reg_needed
);

/*! \brief Inserts code for handling combinational logic coverage for case blocks. */
void generator_insert_case_comb_cov(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Inserts FSM coverage code into module */
void generator_insert_fsm_cov();

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


/*
 $Log$
 Revision 1.25  2009/01/06 23:11:00  phase1geo
 Completed and debugged new generator_replace functionality.  Fixed issue with
 event coverage handling.  Added new event2 diagnostic to verify the event
 coverage handling.  Checkpointing.

 Revision 1.24  2009/01/06 14:35:18  phase1geo
 Starting work on generator_replace functionality.  Not quite complete yet
 but I need to checkpoint.

 Revision 1.23  2009/01/06 06:59:22  phase1geo
 Adding initial support for string replacement.  More work to do here.
 Checkpointing.

 Revision 1.22  2009/01/04 20:11:19  phase1geo
 Completed initial work on event handling.

 Revision 1.21  2009/01/02 06:00:26  phase1geo
 More updates for memory coverage (this is still not working however).  Currently
 segfaults.  Checkpointing.

 Revision 1.20  2008/12/28 19:39:17  phase1geo
 Fixing the handling of wait statements.  Updated regressions as necessary.
 Checkpointing.

 Revision 1.19  2008/12/27 21:05:55  phase1geo
 Updating CDD version and regressions per this change.  Checkpointing.

 Revision 1.18  2008/12/27 04:47:47  phase1geo
 Updating regressions.  Added code to support while loops; however, the new code does
 not support FOR loops as I was hoping so I might end up reverting these changes somewhat
 in the end.  Checkpointing.

 Revision 1.17  2008/12/24 21:19:01  phase1geo
 Initial work at getting FSM coverage put in (this looks to be working correctly
 to this point).  Updated regressions per fixes.  Checkpointing.

 Revision 1.16  2008/12/19 00:04:38  phase1geo
 VCS regression updates.  Checkpointing.

 Revision 1.15  2008/12/17 00:02:57  phase1geo
 More work on inlined coverage code.  Making good progress through the regression
 suite.  Checkpointing.

 Revision 1.14  2008/12/16 00:18:08  phase1geo
 Checkpointing work on for2 diagnostic.  Adding initial support for fork..join
 blocks -- more work to do here.  Starting to add support for FOR loop control
 logic although I'm thinking that I may want to pull this coverage support from
 the current coverage handling structures.

 Revision 1.13  2008/12/14 06:56:02  phase1geo
 Making some code modifications to set the stage for supporting case statements
 with the new inlined code coverage methodology.  Updating regressions per this
 change (IV and Cver fully pass).

 Revision 1.12  2008/12/12 05:57:50  phase1geo
 Checkpointing work on code coverage injection.  We are making decent progress in
 getting regressions back to a working state.  Lots to go, but good progress at
 this point.

 Revision 1.11  2008/12/12 00:17:30  phase1geo
 Fixing some bugs, creating some new ones...  Checkpointing.

 Revision 1.10  2008/12/10 23:37:02  phase1geo
 Working on handling event combinational logic cases.  This does not fully work
 at this point.  Fixed issues with combinational logic generation for IF statements.
 Checkpointing.

 Revision 1.9  2008/12/10 00:19:23  phase1geo
 Fixing issues with aedge1 diagnostic (still need to handle events but this will
 be worked on a later time).  Working on sizing temporary subexpression LHS signals.
 This is not complete and does not compile at this time.  Checkpointing.

 Revision 1.8  2008/12/05 00:22:41  phase1geo
 More work completed on code coverage generator.  Currently working on bug in
 statement finder.  Checkpointing.

 Revision 1.7  2008/12/04 14:19:50  phase1geo
 Fixing bug in code generator.  Checkpointing.

 Revision 1.6  2008/12/04 07:14:39  phase1geo
 Doing more work on the code generator to handle combination logic output.
 Still more coding and debugging work to do here.  Need to clear the added
 bit in the statement lists to get current code working correctly.  Checkpointing.

 Revision 1.5  2008/12/03 23:29:07  phase1geo
 Finished getting line coverage insertion working.  Starting to work on combinational logic
 coverage.  Checkpointing.

 Revision 1.4  2008/12/03 17:15:11  phase1geo
 Code to output coverage file is now working from an end-to-end perspective.  Checkpointing.
 We are now ready to start injecting actual coverage information into this file.

 Revision 1.3  2008/12/02 23:43:21  phase1geo
 Reimplementing inlined code generation code.  Added this code to Verilog lexer and parser.
 More work to do here.  Checkpointing.

 Revision 1.2  2008/11/26 05:34:48  phase1geo
 More work on Verilog generator file.  We are now able to create the needed
 directories and output a non-instrumented version of a module to the appropriate
 directory.  Checkpointing.

 Revision 1.1  2008/11/25 23:53:07  phase1geo
 Adding files for Verilog generator functions.

*/

#endif

