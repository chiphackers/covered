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
 \file     scope.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/10/2005
*/

#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "scope.h"
#include "link.h"
#include "instance.h"
#include "util.h"
#include "func_unit.h"
#include "gen_item.h"
#include "obfuscate.h"


extern db**         db_list;
extern unsigned int curr_db;
extern func_unit*   global_funit;
extern char         user_msg[USER_MSG_LENGTH];


/*!
 \param scope       Verilog hierachical scope to a functional unit.
 \param curr_funit  Pointer to current functional unit whose member is calling this function
 \param rm_unnamed  Set to TRUE to cause unnamed scopes to be discarded

 \return Returns a pointer to the functional unit being scoped if it can be found; otherwise, returns
         a value of NULL.

 Searches the instance structure for the specified scope.  Initially searches the tree assuming the
 user is attempting to do a relative hierarchical reference.  If the reference is not relative, attempts
 a top-of-tree search.  The specified scope should only be for a functional unit.  If the user is attempting
 to get the functional unit for a signal, the signal name should be removed prior to calling this function.
*/
static func_unit* scope_find_funit_from_scope(
  const char* scope,
  func_unit*  curr_funit,
  bool        rm_unnamed
) { PROFILE(SCOPE_FIND_FUNIT_FROM_SCOPE);

  funit_inst* curr_inst;      /* Pointer to current instance */
  funit_inst* funiti = NULL;  /* Pointer to functional unit instance found */
  int         ignore = 0;     /* Used for functional unit instance search */
  char        tscope[4096];   /* Temporary scope value */

  assert( curr_funit != NULL );

  /* Get current instance */
  if( (curr_inst = inst_link_find_by_funit( curr_funit, db_list[curr_db]->inst_head, &ignore )) != NULL ) {

    /* First check scope based on a relative path if unnamed scopes are not ignored */
    unsigned int rv = snprintf( tscope, 4096, "%s.%s", curr_inst->name, scope );
    assert( rv < 4096 );
    funiti = instance_find_scope( curr_inst, tscope, rm_unnamed );

    /*
     If we still did not find the functional unit, iterate up the scope tree looking for a module
     that matches.
    */
    if( funiti == NULL ) {
      do {
        if( curr_inst->parent == NULL ) {
          strcpy( tscope, scope );
          funiti = instance_find_scope( curr_inst, tscope, rm_unnamed );
          curr_inst = curr_inst->parent;
        } else {
          unsigned int rv;
          curr_inst = curr_inst->parent;
          rv = snprintf( tscope, 4096, "%s.%s", curr_inst->name, scope );
          assert( rv < 4096 );
          funiti = instance_find_scope( curr_inst, tscope, rm_unnamed );
        }
      } while( (curr_inst != NULL) && (funiti == NULL) );
    }

  }

  PROFILE_END;

  return( (funiti == NULL) ? NULL : funiti->funit );

}

#ifndef RUNLIB
/*!
 \param name         Name of parameter to find in design
 \param curr_funit   Pointer to current functional unit to start searching in
 \param found_parm   Pointer to module parameter that has been found in the design
 \param found_funit  Pointer to found signal's functional unit
 \param line         Line number where signal is being used (used for error output)

 \return Returns TRUE if specified signal was found in the design; otherwise, returns FALSE.

 \throws anonymous Throw

 Searches for a given parameter in the design starting with the functional unit in which the parameter is being
 accessed from.  Attempts to find the parameter locally (if the name is not hierarchically referenced); otherwise,
 performs relative referencing to find the parameter.  If the parameter is found, the found_parm and found_funit pointers
 are set to the found module parameter and its functional unit; otherwise, a value of FALSE is returned to the
 calling function.
*/
bool scope_find_param(
  const char* name,
  func_unit*  curr_funit,
  mod_parm**  found_parm,
  func_unit** found_funit,
  int         line
) { PROFILE(SCOPE_FIND_PARAM);

  char* parm_name;  /* Parameter basename holder */
  char* scope;      /* Parameter scope holder */

  assert( curr_funit != NULL );

  *found_funit = curr_funit;
  parm_name    = strdup_safe( name );

  Try {

    /* If there is a hierarchical reference being made, adjust the signal name and current functional unit */
    if( !scope_local( name ) ) {

      scope = (char *)malloc_safe( strlen( name ) + 1 );

      Try {

        /* Extract the signal name from its scope */
        scope_extract_back( name, parm_name, scope );

        /* Get the functional unit that contains this signal */
        if( (*found_funit = scope_find_funit_from_scope( scope, curr_funit, TRUE )) == NULL ) {

          if( line > 0 ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined signal hierarchy (%s) in %s %s, file %s, line %d",
                                        obf_sig( name ), get_funit_type( curr_funit->suppl.part.type ), obf_funit( curr_funit->name ),
                                        obf_file( curr_funit->orig_fname ), line );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
 
        }

      } Catch_anonymous {
        free_safe( scope, (strlen( name ) + 1) );
        Throw 0;
      }

      free_safe( scope, (strlen( name ) + 1) );

    }

    /* Get the module parameter, if it exists */
    *found_parm = funit_find_param( parm_name, *found_funit );

    /* If we could not find the module parameter in the found_funit, search the global funit, if it exists */
    if( (*found_parm == NULL) && (global_funit != NULL) ) {
      *found_funit = global_funit;
      *found_parm  = funit_find_param( parm_name, *found_funit );
    }

  } Catch_anonymous {
    free_safe( parm_name, (strlen( name ) + 1) );
    Throw 0;
  }

  free_safe( parm_name, (strlen( name ) + 1) );

  PROFILE_END;

  return( *found_parm != NULL );

}
#endif /* RUNLIB */

/*!
 \param name         Name of signal to find in design
 \param curr_funit   Pointer to current functional unit to start searching in
 \param found_sig    Pointer to signal that has been found in the design
 \param found_funit  Pointer to found signal's functional unit
 \param line         Line number where signal is being used (used for error output)

 \return Returns TRUE if specified signal was found in the design; otherwise, returns FALSE.

 \throws anonymous Throw

 Searches for a given signal in the design starting with the functional unit in which the signal is being
 accessed from.  Attempts to find the signal locally (if the signal name is not hierarchically referenced); otherwise,
 performs relative referencing to find the signal.  If the signal is found the found_sig and found_funit pointers
 are set to the found signal and its functional unit; otherwise, a value of FALSE is returned to the calling function.
*/
bool scope_find_signal(
  const char* name,
  func_unit*  curr_funit,
  vsignal**   found_sig,
  func_unit** found_funit,
  int         line
) { PROFILE(SCOPE_FIND_SIGNAL);

  char* sig_name;  /* Signal basename holder */
  char* scope;     /* Signal scope holder */

  assert( curr_funit != NULL );

  *found_funit = curr_funit;
  *found_sig   = NULL;

  sig_name = strdup_safe( name );

  Try {

    /* If there is a hierarchical reference being made, adjust the signal name and current functional unit */
    if( !scope_local( name ) ) {

      scope = (char *)malloc_safe( strlen( name ) + 1 );

      Try {

        /* Extract the signal name from its scope */
        scope_extract_back( name, sig_name, scope );

        /* Get the functional unit that contains this signal */
        if( (*found_funit = scope_find_funit_from_scope( scope, curr_funit, TRUE )) == NULL ) {

          if( line > 0 ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined signal hierarchy (%s) in %s %s, file %s, line %d",
                                        obf_sig( name ), get_funit_type( curr_funit->suppl.part.type ), obf_funit( curr_funit->name ),
                                        obf_file( curr_funit->orig_fname ), line );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
 
        }

      } Catch_anonymous {
        free_safe( scope, (strlen( name ) + 1) );
        Throw 0;
      }

      free_safe( scope, (strlen( name ) + 1) );

    }

    if( *found_funit != NULL ) {

      /* First, look in the current functional unit */
      if( (*found_sig = funit_find_signal( sig_name, *found_funit )) == NULL ) {
  
        /* Continue to look in parent modules (if there are any) */
        *found_funit = (*found_funit)->parent;
        while( (*found_funit != NULL) && ((*found_sig = funit_find_signal( sig_name, *found_funit )) == NULL) ) {
          *found_funit = (*found_funit)->parent;
        }

        /* If we could still not find the signal, look in the global funit, if it exists */
        if( (*found_sig == NULL) && (global_funit != NULL) ) {
          *found_funit = global_funit;
          *found_sig = funit_find_signal( sig_name, *found_funit );
        }

      }

    }

  } Catch_anonymous {
    free_safe( sig_name, (strlen( name ) + 1) );
    Throw 0;
  }

  free_safe( sig_name, (strlen( name ) + 1) );

  PROFILE_END;

  return( *found_sig != NULL );

}

/*!
 \return Returns TRUE if the functional unit was found in the design; otherwise, returns FALSE.

 \throws anonymous Throw

 Searches the design for the specified functional unit based on its scoped name.  If the functional unit is
 found, the found_funit pointer is set to the functional unit and the function returns TRUE; otherwise, the function
 returns FALSE to the calling function.
*/
bool scope_find_task_function_namedblock(
  const char* name,         /*!< Name of functional unit to find based on scope information */
  int         type,         /*!< Type of functional unit to find */
  func_unit*  curr_funit,   /*!< Pointer to the functional unit which needs to bind to this functional unit */
  func_unit** found_funit,  /*!< Pointer to found functional unit within the design */
  int         line,         /*!< Line number where functional unit is being used (for error output purposes only) */
  bool        must_find,    /*!< Set to TRUE if the scope MUST be found */
  bool        rm_unnamed    /*!< Set to TRUE if unnamed scopes should be ignored */
) { PROFILE(SCOPE_FIND_TASK_FUNCTION_NAMEDBLOCK);

  assert( (type == FUNIT_FUNCTION)  || (type == FUNIT_TASK)  || (type == FUNIT_NAMED_BLOCK) ||
          (type == FUNIT_AFUNCTION) || (type == FUNIT_ATASK) || (type == FUNIT_ANAMED_BLOCK) );
  assert( curr_funit != NULL );

  /*
   Find the functional unit that refers to this scope.
  */
  if( ((*found_funit = scope_find_funit_from_scope( name, curr_funit, rm_unnamed )) == NULL) && must_find ) {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined %s hierarchy (%s) in %s %s, file %s, line %d",
                                get_funit_type( type ), obf_funit( name ), get_funit_type( curr_funit->suppl.part.type ),
                                obf_funit( curr_funit->name ), obf_file( curr_funit->orig_fname ), line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

  return( *found_funit != NULL );

}

/*!
 \return Returns a pointer to the parent functional unit of this functional unit.

 \note This function should only be called when the scope refers to a functional unit
       that is NOT a module!
*/
func_unit* scope_get_parent_funit(
  funit_inst* root,  /*!< Pointer to root of instance tree to search */
  const char* scope  /*!< Scope of current functional unit to get parent functional unit for */
) { PROFILE(SCOPE_GET_PARENT_FUNIT);

  funit_inst* inst;     /* Pointer to functional unit instance with the specified scope */
  char*       rest;     /* Temporary holder */
  char*       back;     /* Temporary holder */
  int         str_len;  /* Length of string to allocated/deallocate */

  /* Calculate the str_len */
  str_len = strlen( scope ) + 1;

  rest = (char*)malloc_safe( str_len );
  back = (char*)malloc_safe( str_len );

  /* Go up one in hierarchy */
  scope_extract_back( scope, back, rest );

  assert( rest != '\0' );

  /* Get functional instance for the rest of the scope */
  inst = instance_find_scope( root, rest, FALSE );

  assert( inst != NULL );

  free_safe( rest, str_len );
  free_safe( back, str_len );

  PROFILE_END;

  return( inst->funit );

}

/*!
 \return Returns pointer to module that is the parent of the specified functional unit.

 \note Assumes that the given scope is not that of a module itself!
*/
func_unit* scope_get_parent_module(
  funit_inst* root,  /*!< Pointer to root of instance tree to search */
  const char* scope  /*!< Full hierarchical scope of functional unit to find parent module for */
) { PROFILE(SCOPE_GET_PARENT_MODULE);

  funit_inst* inst;        /* Pointer to functional unit instance with the specified scope */
  char*       curr_scope;  /* Current scope to search for */
  char*       rest;        /* Temporary holder */
  char*       back;        /* Temporary holder */
  int         str_len;     /* Length of string to allocate */

  assert( scope != NULL );

  /* Calculate the length of the string */
  str_len = strlen( scope ) + 1;

  /* Get a local copy of the specified scope */
  curr_scope = strdup_safe( scope );
  rest       = strdup_safe( scope );
  back       = strdup_safe( scope );

  do {
    scope_extract_back( curr_scope, back, rest );
    assert( rest[0] != '\0' );
    strcpy( curr_scope, rest );
    inst = instance_find_scope( root, curr_scope, FALSE );
    assert( inst != NULL );
  } while( inst->funit->suppl.part.type != FUNIT_MODULE );

  free_safe( curr_scope, str_len );
  free_safe( rest,       str_len );
  free_safe( back,       str_len );

  PROFILE_END;

  return( inst->funit );

}
