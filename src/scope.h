#ifndef __SCOPE_H__
#define __SCOPE_H__

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
 \file     scope.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/10/2005
 \brief    Contains functions for looking up structures for the given scope
*/


#include "defines.h"


/*! \brief  Find the given signal in the provided scope */
bool scope_find_param(
  const char* name,
  func_unit*  curr_funit,
  mod_parm**  found_parm,
  func_unit** found_funit,
  int         line
);

/*! \brief  Find the given signal in the provided scope */
bool scope_find_signal(
  const char* name,
  func_unit*  curr_funit,
  vsignal**   found_sig,
  func_unit** found_funit,
  int         line
);

/*! \brief  Finds the given task or function in the provided scope. */
bool scope_find_task_function_namedblock(
  const char* name,
  int         type,
  func_unit*  curr_funit,
  func_unit** found_funit,
  int         line,
  bool        must_find,
  bool        rm_unnamed
);

/*! \brief  Finds the parent functional unit of the functional unit with the given scope */
func_unit* scope_get_parent_funit(
  funit_inst* root,
  const char* scope
);

/*! \brief  Finds the parent module of the functional unit with the given scope */
func_unit* scope_get_parent_module(
  funit_inst* root,
  const char* scope
);

#endif

