/*!
 \file     fsm_arg.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     10/02/2003
*/

#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "fsm_arg.h"
#include "util.h"
#include "fsm.h"
#include "fsm_var.h"
#include "signal.h"
#include "expr.h"
#include "vector.h"
#include "statement.h"
#include "link.h"


extern int  curr_expr_id;
extern char user_msg[USER_MSG_LENGTH];


/*!
 \param arg       Pointer to argument to parse.
 \param mod_name  Name of module that this expression belongs to.

 \return Returns pointer to expression tree containing parsed state variable expression.

 Parses the specified argument value for all information regarding a state variable
 expression.  This function places all 
*/
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
      if( ((expl != NULL) && (**arg == ',')) || (**arg == '{') ) {
        (*arg)++;
      }
      if( (sig = signal_from_string( arg )) != NULL ) {

        if( sig->value->width == 0 ) {

          expr = expression_create( NULL, NULL, EXP_OP_SIG, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, mod_name );

        } else if( sig->value->width == 1 ) {

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, TRUE );
          vector_from_int( expr->value, sig->lsb );

          expr = expression_create( NULL, expr, EXP_OP_SBIT_SEL, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, mod_name );

        } else {

          expt = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expt->value );
          expt->value = vector_create( 32, TRUE );
          vector_from_int( expt->value, sig->lsb );

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, TRUE );
          vector_from_int( expr->value, ((sig->value->width - 1) + sig->lsb) );

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

      if( sig->value->width == 0 ) {

        expl = expression_create( NULL, NULL, EXP_OP_SIG, curr_expr_id, 0, FALSE );
        curr_expr_id++;

      } else if( sig->value->width == 1 ) {

        expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expr->value );
        expr->value = vector_create( 32, TRUE );
        vector_from_int( expr->value, sig->lsb );

        expl = expression_create( NULL, expr, EXP_OP_SBIT_SEL, curr_expr_id, 0, FALSE );
        curr_expr_id++;

      } else {

        expt = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expt->value );
        expt->value = vector_create( 32, TRUE );
        vector_from_int( expt->value, sig->lsb );

        expr = expression_create( NULL, NULL, EXP_OP_STATIC, curr_expr_id, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expr->value );
        expr->value = vector_create( 32, TRUE );
        vector_from_int( expr->value, ((sig->value->width - 1) + sig->lsb) );

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
  char*       ptr    = arg;   /* Pointer to current character in arg   */
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

      if( *ptr == ',' ) {

        ptr++;

        if( (out_state = fsm_arg_parse_state( &ptr, arg )) != NULL ) {
          fsm_var_add( arg, in_state, out_state, NULL );
        } else {
          retval = TRUE;
        }

      } else {

        /* Copy the current expression */
        fsm_var_add( arg, in_state, in_state, NULL );

      }

    } else {
  
      retval = FALSE;
 
    }

  }

  return( retval );

}

/*!
 \param ap   Pointer to attribute parameter list.
 \param mod  Pointer to module containing this attribute.

 Parses the specified attribute parameter for validity and updates FSM structure
 accordingly.
*/
void fsm_arg_parse_attr( attr_param* ap, module* mod ) {

  attr_param* curr;               /* Pointer to current attribute parameter in list */
  fsm_link*   fsml;               /* Pointer to found FSM structure                 */
  fsm         table;              /* Temporary FSM used for searching purposes      */
  int         index     = 1;      /* Current index number in list                   */
  bool        ignore    = FALSE;  /* Set to TRUE if we should ignore this attribute */
  expression* in_state  = NULL;   /* Pointer to input state                         */
  expression* out_state = NULL;   /* Pointer to output state                        */
  char*       str;                /* Temporary holder for string value              */

  curr = ap;
  while( (curr != NULL) && !ignore ) {

    printf( "Here 1, name: %s\n", curr->name );

    if( curr->expr == NULL ) {
      printf( "Parsing attribute parameter: %s\n", curr->name );
    } else {
      printf( "Parsing attribute parameter: %s, expr: %s\n", curr->name, (char*)(curr->expr->value->value) );
    }

    /* This name is the name of the FSM structure to update */
    if( index == 1 ) {
      if( curr->expr != NULL ) {
        ignore = TRUE;
      } else {
        table.name = curr->name;
        fsml       = fsm_link_find( &table, mod->fsm_head );
      }
    } else if( (index == 2) && (strcmp( curr->name, "is" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        str = (char*)(curr->expr->value->value);
        if( (in_state = fsm_arg_parse_state( &str, mod->name )) == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Illegal input state expression (%s), file: %s", str, mod->filename );
          print_output( user_msg, FATAL );
          exit( 1 );
        }
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Input state specified after output state for this FSM has already been specified, file: %s",
                  mod->filename );
        print_output( user_msg, FATAL );
        exit( 1 );
      }
    } else if( (index == 2) && (strcmp( curr->name, "os" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        str = (char*)(curr->expr->value->value);
        if( (out_state = fsm_arg_parse_state( &str, mod->name )) == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, mod->filename );
          print_output( user_msg, FATAL );
          exit( 1 );
        } else {
          fsm_var_add( mod->name, out_state, out_state, table.name );
          fsml = fsm_link_find( &table, mod->fsm_head );
        }
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                  mod->filename );
        print_output( user_msg, FATAL );
        exit( 1 );
      }
    } else if( (index == 3) && (strcmp( curr->name, "os" ) == 0) && (out_state == NULL) &&
               (in_state != NULL) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        str = (char*)(curr->expr->value->value);
        if( (out_state = fsm_arg_parse_state( &str, mod->name )) == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, mod->filename );
          print_output( user_msg, FATAL );
          exit( 1 );
        } else {
          fsm_var_add( mod->name, in_state, out_state, table.name );
          fsml = fsm_link_find( &table, mod->fsm_head );
        }
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                  mod->filename );
        print_output( user_msg, FATAL );
        exit( 1 );
      }
    } else if( (index > 1) && (strcmp( curr->name, "trans" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Attribute FSM name (%s) has not been previously created, file: %s", table.name,
                  mod->filename );
        print_output( user_msg, FATAL );
        exit( 1 );
      } else {
        /* Handle state transition information here */
      }
    } else {
      snprintf( user_msg, USER_MSG_LENGTH, "Invalid covered_fsm attribute parameter (%s=%s), file: %s",
                curr->name,
                curr->expr->value->value,
                mod->filename );
      print_output( user_msg, FATAL );
      exit( 1 );
    }

    /* We need to work backwards in attribute parameter lists */
    curr = curr->prev;
    index++;
    
  }

}


/*
 $Log$
 Revision 1.6  2003/10/19 05:13:26  phase1geo
 Updating user documentation for changes to FSM specification syntax.  Added
 new fsm5.3 diagnostic to verify concatenation syntax.  Fixing bug in concatenation
 syntax handling.

 Revision 1.5  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.4  2003/10/13 12:27:25  phase1geo
 More fixes to FSM stuff.

 Revision 1.3  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.2  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.1  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.
*/

