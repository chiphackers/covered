#ifndef __PARAM_H__
#define __PARAM_H__

/*!
 \file     param.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002
 \brief    Contains functions and structures necessary to handle parameters.
*/

#include "defines.h"
#include "expr.h"
#include "module.h"
#include "vector.h"


//! Searches specified parameter list for the specified parameter name.
parameter* param_find( char* name, parameter* parm );

//! Adds parameter to specified module's parameter list.
void param_add( char* name, expression* expr, module* mod );

//! Adds parameter override to defparam list.
void param_add_defparam( char* scope, vector* expr );


/* $Log$
/* Revision 1.1  2002/08/23 12:55:33  phase1geo
/* Starting to make modifications for parameter support.  Added parameter source
/* and header files, changed vector_from_string function to be more verbose
/* and updated Makefiles for new param.h/.c files.
/* */

#endif

