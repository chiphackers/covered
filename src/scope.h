#ifndef __SCOPE_H__
#define __SCOPE_H__

/*!
 \file     scope.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/10/2005
 \brief    Contains functions for looking up structures for the given scope
*/


#include "defines.h"


/*! \brief  Finds the functional unit that corresponds to the given scope relative to the current
            functional unit */
func_unit* scope_find_funit_from_scope( char* scope, func_unit* curr_funit );

/*! \brief  Find the given signal in the provided scope */
vsignal* scope_find_signal( char* name, func_unit* funit );

/*! \brief  Finds the given task or function in the provided scope. */
func_unit* scope_find_task_function( char* name, int type, func_unit* funit );


/* $Log$
*/

#endif

