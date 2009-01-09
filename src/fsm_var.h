#ifndef __FSM_VAR_H__
#define __FSM_VAR_H__

/*
 Copyright (c) 2006-2009 Trevor Williams

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
 \file     fsm_var.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/3/2003
 \brief    Contains functions for handling FSM variable structure.
*/

#include "defines.h"


/*! \brief Allocates, initializes and adds FSM variable to global list. */
fsm_var* fsm_var_add(
  const char* funit_name,
  expression* in_state,
  expression* out_state,
  char*       name,
  bool        exclude
);

/*! \brief Adds specified signal and expression to binding list. */
void fsm_var_bind_add( char* sig_name, expression* expr, char* funit_name );

/*! \brief Add specified functional unit and statement to binding list. */
void fsm_var_stmt_add( statement* stmt, char* funit_name );

/*! \brief Performs FSM signal/expression binding process. */
void fsm_var_bind();

/*! \brief Cleans up the global lists used in this file. */
void fsm_var_cleanup();


/*
 $Log$
 Revision 1.11  2008/04/01 23:08:21  phase1geo
 More updates for error diagnostic cleanup.  Full regression still not
 passing (but is getting close).

 Revision 1.10  2008/02/01 06:37:08  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.9  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.8  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.7  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.6  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.5  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.4  2003/11/11 21:48:09  phase1geo
 Fixing bug where next pointers in bind lists were not being initialized to
 NULL (manifested itself in Irix).  Also added missing development documentation
 to functions in fsm_var.c and removed unnecessary function.

 Revision 1.3  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.2  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.1  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

*/

#endif

