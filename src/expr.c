/*!
 \file     expr.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 
 \par Expressions
 The following are special expressions that are handled differently than standard
 unary (i.e., ~a) and dual operators (i.e., a & b).  These expressions are documented
 to help remove confusion (my own) about how they are implemented by Covered and
 handled during the parsing and scoring phases of the tool.
 
 \par \sub EXP_OP_SIG
 A signal expression has no left or right child (they are both NULL).  Its vector
 value is a pointer to the signal vector value to which is belongs.  This allows
 the signal expression value to change automatically when the signal value is
 updated.  No further expression operation is necessary to calculate its value.
 
 \par \sub EXP_OP_SBIT_SEL
 A single-bit signal expression has its left child pointed to the expression tree
 that is required to select the bit from the specified signal value.  The left
 child is allowed to change values during simulation.  To verify that the current
 bit select has not exceeded the ranges of the signal, the signal pointer value
 in the expression structure is used to reference the signal.  The LSB and width
 values from the actual signal can then be used to verify that we are still
 within range.  If we are found to be out of range, a value of X must be assigned
 the the SBIT_SEL expression.  The width of an SBIT_SEL is always constant (1).  The
 LSB of the SBIT_SEL is manipulated by the left expression value.
 
 \par \sub EXP_OP_MBIT_SEL
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


extern nibble xor_optab[16];
extern nibble and_optab[16];
extern nibble or_optab[16];
extern nibble nand_optab[16];
extern nibble nor_optab[16];
extern nibble nxor_optab[16];

extern int  curr_sim_time;
extern char user_msg[USER_MSG_LENGTH];


/*!
 \param exp    Pointer to expression to add value to.
 \param width  Width of value to create.
 \param lsb    Least significant value of value field.
 \param data   Specifies if nibble array should be allocated for vector.

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.
*/
void expression_create_value( expression* exp, int width, int lsb, bool data ) {

  nibble* value = NULL;    /* Temporary storage of vector nibble array */

  if( data == TRUE ) {
    value = (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_SIZE( width ) );
  }

  /* Create value */
  vector_init( exp->value, value, width, lsb );

}

/*!
 \param right  Pointer to expression on right.
 \param left   Pointer to expression on left.
 \param op     Operation to perform for this expression.
 \param id     ID for this expression as determined by the parent.
 \param line   Line number this expression is on.
 \param data   Specifies if we should create a nibble array for the vector value.

 \return Returns pointer to newly created expression.

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create( expression* right, expression* left, int op, int id, int line, bool data ) {

  expression* new_expr;    /* Pointer to newly created expression */
  int         rwidth = 0;  /* Bit width of expression on right    */
  int         lwidth = 0;  /* Bit width of expression on left     */

  new_expr = (expression*)malloc_safe( sizeof( expression ) );

  new_expr->suppl        = ((op & 0x7f) << SUPPL_LSB_OP);
  new_expr->id           = id;
  new_expr->line         = line;
  new_expr->sig          = NULL;
  new_expr->parent       = (expr_stmt*)malloc_safe( sizeof( expr_stmt ) );
  new_expr->parent->expr = NULL;
  new_expr->right        = right;
  new_expr->left         = left;
  new_expr->value        = (vector*)malloc_safe( sizeof( vector ) );

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
    expression_create_value( new_expr, (lwidth + rwidth), 0, data );

  } else if( (op == EXP_OP_CONCAT) && (rwidth > 0) ) {

    assert( rwidth < 1024 );
    expression_create_value( new_expr, rwidth, 0, data );

  } else if( (op == EXP_OP_EXPAND) && (rwidth > 0) && (lwidth > 0) ) {

    assert( rwidth < 1024 );
    assert( lwidth < 1024 );
    expression_create_value( new_expr, (vector_to_int( left->value ) * rwidth), 0, data );

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
    expression_create_value( new_expr, 1, 0, data );

  } else {

    /* If both right and left values have their width values set. */
    if( (rwidth > 0) && (lwidth > 0) ) {

      if( rwidth >= lwidth ) {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( rwidth < 1024 );
        expression_create_value( new_expr, rwidth, 0, data );
      } else {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( lwidth < 1024 );
        expression_create_value( new_expr, lwidth, 0, data );
      }

    } else {
 
      expression_create_value( new_expr, 0, 0, FALSE );
 
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
  
  /* printf( "In expression_set_value, exp_id: %d\n", exp->id ); */
  
  switch( SUPPL_OP( exp->suppl ) ) {
    case EXP_OP_SIG   :
    case EXP_OP_PARAM :
      exp->value->value = vec->value;
      exp->value->width = vec->width;
      exp->value->lsb   = 0;
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
        if( SUPPL_OP( exp->suppl ) == EXP_OP_PARAM_MBIT ) {
          exp->value->lsb = lbit;
        } else {
          exp->value->lsb = lbit - exp->sig->value->lsb;
        }
      } else {
        exp->value->width = ((lbit - rbit) + 1);
        if( SUPPL_OP( exp->suppl ) == EXP_OP_PARAM_MBIT ) {
          exp->value->lsb = rbit;
        } else {
          exp->value->lsb = rbit - exp->sig->value->lsb;
        }
      }
      assert( exp->value->width <= vec->width );
      assert( exp->value->value == NULL );
      assert( vec->value != NULL );
      exp->value->value = vec->value;
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
    
    /* printf( "Resizing expression %d, op: %d, presize: %d\n", expr->id, SUPPL_OP( expr->suppl ), expr->value->width ); */

    switch( SUPPL_OP( expr->suppl ) ) {

      /* These operations will already be sized so nothing to do here */
      case EXP_OP_STATIC     :
      case EXP_OP_PARAM      :
      case EXP_OP_PARAM_SBIT :
      case EXP_OP_PARAM_MBIT :
      case EXP_OP_SIG :
      case EXP_OP_SBIT_SEL :
      case EXP_OP_MBIT_SEL :
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
      case EXP_OP_LAST    :
        assert( expr->value->value == NULL );
        expression_create_value( expr, 1, 0, FALSE );
        break;

      /*
       In the case of an AEDGE expression, it needs to have the size of its LAST child expression
       to be the width of its right child.
      */
      case EXP_OP_AEDGE :
        assert( expr->left->value->value == NULL );
        assert( expr->value->value == NULL );
        expression_create_value( expr->left, expr->right->value->width, expr->right->value->lsb, FALSE );
        expression_create_value( expr, 1, 0, FALSE );
        break;

      /*
       In the case of an EXPAND, we need to set the width to be the product of the value of
       the left child and the bit-width of the right child.
      */
      case EXP_OP_EXPAND :
        assert( expr->value->value == NULL );
        expression_create_value( expr, (vector_to_int( expr->left->value ) * expr->right->value->width), 0, FALSE );
        break;

      /* 
       In the case of a MULTIPLY or LIST (for concatenation) operation, its expression width must be the sum of its
       children's width.  Remove the current vector and replace it with the appropriately
       sized vector.
      */
      case EXP_OP_MULTIPLY :
      case EXP_OP_LIST :
        assert( expr->value->value == NULL );
        expression_create_value( expr, (expr->left->value->width + expr->right->value->width), 0, FALSE );
        break;

      default :
        assert( expr->value->value == NULL );
        if( (expr->left != NULL) && ((expr->right == NULL) || (expr->left->value->width > expr->right->value->width)) ) {
          largest_width = expr->left->value->width;
        } else if( expr->right != NULL ) {
          largest_width = expr->right->value->width;
        } else {
          largest_width = 1;
        }
        expression_create_value( expr, largest_width, 0, FALSE );
        break;

    }

    /* printf( "Resized expression %d, op: %d, size: %d\n", expr->id, SUPPL_OP( expr->suppl ), expr->value->width ); */

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
 \param expr   Pointer to expression to write to database file.
 \param file   Pointer to database file to write to.
 \param scope  Name of Verilog hierarchical scope for this expression.

 This function recursively displays the expression information for the specified
 expression tree to the coverage database specified by file.
*/
void expression_db_write( expression* expr, FILE* file, char* scope ) {

  fprintf( file, "%d %d %s %d %x %d %d ",
    DB_TYPE_EXPRESSION,
    expr->id,
    scope,
    expr->line,
    (expr->suppl & 0xffff),
    (SUPPL_OP( expr->suppl ) == EXP_OP_STATIC) ? 0 : expression_get_id( expr->right ),
    (SUPPL_OP( expr->suppl ) == EXP_OP_STATIC) ? 0 : expression_get_id( expr->left )
  );

  if( (SUPPL_OP( expr->suppl ) != EXP_OP_SIG) && 
      (SUPPL_OP( expr->suppl ) != EXP_OP_SBIT_SEL) && 
      (SUPPL_OP( expr->suppl ) != EXP_OP_MBIT_SEL) ) {
    vector_db_write( expr->value, file, (SUPPL_OP( expr->suppl ) == EXP_OP_STATIC) );
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
  control     suppl;          /* Holder of supplemental value of this expression  */
  int         right_id;       /* Holder of expression ID to the right             */
  int         left_id;        /* Holder of expression ID to the left              */
  expression* right;          /* Pointer to current expression's right expression */
  expression* left;           /* Pointer to current expression's left expression  */
  int         chars_read;     /* Number of characters scanned in from line        */
  vector*     vec;            /* Holders vector value of this expression          */
  char        modname[4096];  /* Name of this expression's module instance        */
  expression  texp;           /* Temporary expression link holder for searching   */
  exp_link*   expl;           /* Pointer to found expression in module            */

  if( sscanf( *line, "%d %s %d %hx %d %d%n", &id, modname, &linenum, &suppl, &right_id, &left_id, &chars_read ) == 6 ) {

    *line = *line + chars_read;

    /* Find module instance name */
    if( curr_mod == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  expression (%d) in database written before its module", id );
      print_output( user_msg, FATAL );
      retval = FALSE;

    } else {

      /* Find right expression */
      texp.id = right_id;
      if( right_id == 0 ) {
        right = NULL;
      } else if( (expl = exp_link_find( &texp, curr_mod->exp_head )) == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, right_id );
  	    print_output( user_msg, FATAL );
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
        print_output( user_msg, FATAL );
        exit( 1 );
      } else {
        left = expl->exp;
      }

      /* Create new expression */
      expr = expression_create( right, left, SUPPL_OP( suppl ), id, linenum,
                                ((SUPPL_OP( suppl ) != EXP_OP_SIG)        && 
                                 (SUPPL_OP( suppl ) != EXP_OP_PARAM)      &&
                                 (SUPPL_OP( suppl ) != EXP_OP_SBIT_SEL)   &&
                                 (SUPPL_OP( suppl ) != EXP_OP_PARAM_SBIT) &&
                                 (SUPPL_OP( suppl ) != EXP_OP_MBIT_SEL)   &&
                                 (SUPPL_OP( suppl ) != EXP_OP_PARAM_MBIT)) );
      expr->suppl = suppl;

      if( right != NULL ) {
        right->parent->expr = expr;
      }

      /* Don't set left child's parent if the parent is a CASE, CASEX, or CASEZ type expression */
      if( (left != NULL) && 
          (SUPPL_OP( suppl ) != EXP_OP_CASE) &&
          (SUPPL_OP( suppl ) != EXP_OP_CASEX) &&
          (SUPPL_OP( suppl ) != EXP_OP_CASEZ) ) {
        left->parent->expr = expr;
      }

      if( (SUPPL_OP( suppl ) != EXP_OP_SIG) && 
          (SUPPL_OP( suppl ) != EXP_OP_SBIT_SEL) && 
          (SUPPL_OP( suppl ) != EXP_OP_MBIT_SEL) ) {

        /* Read in vector information */
        if( vector_db_read( &vec, line ) ) {

          /* Copy expression value */
          vector_dealloc( expr->value );
          expr->value = vec;

        } else {

          print_output( "Unable to read vector value for expression", FATAL );
          retval = FALSE;
 
        }

      }

      exp_link_add( expr, &(curr_mod->exp_head), &(curr_mod->exp_tail) );

      /*
       If this expression has any combination of EXP_OP_STATIC or WAS_EXECUTED,
       perform the expression operation now and set WAS_EXECUTED bit of expression's
       supplemental field.
      */
      if( eval && EXPR_EVAL_STATIC( expr ) ) {
        expression_operate( expr );
        expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_EXECUTED);
      }
      
    }

  } else {

    print_output( "Unable to read expression value", FATAL );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Expression to merge data into.
 \param line  Pointer to CDD line to parse.

 \return Returns TRUE if parse and merge was sucessful; otherwise, returns FALSE.

 Parses specified line for expression information and merges contents into the
 base expression.  If the two expressions given are not the same (IDs, op,
 and/or line position differ) we know that the database files being merged 
 were not created from the same design; therefore, display an error message 
 to the user in this case.  If both expressions are the same, perform the 
 merge.
*/
bool expression_db_merge( expression* base, char** line ) {

  bool retval = TRUE;  /* Return value for this function */
  int  id;             /* Expression ID field            */
  char modname[4096];  /* Module name of this expression */
  int  linenum;        /* Expression line number         */
  int  suppl;          /* Supplemental field             */
  int  right_id;       /* ID of right child              */
  int  left_id;        /* ID of left child               */
  int  chars_read;     /* Number of characters read      */

  assert( base != NULL );

  if( sscanf( *line, "%d %s %d %x %d %d%n", &id, modname, &linenum, &suppl, &right_id, &left_id, &chars_read ) == 6 ) {

    *line = *line + chars_read;

    if( (base->id != id) || (SUPPL_OP( base->suppl ) != SUPPL_OP( suppl )) || (base->line != linenum) ) {

      print_output( "Attempting to merge databases derived from different designs.  Unable to merge", FATAL );
      exit( 1 );

    } else {

      /* Merge expression supplemental fields */
      base->suppl = (base->suppl & SUPPL_MERGE_MASK) | (suppl & SUPPL_MERGE_MASK);

      if( (SUPPL_OP( suppl ) != EXP_OP_SIG) &&
          (SUPPL_OP( suppl ) != EXP_OP_SBIT_SEL) &&
          (SUPPL_OP( suppl ) != EXP_OP_MBIT_SEL) ) {

        /* Merge expression vectors */
        retval = vector_db_merge( base->value, line );

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

  printf( "  Expression =>  id: %d, line: %d, suppl: %x, width: %d, left: %d, right: %d\n", 
          expr->id,
          expr->line,
          expr->suppl,
          expr->value->width,
          left_id, 
          right_id );

}

/*!
 \param expr   Pointer to expression to set value to.

 Performs expression operation.  This function must only be run after its
 left and right expressions have been calculated during this clock period.
 Sets the value of the operation in its own vector value and updates the
 suppl nibble as necessary.
*/
void expression_operate( expression* expr ) {

  vector  vec1;                          /* Used for logical reduction          */ 
  vector  vec2;                          /* Used for logical reduction          */
  vector* vec;                           /* Pointer to vector of unknown size   */
  int     i;                             /* Loop iterator                       */
  int     j;                             /* Loop iterator                       */
  nibble  bit;                           /* Bit holder for some ops             */
  int     intval1;                       /* Temporary integer value for *, /, % */
  int     intval2;                       /* Temporary integer value for *, /, % */
  nibble  value1a;                       /* 1-bit nibble value                  */
  nibble  value1b;                       /* 1-bit nibble value                  */
  nibble  value32[ VECTOR_SIZE( 32 ) ];  /* 32-bit nibble value                 */

  if( expr != NULL ) {

    snprintf( user_msg, USER_MSG_LENGTH, "In expression_operate, id: %d, op: %d, line: %d", expr->id, SUPPL_OP( expr->suppl ), expr->line );
    print_output( user_msg, DEBUG );

    assert( expr->value != NULL );

    switch( SUPPL_OP( expr->suppl ) ) {

      case EXP_OP_XOR :
        vector_bitwise_op( expr->value, expr->left->value, expr->right->value, xor_optab );
        break;

      case EXP_OP_MULTIPLY :
        vector_op_multiply( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_DIVIDE :
        vector_init( &vec1, value32, 32, 0 );
        intval1 = vector_to_int( expr->left->value );
        intval2 = vector_to_int( expr->right->value );
        if( intval2 == 0 ) {
          print_output( "Division by 0 error", FATAL );
          exit( 1 );
        }
        intval1 = intval1 / intval2;
        vector_from_int( &vec1, intval1 );
        vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );
        break;

      case EXP_OP_MOD :
        vector_init( &vec1, value32, 32, 0 );
        intval1 = vector_to_int( expr->left->value );
        intval2 = vector_to_int( expr->right->value );
        if( intval2 == 0 ) {
          print_output( "Division by 0 error", FATAL );
          exit( 1 );
        }
        intval1 = intval1 % intval2;
        vector_from_int( &vec1, intval1 );
        vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );
        break;
 
      case EXP_OP_ADD :
        vector_op_add( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_SUBTRACT :
        vector_op_subtract( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_AND :
        vector_bitwise_op( expr->value, expr->left->value, expr->right->value, and_optab );
        break;

      case EXP_OP_OR :
        vector_bitwise_op( expr->value, expr->left->value, expr->right->value, or_optab );
        break;

      case EXP_OP_NAND :
        vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nand_optab );
        break;

      case EXP_OP_NOR :
        vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nor_optab );
        break;

      case EXP_OP_NXOR :
        vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nxor_optab );
        break;

      case EXP_OP_LT :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LT );
        break;

      case EXP_OP_GT :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GT );
        break;

      case EXP_OP_LSHIFT :
        vector_op_lshift( expr->value, expr->left->value, expr->right->value );
        break;
 
      case EXP_OP_RSHIFT :
        vector_op_rshift( expr->value, expr->left->value, expr->right->value );
        break;

      case EXP_OP_EQ :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_EQ );
        break;

      case EXP_OP_CEQ :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ );
        break;

      case EXP_OP_LE :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LE );
        break;

      case EXP_OP_GE :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GE );
        break;
 
      case EXP_OP_NE :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_NE );
        break;

      case EXP_OP_CNE :
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CNE );
        break;

      case EXP_OP_LOR :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_init( &vec2, &value1b, 1, 0 );
        vector_unary_op( &vec1, expr->left->value,  or_optab );
        vector_unary_op( &vec2, expr->right->value, or_optab );
        vector_bitwise_op( expr->value, &vec1, &vec2, or_optab );
        break;

      case EXP_OP_LAND :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_init( &vec2, &value1b, 1, 0 );
        vector_unary_op( &vec1, expr->left->value,  or_optab );
        vector_unary_op( &vec2, expr->right->value, or_optab );
        vector_bitwise_op( expr->value, &vec1, &vec2, and_optab );
        break;

      case EXP_OP_COND :
        /* Simple vector copy from right side */
        vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        break;

      case EXP_OP_COND_SEL :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->parent->expr->left->value, or_optab );
        if( vector_bit_val( vec1.value, 0 ) == 0 ) {
          vector_set_value( expr->value, expr->right->value->value, expr->right->value->width,
                            expr->right->value->lsb, 0 );
        } else if( vector_bit_val( vec1.value, 0 ) == 1 ) {
          vector_set_value( expr->value, expr->left->value->value, expr->left->value->width,
                            expr->left->value->lsb, 0 );
        } else {
          vec = vector_create( expr->value->width, 0, TRUE );
          for( i=0; i<vec->width; i++ ) {
            vector_set_bit( vec->value, 2, i );
          }
          vector_set_value( expr->value, vec->value, vec->width, vec->lsb, 0 );
          vector_dealloc( vec );
        }
        break;

      case EXP_OP_UINV :
        vector_unary_inv( expr->value, expr->right->value );
        break;

      case EXP_OP_UAND :
        vector_unary_op( expr->value, expr->right->value, and_optab );
        break;

      case EXP_OP_UNOT :
        vector_unary_not( expr->value, expr->right->value );
        break;

      case EXP_OP_UOR :
        vector_unary_op( expr->value, expr->right->value, or_optab );
        break;
 
      case EXP_OP_UXOR :
        vector_unary_op( expr->value, expr->right->value, xor_optab );
        break;

      case EXP_OP_UNAND :
        vector_unary_op( expr->value, expr->right->value, nand_optab );
        break;

      case EXP_OP_UNOR :
        vector_unary_op( expr->value, expr->right->value, nor_optab );
        break;

      case EXP_OP_UNXOR :
        vector_unary_op( expr->value, expr->right->value, nxor_optab );
        break;

      case EXP_OP_STATIC :
      case EXP_OP_SIG    :
      case EXP_OP_PARAM  :
      case EXP_OP_LAST   :
        break;

      case EXP_OP_SBIT_SEL   :
      case EXP_OP_PARAM_SBIT :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->left->value, or_optab );
        if( (vec1.value[0] & 0x3) != 2 ) {
          expr->value->lsb = vector_to_int( expr->left->value ) - expr->sig->value->lsb;
          if( expr->value->lsb >= expr->sig->value->width ) {
            expr->value->lsb = -1;
          }
        } else {
          expr->value->lsb = -1;
        }
        break;

      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
          break;

      case EXP_OP_EXPAND :
        for( j=0; j<expr->right->value->width; j++ ) {
          bit = vector_bit_val( expr->right->value->value, j );
          for( i=0; i<vector_to_int( expr->left->value ); i++ ) {
            vector_set_value( expr->value, &bit, 1, 0, ((j * expr->right->value->width) + i) );
          }
        }
        break;

      case EXP_OP_LIST :
        vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, expr->right->value->lsb, 0 );
        vector_set_value( expr->value, expr->left->value->value,  expr->left->value->width,  expr->left->value->lsb,  expr->right->value->width );
        break;

      case EXP_OP_CONCAT :
        vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        break;

      case EXP_OP_PEDGE :
        value1a = vector_bit_val( expr->right->value->value, 0 );
        value1b = vector_bit_val( expr->left->value->value,  0 );
        if( (value1a != value1b) && (value1a == 1) ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        /* Set left LAST value to current value of right */
        vector_set_value( expr->left->value, &value1a, 1, 0, 0 );
        break;
 
      case EXP_OP_NEDGE :
        value1a = vector_bit_val( expr->right->value->value, 0 );
        value1b = vector_bit_val( expr->left->value->value,  0 );
        if( (value1a != value1b) && (value1a == 0) ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
       } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        /* Set left LAST value to current value of right */
        vector_set_value( expr->left->value, &value1a, 1, 0, 0 );
        break;

      case EXP_OP_AEDGE :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_op_compare( &vec1, expr->left->value, expr->right->value, COMP_CEQ );
        if( vector_to_int( &vec1 ) == 0 ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        /* Set left LAST value to current value of right */
        vector_set_value( expr->left->value, expr->right->value->value, expr->right->value->width, expr->right->value->lsb, 0 );
        break;

      case EXP_OP_EOR :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_init( &vec2, &value1b, 1, 0 );
        expression_operate( expr->left );
        expression_operate( expr->right );
        vector_unary_op( &vec1, expr->left->value,  or_optab );
        vector_unary_op( &vec2, expr->right->value, or_optab );
        vector_bitwise_op( expr->value, &vec1, &vec2, or_optab );
        break;

      case EXP_OP_DELAY :
        /* If this expression is not currently waiting, set the start time of delay */
        if( vector_to_int( expr->left->value ) == 0xffffffff ) {
          vector_from_int( expr->left->value, curr_sim_time );
        }
        intval1 = vector_to_int( expr->left->value );           /* Start time of delay */
        intval2 = vector_to_int( expr->right->value );          /* Number of clocks to delay */
        if( ((intval1 + intval2) <= curr_sim_time) || ((curr_sim_time == -1) && (vector_to_int( expr->left->value ) != 0xffffffff)) ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
          vector_from_int( expr->left->value, 0xffffffff );
        } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        break;

      case EXP_OP_CASE :
        assert( expr->left != NULL );
        assert( expr->right != NULL );
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ );
        break;

      case EXP_OP_CASEX :
        assert( expr->left != NULL );
        assert( expr->right != NULL );
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CXEQ );
        break;

      case EXP_OP_CASEZ :
        assert( expr->left != NULL );
        assert( expr->right != NULL );
        vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CZEQ );
        break;

      case EXP_OP_DEFAULT :
        bit = 1;
        vector_set_value( expr->value, &bit, 1, 0, 0 );
        break;

      default :
        print_output( "Internal error:  Unidentified expression operation!", FATAL );
        exit( 1 );
        break;

    }

    /* Set TRUE/FALSE bits to indicate value */
    vector_init( &vec1, &value1a, 1, 0 );
    vector_unary_op( &vec1, expr->value, or_optab );
    switch( vector_bit_val( vec1.value, 0 ) ) {
      case 0 :  expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_FALSE);  break;
      case 1 :  expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_TRUE);   break;
      default:  break;
    }

  }

}

/*!
 \param expr  Pointer to top of expression tree to perform recursive operations.
 
 Recursively performs the proper operations to cause the top-level expression to
 be set to a value.  This function is called during the parse stage to derive 
 pre-CDD widths of multi-bit expressions.  Each MSB/LSB is an expression tree that 
 needs to be evaluated to set the width properly on the MBIT_SEL expression.
*/
void expression_operate_recursively( expression* expr ) {
    
  if( expr != NULL ) {
    
    /*
     Non-static expression found where static expression required.  Simulator
     should catch this error before us, so no user error (too much work to find
     expression in module expression list for now.
    */
    assert( (SUPPL_OP( expr->suppl ) != EXP_OP_SIG)      &&
            (SUPPL_OP( expr->suppl ) != EXP_OP_SBIT_SEL) &&
            (SUPPL_OP( expr->suppl ) != EXP_OP_MBIT_SEL) );

    /* Evaluate children */
    expression_operate_recursively( expr->left  );
    expression_operate_recursively( expr->right );
    
    /* Resize current expression only */
    expression_resize( expr, FALSE );
    
    /* Create vector value to store operation information */
    if( expr->value == NULL ) {
      expression_create_value( expr, expr->value->width, expr->value->lsb, TRUE );
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

  vector result;        /* Vector containing result of expression tree */
  nibble data;          /* Data for result vector                      */

  /* Evaluate the value of the root expression and return this value */
  vector_init( &result, &data, 1, 0 );
  vector_unary_op( &result, expr->value, or_optab );

  return( data & 0x3 );

}

/*!
 \param expr      Pointer to root expression to deallocate.
 \param exp_only  Removes only the specified expression and not its children.

 Deallocates all heap memory allocated with the malloc routine.
*/
void expression_dealloc( expression* expr, bool exp_only ) {

  int op;     /* Temporary operation holder */

  if( expr != NULL ) {

    op = SUPPL_OP( expr->suppl );

    if( (op != EXP_OP_SIG       ) && 
        (op != EXP_OP_SBIT_SEL  ) &&
        (op != EXP_OP_MBIT_SEL  ) &&
        (op != EXP_OP_PARAM     ) &&
        (op != EXP_OP_PARAM_SBIT) &&
        (op != EXP_OP_PARAM_MBIT) ) {

      /* Free up memory from vector value storage */
      vector_dealloc( expr->value );
      expr->value = NULL;

    } else {

      if( expr->sig == NULL ) {
        bind_remove( expr->id );
      } else {
        exp_link_remove( expr, &(expr->sig->exp_head), &(expr->sig->exp_tail) );
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

