#ifndef __ENUMERATE_H__
#define __ENUMERATE_H__

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
 \file     enumerate.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/29/2006
 \brief    Contains functions for handling enumerations.
*/

#include "defines.h"


/*! \brief Allocates, initializes and adds a new enumerated item to the given functional unit */
void enumerate_add_item( vsignal* enum_sig, static_expr* value, func_unit* funit );

/*! \brief Sets the last status of the last item in the enumerated list */
void enumerate_end_list( func_unit* funit );

/*! \brief Resolves all enumerations within the given functional unit instance */
void enumerate_resolve( funit_inst* inst );

/*! \brief Deallocates all memory associated with the given enumeration. */
void enumerate_dealloc( enum_item* ei );

/*! \brief Deallocates enumeration list from given functional unit */
void enumerate_dealloc_list( func_unit* funit );

#endif

