/*!
 \file     expr.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
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


/*!
 \param exp    Pointer to expression to add value to.
 \param width  Width of value to create.
 \param lsb    Least significant value of value field.

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.  It also evaluates the width and
 expression operation to determine if the current expression is measurable.
 If it is deemed to be so, it sets the MEASURABLE bit in the specified
 expression's supplemental field.
*/
void expression_create_value( expression* exp, int width, int lsb ) {

  /* Create value */
  vector_init( exp->value, 
               (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_SIZE( width ) ),
               width, 
               lsb );
 
  /* If this expression is considered measurable, set the MEASURABLE supplemental bit. */
  if( (width == 1) &&
      (SUPPL_OP( exp->suppl ) != EXP_OP_NONE) &&
      (SUPPL_OP( exp->suppl ) != EXP_OP_SIG) &&
      (SUPPL_OP( exp->suppl ) != EXP_OP_SBIT_SEL) &&
      (SUPPL_OP( exp->suppl ) != EXP_OP_MBIT_SEL) ) {

    exp->suppl = exp->suppl | (0x1 << SUPPL_LSB_MEASURABLE);

  }

}

/*!
 \param right  Pointer to expression on right.
 \param left   Pointer to expression on left.
 \param op     Operation to perform for this expression.
 \param id     ID for this expression as determined by the parent.
 \param line   Starting line for this expression (only valid for root)

 \return Returns pointer to newly created expression.

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create( expression* right, expression* left, int op, int id, int line ) {

  expression* new_expr;    /* Pointer to newly created expression */
  int         rwidth = 0;  /* Bit width of expression on right    */
  int         lwidth = 0;  /* Bit width of expression on left     */

  new_expr = (expression*)malloc_safe( sizeof( expression ) );

  new_expr->suppl  = ((op & 0xff) << SUPPL_LSB_OP);
  new_expr->line   = line;
  new_expr->id     = id;
  new_expr->sig    = NULL;
  new_expr->parent = NULL;
  new_expr->right  = right;
  new_expr->left   = left;
  new_expr->value  = (vector*)malloc_safe( sizeof( vector ) );

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
  if( op == EXP_OP_MULTIPLY ) {

    /* For multiplication, we need a width the sum of the left and right expressions */
    assert( rwidth < 1024 );
    assert( lwidth < 1024 );
    expression_create_value( new_expr, (lwidth + rwidth), 0 );

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
             (op == EXP_OP_UINV ) ||
             (op == EXP_OP_UAND ) ||
             (op == EXP_OP_UNOT ) ||
             (op == EXP_OP_UOR  ) ||
             (op == EXP_OP_UXOR ) ||
             (op == EXP_OP_UNAND) ||
             (op == EXP_OP_UNOR ) ||
             (op == EXP_OP_UNXOR) ) {

    /* If this expression will evaluate to a single bit, create vector now */
    expression_create_value( new_expr, 1, 0 );

  } else {

    /* If both right and left values have their width values set. */
    if( (rwidth > 0) && (lwidth > 0) ) {

      if( rwidth >= lwidth ) {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( rwidth < 1024 );
        expression_create_value( new_expr, rwidth, 0 );
      } else {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( lwidth < 1024 );
        expression_create_value( new_expr, lwidth, 0 );
      }

    } else {
 
      new_expr->value->value = NULL;
      new_expr->value->width = 0;
      new_expr->value->lsb   = 0;
  
    }

  }

  return( new_expr );

}

/*!
 \param base  Expression to merge data into.
 \param in    Expression to get merged into base expression.

 Merges contents of the base and in expressions and places the result into
 the base expression.  If the two expressions given are not the same (IDs, op,
 and/or line position differ) we know that the database files being merged 
 were not created from the same design; therefore, display an error message 
 to the user in this case.  If both expressions are the same, perform the 
 merge.
*/
void expression_merge( expression* base, expression* in ) {

  char msg[4096];    /* Error message to display to user */

  assert( base != NULL );
  assert( in != NULL );

  if( (base->id != in->id) || (SUPPL_OP( base->suppl ) != SUPPL_OP( in->suppl )) || (base->line != in->line) ) {

    print_output( "Attempting to merge databases derived from different designs.  Unable to merge", FATAL );
    exit( 1 );

  } else {

    /* Merge expression vectors */
    vector_merge( base->value, in->value );

    /* Merge expression supplemental fields */
    base->suppl = (base->suppl & SUPPL_MERGE_MASK) | (in->suppl & SUPPL_MERGE_MASK);

  }

}

/*!
 \param expr  Pointer to expression to get ID from.
 \return Returns expression ID for this expression.

 Recursively gets IDs of parents to formulate the unique expression
 ID for this expression.
*/
int expression_get_id( expression* expr ) {

  if( expr == NULL ) {
    /* This is the root expression, it does not have a parent */
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

  fprintf( file, "%d %d %s %x %d %d %d ",
    DB_TYPE_EXPRESSION,
    expr->id,
    scope,
    (expr->suppl & 0xffff),
    expr->line,
    expression_get_id( expr->right ),
    expression_get_id( expr->left )
  );

  if( (SUPPL_OP( expr->suppl ) != EXP_OP_SIG) && 
      (SUPPL_OP( expr->suppl ) != EXP_OP_SBIT_SEL) && 
      (SUPPL_OP( expr->suppl ) != EXP_OP_MBIT_SEL) ) {
    vector_db_write( expr->value, file, (SUPPL_OP( expr->suppl ) == EXP_OP_NONE) );
  }

  fprintf( file, "\n" );

}

/*!
 \param line      String containing database line to read information from.
 \param curr_mod  Pointer to current module that instantiates this expression.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Reads in the specified expression information, creates new expression from
 heap, populates the expression with specified information from file and 
 returns that value in the specified expression pointer.  If all is 
 successful, returns TRUE; otherwise, returns FALSE.
*/
bool expression_db_read( char** line, module* curr_mod ) {

  bool        retval = TRUE;    /* Return value for this function                      */
  int         id;               /* Holder of expression ID                             */
  expression* expr;             /* Pointer to newly created expression                 */
  char        scope[4096];      /* Holder for scope of this expression                 */
  nibble      suppl;            /* Holder of supplemental value of this expression     */
  int         eline;            /* Holder of starting line                             */
  int         right_id;         /* Holder of expression ID to the right                */
  int         left_id;          /* Holder of expression ID to the left                 */
  expression* right;            /* Pointer to current expression's right expression    */
  expression* left;             /* Pointer to current expression's left expression     */
  int         chars_read;       /* Number of characters scanned in from line           */
  vector*     vec;              /* Holders vector value of this expression             */
  char        modname[4096];    /* Name of this expression's module instance           */
  expression  texp;             /* Temporary expression link holder for searching      */
  exp_link*   expl;             /* Pointer to found expression in module               */
  char        msg[4096];        /* Error message string                                */

  if( sscanf( *line, "%d %s %x %d %d %d%n", &id, modname, &suppl, &eline, &right_id, &left_id, &chars_read ) == 6 ) {

    *line = *line + chars_read;

    /* Find module instance name */
    if( (curr_mod == NULL) || (strcmp( curr_mod->name, modname ) != 0) ) {

      print_output( "Internal error:  expression in database written before its module", FATAL );
      retval = FALSE;

    } else {

      /* Find right expression */
      texp.id = right_id;
      if( right_id == 0 ) {
        right = NULL;
      } else if( (expl = exp_link_find( &texp, curr_mod->exp_head )) == NULL ) {
        snprintf( msg, 4096, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, right_id );
  	print_output( msg, FATAL );
	exit( 1 );
      } else {
        right = expl->exp;
      }

      /* Find left expression */
      texp.id = left_id;
      if( left_id == 0 ) {
        left = NULL;
      } else if( (expl = exp_link_find( &texp, curr_mod->exp_head )) == NULL ) {
        snprintf( msg, 4096, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, left_id );
  	print_output( msg, FATAL );
        exit( 1 );
      } else {
        left = expl->exp;
      }

      /* Create new expression */
      expr        = expression_create( right, left, SUPPL_OP( suppl ), id, eline );
      expr->suppl = suppl;

      if( right != NULL ) {
        right->parent = expr;
      }

      if( left != NULL ) {
        left->parent = expr;
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

    }

  } else {

    print_output( "Unable to read expression value", FATAL );
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

  assert( expr != NULL );

  printf( "  Expression =>  id: %d, addr: 0x%lx, right: 0x%lx, left: 0x%lx\n", 
          expr->id, 
          expr,
          expr->right, 
          expr->left );

}

/*!
 \param expr   Pointer to expression to set value to.

 Performs expression operation.  This function must only be run after its
 left and right expressions have been calculated during this clock period.
 Sets the value of the operation in its own vector value and updates the
 suppl nibble as necessary.
*/
void expression_operate( expression* expr ) {

  vector  vec1;                          /* Used for logical reduction            */ 
  vector  vec2;                          /* Used for logical reduction            */
  vector* vec;                           /* Pointer to vector of unknown size     */
  int     i;                             /* Loop iterator                         */
  nibble  bit;                           /* Bit holder for some ops               */
  int     intval1;                       /* Temporary integer value for *, /, %   */
  int     intval2;                       /* Temporary integer value for *, /, %   */
  vector* oldval;                        /* Old vector value                      */
  vector  comp;                          /* Vector containing contents of compare */
  nibble  value1a;                       /* 1-bit nibble value                    */
  nibble  value1b;                       /* 1-bit nibble value                    */
  nibble  value32[ VECTOR_SIZE( 32 ) ];  /* 32-bit nibble value                   */            

  if( expr != NULL ) {

    /* Evaluate left and right expressions before evaluating this expression */
    expression_operate( expr->right );
    expression_operate( expr->left  );

    assert( expr->value != NULL );

    /* Make copy of original expression value for comparison purposes later */
    oldval = vector_create( expr->value->width, expr->value->lsb );
    vector_set_value( oldval, expr->value->value, oldval->width, expr->value->lsb, oldval->lsb );

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

      case EXP_OP_COND_T :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->left->value, or_optab );
        if( vector_bit_val( vec1.value, 0 ) == 1 ) {
          vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 
                            expr->right->value->lsb, expr->value->lsb );
        } else if( vector_bit_val( vec1.value, 0 ) >= 2 ) {
          vec = vector_create( expr->value->width, 0 );
          for( i=0; i<vec->width; i++ ) {
            vector_set_bit( vec->value, 2, i );
          }
          vector_set_value( expr->value, vec->value, vec->width, vec->lsb, expr->value->lsb );
          vector_dealloc( vec );
        }
        break;

      case EXP_OP_COND_F :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->left->value, or_optab );
        if( vector_bit_val( vec1.value, 0 ) == 0 ) {
          vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 
                            expr->right->value->lsb, expr->value->lsb );
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

      case EXP_OP_SIG :
      case EXP_OP_NONE :
        break;

      case EXP_OP_SBIT_SEL :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->right->value, xor_optab );
        if( (vec1.value[0] & 0x3) != 2 ) {  
          expr->value->lsb = vector_to_int( expr->right->value ) - ((expr->suppl >> SUPPL_LSB_SIG_LSB) & 0xffff);
        } else {
          /* Indicate that the value of the LSB could not be calculated -- unknowns exist */
          expr->value->lsb = -1;
        }
        break;

      case EXP_OP_MBIT_SEL :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_init( &vec2, &value1b, 1, 0 );
        vector_unary_op( &vec1, expr->right->value, xor_optab );
        vector_unary_op( &vec2, expr->left->value,  xor_optab );
        if( ((vec1.value[0] & 0x3) != 2) && ((vec2.value[0] & 0x3) != 2) ) {
          expr->value->lsb   = vector_to_int( expr->right->value ) - ((expr->suppl >> SUPPL_LSB_SIG_LSB) & 0xffff);
          expr->value->width = vector_to_int( expr->left->value ) - vector_to_int( expr->right->value ) + 1;
        } else {
          /* Indicate that the value of the LSB and width could not be calculated */
          expr->value->lsb   = -1;
          expr->value->width = -1;
        }
        break;

      case EXP_OP_EXPAND :
        bit = vector_bit_val( expr->right->value->value, 0 );
        for( i=0; i<vector_to_int( expr->left->value ); i++ ) {
          vector_set_value( expr->value, &bit, 1, 0, i );
        }
        break;

      case EXP_OP_CONCAT :
        vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
        vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, expr->right->value->width );
        break;

      case EXP_OP_PEDGE :
        if( vector_bit_val( expr->right->value->value, 0 ) == 1 ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        break;
 
      case EXP_OP_NEDGE :
        bit = 1;
        if( vector_bit_val( expr->right->value->value, 0 ) == 0 ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
        break;

      case EXP_OP_AEDGE :
        bit = 1;
        vector_set_value( expr->value, &bit, 1, 0, 0 );
        break;

      default :
        print_output( "Internal error:  Unidentified expression operation!", FATAL );
        exit( 1 );
        break;

    } 

    /* 
     Compare old expression value to new expression value and set changed bit
     of supplemental field if the two values do not pass case equality. 
    */
    vector_init( &comp, &value1a, 1, 0 );
    vector_op_compare( &comp, oldval, expr->value, COMP_CEQ );
    if( (comp.value[0] & 0x3) == 0 ) { 
      expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_CHANGED);
    }

    vector_dealloc( oldval );

    /* Set TRUE/FALSE bits to indicate value */
    switch( expr->value->value[0] & 0x3 ) {
      case 0 :  expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_FALSE);  break;
      case 1 :  expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_TRUE);   break;
      default:  break;
    }

  }

}

/*!
 \param expr      Pointer to root expression to deallocate.
 \param exp_only  Removes only the specified expression and not its children.

 Deallocates all heap memory allocated with the malloc routine.
*/
void expression_dealloc( expression* expr, bool exp_only ) {

  if( expr != NULL ) {

    if( (SUPPL_OP( expr->suppl ) != EXP_OP_SIG) && 
        (SUPPL_OP( expr->suppl ) != EXP_OP_SBIT_SEL) && 
        (SUPPL_OP( expr->suppl ) != EXP_OP_MBIT_SEL) ) {

      /* Free up memory from vector value storage */
      vector_dealloc( expr->value );
      expr->value = NULL;

    } else {

      bind_remove( expr->id );
 
    }

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

