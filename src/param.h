#ifndef __PARAM_H__
#define __PARAM_H__

/*!
 \file     param.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002
 \brief    Contains functions and structures necessary to handle parameters.
*/

#include "defines.h"


//! Searches specified module parameter list for matching parameter.
mod_parm* mod_parm_find( char* name, mod_parm* parm );

//! Creates new module parameter and adds it to the specified list.
mod_parm* mod_parm_add( char* scope, expression* expr, int type, mod_parm** head, mod_parm** tail );

//! Adds parameter override to defparam list.
void defparam_add( char* scope, vector* expr );

//! Transforms a declared module parameter into an instance parameter.
void param_resolve_declared( char* mscope, mod_parm* mparm, inst_parm* ip_head, inst_parm** ihead, inst_parm** itail );

//! Transforms an override module parameter into an instance parameter.
void param_resolve_override( mod_parm* oparm, inst_parm** ihead, inst_parm** itail );


/* $Log$
/* Revision 1.4  2002/09/21 04:11:32  phase1geo
/* Completed phase 1 for adding in parameter support.  Main code is written
/* that will create an instance parameter from a given module parameter in
/* its entirety.  The next step will be to complete the module parameter
/* creation code all the way to the parser.  Regression still passes and
/* everything compiles at this point.
/*
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

