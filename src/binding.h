#ifndef __BINDING_H__
#define __BINDING_H__

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
 \file     binding.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/4/2002
 \brief    Contains all functions for vsignal/expression binding.
*/

#include "defines.h"


/*! \brief Adds vsignal and expression to binding list. */
void bind_add(
  int         type,
  const char* name,
  expression* exp,
  func_unit*  funit,
  bool        staticf
);

/*! \brief Appends an FSM expression to a matching expression binding structure */
void bind_append_fsm_expr(
  expression*       fsm_exp,
  const expression* exp,
  const func_unit*  curr_funit
);

/*! \brief Removes the expression with ID of id from binding list. */
void bind_remove(
  int  id,
  bool clear_assigned
);

/*! \brief Searches current binding list for the signal name associated with the given expression */
char* bind_find_sig_name(
  const expression* exp
);

/*! \brief Removes the statement block associated with the expression with ID of id after binding has occurred */
void bind_rm_stmt(
  int id
);

/*! \brief Binds a signal to an expression */
bool bind_signal(
  char*       name,
  expression* exp,
  func_unit*  funit_exp,
  bool        fsm_bind,
  bool        cdd_reading,
  bool        clear_assigned,
  int         exp_line,
  bool        bind_locally
);

/*! \brief Performs vsignal/expression bind (performed after parse completed). */
void bind_perform(
  bool cdd_reading,
  int  pass
);

/*! \brief Deallocates memory used for binding */
void bind_dealloc();

#endif

