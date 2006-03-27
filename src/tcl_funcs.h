#ifndef __TCL_FUNCS_H__
#define __TCL_FUNCS_H__

/*!
 \file     tcl_funcs.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/31/2004
 \brief    Contains functions to interact with TCL scripts.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_TCLTK
#include <tcl.h>
#include <tk.h>

/*! \brief Gathers the list of all functional units within the design */
int tcl_func_get_funit_list( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] );

/*! \brief Initializes TCL interface */
void tcl_func_initialize( Tcl_Interp* tcl, char* user_home, char* home, char* version, char* browser );
#endif

/* $Log$
/* Revision 1.9  2006/01/28 06:42:53  phase1geo
/* Added configuration read/write functionality for tool preferences and integrated
/* the preferences.tcl file into Covered's GUI.  This is now functioning correctly.
/*
/* Revision 1.8  2005/12/02 05:46:50  phase1geo
/* Fixing compile errors when HAVE_TCLTK is defined in config.h.
/*
/* Revision 1.7  2005/12/01 19:46:50  phase1geo
/* Removed Tcl/Tk from source files if HAVE_TCLTK is not defined.
/*
/* Revision 1.6  2005/11/08 23:12:10  phase1geo
/* Fixes for function/task additions.  Still a lot of testing on these structures;
/* however, regressions now pass again so we are checkpointing here.
/*
*/

#endif

