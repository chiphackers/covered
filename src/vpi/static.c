/*!
 \file     static.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/02/2002
 
 \par
 To accommodate the need for parameters (variables) in static expressions, the static_expr
 structure and supporting code was created to maintain the efficiency of static expressions
 that consist of known values while being able to keep track of parameter uses in
 static expressions.
 
 \par
 A static_expr structure consists of members:  an int (stores known integer values) and an
 expression pointer.  If the expression pointer is set to NULL for the given static_expr,
 it is assumed that the static_expr structure contains a valid, known value that can be
 used in immediate computations.  If the expression pointer is not NULL, it is assumed
 that the static_expr structure contains an expression tree that needs to be evaluated at
 a later time (when parameters are being elaborated).
 
 \par
 When a static expression is being parsed and a static value (integer value) is encountered,
 a new static_expr is allocated from heap memory and the number field is assigned to this
 integer value.  The new static_expr structure is then passed up the tree to be used in
 further calculations, if necessary.  If a static expression is being parsed and an identifier
 (parameter) is encountered, an expression is created with an operation type of EXP_OP_SIG
 to indicate that a parameter is required during elaboration.  The name of the necessary
 parameter is bound to the newly created expression.
 
 \par
 Using this strategy for handling static expressions, it becomes evident that we retain
 the efficiency of calculating static expression that consists entirely of known values (the
 only overhead is the allocation/deallocation of a static_expr structure from the heap).
 If a parameter is found during the parse stage, more effort is required to calculate the
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
static_expr* static_expr_gen_unary( static_expr* stexp, int op, int line, int first, int last ) {

  expression* tmpexp;  /* Container for newly created expression */
  int uop;             /* Temporary bit holder                   */
  int i;               /* Loop iterator                          */

  if( stexp != NULL ) {

    assert( (op == EXP_OP_UINV)  || (op == EXP_OP_UAND) || (op == EXP_OP_UOR)   || (op == EXP_OP_UXOR)  ||
            (op == EXP_OP_UNAND) || (op == EXP_OP_UNOR) || (op == EXP_OP_UNXOR) || (op == EXP_OP_UNOT) );

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

        default :  break;
      }

    } else {

      tmpexp = expression_create( stexp->exp, NULL, op, FALSE, 0, line, first, last, FALSE );
      stexp->exp->parent->expr = tmpexp;
      stexp->exp = tmpexp;

    }

  }

  return( stexp );

}

/*!
 \param right  Pointer to right static expression.
 \param left   Pointer to left static expression.
 \param op     Static expression operation.
 \param line   Line number that static expression operation found on.
 \param first  Column index of first character in expression.
 \param last   Column index of last character in expression.

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
static_expr* static_expr_gen( static_expr* right, static_expr* left, int op, int line, int first, int last ) {

  expression* tmpexp;    /* Temporary expression for holding newly created parent expression */
  
  assert( (op == EXP_OP_XOR) || (op == EXP_OP_MULTIPLY) || (op == EXP_OP_DIVIDE) || (op == EXP_OP_MOD) ||
          (op == EXP_OP_ADD) || (op == EXP_OP_SUBTRACT) || (op == EXP_OP_AND)    || (op == EXP_OP_OR)  ||
          (op == EXP_OP_NOR) || (op == EXP_OP_NAND)     || (op == EXP_OP_NXOR) );

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
          case EXP_OP_AND      :  right->num = left->num & right->num;     break;
          case EXP_OP_OR       :  right->num = left->num | right->num;     break;
          case EXP_OP_NOR      :  right->num = ~(left->num | right->num);  break;
          case EXP_OP_NAND     :  right->num = ~(left->num & right->num);  break;
          case EXP_OP_NXOR     :  right->num = ~(left->num ^ right->num);  break;
          default              :  break;
        }

      } else {

        right->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, 0, line, first, last, FALSE );
        vector_init( right->exp->value, (vec_data*)malloc_safe( (sizeof( vec_data ) * 32), __FILE__, __LINE__ ), 32 );  
        vector_from_int( right->exp->value, right->num );

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, 0, line, first, last, FALSE );
        right->exp->parent->expr = tmpexp;
        left->exp->parent->expr  = tmpexp;
        right->exp = tmpexp;
        
      }

    } else {

      if( left->exp == NULL ) {

        left->exp = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, 0, line, first, last, FALSE );
        vector_init( left->exp->value, (vec_data*)malloc_safe( (sizeof( vec_data ) * 32), __FILE__, __LINE__ ), 32 );
        vector_from_int( left->exp->value, left->num );

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, 0, line, first, last, FALSE );
        right->exp->parent->expr = tmpexp;
        right->exp = tmpexp;

      } else {

        tmpexp = expression_create( right->exp, left->exp, op, FALSE, 0, line, first, last, FALSE );
        right->exp->parent->expr = tmpexp;
        left->exp->parent->expr  = tmpexp;
        right->exp = tmpexp;

      }

    }

  }

  static_expr_dealloc( left, FALSE );

  return( right );

}

/*!
 \param left   Pointer to static expression on left of vector.
 \param right  Pointer to static expression on right of vector.
 \param width  Calculated width of combined right/left static expressions.
 \param lsb    Calculated lsb of combined right/left static expressions.
 
 Calculates the LSB and width of a vector defined by the specified left and right
 static expressions.  If the width cannot be obtained immediately (parameter in static
 expression), set width to -1.  If the LSB cannot be obtained immediately (parameter in
 static expression), set LSB to -1.  The returned width and lsb parameters can be used
 to size a vector instantiation.
*/
void static_expr_calc_lsb_and_width( static_expr* left, static_expr* right, int* width, int* lsb ) {
  
  *width = -1;
  *lsb   = -1;
  
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
        *width = (*lsb - left->num) + 1;
        *lsb   = left->num;
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
 \param stexp   Pointer to static expression to deallocate.
 \param rm_exp  Specifies that expression tree should be deallocated.

 Deallocates all allocated memory from the heap for the specified static_expr
 structure.
*/
void static_expr_dealloc( static_expr* stexp, bool rm_exp ) {

  if( stexp != NULL ) {

    if( rm_exp && (stexp->exp != NULL) ) {
      expression_dealloc( stexp->exp, FALSE );
    }

    free_safe( stexp );

  }

}

/*
 $Log$
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

