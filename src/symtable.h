#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

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
 \file     symtable.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/3/2002
 \brief    Contains functions for manipulating a symtable structure.
*/

#include "defines.h"


/*! \brief Creates a new symtable structure. */
symtable* symtable_create();

/*! \brief Creates a new symtable entry and adds it to the specified symbol table. */
void symtable_add_signal(
  const char* sym,
  vsignal*    sig,
  int         msb,
  int         lsb
);

/*! \brief Adds the given expression to the symtable for the purposes as specified by type. */
void symtable_add_expression(
  const char* sym,
  expression* exp,
  char        type
);

/*! \brief Adds the given expression to the symtable for the purposes of memory coverage. */
void symtable_add_memory(
  const char* sym,
  expression* exp,
  char        action,
  int         msb
);

/*! \brief Adds the given FSM to the symtable */
void symtable_add_fsm(
  const char* sym,
  fsm*        table,
  int         msb,
  int         lsb
);

/*! \brief Sets all matching symtable entries to specified value */
void symtable_set_value(
  const char* sym,
  const char* value
);

/*! \brief Assigns stored values to all associated signals stored in specified symbol table. */
void symtable_assign(
  const sim_time* time
);

/*! \brief Deallocates all symtable entries for specified symbol table. */
void symtable_dealloc(
  symtable* symtab
);

#endif

