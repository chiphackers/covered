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


//! Initializes specified vector.
void vector_init( vector* vec, nibble* value, int width, int lsb );

//! Creates and initializes new vector
vector* vector_create( int width, int lsb );

//! Merges two vectors and places result into base vector.
void vector_merge( vector* base, vector* in );

//! Displays vector information to specified database file.
void vector_db_write( vector* vec, FILE* file, bool write_data );

//! Creates and parses current file line for vector information */
bool vector_db_read( vector** vec, char** line );

//! Outputs the toggle01 information from the specified nibble to the specified output stream.
void vector_display_toggle01( nibble* nib, int width, int lsb, FILE* ofile );

//! Outputs the toggle10 information from the specified nibble to the specified output stream.
void vector_display_toggle10( nibble* nib, int width, int lsb, FILE* ofile );

//! Outputs nibble to standard output.
void vector_display_nibble( nibble* nib, int width, int lsb );

//! Outputs vector contents to standard output.
void vector_display( vector* vec );

//! Sets specified bit to specified value in given nibble.
void vector_set_bit( nibble* nib, nibble value, int pos );

//! Sets specified vector value to new value and maintains coverage history.
void vector_set_value( vector* vec, nibble* value, int width, int from_idx, int to_idx );

//! Sets vector output type (DECIMAL, BINARY, OCTAL or HEXIDECIMAL) in first nibble
void vector_set_type( vector* vec, int type );

//! Returns value of vector output type.
int vector_get_type( vector* vec );

//! Converts vector into integer value.
int vector_to_int( vector* vec );

//! Converts integer into vector value.
void vector_from_int( vector* vec, int value );

//! Converts vector into a string value in specified format.
char* vector_to_string( vector* vec, int type );

//! Converts character string value into vector.
vector* vector_from_string( char* str, bool sized, int type );

//! Counts toggle01 and toggle10 information from specifed vector.
void vector_toggle_count( vector* vec, int* tog01_cnt, int* tog10_cnt );

//! Counts FALSE and TRUE information from the specified vector.
void vector_logic_count( vector* vec, int* false_cnt, int* true_cnt );

//! Performs bitwise operation on two source vectors from specified operation table.
void vector_bitwise_op( vector* tgt, vector* src1, vector* src2, int* optab );

//! Performs bitwise comparison of two vectors.
void vector_op_compare( vector* tgt, vector* left, vector* right, int comp_type );

//! Performs left shift operation on left expression by right expression bits.
void vector_op_lshift( vector* tgt, vector* left, vector* right );
 
//! Performs right shift operation on left expression by right expression bits.
void vector_op_rshift( vector* tgt, vector* left, vector* right );

//! Performs addition operation on left and right expression values.
void vector_op_add( vector* tgt, vector* left, vector* right );

//! Performs subtraction operation on left and right expression values.
void vector_op_subtract( vector* tgt, vector* left, vector* right );

//! Performs multiplication operation on left and right expression values.
void vector_op_multiply( vector* tgt, vector* left, vector* right );

//! Performs unary bitwise inversion operation on specified vector value.
void vector_unary_inv( vector* tgt, vector* src );

//! Performs unary operation on specified vector value.
void vector_unary_op( vector* tgt, vector* src, nibble* optab );

//! Performs unary logical NOT operation on specified vector value.
void vector_unary_not( vector* tgt, vector* src );

//! Deallocates all memory allocated for vector
void vector_dealloc( vector* vec );

/* $Log$
/* Revision 1.5  2002/07/10 04:57:07  phase1geo
/* Adding bits to vector nibble to allow us to specify what type of input
/* static value was read in so that the output value may be displayed in
/* the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
/* passes.
/*
/* Revision 1.4  2002/07/03 03:31:11  phase1geo
/* Adding RCS Log strings in files that were missing them so that file version
/* information is contained in every source and header file.  Reordering src
/* Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
/* */

#endif

