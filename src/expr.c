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

extern int curr_sim_time;


/*!
 \param exp    Pointer to expression to add value to.
 \param width  Width of value to create.
 \param lsb    Least significant value of value field.

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.
*/
void expression_create_value( expression* exp, int width, int lsb ) {

  /* Create value */
  vector_init( exp->value, 
               (nibble*)malloc_safe( sizeof( nibble ) * VECTOR_SIZE( width ) ),
               width, 
               lsb );

}

/*!
 \param right  Pointer to expression on right.
 \param left   Pointer to expression on left.
 \param op     Operation to perform for this expression.
 \param id     ID for this expression as determined by the parent.
 \param line   Line number this expression is on.

 \return Returns pointer to newly created expression.

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create( expression* right, expression* left, int op, int id, int line ) {

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

  if( (base->id != in->id) || 
      (SUPPL_OP( base->suppl ) != SUPPL_OP( in->suppl )) ||
      (base->line != in->line) ) {

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

  // printf( "In expression_db_write, writing expression %d\n", expr->id );

  fprintf( file, "%d %d %s %d %x %d %d ",
    DB_TYPE_EXPRESSION,
    expr->id,
    scope,
    expr->line,
    (expr->suppl & 0xffff),
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
  int         linenum;          /* Holder of current line for this expression          */
  nibble      suppl;            /* Holder of supplemental value of this expression     */
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

  if( sscanf( *line, "%d %s %d %x %d %d%n", &id, modname, &linenum, &suppl, &right_id, &left_id, &chars_read ) == 6 ) {

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
      expr        = expression_create( right, left, SUPPL_OP( suppl ), id, linenum );
      expr->suppl = suppl;

      if( right != NULL ) {
        right->parent->expr = expr;
      }

      if( left != NULL ) {
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

  printf( "  Expression =>  id: %d, line: %d, addr: 0x%lx, right: 0x%lx, left: 0x%lx\n", 
          expr->id,
          expr->line,
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
  nibble  value1a;                       /* 1-bit nibble value                    */
  nibble  value1b;                       /* 1-bit nibble value                    */
  nibble  value32[ VECTOR_SIZE( 32 ) ];  /* 32-bit nibble value                   */            

  if( expr != NULL ) {

    /* Evaluate left and right expressions before evaluating this expression */
//    expression_operate( expr->right );
//    expression_operate( expr->left  );

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
        vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, expr->right->value->lsb, expr->value->lsb );
        break;

      case EXP_OP_COND_SEL :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->parent->expr->left->value, or_optab );
        if( vector_bit_val( vec1.value, 0 ) == 0 ) {
          vector_set_value( expr->value, expr->right->value->value, expr->right->value->width,
                            expr->right->value->lsb, expr->value->lsb );
        } else if( vector_bit_val( vec1.value, 0 ) == 1 ) {
          vector_set_value( expr->value, expr->left->value->value, expr->left->value->width,
                            expr->left->value->lsb, expr->value->lsb );
        } else {
          vec = vector_create( expr->value->width, 0 );
          for( i=0; i<vec->width; i++ ) {
            vector_set_bit( vec->value, 2, i );
          }
          vector_set_value( expr->value, vec->value, vec->width, vec->lsb, expr->value->lsb );
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

      case EXP_OP_SIG :
      case EXP_OP_NONE :
        break;

      case EXP_OP_SBIT_SEL :
        vector_init( &vec1, &value1a, 1, 0 );
        vector_unary_op( &vec1, expr->right->value, or_optab );
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
        vector_unary_op( &vec1, expr->right->value, or_optab );
        vector_unary_op( &vec2, expr->left->value,  or_optab );
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

      case EXP_OP_DELAY :
        /* If this expression is not currently waiting, set the start time of delay */
        if( vector_to_int( expr->left->value ) == 0xffffffff ) {
          vector_from_int( expr->left->value, curr_sim_time );
        }
        intval1 = vector_to_int( expr->left->value );           // Start time of delay
        intval2 = vector_to_int( expr->right->value );          // Number of clocks to delay
        if( ((intval1 + intval2) <= curr_sim_time) || ((curr_sim_time == -1) && (vector_to_int( expr->left->value ) != 0xffffffff)) ) {
          bit = 1;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
          vector_from_int( expr->left->value, 0xffffffff );
        } else {
          bit = 0;
          vector_set_value( expr->value, &bit, 1, 0, 0 );
        }
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
 \param expr  Pointer to expression to evaluate.

 \return Returns 1 if the specified expression evaluates to a unary 1.

 Returns a value of 1 if the specified expression contains at least one 1 value
 and no X or Z values in its bits.  It accomplishes this by performing a unary 
 OR operation on the specified expression value and testing bit 0 of the result.
*/
bool expression_is_true( expression* expr ) {

  vector result;        /* Vector containing result of expression tree */
  nibble data;          /* Data for result vector                      */

  /* Evaluate the value of the root expression and return this value */
  vector_init( &result, &data, 1, 0 );
  vector_unary_op( &result, expr->value, or_optab );

  return( (data & 0x3) == 1 );

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


/* $Log$
/* Revision 1.21  2002/07/02 18:42:18  phase1geo
/* Various bug fixes.  Added support for multiple signals sharing the same VCD
/* symbol.  Changed conditional support to allow proper simulation results.
/* Updated VCD parser to allow for symbols containing only alphanumeric characters.
/*
/* Revision 1.20  2002/07/01 15:10:42  phase1geo
/* Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
/* seem to be passing with these fixes.
/*
/* Revision 1.19  2002/06/29 04:51:31  phase1geo
/* Various fixes for bugs found in regression testing.
/*
/* Revision 1.18  2002/06/28 03:04:59  phase1geo
/* Fixing more errors found by diagnostics.  Things are running pretty well at
/* this point with current diagnostics.  Still some report output problems.
/*
/* Revision 1.17  2002/06/28 00:40:37  phase1geo
/* Cleaning up extraneous output from debugging.
/*
/* Revision 1.16  2002/06/27 20:39:43  phase1geo
/* Fixing scoring bugs as well as report bugs.  Things are starting to work
/* fairly well now.  Added rest of support for delays.
/*
/* Revision 1.15  2002/06/26 22:09:17  phase1geo
/* Removing unecessary output and updating regression Makefile.
/*
/* Revision 1.14  2002/06/26 04:59:50  phase1geo
/* Adding initial support for delays.  Support is not yet complete and is
/* currently untested.
/*
/* Revision 1.13  2002/06/25 03:39:03  phase1geo
/* Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
/* Fixed some report bugs though there are still some remaining.
/*
/* Revision 1.12  2002/06/24 04:54:48  phase1geo
/* More fixes and code additions to make statements work properly.  Still not
/* there at this point.
/*
/* Revision 1.11  2002/06/23 21:18:21  phase1geo
/* Added appropriate statement support in parser.  All parts should be in place
/* and ready to start testing.
/*
/* Revision 1.10  2002/06/22 21:08:23  phase1geo
/* Added simulation engine and tied it to the db.c file.  Simulation engine is
/* currently untested and will remain so until the parser is updated correctly
/* for statements.  This will be the next step.
/*
/* Revision 1.9  2002/06/22 05:27:30  phase1geo
/* Additional supporting code for simulation engine and statement support in
/* parser.
/*
/* Revision 1.8  2002/06/21 05:55:05  phase1geo
/* Getting some codes ready for writing simulation engine.  We should be set
/* now.
/*
/* Revision 1.7  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/*
/* Revision 1.6  2002/05/03 03:39:36  phase1geo
/* Removing all syntax errors due to addition of statements.  Added more statement
/* support code.  Still have a ways to go before we can try anything.  Removed lines
/* from expressions though we may want to consider putting these back for reporting
/* purposes.
/* */

