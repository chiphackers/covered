#ifndef __TCL_FUNCS_H__
#define __TCL_FUNCS_H__

/*!
 \file     tcl_funcs.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/31/2004
 \brief    Contains functions to interact with TCL scripts.
*/

#include <tcl.h>
#include <tk.h>

int tcl_func_get_module_list( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] );

void tcl_func_initialize( Tcl_Interp* tcl, char* home, char* version, char* browser );

#endif

