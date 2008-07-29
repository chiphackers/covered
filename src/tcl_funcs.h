#ifndef __TCL_FUNCS_H__
#define __TCL_FUNCS_H__

/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     tcl_funcs.h
 \author   Trevor Williams  (phase1geo@gmail.com)
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
void tcl_func_initialize(
  Tcl_Interp* tcl,
  const char* program,
  const char* user_home,
  const char* home,
  const char* version,
  const char* browser
);

#endif

/*
 $Log$
 Revision 1.13.6.1  2008/07/19 00:25:52  phase1geo
 Forgot to update some files per the last checkin.

 Revision 1.13  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.12  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.11  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.10  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.9  2006/01/28 06:42:53  phase1geo
 Added configuration read/write functionality for tool preferences and integrated
 the preferences.tcl file into Covered's GUI.  This is now functioning correctly.

 Revision 1.8  2005/12/02 05:46:50  phase1geo
 Fixing compile errors when HAVE_TCLTK is defined in config.h.

 Revision 1.7  2005/12/01 19:46:50  phase1geo
 Removed Tcl/Tk from source files if HAVE_TCLTK is not defined.

 Revision 1.6  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.
*/

#endif

