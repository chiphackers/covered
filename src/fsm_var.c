/*!
 \file     fsm_var.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/3/2003
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "fsm_var.h"
#include "fsm_sig.h"
#include "util.h"

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
 \param mod  String containing module containing FSM state variable.

 \return Returns pointer to newly allocated FSM variable.

 Adds the specified Verilog hierarchical scope to a list of FSM scopes to
 find during the parsing phase.
*/
fsm_var* fsm_var_add( char* mod ) {

  fsm_var* new_var;  /* Pointer to newly created FSM variable */

  new_var            = (fsm_var*)malloc_safe( sizeof( fsm_var ) );
  new_var->mod       = strdup( mod );
  new_var->ivar_head = NULL;
  new_var->ivar_tail = NULL;
  new_var->ovar_head = NULL;
  new_var->ovar_tail = NULL;
  new_var->isig      = NULL;
  new_var->table     = NULL;
  new_var->next      = NULL;

  if( fsm_var_head == NULL ) {
    fsm_var_head = fsm_var_tail = new_var;
  } else {
    fsm_var_tail->next = new_var;
    fsm_var_tail       = new_var;
  }

  return( new_var );

}

/*!
 \param fv     Pointer to FSM variable to add signal to.
 \param name   Name of signal to add.
 \param width  Width of signal to add.
 \param lsb    LSB of signal to add.

 Adds specified signal information to FSM variable input list.
*/
void fsm_var_add_in_sig( fsm_var* fv, char* name, int width, int lsb ) {

  fsm_sig_add( &(fv->ivar_head), &(fv->ivar_tail), name, width, lsb );

}

/*!
 \param fv     Pointer to FSM variable to add signal to.
 \param name   Name of signal to add.
 \param width  Width of signal to add.
 \param lsb    LSB of signal to add.

 Adds specified signal information to FSM variable output list.
*/
void fsm_var_add_out_sig( fsm_var* fv, char* name, int width, int lsb ) {

  fsm_sig_add( &(fv->ovar_head), &(fv->ovar_tail), name, width, lsb );

}

/*!
 \param mod  Name of current module being parsed.
 \param var  Name of current signal being created.

 \return Returns pointer to found fsm_var structure containing matching signal;
         otherwise, returns NULL.

 Checks FSM variable list to see if any entries in this list match the
 specified mod and var values.  If an entry is found to match, return a
 pointer to the found structure to indicate to the calling function that
 the specified signal needs to have an FSM structure associated with it.
*/
fsm_var* fsm_var_find_in_var( char* mod, char* var ) {

  fsm_var* curr;  /* Pointer to current FSM variable element in list */

  curr = fsm_var_head;
  while( (curr != NULL) && ((strcmp( mod, curr->mod ) != 0) || (fsm_sig_find( curr->ivar_head, var ) == NULL)) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param mod  Name of current module being parsed.
 \param var  Name of current signal being created.

 \return Returns pointer to found fsm_var structure containing matching
         signal; otherwise, returns NULL.

 Checks FSM variable list to see if any entries in this list match the
 specified mod and var values.  If an entry is found to match, return a
 pointer to the found structure to indicate to the calling function that
 the specified signal needs to have an FSM structure associated with it.
*/
fsm_var* fsm_var_find_out_var( char* mod, char* var ) {

  fsm_var* curr;  /* Pointer to current FSM variable element in list */

  curr = fsm_var_head;
  while( (curr != NULL) && ((strcmp( mod, curr->mod ) != 0) || (fsm_sig_find( curr->ovar_head, var ) == NULL)) ) {
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

  if( fsm_var_head != NULL ) {

    print_output( "The following FSM state variables were not found:", WARNING );
    print_output( "Module                     Variable", WARNING_WRAP );
    print_output( "-------------------------  -------------------------", WARNING_WRAP );

    curr = fsm_var_head;
    while( curr != NULL ) {
      if( curr->isig == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "%-25.25s  %-25.25s", curr->mod, curr->ivar_head->name );
        print_output( user_msg, WARNING_WRAP );
      }
      if( curr->table == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "%-25.25s  %-25.25s", curr->mod, curr->ovar_head->name );
        print_output( user_msg, WARNING_WRAP );
      }
      curr = curr->next;
    }

    print_output( "", WARNING_WRAP );

  }

}

/*!
 \param fv  Pointer to FSM variable to deallocate.

 Deallocates an FSM variable entry from memory.
*/
void fsm_var_dealloc( fsm_var* fv ) {

  if( fv != NULL ) {

    /* Deallocate both FSM signal lists */
    fsm_sig_delete_list( fv->ivar_head );
    fsm_sig_delete_list( fv->ovar_head );

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
*/

