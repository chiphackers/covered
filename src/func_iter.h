#ifndef __FUNC_ITER_H__
#define __FUNC_ITER_H__

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
 \file     func_iter.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/2/2007
 \brief    Contains functions for dealing with functional unit iterators.
*/

#include "defines.h"


/*!
 Structure for iterating through a functional unit and its unnamed scopes.
*/
typedef struct func_iter_s {
  stmt_iter** sis;
  int         si_num;
} func_iter;


/*! \brief Resets the specified statement iterator at start point. */
void func_iter_init( func_iter* fi, func_unit* funit );

/*! \brief Copies the given statement iterator */
statement* func_iter_get_next_statement( func_iter* fi );

/*! \brief Sets current iterator to statement just prior to the given line number */
void func_iter_dealloc( func_iter* si );


/*
 $Log$
*/

#endif

