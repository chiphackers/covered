/*!
 \file     param.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002
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


parameter* param_head = NULL;
parameter* param_tail = NULL;

parameter* defparam_head = NULL;
parameter* defparam_tail = NULL;


/*!
 \param name  Name of parameter value to find.
 \param parm  Pointer to head of parameter list to search.

 \return Returns pointer to found parameter or NULL if parameter is not
         found.

 Searches specified parameter list for a parameter that matches the name of
 the specified parameter.  If a match is found, a pointer to the found
 parameter is returned to the calling function; otherwise, a value of NULL
 is returned if no match was found.
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
 \param mod    Pointer to module to add parameter to.

 Creates a new parameter with the specified information and
 adds it to the end of the global parameter list.  This
 function is only called when a parameter is found in a
 particular module.
*/
void param_add( char* name, expression* expr, module* mod ) {

  parameter* parm;     /* Temporary pointer to parameter */
  parameter* defparm;  /* Pointer to found defparam      */
  
  /* Create new signal/expression binding */
  parm       = (parameter *)malloc_safe( sizeof( parameter ) );
  parm->name = strdup( name );
  parm->expr = expr;
  parm->next = NULL;

  /* Search defparam list and substitute expression trees if match is found */
  if( (defparm = param_find_and_remove( name, &defparam_head, &defparam_tail )) != NULL ) {

    /* Exchange expression values */
    expression_dealloc( parm->expr, TRUE );
    parm->expr = defparm->expr;
    
    /* Remove found defparam */
    free_safe( defparm->name );
    free_safe( defparm );

  }
    
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
*/
void param_add_defparam( char* scope, vector* value ) {

  parameter*  parm;           /* Newly created parameter value      */
  expression* expr;           /* Expression containing vector value */
  char        err_msg[4096];  /* Error message to display to user   */

  assert( scope != NULL );

  if( param_find( scope, param_head ) == NULL ) {

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
/* Revision 1.1  2002/08/23 12:55:33  phase1geo
/* Starting to make modifications for parameter support.  Added parameter source
/* and header files, changed vector_from_string function to be more verbose
/* and updated Makefiles for new param.h/.c files.
/* */

