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
 \file     struct_union.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/13/2007
*/

#include "defines.h"
#include "struct_union.h"
#include "util.h"


int struct_union_length( struct_union* su ) {

}

/*!
 \param su  Pointer to struct/union to add member to

 \return Returns a pointer to the newly allocated struct/union member.

 Allocates, initializes and adds a 'void' type struct/union member to the given struct/union.
*/
su_member* struct_union_add_member_void( struct_union* su ) {

  su_member* mem;  /* Pointer to newly created struct/union member */

  /* Allocate memory for the new member */
  mem = (su_member*)malloc_safe( sizeof( su_member ), __FILE__, __LINE__ );

  /* Initialize the contents of the member */
  mem->type = SU_TYPE_VOID;
  mem->pos  = struct_union_length( su );

  return( mem );

}

su_member* struct_union_add_member_sig( struct_union* su, vsignal* sig ) {

}

su_member* struct_union_add_member_typedef( struct_union* su, typedef_item* tdi ) {

}

su_member* struct_union_add_member_enum( struct_union* su, enum_item* ei ) {

}

su_member* struct_union_add_member_struct_union( struct_union* su, struct_union* sui ) {

}

struct_union* struct_union_create( const char* name, sig_range* range, int type, bool packed, bool is_signed, func_unit* funit ) {

}

void struct_union_dealloc_list( func_unit* funit ) {

}


/*
 $Log$
*/

