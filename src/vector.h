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

//! Outputs nibble to standard output.
void vector_display_nibble( nibble* nib, int width, int lsb );

//! Outputs vector contents to standard output.
void vector_display( vector* vec );

//! Sets specified bit to specified value in given nibble.
void vector_set_bit( nibble* nib, nibble value, int pos );

//! Sets specified vector value to new value and maintains coverage history.
bool vector_set_value( vector* vec, nibble* value, int width, int from_lsb, int to_lsb );

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


/*
#define EXP_OP_LT       0x0b
#define EXP_OP_GT       0x0c 
#define EXP_OP_LSHIFT   0x0d
#define EXP_OP_RSHIFT   0x0e
#define EXP_OP_EQ       0x0f
#define EXP_OP_CEQ      0x10
#define EXP_OP_LE       0x11
#define EXP_OP_GE       0x12
#define EXP_OP_NE       0x13
#define EXP_OP_CNE      0x14
#define EXP_OP_LOR      0x15
#define EXP_OP_LAND     0x16
#define EXP_OP_COND     0x17
#define EXP_OP_SBIT_SEL 0x20
#define EXP_OP_MBIT_SEL 0x21
#define EXP_OP_EXPAND   0x22
#define EXP_OP_CONCAT   0x23
#define EXP_OP_PEDGE    0x24
#define EXP_OP_NEDGE    0x25
#define EXP_OP_AEDGE    0x26
*/


//! Deallocates all memory allocated for vector
void vector_dealloc( vector* vec );

#endif

