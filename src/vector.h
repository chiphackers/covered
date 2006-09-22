#ifndef __VECTOR_H__
#define __VECTOR_H__

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
 \file     vector.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains vector-related functions for signal vectors.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Initializes specified vector. */
void vector_init( vector* vec, vec_data* value, int width, int type );

/*! \brief Creates and initializes new vector */
vector* vector_create( int width, int type, bool data );

/*! \brief Copies contents of from_vec to to_vec, allocating memory */
void vector_copy( vector* from_vec, vector** to_vec );

/*! \brief Displays vector information to specified database file. */
void vector_db_write( vector* vec, FILE* file, bool write_data );

/*! \brief Creates and parses current file line for vector information */
bool vector_db_read( vector** vec, char** line );

/*! \brief Reads and merges two vectors, placing the result into base vector. */
bool vector_db_merge( vector* base, char** line, bool same );

/*! \brief Returns string containing toggle 0 -> 1 information in binary format */
char* vector_get_toggle01( vec_data* nib, int width );

/*! \brief Returns string containing toggle 1 -> 0 information in binary format */
char* vector_get_toggle10( vec_data* nib, int width );

/*! \brief Outputs the toggle01 information from the specified nibble to the specified output stream. */
void vector_display_toggle01( vec_data* nib, int width, FILE* ofile );

/*! \brief Outputs the toggle10 information from the specified nibble to the specified output stream. */
void vector_display_toggle10( vec_data* nib, int width, FILE* ofile );

/*! \brief Outputs the binary value of the specified nibble array to standard output */
void vector_display_value( vec_data* nib, int width );

/*! \brief Outputs nibble to standard output. */
void vector_display_nibble( vec_data* nib, int width, int type );

/*! \brief Outputs vector contents to standard output. */
void vector_display( vector* vec );

/*! \brief Selects bit from value array from bit position pos. */
nibble vector_bit_val( nibble* value, int pos );

/*! \brief Sets specified vector value to new value and maintains coverage history. */
bool vector_set_value( vector* vec, vec_data* value, int width, int from_idx, int to_idx );

/*! \brief Performs a zero-fill of all bits starting at lsb and continuing to the vector's msb */
bool vector_zero_fill( vector* vec, int msb, int lsb );

/*! \brief Sets vector output type (DECIMAL, BINARY, OCTAL or HEXIDECIMAL) in first nibble */
void vector_set_type( vector* vec, int type );

/*! \brief Returns value of vector output type. */
int vector_get_type( vector* vec );

/*! \brief Specifies if vector contains unknown values (X or Z) */
bool vector_is_unknown( vector* vec );

/*! \brief Returns TRUE if specified vector has been set (simulated) */
bool vector_is_set( vector* vec );

/*! \brief Converts vector into integer value. */
int vector_to_int( vector* vec );

/*! \brief Converts integer into vector value. */
void vector_from_int( vector* vec, int value );

/*! \brief Converts vector into a string value in specified format. */
char* vector_to_string( vector* vec );

/*! \brief Converts character string value into vector. */
vector* vector_from_string( char** str, bool quoted );

/*! \brief Assigns specified VCD value to specified vector. */
bool vector_vcd_assign( vector* vec, char* value, int msb, int lsb );

/*! \brief Counts toggle01 and toggle10 information from specifed vector. */
void vector_toggle_count( vector* vec, int* tog01_cnt, int* tog10_cnt );

/*! \brief Sets all assigned bits in vector bit value array within specified range. */
bool vector_set_assigned( vector* vec, int msb, int lsb );

/*! \brief Performs bitwise operation on two source vectors from specified operation table. */
bool vector_bitwise_op( vector* tgt, vector* src1, vector* src2, nibble* optab );

/*! \brief Performs bitwise comparison of two vectors. */
bool vector_op_compare( vector* tgt, vector* left, vector* right, int comp_type );

/*! \brief Performs left shift operation on left expression by right expression bits. */
bool vector_op_lshift( vector* tgt, vector* left, vector* right );
 
/*! \brief Performs right shift operation on left expression by right expression bits. */
bool vector_op_rshift( vector* tgt, vector* left, vector* right );

/*! \brief Performs arithmetic right shift operation on left expression by right expression bits. */
bool vector_op_arshift( vector* tgt, vector* left, vector* right );

/*! \brief Performs addition operation on left and right expression values. */
bool vector_op_add( vector* tgt, vector* left, vector* right );

/*! \brief Performs a twos complement of the src vector and stores the new vector in tgt. */
bool vector_op_negate( vector* tgt, vector* src );

/*! \brief Performs subtraction operation on left and right expression values. */
bool vector_op_subtract( vector* tgt, vector* left, vector* right );

/*! \brief Performs multiplication operation on left and right expression values. */
bool vector_op_multiply( vector* tgt, vector* left, vector* right );

/*! \brief Performs increment operation on specified vector. */
bool vector_op_inc( vector* tgt );

/*! \brief Performs increment operation on specified vector. */
bool vector_op_dec( vector* tgt );

/*! \brief Performs unary bitwise inversion operation on specified vector value. */
bool vector_unary_inv( vector* tgt, vector* src );

/*! \brief Performs unary operation on specified vector value. */
bool vector_unary_op( vector* tgt, vector* src, nibble* optab );

/*! \brief Performs unary logical NOT operation on specified vector value. */
bool vector_unary_not( vector* tgt, vector* src );

/*! \brief Deallocates all memory allocated for vector */
void vector_dealloc( vector* vec );


/*
 $Log$
 Revision 1.37  2006/09/20 22:38:10  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.36  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.35  2006/08/20 03:21:00  phase1geo
 Adding support for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, <<<=, >>>=, ++
 and -- operators.  The op-and-assign operators are currently good for
 simulation and code generation purposes but still need work in the comb.c
 file for proper combinational logic underline and reporting support.  The
 increment and decrement operations should be fully supported with the exception
 of their use in FOR loops (I'm not sure if this is supported by SystemVerilog
 or not yet).  Also started adding support for delayed assignments; however, I
 need to rework this completely as it currently causes segfaults.  Added lots of
 new diagnostics to verify this new functionality and updated regression for
 these changes.  Full IV regression now passes.

 Revision 1.34  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.33  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.32  2006/01/10 05:12:48  phase1geo
 Added arithmetic left and right shift operators.  Added ashift1 diagnostic
 to verify their correct operation.

 Revision 1.31  2005/11/21 22:21:58  phase1geo
 More regression updates.  Also made some updates to debugging output.

 Revision 1.30  2005/11/18 05:17:01  phase1geo
 Updating regressions with latest round of changes.  Also added bit-fill capability
 to expression_assign function -- still more changes to come.  We need to fix the
 expression sizing problem for RHS expressions of assignment operators.

 Revision 1.29  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.28  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.27  2005/01/07 23:00:10  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.26  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.25  2005/01/06 23:51:18  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.24  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.23  2004/10/22 22:03:32  phase1geo
 More incremental changes to increase score command efficiency.

 Revision 1.22  2004/10/22 20:31:07  phase1geo
 Returning assignment status in vector_set_value and speeding up assignment procedure.
 This is an incremental change to help speed up design scoring.

 Revision 1.21  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.20  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.19  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.18  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.17  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.16  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.15  2003/01/04 03:56:28  phase1geo
 Fixing bug with parameterized modules.  Updated regression suite for changes.

 Revision 1.14  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.13  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.12  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.11  2002/10/31 23:14:37  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.10  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.9  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.8  2002/08/23 12:55:34  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.7  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.6  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.5  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.4  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

