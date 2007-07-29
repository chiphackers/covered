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
 \file     binding.c
 \author   Trevor Williams  (trevorw@charter.net)
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
#include "iter.h"
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


extern inst_link*  inst_head;
extern funit_link* funit_head;
extern char        user_msg[USER_MSG_LENGTH];
extern bool        debug_mode;


/*!
 Pointer to the head of the signal/functional unit/expression binding list.
*/
exp_bind* eb_head;

/*!
 Pointer to the tail of the signal/functional unit/expression binding list.
*/
exp_bind* eb_tail;


/*!
 \param type   Type of thing being bound with the specified expression (0=signal, 1=functional unit)
 \param name   Signal/Function/Task scope to bind.
 \param exp    Expression ID to bind.
 \param funit  Pointer to module containing specified expression.

 Adds the specified signal/function/task and expression to the bindings linked list.
 This bindings list will be handled after all input Verilog has been
 parsed.
*/
void bind_add( int type, const char* name, expression* exp, func_unit* funit ) {
  
  exp_bind* eb;   /* Temporary pointer to signal/expressing binding */

  assert( exp != NULL );
  
  /* Create new signal/expression binding */
  eb                 = (exp_bind *)malloc_safe( sizeof( exp_bind ), __FILE__, __LINE__ );
  eb->type           = type;
  eb->name           = strdup_safe( name, __FILE__, __LINE__ );
  eb->clear_assigned = 0;
  eb->line           = exp->line;
  eb->funit          = funit;
  eb->exp            = exp;
  eb->fsm            = NULL;
  eb->next           = NULL;
  
  /* Add new signal/expression binding to linked list */
  if( eb_head == NULL ) {
    eb_head = eb_tail = eb;
  } else {
    eb_tail->next = eb;
    eb_tail       = eb;
  }
  
}

/*!
 \param fsm_exp     Expression pertaining to an FSM input state that needs to be sized when
                    its associated expression is bound to its signal
 \param exp         Expression to match
 \param curr_funit  Functional unit that the FSM expression resides in (this will be the same
                    functional unit as the expression functional unit).

 Searches the expression binding list for the entry that matches the given exp and curr_funit
 parameters.  When the entry is found, the FSM expression is added to the exp_bind structure
 to be sized when the expression is bound.
*/
void bind_append_fsm_expr( expression* fsm_exp, expression* exp, func_unit* curr_funit ) {

  exp_bind* curr;

  curr = eb_head;
  while( (curr != NULL) && ((exp != curr->exp) || (curr_funit != curr->funit)) ) {
    curr = curr->next;
  }

  assert( curr != NULL );

  curr->fsm = fsm_exp;

}

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
        printf( "  Expr: %d, %s, line %d;  Functional Unit: %s;  Function: %s\n",
                curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        break;
      case FUNIT_ATASK :
      case FUNIT_TASK :
        printf( "  Expr: %d, %s, line %d;  Functional Unit: %s;  Task: %s\n",
                curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK :
        printf( "  Expr: %d, %s, line %d;  Functional Unit: %s;  Named Block: %s\n",
                curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        break;
      case 0 :
        if( curr->clear_assigned > 0 ) {
          printf( "  Signal to be cleared: %s\n", obf_sig( curr->name ) );
        } else {
          printf( "  Expr: %d, %s, line %d;  Functional Unit: %s;  Signal: %s\n",
                  curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line,
                  obf_funit( curr->funit->name ), obf_sig( curr->name ) );
        }
        break;
      default :  break;
    }

    curr = curr->next;

  }

}

/*!
 \param id              Expression ID of binding to remove.
 \param clear_assigned  If set to TRUE, clears the assigned bit in the specified expression.

 Removes the binding containing the expression ID of id.  This needs to
 be called before an expression is removed.
*/
void bind_remove( int id, bool clear_assigned ) {

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
        free_safe( curr->name );
        free_safe( curr );

      }

      curr = NULL;
      
    } else {

      last = curr;
      curr = curr->next;

    }

  }

}

/*!
 \param exp  Pointer to expression to search for

 \return Returns the name of the signal to be bound with the given expression (if one exists);
         otherwise, returns NULL if no match was found.
*/
char* bind_find_sig_name( expression* exp ) {

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
      if( funit_get_curr_module( curr->funit ) == funit_get_curr_module( found_funit ) ) {
        front = strdup_safe( found_funit->name, __FILE__, __LINE__ );
        rest  = strdup_safe( found_funit->name, __FILE__, __LINE__ );
        scope_extract_front( found_funit->name, front, rest );
        if( rest[0] != '\0' ) {
          name = (char*)malloc_safe( (strlen( curr->name ) + strlen( rest ) + 2), __FILE__, __LINE__ );
          snprintf( name, (strlen( curr->name ) + strlen( found_funit->name ) + 2), "%s.%s", rest, curr->name );
        }
        free_safe( front );
        free_safe( rest );
      }
    }
    if( name == NULL ) {
      name = strdup_safe( curr->name, __FILE__, __LINE__ );
    }
  }

  return( name );

}

/*!
 \param name          Name of parameter to bind to
 \param exp           Pointer to expression to bind parameter to
 \param funit_exp     Pointer to functional unit containing exp
 \param exp_line      Line number of given expression to bind (for error output purposes only)
 \param bind_locally  Set to TRUE if we are attempting to bind locally.

 \return Returns TRUE if the given name referred to a parameter value that was bound; otherwise,
         returns FALSE.

 Attempts to bind the specified expression to a parameter in the design.  If binding is successful,
 returns TRUE; otherwise, returns FALSE.
*/
bool bind_param( char* name, expression* exp, func_unit* funit_exp, int exp_line, bool bind_locally ) {

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
      exp_link_add( exp, &(found_parm->exp_head), &(found_parm->exp_tail) );

      /* Indicate that we have successfully bound */
      retval = TRUE;

    }

  }

  return( retval );

}

/*!
 \param name            String name of signal to bind to specified expression.
 \param exp             Pointer to expression to bind.
 \param funit_exp       Pointer to functional unit containing expression.
 \param fsm_bind        If set to TRUE, handling binding for FSM binding.
 \param cdd_reading     If set to TRUE, specifies that we are binding after reading a design from a CDD file (instead of the design files).
 \param clear_assigned  If set to TRUE, clears signal assigned bit.
 \param exp_line        Line of specified expression (when expression is NULL)
 \param bind_locally    If TRUE, only search for specified signal within the same functional unit as this expression

 \return Returns TRUE if bind occurred successfully; otherwise, returns FALSE.
 
 Performs a binding of an expression and signal based on the name of the
 signal.  Looks up signal name in the specified functional unit and sets the expression
 and signal to point to each other.  If the signal is unused, the bind does not occur and
 the function returns a value of FALSE.  If the signal does not exist, it is considered to
 be an implicit signal and a 1-bit signal is created.
*/
bool bind_signal( char* name, expression* exp, func_unit* funit_exp, bool fsm_bind, bool cdd_reading,
                  bool clear_assigned, int exp_line, bool bind_locally ) {

  bool       retval = TRUE;  /* Return value for this function */
  vsignal*   found_sig;      /* Pointer to found signal in design for the given name */
  func_unit* found_funit;    /* Pointer to found functional unit containing given signal */
  statement* stmt;           /* Pointer to root statement for the given expression */
  exp_link*  expl;           /* Pointer to current expression link */

  /* Skip signal binding if the name is not local and we are binding locally */
  if( scope_local( name ) || !bind_locally || (!clear_assigned && (exp->op == EXP_OP_PASSIGN)) ) {

    /* Search for specified signal in current functional unit */
    if( !scope_find_signal( name, funit_exp, &found_sig, &found_funit, exp_line ) ) {

      /* If we are binding an FSM, output an error message */
      if( fsm_bind ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Unable to find specified FSM signal \"%s\" in module \"%s\" in file %s",
                  obf_sig( name ),
                  obf_funit( funit_exp->name ),
                  obf_file( funit_exp->filename ) );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;

      /* Otherwise, implicitly create the signal and bind to it */
      } else {
        assert( exp != NULL );
        snprintf( user_msg, USER_MSG_LENGTH, "Implicit declaration of signal \"%s\", creating 1-bit version of signal", obf_sig( name ) );
        print_output( user_msg, WARNING, __FILE__, __LINE__ );
        snprintf( user_msg, USER_MSG_LENGTH, "module \"%s\", file \"%s\", line %d",
                  obf_funit( funit_exp->name ), obf_file( funit_exp->filename ), exp_line );
        print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
        found_sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, exp->line, ((exp->col >> 16) & 0xffff) );
        found_sig->pdim_num   = 1;
        found_sig->dim        = (dim_range*)malloc_safe( (sizeof( dim_range ) * 1), __FILE__, __LINE__ );
        found_sig->dim[0].msb = 0;
        found_sig->dim[0].lsb = 0;
        sig_link_add( found_sig, &(funit_exp->sig_head), &(funit_exp->sig_tail) );
      }

    } else {

      /* If the found signal is not handled, do not attempt to bind to it */
      if( found_sig->suppl.part.not_handled == 1 ) {
        retval = FALSE;
      }

    }

    if( retval ) {

      /* Bind signal and expression if we are not clearing or this is an MBA */
      if( !clear_assigned ) {

        /* Add expression to signal expression list */
        exp_link_add( exp, &(found_sig->exp_head), &(found_sig->exp_tail) );

        /* Set expression to point at signal */
        exp->sig = found_sig;

        /* If this is a port assignment, we need to link the expression and signal together immediately */
        if( exp->op == EXP_OP_PASSIGN ) {
          vector_dealloc( exp->value );
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
          expression_set_value( exp, found_sig );
        }

      }

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
          expl = found_sig->exp_head;
          while( expl != NULL ) {
            if( (stmt = expression_get_root_statement( expl->exp )) != NULL ) {
#ifdef DEBUG_MODE
              snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block %d, line %d because it needed to be assigned but would not be",
                        stmt->exp->id, stmt->exp->line );
              print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
              stmt_blk_add_to_remove_list( stmt );
            }
            expl = expl->next;
          }
        }

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param expr       Pointer to port expression to potentially bind to the specified port
 \param funit      Pointer to task/function to bind port list to
 \param name       Hierachical name of function to bind port list to
 \param order      Tracks the port order for the current task/function call parameter
 \param funit_exp  Pointer to functional unit containing the given expression

 Binds a given task/function call port parameter to the matching signal in the specified
 task/function.
*/
void bind_task_function_ports( expression* expr, func_unit* funit, char* name, int* order, func_unit* funit_exp ) {

  sig_link* sigl;            /* Pointer to current signal link to examine */
  int       i;               /* Loop iterator */
  bool      found;           /* Specifies if we have found a matching port */
  char      sig_name[4096];  /* Hierarchical path to matched port signal */

  assert( funit != NULL );

  if( expr != NULL ) {

    /* If the expression is a list, traverse left and right expression trees */
    if( expr->op == EXP_OP_LIST ) {

      bind_task_function_ports( expr->left,  funit, name, order, funit_exp );
      bind_task_function_ports( expr->right, funit, name, order, funit_exp );

    /* Otherwise, we have found an expression to bind to a port */
    } else {

      assert( expr->op == EXP_OP_PASSIGN );

      /* Find the port that matches our order */
      found = FALSE;
      i     = 0;
      sigl  = funit->sig_head;
      while( (sigl != NULL) && !found ) {
        if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_INPUT)  ||
            (sigl->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT) ||
            (sigl->sig->suppl.part.type == SSUPPL_TYPE_INOUT) ) {
          if( i == *order ) {
            found = TRUE;
          } else {
            i++;
            sigl = sigl->next;
          }
        } else {
          sigl = sigl->next;
        }
      }

      /*
       If we found our signal to bind to, do it now; otherwise, just skip ahead (the error will be handled by
       the calling function.
      */
      if( sigl != NULL ) {

        /* Create signal name to bind */
        snprintf( sig_name, 4096, "%s.%s", name, sigl->sig->name );

        /* Add the signal to the binding list */
        bind_add( 0, sig_name, expr, funit_exp );

        /* Specify that this vector will be assigned by Covered and not the dumpfile */
        sigl->sig->suppl.part.assigned = 1;

        /* Increment the port order number */
        (*order)++;

      }

    }

  }

}

/*!
 \param type          Type of functional unit to bind
 \param name          Name of functional unit to bind
 \param exp           Pointer to expression containing FUNC_CALL/TASK_CALL operation type to bind
 \param funit_exp     Pointer to functional unit containing exp
 \param cdd_reading   Set to TRUE when we are reading from the CDD file (FALSE when parsing)
 \param exp_line      Line number of expression that is being bound (used when exp is NULL)
 \param bind_locally  If set to TRUE, only attempt to bind a task/function local to the expression functional unit

 \return Returns TRUE if there were no errors in binding the specified expression to the needed
         functional unit; otherwise, returns FALSE to indicate that we had an error.

 Binds an expression to a function/task/named block.
*/
bool bind_task_function_namedblock( int type, char* name, expression* exp, func_unit* funit_exp,
                                    bool cdd_reading, int exp_line, bool bind_locally ) {

  bool       retval = FALSE;  /* Return value for this function */
  vsignal    sig;             /* Temporary signal for comparison purposes */
  sig_link*  sigl;            /* Temporary signal link holder */
  func_unit* found_funit;     /* Pointer to found task/function functional unit */
  char       rest[4096];      /* Temporary string */
  char       back[4096];      /* Temporary string */
  int        port_order;      /* Port order value */
  int        port_cnt;        /* Number of ports in the found function/task's port list */

  assert( (type == FUNIT_FUNCTION)    || (type == FUNIT_AFUNCTION) ||
          (type == FUNIT_TASK)        || (type == FUNIT_ATASK)     ||
          (type == FUNIT_NAMED_BLOCK) || (type == FUNIT_ANAMED_BLOCK) );

  /* Don't continue if the name is not local and we are told to bind locally */
  if( scope_local( name ) || !bind_locally ) {

    if( scope_find_task_function_namedblock( name, type, funit_exp, &found_funit, exp_line, !bind_locally, 
                                             ((exp->op != EXP_OP_NB_CALL) && (exp->op != EXP_OP_FORK)) ) ) {

      exp->elem.funit      = found_funit;
      exp->suppl.part.type = ETYPE_FUNIT;
      retval = (found_funit->type != FUNIT_NO_SCORE);

      if( retval ) {

        /* If this is a function, bind the return value signal */
        if( type == FUNIT_FUNCTION ) {

          scope_extract_back( found_funit->name, back, rest );
          sig.name = back;
          sigl     = sig_link_find( &sig, found_funit->sig_head );

          assert( sigl != NULL );

          /* Add expression to signal expression list */
          exp_link_add( exp, &(sigl->sig->exp_head), &(sigl->sig->exp_tail) );

          /* Set expression to point at signal */
          exp->sig = sigl->sig;

        }

        /* If this is a function or task, bind the ports as well */
        if( ((type == FUNIT_FUNCTION) || (type == FUNIT_TASK)) && !cdd_reading ) {

          /* First, bind the ports */
          port_order = 0;
          bind_task_function_ports( exp->left, found_funit, name, &port_order, funit_exp );

          /* Check to see if the call port count matches the actual port count */
          if( (port_cnt = funit_get_port_count( found_funit )) != port_order ) {
            snprintf( user_msg, USER_MSG_LENGTH, "Number of arguments in %s call (%d) does not match its %s port list (%d), file %s, line %d",
                      get_funit_type( type ), port_order, get_funit_type( type ), port_cnt, obf_file( funit_exp->filename ), exp->line );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            exit( 1 );
          }

        }

      }

    }

  }

  return( retval );

}

/*!
 \param cdd_reading  Set to TRUE if we are binding after reading the CDD file; otherwise, set to FALSE.
 \param pass         Specifies the starting pass to perform (setting this to 1 will bypass resolutions).

 In the process of binding, we go through each element of the binding list,
 finding the signal to be bound in the specified tree, adding the expression
 to the signal's expression pointer list, and setting the expression vector pointer
 to point to the signal vector.
*/
void bind_perform( bool cdd_reading, int pass ) {
  
  exp_bind*  curr_eb;   /* Pointer to current expression bind structure */
  int        id;        /* Current expression id -- used for removal */
  bool       bound;     /* Specifies if the current expression was successfully bound or not */
  statement* tmp_stmt;  /* Pointer to temporary statement */

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

      /* Handle signal/parameter binding */
      if( curr_eb->type == 0 ) {

        /* Attempt to bind the expression to a parameter; otherwise, bind to a signal */
        if( !(bound = bind_param( curr_eb->name, curr_eb->exp, curr_eb->funit, curr_eb->line, (pass == 0) )) ) {
          bound = bind_signal( curr_eb->name, curr_eb->exp, curr_eb->funit, FALSE, cdd_reading,
                               (curr_eb->clear_assigned > 0), curr_eb->line, (pass == 0) );
        }

        /* If an FSM expression is attached, size it now */
        if( curr_eb->fsm != NULL ) {
          curr_eb->fsm->value = vector_create( curr_eb->exp->value->width, VTYPE_EXP, TRUE );
        }

      /* Otherwise, handle disable binding */
      } else if( curr_eb->type == 1 ) {

        /* Attempt to bind a named block -- if unsuccessful, attempt to bind with a task */
        if( !(bound = bind_task_function_namedblock( FUNIT_NAMED_BLOCK, curr_eb->name, curr_eb->exp, curr_eb->funit,
                                                     cdd_reading, curr_eb->line, (pass == 0) )) ) {
          bound = bind_task_function_namedblock( FUNIT_TASK, curr_eb->name, curr_eb->exp, curr_eb->funit,
                                                 cdd_reading, curr_eb->line, (pass == 0) );
        }

      /* Otherwise, handle function/task binding */
      } else {

        /*
         Bind the expression to the task/function.  If it is unsuccessful, we need to remove the statement
         that this expression is a part of.
        */
        bound = bind_task_function_namedblock( curr_eb->type, curr_eb->name, curr_eb->exp, curr_eb->funit,
                                               cdd_reading, curr_eb->line, (pass == 0) );

      }

      /* If we have bound successfully, copy the name of this exp_bind to the expression */
      if( bound && (curr_eb->exp != NULL) ) {
        curr_eb->exp->name = strdup_safe( curr_eb->name, __FILE__, __LINE__ );
      }

      /*
       If the expression was unable to be bound, put its statement block in a list to be removed after
       binding has been completed.
      */
      if( !bound && (curr_eb->clear_assigned == 0) && (pass == 1) ) {
        if( (tmp_stmt = expression_get_root_statement( curr_eb->exp )) != NULL ) {
#ifdef DEBUG_MODE
          snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block containing line %d in file \"%s\", because it was unbindable",
                    curr_eb->exp->line, obf_file( curr_eb->funit->filename ) );
          print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif        
          stmt_blk_add_to_remove_list( tmp_stmt );
        }
      }

      curr_eb = curr_eb->next;

      /* Remove this from the binding list */
      if( bound ) {
        bind_remove( id, FALSE );
      }

    }

#ifndef VPI_ONLY
    /* If we are in parse mode, resolve all parameters and arrays of instances now */
    if( !cdd_reading && (pass == 0) ) {
      inst_link* instl;
#ifdef DEBUG_MODE
      if( debug_mode ) {
        print_output( "Resolving parameters...", DEBUG, __FILE__, __LINE__ );
      }
#endif
      instl = inst_head;
      while( instl != NULL ) {
        param_resolve( instl->inst );
        instl = instl->next;
      }
#ifdef DEBUG_MODE
      if( debug_mode ) {
        print_output( "Resolving generate statements...", DEBUG, __FILE__, __LINE__ );
      }
#endif
      instl = inst_head;
      while( instl != NULL ) {
        generate_resolve( instl->inst );
        instl = instl->next;
      }
#ifdef DEBUG_MODE
      if( debug_mode ) {
        print_output( "Resolving arrays of instances...", DEBUG, __FILE__, __LINE__ );
      }
#endif
      instl = inst_head;
      while( instl != NULL ) {
        instance_resolve( instl->inst );
        instl = instl->next;
      }
    }
#endif

  }

}

/*!
 Deallocates all memory used for the storage of the binding list.
*/
void bind_dealloc() {

  exp_bind* tmp;  /* Temporary binding pointer */

  while( eb_head != NULL ) {

    tmp     = eb_head;
    eb_head = tmp->next;

    /* Deallocate the name, if specified */
    if( tmp->name != NULL ) {
      free_safe( tmp->name );
    }

    /* Deallocate this structure */
    free_safe( tmp );

  }

  /* Reset the head and tail pointers */
  eb_head = eb_tail = NULL;

}

/* 
 $Log$
 Revision 1.109  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.108  2007/03/16 21:41:07  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.107  2007/03/15 22:39:05  phase1geo
 Fixing bug in unnamed scope binding.

 Revision 1.106  2006/12/19 05:23:38  phase1geo
 Added initial code for handling instance flattening for unnamed scopes.  This
 is partially working at this point but still needs some debugging.  Checkpointing.

 Revision 1.105  2006/12/19 02:36:18  phase1geo
 Fixing error in parser when parsing begin..end blocks.  Still need to properly
 handle unnamed scopes.  Checkpointing.

 Revision 1.104  2006/12/14 23:46:56  phase1geo
 Fixing remaining compile issues with support for functional unit pointers in
 expressions and unnamed scope handling.  Starting to debug run-time issues now.
 Added atask1 diagnostic to begin this verification process.  Checkpointing.

 Revision 1.103  2006/12/12 06:20:22  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.102  2006/10/12 22:48:45  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.101  2006/10/09 17:54:18  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.100  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.99  2006/09/26 22:36:37  phase1geo
 Adding code for memory coverage to GUI and related files.  Lots of work to go
 here so we are checkpointing for the moment.

 Revision 1.98  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.97  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.96  2006/09/22 04:23:04  phase1geo
 More fixes to support new signal range structure.  Still don't have full
 regressions passing at the moment.

 Revision 1.95  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.94  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.93  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.92  2006/09/06 22:09:22  phase1geo
 Fixing bug with multiply-and-op operation.  Also fixing bug in gen_item_resolve
 function where an instance was incorrectly being placed into a new instance tree.
 Full regression passes with these changes.  Also removed verbose output.

 Revision 1.91  2006/09/05 21:00:44  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.90  2006/09/01 04:06:36  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.89  2006/08/18 22:07:44  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.88  2006/08/11 15:16:48  phase1geo
 Joining slist3.3 diagnostic to latest development branch.  Adding changes to
 fix memory issues from bug 1535412.

 Revision 1.87  2006/08/10 22:35:13  phase1geo
 Updating with fixes for upcoming 0.4.7 stable release.  Updated regressions
 for this change.  Full regression still fails due to an unrelated issue.

 Revision 1.86  2006/08/02 22:28:31  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.85  2006/08/01 04:38:20  phase1geo
 Fixing issues with binding to non-module scope and not binding references
 that reference a "no score" module.  Full regression passes.

 Revision 1.84  2006/07/31 22:11:07  phase1geo
 Fixing bug with generated tasks.  Added diagnostic to test generate functions
 (this is currently failing with a binding issue).

 Revision 1.83  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.82  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.81  2006/07/22 03:57:07  phase1geo
 Adding support for parameters within generate blocks.  Adding more diagnostics
 to verify statement support and parameter usage (signal sizing).

 Revision 1.80  2006/07/21 20:12:46  phase1geo
 Fixing code to get generated instances and generated array of instances to
 work.  Added diagnostics to verify correct functionality.  Full regression
 passes.

 Revision 1.79  2006/07/20 20:11:08  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.78  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.77  2006/07/11 04:59:08  phase1geo
 Reworking the way that instances are being generated.  This is to fix a bug and
 pave the way for generate loops for instances.  Code not working at this point
 and may cause serious problems for regression runs.

 Revision 1.76  2006/05/29 23:47:44  phase1geo
 Adding more diagnostics to verify assertion coverage handling.  Fixed bug
 to force all signals within an OVL module to be assigned only by the dumpfile
 results (to ensure accuracy for assertion coverage).  Full regression passes
 and we should now be ready to release the next development release once the
 VCS full regression has been updated and passed.

 Revision 1.75  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.74  2006/05/25 12:10:57  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.73  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.72  2006/04/07 03:47:50  phase1geo
 Fixing run-time issues with VPI.  Things are running correctly now with IV.

 Revision 1.71.4.1.4.3  2006/05/27 05:56:14  phase1geo
 Fixing last problem with bug fix.  Updated date on NEWS.

 Revision 1.71.4.1.4.2  2006/05/26 22:35:18  phase1geo
 More fixes to parameter binding to fix broken diagnostic run.  Updated
 vcs diagnostics.

 Revision 1.71.4.1.4.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.71.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.71  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.70  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.69  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.68  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.67  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.66  2006/01/25 16:51:26  phase1geo
 Fixing performance/output issue with hierarchical references.  Added support
 for hierarchical references to parser.  Full regression passes.

 Revision 1.65  2006/01/25 04:32:46  phase1geo
 Fixing bug with latest checkins.  Full regression is now passing for IV simulated
 diagnostics.

 Revision 1.64  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.63  2006/01/23 22:55:10  phase1geo
 Updates to fix constant function support.  There is some issues to resolve
 here but full regression is passing with the exception of the newly added
 static_func1.1 diagnostic.  Fixed problem where expand and multi-bit expressions
 were getting coverage numbers calculated for them before they were simulated.

 Revision 1.62  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.61  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.60  2006/01/20 22:44:51  phase1geo
 Moving parameter resolution to post-bind stage to allow static functions to
 be considered.  Regression passes without static function testing.  Static
 function support still has some work to go.  Checkpointing.

 Revision 1.59  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.58  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.57  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.56  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.55  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.54  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.53  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.52  2005/12/05 23:30:35  phase1geo
 Adding support for disabling tasks.  Full regression passes.

 Revision 1.51  2005/12/05 22:02:24  phase1geo
 Added initial support for disable expression.  Added test to verify functionality.
 Full regression passes.

 Revision 1.50  2005/12/05 21:28:07  phase1geo
 Getting fork statements with scope to work.  Added test to regression to verify
 this functionality.  Fixed bug in binding expression to named block.

 Revision 1.49  2005/12/05 20:26:55  phase1geo
 Fixing bugs in code to remove statement blocks that are pointed to by expressions
 in NB_CALL and FORK cases.  Fixed bugs in fork code -- this is now working at the
 moment.  Updated regressions which now fully pass.

 Revision 1.48  2005/12/02 19:58:36  phase1geo
 Added initial support for FORK/JOIN expressions.  Code is not working correctly
 yet as we need to determine if a statement should be done in parallel or not.

 Revision 1.47  2005/12/02 12:03:17  phase1geo
 Adding support for excluding functions, tasks and named blocks.  Added tests
 to regression suite to verify this support.  Full regression passes.

 Revision 1.46  2005/11/30 18:25:55  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.45  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.44  2005/11/29 19:04:47  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.43  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

 Revision 1.42  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.41  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.40  2005/11/22 16:46:27  phase1geo
 Fixed bug with clearing the assigned bit in the binding phase.  Full regression
 now runs cleanly.

 Revision 1.39  2005/11/22 05:30:33  phase1geo
 Updates to regression suite for clearing the assigned bit when a statement
 block is removed from coverage consideration and it is assigning that signal.
 This is not fully working at this point.

 Revision 1.38  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.37  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.36  2005/11/16 05:41:31  phase1geo
 Fixing implicit signal creation in binding functions.

 Revision 1.35  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.34  2005/11/11 22:53:40  phase1geo
 Updated bind process to allow binding of structures from different hierarchies.
 Added task port signals to get added.

 Revision 1.33  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.

 Revision 1.32  2005/11/10 19:28:22  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.31  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.30  2005/02/09 14:12:20  phase1geo
 More code for supporting expression assignments.

 Revision 1.29  2005/02/08 23:18:22  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.28  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.27  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.26  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.25  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.24  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.23  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.22  2003/02/18 13:35:51  phase1geo
 Updates for RedHat8.0 compilation.

 Revision 1.21  2003/01/05 22:25:22  phase1geo
 Fixing bug with declared integers, time, real, realtime and memory types where
 they are confused with implicitly declared signals and given 1-bit value types.
 Updating regression for changes.

 Revision 1.20  2002/12/13 16:49:45  phase1geo
 Fixing infinite loop bug with statement set_stop function.  Removing
 hierarchical references from scoring (same problem as defparam statement).
 Fixing problem with checked in version of param.c and fixing error output
 in bind() function to be more meaningful to user.

 Revision 1.19  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.18  2002/10/31 23:13:18  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.17  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.16  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.15  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.14  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.13  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.12  2002/07/20 22:22:52  phase1geo
 Added ability to create implicit signals for local signals.  Added implicit1.v
 diagnostic to test for correctness.  Full regression passes.  Other tweaks to
 output information.

 Revision 1.11  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.10  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.9  2002/07/16 00:05:31  phase1geo
 Adding support for replication operator (EXPAND).  All expressional support
 should now be available.  Added diagnostics to test replication operator.
 Rewrote binding code to be more efficient with memory use.

 Revision 1.8  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).
*/
