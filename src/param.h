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

//! Outputs contents of module parameter list to standard output.
void mod_parm_display( mod_parm* mparm );

//! Searches specified instance parameter list for matching parameter.
inst_parm* inst_parm_find( char* name, inst_parm* parm );

//! Adds parameter override to defparam list.
void defparam_add( char* scope, vector* expr );

//! Transforms a declared module parameter into an instance parameter.
void param_resolve_declared( char* mscope, mod_parm* mparm, inst_parm* ip_head, inst_parm** ihead, inst_parm** itail );

//! Transforms an override module parameter into an instance parameter.
void param_resolve_override( mod_parm* oparm, inst_parm** ihead, inst_parm** itail );

//! Deallocates specified module parameter and possibly entire module parameter list.
void mod_parm_dealloc( mod_parm* parm, bool recursive );

//! Deallocates specified instance parameter and possibly entire instance parameter list.
void inst_parm_dealloc( inst_parm* parm, bool recursive );


/* $Log$
/* Revision 1.6  2002/09/23 01:37:45  phase1geo
/* Need to make some changes to the inst_parm structure and some associated
/* functionality for efficiency purposes.  This checkin contains most of the
/* changes to the parser (with the exception of signal sizing).
/*
/* Revision 1.5  2002/09/21 07:03:28  phase1geo
/* Attached all parameter functions into db.c.  Just need to finish getting
/* parser to correctly add override parameters.  Once this is complete, phase 3
/* can start and will include regenerating expressions and signals before
/* getting output to CDD file.
/*
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

