#ifndef __STRUCT_UNION_H__
#define __STRUCT_UNION_H__

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
 \file     struct_union.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     9/13/2007
 \brief    Contains functions for handling SystemVerilog structs/unions.
*/

#include "defines.h"


/*! \brief Allocates, initializes and adds a new void member to the given struct/union */
su_member* struct_union_add_member_void( struct_union* su );

/*! \brief Allocates, initializes and adds a new signal member to the given struct/union */
su_member* struct_union_add_member_sig( struct_union* su, vsignal* sig );

/*! \brief Allocates, initializes and adds a new typedef member to the given struct/union */
su_member* struct_union_add_member_typedef( struct_union* su, typedef_item* tdi );

/*! \brief Allocates, initializes and adds a new enum member to the given struct/union */
su_member* struct_union_add_member_enum( struct_union* su, enum_item* ei );

/*! \brief Allocates, initializes and adds a new struct/union member to the given struct/union */
su_member* struct_union_add_member_struct_union( struct_union* su, struct_union* sui );

/*! \brief Allocates, initializes and adds a new enumerated item to the given functional unit */
struct_union* struct_union_create( const char* name, /*@unused@*/ sig_range* range, int type, bool packed, bool is_signed, func_unit* funit );

/*! \brief Deallocates given struct/union */
void struct_union_dealloc( struct_union* su );

/*! \brief Deallocates struct/union list from given functional unit */
void struct_union_dealloc_list( func_unit* funit );

#endif

