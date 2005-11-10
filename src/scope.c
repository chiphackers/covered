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
vsignal* scope_find_signal( char* name, func_unit* curr_funit ) {

  vsignal    sig;                     /* Temporary holder for signal */
  sig_link*  sigl;                    /* Pointer to current signal link */
  char*      sig_name;                /* Base signal name holder */
  char*      scope;                   /* Signal scope holder */
  func_unit* tmp_funit = curr_funit;  /* Pointer to original functional unit for error reporting purposes */

  assert( curr_funit != NULL );

  /* If there is a hierarchical reference being made, adjust the signal name and current functional unit */
  if( !scope_local( name ) ) {

    sig_name = (char *)malloc_safe( strlen( name ) + 1, __FILE__, __LINE__ );
    scope    = (char *)malloc_safe( strlen( name ) + 1, __FILE__, __LINE__ );

    /* Extract the signal name from its scope */
    scope_extract_back( name, sig_name, scope );

    /* Get the functional unit that contains this signal */
    if( (curr_funit = scope_find_funit_from_scope( scope, curr_funit )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined signal hierarchy (%s) in %s %s, file %s",
                name, get_funit_type( tmp_funit->type ), tmp_funit->name, tmp_funit->filename );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
 
    } else {

      curr_funit = tmp_funit;
      sig.name   = sig_name;

    }

    free_safe( sig_name );
    free_safe( scope );

  } else {
  
    sig.name = name;

  }

  /* First, look in the current functional unit */
  if( (sigl = sig_link_find( &sig, curr_funit->sig_head )) == NULL ) {

    /* Look in parent module if we are a task, function or named block */
    sigl = sig_link_find( &sig, curr_funit->tf_head->funit->sig_head );

  }

  return( (sigl == NULL) ? NULL : sigl->sig );

}

func_unit* scope_find_task_function( char* name, int type, func_unit* curr_funit ) {

  func_unit   funit;  /* Temporary holder of task */
  funit_link* funitl;
  func_unit*  tmp_funit = curr_funit;

  assert( (type == FUNIT_FUNCTION) || (type == FUNIT_TASK) );
  assert( curr_funit != NULL );

  funit.name = name;
  funit.type = type;

  if( !scope_local( name ) ) {

    if( (curr_funit = scope_find_funit_from_scope( name, curr_funit )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined %s hierarchy in %s %s, file %s",
                get_funit_type( type ), get_funit_type( tmp_funit->type ), name, tmp_funit->filename );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );

    }

  }

  /* Look in the current functional unit if we are a module */
  if( curr_funit->type == FUNIT_MODULE ) {

    funitl = funit_link_find( &funit, curr_funit->tf_head );

  /* Otherwise, look in the parent module for the task */
  } else {

    funitl = funit_link_find( &funit, curr_funit->tf_head->funit->tf_head );

  }

  return( (funitl == NULL) ? NULL : funitl->funit );    

}


/* $Log$
*/

