/*!
 \file     param.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002

 The following is a list of the goals that I would ideally like for parameters:

 -#  All parameter information is encapsulated in a module structure (nothing in the
     instance structures).

 -#  Parameter support should have a minimal impact on parsing performance.

 -#  All parameter information should be handled by the time that the parsed information
     is initially output to the CDD file.

 -#  The defparam statement will be ignored for all found statements and a warning generated
     to standard error for all found defparams.

 -#  Parameter overloading will be allowed at the instance declaration for instances that
     are deemed to be parsed.

 -#  A Verilog module need only be parsed once.

 -#  No module copying is to occur.

 -#  Parameter structure is to be as condensed as possible.  A parameter structure should
     contain at least the following information:
     - Name of parameter in module OR relative hierarchical name of parameter in submodule.
     - Pointer to vector containing value of parameter or parameter override (might require
       expression tree instead of vector -- root of expression tree eventually contains
       vector value of parameter).

 -#  Any code allowed for assigning parameters is allowed (with the noted exception of the
     defparam statement).


 So how do all of these goals get met?

 -#  All parameter information can be stored in the module structure by storing parameter
     assignments and parameter overloads (for submodules) in current module.

 -#  Both signals and expressions will need to change the way that they write themselves to
     the CDD file.  If a signal's width/lsb is determined with parameters, the signal and
     associated expressions will need to have width's and/or lsb's reset to new values.
     If an expression's value is determined with parameters, the expression will need
     to have width's reset to new values.

 -#  If all Verilog information can be contained in the module structure only, it will not
     be necessary to reparse a module or to copy an existing module.

 -#  It is important to note that parameters can have chain reactions in submodule evaluations
     due to instance parameter overriding of parameters that are used to override other
     submodule instance parameters.  Therefore, all parameters will need to be evaluated in
     a top-down manner (root module will need to be evaluated first followed by its children, etc.)

 -#  All defparam values supplied by the user must be static values (no expressions allowed).

 -#  Defparam overrides should be applied before default values to eliminate unnecessary default
     parameter expression evaluation.

 -#  Expression trees need to be stored in module parameter lists for parameter expressions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "param.h"
#include "util.h"
#include "link.h"
#include "signal.h"
#include "expr.h"


parameter* defparam_head = NULL;   /*!< Pointer to head of parameter list for global defparams */
parameter* defparam_tail = NULL;   /*!< Pointer to tail of parameter list for global defparams */


/*!
 \param name  Name of parameter value to find.
 \param parm  Pointer to head of parameter list to search.

 \return Returns pointer to found parameter or NULL if parameter is not
         found.

 Searches specified parameter list for a parameter that matches the name of
 the specified parameter.  If a match is found, a pointer to the found
 parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.

 Note:  Necessary
*/
parameter* param_find( char* name, parameter* parm ) {

  assert( name != NULL );

  while( (parm != NULL) && (strcmp( parm->name, name ) != 0) ) {
    parm = parm->next;
  }

  return( parm );
 
}

/*!
 \param name  Name of parameter to search for in current list.
 \param head  Pointer to head of parameter list to search.
 \param tail  Pointer to tail of parameter list to search.

 \return Returns pointer to found parameter or NULL if parameter was not found.

 Iterates through specified parameter list searching for parameter whose name matches
 the specified string.  If a match is found, it is removed from the specified list and
 returned to the calling function.  If no match is found, nothing is removed from the 
 list and we return NULL to the calling function.
*/
parameter* param_find_and_remove( char* name, parameter** head, parameter** tail ) {

  parameter* curr;   /* Pointer to current parameter value */
  parameter* last;   /* Pointer to last parameter value    */

  assert( name != NULL );

  last = *head;
  curr = *head;
  while( (curr != NULL) && (strcmp( curr->name, name ) != 0) ) {
    last = curr;
    curr = curr->next;
  } 

  /* If found, remove from list */
  if( curr != NULL ) {

    /* Remove parameter from list but do not deallocated it */
    if( (curr == *head) && (curr == *tail) ) {
      *head = *tail = NULL;
    } else if( curr == *head ) {
      *head = curr->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail = last;
    } else {
      last->next = curr->next;
    }

  }

  return( curr );

} 

/*!
 \param parm  Pointer to parameter to add to specified list.
 \param head  Pointer to head parameter in list.
 \param tail  Pointer to tail parameter in list.

 Adds specified parameter to the tail of the specified parameter list.

 Note:  Necessary
*/
void param_add_to_list( parameter* parm, parameter** head, parameter** tail ) {

  if( *head == NULL ) {
    *head = *tail = parm;
  } else {
    (*tail)->next = parm;
    *tail         = parm;
  }

}

/*!
 \param name   Full hierarchical name of parameter value.
 \param expr   Expression to calculate parameter value.
 \param mod    Pointer to module to store default parameter value.

 Creates a new parameter with the specified information and adds it to the 
 instance parameter list.  This function is only called when a parameter
 is found in a particular module.

 Note:  Necessary
*/
void param_add( char* name, expression* expr, module* mod ) {

  parameter* parm;    /* Temporary pointer to parameter */
  
  assert( name != NULL );

  /* Create new signal/expression binding */
  parm       = (parameter *)malloc_safe( sizeof( parameter ) );
  parm->name = strdup( name );
  parm->expr = expr;
  parm->next = NULL;

  /* Now add the parameter to the current expression */
  param_add_to_list( parm, &(mod->param_head), &(mod->param_tail) );

}

/*!
 \param scope  Full hierarchical reference to specified scope to change value to.
 \param value  User-specified parameter override value.

 Scans list of all parameters to make sure that specified parameter isn't already
 being set to a new value.  If no match occurs, creates a new parameter link and
 places at the tail of the parameter list.  If match is found, display error
 message to user and exit covered immediately.  This function is called for each
 -P option to the score command.

 Note:  Necessary -- used by score.c
*/
void param_add_defparam( char* scope, vector* value ) {

  parameter*  parm;           /* Newly created parameter value      */
  expression* expr;           /* Expression containing vector value */
  char        err_msg[4096];  /* Error message to display to user   */

  assert( scope != NULL );

  if( param_find( scope, defparam_head ) == NULL ) {

    assert( expr != NULL );

    expr = expression_create( NULL, NULL, EXP_OP_STATIC, 0, 0 );
    vector_dealloc( expr->value );
    expr->value = value;

    parm = (parameter *)malloc_safe( sizeof( parameter ) );
    parm->name = strdup( scope );
    parm->expr = expr;
    parm->next = NULL;

    param_add_to_list( parm, &defparam_head, &defparam_tail );

  } else {

    snprintf( err_msg, 4096, "Parameter (%s) value is assigned more than once", scope );
    print_output( err_msg, FATAL );
    exit( 1 );

  }

}

/* $Log$
/* Revision 1.4  2002/09/06 03:05:28  phase1geo
/* Some ideas about handling parameters have been added to these files.  Added
/* "Special Thanks" section in User's Guide for acknowledgements to people
/* helping in project.
/*
/* Revision 1.3  2002/08/27 11:53:16  phase1geo
/* Adding more code for parameter support.  Moving parameters from being a
/* part of modules to being a part of instances and calling the expression
/* operation function in the parameter add functions.
/*
/* Revision 1.2  2002/08/26 12:57:04  phase1geo
/* In the middle of adding parameter support.  Intermediate checkin but does
/* not break regressions at this point.
/*
/* Revision 1.1  2002/08/23 12:55:33  phase1geo
/* Starting to make modifications for parameter support.  Added parameter source
/* and header files, changed vector_from_string function to be more verbose
/* and updated Makefiles for new param.h/.c files.
/* */

