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
bool scope_find_signal( char* name, func_unit* curr_funit, vsignal** found_sig, func_unit** found_funit, int line );

/*! \brief  Finds the given task or function in the provided scope. */
bool scope_find_task_function_namedblock( char* name, int type, func_unit* curr_funit, func_unit** found_funit, int line );

/*! \brief  Finds the parent functional unit of the functional unit with the given scope */
func_unit* scope_get_parent_funit( char* scope );

/*! \brief  Finds the parent module of the functional unit with the given scope */
func_unit* scope_get_parent_module( char* scope );


/* $Log$
/* Revision 1.4  2005/11/29 23:14:37  phase1geo
/* Adding support for named blocks.  Still not working at this point but checkpointing
/* anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
/* another task.
/*
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

#endif

