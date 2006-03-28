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
 \file     fsm_arg.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     10/02/2003
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "fsm_arg.h"
#include "util.h"
#include "fsm.h"
#include "fsm_var.h"
#include "vsignal.h"
#include "expr.h"
#include "vector.h"
#include "statement.h"
#include "link.h"
#include "param.h"


extern int  curr_expr_id;
extern char user_msg[USER_MSG_LENGTH];


/*!
 \param arg         Pointer to argument to parse.
 \param funit_name  Name of functional unit that this expression belongs to.

 \return Returns pointer to expression tree containing parsed state variable expression.

 Parses the specified argument value for all information regarding a state variable
 expression.  This function places all 
*/
expression* fsm_arg_parse_state( char** arg, char* funit_name ) {

  bool        error = FALSE;  /* Specifies if a parsing error has been found */
  vsignal*    sig;            /* Pointer to read-in signal */
  expression* expl  = NULL;   /* Pointer to left expression */
  expression* expr  = NULL;   /* Pointer to right expression */
  expression* expt  = NULL;   /* Pointer to temporary expression */
  statement*  stmt;           /* Pointer to statement containing top expression */
  exp_op_type op;             /* Type of operation to decode for this signal */

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
      if( (sig = vsignal_from_string( arg )) != NULL ) {

        if( sig->value->width == 0 ) {

          expr = expression_create( NULL, NULL, EXP_OP_SIG, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, funit_name );

        } else if( sig->value->width == 1 ) {

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, TRUE );
          vector_from_int( expr->value, sig->lsb );

          expr = expression_create( NULL, expr, EXP_OP_SBIT_SEL, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, funit_name );

        } else {

          expt = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expt->value );
          expt->value = vector_create( 32, TRUE );
          vector_from_int( expt->value, sig->lsb );

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, TRUE );
          vector_from_int( expr->value, ((sig->value->width - 1) + sig->lsb) );

          switch( sig->suppl.part.type ) {
            case SSUPPL_TYPE_IMPLICIT     :  op = EXP_OP_MBIT_SEL;  break;
            case SSUPPL_TYPE_IMPLICIT_POS :  op = EXP_OP_MBIT_POS;  break;
            case SSUPPL_TYPE_IMPLICIT_NEG :  op = EXP_OP_MBIT_NEG;  break;
            default                       :  assert( 0 );           break;
          }
          expr = expression_create( expt, expr, op, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          fsm_var_bind_add( sig->name, expr, funit_name );

        }

        if( expl != NULL ) {
          expl = expression_create( expr, expl, EXP_OP_LIST, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
        } else {
          expl = expr;
        }

        /* Add signal name and expression to FSM var binding list */
        fsm_var_bind_add( sig->name, expr, funit_name );

        /* Deallocate signal */
        vsignal_dealloc( sig );

      } else {
        expression_dealloc( expl, FALSE );
        error = TRUE;
      }
    }
    if( !error ) {
      (*arg)++;
      expl = expression_create( expl, NULL, EXP_OP_CONCAT, FALSE, curr_expr_id, 0, 0, 0, FALSE );
      curr_expr_id++;
    }

  } else {

    if( (sig = vsignal_from_string( arg )) != NULL ) {

      if( sig->value->width == 0 ) {

        expl = expression_create( NULL, NULL, EXP_OP_SIG, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;

      } else if( sig->value->width == 1 ) {

        expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expr->value );
        expr->value = vector_create( 32, TRUE );
        vector_from_int( expr->value, sig->lsb );

        expl = expression_create( NULL, expr, EXP_OP_SBIT_SEL, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;

      } else {

        expt = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expt->value );
        expt->value = vector_create( 32, TRUE );
        vector_from_int( expt->value, sig->lsb );

        expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( expr->value );
        expr->value = vector_create( 32, TRUE );
        vector_from_int( expr->value, ((sig->value->width - 1) + sig->lsb) );

        switch( sig->suppl.part.type ) {
          case SSUPPL_TYPE_IMPLICIT     :  op = EXP_OP_MBIT_SEL;  break;
          case SSUPPL_TYPE_IMPLICIT_POS :  op = EXP_OP_MBIT_POS;  break;
          case SSUPPL_TYPE_IMPLICIT_NEG :  op = EXP_OP_MBIT_NEG;  break;
          default                       :  assert( 0 );           break;
        }

        expl = expression_create( expt, expr, op, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;

      }

      /* Add signal name and expression to FSM var binding list */
      fsm_var_bind_add( sig->name, expl, funit_name );

      /* Deallocate signal */
      vsignal_dealloc( sig );

    } else {
      error = TRUE;
    }

  }

  /* Create statement for top-level expression, this statement will work like a continuous assignment */
  if( !error ) {
    stmt = statement_create( expl );
    stmt->exp->suppl.part.stmt_head       = 1;
    stmt->exp->suppl.part.stmt_stop_true  = 1;
    stmt->exp->suppl.part.stmt_stop_false = 1;
    stmt->exp->suppl.part.stmt_cont       = 1;
    stmt->next_true                       = stmt;
    stmt->next_false                      = stmt;
    fsm_var_stmt_add( stmt, funit_name );
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

  bool        retval = TRUE;  /* Return value for this function */
  char*       ptr    = arg;   /* Pointer to current character in arg */
  expression* in_state;       /* Pointer to input state expression */
  expression* out_state;      /* Pointer to output state expression */

  while( (*ptr != '\0') && (*ptr != '=') ) {
    ptr++;
  }

  if( *ptr == '\0' ) {

    print_output( "Option -F must specify a module/task/function/named block and one or two variables.  See \"covered score -h\" for more information.",
                  FATAL, __FILE__, __LINE__ );
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
 \param str    Pointer to string containing parameter or constant value.
 \param funit  Pointer to functional unit containing this FSM.

 \return Returns a pointer to the expression created from the found value; otherwise,
         returns a value of NULL to indicate the this parser was unable to parse the
         specified transition value.

 Parses specified string value for a parameter or constant value.  If the string
 is parsed correctly, a new expression is created to hold the contents of the
 parsed value and is returned to the calling function.  If the string is not
 parsed correctly, a value of NULL is returned to the calling function.
*/
expression* fsm_arg_parse_value( char** str, func_unit* funit ) {

  expression* expr = NULL;   /* Pointer to expression containing state value */
  expression* left;          /* Left child expression */
  expression* right;         /* Right child expression */
  vector*     vec;           /* Pointer to newly allocated vector value */
  char        str_val[256];  /* String version of value parsed */
  int         msb;           /* Most-significant bit position of parameter */
  int         lsb;           /* Least-significant bit position of parameter */
  int         chars_read;    /* Number of characters read from sscanf() */
  mod_parm*   mparm;         /* Pointer to module parameter found */

  if( (vec = vector_from_string( str, FALSE )) != NULL ) {

    /* This value represents a static value, handle as such */
    expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
    curr_expr_id++;

    vector_dealloc( expr->value );
    expr->value = vec;

  } else {

    /* This value should be a parameter value, parse it */
    if( sscanf( *str, "%[a-zA-Z0-9_]\[%d:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, TRUE );
        vector_from_int( left->value, msb );

        /* Generate right child expression */
        right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, TRUE );
        vector_from_int( right->value, lsb );

        /* Generate multi-bit parameter expression */
        expr = expression_create( right, left, EXP_OP_PARAM_MBIT, FALSE, curr_expr_id, 0, 0, 0, FALSE ); 
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d+:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, TRUE );
        vector_from_int( left->value, msb );

        /* Generate right child expression */
        right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, TRUE );
        vector_from_int( right->value, lsb );

        /* Generate variable positive multi-bit parameter expression */
        expr = expression_create( right, left, EXP_OP_PARAM_MBIT_POS, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d-:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, TRUE );
        vector_from_int( left->value, msb );

        /* Generate right child expression */
        right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, TRUE );
        vector_from_int( right->value, lsb );

        /* Generate variable positive multi-bit parameter expression */
        expr = expression_create( right, left, EXP_OP_PARAM_MBIT_NEG, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d]%n", str_val, &lsb, &chars_read ) == 2 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, TRUE );
        vector_from_int( left->value, lsb );

        /* Generate single-bit parameter expression */
        expr = expression_create( NULL, left, EXP_OP_PARAM_SBIT, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]%n", str_val, &chars_read ) == 1 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate parameter expression */
        expr = expression_create( NULL, NULL, EXP_OP_PARAM, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else {
      expr = NULL;
    }

  }

  return( expr );

}

/*!
 \param expr   Pointer to expression containing string value in vector value array.
 \param table  Pointer to FSM table to add the transition arcs to.
 \param funit  Pointer to the functional unit that contains the specified FSM.

 \par
 Parses a transition string carried in the specified expression argument.  All transitions
 must be in the form of:

 \par
 (\\<parameter\\>|\\<constant_value\\>)->(\\<parameter\\>|\\<constant_value\\>)

 \par
 Each transition is then added to the specified FSM table's arc list which is added to the
 FSM arc transition table when the fsm_create_tables() function is called.
*/
void fsm_arg_parse_trans( expression* expr, fsm* table, func_unit* funit ) {

  expression* from_state;  /* Pointer to from_state value of transition */
  expression* to_state;    /* Pointer to to_state value of transition */
  char*       str;         /* String version of expression value */
  char*       tmp;         /* Temporary pointer to string */

  assert( expr != NULL );

  /* Convert expression value to a string */
  tmp = str = vector_to_string( expr->value );

  if( (from_state = fsm_arg_parse_value( &str, funit )) == NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Left-hand side FSM transition value must be a constant value or parameter, line: %d, file: %s",
              expr->line, funit->filename );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );
  } else {

    if( (str[0] != '-') || (str[1] != '>') ) {
      snprintf( user_msg, USER_MSG_LENGTH, "FSM transition values must contain the string '->' between them, line: %d, file: %s",
                expr->line, funit->filename );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
    } else {
      str += 2;
    }

    if( (to_state = fsm_arg_parse_value( &str, funit )) == NULL ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Right-hand side FSM transition value must be a constant value or parameter, line: %d, file: %s",
                expr->line, funit->filename );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
    } else {

      /* Add both expressions to FSM arc list */
      fsm_add_arc( table, from_state, to_state );

    }

  }

  /* Deallocate string */
  free_safe( tmp );

}

/*!
 \param ap     Pointer to attribute parameter list.
 \param funit  Pointer to functional unit containing this attribute.

 Parses the specified attribute parameter for validity and updates FSM structure
 accordingly.
*/
void fsm_arg_parse_attr( attr_param* ap, func_unit* funit ) {

  attr_param* curr;               /* Pointer to current attribute parameter in list */
  fsm_link*   fsml      = NULL;   /* Pointer to found FSM structure */
  fsm         table;              /* Temporary FSM used for searching purposes */
  int         index     = 1;      /* Current index number in list */
  bool        ignore    = FALSE;  /* Set to TRUE if we should ignore this attribute */
  expression* in_state  = NULL;   /* Pointer to input state */
  expression* out_state = NULL;   /* Pointer to output state */
  char*       str;                /* Temporary holder for string value */
  char*       tmp;                /* Temporary holder for string value */

  curr = ap;
  while( (curr != NULL) && !ignore ) {

    /* This name is the name of the FSM structure to update */
    if( index == 1 ) {
      if( curr->expr != NULL ) {
        ignore = TRUE;
      } else {
        table.name = curr->name;
        fsml       = fsm_link_find( &table, funit->fsm_head );
      }
    } else if( (index == 2) && (strcmp( curr->name, "is" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        tmp = str = vector_to_string( curr->expr->value );
        if( (in_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Illegal input state expression (%s), file: %s", str, funit->filename );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          exit( 1 );
        }
        free_safe( tmp );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Input state specified after output state for this FSM has already been specified, file: %s",
                  funit->filename );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }
    } else if( (index == 2) && (strcmp( curr->name, "os" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        tmp = str = vector_to_string( curr->expr->value );
        if( (out_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, funit->filename );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          exit( 1 );
        } else {
          fsm_var_add( funit->name, out_state, out_state, table.name );
          fsml = fsm_link_find( &table, funit->fsm_head );
        }
        free_safe( tmp );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                  funit->filename );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }
    } else if( (index == 3) && (strcmp( curr->name, "os" ) == 0) && (out_state == NULL) &&
               (in_state != NULL) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        tmp = str = vector_to_string( curr->expr->value );
        if( (out_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, funit->filename );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          exit( 1 );
        } else {
          fsm_var_add( funit->name, in_state, out_state, table.name );
          fsml = fsm_link_find( &table, funit->fsm_head );
        }
        free_safe( tmp );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                  funit->filename );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }
    } else if( (index > 1) && (strcmp( curr->name, "trans" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Attribute FSM name (%s) has not been previously created, file: %s", table.name,
                  funit->filename );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      } else {
        fsm_arg_parse_trans( curr->expr, fsml->table, funit );
      }
    } else {
      tmp = vector_to_string( curr->expr->value );
      snprintf( user_msg, USER_MSG_LENGTH, "Invalid covered_fsm attribute parameter (%s=%s), file: %s",
                curr->name,
                tmp,
                funit->filename );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      free_safe( tmp );
      exit( 1 );
    }

    /* We need to work backwards in attribute parameter lists */
    curr = curr->prev;
    index++;
    
  }

}


/*
 $Log$
 Revision 1.25  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.24  2005/12/23 05:41:52  phase1geo
 Fixing several bugs in score command per bug report #1388339.  Fixed problem
 with race condition checker statement iterator to eliminate infinite looping (this
 was the problem in the original bug).  Also fixed expression assigment when static
 expressions are used in the LHS (caused an assertion failure).  Also fixed the race
 condition checker to properly pay attention to task calls, named blocks and fork
 statements to make sure that these are being handled correctly for race condition
 checking.  Fixed bug for signals that are on the LHS side of an assignment expression
 but is not being assigned (bit selects) so that these are NOT considered for race
 conditions.  Full regression is a bit broken now but the opened bug can now be closed.

 Revision 1.23  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.22  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.21  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.20  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.19  2005/01/07 23:00:10  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.18  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.17  2005/01/06 23:51:17  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.16  2004/04/19 04:54:56  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.15  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.14  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.13  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.12  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.11  2003/11/15 04:25:02  phase1geo
 Fixing syntax error found in Doxygen.

 Revision 1.10  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.9  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.8  2003/10/28 01:09:38  phase1geo
 Cleaning up unnecessary output.

 Revision 1.7  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

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

