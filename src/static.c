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


#ifndef RUNLIB
/*!
 \param stexp  Pointer to static expression.
 \param op     Unary static expression operation.
 \param line   Line number that this expression was found on in file.
 \param first  Column index of first character in this expression.
 \param last   Column index of last character in this expression.

 \return Returns pointer to new static_expr structure.

 \throws anonymous expression_create expression_create

 Used by the parser to calculate a new static_expr structure based on the
 unary operation encountered while parsing.  Based on the operation type
 specified in the argument list, performs unary operation (if operand is
 a static number and not an expression -- parameter in operand expression
 tree), storing result into original static_expr number field and returns
 the original structure back to the calling function.  If the operand is an
 expression, create an expression for the specified operation type and store
 this expression in the original expression pointer field.
*/
static_expr* static_expr_gen_unary(
  static_expr* stexp,    /*!< Pointer to static expression */
  exp_op_type  op,       /*!< Unary static expression operation */
  unsigned int line,     /*!< Line number that this expression was found on in file */
  unsigned int ppfline,  /*!< First line number from preprocessed file of this expression */
  unsigned int pplline,  /*!< Last line number from preprocessed file of this expression */
  int          first,    /*!< Column index of first character in this expression */
  int          last      /*!< Column index of last character in this expression */
) { PROFILE(STATIC_EXPR_GEN_UNARY);

  expression*  tmpexp;  /* Container for newly created expression */
  int          uop;     /* Temporary bit holder */
  unsigned int i;       /* Loop iterator */

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
              /*@-shiftimplementation@*/
              case EXP_OP_UAND  :  uop = uop & ((stexp->num >> i) & 0x1);  break;
              case EXP_OP_UNOR  :
              case EXP_OP_UOR   :  uop = uop | ((stexp->num >> i) & 0x1);  break;
              case EXP_OP_UNXOR :
              case EXP_OP_UXOR  :  uop = uop ^ ((stexp->num >> i) & 0x1);  break;
              /*@=shiftimplementation@*/
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
          tmpexp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
          curr_expr_id++;
          {
            vector* vec = vector_create( 32, VTYPE_EXP, VDATA_UL, TRUE );
            vector_dealloc( tmpexp->value );
            tmpexp->value = vec;
          }
          (void)vector_from_int( tmpexp->value, stexp->num );

          stexp->exp = expression_create( tmpexp, NULL, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
          curr_expr_id++;
          break;
        default :  break;
      }

    } else {

      tmpexp = expression_create( stexp->exp, NULL, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
      curr_expr_id++;
      stexp->exp = tmpexp;

    }

  }

  PROFILE_END;

  return( stexp );

}

/*!
 \return Returns pointer to new static_expr structure.

 \throws anonymous expression_create expression_create expression_create expression_create expression_create expression_create

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
static_expr* static_expr_gen(
  static_expr* right,     /*!< Pointer to right static expression */
  static_expr* left,      /*!< Pointer to left static expression */
  int          op,        /*!< Static expression operation */
  unsigned int line,      /*!< Line number that static expression operation found on */
  unsigned int ppfline,   /*!< First line number from preprocessed file that static expression is found at */
  unsigned int pplline,   /*!< Last line number from preprocessed file that static expression is found at */
  int          first,     /*!< Column index of first character in expression */
  int          last,      /*!< Column index of last character in expression */
  char*        func_name  /*!< Name of function to call (only valid when op == EXP_OP_FUNC_CALL) */
) { PROFILE(STATIC_EXPR_GEN);

  expression* tmpexp;     /* Temporary expression for holding newly created parent expression */
  int         i;          /* Loop iterator */
  int         value = 1;  /* Temporary value */
  
  assert( (op == EXP_OP_XOR)    || (op == EXP_OP_MULTIPLY) || (op == EXP_OP_DIVIDE) || (op == EXP_OP_MOD)       ||
          (op == EXP_OP_ADD)    || (op == EXP_OP_SUBTRACT) || (op == EXP_OP_AND)    || (op == EXP_OP_OR)        ||
          (op == EXP_OP_NOR)    || (op == EXP_OP_NAND)     || (op == EXP_OP_NXOR)   || (op == EXP_OP_EXPONENT)  ||
          (op == EXP_OP_LSHIFT) || (op == EXP_OP_RSHIFT)   || (op == EXP_OP_LIST)   || (op == EXP_OP_FUNC_CALL) ||
          (op == EXP_OP_GE)     || (op == EXP_OP_LE)       || (op == EXP_OP_EQ)     || (op == EXP_OP_GT)        ||
          (op == EXP_OP_LT)     || (op == EXP_OP_SBIT_SEL) || (op == EXP_OP_LAND)   || (op == EXP_OP_LOR)       ||
          (op == EXP_OP_NE)     || (op == EXP_OP_PLIST)    || (op == EXP_OP_SCLOG2) );

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
          /*@-shiftnegative -shiftimplementation@*/
          case EXP_OP_LSHIFT   :  right->num = left->num << right->num;    break;
          case EXP_OP_RSHIFT   :  right->num = left->num >> right->num;    break;
          /*@=shiftnegative =shiftimplementation@*/
          case EXP_OP_GE       :  right->num = (left->num >= right->num) ? 1 : 0;  break;
          case EXP_OP_LE       :  right->num = (left->num <= right->num) ? 1 : 0;  break;
          case EXP_OP_EQ       :  right->num = (left->num == right->num) ? 1 : 0;  break;
          case EXP_OP_NE       :  right->num = (left->num != right->num) ? 1 : 0;  break;
          case EXP_OP_GT       :  right->num = (left->num > right->num)  ? 1 : 0;  break;
          case EXP_OP_LT       :  right->num = (left->num < right->num)  ? 1 : 0;  break;
          case EXP_OP_LAND     :  right->num = (left->num && right->num) ? 1 : 0;  break;
          case EXP_OP_LOR      :  right->num = (left->num || right->num) ? 1 : 0;  break;
          default              :  break;
        }

      } else {

        right->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
        curr_expr_id++;
        {
          vector* vec = vector_create( 32, VTYPE_EXP, VDATA_UL, TRUE );
          vector_dealloc( right->exp->value );
          right->exp->value = vec;
        }
        (void)vector_from_int( right->exp->value, right->num );

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
        curr_expr_id++;
        right->exp = tmpexp;
        
      }

    } else {

      if( left->exp == NULL ) {

        left->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
        curr_expr_id++;
        {
          vector* vec = vector_create( 32, VTYPE_EXP, VDATA_UL, TRUE );
          vector_dealloc( left->exp->value );
          left->exp->value = vec;
        }
        (void)vector_from_int( left->exp->value, left->num );

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
        curr_expr_id++;
        right->exp = tmpexp;

      } else {

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
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
    right->exp = expression_create( NULL, left->exp, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
    curr_expr_id++;

    /* Make sure that we bind this later */
    bind_add( FUNIT_FUNCTION, func_name, right->exp, curr_funit, TRUE );

  } else if( op == EXP_OP_SCLOG2 ) {

    assert( right == NULL );
    assert( left  != NULL );

    right      = (static_expr*)malloc_safe( sizeof( static_expr ) );
    right->exp = NULL;

    if( left->exp == NULL ) {
      unsigned int val      = left->num;
      unsigned int num_ones = 0;
      right->num = 0;
      while( val != 0 ) {
        right->num++;
        num_ones += (val & 0x1);
        val >>= 1;
      }
      if( num_ones == 1 ) {
        right->num--;
      }
    } else {
      right->exp = expression_create( NULL, left->exp, op, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
      curr_expr_id++;
    }

  }

  static_expr_dealloc( left, FALSE );

  PROFILE_END;

  return( right );

}

/*!
 \return Returns a static expression structure which represents the value of the given ternary expression (?:).
*/
static_expr* static_expr_gen_ternary( 
  static_expr* sel,       /*!< Pointer to selection static expression */
  static_expr* right,     /*!< Pointer to right static expression */
  static_expr* left,      /*!< Pointer to left static expression */
  unsigned int line,      /*!< Line number that static expression operation found on */
  unsigned int ppfline,   /*!< First line number from preprocessed file that static expression is found at */
  unsigned int pplline,   /*!< Last line number from preprocessed file that static expression is found at */
  int          first,     /*!< Column index of first character in expression */
  int          last       /*!< Column index of last character in expression */
) { PROFILE(STATIC_EXPR_GEN_TERNARY);

  if( (sel != NULL) && (right != NULL) && (left != NULL) ) {

    /* If the selector is known, select the needed value immediately */
    if( sel->exp == NULL ) {

      /* If the left is selected, get the number or expression */
      if( sel->num ) {
        if( left->exp == NULL ) {
          sel->num = left->num;
        } else {
          sel->exp = left->exp;
        }
        expression_dealloc( right->exp, FALSE );

      /* Otherwise, get the number of expression for the right static expression */
      } else {
        if( right->exp == NULL ) {
          sel->num = right->num;
        } else {
          sel->exp = right->exp;
        }
        expression_dealloc( left->exp, FALSE );
  
      }

    /* Otherwise, create the ternary expression */
    } else {

      expression* tmpcond;
      expression* tmpcsel;

      if( left->exp == NULL ) {
        left->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
        curr_expr_id++;
        {
          vector* vec = vector_create( 32, VTYPE_EXP, VDATA_UL, TRUE );
          vector_dealloc( left->exp->value );
          left->exp->value = vec;
        }
        (void)vector_from_int( left->exp->value, left->num );
      }

      if( right->exp == NULL ) {
        right->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
        curr_expr_id++;
        {
          vector* vec = vector_create( 32, VTYPE_EXP, VDATA_UL, TRUE );
          vector_dealloc( right->exp->value );
          right->exp->value = vec;
        }
        (void)vector_from_int( right->exp->value, right->num );
      }

      tmpcsel = expression_create( right->exp, left->exp, EXP_OP_COND_SEL, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
      curr_expr_id++;

      tmpcond = expression_create( tmpcsel, sel->exp, EXP_OP_COND, FALSE, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
      curr_expr_id++;

      sel->exp = tmpcond;

    }

  }

  static_expr_dealloc( right, FALSE );
  static_expr_dealloc( left,  FALSE );

  PROFILE_END;

  return( sel );

}

/*!
 \param left        Pointer to static expression on left of vector.
 \param right       Pointer to static expression on right of vector.
 \param width       Calculated width of combined right/left static expressions.
 \param lsb         Calculated lsb of combined right/left static expressions.
 \param big_endian  Set to 1 if the MSB/LSB is specified in big endian order.

 Calculates the LSB, width and endianness of a vector defined by the specified left and right
 static expressions.  If the width cannot be obtained immediately (parameter in static
 expression), set width to 0.  If the LSB cannot be obtained immediately (parameter in
 static expression), set LSB to -1.  The endianness can only be used if the width is known.
 The returned width and lsb parameters can be used to size a vector instantiation.
*/
void static_expr_calc_lsb_and_width_pre(
            static_expr*  left,
            static_expr*  right,
  /*@out@*/ unsigned int* width,
  /*@out@*/ int*          lsb,
  /*@out@*/ int*          big_endian
) { PROFILE(STATIC_EXPR_CALC_LSB_AND_WIDTH_PRE);

  *width      = 0;
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

  PROFILE_END;

}
#endif /* RUNLIB */

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
void static_expr_calc_lsb_and_width_post(
            static_expr*  left,
            static_expr*  right,
  /*@out@*/ unsigned int* width,
  /*@out@*/ int*          lsb,
  /*@out@*/ int*          big_endian
) { PROFILE(STATIC_EXPR_CALC_LSB_AND_WIDTH_POST);
  
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

  PROFILE_END;

}

/*!
 \param stexp   Pointer to static expression to deallocate.
 \param rm_exp  Specifies that expression tree should be deallocated.

 Deallocates all allocated memory from the heap for the specified static_expr
 structure.
*/
void static_expr_dealloc(
  static_expr* stexp,
  bool         rm_exp
) { PROFILE(STATIC_EXPR_DEALLOC);

  if( stexp != NULL ) {

    if( rm_exp && (stexp->exp != NULL) ) {
      expression_dealloc( stexp->exp, FALSE );
    }

    free_safe( stexp, sizeof( static_expr ) );

  }

}

