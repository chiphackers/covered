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
  vector* vec, 
  int     base,
  bool    show_all
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
  int     msb,
  int     lsb
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

/*! \brief Deallocates the value structure for the given vector. */
void vector_dealloc_value( vector* vec );

/*! \brief Deallocates all memory allocated for vector */
void vector_dealloc( vector* vec );


/*
 $Log$
 Revision 1.73  2009/01/01 07:24:44  phase1geo
 Checkpointing work on memory coverage.  Simple testing now works but still need
 to do some debugging here.

 Revision 1.72  2008/12/24 21:19:02  phase1geo
 Initial work at getting FSM coverage put in (this looks to be working correctly
 to this point).  Updated regressions per fixes.  Checkpointing.

 Revision 1.71  2008/10/27 21:14:02  phase1geo
 First pass at getting the $value$plusargs system function call to work.  More
 work to do here.  Checkpointing.

 Revision 1.70  2008/10/21 05:38:42  phase1geo
 More updates to support real values.  Added vector_from_real64 functionality.
 Checkpointing.

 Revision 1.69  2008/10/20 23:20:02  phase1geo
 Adding support for vector_from_int coverage accumulation (untested at this point).
 Updating Cver regressions.  Checkpointing.

 Revision 1.68  2008/10/17 23:20:51  phase1geo
 Continuing to add support support for real values.  Making some good progress here
 (real delays should be working now).  Updated regressions per recent changes.
 Checkpointing.

 Revision 1.67  2008/10/17 07:26:49  phase1geo
 Updating regressions per recent changes and doing more work to fixing real
 value bugs (still not working yet).  Checkpointing.

 Revision 1.66  2008/10/16 23:11:50  phase1geo
 More work on support for real numbers.  I believe that all of the code now
 exists in vector.c to support them.  Still need to do work in expr.c.  Added
 two new tests for real numbers to begin verifying their support (they both do
 not currently pass, however).  Checkpointing.

 Revision 1.65  2008/10/15 13:28:37  phase1geo
 Beginning to add support for real numbers.  Things are broken in regards
 to real numbers at the moment.  Checkpointing.

 Revision 1.64  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.60.2.1  2008/07/10 22:43:55  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.62  2008/06/27 14:02:05  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.61  2008/06/19 16:14:56  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.60  2008/05/30 23:00:48  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.59  2008/05/30 05:38:33  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.58.2.23  2008/05/28 05:57:12  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.58.2.22  2008/05/27 05:52:52  phase1geo
 Starting to add fix for sign extension.  Not finished at this point.

 Revision 1.58.2.21  2008/05/24 21:20:57  phase1geo
 Fixing bugs with comparison functions when values contain Xs.  Adding more diagnostics
 to regression suite to cover coverage holes.

 Revision 1.58.2.20  2008/05/23 14:50:23  phase1geo
 Optimizing vector_op_add and vector_op_subtract algorithms.  Also fixing issue with
 vector set bit.  Updating regressions per this change.

 Revision 1.58.2.19  2008/05/15 07:02:05  phase1geo
 Another attempt to fix static_afunc1 diagnostic failure.  Checkpointing.

 Revision 1.58.2.18  2008/05/12 23:12:05  phase1geo
 Ripping apart part selection code and reworking it.  Things compile but are
 functionally quite broken at this point.  Checkpointing.

 Revision 1.58.2.17  2008/05/07 23:09:11  phase1geo
 Fixing vector_mem_wr_count function and calling code.  Updating regression
 files accordingly.  Checkpointing.

 Revision 1.58.2.16  2008/05/07 21:09:10  phase1geo
 Added functionality to allow to_string to output full vector bits (even
 non-significant bits) for purposes of reporting for FSMs (matches original
 behavior).

 Revision 1.58.2.15  2008/05/04 22:05:29  phase1geo
 Adding bit-fill in vector_set_static and changing name of old bit-fill functions
 in vector.c to sign_extend to reflect their true nature.  Added new diagnostics
 to regression suite to verify single-bit select bit-fill (these tests do not work
 at this point).  Checkpointing.

 Revision 1.58.2.14  2008/05/02 22:06:13  phase1geo
 Updating arc code for new data structure.  This code is completely untested
 but does compile and has been completely rewritten.  Checkpointing.

 Revision 1.58.2.13  2008/05/02 05:02:20  phase1geo
 Completed initial pass of bit-fill code in vector_part_select_push function.
 Updating regression files.  Checkpointing.

 Revision 1.58.2.12  2008/04/30 05:56:21  phase1geo
 More work on right-shift function.  Added and connected part_select_push and part_select_pull
 functionality.  Also added new right-shift diagnostics.  Checkpointing.

 Revision 1.58.2.11  2008/04/23 21:27:06  phase1geo
 Fixing several bugs found in initial testing.  Checkpointing.

 Revision 1.58.2.10  2008/04/23 06:32:32  phase1geo
 Starting to debug vector changes.  Checkpointing.

 Revision 1.58.2.9  2008/04/23 05:20:45  phase1geo
 Completed initial pass of code updates.  I can now begin testing...  Checkpointing.

 Revision 1.58.2.8  2008/04/22 23:01:43  phase1geo
 More updates.  Completed initial pass of expr.c and fsm_arg.c.  Working
 on memory.c.  Checkpointing.

 Revision 1.58.2.7  2008/04/22 14:03:57  phase1geo
 More work on expr.c.  Checkpointing.

 Revision 1.58.2.6  2008/04/22 05:51:36  phase1geo
 Continuing work on expr.c.  Checkpointing.

 Revision 1.58.2.5  2008/04/21 23:13:05  phase1geo
 More work to update other files per vector changes.  Currently in the middle
 of updating expr.c.  Checkpointing.

 Revision 1.58.2.4  2008/04/21 04:37:23  phase1geo
 Attempting to get other files (besides vector.c) to compile with new vector
 changes.  Still work to go here.  The initial pass through vector.c is not
 complete at this time as I am attempting to get what I have completed
 debugged.  Checkpointing work.

 Revision 1.58.2.3  2008/04/20 05:43:46  phase1geo
 More work on the vector file.  Completed initial pass of conversion operations,
 bitwise operations and comparison operations.

 Revision 1.58.2.2  2008/04/18 05:05:28  phase1geo
 More updates to vector file.  Updated merge and output functions.  Checkpointing.

 Revision 1.58.2.1  2008/04/17 23:16:08  phase1geo
 More work on vector.c.  Completed initial pass of vector_db_write/read and
 vector_copy/clone functionality.  Checkpointing.

 Revision 1.58  2008/04/15 06:08:47  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.57  2008/04/08 19:50:37  phase1geo
 Removing LAST operator for PEDGE, NEDGE and AEDGE expression operations and
 replacing them with the temporary vector solution.

 Revision 1.56  2008/04/08 05:26:34  phase1geo
 Second checkin of performance optimizations (regressions do not pass at this
 point).

 Revision 1.55  2008/03/31 21:40:24  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.54  2008/03/28 17:27:00  phase1geo
 Fixing expression assignment problem due to recent changes.  Updating
 regression files per changes.

 Revision 1.53  2008/03/26 21:29:32  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.52  2008/03/13 10:28:55  phase1geo
 The last of the exception handling modifications.

 Revision 1.51  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.50  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.49  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.48  2008/01/15 23:01:16  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.47  2008/01/10 04:59:05  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.46  2008/01/07 05:01:58  phase1geo
 Cleaning up more splint errors.

 Revision 1.45  2007/12/19 04:27:52  phase1geo
 More fixes for compiler errors (still more to go).  Checkpointing.

 Revision 1.44  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.43  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.42  2006/11/22 20:20:01  phase1geo
 Updates to properly support 64-bit time.  Also starting to make changes to
 simulator to support "back simulation" for when the current simulation time
 has advanced out quite a bit of time and the simulator needs to catch up.
 This last feature is not quite working at the moment and regressions are
 currently broken.  Checkpointing.

 Revision 1.41  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.40  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.39  2006/09/25 22:22:29  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.38  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

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

