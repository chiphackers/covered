/*!
 \file     scope.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/10/2005
*/

#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "scope.h"
#include "link.h"
#include "instance.h"
#include "util.h"


extern funit_inst* instance_root;
extern char        user_msg[USER_MSG_LENGTH];


/*!
 \param scope       Verilog hierachical scope to a functional unit.
 \param curr_funit  Pointer to current functional unit whose member is calling this function

 \return Returns a pointer to the functional unit being scoped if it can be found; otherwise, returns
         a value of NULL.

 Searches the instance structure for the specified scope.  Initially searches the tree assuming the
 user is attempting to do a relative hierarchical reference.  If the reference is not relative, attempts
 a top-of-tree search.  The specified scope should only be for a functional unit.  If the user is attempting
 to get the functional unit for a signal, the signal name should be removed prior to calling this function.
*/
func_unit* scope_find_funit_from_scope( char* scope, func_unit* curr_funit ) {

  funit_inst* funiti;
  int         ignore = 0;

  assert( curr_funit != NULL );

  /* First check scope based on a relative path */
  if( (funiti = instance_find_by_funit( instance_root, curr_funit, &ignore )) != NULL ) {

    if( (funiti = instance_find_scope( funiti, scope )) == NULL ) {

      /* If the scope isn't relative, check the scope from top-of-tree */
      funiti = instance_find_scope( instance_root, scope );

    }

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find functional unit %s in hierarchy", curr_funit->name );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }
    
  return( (funiti == NULL) ? NULL : funiti->funit );

}

/*!
 \param name
 \param curr_funit

 \return

*/
bool scope_find_signal( char* name, func_unit* curr_funit, vsignal** found_sig, func_unit** found_funit, int line ) {

  vsignal    sig;       /* Temporary holder for signal */
  sig_link*  sigl;      /* Pointer to current signal link */
  char*      sig_name;  /* Signal basename holder */
  char*      scope;     /* Signal scope holder */

  assert( curr_funit != NULL );

  *found_funit = curr_funit;
  sig_name     = strdup_safe( name, __FILE__, __LINE__ );
  sig.name     = sig_name;

  /* If there is a hierarchical reference being made, adjust the signal name and current functional unit */
  if( !scope_local( name ) ) {

    scope = (char *)malloc_safe( strlen( name ) + 1, __FILE__, __LINE__ );

    /* Extract the signal name from its scope */
    scope_extract_back( name, sig_name, scope );

    /* Get the functional unit that contains this signal */
    if( (*found_funit = scope_find_funit_from_scope( scope, curr_funit )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined signal hierarchy (%s) in %s %s, file %s, line %d",
                name, get_funit_type( curr_funit->type ), curr_funit->name, curr_funit->filename, line );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
 
    }

    free_safe( scope );

  }

  /* First, look in the current functional unit */
  if( (sigl = sig_link_find( &sig, (*found_funit)->sig_head )) == NULL ) {

    /* Look in parent module if we are a task, function or named block */
    if( (*found_funit)->type != FUNIT_MODULE ) {
      sigl = sig_link_find( &sig, (*found_funit)->tf_head->funit->sig_head );
    }

  }

  free_safe( sig_name );

  /* Get found signal pointer if it can be found */
  *found_sig = (sigl == NULL) ? NULL : sigl->sig;

  return( sigl != NULL );

}

/*!
 \param name
 \param type
 \param curr_funit
 \param found_funit

 \return TBD

 TBD
*/
bool scope_find_task_function( char* name, int type, func_unit* curr_funit, func_unit** found_funit, int line ) {

  func_unit   funit;   /* Temporary holder of task */
  funit_link* funitl;  /* Pointer to current functional unit link */

  assert( (type == FUNIT_FUNCTION) || (type == FUNIT_TASK) );
  assert( curr_funit != NULL );

  *found_funit = curr_funit;
  funit.name   = name;
  funit.type   = type;

  /*
   If we are performing a hierarchical reference to a task/function, find the functional unit
   that refers to this scope.
  */
  if( !scope_local( name ) ) {

    if( (*found_funit = scope_find_funit_from_scope( name, curr_funit )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined %s hierarchy in %s %s, file %s, line %d",
                get_funit_type( type ), get_funit_type( curr_funit->type ), name, curr_funit->filename, line );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );

    }

  }

  /* Look in the current functional unit if we are a module */
  if( (*found_funit)->type == FUNIT_MODULE ) {

    funitl = funit_link_find( &funit, (*found_funit)->tf_head );

  /* Otherwise, look in the parent module for the task */
  } else {

    assert( (*found_funit)->tf_head != NULL );
    funitl = funit_link_find( &funit, (*found_funit)->tf_head->funit->tf_head );

  }

  *found_funit = (funitl == NULL) ? NULL : funitl->funit;

  return( funitl != NULL );    

}


/* $Log$
/* Revision 1.3  2005/11/16 05:41:31  phase1geo
/* Fixing implicit signal creation in binding functions.
/*
/* Revision 1.2  2005/11/11 22:53:40  phase1geo
/* Updated bind process to allow binding of structures from different hierarchies.
/* Added task port signals to get added.
/*
/* Revision 1.1  2005/11/10 23:27:37  phase1geo
/* Adding scope files to handle scope searching.  The functions are complete (not
/* debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
/* which brings out scoping issues for functions.
/*
*/

