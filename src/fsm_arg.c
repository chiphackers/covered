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
 \file     fsm_arg.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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
#include "obfuscate.h"


extern int  curr_expr_id;
extern char user_msg[USER_MSG_LENGTH];


/*!
 \return Returns pointer to expression tree containing parsed state variable expression.

 \throws anonymous fsm_var_bind_add fsm_var_bind_add fsm_var_bind_add fsm_var_bind_add fsm_var_bind_add Throw Throw expression_create expression_create
                   expression_create expression_create expression_create expression_create expression_create expression_create expression_create
                   expression_create expression_create expression_create expression_create expression_create

 Parses the specified argument value for all information regarding a state variable
 expression.
*/
static expression* fsm_arg_parse_state(
  char** arg,        /*!< Pointer to argument to parse */
  char*  funit_name  /*!< Name of functional unit that this expression belongs to */
) { PROFILE(FSM_ARG_PARSE_STATE);

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

        Try {

          if( sig->value->width == 0 ) {

            expr = expression_create( NULL, NULL, EXP_OP_SIG, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
            fsm_var_bind_add( sig->name, expr, funit_name );

          } else if( sig->value->width == 1 ) {

            expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
            vector_dealloc( expr->value );
            expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
            (void)vector_from_int( expr->value, sig->dim[0].lsb );

            expr = expression_create( NULL, expr, EXP_OP_SBIT_SEL, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
            fsm_var_bind_add( sig->name, expr, funit_name );

          } else {

            expt = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
            vector_dealloc( expt->value );
            expt->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
            (void)vector_from_int( expt->value, sig->dim[0].lsb );

            expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
            vector_dealloc( expr->value );
            expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
            (void)vector_from_int( expr->value, ((sig->value->width - 1) + sig->dim[0].lsb) );

            switch( sig->suppl.part.type ) {
              case SSUPPL_TYPE_IMPLICIT     :  op = EXP_OP_MBIT_SEL;  break;
              case SSUPPL_TYPE_IMPLICIT_POS :  op = EXP_OP_MBIT_POS;  break;
              case SSUPPL_TYPE_IMPLICIT_NEG :  op = EXP_OP_MBIT_NEG;  break;
              default                       :  assert( 0 );           break;
            }
            expr = expression_create( expt, expr, op, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
            fsm_var_bind_add( sig->name, expr, funit_name );

          }

          if( expl != NULL ) {
            expl = expression_create( expr, expl, EXP_OP_LIST, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
            curr_expr_id++;
          } else {
            expl = expr;
          }

        } Catch_anonymous {
          vsignal_dealloc( sig );
          expression_dealloc( expr, FALSE );
          Throw 0;
        }

        /* Deallocate signal */
        vsignal_dealloc( sig );

      } else {
        expression_dealloc( expl, FALSE );
        error = TRUE;
      }
    }
    if( !error ) {
      (*arg)++;
      expl = expression_create( expl, NULL, EXP_OP_CONCAT, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
      curr_expr_id++;
    }

  } else {

    if( (sig = vsignal_from_string( arg )) != NULL ) {

      Try {

        if( sig->value->width == 0 ) {

          expl = expression_create( NULL, NULL, EXP_OP_SIG, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
          curr_expr_id++;

        } else if( sig->value->width == 1 ) {

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          (void)vector_from_int( expr->value, sig->dim[0].lsb );

          expl = expression_create( NULL, expr, EXP_OP_SBIT_SEL, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
          curr_expr_id++;

        } else {

          expt = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expt->value );
          expt->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          (void)vector_from_int( expt->value, sig->dim[0].lsb );

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          (void)vector_from_int( expr->value, ((sig->value->width - 1) + sig->dim[0].lsb) );

          switch( sig->suppl.part.type ) {
            case SSUPPL_TYPE_IMPLICIT     :  op = EXP_OP_MBIT_SEL;  break;
            case SSUPPL_TYPE_IMPLICIT_POS :  op = EXP_OP_MBIT_POS;  break;
            case SSUPPL_TYPE_IMPLICIT_NEG :  op = EXP_OP_MBIT_NEG;  break;
            default                       :  assert( 0 );           break;
          }

          expl = expression_create( expt, expr, op, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
          curr_expr_id++;

        }

        /* Add signal name and expression to FSM var binding list */
        fsm_var_bind_add( sig->name, expl, funit_name );

      } Catch_anonymous {
        vsignal_dealloc( sig );
        expression_dealloc( expl, FALSE );
        Throw 0;
      }

      /* Deallocate signal */
      vsignal_dealloc( sig );

    } else {
      error = TRUE;
    }

  }

  /* Create statement for top-level expression, this statement will work like a continuous assignment */
  if( !error ) {
    stmt = statement_create( expl, NULL );
    stmt->suppl.part.head       = 1;
    stmt->suppl.part.stop_true  = 1;
    stmt->suppl.part.stop_false = 1;
    stmt->suppl.part.cont       = 1;
    stmt->next_true             = stmt;
    stmt->next_false            = stmt;
    fsm_var_stmt_add( stmt, funit_name );
  } else {
    expl = NULL;
  }

  PROFILE_END;

  return( expl );

}

/*!
 \throws anonymous Throw

 Parses specified argument string for FSM information.  If the FSM information
 is considered legal, returns TRUE; otherwise, returns FALSE.
*/
void fsm_arg_parse(
  const char* arg  /*!< Command-line argument following -F specifier */
) { PROFILE(FSM_ARG_PARSE);

  char*       tmp = strdup_safe( arg );  /* Temporary copy of given argument */
  char*       ptr = tmp;                 /* Pointer to current character in arg */
  expression* in_state;                  /* Pointer to input state expression */
  expression* out_state;                 /* Pointer to output state expression */

  Try {

    while( (*ptr != '\0') && (*ptr != '=') ) {
      ptr++;
    }

    if( *ptr == '\0' ) {

      print_output( "Option -F must specify a module/task/function/named block and one or two variables.  See \"covered score -h\" for more information.",
                    FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      *ptr = '\0';
      ptr++;

      if( (in_state = fsm_arg_parse_state( &ptr, tmp )) != NULL ) {

        if( *ptr == ',' ) {

          ptr++;

          if( (out_state = fsm_arg_parse_state( &ptr, tmp )) != NULL ) {
            (void)fsm_var_add( arg, in_state, out_state, NULL, FALSE );
          } else {
            print_output( "Illegal FSM command-line output state", FATAL, __FILE__, __LINE__ );
            Throw 0;
          }

        } else {

          /* Copy the current expression */
          (void)fsm_var_add( arg, in_state, in_state, NULL, FALSE );

        }

      } else {
  
        print_output( "Illegal FSM command-line input state", FATAL, __FILE__, __LINE__ );
        Throw 0;
 
      }

    }

  } Catch_anonymous {
    free_safe( tmp, (strlen( arg ) + 1) );
    Throw 0;
  }

  /* Deallocate temporary memory */
  free_safe( tmp, (strlen( arg ) + 1) );

  PROFILE_END;

}

/*!
 \return Returns a pointer to the expression created from the found value; otherwise,
         returns a value of NULL to indicate the this parser was unable to parse the
         specified transition value.

 \throws anonymous Throw expression_create expression_create expression_create expression_create expression_create expression_create
                   expression_create expression_create expression_create expression_create expression_create expression_create expression_create

 Parses specified string value for a parameter or constant value.  If the string
 is parsed correctly, a new expression is created to hold the contents of the
 parsed value and is returned to the calling function.  If the string is not
 parsed correctly, a value of NULL is returned to the calling function.
*/
static expression* fsm_arg_parse_value(
  char**           str,   /*!< Pointer to string containing parameter or constant value */
  const func_unit* funit  /*!< Pointer to functional unit containing this FSM */
) { PROFILE(FSM_ARG_PARSE_VALUE);

  expression* expr = NULL;   /* Pointer to expression containing state value */
  expression* left;          /* Left child expression */
  expression* right = NULL;  /* Right child expression */
  vector*     vec;           /* Pointer to newly allocated vector value */
  int         base;          /* Base of parsed string value */
  char        str_val[256];  /* String version of value parsed */
  int         msb;           /* Most-significant bit position of parameter */
  int         lsb;           /* Least-significant bit position of parameter */
  int         chars_read;    /* Number of characters read from sscanf() */
  mod_parm*   mparm;         /* Pointer to module parameter found */

  vector_from_string( str, FALSE, &vec, &base );

  if( vec != NULL ) {

    /* This value represents a static value, handle as such */
    Try {
      expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
    } Catch_anonymous {
      vector_dealloc( vec );
      Throw 0;
    }
    curr_expr_id++;

    vector_dealloc( expr->value );
    expr->value = vec;

  } else {

    /* This value should be a parameter value, parse it */
    if( sscanf( *str, "%[a-zA-Z0-9_]\[%d:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( left->value, msb );

        /* Generate right child expression */
        Try {
          right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( right->value, lsb );

        /* Generate multi-bit parameter expression */
        Try {
          expr = expression_create( right, left, EXP_OP_PARAM_MBIT, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE ); 
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          expression_dealloc( right, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exps), &(mparm->exp_size) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d+:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( left->value, msb );

        /* Generate right child expression */
        Try {
          right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( right->value, lsb );

        /* Generate variable positive multi-bit parameter expression */
        Try {
          expr = expression_create( right, left, EXP_OP_PARAM_MBIT_POS, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          expression_dealloc( right, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exps), &(mparm->exp_size) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d-:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( left->value, msb );

        /* Generate right child expression */
        Try {
          right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( right->value, lsb );

        /* Generate variable positive multi-bit parameter expression */
        Try {
          expr = expression_create( right, left, EXP_OP_PARAM_MBIT_NEG, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          expression_dealloc( right, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exps), &(mparm->exp_size) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d]%n", str_val, &lsb, &chars_read ) == 2 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        (void)vector_from_int( left->value, lsb );

        /* Generate single-bit parameter expression */
        Try {
          expr = expression_create( NULL, left, EXP_OP_PARAM_SBIT, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exps), &(mparm->exp_size) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]%n", str_val, &chars_read ) == 1 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate parameter expression */
        expr = expression_create( NULL, NULL, EXP_OP_PARAM, FALSE, curr_expr_id, 0, 0, 0, 0, 0, FALSE );
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exps), &(mparm->exp_size) );

      }
    } else {
      expr = NULL;
    }

  }

  PROFILE_END;

  return( expr );

}

/*!
 \throws anonymous Throw

 \par
 Parses a transition string carried in the specified expression argument.  All transitions
 must be in the form of:

 \par
 (\\<parameter\\>|\\<constant_value\\>)->(\\<parameter\\>|\\<constant_value\\>)

 \par
 Each transition is then added to the specified FSM table's arc list which is added to the
 FSM arc transition table when the fsm_create_tables() function is called.
*/
static void fsm_arg_parse_trans(
  expression*      expr,   /*!< Pointer to expression containing string value in vector value array */
  fsm*             table,  /*!< Pointer to FSM table to add the transition arcs to */
  const func_unit* funit   /*!< Pointer to the functional unit that contains the specified FSM */
) { PROFILE(FSM_ARG_PARSE_TRANS);

  expression* from_state;  /* Pointer to from_state value of transition */
  expression* to_state;    /* Pointer to to_state value of transition */
  char*       str;         /* String version of expression value */
  char*       tmp;         /* Temporary pointer to string */

  assert( expr != NULL );

  /* Convert expression value to a string */
  tmp = str = vector_to_string( expr->value, ESUPPL_STATIC_BASE( expr->suppl ), FALSE, 0 );

  Try {

    if( (from_state = fsm_arg_parse_value( &str, funit )) == NULL ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Left-hand side FSM transition value must be a constant value or parameter, line: %u, file: %s",
                                  expr->line, obf_file( funit->orig_fname ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;
    } else {

      if( (str[0] != '-') || (str[1] != '>') ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "FSM transition values must contain the string '->' between them, line: %u, file: %s",
                                    expr->line, obf_file( funit->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      } else {
        str += 2;
      }

      if( (to_state = fsm_arg_parse_value( &str, funit )) == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Right-hand side FSM transition value must be a constant value or parameter, line: %u, file: %s",
                                    expr->line, obf_file( funit->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      } else {

        /* Add both expressions to FSM arc list */
        fsm_add_arc( table, from_state, to_state );

      }

    }

  } Catch_anonymous {
    free_safe( tmp, (strlen( tmp ) + 1) );
    Throw 0;
  }

  /* Deallocate string */
  free_safe( tmp, (strlen( tmp ) + 1) );

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw Throw Throw Throw Throw Throw fsm_arg_parse_trans

 Parses the specified attribute parameter for validity and updates FSM structure
 accordingly.
*/
void fsm_arg_parse_attr(
  attr_param*      ap,      /*!< Pointer to attribute parameter list */
  const func_unit* funit,   /*!< Pointer to functional unit containing this attribute */
  bool             exclude  /*!< If TRUE, sets the exclude bits in the FSM */
) { PROFILE(FSM_ARG_PARSE_ATTR);

  attr_param* curr;               /* Pointer to current attribute parameter in list */
  fsm*        table     = NULL;   /* Pointer to found FSM structure */
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
        table = fsm_link_find( curr->name, funit->fsms, funit->fsm_size );
      }
    } else if( (index == 2) && (strcmp( curr->name, "is" ) == 0) && (curr->expr != NULL) ) {
      if( table == NULL ) {
        int slen;
        tmp = str = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE, 0 );
        slen = strlen( tmp );
        Try {
          if( (in_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal input state expression (%s), file: %s", str, obf_file( funit->orig_fname ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
        } Catch_anonymous {
          free_safe( tmp, (slen + 1) );
          Throw 0;
        }
        free_safe( tmp, (slen + 1) );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Input state specified after output state for this FSM has already been specified, file: %s",
                                    obf_file( funit->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }
    } else if( (index == 2) && (strcmp( curr->name, "os" ) == 0) && (curr->expr != NULL) ) {
      if( table == NULL ) {
        int slen;
        tmp = str = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE, 0 );
        slen = strlen( tmp );
        Try {
          if( (out_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, obf_file( funit->orig_fname ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            (void)fsm_var_add( funit->name, out_state, out_state, curr->name, exclude );
            table = fsm_link_find( curr->name, funit->fsms, funit->fsm_size );
          }
        } Catch_anonymous {
          free_safe( tmp, (slen + 1) );
          Throw 0;
        }
        free_safe( tmp, (slen + 1) );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                                    obf_file( funit->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }
    } else if( (index == 3) && (strcmp( curr->name, "os" ) == 0) && (out_state == NULL) &&
               (in_state != NULL) && (curr->expr != NULL) ) {
      if( table == NULL ) {
        int slen;
        tmp = str = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE, 0 );
        slen = strlen( tmp );
        Try {
          if( (out_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, obf_file( funit->orig_fname ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            (void)fsm_var_add( funit->name, in_state, out_state, curr->name, exclude );
            table = fsm_link_find( curr->name, funit->fsms, funit->fsm_size );
          }
        } Catch_anonymous {
          free_safe( tmp, (slen + 1) );
          Throw 0;
        }
        free_safe( tmp, (slen + 1) );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                                    obf_file( funit->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }
    } else if( (index > 1) && (strncmp( curr->name, "trans", 5 ) == 0) && (curr->expr != NULL) ) {
      if( table == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Attribute FSM name (%s) has not been previously created, file: %s",
                                    obf_sig( curr->name ), obf_file( funit->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      } else {
        fsm_arg_parse_trans( curr->expr, table, funit );
      }
    } else {
      unsigned int rv;
      tmp = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE, 0 );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Invalid covered_fsm attribute parameter (%s=%s), file: %s",
                     curr->name, tmp, obf_file( funit->orig_fname ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      free_safe( tmp, (strlen( tmp ) + 1) );
      Throw 0;
    }

    /* We need to work backwards in attribute parameter lists */
    curr = curr->prev;
    index++;
    
  }

  PROFILE_END;

}

