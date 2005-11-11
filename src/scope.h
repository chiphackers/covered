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
bool scope_find_signal( char* name, func_unit* curr_funit, vsignal** found_sig, func_unit** found_funit );

/*! \brief  Finds the given task or function in the provided scope. */
bool scope_find_task_function( char* name, int type, func_unit* curr_funit, func_unit** found_funit );


/* $Log$
/* Revision 1.1  2005/11/10 23:27:37  phase1geo
/* Adding scope files to handle scope searching.  The functions are complete (not
/* debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
/* which brings out scoping issues for functions.
/*
*/

#endif

