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
 \file     binding.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/4/2002
 
 \par Binding
 When a input, output, inout, reg, wire, etc. is parsed in a module a new signal
 structure is created and is placed into the current module's signal list.  However,
 the expression list of the newly created signal is initially empty (because we have
 not encountered any expressions that use the signal yet).  Each signal contains a
 list of expressions that the signal is a part of so that when the signal changes its
 value during simulation time, it can notify the expressions that it is a part of that
 they need to be re-evaluated.

 \par
 Additionally, the expression structure contains a pointer to the signal from which
 it receives its value.  However, not all expressions point to signals.  Only the
 expressions which have an operation type of EXP_OP_SIG, EXP_OP_SBIT_SEL, EXP_OP_MBIT_SEL,
 and EXP_OP_FUNC_CALL have pointers to signals.  These pointers are used for quick
 retrieval of the signal name when outputting expressions.

 \par
 Because both signals and expressions point to each other, we say that signals and
 expressions need to be bound to each other.  The process of binding takes place after
 all design file parsing has been completed, allowing an expression to be bound to a signal
 elsewhere in the design.  Binding is performed twice at this point in the score command
 (occurs once for the merge and report commands).  The first binding pass binds all
 expressions to local signals/parameters.  This local binding is required to allow parameters and constant
 function calls in parameter assignments to be calculated correctly.  As each binding is performed,
 it is removed from the list of all bindings that need to be performed for the design.  After the
 first binding pass is performed, all parameters are resolved for their values and all generated logic
 is created, after which all other expressions that still need to bound are handled.

 \par Implicit Signal Creation
 In several Verilog simulators, the automatic creation of one-bit wires is allowed.
 These signals are considered "automatically created" because they are not declared
 in either the port list or the wire list for its particular module.  Therefore, when the
 binding process occurs and a signal structure has not been created for a used signal
 (because the signal was not declared in the port list or wire list), the bind_signal
 function needs to do one of the following:

 \par
 -# If the signal name expresses a signal name that will be local to the current
    module (i.e., there aren't any periods in the signal name), automatically create
    a one-bit signal for the missing signal and bind this new signal to the expression
    that uses the implicit signal.
 -# If the signal name expresses a signal name that will be remote to the current
    module (i.e., if there are periods in the signal name), generate an error message
    to the user about using a bad hierarchical reference.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "binding.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "instance.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "param.h"
#include "scope.h"
#include "statement.h"
#include "stmt_blk.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern db**         db_list;
extern unsigned int curr_db;
extern funit_link*  funit_head;
extern char         user_msg[USER_MSG_LENGTH];
extern bool         debug_mode;


/*!
 Pointer to the head of the signal/functional unit/expression binding list.
*/
static exp_bind* eb_head;

/*!
 Pointer to the tail of the signal/functional unit/expression binding list.
*/
static exp_bind* eb_tail;


/*!
 Adds the specified signal/function/task and expression to the bindings linked list.
 This bindings list will be handled after all input Verilog has been
 parsed.
*/
void bind_add(
  int         type,    /*!< Type of thing being bound with the specified expression (0=signal, 1=functional unit) */
  const char* name,    /*!< Signal/Function/Task scope to bind */
  expression* exp,     /*!< Expression ID to bind */
  func_unit*  funit,   /*!< Pointer to module containing specified expression */
  bool        staticf  /*!< Set to TRUE if the given binding is a static function binding */
) { PROFILE(BIND_ADD);
  
  exp_bind* eb;   /* Temporary pointer to signal/expressing binding */

  assert( exp != NULL );
  
  /* Create new signal/expression binding */
  eb                 = (exp_bind *)malloc_safe( sizeof( exp_bind ) );
  eb->type           = type;
  eb->name           = strdup_safe( name );
  eb->clear_assigned = 0;
  eb->line           = exp->line;
  eb->funit          = funit;
  eb->exp            = exp;
  eb->fsm            = NULL;
  eb->staticf        = staticf;
  eb->next           = NULL;
  
  /* Add new signal/expression binding to linked list */
  if( eb_head == NULL ) {
    eb_head = eb_tail = eb;
  } else {
    eb_tail->next = eb;
    eb_tail       = eb;
  }

  PROFILE_END;
  
}

/*!
 Searches the expression binding list for the entry that matches the given exp and curr_funit
 parameters.  When the entry is found, the FSM expression is added to the exp_bind structure
 to be sized when the expression is bound.
*/
void bind_append_fsm_expr(
  expression*       fsm_exp,    /*!< Expression pertaining to an FSM input state that needs to be sized when
                                     its associated expression is bound to its signal */
  const expression* exp,        /*!< Expression to match */
  const func_unit*  curr_funit  /*!< Functional unit that the FSM expression resides in (this will be the same
                                     functional unit as the expression functional unit) */
) { PROFILE(BIND_APPEND_FSM_EXPR);

  exp_bind* curr;

  curr = eb_head;
  while( (curr != NULL) && ((exp != curr->exp) || (curr_funit != curr->funit)) ) {
    curr = curr->next;
  }

  assert( curr != NULL );

  curr->fsm = fsm_exp;

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Displays to standard output the current state of the binding list (debug purposes only).
*/
void bind_display_list() {

  exp_bind* curr;  /* Pointer to current expression binding */

  curr = eb_head;
 
  printf( "Expression binding list:\n" );

  while( curr != NULL ) {

    switch( curr->type ) {
      case FUNIT_AFUNCTION :
      case FUNIT_FUNCTION :
        printf( "  Expr: %d, %s, line %u;  Functional Unit: %s;  Function: %s\n",
                curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        break;
      case FUNIT_ATASK :
      case FUNIT_TASK :
        printf( "  Expr: %d, %s, line %u;  Functional Unit: %s;  Task: %s\n",
                curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK :
        printf( "  Expr: %d, %s, line %u;  Functional Unit: %s;  Named Block: %s\n",
                curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        break;
      case 0 :
        if( curr->clear_assigned > 0 ) {
          printf( "  Signal to be cleared: %s\n", obf_sig( curr->name ) );
        } else {
          printf( "  Expr: %d, %s, line %u;  Functional Unit: %s;  Signal: %s\n",
                  curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                  obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        }
        break;
      default :  break;
    }

    curr = curr->next;

  }

}
#endif /* RUNLIB */

/*!
 Removes the binding containing the expression ID of id.  This needs to
 be called before an expression is removed.
*/
void bind_remove(
  int  id,             /*!< Expression ID of binding to remove */
  bool clear_assigned  /*!< If set to TRUE, clears the assigned bit in the specified expression */
) { PROFILE(BIND_REMOVE);

  exp_bind* curr;  /* Pointer to current exp_bind link */
  exp_bind* last;  /* Pointer to last exp_bind link examined */

  curr = eb_head;
  last = eb_head;

  while( curr != NULL ) {

    if( ((curr->exp != NULL) && (curr->exp->id == id)) || (curr->clear_assigned == id) ) {
      
      if( clear_assigned ) {

        curr->clear_assigned = id;
        curr->exp            = NULL;

      } else {

        /* Remove this binding element */
        if( (curr == eb_head) && (curr == eb_tail) ) {
          eb_head = eb_tail = NULL;
        } else if( curr == eb_head ) {
          eb_head = eb_head->next;
        } else if( curr == eb_tail ) {
          eb_tail       = last;
          eb_tail->next = NULL;
        } else {
          last->next = curr->next;
        }

        /* Now free the binding element memory */
        free_safe( curr->name, (strlen( curr->name ) + 1) );
        free_safe( curr, sizeof( exp_bind ) );

      }

      curr = NULL;
      
    } else {

      last = curr;
      curr = curr->next;

    }

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \return Returns the name of the signal to be bound with the given expression (if one exists);
         otherwise, returns NULL if no match was found.
*/
char* bind_find_sig_name(
  const expression* exp  /*!< Pointer to expression to search for */
) { PROFILE(BIND_FIND_SIG_NAME);

  exp_bind*  curr;         /* Pointer to current exp_bind link */
  vsignal*   found_sig;    /* Placeholder */
  func_unit* found_funit;  /* Specifies the functional unit containing this signal */
  char*      name = NULL;  /* Specifies the signal name relative to its parent module */
  char*      front;        /* Front part of functional unit hierarchy */
  char*      rest;         /* Rest of functional unit hierarchy (minus front) */
 
  /* Find matching binding element that matches the given expression */
  curr = eb_head;
  while( (curr != NULL) && (curr->exp != exp) ) {
    curr = curr->next;
  }

  /*
   If we found the matching expression, find the signal and construct its hierarchical pathname
   relative to its parent module.
  */
  if( curr != NULL ) {
    if( scope_find_signal( curr->name, curr->funit, &found_sig, &found_funit, -1 ) ) {
      if( funit_get_curr_module_safe( curr->funit ) == funit_get_curr_module_safe( found_funit ) ) {
        front = strdup_safe( found_funit->name );
        rest  = strdup_safe( found_funit->name );
        scope_extract_front( found_funit->name, front, rest );
        if( rest[0] != '\0' ) {
          unsigned int sig_size = strlen( curr->name ) + strlen( rest ) + 2;
          unsigned int rv;
          name = (char*)malloc_safe( sig_size );
          rv = snprintf( name, sig_size, "%s.%s", rest, curr->name );
          assert( rv < sig_size );
        }
        free_safe( front, (strlen( found_funit->name ) + 1) );
        free_safe( rest, (strlen( found_funit->name ) + 1) );
      }
    }
    if( name == NULL ) {
      name = strdup_safe( curr->name );
    }
  }

  PROFILE_END;

  return( name );

}

/*!
 \return Returns TRUE if the given name referred to a parameter value that was bound; otherwise,
         returns FALSE.

 Attempts to bind the specified expression to a parameter in the design.  If binding is successful,
 returns TRUE; otherwise, returns FALSE.
*/
static bool bind_param(
  const char* name,          /*!< Name of parameter to bind to */
  expression* exp,           /*!< Pointer to expression to bind parameter to */
  func_unit*  funit_exp,     /*!< Pointer to functional unit containing exp */
  int         exp_line,      /*!< Line number of given expression to bind (for error output purposes only) */
  bool        bind_locally   /*!< Set to TRUE if we are attempting to bind locally */
) { PROFILE(BIND_PARAM);

  bool       retval = FALSE;  /* Return value for this function */
  mod_parm*  found_parm;      /* Pointer to found parameter in design for the given name */
  func_unit* found_funit;     /* Pointer to found functional unit containing given signal */

  /* Skip parameter binding if the name is not local and we are binding locally */
  if( scope_local( name ) || !bind_locally ) {

    /* Search for specified parameter in current functional unit */
    if( scope_find_param( name, funit_exp, &found_parm, &found_funit, exp_line ) ) {

      /* Swap operation type */
      switch( exp->op ) {
        case EXP_OP_SIG      :  exp->op = EXP_OP_PARAM;           break;
        case EXP_OP_SBIT_SEL :  exp->op = EXP_OP_PARAM_SBIT;      break;
        case EXP_OP_MBIT_SEL :  exp->op = EXP_OP_PARAM_MBIT;      break;
        case EXP_OP_MBIT_POS :  exp->op = EXP_OP_PARAM_MBIT_POS;  break;
        case EXP_OP_MBIT_NEG :  exp->op = EXP_OP_PARAM_MBIT_NEG;  break;
        default :
          assert( (exp->op == EXP_OP_SIG)      ||
                  (exp->op == EXP_OP_SBIT_SEL) ||
                  (exp->op == EXP_OP_MBIT_SEL) ||
                  (exp->op == EXP_OP_MBIT_POS) ||
                  (exp->op == EXP_OP_MBIT_NEG) );
          break;
      }

      /* Link the expression to the module parameter */
      exp_link_add( exp, &(found_parm->exps), &(found_parm->exp_size) );

      /* Indicate that we have successfully bound */
      retval = TRUE;

    }

  }

  PROFILE_END;

  return( retval );

}
#endif /* RUNLIB */

/*!
 \return Returns TRUE if bind occurred successfully; otherwise, returns FALSE.

 \throws anonymous expression_set_value
 
 Performs a binding of an expression and signal based on the name of the
 signal.  Looks up signal name in the specified functional unit and sets the expression
 and signal to point to each other.  If the signal is unused, the bind does not occur and
 the function returns a value of FALSE.  If the signal does not exist, it is considered to
 be an implicit signal and a 1-bit signal is created.
*/
bool bind_signal(
  char*       name,            /*!< String name of signal to bind to specified expression */
  expression* exp,             /*!< Pointer to expression to bind */
  func_unit*  funit_exp,       /*!< Pointer to functional unit containing expression */
  bool        fsm_bind,        /*!< If set to TRUE, handling binding for FSM binding */
  bool        cdd_reading,     /*!< If set to TRUE, specifies that we are binding after reading a design from a CDD file (instead of the design files) */
  bool        clear_assigned,  /*!< If set to TRUE, clears signal assigned bit */
  int         exp_line,        /*!< Line of specified expression (when expression is NULL) */
  bool        bind_locally     /*!< If TRUE, only search for specified signal within the same functional unit as this expression */
) { PROFILE(BIND_SIGNAL);

  bool         retval = TRUE;  /* Return value for this function */
  vsignal*     found_sig;      /* Pointer to found signal in design for the given name */
  func_unit*   found_funit;    /* Pointer to found functional unit containing given signal */
  statement*   stmt;           /* Pointer to root statement for the given expression */
  unsigned int rv;             /* Return value from snprintf calls */

  /* Skip signal binding if the name is not local and we are binding locally */
  if( scope_local( name ) || !bind_locally || (!clear_assigned && (exp->op == EXP_OP_PASSIGN)) ) {

    /* Search for specified signal in current functional unit */
    if( !scope_find_signal( name, funit_exp, &found_sig, &found_funit, exp_line ) ) {

      /* If we are binding an FSM, output an error message */
      if( fsm_bind ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find specified FSM signal \"%s\" in module \"%s\" in file %s",
                       obf_sig( name ), obf_funit( funit_exp->name ), obf_file( funit_exp->orig_fname ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;

      /* If the expression is within a generate expression, emit an error message */
      } else if( (exp != NULL) && (exp->suppl.part.gen_expr == 1) ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Generate expression could not find variable (%s), file %s, line %d",
                       obf_sig( name ), obf_file( funit_exp->orig_fname ), exp_line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      /* Otherwise, implicitly create the signal and bind to it if the signal exists on the LHS of the equation */
      } else if( ESUPPL_IS_LHS( exp->suppl ) == 1 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Implicit declaration of signal \"%s\", creating 1-bit version of signal", obf_sig( name ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, WARNING, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "module \"%s\", file \"%s\", line %d",
                       obf_funit( funit_exp->name ), obf_file( funit_exp->orig_fname ), exp_line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
        found_sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, exp->line, exp->col.part.first );
        found_sig->pdim_num   = 1;
        found_sig->dim        = (dim_range*)malloc_safe( sizeof( dim_range ) * 1 );
        found_sig->dim[0].msb = 0;
        found_sig->dim[0].lsb = 0;
        sig_link_add( found_sig, TRUE, &(funit_exp->sigs), &(funit_exp->sig_size), &(funit_exp->sig_no_rm_index) );

      /* Otherwise, don't attempt to bind the signal */
      } else {
        retval = FALSE;
      }

    } else {

      /* If the found signal is not handled, do not attempt to bind to it */
      if( found_sig->suppl.part.not_handled == 1 ) {
        retval = FALSE;
      }

      /*
       If the expression is a generate expression on the LHS and the found signal is not a generate variable, emit an error message
       and exit immediately.
      */
      if( (exp != NULL) && (exp->suppl.part.gen_expr == 1) && (ESUPPL_IS_LHS( exp->suppl ) == 1) && (found_sig->suppl.part.type != SSUPPL_TYPE_GENVAR) ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Attempting to bind an generate expression to a signal that is not a genvar, file %s, line %d",
                       obf_file( funit_exp->orig_fname ), exp_line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

    }

    if( retval ) {

      /* Bind signal and expression if we are not clearing or this is an MBA */
      if( !clear_assigned ) {

        /* Add expression to signal expression list */
        exp_link_add( exp, &(found_sig->exps), &(found_sig->exp_size) );

        /* Set expression to point at signal */
        exp->sig = found_sig;

        /* If this is a port assignment, we need to link the expression and signal together immediately */
        if( exp->op == EXP_OP_PASSIGN ) {
          vector_dealloc( exp->value );
          exp->suppl.part.owns_vec = 0;
          exp->value = found_sig->value;
        }

        if( ((exp->op == EXP_OP_SIG)            ||
             (exp->op == EXP_OP_SBIT_SEL)       ||
             (exp->op == EXP_OP_MBIT_SEL)       ||
             (exp->op == EXP_OP_MBIT_POS)       ||
             (exp->op == EXP_OP_MBIT_NEG)       ||
             (exp->op == EXP_OP_PARAM)          ||
             (exp->op == EXP_OP_PARAM_SBIT)     ||
             (exp->op == EXP_OP_PARAM_MBIT)     ||
             (exp->op == EXP_OP_PARAM_MBIT_POS) ||
             (exp->op == EXP_OP_PARAM_MBIT_NEG) ||
             (exp->op == EXP_OP_TRIGGER)) &&
            (cdd_reading || (found_sig->suppl.part.type == SSUPPL_TYPE_GENVAR)) ) {
          expression_set_value( exp, found_sig, funit_exp );
        }

#ifndef RUNLIB
        /*
         Create a non-blocking assignment handler for the given expression if the attached signal is a memory
         and the expression is assignable on the LHS of a non-blocking assignment operator.  Only perform this
         if we are reading from the CDD file and binding.
        */
        if( cdd_reading && (found_sig->suppl.part.type == SSUPPL_TYPE_MEM) ) {
          expression* nba_exp;
          if( (nba_exp = expression_is_nba_lhs( exp )) != NULL ) {
            expression_create_nba( exp, found_sig, nba_exp->right->value );
          }
        }
#endif /* RUNLIB */

      }

#ifndef RUNLIB
      if( !cdd_reading ) {

        /* Check to see if this signal should be assigned by Covered or the dumpfile */
        if( clear_assigned ) {
          found_sig->suppl.part.assigned = 0;
        }

        if( !clear_assigned &&
            ((exp->op == EXP_OP_SIG)      ||
             (exp->op == EXP_OP_SBIT_SEL) ||
             (exp->op == EXP_OP_MBIT_SEL) ||
             (exp->op == EXP_OP_MBIT_POS) ||
             (exp->op == EXP_OP_MBIT_NEG)) &&
            !ovl_is_assertion_module( funit_exp ) ) {
          expression_set_assigned( exp );
        }

        /* Set signed bits */
        if( !clear_assigned ) {
          expression_set_signed( exp );
        }

        /*
         If the signal is found for the given expression but the signal is marked as "must be assigned" but is also marked as
         "won't be assigned", we need to remove all statement blocks that contain this signal from coverage consideration.
        */
        if( (found_sig->suppl.part.assigned == 0) && (found_sig->suppl.part.mba == 1) ) {
          unsigned int i;
          for( i=0; i<found_sig->exp_size; i++ ) {
            if( (stmt = expression_get_root_statement( found_sig->exps[i] )) != NULL ) {
#ifdef DEBUG_MODE
              if( debug_mode ) {
                unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block %d, line %d because it needed to be assigned but would not be",
                                            stmt->exp->id, stmt->exp->line );
                assert( rv < USER_MSG_LENGTH );
                print_output( user_msg, DEBUG, __FILE__, __LINE__ );
              }
#endif /* DEBUG_MODE */
              stmt_blk_add_to_remove_list( stmt );
            }
          }
        }

      }
#endif /* RUNLIB */

    }

  } else {

    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

#ifndef RUNLIB
/*!
 Binds a given task/function call port parameter to the matching signal in the specified
 task/function.
*/
static void bind_task_function_ports(
  expression* expr,      /*!< Pointer to port expression to potentially bind to the specified port */
  func_unit*  funit,     /*!< Pointer to task/function to bind port list to */
  char*       name,      /*!< Hierachical name of function to bind port list to */
  int*        order,     /*!< Tracks the port order for the current task/function call parameter */
  func_unit*  funit_exp  /*!< Pointer to functional unit containing the given expression */
) { PROFILE(BIND_TASK_FUNCTION_PORTS);

  int  i;               /* Loop iterator */
  bool found;           /* Specifies if we have found a matching port */
  char sig_name[4096];  /* Hierarchical path to matched port signal */

  assert( funit != NULL );

  if( expr != NULL ) {

    /* If the expression is a list, traverse left and right expression trees */
    if( expr->op == EXP_OP_PLIST ) {

      bind_task_function_ports( expr->left,  funit, name, order, funit_exp );
      bind_task_function_ports( expr->right, funit, name, order, funit_exp );

    /* Otherwise, we have found an expression to bind to a port */
    } else {

      unsigned int j = 0;

      assert( expr->op == EXP_OP_PASSIGN );

      /* Find the port that matches our order */
      found = FALSE;
      i     = 0;
      while( (j < funit->sig_size) && !found ) {
        if( (funit->sigs[j]->suppl.part.type == SSUPPL_TYPE_INPUT_NET)  ||
            (funit->sigs[j]->suppl.part.type == SSUPPL_TYPE_INPUT_REG)  ||
            (funit->sigs[j]->suppl.part.type == SSUPPL_TYPE_OUTPUT_NET) ||
            (funit->sigs[j]->suppl.part.type == SSUPPL_TYPE_OUTPUT_REG) ||
            (funit->sigs[j]->suppl.part.type == SSUPPL_TYPE_INOUT_NET)  ||
            (funit->sigs[j]->suppl.part.type == SSUPPL_TYPE_INOUT_REG) ) {
          if( i == *order ) {
            found = TRUE;
          } else {
            i++;
            j++;
          }
        } else {
          j++;
        }
      }

      /*
       If we found our signal to bind to, do it now; otherwise, just skip ahead (the error will be handled by
       the calling function.
      */
      if( j < funit->sig_size ) {

        /* Create signal name to bind */
        unsigned int rv = snprintf( sig_name, 4096, "%s.%s", name, funit->sigs[j]->name );
        assert( rv < 4096 );

        /* Add the signal to the binding list */
        bind_add( 0, sig_name, expr, funit_exp, FALSE );

        /* Specify that this vector will be assigned by Covered and not the dumpfile */
        funit->sigs[j]->suppl.part.assigned = 1;

        /* Increment the port order number */
        (*order)++;

      }

    }

  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \return Returns TRUE if there were no errors in binding the specified expression to the needed
         functional unit; otherwise, returns FALSE to indicate that we had an error.

 \throws anonymous Throw

 Binds an expression to a function/task/named block.
*/
static bool bind_task_function_namedblock(
  int         type,          /*!< Type of functional unit to bind */
  char*       name,          /*!< Name of functional unit to bind */
  expression* exp,           /*!< Pointer to expression containing FUNC_CALL/TASK_CALL operation type to bind */
  func_unit*  funit_exp,     /*!< Pointer to functional unit containing exp */
  bool        cdd_reading,   /*!< Set to TRUE when we are reading from the CDD file (FALSE when parsing) */
  int         exp_line,      /*!< Line number of expression that is being bound (used when exp is NULL) */
  bool        bind_locally,  /*!< If set to TRUE, only attempt to bind a task/function local to the expression functional unit */
  bool        staticf        /*!< If set to TRUE, set the staticf bit on the found functional unit; otherwise, set the normalf
                                  bit on the found functional unit */
) { PROFILE(BIND_TASK_FUNCTION_NAMEDBLOCK);

  bool       retval = FALSE;  /* Return value for this function */
  vsignal*   sig;             /* Temporary signal link holder */
  func_unit* found_funit;     /* Pointer to found task/function functional unit */
  char       rest[4096];      /* Temporary string */
  char       back[4096];      /* Temporary string */
  int        port_order;      /* Port order value */
  int        port_cnt;        /* Number of ports in the found function/task's port list */

  assert( (type == FUNIT_FUNCTION) || (type == FUNIT_TASK) || (type == FUNIT_NAMED_BLOCK) || (type == FUNIT_ANAMED_BLOCK) );

  /* Don't continue if the name is not local and we are told to bind locally */
  if( scope_local( name ) || !bind_locally ) {

    if( scope_find_task_function_namedblock( name, type, funit_exp, &found_funit, exp_line, !bind_locally, 
                                             ((exp->op != EXP_OP_NB_CALL) && (exp->op != EXP_OP_FORK)) ) ) {

      /* Set the static/normal bit in the functional unit as needed */
      if( type == FUNIT_FUNCTION ) {
        if( staticf ) {
          found_funit->suppl.part.staticf = 1;
        } else {
          func_unit* parent_func = funit_get_curr_function( funit_exp );
          if( parent_func != found_funit ) {
            found_funit->suppl.part.normalf = 1;
          }
        }
      }

      exp->elem.funit      = found_funit;
      exp->suppl.part.type = ETYPE_FUNIT;
      retval = (found_funit->suppl.part.type != FUNIT_NO_SCORE);

      if( retval ) {

        /* If this is a function, bind the return value signal */
        if( type == FUNIT_FUNCTION ) {

          scope_extract_back( found_funit->name, back, rest );
          sig = sig_link_find( back, found_funit->sigs, found_funit->sig_size );

          assert( sig != NULL );

          /* Add expression to signal expression list */
          exp_link_add( exp, &(sig->exps), &(sig->exp_size) );

          /* Set expression to point at signal */
          exp->sig = sig;

          /* Make sure that our vector type matches that of the found signal */
          exp->value->suppl.part.data_type = sig->value->suppl.part.data_type;

        }

#ifndef RUNLIB
        /* If this is a function or task, bind the ports as well */
        if( ((type == FUNIT_FUNCTION) || (type == FUNIT_TASK)) && !cdd_reading ) {

          /* First, bind the ports */
          port_order = 0;
          bind_task_function_ports( exp->left, found_funit, name, &port_order, funit_exp );

          /* Check to see if the call port count matches the actual port count */
          if( (port_cnt = funit_get_port_count( found_funit )) != port_order ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Number of arguments in %s call (%d) does not match its %s port list (%d), file %s, line %u",
                                        get_funit_type( type ), port_order, get_funit_type( type ), port_cnt, obf_file( funit_exp->orig_fname ), exp->line );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }

        }
#endif

      }

    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \throws anonymous Throw param_resolve bind_signal generate_resolve bind_task_function_namedblock bind_task_function_namedblock

 In the process of binding, we go through each element of the binding list,
 finding the signal to be bound in the specified tree, adding the expression
 to the signal's expression pointer list, and setting the expression vector pointer
 to point to the signal vector.
*/
void bind_perform(
  bool cdd_reading,  /*!< Set to TRUE if we are binding after reading the CDD file; otherwise, set to FALSE */
  int  pass          /*!< Specifies the starting pass to perform (setting this to 1 will bypass resolutions) */
) { PROFILE(BIND_PERFORM);
  
  exp_bind*  curr_eb;   /* Pointer to current expression bind structure */
  int        id;        /* Current expression id -- used for removal */
  bool       bound;     /* Specifies if the current expression was successfully bound or not */
  statement* tmp_stmt;  /* Pointer to temporary statement */

  Try {

    /* Make three passes through binding list, 0=local signal/param bindings, 1=remote signal/param bindings */
    for( ; pass<2; pass++ ) {

      curr_eb = eb_head;
      while( curr_eb != NULL ) {

        /* Figure out ID to clear from the binding list after the bind occurs */
        if( curr_eb->clear_assigned == 0 ) {
          id = curr_eb->exp->id;
        } else {
          id = curr_eb->clear_assigned;
        }

        /* If the expression has already been bound, do not attempt to do it again */
        if( (curr_eb->exp != NULL) && (curr_eb->exp->name != NULL) ) {

          bound = TRUE;

        } else {

          /* Handle signal/parameter binding */
          if( curr_eb->type == 0 ) {

            /* Attempt to bind the expression to a parameter; otherwise, bind to a signal */
#ifndef RUNLIB
            if( !(bound = bind_param( curr_eb->name, curr_eb->exp, curr_eb->funit, curr_eb->line, (pass == 0) )) ) {
#endif /* RUNLIB */
              bound = bind_signal( curr_eb->name, curr_eb->exp, curr_eb->funit, FALSE, cdd_reading,
                                   (curr_eb->clear_assigned > 0), curr_eb->line, (pass == 0) );
#ifndef RUNLIB
            }
#endif /* RUNLIB */

            /* If an FSM expression is attached, size it now */
            if( curr_eb->fsm != NULL ) {
              curr_eb->fsm->value = vector_create( curr_eb->exp->value->width, VTYPE_EXP, VDATA_UL, TRUE );
            }

          /* Otherwise, handle disable binding */
          } else if( curr_eb->type == 1 ) {

            /* Attempt to bind a named block -- if unsuccessful, attempt to bind with a task */
            if( !(bound = bind_task_function_namedblock( FUNIT_NAMED_BLOCK, curr_eb->name, curr_eb->exp, curr_eb->funit,
                                                         cdd_reading, curr_eb->line, (pass == 0), FALSE )) ) {
              bound = bind_task_function_namedblock( FUNIT_TASK, curr_eb->name, curr_eb->exp, curr_eb->funit,
                                                     cdd_reading, curr_eb->line, (pass == 0), FALSE );
            }

          /* Otherwise, handle function/task binding */
          } else {

            /*
             Bind the expression to the task/function.  If it is unsuccessful, we need to remove the statement
             that this expression is a part of.
            */
            bound = bind_task_function_namedblock( curr_eb->type, curr_eb->name, curr_eb->exp, curr_eb->funit,
                                                   cdd_reading, curr_eb->line, (pass == 0), curr_eb->staticf );

          }

          /* If we have bound successfully, copy the name of this exp_bind to the expression */
          if( bound && (curr_eb->exp != NULL) ) {
            curr_eb->exp->name = strdup_safe( curr_eb->name );
          }

        }

        /*
         If the expression was unable to be bound, put its statement block in a list to be removed after
         binding has been completed.
        */
        if( !bound && (curr_eb->clear_assigned == 0) && (pass == 1) ) {
          if( (tmp_stmt = expression_get_root_statement( curr_eb->exp )) != NULL ) {
#ifndef RUNLIB
#ifdef DEBUG_MODE
            if( debug_mode ) {
              unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block containing line %d in file \"%s\", because it was unbindable",
                                          curr_eb->exp->line, obf_file( curr_eb->funit->orig_fname ) );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, DEBUG, __FILE__, __LINE__ );
            }
#endif /* DEBUG_MODE */
            stmt_blk_add_to_remove_list( tmp_stmt );
#else
            assert( 0 );  /* I don't believe that we should ever get here when running with the RUNLIB */
#endif /* RUNLIB */
          }
        }

        curr_eb = curr_eb->next;

        /* Remove this from the binding list */
        if( bound ) {
          bind_remove( id, FALSE );
        }

      }

#ifndef VPI_ONLY
#ifndef RUNLIB
      /* If we are in parse mode, resolve all parameters and arrays of instances now */
      if( !cdd_reading && (pass == 0) ) {
        inst_link* instl = db_list[curr_db]->inst_head;
        while( instl != NULL ) {
          instance_resolve( instl->inst );
          instl = instl->next;
        }
      }
#endif /* RUNLIB */
#endif /* VPI_ONLY */

    }

  } Catch_anonymous {
    exp_bind* tmp_eb;
    curr_eb = eb_head;
    while( curr_eb != NULL ) {
      tmp_eb  = curr_eb;
      curr_eb = curr_eb->next;
      free_safe( tmp_eb->name, (strlen( tmp_eb->name ) + 1) );
      free_safe( tmp_eb, sizeof( exp_bind ) );
    }
    eb_head = eb_tail = NULL;
    Throw 0;
  }

  PROFILE_END;

}

/*!
 Deallocates all memory used for the storage of the binding list.
*/
void bind_dealloc() { PROFILE(BIND_DEALLOC);

  exp_bind* tmp;  /* Temporary binding pointer */

  while( eb_head != NULL ) {

    tmp     = eb_head;
    eb_head = tmp->next;

    /* Deallocate the name, if specified */
    if( tmp->name != NULL ) {
      free_safe( tmp->name, (strlen( tmp->name ) + 1) );
    }

    /* Deallocate this structure */
    free_safe( tmp, sizeof( exp_bind ) );

  }

  /* Reset the head and tail pointers */
  eb_head = eb_tail = NULL;

  PROFILE_END;

}

