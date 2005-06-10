/*!
 \file     expr.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 
 \par Expressions
 The following are special expressions that are handled differently than standard
 unary (i.e., ~a) and dual operators (i.e., a & b).  These expressions are documented
 to help remove confusion (my own) about how they are implemented by Covered and
 handled during the parsing and scoring phases of the tool.
 
 \par EXP_OP_SIG
 A signal expression has no left or right child (they are both NULL).  Its vector
 value is a pointer to the signal vector value to which is belongs.  This allows
 the signal expression value to change automatically when the signal value is
 updated.  No further expression operation is necessary to calculate its value.
 
 \par EXP_OP_SBIT_SEL
 A single-bit signal expression has its left child pointed to the expression tree
 that is required to select the bit from the specified signal value.  The left
 child is allowed to change values during simulation.  To verify that the current
 bit select has not exceeded the ranges of the signal, the signal pointer value
 in the expression structure is used to reference the signal.  The LSB and width
 values from the actual signal can then be used to verify that we are still
 within range.  If we are found to be out of range, a value of X must be assigned
 to the SBIT_SEL expression.  The width of an SBIT_SEL is always constant (1).  The
 LSB of the SBIT_SEL is manipulated by the left expression value.
 
 \par EXP_OP_MBIT_SEL
 A multi-bit signal expression has its left child set to the expression tree on the
 left side of the ':' in the vector and the right child set to the expression tree on
 the right side of the ':' in the vector.  The width of the MBIT_SEL must be constant
 but is related to the difference between the left and right child values; therefore,
 it is required that the left and right child values be constant expressions (consisting
 of only expressions, parameters, and static values).  The width of the MBIT_SEL
 expression is calculated after reading in the MBIT_SEL expression from the CDD file.
 If the left or right child expressions are found to not be constant, an error is
 signaled to the user immediately.  The LSB is also calculated to be the lesser of the
 two child values.  The width and lsb are assigned to the MBIT_SEL expression vector
 immediately.  In the case of MBIT_SEL, the LSB is also constant.  Vector direction
 is currently not considered at this point.

 \par EXP_OP_ASSIGN, EXP_OP_NASSIGN, EXP_OP_IF
 All of these expressions are assignment operators that are in assign statements,
 behavioral non-blocking assignments, and if expressions, respectively.
 These expressions do not have an operation to perform because their vector value pointers
 point to the vector value on the right-hand side of the assignment operator.

 \par EXP_OP_BASSIGN
 The blocking assignment operator differs from the assignment operators mentioned above in that
 Covered will perform the assignment for a blocking assignment expression.  This allows us to
 expand the amount of code that can be covered by allowing several "zero-time" assignments to
 occur while maintaining accurate coverage information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "link.h"
#include "vector.h"
#include "binding.h"
#include "util.h"
#include "sim.h"
#include "fsm.h"


extern nibble xor_optab[OPTAB_SIZE];
extern nibble and_optab[OPTAB_SIZE];
extern nibble or_optab[OPTAB_SIZE];
extern nibble nand_optab[OPTAB_SIZE];
extern nibble nor_optab[OPTAB_SIZE];
extern nibble nxor_optab[OPTAB_SIZE];

extern int  curr_sim_time;
extern char user_msg[USER_MSG_LENGTH];

extern exp_link* static_expr_head;
extern exp_link* static_expr_tail;


/*!
 \param exp    Pointer to expression to add value to.
 \param width  Width of value to create.
 \param data   Specifies if nibble array should be allocated for vector.

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.
*/
void expression_create_value( expression* exp, int width, bool data ) {

  vec_data* value = NULL;    /* Temporary storage of vector nibble array */

  if( data == TRUE ) {
    value = (vec_data*)malloc_safe( (sizeof( vec_data ) * width), __FILE__, __LINE__ );
  }

  /* Create value */
  vector_init( exp->value, value, width );

}

/*!
 \param right  Pointer to expression on right.
 \param left   Pointer to expression on left.
 \param op     Operation to perform for this expression.
 \param lhs    Specifies this expression is a left-hand-side assignment expression.
 \param id     ID for this expression as determined by the parent.
 \param line   Line number this expression is on.
 \param first  First column index of expression.
 \param last   Last column index of expression.
 \param data   Specifies if we should create a nibble array for the vector value.

 \return Returns pointer to newly created expression.

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create( expression* right, expression* left, int op, bool lhs, int id, int line, int first, int last, bool data ) {

  expression* new_expr;    /* Pointer to newly created expression */
  int         rwidth = 0;  /* Bit width of expression on right    */
  int         lwidth = 0;  /* Bit width of expression on left     */

  new_expr = (expression*)malloc_safe( sizeof( expression ), __FILE__, __LINE__ );

  new_expr->suppl.all      = 0;
  new_expr->suppl.part.lhs = (nibble)lhs & 0x1;
  new_expr->op             = op;
  new_expr->id             = id;
  new_expr->ulid           = -1;
  new_expr->line           = line;
  new_expr->col            = ((first & 0xffff) << 16) | (last & 0xffff);
  new_expr->sig            = NULL;
  new_expr->parent         = (expr_stmt*)malloc_safe( sizeof( expr_stmt ), __FILE__, __LINE__ );
  new_expr->parent->expr   = NULL;
  new_expr->right          = right;
  new_expr->left           = left;
  new_expr->value          = (vector*)malloc_safe( sizeof( vector ), __FILE__, __LINE__ );
  new_expr->table          = NULL;

  if( right != NULL ) {

    /* Get information from right */
    assert( right->value != NULL );
    rwidth = right->value->width;

  }

  if( left != NULL ) {

    /* Get information from left */
    assert( left->value != NULL );
    lwidth = left->value->width;

  }

  /* Create value vector */
  if( ((op == EXP_OP_MULTIPLY) || (op == EXP_OP_LIST)) && (rwidth > 0) && (lwidth > 0) ) {

    /* For multiplication, we need a width the sum of the left and right expressions */
    assert( rwidth < 1024 );
    assert( lwidth < 1024 );
    expression_create_value( new_expr, (lwidth + rwidth), data );

  } else if( (op == EXP_OP_CONCAT) && (rwidth > 0) ) {

    assert( rwidth < 1024 );
    expression_create_value( new_expr, rwidth, data );

  } else if( (op == EXP_OP_EXPAND) && (rwidth > 0) && (lwidth > 0) && (left->value->value != NULL) ) {

    assert( rwidth < 1024 );
    assert( lwidth < 1024 );

    /*
     If the left-hand expression is a known value, go ahead and create the value here; otherwise,
     hold off because our vector value will be coming.
    */
    if( !vector_is_unknown( left->value ) ) {
      expression_create_value( new_expr, (vector_to_int( left->value ) * rwidth), data );
    } else {
      expression_create_value( new_expr, 1, data );
    }

  } else if( (op == EXP_OP_LT   ) ||
             (op == EXP_OP_GT   ) ||
             (op == EXP_OP_EQ   ) ||
             (op == EXP_OP_CEQ  ) ||
             (op == EXP_OP_LE   ) ||
             (op == EXP_OP_GE   ) ||
             (op == EXP_OP_NE   ) ||
             (op == EXP_OP_CNE  ) ||
             (op == EXP_OP_LOR  ) ||
             (op == EXP_OP_LAND ) ||
             (op == EXP_OP_UAND ) ||
             (op == EXP_OP_UNOT ) ||
             (op == EXP_OP_UOR  ) ||
             (op == EXP_OP_UXOR ) ||
             (op == EXP_OP_UNAND) ||
             (op == EXP_OP_UNOR ) ||
             (op == EXP_OP_UNXOR) ||
             (op == EXP_OP_EOR)   ||
             (op == EXP_OP_NEDGE) ||
             (op == EXP_OP_PEDGE) ||
             (op == EXP_OP_AEDGE) ||
             (op == EXP_OP_CASE)  ||
             (op == EXP_OP_CASEX) ||
             (op == EXP_OP_CASEZ) ||
             (op == EXP_OP_DEFAULT) ) {

    /* If this expression will evaluate to a single bit, create vector now */
    expression_create_value( new_expr, 1, data );

  } else {

    /* If both right and left values have their width values set. */
    if( (rwidth > 0) && (lwidth > 0) && 
        (op != EXP_OP_MBIT_SEL) &&
        (op != EXP_OP_PARAM_MBIT) ) {

      if( rwidth >= lwidth ) {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( rwidth < 1024 );
        expression_create_value( new_expr, rwidth, data );
      } else {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( lwidth < 1024 );
        expression_create_value( new_expr, lwidth, data );
      }

    } else {
 
      expression_create_value( new_expr, 0, FALSE );
 
    }

  }

  if( data == FALSE ) {
    assert( new_expr->value->value == NULL );
  }

  return( new_expr );

}

/*!
 \param exp  Pointer to expression to set value to.
 \param vec  Pointer to vector value to set expression to.
 
 Sets the specified expression (if necessary) to the value of the
 specified vector value.
*/
void expression_set_value( expression* exp, vector* vec ) {
  
  int lbit;  /* Bit boundary specified by left child  */
  int rbit;  /* Bit boundary specified by right child */
  
  assert( exp != NULL );
  assert( exp->value != NULL );
  assert( vec != NULL );
  
  /* printf( "In expression_set_value, expr: %d, op: %d, line: %d\n", exp->id, exp->op, exp->line ); */
  
  switch( exp->op ) {
    case EXP_OP_SIG   :
    case EXP_OP_PARAM :
      exp->value->value = vec->value;
      exp->value->width = vec->width;
      break;
    case EXP_OP_SBIT_SEL   :
    case EXP_OP_PARAM_SBIT :
      exp->value->value = vec->value;
      exp->value->width = 1;
      break;
    case EXP_OP_MBIT_SEL   :
    case EXP_OP_PARAM_MBIT :
      expression_operate_recursively( exp->left  );
      expression_operate_recursively( exp->right );
      lbit = vector_to_int( exp->left->value  );
      rbit = vector_to_int( exp->right->value );
      if( lbit <= rbit ) {
        exp->value->width = ((rbit - lbit) + 1);
        if( exp->op == EXP_OP_PARAM_MBIT ) {
          exp->value->value = vec->value + lbit;
        } else {
          exp->value->value = vec->value + (lbit - exp->sig->lsb);
        }
      } else {
        exp->value->width = ((lbit - rbit) + 1);
        if( exp->op == EXP_OP_PARAM_MBIT ) {
          exp->value->value = vec->value + rbit;
        } else {
          exp->value->value = vec->value + (rbit - exp->sig->lsb);
        }
      }
      break;
    default :  break;
  }

}

/*!
 \param expr       Pointer to expression to potentially resize.
 \param recursive  Specifies if we should perform a recursive depth-first resize

 Resizes the given expression depending on the expression operation and its
 children's sizes.  If recursive is TRUE, performs the resize in a depth-first
 fashion, resizing the children before resizing the current expression.  If
 recursive is FALSE, only the given expression is evaluated and resized.
*/
void expression_resize( expression* expr, bool recursive ) {

  int  largest_width;  /* Holds larger width of left and right children */

  if( expr != NULL ) {

    if( recursive ) {
      expression_resize( expr->left, recursive );
      expression_resize( expr->right, recursive );
    }

    switch( expr->op ) {

      /* These operations will already be sized so nothing to do here */
      case EXP_OP_STATIC     :
      case EXP_OP_PARAM      :
      case EXP_OP_PARAM_SBIT :
      case EXP_OP_PARAM_MBIT :
      case EXP_OP_SIG :
      case EXP_OP_SBIT_SEL :
      case EXP_OP_MBIT_SEL :
      case EXP_OP_ASSIGN :
      case EXP_OP_BASSIGN :
      case EXP_OP_NASSIGN :
      case EXP_OP_IF :
        break;

      /* These operations should always be set to a width 1 */
      case EXP_OP_LT      :
      case EXP_OP_GT      :
      case EXP_OP_EQ      :
      case EXP_OP_CEQ     :
      case EXP_OP_LE      :
      case EXP_OP_GE      :
      case EXP_OP_NE      :
      case EXP_OP_CNE     :
      case EXP_OP_LOR     :
      case EXP_OP_LAND    :
      case EXP_OP_UAND    :
      case EXP_OP_UNOT    :
      case EXP_OP_UOR     :
      case EXP_OP_UXOR    :
      case EXP_OP_UNAND   :
      case EXP_OP_UNOR    :
      case EXP_OP_UNXOR   :
      case EXP_OP_EOR     :
      case EXP_OP_NEDGE   :
      case EXP_OP_PEDGE   :
      case EXP_OP_CASE    :
      case EXP_OP_CASEX   :
      case EXP_OP_CASEZ   :
      case EXP_OP_DEFAULT :
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, FALSE );
        }
        break;

      case EXP_OP_LAST    :
        if( (expr->value->width != 2) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 2, FALSE );
        }
        break;

      /*
       In the case of an AEDGE expression, it needs to have the size of its LAST child expression
       to be the width of its right child + one bit for an armed value.
      */
      case EXP_OP_AEDGE :
        if( (expr->left->value->width != expr->right->value->width) || (expr->left->value->value == NULL) ) {
          assert( expr->left->value->value == NULL );
          expression_create_value( expr->left, (expr->right->value->width + 1), FALSE );
        }
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, FALSE );
        }
        break;

      /*
       In the case of an EXPAND, we need to set the width to be the product of the value of
       the left child and the bit-width of the right child.
      */
      case EXP_OP_EXPAND :
        expression_operate_recursively( expr->left );
        if( (expr->value->width != (vector_to_int( expr->left->value ) * expr->right->value->width)) ||
            (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, (vector_to_int( expr->left->value ) * expr->right->value->width), FALSE );
        }
        break;

      /* 
       In the case of a MULTIPLY or LIST (for concatenation) operation, its expression width must be the sum of its
       children's width.  Remove the current vector and replace it with the appropriately
       sized vector.
      */
      case EXP_OP_MULTIPLY :
      case EXP_OP_LIST :
        if( (expr->value->width != (expr->left->value->width + expr->right->value->width)) ||
            (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, (expr->left->value->width + expr->right->value->width), FALSE );
        }
        break;

      default :
        if( (expr->left != NULL) && ((expr->right == NULL) || (expr->left->value->width > expr->right->value->width)) ) {
          largest_width = expr->left->value->width;
        } else if( expr->right != NULL ) {
          largest_width = expr->right->value->width;
        } else {
          largest_width = 1;
        }
        if( (expr->value->width != largest_width) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, largest_width, FALSE );
        }
        break;

    }

  }

}

/*!
 \param expr  Pointer to expression to get ID from.
 \return Returns expression ID for this expression.

 If specified expression is non-NULL, return expression ID of this
 expression; otherwise, return a value of 0 to indicate that this
 is a leaf node.
*/
int expression_get_id( expression* expr ) {

  if( expr == NULL ) {
    return( 0 );
  } else {
    return( expr->id );
  }

}

/*!
 \param expr  Pointer to root expression to extract last line from

 \return Returns the line number of the last line in this expression. 
*/
expression* expression_get_last_line_expr( expression* expr ) {

  expression* last = NULL;

  if( expr != NULL ) {

    last = expression_get_last_line_expr( expr->right );
    if( (last == NULL) || (last->line < expr->line) ) {
      last = expr;
    }
      
  }

  return( last );

}

/*!
 \param expr  Pointer to expression tree to parse.
 \param head  Pointer to head of signal list to populate.
 \param tail  Pointer to tail of signal list to populate.

 Recursively parses specified expression list in search of signals within the
 expression.  When a signal is found, it is added the the signal list specified
 by head and tail.  This function is called by the expression_get_wait_sig_list
 function.
*/
void expression_get_wait_sig_list_helper( expression* expr, sig_link** head, sig_link** tail ) {

  if( expr != NULL ) {

    if( expr->op == EXP_OP_SIG ) {
 
      assert( expr->sig != NULL );
      sig_link_add( expr->sig, head, tail );

    } else {

      if( (expr->op == EXP_OP_SBIT_SEL) ||
          (expr->op == EXP_OP_MBIT_SEL) ) {
        assert( expr->sig != NULL );
        sig_link_add( expr->sig, head, tail );
      }

      expression_get_wait_sig_list_helper( expr->right, head, tail );
      expression_get_wait_sig_list_helper( expr->left,  head, tail );

    }

  }

}

/*!
 \param expr  Pointer to expression tree to parse.
 \param head  Pointer to head of signal list to populate.
 \param tail  Pointer to tail of signal list to populate.

 If the specified expression tree has an event operation at the root of the
 expression tree (EOR, PEDGE, NEDGE, AEDGE), send the expression, head and tail
 to the expression_get_wait_sig_list_helper function.
*/
void expression_get_wait_sig_list( expression* expr, sig_link** head, sig_link** tail ) {

  if( (expr != NULL) &&
      ((expr->op == EXP_OP_EOR)   ||
       (expr->op == EXP_OP_PEDGE) ||
       (expr->op == EXP_OP_NEDGE) ||
       (expr->op == EXP_OP_AEDGE)) ) {

    expression_get_wait_sig_list_helper( expr, head, tail );

  }

}

/*!
 \param expr   Pointer to expression to write to database file.
 \param file   Pointer to database file to write to.

 This function recursively displays the expression information for the specified
 expression tree to the coverage database specified by file.
*/
void expression_db_write( expression* expr, FILE* file ) {

  fprintf( file, "%d %d %d %x %x %x %d %d ",
    DB_TYPE_EXPRESSION,
    expr->id,
    expr->line,
    expr->col,
    expr->op,
    (expr->suppl.all & ESUPPL_MERGE_MASK),
    (expr->op == EXP_OP_STATIC) ? 0 : expression_get_id( expr->right ),
    (expr->op == EXP_OP_STATIC) ? 0 : expression_get_id( expr->left )
  );

  if( (expr->op != EXP_OP_SIG)        &&
      (expr->op != EXP_OP_SBIT_SEL)   &&
      (expr->op != EXP_OP_MBIT_SEL)   &&
      (expr->op != EXP_OP_PARAM)      &&
      (expr->op != EXP_OP_PARAM_SBIT) &&
      (expr->op != EXP_OP_PARAM_MBIT) &&
      (expr->op != EXP_OP_ASSIGN)     &&
      (expr->op != EXP_OP_BASSIGN)    &&
      (expr->op != EXP_OP_NASSIGN)    &&
      (expr->op != EXP_OP_IF)         &&
      ((expr->op == EXP_OP_STATIC) || (ESUPPL_IS_LHS( expr->suppl ) == 0)) ) {
    vector_db_write( expr->value, file, (expr->op == EXP_OP_STATIC) );
  }

  fprintf( file, "\n" );

}

/*!
 \param line      String containing database line to read information from.
 \param curr_mod  Pointer to current module that instantiates this expression.
 \param eval      If TRUE, evaluate expression if children are static.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Reads in the specified expression information, creates new expression from
 heap, populates the expression with specified information from file and 
 returns that value in the specified expression pointer.  If all is 
 successful, returns TRUE; otherwise, returns FALSE.
*/
bool expression_db_read( char** line, module* curr_mod, bool eval ) {

  bool        retval = TRUE;  /* Return value for this function                   */
  int         id;             /* Holder of expression ID                          */
  expression* expr;           /* Pointer to newly created expression              */
  int         linenum;        /* Holder of current line for this expression       */
  int         column;         /* Holder of column alignment information           */
  control     op;             /* Holder of expression operation                   */
  esuppl      suppl;          /* Holder of supplemental value of this expression  */
  int         right_id;       /* Holder of expression ID to the right             */
  int         left_id;        /* Holder of expression ID to the left              */
  expression* right;          /* Pointer to current expression's right expression */
  expression* left;           /* Pointer to current expression's left expression  */
  int         chars_read;     /* Number of characters scanned in from line        */
  vector*     vec;            /* Holders vector value of this expression          */
  expression  texp;           /* Temporary expression link holder for searching   */
  exp_link*   expl;           /* Pointer to found expression in module            */

  if( sscanf( *line, "%d %d %x %x %x %d %d%n", &id, &linenum, &column, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 7 ) {

    *line = *line + chars_read;

    /* Find module instance name */
    if( curr_mod == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  expression (%d) in database written before its module", id );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    } else {

      /* Find right expression */
      texp.id = right_id;
      if( right_id == 0 ) {
        right = NULL;
      } else if( (expl = exp_link_find( &texp, curr_mod->exp_head )) == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, right_id );
  	    print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      } else {
        right = expl->exp;
      }

      /* Find left expression */
      texp.id = left_id;
      if( left_id == 0 ) {
        left = NULL;
      } else if( (expl = exp_link_find( &texp, curr_mod->exp_head )) == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, left_id );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      } else {
        left = expl->exp;
      }

      /* Create new expression */
      expr = expression_create( right, left, op, ESUPPL_IS_LHS( suppl ), id, linenum,
                                ((column >> 16) & 0xffff), (column & 0xffff),
                                ((op != EXP_OP_SIG)        && 
                                 (op != EXP_OP_PARAM)      &&
                                 (op != EXP_OP_SBIT_SEL)   &&
                                 (op != EXP_OP_PARAM_SBIT) &&
                                 (op != EXP_OP_MBIT_SEL)   &&
                                 (op != EXP_OP_PARAM_MBIT) &&
                                 (op != EXP_OP_ASSIGN)     &&
                                 (op != EXP_OP_BASSIGN)    &&
                                 (op != EXP_OP_NASSIGN)    &&
                                 (op != EXP_OP_IF)         &&
                                 ((op == EXP_OP_STATIC) || (ESUPPL_IS_LHS( suppl ) == 0))) );

      expr->suppl.all = suppl.all;

      if( right != NULL ) {
        right->parent->expr = expr;
      }

      /* Don't set left child's parent if the parent is a CASE, CASEX, or CASEZ type expression */
      if( (left != NULL) && 
          (op != EXP_OP_CASE)  &&
          (op != EXP_OP_CASEX) &&
          (op != EXP_OP_CASEZ) ) {
        left->parent->expr = expr;
      }

      if( (op != EXP_OP_SIG)        && 
          (op != EXP_OP_SBIT_SEL)   && 
          (op != EXP_OP_MBIT_SEL)   &&
          (op != EXP_OP_PARAM)      &&
          (op != EXP_OP_PARAM_SBIT) &&
          (op != EXP_OP_PARAM_MBIT) &&
          (op != EXP_OP_ASSIGN)     &&
          (op != EXP_OP_BASSIGN)    &&
          (op != EXP_OP_NASSIGN)    &&
          (op != EXP_OP_IF)         &&
          ((op == EXP_OP_STATIC) || (ESUPPL_IS_LHS( suppl ) == 0)) ) {

        /* Read in vector information */
        if( vector_db_read( &vec, line ) ) {

          /* Copy expression value */
          vector_dealloc( expr->value );
          expr->value = vec;

        } else {

          print_output( "Unable to read vector value for expression", FATAL, __FILE__, __LINE__ );
          retval = FALSE;
 
        }

      }

      /* If we are an assignment operator, set our vector value to that of the right child */
      if( (op == EXP_OP_ASSIGN)  ||
          (op == EXP_OP_BASSIGN) ||
          (op == EXP_OP_NASSIGN) ||
          (op == EXP_OP_IF) ) {

        vector_dealloc( expr->value );
        expr->value = right->value;

      }

      exp_link_add( expr, &(curr_mod->exp_head), &(curr_mod->exp_tail) );

      /*
       If this expression is a constant expression, force the simulator to evaluate
       this expression and all parent expressions of it.
      */
      if( eval && EXPR_IS_STATIC( expr ) && (ESUPPL_IS_LHS( suppl ) == 0) ) {
        exp_link_add( expr, &static_expr_head, &static_expr_tail );
      }
      
    }

  } else {

    print_output( "Unable to read expression value", FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Expression to merge data into.
 \param line  Pointer to CDD line to parse.
 \param same  Specifies if expression to be merged needs to be exactly the same as the existing expression.

 \return Returns TRUE if parse and merge was sucessful; otherwise, returns FALSE.

 Parses specified line for expression information and merges contents into the
 base expression.  If the two expressions given are not the same (IDs, op,
 and/or line position differ) we know that the database files being merged 
 were not created from the same design; therefore, display an error message 
 to the user in this case.  If both expressions are the same, perform the 
 merge.
*/
bool expression_db_merge( expression* base, char** line, bool same ) {

  bool    retval = TRUE;  /* Return value for this function */
  int     id;             /* Expression ID field            */
  int     linenum;        /* Expression line number         */
  int     column;         /* Column information             */
  control op;             /* Expression operation           */
  esuppl  suppl;          /* Supplemental field             */
  int     right_id;       /* ID of right child              */
  int     left_id;        /* ID of left child               */
  int     chars_read;     /* Number of characters read      */

  assert( base != NULL );

  if( sscanf( *line, "%d %d %x %x %x %d %d%n", &id, &linenum, &column, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 7 ) {

    *line = *line + chars_read;

    if( (base->id != id) || (base->op != op) ) {

      print_output( "Attempting to merge databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      /* Merge expression supplemental fields */
      base->suppl.all = (base->suppl.all & ESUPPL_MERGE_MASK) | (suppl.all & ESUPPL_MERGE_MASK);

      if( (op != EXP_OP_SIG)        &&
          (op != EXP_OP_SBIT_SEL)   &&
          (op != EXP_OP_MBIT_SEL)   &&
          (op != EXP_OP_PARAM)      &&
          (op != EXP_OP_PARAM_SBIT) &&
          (op != EXP_OP_PARAM_MBIT) &&
          (op != EXP_OP_ASSIGN)     &&
          (op != EXP_OP_BASSIGN)    &&
          (op != EXP_OP_NASSIGN)    &&
          (op != EXP_OP_IF)         &&
          ((op == EXP_OP_STATIC) || (ESUPPL_IS_LHS( suppl ) == 0)) ) {

        /* Merge expression vectors */
        retval = vector_db_merge( base->value, line, same );

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Expression to replace data with.
 \param line  Pointer to CDD line to parse.

 \return Returns TRUE if parse and merge was sucessful; otherwise, returns FALSE.

 Parses specified line for expression information and replaces contents of the
 original expression with the contents of the newly read expression.  If the two
 expressions given are not the same (IDs, op, and/or line position differ) we
 know that the database files being merged were not created from the same design;
 therefore, display an error message to the user in this case.  If both expressions
 are the same, perform the replacement.
*/
bool expression_db_replace( expression* base, char** line ) {

  bool    retval = TRUE;  /* Return value for this function */
  int     id;             /* Expression ID field            */
  int     linenum;        /* Expression line number         */
  int     column;         /* Column location information    */
  control op;             /* Expression operation           */
  esuppl  suppl;          /* Supplemental field             */
  int     right_id;       /* ID of right child              */
  int     left_id;        /* ID of left child               */
  int     chars_read;     /* Number of characters read      */

  assert( base != NULL );

  if( sscanf( *line, "%d %d %x %x %x %d %d%n", &id, &linenum, &column, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 7 ) {

    *line = *line + chars_read;

    if( (base->id != id) || (base->op != op) ) {

      print_output( "Attempting to replace a database derived from a different design.  Unable to replace",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      /* Merge expression supplemental fields */
      base->suppl.all = suppl.all;

      if( (op != EXP_OP_SIG)        &&
          (op != EXP_OP_SBIT_SEL)   &&
          (op != EXP_OP_MBIT_SEL)   &&
          (op != EXP_OP_PARAM)      &&
          (op != EXP_OP_PARAM_SBIT) &&
          (op != EXP_OP_PARAM_MBIT) &&
          (op != EXP_OP_ASSIGN)     &&
          (op != EXP_OP_BASSIGN)    &&
          (op != EXP_OP_NASSIGN)    &&
          (op != EXP_OP_IF)         &&
          ((op == EXP_OP_STATIC) || (ESUPPL_IS_LHS( suppl ) == 0)) ) {

        /* Merge expression vectors */
        retval = vector_db_replace( base->value, line );

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to display.

 Displays contents of the specified expression to standard output.  This function
 is called by the module_display function.
*/
void expression_display( expression* expr ) {

  int right_id;        /* Value of right expression ID */
  int left_id;         /* Value of left expression ID  */

  assert( expr != NULL );

  if( expr->left == NULL ) {
    left_id = 0;
  } else {
    left_id = expr->left->id;
  }

  if( expr->right == NULL ) {
    right_id = 0;
  } else {
    right_id = expr->right->id;
  }

  printf( "  Expression =>  id: %d, line: %d, col: %x, suppl: %x, width: %d, left: %d, right: %d\n", 
          expr->id,
          expr->line,
	  expr->col,
          expr->suppl.all,
          expr->value->width,
          left_id, 
          right_id );

}

/*!
 \param expr   Pointer to expression to set value to.

 \return Returns TRUE if the assigned expression value was different than the previously stored value;
         otherwise, returns FALSE.

 Performs expression operation.  This function must only be run after its
 left and right expressions have been calculated during this clock period.
 Sets the value of the operation in its own vector value and updates the
 suppl nibble as necessary.
*/
bool expression_operate( expression* expr ) {

  bool     retval = TRUE;   /* Return value for this function                   */
  vector   vec1;            /* Used for logical reduction                       */ 
  vector   vec2;            /* Used for logical reduction                       */
  vector*  vec;             /* Pointer to vector of unknown size                */
  int      i;               /* Loop iterator                                    */
  int      j;               /* Loop iterator                                    */
  vec_data bit;             /* Bit holder for some ops                          */
  int      intval1;         /* Temporary integer value for *, /, %              */
  int      intval2;         /* Temporary integer value for *, /, %              */
  vec_data value1a;         /* 1-bit nibble value                               */
  vec_data value1b;         /* 1-bit nibble value                               */
  vec_data value32[32];     /* 32-bit nibble value                              */
  control  lf, lt, rf, rt;  /* Specify left and right WAS_TRUE/WAS_FALSE values */

  if( (expr != NULL) && (expr->suppl.part.lhs == 0) ) {

    snprintf( user_msg, USER_MSG_LENGTH, "In expression_operate, id: %d, op: %d, line: %d", expr->id, expr->op, expr->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    assert( expr->value != NULL );
    assert( ESUPPL_IS_LHS( expr->suppl ) == 0 );

    bit.all     = 0;
    value1a.all = 0;
    value1b.all = 0;

    switch( expr->op ) {

      case EXP_OP_XOR :
        retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, xor_optab );
        break;

      case EXP_OP_MULTIPLY :
        retval = vector_op_multiply( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_DIVIDE :
        if( vector_is_unknown( expr->left->value ) || vector_is_unknown( expr->right->value ) ) {
          bit.part.value = 0x2;
          for( i=0; i<expr->value->width; i++ ) {
            retval |= vector_set_value( expr->value, &bit, 1, 0, i );
          }
        } else {
          vector_init( &vec1, value32, 32 );
          intval1 = vector_to_int( expr->left->value );
          intval2 = vector_to_int( expr->right->value );
          if( intval2 == 0 ) {
            print_output( "Division by 0 error", FATAL, __FILE__, __LINE__ );
            exit( 1 );
          }
          intval1 = intval1 / intval2;
          vector_from_int( &vec1, intval1 );
          retval = vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );
        }
        break;

      case EXP_OP_MOD :
        if( vector_is_unknown( expr->left->value ) || vector_is_unknown( expr->right->value ) ) {
          bit.part.value = 0x2;
          for( i=0; i<expr->value->width; i++ ) {
            retval |= vector_set_value( expr->value, &bit, 1, 0, i );
          }
        } else {
          vector_init( &vec1, value32, 32 );
          intval1 = vector_to_int( expr->left->value );
          intval2 = vector_to_int( expr->right->value );
          if( intval2 == 0 ) {
            print_output( "Division by 0 error", FATAL, __FILE__, __LINE__ );
            exit( 1 );
          }
          intval1 = intval1 % intval2;
          vector_from_int( &vec1, intval1 );
          retval = vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );
        }
        break;
 
      case EXP_OP_ADD :
        retval = vector_op_add( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_SUBTRACT :
        retval = vector_op_subtract( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_AND :
        retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, and_optab );
        break;

      case EXP_OP_OR :
        retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, or_optab );
        break;

      case EXP_OP_NAND :
        retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nand_optab );
        break;

      case EXP_OP_NOR :
        retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nor_optab );
        break;

      case EXP_OP_NXOR :
        retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nxor_optab );
        break;

      case EXP_OP_LT :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LT );
        break;

      case EXP_OP_GT :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GT );
        break;

      case EXP_OP_LSHIFT :
        retval = vector_op_lshift( expr->value, expr->left->value, expr->right->value );
        break;
 
      case EXP_OP_RSHIFT :
        retval = vector_op_rshift( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_EQ :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_EQ );
        break;

      case EXP_OP_CEQ :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ );
        break;

      case EXP_OP_LE :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LE );
        break;

      case EXP_OP_GE :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GE );
        break;
 
      case EXP_OP_NE :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_NE );
        break;

      case EXP_OP_CNE :
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CNE );
        break;

      case EXP_OP_LOR :
        vector_init( &vec1, &value1a, 1 );
        vector_init( &vec2, &value1b, 1 );
        vector_unary_op( &vec1, expr->left->value,  or_optab );
        vector_unary_op( &vec2, expr->right->value, or_optab );
        retval = vector_bitwise_op( expr->value, &vec1, &vec2, or_optab );
        break;

      case EXP_OP_LAND :
        vector_init( &vec1, &value1a, 1 );
        vector_init( &vec2, &value1b, 1 );
        vector_unary_op( &vec1, expr->left->value,  or_optab );
        vector_unary_op( &vec2, expr->right->value, or_optab );
        retval = vector_bitwise_op( expr->value, &vec1, &vec2, and_optab );
        break;

      case EXP_OP_COND :
        /* Simple vector copy from right side */
        retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        break;

      case EXP_OP_COND_SEL :
        vector_init( &vec1, &value1a, 1 );
        vector_unary_op( &vec1, expr->parent->expr->left->value, or_optab );
        if( vec1.value[0].part.value == 0 ) {
          retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        } else if( vec1.value[0].part.value == 1 ) {
          retval = vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );
        } else {
          vec = vector_create( expr->value->width, TRUE );
          for( i=0; i<vec->width; i++ ) {
            vec->value[i].part.value = 2;
          }
          retval = vector_set_value( expr->value, vec->value, vec->width, 0, 0 );
          vector_dealloc( vec );
        }
        break;

      case EXP_OP_UINV :
        retval = vector_unary_inv( expr->value, expr->right->value );
        break;

      case EXP_OP_UAND :
        retval = vector_unary_op( expr->value, expr->right->value, and_optab );
        break;

      case EXP_OP_UNOT :
        retval = vector_unary_not( expr->value, expr->right->value );
        break;

      case EXP_OP_UOR :
        retval = vector_unary_op( expr->value, expr->right->value, or_optab );
        break;
 
      case EXP_OP_UXOR :
        retval = vector_unary_op( expr->value, expr->right->value, xor_optab );
        break;

      case EXP_OP_UNAND :
        retval = vector_unary_op( expr->value, expr->right->value, nand_optab );
        break;

      case EXP_OP_UNOR :
        retval = vector_unary_op( expr->value, expr->right->value, nor_optab );
        break;

      case EXP_OP_UNXOR :
        retval = vector_unary_op( expr->value, expr->right->value, nxor_optab );
        break;

      case EXP_OP_STATIC :
      case EXP_OP_SIG    :
      case EXP_OP_PARAM  :
      case EXP_OP_LAST   :
        break;

      case EXP_OP_SBIT_SEL   :
      case EXP_OP_PARAM_SBIT :
        if( !vector_is_unknown( expr->left->value ) ) {
          intval1 = vector_to_int( expr->left->value ) - expr->sig->lsb;
          assert( intval1 >= 0 );
          assert( intval1 < expr->sig->value->width );
          expr->value->value = expr->sig->value->value + intval1;
        }
        break;

      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
          break;

      case EXP_OP_EXPAND :
        if( vector_is_unknown( expr->left->value ) ) {
          bit.part.value = 0x2;
          for( i=0; i<expr->value->width; i++ ) {
            retval |= vector_set_value( expr->value, &bit, 1, 0, i );
          }
        } else {
          for( j=0; j<expr->right->value->width; j++ ) {
            bit.part.value = expr->right->value->value[j].part.value;
            for( i=0; i<vector_to_int( expr->left->value ); i++ ) {
              retval |= vector_set_value( expr->value, &bit, 1, 0, ((j * expr->right->value->width) + i) );
            }
          }
        }
        break;

      case EXP_OP_LIST :
        retval |= vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        retval |= vector_set_value( expr->value, expr->left->value->value,  expr->left->value->width,  0, expr->right->value->width );
        break;

      case EXP_OP_CONCAT :
        retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        break;

      case EXP_OP_PEDGE :
        value1a.part.value = expr->right->value->value[0].part.value;
        value1b.all        = expr->left->value->value[0].all;
        /* If the event has been armed previously, evaluate */
        if( (value1b.part.misc == 1) && (value1a.part.value != expr->left->value->value[0].part.value) && (value1a.part.value == 1) ) {
          bit.part.value = 1;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          /* Clear armed bit */
          value1a.part.misc = 0;
        } else {
          bit.part.value = 0;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          /* Set armed bit */
          value1a.part.misc = 1;
        }
        /* Set left LAST value to current value of right */
        expr->left->value->value[0].all = value1a.all;
        break;
 
      case EXP_OP_NEDGE :
        value1a.part.value = expr->right->value->value[0].part.value;
        value1b.all        = expr->left->value->value[0].all;
        if( (value1b.part.misc == 1) && (value1a.part.value != expr->left->value->value[0].part.value) && (value1a.part.value == 0) ) {
          bit.part.value = 1;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          /* Clear armed bit */
          value1a.part.misc = 0;
        } else {
          bit.part.value = 0;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          /* Set armed bit */
          value1a.part.misc = 1;
        }
        /* Set left LAST value to current value of right */
        expr->left->value->value[0].all = value1a.all;
        break;

      case EXP_OP_AEDGE :
        vector_init( &vec1, &value1a, 1 );
        vector_op_compare( &vec1, expr->left->value, expr->right->value, COMP_CEQ );
        value1b.all = expr->left->value->value[0].all;
        /* Set left LAST value to current value of right */
        vector_set_value( expr->left->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        if( (value1b.part.misc == 1) && (vector_to_int( &vec1 ) == 0) ) {
          bit.part.value = 1;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          /* Clear armed bit */
          expr->left->value->value[0].part.misc = 0;
        } else {
          bit.part.value = 0;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          /* Set armed bit */
          expr->left->value->value[0].part.misc = 1;
        }
        break;

      case EXP_OP_EOR :
        vector_init( &vec1, &value1a, 1 );
        vector_init( &vec2, &value1b, 1 );
        expression_operate( expr->left );
        expression_operate( expr->right );
        vector_unary_op( &vec1, expr->left->value,  or_optab );
        vector_unary_op( &vec2, expr->right->value, or_optab );
        retval = vector_bitwise_op( expr->value, &vec1, &vec2, or_optab );
        break;

      case EXP_OP_DELAY :
        /* If this expression is not currently waiting, set the start time of delay */
        if( vector_to_int( expr->left->value ) == 0xffffffff ) {
          vector_from_int( expr->left->value, curr_sim_time );
        }
        intval1 = vector_to_int( expr->left->value );           /* Start time of delay */
        intval2 = vector_to_int( expr->right->value );          /* Number of clocks to delay */
        if( ((intval1 + intval2) <= curr_sim_time) || ((curr_sim_time == -1) && (intval1 != 0xffffffff)) ) {
          bit.part.value = 1;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
          vector_from_int( expr->left->value, 0xffffffff );
        } else {
          bit.part.value = 0;
          retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        break;

      case EXP_OP_CASE :
        assert( expr->left != NULL );
        assert( expr->right != NULL );
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ );
        break;

      case EXP_OP_CASEX :
        assert( expr->left != NULL );
        assert( expr->right != NULL );
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CXEQ );
        break;

      case EXP_OP_CASEZ :
        assert( expr->left != NULL );
        assert( expr->right != NULL );
        retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CZEQ );
        break;

      case EXP_OP_DEFAULT :
        bit.part.value = 1;
        retval = vector_set_value( expr->value, &bit, 1, 0, 0 );
        break;

      case EXP_OP_BASSIGN :
#ifdef PERFORM_ASSIGNMENT
        intval1 = 0;
        expression_assign( expr->left, expr->right, &intval1 );
#endif
        break;

      case EXP_OP_ASSIGN :
      case EXP_OP_NASSIGN :
      case EXP_OP_IF :
        break;

      default :
        print_output( "Internal error:  Unidentified expression operation!", FATAL, __FILE__, __LINE__ );
        exit( 1 );
        break;

    }
    
    /* If we have a new value, recalculate TRUE/FALSE indicators */
    if( retval ) {

      /* Clear current TRUE/FALSE indicators */
      if( (expr->op != EXP_OP_STATIC) && (expr->op != EXP_OP_PARAM ) ) {
        expr->suppl.part.eval_t = 0;
        expr->suppl.part.eval_f = 0;
      }
    
      /* Set TRUE/FALSE bits to indicate value */
      vector_init( &vec1, &value1a, 1 );
      vector_unary_op( &vec1, expr->value, or_optab );
      switch( vec1.value[0].part.value ) {
        case 0 :  expr->suppl.part.false = 1;  expr->suppl.part.eval_f = 1;  break;
        case 1 :  expr->suppl.part.true  = 1;  expr->suppl.part.eval_t = 1;  break;
        default:  break;
      }

    }

    /* Set EVAL00, EVAL01, EVAL10 or EVAL11 bits based on current value of children */
    if( (expr->left != NULL) && (expr->right != NULL) ) {
      lf = ESUPPL_IS_FALSE( expr->left->suppl  );
      lt = ESUPPL_IS_TRUE(  expr->left->suppl  );
      rf = ESUPPL_IS_FALSE( expr->right->suppl );
      rt = ESUPPL_IS_TRUE(  expr->right->suppl );
      /* printf( "expr %d, lf: %d, lt: %d, rf: %d, rt: %d\n", expr->id, lf, lt, rf, rt ); */
      expr->suppl.part.eval_00 |= lf & rf;
      expr->suppl.part.eval_01 |= lf & rt;
      expr->suppl.part.eval_10 |= lt & rf;
      expr->suppl.part.eval_11 |= lt & rt;
    }

    /* If this expression is attached to an FSM, perform the FSM calculation now */
    if( expr->table != NULL ) {
      fsm_table_set( expr->table );
      /* If from_state was not specified, we need to copy the current contents of to_state to from_state */
      if( expr->table->from_state->id == expr->id ) {
        vector_dealloc( expr->table->from_state->value );
        vector_copy( expr->value, &(expr->table->from_state->value) );
      }
    }

  }

  return( retval );

}

/*!
 \param expr  Pointer to top of expression tree to perform recursive operations.
 
 Recursively performs the proper operations to cause the top-level expression to
 be set to a value.  This function is called during the parse stage to derive 
 pre-CDD widths of multi-bit expressions.  Each MSB/LSB is an expression tree that 
 needs to be evaluated to set the width properly on the MBIT_SEL expression.
*/
void expression_operate_recursively( expression* expr ) {
    
  if( (expr != NULL) && (expr->suppl.part.lhs == 0) ) {
    
    /*
     Non-static expression found where static expression required.  Simulator
     should catch this error before us, so no user error (too much work to find
     expression in module expression list for now.
    */
    assert( (expr->op != EXP_OP_SIG)      &&
            (expr->op != EXP_OP_SBIT_SEL) &&
            (expr->op != EXP_OP_MBIT_SEL) );

    /* Evaluate children */
    expression_operate_recursively( expr->left  );
    expression_operate_recursively( expr->right );
    
    /* Resize current expression only */
    expression_resize( expr, FALSE );
    
    /* Create vector value to store operation information */
    if( expr->value->value == NULL ) {
      expression_create_value( expr, expr->value->width, TRUE );
    }
    
    /* Perform operation */
    expression_operate( expr );
    
  }
  
}

/*!
 \param expr  Pointer to expression to evaluate.

 \return Returns the value of the expression after being compressed to 1 bit via
         a unary OR.

 Returns a value of 1 if the specified expression contains at least one 1 value
 and no X or Z values in its bits.  It accomplishes this by performing a unary 
 OR operation on the specified expression value and testing bit 0 of the result.
*/
int expression_bit_value( expression* expr ) {

  vector   result;  /* Vector containing result of expression tree */
  vec_data data;    /* Data for result vector                      */

  /* Evaluate the value of the root expression and return this value */
  vector_init( &result, &data, 1 );
  vector_unary_op( &result, expr->value, or_optab );

  return( data.part.value );

}

/*!
 \param expr  Pointer to expression to evaluate.
 
 \return Returns TRUE if expression contains only static expressions; otherwise, returns FALSE.
 
 Recursively iterates through specified expression tree and returns TRUE if all of
 the children expressions are static expressions (STATIC or parameters).
*/
bool expression_is_static_only( expression* expr ) {

  if( expr != NULL ) {

    if( (EXPR_IS_STATIC( expr ) == 1) || (ESUPPL_IS_LHS( expr->suppl ) == 1) ) {
      return( TRUE );
    } else {
      return( (expr->op != EXP_OP_MBIT_SEL)           &&
              expression_is_static_only( expr->left ) &&
              expression_is_static_only( expr->right ) );
    }

  } else {

    return( FALSE );

  }

}

/*!
 \param expr  Pointer to current expression to evaluate

 Recursively traverses expression tree specified by expr and sets the assigned vector supplemental
 field attribute to true for all signal (including single and multi-bit expressions).  This assumes
 that the expression tree is the LHS of a blocking assignment operator (which is called in db.c)
*/
void expression_set_assigned( expression* expr ) {

  if( expr != NULL ) {

    if( (expr->op == EXP_OP_SIG) ||
        (expr->op == EXP_OP_SBIT_SEL) ||
	(expr->op == EXP_OP_MBIT_SEL) ) { 

      /* printf( "Setting assigned for signal %s\n", expr->sig->name ); */
      expr->sig->value->suppl.part.assigned = 1;

    } else {

      expression_set_assigned( expr->left  );
      expression_set_assigned( expr->right );

    }

  }

}

/*!
 \param lhs  Pointer to current expression on left-hand-side of assignment to calculate for.
 \param rhs  Pointer to the right-hand-expression that will be assigned from.
 \param lsb  Current least-significant bit in rhs value to start assigning.

 Recursively iterates through specified LHS expression, assigning the value from the RHS expression.
 This is called whenever a blocking assignment expression is found during simulation.
*/
void expression_assign( expression* lhs, expression* rhs, int* lsb ) {

  int intval1;  /* Integer value to use */

  if( lhs != NULL ) {

    snprintf( user_msg, USER_MSG_LENGTH, "In expression_assign, lhs_op: %d, rhs_op: %d, lsb: %d", lhs->op, rhs->op, *lsb );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

    switch( lhs->op ) {
      case EXP_OP_SIG      :
        vector_set_value( lhs->value, rhs->value->value, lhs->value->width, *lsb, 0 );
	vsignal_propagate( lhs );
        *lsb = *lsb + lhs->value->width;
        break;
      case EXP_OP_SBIT_SEL :
        if( !vector_is_unknown( lhs->left->value ) ) {
          intval1 = vector_to_int( lhs->left->value ) - lhs->sig->lsb;
          assert( intval1 >= 0 );
          assert( intval1 < lhs->sig->value->width );
          lhs->value->value = lhs->sig->value->value + intval1;
        }
        vector_set_value( lhs->value, rhs->value->value, 1, *lsb, 0 );
	vsignal_propagate( lhs );
        *lsb = *lsb + lhs->value->width;
        break;
      case EXP_OP_MBIT_SEL :
        vector_set_value( lhs->value, rhs->value->value, lhs->value->width, *lsb, 0 );
	vsignal_propagate( lhs );
        *lsb = *lsb + lhs->value->width;
        break;
      case EXP_OP_CONCAT   :
      case EXP_OP_LIST     :
        break;
      default:
        /* This is an illegal expression to have on the left-hand-side of an expression */
	assert( (lhs->op == EXP_OP_SIG)      ||
	        (lhs->op == EXP_OP_SBIT_SEL) ||
		(lhs->op == EXP_OP_MBIT_SEL) ||
		(lhs->op == EXP_OP_CONCAT)   ||
		(lhs->op == EXP_OP_LIST) );
        break;
    }

    /* Not sure at this point if this code belongs above or below the above switch statement */
    expression_assign( lhs->right, rhs, lsb );
    expression_assign( lhs->left,  rhs, lsb );

  }

}

/*!
 \param expr      Pointer to root expression to deallocate.
 \param exp_only  Removes only the specified expression and not its children.

 Deallocates all heap memory allocated with the malloc routine.
*/
void expression_dealloc( expression* expr, bool exp_only ) {

  int op;     /* Temporary operation holder */

  if( expr != NULL ) {

    op = expr->op;

    if( (op != EXP_OP_SIG       ) && 
        (op != EXP_OP_SBIT_SEL  ) &&
        (op != EXP_OP_MBIT_SEL  ) &&
        (op != EXP_OP_PARAM     ) &&
        (op != EXP_OP_PARAM_SBIT) &&
        (op != EXP_OP_PARAM_MBIT) &&
        (op != EXP_OP_ASSIGN    ) &&
        (op != EXP_OP_BASSIGN   ) &&
        (op != EXP_OP_NASSIGN   ) &&
        (op != EXP_OP_IF        ) &&
        ((ESUPPL_IS_LHS( expr->suppl ) == 0) || (op == EXP_OP_STATIC)) ) {

      /* Free up memory from vector value storage */
      vector_dealloc( expr->value );
      expr->value = NULL;

    } else {

      if( expr->sig == NULL ) {
        bind_remove( expr->id );
      } else {
        exp_link_remove( expr, &(expr->sig->exp_head), &(expr->sig->exp_tail), FALSE );
      }  
 
    }

    free_safe( expr->parent );

    if( !exp_only ) {

      expression_dealloc( expr->right, FALSE );
      expr->right = NULL;

      expression_dealloc( expr->left, FALSE );
      expr->left = NULL;
  
    }

    /* Remove this expression memory */
    free_safe( expr );

  }

}


/* 
 $Log$
 Revision 1.114  2005/02/16 13:44:55  phase1geo
 Adding value propagation function to vsignal.c and adding this propagation to
 BASSIGN expression assignment after the assignment occurs.

 Revision 1.113  2005/02/11 22:50:33  phase1geo
 Fixing bug with removing statement blocks that contain statements that cannot
 currently be handled by Covered correctly.  There was a problem when the bad statement
 was the last statement in the statement block.  Updated regression accordingly.
 Added race condition diagnostics that currently are not in regression due to lack
 of code support for them.  Ifdef'ed out the BASSIGN stuff for this checkin.

 Revision 1.112  2005/02/09 14:12:22  phase1geo
 More code for supporting expression assignments.

 Revision 1.111  2005/02/08 23:18:23  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.110  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.109  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.108  2005/01/07 23:00:09  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.107  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.106  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.105  2004/11/06 14:49:43  phase1geo
 Fixing problem in expression_operate.  This removes some more code from the score command
 to improve run-time efficiency.

 Revision 1.104  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.103  2004/10/22 22:03:31  phase1geo
 More incremental changes to increase score command efficiency.

 Revision 1.102  2004/10/22 21:40:30  phase1geo
 More incremental updates to improve efficiency in score command (though this
 change should not, in and of itself, improve efficiency).

 Revision 1.101  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.100  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.99  2004/07/22 04:43:04  phase1geo
 Finishing code to calculate start and end columns of expressions.  Regression
 has been updated for these changes.  Other various minor changes as well.

 Revision 1.98  2004/04/19 13:42:31  phase1geo
 Forgot to modify replace function for column information.

 Revision 1.97  2004/04/19 04:54:55  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.96  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.95  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.94  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.93  2004/01/25 03:41:48  phase1geo
 Fixes bugs in summary information not matching verbose information.  Also fixes
 bugs where instances were output when no logic was missing, where instance
 children were missing but not output.  Changed code to output summary
 information on a per instance basis (where children instances are not merged
 into parent instance summary information).  Updated regressions as a result.
 Updates to user documentation (though this is not complete at this time).

 Revision 1.92  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.91  2003/11/30 21:50:45  phase1geo
 Modifying line_collect_uncovered function to create array containing all physical
 lines (rather than just uncovered statement starting line values) for more
 accurate line coverage results for the GUI.  Added new long_exp2 diagnostic that
 is used to test this functionality.

 Revision 1.90  2003/11/30 05:46:45  phase1geo
 Adding IF report outputting capability.  Updated always9 diagnostic for these
 changes and updated rest of regression CDD files accordingly.

 Revision 1.89  2003/11/29 06:55:48  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.88  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.87  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.86  2003/10/31 01:38:13  phase1geo
 Adding new expand diagnostics to verify more situations regarding expansion
 operators.  Fixing expression_create to properly handle all situations of
 this operator's use.

 Revision 1.85  2003/10/30 05:05:11  phase1geo
 Partial fix to bug 832730.  This doesn't seem to completely fix the parameter
 case, however.

 Revision 1.84  2003/10/19 04:23:49  phase1geo
 Fixing bug in VCD parser for new Icarus Verilog VCD dumpfile formatting.
 Fixing bug in signal.c for signal merging.  Updates all CDD files to match
 new format.  Added new diagnostics to test advanced FSM state variable
 features.

 Revision 1.83  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.82  2003/10/16 12:27:19  phase1geo
 Fixing bug in arc.c related to non-zero LSBs.

 Revision 1.81  2003/10/14 04:02:44  phase1geo
 Final fixes for new FSM support.  Full regression now passes.  Need to
 add new diagnostics to verify new functionality, but at least all existing
 cases are supported again.

 Revision 1.80  2003/10/11 05:15:08  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.79  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.78  2003/08/10 00:05:16  phase1geo
 Fixing bug with posedge, negedge and anyedge expressions such that these expressions
 must be armed before they are able to be evaluated.  Fixing bug in vector compare function
 to cause compare to occur on smallest vector size (rather than on largest).  Updated regression
 files and added new diagnostics to test event fix.

 Revision 1.77  2003/08/09 22:16:37  phase1geo
 Updates to development documentation for newly added functions from previous
 checkin.

 Revision 1.76  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.75  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.74  2003/02/26 23:00:46  phase1geo
 Fixing bug with single-bit parameter handling (param4.v diagnostic miscompare
 between Linux and Irix OS's).  Updates to testsuite and new diagnostic added
 for additional testing in this area.

 Revision 1.73  2003/02/07 02:28:23  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.72  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.71  2002/12/07 17:46:53  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.70  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.69  2002/12/03 00:04:56  phase1geo
 Fixing bug uncovered by param6.1.v diagnostic.  Full regression now passes.

 Revision 1.68  2002/12/02 06:14:27  phase1geo
 Fixing bug when an MBIT_SEL expression is used in a module that is instantiated
 more than once (assertion error was firing).  Added diagnostic to test suite to
 test that this case passes.

 Revision 1.67  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.66  2002/11/24 14:38:12  phase1geo
 Updating more regression CDD files for bug fixes.  Fixing bugs where combinational
 expressions were counted more than once.  Adding new diagnostics to regression
 suite.

 Revision 1.65  2002/11/23 16:10:46  phase1geo
 Updating changelog and development documentation to include FSM description
 (this is a brainstorm on how to handle FSMs when we get to this point).  Fixed
 bug with code underlining function in handling parameter in reports.  Fixing bugs
 with MBIT/SBIT handling (this is not verified to be completely correct yet).

 Revision 1.64  2002/11/08 00:58:04  phase1geo
 Attempting to fix problem with parameter handling.  Updated some diagnostics
 in test suite.  Other diagnostics to follow.

 Revision 1.63  2002/11/05 22:27:02  phase1geo
 Adding diagnostic to verify usage of parameters in signal sizing expressions.
 Added diagnostic to regression suite.  Fixed bug with sizing of EXPAND
 expressions in expression creation function.

 Revision 1.62  2002/11/05 16:43:55  phase1geo
 Bug fix for expansion expressions where multiplier wasn't being calculated
 before the expand expression was being sized (leads to a segmentation fault).
 Updated CDD file for expand3.v.  Cleaned up missing CDD file for full
 VCS regression run.

 Revision 1.61  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.60  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.59  2002/10/31 23:13:43  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.58  2002/10/31 05:22:36  phase1geo
 Fixing bug with reading in an integer value from the expression line into
 a short integer.  Needed to use the 'h' value in the sscanf function.  Also
 added VCSONLYDIAGS variable to regression Makefile for diagnostics that can
 only run under VCS (not supported by Icarus Verilog).

 Revision 1.57  2002/10/30 06:07:10  phase1geo
 First attempt to handle expression trees/statement trees that contain
 unsupported code.  These are removed completely and not evaluated (because
 we can't guarantee that our results will match the simulator).  Added real1.1.v
 diagnostic that verifies one case of this scenario.  Full regression passes.

 Revision 1.56  2002/10/29 17:25:56  phase1geo
 Fixing segmentation fault in expression resizer for expressions with NULL
 values in right side child expressions.  Also trying fix for log comments.

 Revision 1.55  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.54  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.53  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.52  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.51  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.50  2002/09/25 05:36:08  phase1geo
 Initial version of parameter support is now in place.  Parameters work on a
 basic level.  param1.v tests this basic functionality and param1.cdd contains
 the correct CDD output from handling parameters in this file.  Yeah!

 Revision 1.49  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.48  2002/09/23 01:37:44  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.47  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.46  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.45  2002/07/20 13:58:01  phase1geo
 Fixing bug in EXP_OP_LAST for changes in binding.  Adding correct line numbering
 to lexer (tested).  Added '+' to report outputting for signals larger than 1 bit.
 Added mbit_sel1.v diagnostic to verify some multi-bit functionality.  Full
 regression passes.

 Revision 1.43  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.42  2002/07/18 02:33:23  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.41  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.40  2002/07/16 00:05:31  phase1geo
 Adding support for replication operator (EXPAND).  All expressional support
 should now be available.  Added diagnostics to test replication operator.
 Rewrote binding code to be more efficient with memory use.

 Revision 1.39  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.38  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.37  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.36  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.35  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.34  2002/07/09 03:24:48  phase1geo
 Various fixes for module instantiantion handling.  This now works.  Also
 modified report output for toggle, line and combinational information.
 Regression passes.

 Revision 1.33  2002/07/08 19:02:11  phase1geo
 Adding -i option to properly handle modules specified for coverage that
 are instantiated within a design without needing to parse parent modules.

 Revision 1.32  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.31  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.30  2002/07/05 04:12:46  phase1geo
 Correcting case, casex and casez equality calculation to conform to correct
 equality check for each case type.  Verified that case statements work correctly
 at this point.  Added diagnostics to verify case statements.

 Revision 1.28  2002/07/05 00:10:18  phase1geo
 Adding report support for case statements.  Everything outputs fine; however,
 I want to remove CASE, CASEX and CASEZ expressions from being reported since
 it causes redundant and misleading information to be displayed in the verbose
 reports.  New diagnostics to check CASE expressions have been added and pass.

 Revision 1.27  2002/07/04 23:10:12  phase1geo
 Added proper support for case, casex, and casez statements in score command.
 Report command still incorrect for these statement types.

 Revision 1.26  2002/07/03 21:30:53  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.25  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.24  2002/07/03 03:11:01  phase1geo
 Fixing multiplication handling error.

 Revision 1.23  2002/07/03 00:59:14  phase1geo
 Fixing bug with conditional statements and other "deep" expression trees.

 Revision 1.22  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.21  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.20  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.19  2002/06/29 04:51:31  phase1geo
 Various fixes for bugs found in regression testing.

 Revision 1.18  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.17  2002/06/28 00:40:37  phase1geo
 Cleaning up extraneous output from debugging.

 Revision 1.16  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.15  2002/06/26 22:09:17  phase1geo
 Removing unecessary output and updating regression Makefile.

 Revision 1.14  2002/06/26 04:59:50  phase1geo
 Adding initial support for delays.  Support is not yet complete and is
 currently untested.

 Revision 1.13  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.12  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.11  2002/06/23 21:18:21  phase1geo
 Added appropriate statement support in parser.  All parts should be in place
 and ready to start testing.

 Revision 1.10  2002/06/22 21:08:23  phase1geo
 Added simulation engine and tied it to the db.c file.  Simulation engine is
 currently untested and will remain so until the parser is updated correctly
 for statements.  This will be the next step.

 Revision 1.9  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.8  2002/06/21 05:55:05  phase1geo
 Getting some codes ready for writing simulation engine.  We should be set
 now.

 Revision 1.7  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.6  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

