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
#include "func_unit.h"
#include "gen_item.h"


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

  funit_inst* curr_inst;      /* Pointer to current instance */
  funit_inst* funiti = NULL;  /* Pointer to functional unit instance found */
  func_unit*  funit;          /* Pointer to parent functional unit */
  int         ignore = 0;     /* Used for functional unit instance search */
  char        tscope1[4096];  /* Temporary scope value */
  char        tscope2[4096];  /* Temporary scope value */

  assert( curr_funit != NULL );

  /* Get current instance */
  curr_inst = instance_find_by_funit( instance_root, curr_funit, &ignore );
  assert( curr_inst != NULL );

  /* First check scope based on a relative path */
  snprintf( tscope1, 4096, "%s.%s", curr_inst->name, scope );
  funiti = instance_find_scope( curr_inst, tscope1 );

  /*
   If we did not find the functional unit yet, check the scope relative to the parent module
   (if this functional unit is not a module)
  */
  if( (funiti == NULL) && (curr_funit->type != FUNIT_MODULE) ) {
    funit = funit_get_curr_module( curr_funit );
    curr_inst = instance_find_by_funit( instance_root, funit, &ignore );
    assert( curr_inst != NULL );
    snprintf( tscope1, 4096, "%s.%s", curr_inst->name, scope );
    funiti = instance_find_scope( curr_inst, tscope1 );
  }

  /*
   If we still did not find the functional unit, iterate up the scope tree looking for a module
   that matches.
  */
  if( funiti == NULL ) {
    do {
      if( curr_inst->parent == NULL ) {
        strcpy( tscope1, scope );
        funiti = instance_find_scope( curr_inst, tscope1 );
        curr_inst = curr_inst->parent;
      } else {
        curr_inst = curr_inst->parent;
        snprintf( tscope1, 4096, "%s.%s", curr_inst->name, scope );
        funiti = instance_find_scope( curr_inst, tscope1 );
      }
    } while( (curr_inst != NULL) && (funiti == NULL) );
  }

  return( (funiti == NULL) ? NULL : funiti->funit );

}

/*!
 \param name         Name of parameter to find in design
 \param curr_funit   Pointer to current functional unit to start searching in
 \param found_parm   Pointer to module parameter that has been found in the design
 \param found_funit  Pointer to found signal's functional unit
 \param line         Line number where signal is being used (used for error output)

 \return Returns TRUE if specified signal was found in the design; otherwise, returns FALSE.

 Searches for a given parameter in the design starting with the functional unit in which the parameter is being
 accessed from.  Attempts to find the parameter locally (if the name is not hierarchically referenced); otherwise,
 performs relative referencing to find the parameter.  If the parameter is found, the found_parm and found_funit pointers
 are set to the found module parameter and its functional unit; otherwise, a value of FALSE is returned to the
 calling function.
*/
bool scope_find_param( char* name, func_unit* curr_funit, mod_parm** found_parm, func_unit** found_funit, int line ) {

  char* parm_name;  /* Parameter basename holder */
  char* scope;      /* Parameter scope holder */

  assert( curr_funit != NULL );

  *found_funit = curr_funit;
  parm_name    = strdup_safe( name, __FILE__, __LINE__ );

  /* If there is a hierarchical reference being made, adjust the signal name and current functional unit */
  if( !scope_local( name ) ) {

    scope = (char *)malloc_safe( strlen( name ) + 1, __FILE__, __LINE__ );

    /* Extract the signal name from its scope */
    scope_extract_back( name, parm_name, scope );

    /* Get the functional unit that contains this signal */
    if( (*found_funit = scope_find_funit_from_scope( scope, curr_funit )) == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Referencing undefined signal hierarchy (%s) in %s %s, file %s, line %d",
                name, get_funit_type( curr_funit->type ), curr_funit->name, curr_funit->filename, line );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
 
    }

    free_safe( scope );

  }

  /* Get the module parameter, if it exists */
  *found_parm = funit_find_param( parm_name, *found_funit );

  free_safe( parm_name );

  return( *found_parm != NULL );

}

/*!
 \param name         Name of signal to find in design
 \param curr_funit   Pointer to current functional unit to start searching in
 \param found_sig    Pointer to signal that has been found in the design
 \param found_funit  Pointer to found signal's functional unit
 \param line         Line number where signal is being used (used for error output)

 \return Returns TRUE if specified signal was found in the design; otherwise, returns FALSE.

 Searches for a given signal in the design starting with the functional unit in which the signal is being
 accessed from.  Attempts to find the signal locally (if the signal name is not hierarchically referenced); otherwise,
 performs relative referencing to find the signal.  If the signal is found the found_sig and found_funit pointers
 are set to the found signal and its functional unit; otherwise, a value of FALSE is returned to the calling function.
*/
bool scope_find_signal( char* name, func_unit* curr_funit, vsignal** found_sig, func_unit** found_funit, int line ) {

  vsignal     sig;       /* Temporary holder for signal */
  sig_link*   sigl;      /* Pointer to current signal link */
  char*       sig_name;  /* Signal basename holder */
  char*       scope;     /* Signal scope holder */
  func_unit*  parent;    /* Pointer to parent functional unit */

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
  if( (*found_sig = funit_find_signal( sig_name, *found_funit )) == NULL ) {

    /* Continue to look in parent modules (if there are any) */
    parent = (*found_funit)->parent;
    while( (parent != NULL) && ((*found_sig = funit_find_signal( sig_name, parent )) == NULL) ) {
      parent = parent->parent;
    }

  }

  free_safe( sig_name );

  return( *found_sig != NULL );

}

/*!
 \param name         Name of functional unit to find based on scope information
 \param type         Type of functional unit to find
 \param curr_funit   Pointer to the functional unit which needs to bind to this functional unit
 \param found_funit  Pointer to found functional unit within the design.
 \param line         Line number where functional unit is being used (for error output purposes only).

 \return Returns TRUE if the functional unit was found in the design; otherwise, returns FALSE.

 Searches the design for the specified functional unit based on its scoped name.  If the functional unit is
 found, the found_funit pointer is set to the functional unit and the function returns TRUE; otherwise, the function
 returns FALSE to the calling function.
*/
bool scope_find_task_function_namedblock( char* name, int type, func_unit* curr_funit, func_unit** found_funit, int line ) {

  funit_link* funitl;         /* Pointer to current functional unit link */
  char        rest[4096];     /* Temporary string */
  char        back[4096];     /* Temporary string */
  bool        found = FALSE;  /* Specifies if function unit has been found */
  func_unit*  parent;         /* Pointer to the parent module */

  assert( (type == FUNIT_FUNCTION) || (type == FUNIT_TASK) || (type == FUNIT_NAMED_BLOCK) );
  assert( curr_funit != NULL );

  *found_funit = curr_funit;

  /*
   If we are performing a hierarchical reference to a task/function/named block, find the functional unit
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

  /* Get the current module */
  parent = funit_get_curr_module( *found_funit );

  /* First, attempt to find the functional unit relative to the current functional unit */
  funitl = parent->tf_head;
  while( (funitl != NULL) && !found ) {
    snprintf( rest, 4096, "%s.%s", (*found_funit)->name, name );
    if( strcmp( funitl->funit->name, rest ) == 0 ) {
      found = TRUE;
      *found_funit = funitl->funit;
    } else {
      funitl = funitl->next;
    }
  }
  
  /* Search for functional unit in the module's tf_head list */
  if( !found ) {
    funitl = parent->tf_head;
    while( (funitl != NULL) && !found ) {
      scope_extract_back( funitl->funit->name, back, rest );
      if( scope_compare( back, name ) ) {
        found        = TRUE;
        *found_funit = funitl->funit;
      } else {
        funitl = funitl->next;
      }
    }
  }

  return( found );

}

/*!
 \param scope  Scope of current functional unit to get parent functional unit for

 \return Returns a pointer to the parent functional unit of this functional unit.

 \note This function should only be called when the scope refers to a functional unit
       that is NOT a module!
*/
func_unit* scope_get_parent_funit( char* scope ) {

  funit_inst* inst;  /* Pointer to functional unit instance with the specified scope */
  char*       rest;  /* Temporary holder */
  char*       back;  /* Temporary holder */

  rest = (char*)malloc_safe( (strlen( scope ) + 1), __FILE__, __LINE__ );
  back = (char*)malloc_safe( (strlen( scope ) + 1), __FILE__, __LINE__ );

  /* Go up one in hierarchy */
  scope_extract_back( scope, back, rest );

  assert( rest != '\0' );

  /* Get functional instance for the rest of the scope */
  inst = instance_find_scope( instance_root, rest );

  assert( inst != NULL );

  free_safe( rest );
  free_safe( back );

  return( inst->funit );

}

/*!
 \param scope  Full hierarchical scope of functional unit to find parent module for.

 \return Returns pointer to module that is the parent of the specified functional unit.

 \note Assumes that the given scope is not that of a module itself!
*/
func_unit* scope_get_parent_module( char* scope ) {

  funit_inst* inst;        /* Pointer to functional unit instance with the specified scope */
  char*       curr_scope;  /* Current scope to search for */
  char*       rest;        /* Temporary holder */
  char*       back;        /* Temporary holder */

  assert( scope != NULL );

  /* Get a local copy of the specified scope */
  curr_scope = strdup_safe( scope, __FILE__, __LINE__ );
  rest       = strdup_safe( scope, __FILE__, __LINE__ );
  back       = strdup_safe( scope, __FILE__, __LINE__ );

  do {
    scope_extract_back( curr_scope, back, rest );
    assert( rest[0] != '\0' );
    strcpy( curr_scope, rest );
    inst = instance_find_scope( instance_root, curr_scope );
    assert( inst != NULL );
  } while( inst->funit->type != FUNIT_MODULE );

  free_safe( curr_scope );
  free_safe( rest );
  free_safe( back );

  return( inst->funit );

}


/*
 $Log$
 Revision 1.20  2006/07/26 06:22:27  phase1geo
 Fixing rest of issues with generate6 diagnostic.  Still need to know if I
 have broken regressions or not and there are plenty of cases in this area
 to test before I call things good.

 Revision 1.19  2006/07/26 03:33:20  phase1geo
 Update to named block scoping function.  Searching relatively before searching
 broadly; however, this is not working quite as expected yet.  More work to
 come.

 Revision 1.18  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.17  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.16  2006/07/21 15:52:41  phase1geo
 Checking in an initial working version of the generate structure.  Diagnostic
 generate1 passes.  Still a lot of work to go before we fully support generate
 statements, but this marks a working version to enhance on.  Full regression
 passes as well.

 Revision 1.15  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.14  2006/07/18 19:03:21  phase1geo
 Sync'ing up to the scoping fixes from the 0.4.6 stable release.

 Revision 1.13  2006/05/25 12:11:01  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.12.8.1.4.2  2006/07/18 17:22:34  phase1geo
 Fixed upwards name referencing bug (1524705) and reshaped some of the code associated
 with this functionality.  Added diagnostics to regression suite to fully
 test this new behavior.  Full regression passes.

 Revision 1.12.8.1.4.1  2006/07/18 04:16:34  phase1geo
 Attempting to fix upwards scope referencing issue.  Still looks like there
 is a problem in the instance_find_scope routine.  Checkpointing work.

 Revision 1.12.8.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.12  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.11  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.10  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.9  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.8  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.7  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.6  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.5  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.4  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.3  2005/11/16 05:41:31  phase1geo
 Fixing implicit signal creation in binding functions.

 Revision 1.2  2005/11/11 22:53:40  phase1geo
 Updated bind process to allow binding of structures from different hierarchies.
 Added task port signals to get added.

 Revision 1.1  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.
*/

