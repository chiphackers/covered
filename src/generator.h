#ifndef __GENERATOR_H__
#define __GENERATOR_H__

/*
 Copyright (c) 2006-2008 Trevor Williams

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


/*! Shortcut for the generator_flush_hold_code1 function call */
#define generator_flush_hold_code generator_flush_hold_code1( __FILE__, __LINE__ )

/*! Shortcut for the generator_flush_work_code1 function call */
#define generator_flush_work_code generator_flush_work_code1( __FILE__, __LINE__ )

/*! Shortcut for the generator_flush_all1 function call */
#define generator_flush_all generator_flush_all1( __FILE__, __LINE__ )

/*! Shortcut for the generator_flush_event_combs1 function call */
#define generator_flush_event_combs generator_flush_event_combs1( __FILE__, __LINE__ )


/*! \brief Outputs the current state of the code generator to standard output for debugging purposes. */
void generator_display();

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
  const char*  str
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

/*! \brief Outputs all of the event code to the output file. */
void generator_flush_event_combs1(
  const char*  file,
  unsigned int line
);

/*! \brief Flushes the working and holding code buffers. */
void generator_flush_all1(
  const char*  file,
  unsigned int line
);

/*! \brief Inserts line coverage information. */
void generator_insert_line_cov(
  unsigned int first_line,
  unsigned int first_column,
  unsigned int last_column,
  bool         semicolon
);

/*! \brief Inserts combinational logic coverage information. */
void generator_insert_comb_cov(
  bool         net,
  bool         use_right,
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Inserts code for handling combinational logic coverage for case blocks.
void generator_insert_case_comb_cov(
  unsigned int first_line,
  unsigned int first_column
);


/*
 $Log$
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

