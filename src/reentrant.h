#ifndef __REENTRANT_H__
#define __REENTRANT_H__

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
 \file     reentrant.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/11/2006
 \brief    Contains functions for handling re-entrant tasks and functions.
*/

#include "defines.h"


/*! \brief Allocates and initializes the reentrant structure for the given functional unit */
reentrant* reentrant_create( func_unit* funit );

/*! \brief Deallocates all memory associated with the given reentrant structure */
void reentrant_dealloc( reentrant* ren, func_unit* funit, expression* expr );

#endif

