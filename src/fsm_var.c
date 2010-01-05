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
 \file     fsm_var.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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

#include "binding.h"
#include "codegen.h"
#include "db.h"
#include "defines.h"
#include "expr.h"
#include "fsm.h"
#include "fsm_var.h"
#include "link.h"
#include "obfuscate.h"
#include "statement.h"
#include "util.h"


extern char         user_msg[USER_MSG_LENGTH];
extern db**         db_list;
extern unsigned int curr_db;
extern func_unit*   curr_funit;


/*!
 Pointer to the head of the list of FSM scopes in the design.  To extract an FSM, the user must
 specify the scope to the FSM state variable of the FSM to extract.  When the parser finds
 this signal in the design, the appropriate FSM is created and initialized.  As a note, we
 may make the FSM extraction more automatic (smarter) in the future, but we will always allow
 the user to make these choices with the -F option to the score command.
*/
/*@null@*/ static fsm_var* fsm_var_head = NULL;

/*!
 Pointer to the tail of the list of FSM scopes in the design.
*/
/*@null@*/ static fsm_var* fsm_var_tail = NULL;

/*!
 Pointer to the head of the list of FSM variable bindings between signal names and expression
 pointers.  During the command-line parse of FSM variables, bindings will be submitted into
 this list for processing after Verilog parsing has completed.  After Verilog parsing has
 completed, the FSM bind function needs to be called to bind all FSM signals/expressions to
 each other.
*/
/*@null@*/ static fv_bind* fsm_var_bind_head = NULL;

/*!
 Pointer to the tail of the list of FSM variable bindings.
*/
/*@null@*/ static fv_bind* fsm_var_bind_tail = NULL;

/*!
 Pointer to the head of the list of FSM variable statement/functional unit bindings.  During the
 command-line parse of FSM variables, bindings will be submitted into this list for
 processing after Verilog parsing has completed.  After Verilog parsing has completed,
 the FSM bind function needs to be called to bind all FSM statements/functional units to
 each other.
*/
/*@null@*/ static fv_bind* fsm_var_stmt_head = NULL;

/*!
 Pointer to the tail of the list of FSM statement/functional unit bindings.
*/
/*@null@*/ static fv_bind* fsm_var_stmt_tail = NULL;

static void fsm_var_remove( fsm_var* );

/*!
 \param funit_name  String containing functional unit containing FSM state variable.
 \param in_state    Pointer to expression containing input state.
 \param out_state   Pointer to expression containing output state.
 \param name        Name of this FSM (only valid for attributes).
 \param exclude     If TRUE, excludes the FSM from coverage consideration.

 \return Returns pointer to newly allocated FSM variable.

 Adds the specified Verilog hierarchical scope to a list of FSM scopes to
 find during the parsing phase.
*/
fsm_var* fsm_var_add(
  const char* funit_name,
  expression* in_state,
  expression* out_state,
  char*       name,
  bool        exclude
) { PROFILE(FSM_VAR_ADD);

  fsm_var*    new_var = NULL;  /* Pointer to newly created FSM variable */
  funit_link* funitl;          /* Pointer to functional unit link found */
  fsm*        table;           /* Pointer to newly create FSM */

  /* If we have not parsed design, add new FSM variable to list */
  if( db_list[curr_db]->funit_head == NULL ) {

    new_var          = (fsm_var*)malloc_safe( sizeof( fsm_var ) );
    new_var->funit   = strdup_safe( funit_name );
    new_var->name    = NULL;
    new_var->ivar    = in_state;
    new_var->ovar    = out_state;
    new_var->iexp    = NULL;
    new_var->table   = NULL;
    new_var->exclude = exclude;
    new_var->next    = NULL;

    if( fsm_var_head == NULL ) {
      fsm_var_head = fsm_var_tail = new_var;
    } else {
      fsm_var_tail->next = new_var;
      fsm_var_tail       = new_var;
    }

  } else {

    if( (funitl = funit_link_find( funit_name, FUNIT_MODULE, db_list[curr_db]->funit_head )) != NULL ) {
      table = fsm_create( in_state, out_state, exclude );
      if( name != NULL ) {
        table->name = strdup_safe( name );
      }
      in_state->table  = table;
      out_state->table = table;
      fsm_link_add( table, &(funitl->funit->fsms), &(funitl->funit->fsm_size) );
    } else {
      assert( funitl != NULL );
    }

  }

  return( new_var );

}

/*!
 \return Returns pointer to found FSM variable that contains this expression as an
         output expression.

 Searches the FSM variable list for the FSM variable that contains the specified
 expression as its output state expression.  If no FSM variable was found, returns
 a value of NULL to the calling function.
*/
static fsm_var* fsm_var_is_output_state(
  expression* expr  /*!< Pointer to expression to evaluate */
) { PROFILE(FSM_VAR_IS_OUTPUT_STATE);

  fsm_var* curr;  /* Pointer to current FSM variable structure */

  curr = fsm_var_head;
  while( (curr != NULL) && (curr->ovar != expr) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

/*!
 \throws anonymous Throw Throw

 Searches the functional unit list for the functional unit called funit_name.  If the functional unit
 is found in the design, searches this functional unit for the signal called sig_name.  If the signal is found,
 the signal and specified expression expr are bound to each other and this function returns
 a value of TRUE.  If the signal name could not be found or the functional unit name could not be found
 in the design, no binding occurs and the function displays an error message and returns a
 value of FALSE to the calling function.
*/
static void fsm_var_bind_expr(
  char*       sig_name,   /*!< String name of signal to bind to specified expression */
  expression* expr,       /*!< Pointer to expression to bind to signal called sig_name */
  char*       funit_name  /*!< String name of functional unit that contains the expression pointed to by expr */
) { PROFILE(FSM_VAR_BIND_EXPR);

  funit_link* funitl;  /* Pointer to found functional unit link element */

  if( (funitl = funit_link_find( funit_name, FUNIT_MODULE, db_list[curr_db]->funit_head )) != NULL ) {
    if( !bind_signal( sig_name, expr, funitl->funit, TRUE, FALSE, FALSE, expr->line, FALSE ) ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to bind FSM-specified signal (%s) to expression (%d) in module (%s)",
                                  obf_sig( sig_name ), expr->id, obf_funit( funit_name ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
    expr->name = strdup_safe( sig_name );
  } else {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find FSM-specified module (%s) in design", obf_funit( funit_name ) ); 
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  PROFILE_END;

}

/*!
 Iterates through specified expression tree, adding each expression to the
 specified functional unit if the expression does not already exist in the functional unit.
*/
static void fsm_var_add_expr(
  expression* expr,  /*!< Pointer to expression to add to the specified functional unit */
  func_unit* funit   /*!< Pointer to functional unit structure to add expression to */
) { PROFILE(FSM_VAR_ADD_EXPR);

  if( expr != NULL ) {

    if( exp_link_find( expr->id, funit->exps, funit->exp_size ) == NULL ) {

      /* Set the global curr_funit to the expression's functional unit */
      curr_funit = funit;

      /* Add expression's children first. */
      db_add_expression( expr->right );
      db_add_expression( expr->left );

      /* Now add this expression to the list. */
      exp_link_add( expr, &(funit->exps), &(funit->exp_size) );

      /* Now clear the curr_funit */
      curr_funit = NULL;

    }

  }

  PROFILE_END;

}

/*!
 \return Returns a value of TRUE if the statement was successfully bound to
         the specified functional unit name; otherwise, returns a value of FALSE.

 Searches the design functional unit list for a functional unit called funit_name.
 If the functional unit is found in the design, adds the statement's expression tree to the design,
 sets the STMT_ADDED bit in the statement's supplemental field, adds this
 statement to the found functional unit structure, and finally creates an FSM table if
 the statement contains an output state FSM expression tree and returns a value
 of TRUE to the calling function.  If the functional unit could not be found, this
 function, returns a value of FALSE to the calling function.
*/
static bool fsm_var_bind_stmt(
  statement*  stmt,       /*!< Pointer to statement to bind */
  const char* funit_name  /*!< String name of functional unit which will contain stmt */
) { PROFILE(FSM_VAR_BIND_STMT);

  bool        retval = FALSE;  /* Return value for this function */
  funit_link* funitl;          /* Pointer to found functional unit link element */
  fsm_var*    fv;              /* Pointer to found FSM variable */

  if( (funitl = funit_link_find( funit_name, FUNIT_MODULE, db_list[curr_db]->funit_head )) != NULL ) {

    /* First, add expression tree to found functional unit expression list */
    fsm_var_add_expr( stmt->exp, funitl->funit );

    /* Set ADDED bit of this statement */
    stmt->suppl.part.added = 1;

    /* Second, add our statement to this functional unit's statement list */
    stmt_link_add( stmt, TRUE, &(funitl->funit->stmt_head), &(funitl->funit->stmt_tail) );

    /* Third, add the functional unit to this statement's pointer */
    stmt->funit = funitl->funit;

    /* Finally, create the new FSM if we are the output state */
    if( (fv = fsm_var_is_output_state( stmt->exp )) != NULL ) {
      fv->table       = fsm_create( fv->ivar, fv->ovar, fv->exclude );
      fv->ivar->table = fv->table;
      fv->ovar->table = fv->table;
      fsm_link_add( fv->table, &(funitl->funit->fsms), &(funitl->funit->fsm_size) );
      fsm_var_remove( fv );
    }

  } else {

    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \throws anonymous fsm_var_bind_expr

 Creates a new FSM binding structure and initializes it with the specified information.
 The FSM binding structure is then added to the global list of FSM binding structures to
 be bound after parsing is complete.
*/
void fsm_var_bind_add(
  char*       sig_name,   /*!< Name of signal to bind */
  expression* expr,       /*!< Pointer to expression to bind */
  char*       funit_name  /*!< Name of functional unit that will contain the expression and signal being bound */
) { PROFILE(FSM_VAR_BIND_ADD);

  fv_bind* fvb;  /* Pointer to new FSM variable binding structure */

  /* If the functional unit list does not exist yet, we need to bind this later; otherwise, bind now */
  if( db_list[curr_db]->funit_head == NULL ) {

    /* Allocate and initialize FSM variable bind structure */
    fvb             = (fv_bind*)malloc_safe( sizeof( fv_bind ) );
    fvb->sig_name   = strdup_safe( sig_name );
    fvb->expr       = expr;
    fvb->funit_name = strdup_safe( funit_name );
    fvb->next       = NULL;

    /* Add new structure to the global list */
    if( fsm_var_bind_head == NULL ) {
      fsm_var_bind_head = fsm_var_bind_tail = fvb;
    } else {
      fsm_var_bind_tail->next = fvb;
      fsm_var_bind_tail       = fvb;
    }

  } else {

    fsm_var_bind_expr( sig_name, expr, funit_name );

  }

  PROFILE_END;

}

/*!
 Allocates and initializes an FSM variable binding entry and adds it to the
 fsm_var_stmt list for later processing.
*/
void fsm_var_stmt_add(
  statement* stmt,       /*!< Pointer to statement containing FSM state expression */
  char*      funit_name  /*!< Name of functional unit that will contain stmt */
) { PROFILE(FSM_VAR_STMT_ADD);

  fv_bind* fvb;  /* Pointer to new FSM variable binding structure */

  fvb             = (fv_bind*)malloc_safe( sizeof( fv_bind ) );
  fvb->stmt       = stmt;
  fvb->funit_name = strdup_safe( funit_name );
  fvb->next       = NULL;

  /* Add new structure to the head of the global list */
  if( fsm_var_stmt_head == NULL ) {
    fsm_var_stmt_head = fsm_var_stmt_tail = fvb;
  } else {
    fvb->next         = fsm_var_stmt_head;
    fsm_var_stmt_head = fvb;
  }

  PROFILE_END;

}

/*!
 \throws anonymous fsm_var_bind_expr Throw

 After Verilog parsing has completed, this function should be called to bind all signals
 to their associated FSM state expressions and functional units.  For each entry in the FSM binding list
 the signal name is looked in the functional unit specified in the binding entry.  If the signal is found,
 the associated expression pointer is added to the signal's expression list and the expression's
 signal pointer is set to point at the found signal structure.  If the signal was not found,
 an error message is reported to the user, specifying the signal did not exist in the design.

 After the signals and expressions have been bound, the FSM statement binding list is iterated
 through binding all statements containing FSM state expressions to the functional unit that it is a part of.
 If the statement contains an FSM state expression that is an output state expression, create the
 FSM structure for this FSM and add it to the design.
*/
void fsm_var_bind() { PROFILE(FSM_VAR_BIND);

  fv_bind* curr = NULL;  /* Pointer to current FSM variable */
  fv_bind* tmp;          /* Temporary pointer to FSM bind structure */

  Try {

    curr = fsm_var_bind_head;
    while( curr != NULL ) {

      /* Perform binding */
      fsm_var_bind_expr( curr->sig_name, curr->expr, curr->funit_name );

      tmp = curr->next;

      /* Deallocate memory for this bind structure */
      free_safe( curr->sig_name, (strlen( curr->sig_name ) + 1) );
      free_safe( curr->funit_name, (strlen( curr->funit_name ) + 1) );
      free_safe( curr, sizeof( fv_bind ) );

      curr = tmp;

    }

  } Catch_anonymous {
    while( curr != NULL ) {
      tmp = curr->next;
      free_safe( curr->sig_name, (strlen( curr->sig_name ) + 1) );
      free_safe( curr->funit_name, (strlen( curr->funit_name ) + 1) );
      free_safe( curr, sizeof( fv_bind ) );
      curr = tmp;
    } 
    curr = fsm_var_stmt_head;
    while( curr != NULL ) {
      tmp = curr->next;
      free_safe( curr->funit_name, (strlen( curr->funit_name ) + 1) );
      free_safe( curr, sizeof( fv_bind ) );
      curr = tmp;
    }
    Throw 0;
  }

  curr = fsm_var_stmt_head;
  while( curr != NULL ) {

    /* Bind statement to functional unit */
    (void)fsm_var_bind_stmt( curr->stmt, curr->funit_name );

    tmp = curr->next;

    /* Deallocate memory for this bind structure */
    free_safe( curr->funit_name, (strlen( curr->funit_name ) + 1) );
    free_safe( curr, sizeof( fv_bind ) );

    curr = tmp;

  }

  PROFILE_END;

}

/*!
 Deallocates an FSM variable entry from memory.
*/
static void fsm_var_dealloc(
  fsm_var* fv  /*!< Pointer to FSM variable to deallocate */
) { PROFILE(FSM_VAR_DEALLOC);

  if( fv != NULL ) {

    /* Deallocate the functional unit name string */
    free_safe( fv->funit, (strlen( fv->funit ) + 1) );

    /* Finally, deallocate ourself */
    free_safe( fv, sizeof( fsm_var ) );

  }

  PROFILE_END;

}

/*!
 Searches global FSM variable list for matching FSM variable structure.
 When match is found, remove the structure and deallocate it from memory
 being sure to keep the global list intact.
*/
void fsm_var_remove(
  fsm_var* fv  /*!< Pointer to FSM variable structure to remove from global list */
) { PROFILE(FSM_VAR_REMOVE);

  fsm_var* curr;  /* Pointer to current FSM variable structure in list */
  fsm_var* last;  /* Pointer to last FSM variable structure evaluated */

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

  PROFILE_END;

}

/*!
 Iterates through the various global lists in this file, deallocating all memory.  This function
 is called when an error has occurred during the parsing stage.
*/
void fsm_var_cleanup() { PROFILE(FSM_VAR_CLEANUP);

  fsm_var* curr_fv;  /* Pointer to the current fsm_var structure */
  fsm_var* tmp_fv;   /* Temporary pointer */
  fv_bind* curr_fvb = fsm_var_bind_head;  /* Pointer to the current fv_bind structure */ 
  fv_bind* tmp_fvb;                       /* Temporary pointer */

  /* Deallocate fsm_var list */
  curr_fv = fsm_var_head;
  while( curr_fv != NULL ) {
    tmp_fv  = curr_fv;
    curr_fv = curr_fv->next;

    free_safe( tmp_fv->funit, (strlen( curr_fv->funit ) + 1) );
    expression_dealloc( tmp_fv->ivar, FALSE );
    expression_dealloc( tmp_fv->ovar, FALSE );
    free_safe( tmp_fv, sizeof( fsm_var ) );
  }
  fsm_var_head = fsm_var_tail = NULL;

  /* Deallocate fsm_var_bind list */
  curr_fvb = fsm_var_bind_head;
  while( curr_fvb != NULL ) {
    tmp_fvb  = curr_fvb;
    curr_fvb = curr_fvb->next;

    free_safe( tmp_fvb->sig_name,   (strlen( tmp_fvb->sig_name ) + 1) );
    free_safe( tmp_fvb->funit_name, (strlen( tmp_fvb->funit_name ) + 1) );
    expression_dealloc( tmp_fvb->expr, FALSE );
    free_safe( tmp_fvb, sizeof( fv_bind ) );
  }
  fsm_var_bind_head = fsm_var_bind_tail = NULL;

  /* Deallocate fsm_var_stmt list */
  curr_fvb = fsm_var_stmt_head;
  while( curr_fvb != NULL ) {
    tmp_fvb  = curr_fvb;
    curr_fvb = curr_fvb->next;
    
    free_safe( tmp_fvb->funit_name, (strlen( tmp_fvb->funit_name ) + 1) );
    expression_dealloc( tmp_fvb->stmt->exp, FALSE );
    statement_dealloc( tmp_fvb->stmt );
    free_safe( tmp_fvb, sizeof( fv_bind ) );
  }
  fsm_var_stmt_head = fsm_var_stmt_tail = NULL;

  PROFILE_END;

}

