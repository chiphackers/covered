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


//! Adds parameter override to defparam list.
void defparam_add( char* scope, vector* expr );


#ifdef DEPRECATED
//! Searches specified parameter list for the specified parameter name.
parameter* param_find( char* name, parameter* parm );

//! Adds parameter to specified module's parameter list.
void param_add( char* name, expression* expr, module* mod );

//! Generates all instance parameters for specified instance/module.
void param_generate( parameter* parm );

//! Undos the steps performed in the param_generate routine to restore module.
void param_destroy( parameter* parm );
#endif

/* $Log$
/* Revision 1.3  2002/09/19 05:25:19  phase1geo
/* Fixing incorrect simulation of static values and fixing reports generated
/* from these static expressions.  Also includes some modifications for parameters
/* though these changes are not useful at this point.
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

#endif

