#ifndef __TCL_FUNCS_H__
#define __TCL_FUNCS_H__

/*!
 \file     tcl_funcs.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/31/2004
 \brief    Contains functions to interact with TCL scripts.
*/

#ifdef HAVE_TCLTK
#include <tcl.h>
#include <tk.h>

/*! \brief TBD */
int tcl_func_get_funit_list( ClientData d, Tcl_Interp* tcl, int argc, const char *argv[] );

/*! \brief TBD */
void tcl_func_initialize( Tcl_Interp* tcl, char* home, char* version, char* browser );
#endif

/* $Log$
/* Revision 1.6  2005/11/08 23:12:10  phase1geo
/* Fixes for function/task additions.  Still a lot of testing on these structures;
/* however, regressions now pass again so we are checkpointing here.
/*
*/

#endif

