/*!
 \file     fsm_arg.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     10/02/2003
*/

#include <stdio.h>

#include "defines.h"
#include "fsm_arg.h"
#include "util.h"
#include "fsm.h"
#include "fsm_var.h"
#include "signal.h"
#include "expr.h"
#include "vector.h"
#include "statement.h"


extern int curr_expr_id;


expression* fsm_arg_parse_state( char** arg, char* mod_name ) {

  bool        error = FALSE;  /* Specifies if a parsing error has beenf found   */
  signal*     sig;            /* Pointer to read-in signal                      */
  expression* expl  = NULL;   /* Pointer to left expression                     */
  expression* expr  = NULL;   /* Pointer to right expression                    */
  expression* expt  = NULL;   /* Pointer to temporary expression                */
  statement*  stmt;           /* Pointer to statement containing top expression */

  /*
   If we are a concatenation, parse arguments of concatenation as signal names
   in a comma-separated list.
  */
  if( **arg == '{' ) {

    /* Get first state */
    while( (**arg != '}') && !error ) {
      if( expl != NULL && (**arg == ',') ) {
        (*arg)++;
      }
      if( (sig = signal_from_string( arg )) != NULL ) {
        printf( "arg: %s\n", *arg );

        if( sig->value->width == 0 ) {

          expr = expression_create( NULL, NULL, EXP_OP_SIG, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, mod_name );

        } else if( sig->value->width == 1 ) {

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, 0, TRUE );
          vector_from_int( expr->value, sig->value->lsb );

          expr = expression_create( NULL, expr, EXP_OP_SBIT_SEL, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, mod_name );

        } else {

          expt = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expt->value );
          expt->value = vector_create( 32, 0, TRUE );
          vector_from_int( expt->value, sig->value->lsb );

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, 0, TRUE );
          vector_from_int( expr->value, ((sig->value->width - 1) + sig->value->lsb) );

          expr = expression_create( expt, expr, EXP_OP_MBIT_SEL, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, mod_name );

        }

        if( expl != NULL ) {
          expl = expression_create( expr, expl, EXP_OP_LIST, curr_expr_id, 0, FALSE );
          curr_expr_id++;
        } else {
          expl = expr;
        }

        /* Add signal name and expression to FSM var binding list */
        fsm_var_bind_add( sig->name, expr, mod_name );

      } else {
        expression_dealloc( expl, FALSE );
        error = TRUE;
      }
    }
    if( !error ) {
      (*arg)++;
      expl = expression_create( expl, NULL, EXP_OP_CONCAT, curr_expr_id, 0, FALSE );
      curr_expr_id++;
    }

  } else {

    if( (sig = signal_from_string( arg )) != NULL ) {
      printf( "arg: %s\n", *arg );

      if( sig->value->width == 0 ) {

        expl = expression_create( NULL, NULL, EXP_OP_SIG, curr_expr_id, 0, FALSE );
        curr_expr_id++;

      } else if( sig->value->width == 1 ) {

        expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expr->value );
        expr->value = vector_create( 32, 0, TRUE );
        vector_from_int( expr->value, sig->value->lsb );

        expl = expression_create( NULL, expr, EXP_OP_SBIT_SEL, curr_expr_id, 0, FALSE );
        curr_expr_id++;

      } else {

        expt = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expt->value );
        expt->value = vector_create( 32, 0, TRUE );
        vector_from_int( expt->value, sig->value->lsb );

        expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expr->value );
        expr->value = vector_create( 32, 0, TRUE );
        vector_from_int( expr->value, ((sig->value->width - 1) + sig->value->lsb) );

        expl = expression_create( expt, expr, EXP_OP_MBIT_SEL, curr_expr_id, 0, FALSE );
        curr_expr_id++;

      }

      /* Add signal name and expression to FSM var binding list */
      fsm_var_bind_add( sig->name, expl, mod_name );

    } else {
      error = TRUE;
    }

  }

  /* Create statement for top-level expression, this statement will work like a continuous assignment */
  if( !error ) {
    stmt = statement_create( expl );
    stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
    stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_STOP);
    stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_CONTINUOUS);
    stmt->next_true  = stmt;
    stmt->next_false = stmt;
    fsm_var_stmt_add( stmt, mod_name );
  } else {
    expl = NULL;
  }

  return( expl );

}

/*!
 \param arg  Command-line argument following -F specifier.

 \return Returns TRUE if argument is considered legal for the -F specifier;
         otherwise, returns FALSE.

 Parses specified argument string for FSM information.  If the FSM information
 is considered legal, returns TRUE; otherwise, returns FALSE.
*/
bool fsm_arg_parse( char* arg ) {

  bool        retval = TRUE;  /* Return value for this function        */
  char*       ptr   = arg;    /* Pointer to current character in arg   */
  fsm_var*    fv;             /* Pointer to newly created FSM variable */
  expression* in_state;       /* Pointer to input state expression     */
  expression* out_state;      /* Pointer to output state expression    */

  while( (*ptr != '\0') && (*ptr != '=') ) {
    ptr++;
  }

  if( *ptr == '\0' ) {

    print_output( "Option -F must specify a module and one or two variables.  See \"covered score -h\" for more information.", FATAL );
    retval = FALSE;

  } else {

    *ptr = '\0';
    ptr++;

    if( (in_state = fsm_arg_parse_state( &ptr, arg )) != NULL ) {

      printf( "ptr: %c\n", *ptr );

      if( *ptr == ',' ) {

        ptr++;

        if( (out_state = fsm_arg_parse_state( &ptr, arg )) != NULL ) {
          fsm_var_add( arg, in_state, out_state );
        } else {
          retval = TRUE;
        }

      } else {

        /* Copy the current expression */
        fsm_var_add( arg, in_state, in_state );

      }

    } else {
  
      retval = FALSE;
 
    }

  }

  return( retval );

}

/*
 $Log$
 Revision 1.2  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.1  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.
*/

