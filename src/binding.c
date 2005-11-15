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
 expressions which have an operation type of EXP_OP_SIG, EXP_OP_SBIT_SEL, and
 EXP_OP_MBIT_SEL have pointers to signals.  These pointers are used for quick
 retrieval of the signal name when outputting expressions.

 \par
 Because both signals and expressions point to each other, we say that signals and
 expressions need to be bound to each other.  The process of binding takes place at
 two different points in the scoring process:

 \par
 -# When an expression is parsed and the signal that it needs to point to is
    a signal that is local to the current module that the expression exists in.
    If the signal is not local (a hierarchical reference), we cannot bind at this
    time because the signal may be referring to a signal in a module which has not
    been parsed yet.  Because of this, binding also occurs in the second point of
    the score command.
 -# After all parsing has been performed, all signals and expressions which have
    not been bound at point 1 are now bound.

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

#include "defines.h"
#include "binding.h"
#include "vsignal.h"
#include "expr.h"
#include "instance.h"
#include "link.h"
#include "util.h"
#include "vector.h"
#include "param.h"
#include "statement.h"


extern funit_inst* instance_root;
extern funit_link* funit_head;
extern char        user_msg[USER_MSG_LENGTH];

/*!
 Pointer to the head of the signal/functional unit/expression binding list.
*/
exp_bind* eb_head;

/*!
 Pointer to the tail of the signal/functional unit/expression binding list.
*/
exp_bind* eb_tail;

/*!
 \param type  Type of thing being bound with the specified expression (0=signal, 1=functional unit)
 \param name  Signal/Function/Task scope to bind.
 \param exp   Expression ID to bind.
 \param mod   Pointer to module containing specified expression.

 Adds the specified signal/function/task and expression to the bindings linked list.
 This bindings list will be handled after all input Verilog has been
 parsed.
*/
void bind_add( int type, const char* name, expression* exp, func_unit* funit ) {
  
  exp_bind* eb;   /* Temporary pointer to signal/expressing binding */
  
  /* Create new signal/expression binding */
  eb        = (exp_bind *)malloc_safe( sizeof( exp_bind ), __FILE__, __LINE__ );
  eb->type  = type;
  eb->name  = strdup_safe( name, __FILE__, __LINE__ );
  eb->funit = funit;
  eb->exp   = exp;
  eb->fsm   = NULL;
  eb->next  = NULL;
  
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
 \param id  Expression ID of binding to remove.

 Removes the binding containing the expression ID of id.  This needs to
 be called before an expression is removed.
*/
void bind_remove( int id ) {

  exp_bind* curr;    /* Pointer to current exp_bind link       */
  exp_bind* last;    /* Pointer to last exp_bind link examined */

  curr = eb_head;
  last = eb_head;

  while( curr != NULL ) {

    assert( curr->exp != NULL );

    if( curr->exp->id == id ) {
      
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

      curr = NULL;
      
    } else {

      last = curr;
      curr = curr->next;

    }

  }
      
}

/*!
 \param name              String name of signal to bind to specified expression.
 \param exp               Pointer to expression to bind.
 \param funit_exp         Pointer to functional unit containing expression.
 \param implicit_allowed  If set to TRUE, creates any signals that are implicitly defined.
 \param fsm_bind          If set to TRUE, handling binding for FSM binding.

 \return Returns TRUE if bind occurred successfully; otherwise, returns FALSE.
 
 Performs a binding of an expression and signal based on the name of the
 signal.  Looks up signal name in the specified functional unit and sets the expression
 and signal to point to each other.  If the signal name is not found, it is checked to
 see if the signal is an unused type (name preceded by the '!' character).  If the signal
 is unused, the bind does not occur and the function returns a value of FALSE.  If the
 signal neither exists or is an unused signal, it is considered to be an implicit signal
 and a 1-bit signal is created.
*/
bool bind_signal( char* name, expression* exp, func_unit* funit_exp, bool implicit_allowed, bool fsm_bind, bool cdd_reading ) {

  bool       retval = TRUE;  /* Return value for this function */
  char*      tmpname;        /* Temporary name containing unused signal character */
  vsignal*   found_sig;      /* Pointer to found signal in design for the given name */
  func_unit* found_funit;    /* Pointer to found functional unit containing given signal */
  statement* stmt;           /* Pointer to root statement for the given expression */

  /* Search for specified signal in current functional unit */
  if( !scope_find_signal( name, funit_exp, &found_sig, &found_funit ) ) {

    /* Check to see if it is an unused signal */
    tmpname = (char*)malloc_safe( (strlen( name ) + 2), __FILE__, __LINE__ );
    snprintf( tmpname, (strlen( name ) + 2), "!%s", name );

    if( !scope_find_signal( tmpname, found_funit, &found_sig, &found_funit ) ) {

      /* If we are binding an FSM, output an error message */
      if( fsm_bind ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Unable to find specified FSM signal \"%s\" in module \"%s\" in file %s",
                  name,
                  funit_exp->name,
                  funit_exp->filename );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;

      /* If implicit signal creation is not allowed, output an error message */
      } else if( !implicit_allowed || (funit_exp != found_funit) ) {
        /* Bad hierarchical reference -- user error  -- unachievable code due to unsuppported use of hierarchical referencing */
        snprintf( user_msg, USER_MSG_LENGTH, "Hierarchical reference to undefined signal \"%s\" in %s, line %d",
                  name,
                  funit_exp->filename,
                  exp->line );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );

      /* Otherwise, implicitly create the signal and bind to it */
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Implicit declaration of signal \"%s\", creating 1-bit version of signal", name );
        print_output( user_msg, WARNING, __FILE__, __LINE__ );
        found_sig = vsignal_create( name, 1, 0 );
        sig_link_add( found_sig, &(found_funit->sig_head), &(found_funit->sig_tail) );
      }

    } else {

      retval = FALSE;

    }

    free_safe( tmpname );

  }

  if( retval ) {

    /* Add expression to signal expression list */
    exp_link_add( exp, &(found_sig->exp_head), &(found_sig->exp_tail) );

    /* Set expression to point at signal */
    exp->sig = found_sig;

    if( cdd_reading ) {

      if( (exp->op == EXP_OP_SIG)        ||
          (exp->op == EXP_OP_SBIT_SEL)   ||
          (exp->op == EXP_OP_MBIT_SEL)   ||
          (exp->op == EXP_OP_PARAM)      ||
          (exp->op == EXP_OP_PARAM_SBIT) ||
          (exp->op == EXP_OP_PARAM_MBIT) ) {
        // vector_dealloc( exp->value );
        expression_set_value( exp, found_sig->value );
      }

      if( ((exp->op == EXP_OP_SIG) ||
           (exp->op == EXP_OP_SBIT_SEL) ||
           (exp->op == EXP_OP_MBIT_SEL)) &&
          ((stmt = expression_get_root_statement( exp )) != NULL) &&
          ((stmt->exp->op == EXP_OP_EOR) ||
           (stmt->exp->op == EXP_OP_AEDGE) ||
           (stmt->exp->op == EXP_OP_PEDGE) ||
           (stmt->exp->op == EXP_OP_NEDGE)) ) {
        sig_link_add( found_sig, &(stmt->wait_sig_head), &(stmt->wait_sig_tail) );
      }

    }

  }

  return( retval );

}

/*!
 \param type  Type of functional unit
*/
bool bind_task_function( int type, char* name, expression* exp, func_unit* funit_exp, bool cdd_reading ) {

  bool       retval = TRUE;  /* Return value for this function */
  stmt_iter  si;             /* Statement iterator used to find the head statement */
  vsignal    sig;            /* Temporary signal for comparison purposes */
  sig_link*  sigl;           /* Temporary signal link holder */
  func_unit* found_funit;    /* Pointer to found task/function functional unit */
  statement* stmt;           /* Pointer to root statement for expression calling a function */

  assert( (type == FUNIT_FUNCTION) || (type == FUNIT_TASK) );

  if( !scope_find_task_function( name, type, funit_exp, &found_funit ) ) {

    /* Bad hierarchical reference -- user error  -- unachievable code due to unsuppported use of hierarchical referencing */
    snprintf( user_msg, USER_MSG_LENGTH, "Hierarchical reference to undefined %s \"%s\" in %s, line %d",
              get_funit_type( type ),
              name,
              funit_exp->filename,
              exp->line );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );

  } else {

    assert( found_funit->stmt_head != NULL );
    assert( found_funit->stmt_head->stmt != NULL );

    /* Set expression to point at task/function's first head statement */
    stmt_iter_reset( &si, found_funit->stmt_head );
    stmt_iter_find_head( &si, FALSE );
    assert( si.curr->stmt != NULL );
    exp->stmt = si.curr->stmt;

    /* Set head statement to point to this expression */
    exp_link_add( exp, &(si.curr->stmt->tf_exp_head), &(si.curr->stmt->tf_exp_tail) );

    /* If this is a function, also bind the return value signal vector to the expression's vector */
    if( type == FUNIT_FUNCTION ) {

      sig.name = found_funit->name;
      sigl     = sig_link_find( &sig, found_funit->sig_head );

      assert( sigl != NULL );

      /* Add expression to signal expression list */
      exp_link_add( exp, &(sigl->sig->exp_head), &(sigl->sig->exp_tail) );

      /* Set expression to point at signal */
      exp->sig = sigl->sig;

      if( cdd_reading ) {

        /* Attach the signal's value to our expression value */
        expression_set_value( exp, sigl->sig->value );

        /* Add to wait list */
        if( ((stmt = expression_get_root_statement( exp )) != NULL) &&
            ((stmt->exp->op == EXP_OP_EOR) ||
             (stmt->exp->op == EXP_OP_AEDGE) ||
             (stmt->exp->op == EXP_OP_PEDGE) ||
             (stmt->exp->op == EXP_OP_NEDGE)) ) {
          sig_link_add( sigl->sig, &(stmt->wait_sig_head), &(stmt->wait_sig_tail) );
        }

      }

    }

  }

  return( retval );

}

/*!
 \param cdd_reading  Set to TRUE if we are binding after reading the CDD file; otherwise, set to FALSE.

 In the process of binding, we go through each element of the binding list,
 finding the signal to be bound in the specified tree, adding the expression
 to the signal's expression pointer list, and setting the expression vector pointer
 to point to the signal vector.
*/
void bind( bool cdd_reading ) {
  
  funit_inst* funiti;               /* Pointer to found functional unit instance */
  exp_bind*   curr_eb;              /* Pointer to current expression binding */
  int         id;                   /* Current expression id -- used for removal */
  mod_parm*   mparm;                /* Newly created module parameter */
  int         i;                    /* Loop iterator */
  int         ignore;               /* Number of instances to ignore */
  funit_inst* inst;                 /* Pointer to current instance to modify */
  inst_parm*  curr_iparm;           /* Pointer to current instance parameter */
  bool        done = FALSE;         /* Specifies if the current signal is completed */
  int         orig_width;           /* Original width of found signal */
  int         orig_lsb;             /* Original lsb of found signal */
  bool        bound;                /* Specifies if the current expression was successfully bound or not */
  stmt_link*  unbound_head = NULL;  /* Head of list containing head statements of blocks containing unbound expressions */
  stmt_link*  unbound_tail = NULL;  /* Tail of list containing head statements of blocks containing unbound expressions */
  statement*  tmp_stmt;             /* Pointer to temporary statement */
    
  curr_eb = eb_head;

  while( curr_eb != NULL ) {

    assert( curr_eb->exp != NULL );
    id = curr_eb->exp->id;

    /* Handle signal binding */
    if( curr_eb->type == 0 ) {

      /*
       Bind the signal.  If it is unsuccessful, we need to remove the statement that this expression
       is a part of.
      */
      bound = bind_signal( curr_eb->name, curr_eb->exp, curr_eb->funit, FALSE, FALSE, cdd_reading );

      /* If an FSM expression is attached, size it now */
      if( curr_eb->fsm != NULL ) {
        curr_eb->fsm->value = vector_create( curr_eb->exp->value->width, TRUE );
      }

    /* Otherwise, handle function/task binding */
    } else {

      /*
       Bind the expression to the task/function.  If it is unsuccessful, we need to remove the statement
       that this expression is a part of.
      */
      bound = bind_task_function( curr_eb->type, curr_eb->name, curr_eb->exp, curr_eb->funit, cdd_reading );

    }

    /*
     If the expression was unable to be bound, put its statement block in a list to be removed after
     binding has been completed.
    */
    if( !bound ) {
      if( (tmp_stmt = expression_get_root_statement( curr_eb->exp )) != NULL ) {
        tmp_stmt = statement_find_head_statement( tmp_stmt, curr_eb->funit->stmt_head );
        assert( tmp_stmt != NULL );
        if( stmt_link_find( tmp_stmt->exp->id, unbound_head ) == NULL ) {
          stmt_link_add_tail( tmp_stmt, &unbound_head, &unbound_tail );
        }
      }
    }

#ifdef SKIP
    /************************************************************************************
     *  THIS CODE COULD PROBABLY BE PUT SOMEWHERE ELSE BUT WE WILL KEEP IT HERE FOR NOW *
     ************************************************************************************/
     
    /* Create parameter for remote signal in current expression's module */
    mparm = mod_parm_add( NULL, NULL, PARAM_TYPE_EXP_LSB, &(curr_eb->mod->param_head), &(curr_eb->mod->param_tail) );
    
    orig_width = curr_eb->exp->sig->value->width;
    orig_lsb   = curr_eb->exp->sig->lsb;
    i          = 0;
    ignore     = 0;
    while( (inst = instance_find_by_module( instance_root, curr_eb->mod, &ignore )) != NULL ) {
      
      /* Add instance parameter based on size of current signal */
      if( (curr_eb->exp->sig->value->width == -1) || (curr_eb->exp->sig->lsb == -1) ) {
        /* Signal size not known yet, figure out its size based on parameters */
        curr_iparm = inst->param_head;
        while( (curr_iparm != NULL) && !done ) {
          assert( curr_iparm->mparm != NULL );
          /* This parameter sizes a signal so perform the signal size */
          if( curr_iparm->mparm->sig == curr_eb->exp->sig ) {
            done = param_set_sig_size( curr_iparm->mparm->sig, curr_iparm );
          }
          curr_iparm = curr_iparm->next;
        }
      }
      inst_parm_add( NULL, curr_eb->exp->sig->value, mparm, &(inst->param_head), &(inst->param_tail) );
      
      i++;
      ignore = i;
      
    }

    /* Revert signal to its previous state */
    curr_eb->exp->sig->value->width = orig_width;
    curr_eb->exp->sig->lsb          = orig_lsb;
    
    /* Signify that current expression is getting its value elsewhere */
    curr_eb->exp->sig = NULL;
    
    /*************************
     * End of misplaced code *
     *************************/
#endif
   
    curr_eb = curr_eb->next;

    /* Remove binding from list */
    bind_remove( id );

  }

  /* Remove all statement blocks that contain unbindable expressions -- we cannot accurately simulate these */
  while( unbound_head != NULL ) {
    db_remove_statement( unbound_head->stmt );
    stmt_link_unlink( unbound_head->stmt, &unbound_head, &unbound_tail );
  }

}

/* 
 $Log$
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
