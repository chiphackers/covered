/*!
 \file     fsm_var.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/3/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "fsm_var.h"
#include "util.h"
#include "codegen.h"
#include "link.h"
#include "binding.h"
#include "fsm.h"


extern char user_msg[USER_MSG_LENGTH];


/*!
 Pointer to the head of the list of FSM scopes in the design.  To extract an FSM, the user must
 specify the scope to the FSM state variable of the FSM to extract.  When the parser finds
 this signal in the design, the appropriate FSM is created and initialized.  As a note, we
 may make the FSM extraction more automatic (smarter) in the future, but we will always allow
 the user to make these choices with the -F option to the score command.
*/
fsm_var* fsm_var_head = NULL;

/*!
 Pointer to the tail of the list of FSM scopes in the design.
*/
fsm_var* fsm_var_tail = NULL;

/*!
 Pointer to the head of the list of FSM variable bindings between signal names and expression
 pointers.  During the command-line parse of FSM variables, bindings will be submitted into
 this list for processing after Verilog parsing has completed.  After Verilog parsing has
 completed, the FSM bind function needs to be called to bind all FSM signals/expressions to
 each other.
*/
fv_bind* fsm_var_bind_head = NULL;

/*!
 Pointer to the tail of the list of FSM variable bindings.
*/
fv_bind* fsm_var_bind_tail = NULL;

/*!
 Pointer to the head of the list of FSM variable statement/module bindings.  During the
 command-line parse of FSM variables, bindings will be submitted into this list for
 processing after Verilog parsing has completed.  After Verilog parsing has completed,
 the FSM bind function needs to be called to bind all FSM statements/modules to
 each other.
*/
fv_bind* fsm_var_stmt_head = NULL;

/*!
 Pointer to the tail of the list of FSM statement/module bindings.
*/
fv_bind* fsm_var_stmt_tail = NULL;


/*!
 \param mod        String containing module containing FSM state variable.
 \param in_state   Pointer to expression containing input state.
 \param out_state  Pointer to expression containing output state.

 \return Returns pointer to newly allocated FSM variable.

 Adds the specified Verilog hierarchical scope to a list of FSM scopes to
 find during the parsing phase.
*/
fsm_var* fsm_var_add( char* mod, expression* in_state, expression* out_state ) {

  fsm_var* new_var;  /* Pointer to newly created FSM variable */

  new_var        = (fsm_var*)malloc_safe( sizeof( fsm_var ) );
  new_var->mod   = strdup( mod );
  new_var->ivar  = in_state;
  new_var->ovar  = out_state;
  new_var->iexp  = NULL;
  new_var->table = NULL;
  new_var->next  = NULL;

  if( fsm_var_head == NULL ) {
    fsm_var_head = fsm_var_tail = new_var;
  } else {
    fsm_var_tail->next = new_var;
    fsm_var_tail       = new_var;
  }

  return( new_var );

}

fsm_var* fsm_var_is_output_state( expression* expr ) {

  fsm_var* curr;  /* Pointer to current FSM variable structure */

  curr = fsm_var_head;
  while( (curr != NULL) && (curr->ovar != expr) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 Checks the state of the FSM variable list.  If the list is not empty, output all
 FSM state variables (and their associated modules) that have not been found as
 a warning to the user.  This would indicate user error.  This function should be
 called after parsing is complete.
*/
void fsm_var_check_for_unused() {

  fsm_var* curr;  /* Pointer to current FSM variable structure being evaluated */
  char*    code;  /* String containing expression output                       */

  if( fsm_var_head != NULL ) {

    print_output( "The following FSM state variables were not found:", WARNING );
    print_output( "Module                     Variable/Expression", WARNING_WRAP );
    print_output( "-------------------------  -------------------------", WARNING_WRAP );

    curr = fsm_var_head;
    while( curr != NULL ) {
      if( curr->iexp == NULL ) {
        code = codegen_gen_expr( curr->ivar, -1, SUPPL_OP( curr->ivar->suppl ) );
        snprintf( user_msg, USER_MSG_LENGTH, "%-25.25s  %s", curr->mod, code );
        print_output( user_msg, WARNING_WRAP );
        free_safe( code );
      }
      if( curr->table == NULL ) {
        code = codegen_gen_expr( curr->ovar, -1, SUPPL_OP( curr->ovar->suppl ) );
        snprintf( user_msg, USER_MSG_LENGTH, "%-25.25s  %s", curr->mod, code );
        print_output( user_msg, WARNING_WRAP );
        free_safe( code );
      }
      curr = curr->next;
    }

    print_output( "", WARNING_WRAP );

  }

}

void fsm_var_bind_add( char* sig_name, expression* expr, char* mod_name ) {

  fv_bind* fvb;  /* Pointer to new FSM variable binding structure */

  /* Allocate and initialize FSM variable bind structure */
  fvb           = (fv_bind*)malloc_safe( sizeof( fv_bind ) );
  fvb->sig_name = strdup( sig_name );
  fvb->expr     = expr;
  fvb->mod_name = strdup( mod_name );

  /* Add new structure to the global list */
  if( fsm_var_bind_head == NULL ) {
    fsm_var_bind_head = fsm_var_bind_tail = fvb;
  } else {
    fsm_var_bind_tail->next = fvb;
    fsm_var_bind_tail       = fvb;
  }

}

void fsm_var_stmt_add( statement* stmt, char* mod_name ) {

  fv_bind* fvb;  /* Pointer to new FSM variable binding structure */

  fvb           = (fv_bind*)malloc_safe( sizeof( fv_bind ) );
  fvb->stmt     = stmt;
  fvb->mod_name = strdup( mod_name );

  /* Add new structure to the head of the global list */
  if( fsm_var_stmt_head == NULL ) {
    fsm_var_stmt_head = fsm_var_stmt_tail = fvb;
  } else {
    fvb->next         = fsm_var_stmt_head;
    fsm_var_stmt_head = fvb;
  }

}

void fsm_var_add_expr( expression* expr, module* mod ) {

  if( expr != NULL ) {

    if( exp_link_find( expr, mod->exp_head ) == NULL ) {

      /* Add expression's children first. */
      db_add_expression( expr->right );
      db_add_expression( expr->left );

      /* Now add this expression to the list. */
      exp_link_add( expr, &(mod->exp_head), &(mod->exp_tail) );

    }

  }

}

void fsm_var_bind( mod_link* mod_head ) {

  fv_bind*  curr;           /* Pointer to current FSM variable                               */
  fv_bind*  tmp;            /* Temporary pointer to FSM bind structure                       */
  bool      error = FALSE;  /* Specifies if an error occurred during the FSM binding process */
  module    mod;            /* Temporary module used for searching purposes                  */
  mod_link* modl;           /* Pointer to module link element                                */
  fsm_var*  fv;             /* Pointer to found FSM variable structure                       */

  curr = fsm_var_bind_head;
  while( curr != NULL ) {

    mod.name = curr->mod_name;

    /* Perform binding */
    if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {
      error = bind_perform( curr->sig_name, curr->expr, modl->mod, modl->mod, FALSE ) || error;
    } else {
      error = TRUE;
    }

    tmp = curr->next;

    /* Deallocate memory for this bind structure */
    free_safe( curr->sig_name );
    free_safe( curr->mod_name );
    free_safe( curr );

    curr = tmp;

  }

  curr = fsm_var_stmt_head;
  while( curr != NULL ) {

    mod.name = curr->mod_name;

    if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {

      /* First, add expression tree to found module expression list */
      fsm_var_add_expr( curr->stmt->exp, modl->mod );

      /* Set ADDED bit of this statement */
      curr->stmt->exp->suppl = curr->stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_ADDED);

      /* Second, add our statement to this module's statement list */
      stmt_link_add_head( curr->stmt, &(modl->mod->stmt_head), &(modl->mod->stmt_tail) );

      /* Finally, create the new FSM if we are the output state */
      if( (fv = fsm_var_is_output_state( curr->stmt->exp )) != NULL ) {
        fv->table       = fsm_create( fv->ivar, fv->ovar, FALSE );
        fv->ivar->table = fv->table;
        fv->ovar->table = fv->table;
        fsm_link_add( fv->table, &(modl->mod->fsm_head), &(modl->mod->fsm_tail) );
        fsm_var_remove( fv );
      }

    } else if( !error ) {

      /* If we found the module before, we should be able to find it again */
      assert( modl != NULL );

    }

    tmp = curr->next;

    /* Deallocate memory for this bind structure */
    free_safe( curr->mod_name );
    free_safe( curr );

    curr = tmp;

  }

  if( error ) {
    fsm_var_check_for_unused();
  }

}

/*!
 \param fv  Pointer to FSM variable to deallocate.

 Deallocates an FSM variable entry from memory.
*/
void fsm_var_dealloc( fsm_var* fv ) {

  if( fv != NULL ) {

    /* Deallocate the module name string */
    free_safe( fv->mod );

    /* Finally, deallocate ourself */
    free_safe( fv );

  }

}

/*!
 \param fv  Pointer to FSM variable structure to remove from global list.

 Searches global FSM variable list for matching FSM variable structure.
 When match is found, remove the structure and deallocate it from memory
 being sure to keep the global list intact.
*/
void fsm_var_remove( fsm_var* fv ) {

  fsm_var* curr;  /* Pointer to current FSM variable structure in list */
  fsm_var* last;  /* Pointer to last FSM variable structure evaluated  */

  /* Find matching FSM variable structure */
  curr = fsm_var_head;
  last = NULL;
  while( (curr != NULL) && (curr != fv) ) {
    last = curr;
    curr = curr->next;
  }

  /* If a matching FSM variable structure was found, remove it from the global list. */
  if( curr != NULL ) {

    if( (curr == fsm_var_head) && (curr == fsm_var_tail) ) {
      fsm_var_head = fsm_var_tail = NULL;
    } else if( curr == fsm_var_head ) {
      fsm_var_head = curr->next;
    } else if( curr == fsm_var_tail ) {
      fsm_var_tail = last;
    } else {
      last->next = curr->next;
    }

    fsm_var_dealloc( curr );

  }

}


/*
 $Log$
 Revision 1.4  2003/10/13 12:27:25  phase1geo
 More fixes to FSM stuff.

 Revision 1.3  2003/10/13 03:56:29  phase1geo
 Fixing some problems with new FSM code.  Not quite there yet.

 Revision 1.2  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.1  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

*/

