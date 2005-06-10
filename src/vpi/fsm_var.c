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
#include "db.h"


extern char      user_msg[USER_MSG_LENGTH];
extern mod_link* mod_head;


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
 \param mod_name   String containing module containing FSM state variable.
 \param in_state   Pointer to expression containing input state.
 \param out_state  Pointer to expression containing output state.
 \param name       Name of this FSM (only valid for attributes).

 \return Returns pointer to newly allocated FSM variable.

 Adds the specified Verilog hierarchical scope to a list of FSM scopes to
 find during the parsing phase.
*/
fsm_var* fsm_var_add( char* mod_name, expression* in_state, expression* out_state, char* name ) {

  fsm_var*  new_var = NULL;  /* Pointer to newly created FSM variable */
  mod_link* modl;            /* Pointer to module link found          */
  module    mod;             /* Temporary module used for searching   */
  fsm*      table;           /* Pointer to newly create FSM           */

  /* If we have not parsed, design add new FSM variable to list */
  if( mod_head == NULL ) {

    new_var        = (fsm_var*)malloc_safe( sizeof( fsm_var ), __FILE__, __LINE__ );
    new_var->mod   = strdup_safe( mod_name, __FILE__, __LINE__ );
    new_var->name  = NULL;
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

  } else {

    /* Just create the new FSM */
    mod.name = mod_name;

    if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {
      table = fsm_create( in_state, out_state );
      if( name != NULL ) {
        table->name = strdup_safe( name, __FILE__, __LINE__ );
      }
      in_state->table  = table;
      out_state->table = table;
      fsm_link_add( table, &(modl->mod->fsm_head), &(modl->mod->fsm_tail) );
    } else {
      assert( modl != NULL );
    }

  }

  return( new_var );

}

/*!
 \param expr  Pointer to expression to evaluate.

 \return Returns pointer to found FSM variable that contains this expression as an
         output expression.

 Searches the FSM variable list for the FSM variable that contains the specified
 expression as its output state expression.  If no FSM variable was found, returns
 a value of NULL to the calling function.
*/
fsm_var* fsm_var_is_output_state( expression* expr ) {

  fsm_var* curr;  /* Pointer to current FSM variable structure */

  curr = fsm_var_head;
  while( (curr != NULL) && (curr->ovar != expr) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param sig_name  String name of signal to bind to specified expression.
 \param expr      Pointer to expression to bind to signal called sig_name.
 \param mod_name  String name of module that contains the expression pointed to by expr.

 \return Returns TRUE if the signal and expression are able to be bound (specified signal
         name and module name exist in design); otherwise, returns a value of FALSE.

 Searches the module list for the module called mod_name.  If the module is found in the
 design, searches this module for the signal called sig_name.  If the signal is found,
 the signal and specified expression expr are bound to each other and this function returns
 a value of TRUE.  If the signal name could not be found or the module name could not be found
 in the design, no binding occurs and the function displays an error message and returns a
 value of FALSE to the calling function.
*/
bool fsm_var_bind_expr( char* sig_name, expression* expr, char* mod_name ) {

  bool      retval = TRUE;  /* Return value for this function       */
  mod_link* modl;           /* Pointer to found module link element */
  module    mod;             /* Temporary module used for searching  */

  mod.name = mod_name;

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {
    if( !bind_perform( sig_name, expr, modl->mod, modl->mod, FALSE, TRUE ) ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Unable to bind FSM-specified signal (%s) to expression (%d) in module (%s)",
                sig_name, expr->id, mod_name );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = FALSE;
    }
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Unable to find FSM-specified module (%s) in design", mod_name ); 
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to add to the specified module.
 \param mod   Pointer to module structure to add expression to.

 Iterates through specified expression tree, adding each expression to the
 specified module if the expression does not already exist in the module.
*/
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

/*!
 \param stmt      Pointer to statement to bind.
 \param mod_name  String name of module which will contain stmt.

 \return Returns a value of TRUE if the statement was successfully bound to
         the specified module name; otherwise, returns a value of FALSE.

 Searches the design module list for a module called mod_name.  If the module
 is found in the design, adds the statement's expression tree to the design,
 sets the STMT_ADDED bit in the statement's supplemental field, adds this
 statement to the found module structure, and finally creates an FSM table if
 the statement contains an output state FSM expression tree and returns a value
 of TRUE to the calling function.  If the module could not be found, this
 function, returns a value of FALSE to the calling function.
*/
bool fsm_var_bind_stmt( statement* stmt, char* mod_name ) {

  bool      retval = FALSE;  /* Return value for this function       */
  mod_link* modl;            /* Pointer to found module link element */
  module    mod;             /* Temporary module used for searching  */
  fsm_var*  fv;              /* Pointer to found FSM variable        */

  mod.name = mod_name;

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {

    /* First, add expression tree to found module expression list */
    fsm_var_add_expr( stmt->exp, modl->mod );

    /* Set ADDED bit of this statement */
    stmt->exp->suppl.part.stmt_added = 1;

    /* Second, add our statement to this module's statement list */
    stmt_link_add_head( stmt, &(modl->mod->stmt_head), &(modl->mod->stmt_tail) );

    /* Finally, create the new FSM if we are the output state */
    if( (fv = fsm_var_is_output_state( stmt->exp )) != NULL ) {
      fv->table       = fsm_create( fv->ivar, fv->ovar );
      fv->ivar->table = fv->table;
      fv->ovar->table = fv->table;
      fsm_link_add( fv->table, &(modl->mod->fsm_head), &(modl->mod->fsm_tail) );
      fsm_var_remove( fv );
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param sig_name  Name of signal to bind.
 \param expr      Pointer to expression to bind.
 \param mod_name  Name of module that will contain the expression and signal being bound.

 Creates a new FSM binding structure and initializes it with the specified information.
 The FSM binding structure is then added to the global list of FSM binding structures to
 be bound after parsing is complete.
*/
void fsm_var_bind_add( char* sig_name, expression* expr, char* mod_name ) {

  fv_bind* fvb;  /* Pointer to new FSM variable binding structure */

  /* If the module list does not exist yet, we need to bind this later; otherwise, bind now */
  if( mod_head == NULL ) {

    /* Allocate and initialize FSM variable bind structure */
    fvb           = (fv_bind*)malloc_safe( sizeof( fv_bind ), __FILE__, __LINE__ );
    fvb->sig_name = strdup_safe( sig_name, __FILE__, __LINE__ );
    fvb->expr     = expr;
    fvb->mod_name = strdup_safe( mod_name, __FILE__, __LINE__ );
    fvb->next     = NULL;

    /* Add new structure to the global list */
    if( fsm_var_bind_head == NULL ) {
      fsm_var_bind_head = fsm_var_bind_tail = fvb;
    } else {
      fsm_var_bind_tail->next = fvb;
      fsm_var_bind_tail       = fvb;
    }

  } else {

    if( !fsm_var_bind_expr( sig_name, expr, mod_name ) ) {
      exit( 1 );
    }

  }

}

/*!
 \param stmt      Pointer to statement containing FSM state expression
 \param mod_name  Name of module that will contain stmt.

 Allocates and initializes an FSM variable binding entry and adds it to the
 fsm_var_stmt list for later processing.
*/
void fsm_var_stmt_add( statement* stmt, char* mod_name ) {

  fv_bind* fvb;  /* Pointer to new FSM variable binding structure */

  fvb           = (fv_bind*)malloc_safe( sizeof( fv_bind ), __FILE__, __LINE__ );
  fvb->stmt     = stmt;
  fvb->mod_name = strdup_safe( mod_name, __FILE__, __LINE__ );
  fvb->next     = NULL;

  /* Add new structure to the head of the global list */
  if( fsm_var_stmt_head == NULL ) {
    fsm_var_stmt_head = fsm_var_stmt_tail = fvb;
  } else {
    fvb->next         = fsm_var_stmt_head;
    fsm_var_stmt_head = fvb;
  }

}

/*!
 After Verilog parsing has completed, this function should be called to bind all signals
 to their associated FSM state expressions and modules.  For each entry in the FSM binding list
 the signal name is looked in the module specified in the binding entry.  If the signal is found,
 the associated expression pointer is added to the signal's expression list and the expression's
 signal pointer is set to point at the found signal structure.  If the signal was not found,
 an error message is reported to the user, specifying the signal did not exist in the design.

 After the signals and expressions have been bound, the FSM statement binding list is iterated
 through binding all statements containing FSM state expressions to the module that it is a part of.
 If the statement contains an FSM state expression that is an output state expression, create the
 FSM structure for this FSM and add it to the design.
*/
void fsm_var_bind() {

  fv_bind*  curr;           /* Pointer to current FSM variable                               */
  fv_bind*  tmp;            /* Temporary pointer to FSM bind structure                       */
  bool      error = FALSE;  /* Specifies if an error occurred during the FSM binding process */

  curr = fsm_var_bind_head;
  while( curr != NULL ) {

    /* Perform binding */
    error = !fsm_var_bind_expr( curr->sig_name, curr->expr, curr->mod_name ) || error;

    tmp = curr->next;

    /* Deallocate memory for this bind structure */
    free_safe( curr->sig_name );
    free_safe( curr->mod_name );
    free_safe( curr );

    curr = tmp;

  }

  if( !error ) {

    curr = fsm_var_stmt_head;
    while( curr != NULL ) {

      /* Bind statement to module */
      fsm_var_bind_stmt( curr->stmt, curr->mod_name );

      tmp = curr->next;

      /* Deallocate memory for this bind structure */
      free_safe( curr->mod_name );
      free_safe( curr );

      curr = tmp;

    }

  } else {

    exit( 1 );

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
 Revision 1.16  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.15  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.14  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.13  2003/11/15 04:21:57  phase1geo
 Fixing syntax errors found in Doxygen and GCC compiler.

 Revision 1.12  2003/11/11 21:48:09  phase1geo
 Fixing bug where next pointers in bind lists were not being initialized to
 NULL (manifested itself in Irix).  Also added missing development documentation
 to functions in fsm_var.c and removed unnecessary function.

 Revision 1.11  2003/11/11 13:38:00  phase1geo
 Fixing bug in fsm_var that bound FSM statements early.  This caused an
 incorrect ordering of statements that results in incorrect FSM coverage.
 Updated regression suite for changes.  Full regression now passes.

 Revision 1.10  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.9  2003/10/28 01:09:38  phase1geo
 Cleaning up unnecessary output.

 Revision 1.8  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.7  2003/10/20 21:38:49  phase1geo
 Adding function documentation to functions that were missing it.

 Revision 1.6  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.5  2003/10/14 04:02:44  phase1geo
 Final fixes for new FSM support.  Full regression now passes.  Need to
 add new diagnostics to verify new functionality, but at least all existing
 cases are supported again.

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

