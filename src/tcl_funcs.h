#ifndef __TCL_FUNCS_H__
#define __TCL_FUNCS_H__

/*
 Copyright (c) 2006-2010 Trevor Williams

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
  const char* browser,
  const char* input_cdd
);

#endif

#endif

