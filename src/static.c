/*!
 \file     static.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/02/2002
*/

#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "db.h"
#include "util.h"


/*!
 \param stexp  Pointer to static expression.
 \param op     Unary static expression operation.
 \param line   Line number that this expression was found on in file.

 \return Returns pointer to new static_expr structure.

*/
static_expr* static_expr_gen_unary( static_expr* stexp, int op, int line ) {

  int uop;      /* Temporary bit holder */
  int i;        /* Loop iterator        */

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

      stexp->exp = db_create_expression( stexp->exp, NULL, op, line, NULL );

    }

  }

  return( stexp );

}

/*!
 \param right  Pointer to right static expression.
 \param left   Pointer to left static expression.
 \param op     Static expression operation.
 \param line   Line number that static expression operation found on.

 \return Returns pointer to new static_expr structure.

*/
static_expr* static_expr_gen( static_expr* right, static_expr* left, int op, int line ) {

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

        right->exp = db_create_expression( NULL, NULL, EXP_OP_STATIC, line, NULL );
        vector_init( right->exp->value, (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_WIDTH( 32 ) ), 32, 0 );  
        vector_from_int( right->exp->value, right->num );

        right->exp = db_create_expression( right->exp, left->exp, op, line, NULL );

      }

    } else {

      if( left->exp == NULL ) {

        left->exp = db_create_expression( NULL, NULL, EXP_OP_STATIC, line, NULL );
        vector_init( left->exp->value, (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_WIDTH( 32 ) ), 32, 0 );
        vector_from_int( left->exp->value, left->num );

        right->exp = db_create_expression( right->exp, left->exp, op, line, NULL );

      } else {

        right->exp = db_create_expression( right->exp, left->exp, op, line, NULL );

      }

    }

  }

  if( left != NULL ) {
    if( left->exp != NULL ) {
      expression_dealloc( left->exp, FALSE );
    }
    free_safe( left );
  }

  return( right );

}

/* $Log$ */

