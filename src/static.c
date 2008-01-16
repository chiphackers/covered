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
 \file     static.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/02/2002
 
 \par
 To accommodate the need for parameters/genvars (variables) in static expressions, the static_expr
 structure and supporting code was created to maintain the efficiency of static expressions
 that consist of known values while being able to keep track of parameter/genvar uses in
 static expressions.
 
 \par
 A static_expr structure consists of members:  an int (stores known integer values) and an
 expression pointer.  If the expression pointer is set to NULL for the given static_expr,
 it is assumed that the static_expr structure contains a valid, known value that can be
 used in immediate computations.  If the expression pointer is not NULL, it is assumed
 that the static_expr structure contains an expression tree that needs to be evaluated at
 a later time (when parameters/genvars are being elaborated).
 
 \par
 When a static expression is being parsed and a static value (integer value) is encountered,
 a new static_expr is allocated from heap memory and the number field is assigned to this
 integer value.  The new static_expr structure is then passed up the tree to be used in
 further calculations, if necessary.  If a static expression is being parsed and an identifier
 (parameter/genvar) is encountered, an expression is created with an operation type of EXP_OP_SIG
 to indicate that a parameter/genvar is required during elaboration.  The name of the necessary
 parameter/genvar is bound to the newly created expression.
 
 \par
 Using this strategy for handling static expressions, it becomes evident that we retain
 the efficiency of calculating static expression that consists entirely of known values (the
 only overhead is the allocation/deallocation of a static_expr structure from the heap).
 If a parameter/genvar is found during the parse stage, more effort is required to calculate the
 static_expr, but this is considered necessary in the larger scope of things so we will not
 concern ourselves with this overhead (which is fairly minimal anyways).
*/

#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "static.h"
#include "expr.h"
#include "db.h"
#include "util.h"
#include "vector.h"
#include "binding.h"


extern func_unit* curr_funit;
extern int        curr_expr_id;


/*!
 \param stexp  Pointer to static expression.
 \param op     Unary static expression operation.
 \param line   Line number that this expression was found on in file.
 \param first  Column index of first character in this expression.
 \param last   Column index of last character in this expression.

 \return Returns pointer to new static_expr structure.

 Used by the parser to calculate a new static_expr structure based on the
 unary operation encountered while parsing.  Based on the operation type
 specified in the argument list, performs unary operation (if operand is
 a static number and not an expression -- parameter in operand expression
 tree), storing result into original static_expr number field and returns
 the original structure back to the calling function.  If the operand is an
 expression, create an expression for the specified operation type and store
 this expression in the original expression pointer field.
*/
static_expr* static_expr_gen_unary( static_expr* stexp, int op, int line, int first, int last ) { PROFILE(STATIC_EXPR_GEN_UNARY);

  expression* tmpexp;  /* Container for newly created expression */
  int uop;             /* Temporary bit holder */
  int i;               /* Loop iterator */

  if( stexp != NULL ) {

    assert( (op == EXP_OP_UINV)  || (op == EXP_OP_UAND) || (op == EXP_OP_UOR)   || (op == EXP_OP_UXOR)  ||
            (op == EXP_OP_UNAND) || (op == EXP_OP_UNOR) || (op == EXP_OP_UNXOR) || (op == EXP_OP_UNOT)  ||
            (op == EXP_OP_PASSIGN) );

    if( stexp->exp == NULL ) {

      switch( op ) {

        case EXP_OP_UINV :
          stexp->num = ~stexp->num;
          break;

        case EXP_OP_UAND  :
        case EXP_OP_UOR   :
        case EXP_OP_UXOR  :
        case EXP_OP_UNAND :
        case EXP_OP_UNOR  :
        case EXP_OP_UNXOR :
          uop = stexp->num & 0x1;
          for( i=1; i<(SIZEOF_INT * 8); i++ ) {
            switch( op ) {
              case EXP_OP_UNAND :
              case EXP_OP_UAND  :  uop = uop & ((stexp->num >> i) & 0x1);  break;
              case EXP_OP_UNOR  :
              case EXP_OP_UOR   :  uop = uop | ((stexp->num >> i) & 0x1);  break;
              case EXP_OP_UNXOR :
              case EXP_OP_UXOR  :  uop = uop ^ ((stexp->num >> i) & 0x1);  break;
              default           :  break;
            }
          }
          switch( op ) {
            case EXP_OP_UAND  :
            case EXP_OP_UOR   :
            case EXP_OP_UXOR  :  stexp->num = uop;                 break;
            case EXP_OP_UNAND :
            case EXP_OP_UNOR  :
            case EXP_OP_UNXOR :  stexp->num = (uop == 0) ? 1 : 0;  break;
            default           :  break;
          }
          break;

        case EXP_OP_UNOT :
          stexp->num = (stexp->num == 0) ? 1 : 0;
          break;

        case EXP_OP_PASSIGN :
          tmpexp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, first, last, FALSE );
          curr_expr_id++;
          vector_init( tmpexp->value, (vec_data*)malloc_safe( sizeof( vec_data ) * 32 ), TRUE, 32, VTYPE_EXP );
          vector_from_int( tmpexp->value, stexp->num );
        
          stexp->exp = expression_create( tmpexp, NULL, op, FALSE, curr_expr_id, line, first, last, FALSE );
          curr_expr_id++;
          break;
        default :  break;
      }

    } else {

      tmpexp = expression_create( stexp->exp, NULL, op, FALSE, curr_expr_id, line, first, last, FALSE );
      curr_expr_id++;
      stexp->exp = tmpexp;

    }

  }

  return( stexp );

}

/*!
 \param right      Pointer to right static expression.
 \param left       Pointer to left static expression.
 \param op         Static expression operation.
 \param line       Line number that static expression operation found on.
 \param first      Column index of first character in expression.
 \param last       Column index of last character in expression.
 \param func_name  Name of function to call (only valid when op == EXP_OP_FUNC_CALL)

 \return Returns pointer to new static_expr structure.

 Used by the parser to calculate a new static_expr structure based on the
 operation encountered while parsing.  Based on the operation type
 specified in the argument list, performs unary operation (if both operands
 are static numbers, storing result into original static_expr number field and returns
 If only one of the operands is an expression, create a EXP_OP_STATIC expression for
 the other operand and create an expression consisting of these two expressions and
 the specified operation.  If both operands are expressions, simply create a new expression
 consisting of those two expressions and specified operator.  Store the newly create
 expression in the original right static_expr and deallocate the left static_expr.
*/
static_expr* static_expr_gen( static_expr* right, static_expr* left, int op, int line, int first, int last, char* func_name ) { PROFILE(STATIC_EXPR_GEN);

  expression* tmpexp;     /* Temporary expression for holding newly created parent expression */
  int         i;          /* Loop iterator */
  int         value = 1;  /* Temporary value */
  
  assert( (op == EXP_OP_XOR)    || (op == EXP_OP_MULTIPLY) || (op == EXP_OP_DIVIDE) || (op == EXP_OP_MOD)       ||
          (op == EXP_OP_ADD)    || (op == EXP_OP_SUBTRACT) || (op == EXP_OP_AND)    || (op == EXP_OP_OR)        ||
          (op == EXP_OP_NOR)    || (op == EXP_OP_NAND)     || (op == EXP_OP_NXOR)   || (op == EXP_OP_EXPONENT)  ||
          (op == EXP_OP_LSHIFT) || (op == EXP_OP_RSHIFT)   || (op == EXP_OP_LIST)   || (op == EXP_OP_FUNC_CALL) ||
          (op == EXP_OP_GE)     || (op == EXP_OP_LE)       || (op == EXP_OP_EQ)     || (op == EXP_OP_GT)        ||
          (op == EXP_OP_LT)     || (op == EXP_OP_SBIT_SEL) );

  if( (right != NULL) && (left != NULL) ) {

    if( right->exp == NULL ) {

      if( left->exp == NULL ) {

        switch( op ) {
          case EXP_OP_XOR      :  right->num = left->num ^ right->num;     break;
          case EXP_OP_MULTIPLY :  right->num = left->num * right->num;     break;
          case EXP_OP_DIVIDE   :  right->num = left->num / right->num;     break;
          case EXP_OP_MOD      :  right->num = left->num % right->num;     break;
          case EXP_OP_ADD      :  right->num = left->num + right->num;     break;
          case EXP_OP_SUBTRACT :  right->num = left->num - right->num;     break;
          case EXP_OP_EXPONENT :
            for( i=0; i<right->num; i++ ) {
              value *= left->num;
            }
            right->num = value;
            break;
          case EXP_OP_AND      :  right->num = left->num & right->num;     break;
          case EXP_OP_OR       :  right->num = left->num | right->num;     break;
          case EXP_OP_NOR      :  right->num = ~(left->num | right->num);  break;
          case EXP_OP_NAND     :  right->num = ~(left->num & right->num);  break;
          case EXP_OP_NXOR     :  right->num = ~(left->num ^ right->num);  break;
          case EXP_OP_LSHIFT   :  right->num = left->num << right->num;    break;
          case EXP_OP_RSHIFT   :  right->num = left->num >> right->num;    break;
          case EXP_OP_GE       :  right->num = (left->num >= right->num) ? 1 : 0;  break;
          case EXP_OP_LE       :  right->num = (left->num <= right->num) ? 1 : 0;  break;
          case EXP_OP_EQ       :  right->num = (left->num == right->num) ? 1 : 0;  break;
          case EXP_OP_GT       :  right->num = (left->num > right->num)  ? 1 : 0;  break;
          case EXP_OP_LT       :  right->num = (left->num < right->num)  ? 1 : 0;  break;
          default              :  break;
        }

      } else {

        right->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, first, last, FALSE );
        curr_expr_id++;
        vector_init( right->exp->value, (vec_data*)malloc_safe( sizeof( vec_data ) * 32 ), TRUE, 32, VTYPE_EXP );  
        vector_from_int( right->exp->value, right->num );

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, curr_expr_id, line, first, last, FALSE );
        curr_expr_id++;
        right->exp = tmpexp;
        
      }

    } else {

      if( left->exp == NULL ) {

        left->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, first, last, FALSE );
        curr_expr_id++;
        vector_init( left->exp->value, (vec_data*)malloc_safe( sizeof( vec_data ) * 32 ), TRUE, 32, VTYPE_EXP );
        vector_from_int( left->exp->value, left->num );

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, curr_expr_id, line, first, last, FALSE );
        curr_expr_id++;
        right->exp = tmpexp;

      } else {

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, curr_expr_id, line, first, last, FALSE );
        curr_expr_id++;
        right->exp = tmpexp;

      }

    }

  } else if( (op == EXP_OP_FUNC_CALL) || (op == EXP_OP_SBIT_SEL) ) {

    /*
     If this is a function call or SBIT_SEL, only the left expression will be a valid expression (so we need to special
     handle this)
    */

    assert( right == NULL );
    assert( left  != NULL );

    right = (static_expr*)malloc_safe( sizeof( static_expr ) );
    right->exp = expression_create( NULL, left->exp, op, FALSE, curr_expr_id, line, first, last, FALSE );
    curr_expr_id++;

    /* Make sure that we bind this later */
    bind_add( FUNIT_FUNCTION, func_name, right->exp, curr_funit );

  }

  static_expr_dealloc( left, FALSE );

  return( right );

}

/*!
 \param left        Pointer to static expression on left of vector.
 \param right       Pointer to static expression on right of vector.
 \param width       Calculated width of combined right/left static expressions.
 \param lsb         Calculated lsb of combined right/left static expressions.
 \param big_endian  Set to 1 if the MSB/LSB is specified in big endian order.

 Calculates the LSB, width and endianness of a vector defined by the specified left and right
 static expressions.  If the width cannot be obtained immediately (parameter in static
 expression), set width to -1.  If the LSB cannot be obtained immediately (parameter in
 static expression), set LSB to -1.  The endianness can only be used if the width is known.
 The returned width and lsb parameters can be used to size a vector instantiation.
*/
void static_expr_calc_lsb_and_width_pre( static_expr* left, static_expr* right, int* width, int* lsb, int* big_endian ) { PROFILE(STATIC_EXPR_CALC_LSB_AND_WIDTH_PRE);

  *width      = -1;
  *lsb        = -1;
  *big_endian = 0;

  if( (right != NULL) && (right->exp == NULL) ) {
    *lsb = right->num;
    assert( *lsb >= 0 );
  }

  if( (left != NULL) && (left->exp == NULL) ) {
    if( *lsb != -1 ) {
      if( *lsb <= left->num ) {
        *width = (left->num - *lsb) + 1;
        assert( *width > 0 );
      } else {
        *width      = (*lsb - left->num) + 1;
        *lsb        = left->num;
        *big_endian = 1;
        assert( *width > 0 );
        assert( *lsb >= 0 );
      }
    } else {
      *lsb = left->num;
      assert( *lsb >= 0 );
    }
  }

}

/*!
 \param left        Pointer to static expression on left of vector.
 \param right       Pointer to static expression on right of vector.
 \param width       Calculated width of combined right/left static expressions.
 \param lsb         Calculated lsb of combined right/left static expressions.
 \param big_endian  Set to 1 if the left expression is less than the right expression.
 
 Calculates the LSB and width of a vector defined by the specified left and right
 static expressions.  This function assumes that any expressions have been calculated for
 a legal value.
*/
void static_expr_calc_lsb_and_width_post( static_expr* left, static_expr* right, int* width, int* lsb, int* big_endian ) { PROFILE(STATIC_EXPR_CALC_LSB_AND_WIDTH_POST);
  
  assert( left  != NULL );
  assert( right != NULL );

  *width      = 1;
  *lsb        = -1;
  *big_endian = 0;

  /* If the right static expression contains an expression, get its integer value and place it in the num field */
  if( right->exp != NULL ) {
    right->num = vector_to_int( right->exp->value );
  }

  /* If the left static expression contains an expression, get its integer value and place it in the num field */
  if( left->exp != NULL ) {
    left->num = vector_to_int( left->exp->value );
    // expression_display( left->exp );
  }
  
  /* Get initial value for LSB */
  *lsb = right->num;
  assert( *lsb >= 0 );

  /* Calculate width and make sure that LSB is the lower of the two values */
  if( *lsb <= left->num ) {
    *width = (left->num - *lsb) + 1;
    assert( *width > 0 );
  } else {
    *width      = (*lsb - left->num) + 1;
    *lsb        = left->num;
    *big_endian = 1;
    assert( *width > 0 );
    assert( *lsb >= 0 );
  }

}

/*!
 \param stexp   Pointer to static expression to deallocate.
 \param rm_exp  Specifies that expression tree should be deallocated.

 Deallocates all allocated memory from the heap for the specified static_expr
 structure.
*/
void static_expr_dealloc( static_expr* stexp, bool rm_exp ) { PROFILE(STATIC_EXPR_DEALLOC);

  if( stexp != NULL ) {

    if( rm_exp && (stexp->exp != NULL) ) {
      expression_dealloc( stexp->exp, FALSE );
    }

    free_safe( stexp );

  }

}

/*
 $Log$
 Revision 1.28  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.27  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.26  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.25  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.24  2006/09/22 04:23:04  phase1geo
 More fixes to support new signal range structure.  Still don't have full
 regressions passing at the moment.

 Revision 1.23  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.22  2006/07/11 04:59:08  phase1geo
 Reworking the way that instances are being generated.  This is to fix a bug and
 pave the way for generate loops for instances.  Code not working at this point
 and may cause serious problems for regression runs.

 Revision 1.21  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.20  2006/05/25 12:11:02  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.19  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.18  2006/04/13 14:59:25  phase1geo
 Updating CDD version from 6 to 7 due to changes in the merge facility.  Full
 regression now passes.

 Revision 1.17.4.1.4.1  2006/05/27 05:56:14  phase1geo
 Fixing last problem with bug fix.  Updated date on NEWS.

 Revision 1.17.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.17  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.16  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.15  2006/01/19 00:01:09  phase1geo
 Lots of changes/additions.  Summary report window work is now complete (with the
 exception of adding extra features).  Added support for parsing left and right
 shift operators and the exponent operator in static expression scenarios.  Fixed
 issues related to GUI (due to recent changes in the score command).  Things seem
 to be generally working as expected with the GUI now.

 Revision 1.14  2006/01/13 04:01:04  phase1geo
 Adding support for exponential operation.  Added exponent1 diagnostic to verify
 but Icarus does not support this currently.

 Revision 1.13  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.12  2004/04/19 04:54:56  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.11  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.10  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.9  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.8  2003/10/11 05:15:08  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.7  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.6  2002/10/31 23:14:28  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.5  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.4  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.3  2002/10/12 06:51:34  phase1geo
 Updating development documentation to match all changes within source.
 Adding new development pages created by Doxygen for the new source
 files.

 Revision 1.2  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.1  2002/10/02 18:55:29  phase1geo
 Adding static.c and static.h files for handling static expressions found by
 the parser.  Initial versions which compile but have not been tested.
*/

