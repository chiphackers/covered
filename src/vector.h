#ifndef __VECTOR_H__
#define __VECTOR_H__

/*!
 \file     vector.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains vector-related functions for signal vectors.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Initializes specified vector. */
void vector_init( vector* vec, nibble* value, int width );

/*! \brief Creates and initializes new vector */
vector* vector_create( int width, bool data );

/*! \brief Copies contents of from_vec to to_vec, allocating memory */
void vector_copy( vector* from_vec, vector** to_vec );

/*! \brief Displays vector information to specified database file. */
void vector_db_write( vector* vec, FILE* file, bool write_data );

/*! \brief Creates and parses current file line for vector information */
bool vector_db_read( vector** vec, char** line );

/*! \brief Reads and merges two vectors, placing the result into base vector. */
bool vector_db_merge( vector* base, char** line, bool same );

/*! \brief Reads and replaces original vector with newly read vector. */
bool vector_db_replace( vector* base, char** line );

/*! \brief Outputs the toggle01 information from the specified nibble to the specified output stream. */
void vector_display_toggle01( nibble* nib, int width, FILE* ofile );

/*! \brief Outputs the toggle10 information from the specified nibble to the specified output stream. */
void vector_display_toggle10( nibble* nib, int width, FILE* ofile );

/*! \brief Outputs nibble to standard output. */
void vector_display_nibble( nibble* nib, int width );

/*! \brief Outputs vector contents to standard output. */
void vector_display( vector* vec );

/*! \brief Selects bit from value array from bit position pos. */
nibble vector_bit_val( nibble* value, int pos );

/*! \brief Sets specified bit to specified value in given nibble. */
void vector_set_bit( nibble* nib, nibble value, int pos );

/*! \brief Sets specified vector value to new value and maintains coverage history. */
void vector_set_value( vector* vec, nibble* value, int width, int from_idx, int to_idx );

/*! \brief Sets vector output type (DECIMAL, BINARY, OCTAL or HEXIDECIMAL) in first nibble */
void vector_set_type( vector* vec, int type );

/*! \brief Returns value of vector output type. */
int vector_get_type( vector* vec );

/*! \brief Specifies if vector contains unknown values (X or Z) */
bool vector_is_unknown( vector* vec );

/*! \brief Converts vector into integer value. */
int vector_to_int( vector* vec );

/*! \brief Converts integer into vector value. */
void vector_from_int( vector* vec, int value );

/*! \brief Converts vector into a string value in specified format. */
char* vector_to_string( vector* vec, int type );

/*! \brief Converts character string value into vector. */
vector* vector_from_string( char** str );

/*! \brief Assigns specified VCD value to specified vector. */
void vector_vcd_assign( vector* vec, char* value, int msb, int lsb );

/*! \brief Counts toggle01 and toggle10 information from specifed vector. */
void vector_toggle_count( vector* vec, int* tog01_cnt, int* tog10_cnt );

/*! \brief Counts FALSE and TRUE information from the specified vector. */
void vector_logic_count( vector* vec, int* false_cnt, int* true_cnt );

/*! \brief Performs bitwise operation on two source vectors from specified operation table. */
void vector_bitwise_op( vector* tgt, vector* src1, vector* src2, nibble* optab );

/*! \brief Performs bitwise comparison of two vectors. */
void vector_op_compare( vector* tgt, vector* left, vector* right, int comp_type );

/*! \brief Performs left shift operation on left expression by right expression bits. */
void vector_op_lshift( vector* tgt, vector* left, vector* right );
 
/*! \brief Performs right shift operation on left expression by right expression bits. */
void vector_op_rshift( vector* tgt, vector* left, vector* right );

/*! \brief Performs addition operation on left and right expression values. */
void vector_op_add( vector* tgt, vector* left, vector* right );

/*! \brief Performs subtraction operation on left and right expression values. */
void vector_op_subtract( vector* tgt, vector* left, vector* right );

/*! \brief Performs multiplication operation on left and right expression values. */
void vector_op_multiply( vector* tgt, vector* left, vector* right );

/*! \brief Performs unary bitwise inversion operation on specified vector value. */
void vector_unary_inv( vector* tgt, vector* src );

/*! \brief Performs unary operation on specified vector value. */
void vector_unary_op( vector* tgt, vector* src, nibble* optab );

/*! \brief Performs unary logical NOT operation on specified vector value. */
void vector_unary_not( vector* tgt, vector* src );

/*! \brief Deallocates all memory allocated for vector */
void vector_dealloc( vector* vec );


/*
 $Log$
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

