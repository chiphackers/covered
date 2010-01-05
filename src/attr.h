#ifndef __COVERED_ATTR_H__
#define __COVERED_ATTR_H__

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
 \file     attr.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/25/2003
 \brief    Contains functions for handling attributes.
*/

#include "defines.h"


/*! \brief Creates new attribute parameter based on specified values. */
attr_param* attribute_create( const char* name, expression* expr );

/*! \brief Parses and handles specified attribute parameter list. */
void attribute_parse(
  attr_param*      ap,
  const func_unit* mod,
  bool             exclude
);

/*! \brief Deallocates entire attribute parameter list. */
void attribute_dealloc( attr_param* ap );

#endif

