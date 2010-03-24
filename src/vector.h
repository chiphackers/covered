#ifndef __VECTOR_H__
#define __VECTOR_H__

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
 \file     vector.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/1/2001
 \brief    Contains vector-related functions for signal vectors.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Initializes specified vector. */
void vector_init_ulong(
  /*@out@*/ vector* vec,
            ulong** value,
            ulong   data_l,
            ulong   data_h,
            bool    owns_value,
            int     width,
            int     type
);

/*! \brief Initializes specified vector with realtime information. */
void vector_init_r64(
  /*@out@*/ vector* vec,
            rv64*   value,
            double  data,
            char*   str,
            bool    owns_value,
            int     type
);

/*! \brief Initializes specified vector with shortreal information. */
void vector_init_r32(
  /*@out@*/ vector* vec,
            rv32*   value,
            float   data,
            char*   str,
            bool    owns_value,
            int     type
);

/*! \brief Creates and initializes new vector */
vector* vector_create(
  int  width,
  int  type,
  int  data_type,
  bool data
);

/*! \brief Copies contents of from_vec to to_vec */
void vector_copy(
  const vector* from_vec,
  vector*       to_vec
);

/*!
 Copies the entire contents of a bit range from from_vec to to_vec,
 aligning the stored value starting at bit 0.
*/
void vector_copy_range(
  vector*       to_vec,
  const vector* from_vec,
  int           lsb
);

/*! \brief Copies contents of from_vec to to_vec, allocating memory */
void vector_clone(
            const vector* from_vec,
  /*@out@*/ vector**      to_vec
);

/*! \brief Displays vector information to specified database file. */
void vector_db_write(
  vector* vec,
  FILE*   file,
  bool    write_data,
  bool    net
);

/*! \brief Creates and parses current file line for vector information */
void vector_db_read(
  /*@out@*/ vector** vec,
            char**   line
);

/*! \brief Reads and merges two vectors, placing the result into base vector. */
void vector_db_merge(
  vector* base,
  char**  line,
  bool    same
);

/*! \brief Merges two vectors, placing the result into the base vector. */
void vector_merge(
  vector* base,
  vector* other
);

/*! \brief Returns the value of the eval_a for the given bit index. */
int vector_get_eval_a(
  vector* vec,
  int     index
);

/*! \brief Returns the value of the eval_b for the given bit index. */
int vector_get_eval_b(
  vector* vec,
  int     index
);

/*! \brief Returns the value of the eval_c for the given bit index. */
int vector_get_eval_c(
  vector* vec,
  int     index
);

/*! \brief Returns the value of the eval_d for the given bit index. */
int vector_get_eval_d(
  vector* vec,
  int     index
);

/*! \brief Counts the number of eval_a/b bits set in the given vector */
int vector_get_eval_ab_count( vector* vec );

/*! \brief Counts the number of eval_a/b/c bits set in the given vector */
int vector_get_eval_abc_count( vector* vec );

/*! \brief Counts the number of eval_a/b/c/d bits set in the given vector */
int vector_get_eval_abcd_count( vector* vec );

/*! \brief Returns string containing toggle 0 -> 1 information in binary format */
char* vector_get_toggle01_ulong(
  ulong** value,
  int     width
);

/*! \brief Returns string containing toggle 1 -> 0 information in binary format */
char* vector_get_toggle10_ulong(
  ulong** value,
  int     width
);

/*! \brief Outputs the toggle01 information from the specified nibble to the specified output stream. */
void vector_display_toggle01_ulong(
  ulong** value,
  int     width,
  FILE*   ofile
);

/*! \brief Outputs the toggle10 information from the specified nibble to the specified output stream. */
void vector_display_toggle10_ulong(
  ulong** value,
  int     width,
  FILE*   ofile
);

/*! \brief Outputs the binary value of the specified nibble array to standard output */
void vector_display_value_ulong(
  ulong** value,
  int     width
);

/*@-exportlocal@*/
/*! \brief Outputs ulong vector to standard output. */
void vector_display_ulong(
  ulong**      value,
  unsigned int width,
  unsigned int type
);
/*@=exportlocal@*/

/*! \brief Outputs vector contents to standard output. */
void vector_display(
  const vector* vec
);

/*! \brief Sets specified vector value to new value and maintains coverage history. */
bool vector_set_value_ulong(
  vector*      vec,
  ulong**      value,
  unsigned int width
);

/*! \brief Sets the memory read bit of the given vector. */
void vector_set_mem_rd_ulong(
  vector* vec,
  int     msb,
  int     lsb
);

/*! \brief Sets specified target vector to bit range of source vector. */
bool vector_part_select_pull(
  vector* tgt,
  vector* src,
  int     lsb,
  int     msb,
  bool    set_mem_rd
);

/*! \brief Sets specified target vector to bit range of source vector. */
bool vector_part_select_push(
  vector*       tgt,
  int           tgt_lsb,
  int           tgt_msb,
  const vector* src,
  int           src_lsb,
  int           src_msb,
  bool          sign_extend
);

/*!
 \brief Sets eval_a/b bits according to unary coverage
 \note  We may want to create a separate VTYPE_EXP_UNARY to handle this in vector_set_coverage_and_assign.
*/
void vector_set_unary_evals( vector* vec );

/*!
 \brief Sets eval_a/b/c bits according to AND combinational logic coverage
 \note  We may want to create a separate VTYPE_EXP_AND to handle this in vector_set_coverage_and_assign.
*/
void vector_set_and_comb_evals( 
  vector* tgt, 
  vector* left, 
  vector* right 
);

/*!
 \brief Sets eval_a/b/c bits according to OR combinational logic coverage
 \note  We may want to create a separate VTYPE_EXP_OR to handle this in vector_set_coverage_and_assign.
*/
void vector_set_or_comb_evals( 
  vector* tgt, 
  vector* left, 
  vector* right 
);

/*!
 \brief Sets eval_a/b/c/d bits according to other combinational logic coverage
 \note  We may want to create a separate VTYPE_EXP_OTHER to handle this in vector_set_coverage_and_assign.
*/
void vector_set_other_comb_evals(
  vector* tgt,
  vector* left,
  vector* right
);

/*! \brief Bit fills the given vector with the appropriate value starting at the last bit */
bool vector_sign_extend(
  vector* vec,
  int     last
);

/*! \brief Returns TRUE if specified vector has unknown bits set */
bool vector_is_unknown( const vector* vec );

/*! \brief Returns TRUE if specified vector is a non-zero value (does not check unknown bit) */
bool vector_is_not_zero( const vector* vec );

/*! \brief Sets entire vector value to a value of X */
bool vector_set_to_x( vector* vec );

/*! \brief Converts vector into integer value. */
int vector_to_int( const vector* vec );

/*! \brief Converts vector into a 64-bit unsigned integer value. */
uint64 vector_to_uint64( const vector* vec );

/*! \brief Converts vector into a 64-bit real value. */
real64 vector_to_real64( const vector* vec );

/*! \brief Converts vector into a sim_time structure. */
void vector_to_sim_time(
            const vector* vec,
            uint64        scale,
  /*@out@*/ sim_time*     time
);

/*! \brief Converts integer into vector value. */
bool vector_from_int(
  vector* vec,
  int     value
);

/*! \brief Converts a 64-bit integer into a vector value. */
bool vector_from_uint64(
  vector* vec,
  uint64  value
);

/*! \brief Converts a 64-bit real into a vector value. */
bool vector_from_real64(
  vector* vec,
  real64  value
);

/*! \brief Converts vector into a string value in specified format. */
char* vector_to_string(
  vector*      vec, 
  int          base,
  bool         show_all,
  unsigned int width
);

/*! \brief Converts a string to a preallocated vector. */
void vector_from_string_fixed(
  vector*     vec,
  const char* str
);

/*! \brief Converts character string value into vector. */
void vector_from_string(
            char**   str,
            bool     quoted,
  /*@out@*/ vector** vec,
  /*@out@*/ int*     base
);

/*! \brief Assigns specified VCD value to specified vector. */
bool vector_vcd_assign(
  vector*     vec,
  const char* value,
  int         msb,
  int         lsb
);

/*! \brief Assigns specified VCD value to specified vectors. */
bool vector_vcd_assign2(
               vector* vec1,
               vector* vec2,
               char*   value,
  /*@unused@*/ int     msb,
  /*@unused@*/ int     lsb
);

/*! \brief Counts toggle01 and toggle10 information from specifed vector. */
void vector_toggle_count(
            vector*       vec,
  /*@out@*/ unsigned int* tog01_cnt,
  /*@out@*/ unsigned int* tog10_cnt
);

/*! \brief Counts memory write and read information from specified vector. */
void vector_mem_rw_count(
            vector*       vec,
            int           lsb,
            int           msb,
  /*@out@*/ unsigned int* wr_cnt,
  /*@out@*/ unsigned int* rd_cnt
);

/*! \brief Sets all assigned bits in vector bit value array within specified range. */
bool vector_set_assigned( vector* vec, int msb, int lsb );

/*! \brief Set coverage information for given vector and assigns values from scratch arrays to vector. */
bool vector_set_coverage_and_assign_ulong(
  vector*      vec,
  const ulong* scratchl,
  const ulong* scratchh,
  int          lsb,
  int          msb
);

/*! \brief Performs bitwise AND operation on two source vectors. */
bool vector_bitwise_and_op( vector* tgt, vector* src1, vector* src2 );

/*! \brief Performs bitwise NAND operation on two source vectors. */
bool vector_bitwise_nand_op( vector* tgt, vector* src1, vector* src2 );

/*! \brief Performs bitwise OR operation on two source vectors. */
bool vector_bitwise_or_op( vector* tgt, vector* src1, vector* src2 );

/*! \brief Performs bitwise NOR operation on two source vectors. */
bool vector_bitwise_nor_op( vector* tgt, vector* src1, vector* src2 );

/*! \brief Performs bitwise XOR operation on two source vectors. */
bool vector_bitwise_xor_op( vector* tgt, vector* src1, vector* src2 );

/*! \brief Performs bitwise NXOR operation on two source vectors. */
bool vector_bitwise_nxor_op( vector* tgt, vector* src1, vector* src2 );

/*! \brief Performs less-than comparison of two vectors. */
bool vector_op_lt(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs less-than-or-equal comparison of two vectors. */
bool vector_op_le(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs greater-than comparison of two vectors. */
bool vector_op_gt(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs greater-than-or-equal comparison of two vectors. */
bool vector_op_ge(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs equal comparison of two vectors. */
bool vector_op_eq(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs case equal comparison of two vectors and returns result. */
bool vector_ceq_ulong(
  const vector* left,
  const vector* right
);

/*! \brief Performs case equal comparison of two vectors. */
bool vector_op_ceq(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs casex equal comparison of two vectors. */
bool vector_op_cxeq(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs casez equal comparison of two vectors. */
bool vector_op_czeq(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs not-equal comparison of two vectors. */
bool vector_op_ne(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs case not-equal comparison of two vectors. */
bool vector_op_cne(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs logical-OR operation of two vectors. */
bool vector_op_lor(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs logical-AND operation of two vectors. */
bool vector_op_land(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs left shift operation on left expression by right expression bits. */
bool vector_op_lshift(
  vector*       tgt,
  const vector* left,
  const vector* right
);
 
/*! \brief Performs right shift operation on left expression by right expression bits. */
bool vector_op_rshift(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs arithmetic right shift operation on left expression by right expression bits. */
bool vector_op_arshift(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs addition operation on left and right expression values. */
bool vector_op_add(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs a twos complement of the src vector and stores the new vector in tgt. */
bool vector_op_negate(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs subtraction operation on left and right expression values. */
bool vector_op_subtract(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs multiplication operation on left and right expression values. */
bool vector_op_multiply(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs division operation on left and right vector values. */
bool vector_op_divide(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs modulus operation on left and right vector values. */
bool vector_op_modulus(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs increment operation on specified vector. */
bool vector_op_inc(
  vector* tgt,
  vecblk* tvb
);

/*! \brief Performs increment operation on specified vector. */
bool vector_op_dec(
  vector* tgt,
  vecblk* tvb
);

/*! \brief Performs unary bitwise inversion operation on specified vector value. */
bool vector_unary_inv(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary AND operation on specified vector value. */
bool vector_unary_and(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary NAND operation on specified vector value. */
bool vector_unary_nand(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary OR operation on specified vector value. */
bool vector_unary_or(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary NOR operation on specified vector value. */
bool vector_unary_nor(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary XOR operation on specified vector value. */
bool vector_unary_xor(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary NXOR operation on specified vector value. */
bool vector_unary_nxor(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs unary logical NOT operation on specified vector value. */
bool vector_unary_not(
  vector*       tgt,
  const vector* src
);

/*! \brief Performs expansion operation. */
bool vector_op_expand(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs list operation. */
bool vector_op_list(
  vector*       tgt,
  const vector* left,
  const vector* right
);

/*! \brief Performs clog2 operation. */
bool vector_op_clog2(
  vector*       tgt,
  const vector* src
);

/*! \brief Deallocates the value structure for the given vector. */
void vector_dealloc_value(
  vector* vec
);

/*! \brief Deallocates all memory allocated for vector */
void vector_dealloc(
  vector* vec
);

#endif

