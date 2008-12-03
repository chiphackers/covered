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


/*! \brief Generates Verilog containing coverage instrumentation */
void generator_output();

/*! \brief Adds the given string to the work code buffers */
void generator_add_to_work_code(
  const char*  str
);

/*! \brief Flushes the working code to the hold code */
void generator_flush_work_code();

/*! \brief Adds the given string to the hold code buffers */
void generator_add_to_hold_code(
  const char*  str
);

/*! \brief Outputs all held code to the output file. */
void generator_flush_hold_code();

/*! \brief Inserts line coverage information. */
void generator_insert_line_cov(
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Inserts combinational logic coverage information. */
void generator_insert_comb_cov(
  unsigned int first_line,   /*!< First line of expression to generate for */
  unsigned int first_column  /*!< First column of expression to generate for */
);


/*
 $Log$
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

