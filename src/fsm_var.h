#ifndef __FSM_VAR_H__
#define __FSM_VAR_H__

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
void fsm_var_bind_add(
  char*       sig_name,
  expression* expr,
  char*       funit_name
);

/*! \brief Add specified functional unit and statement to binding list. */
void fsm_var_stmt_add(
  statement* stmt,
  char*      funit_name
);

/*! \brief Performs FSM signal/expression binding process. */
void fsm_var_bind();

/*! \brief Cleans up the global lists used in this file. */
void fsm_var_cleanup();

#endif

