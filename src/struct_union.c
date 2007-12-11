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
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     9/13/2007
*/

#include "defines.h"
#include "enumerate.h"
#include "struct_union.h"
#include "util.h"
#include "vsignal.h"


/*!
 \param su  Pointer to struct/union to count members of

 \return Returns the number of members stored in the given struct/union.

 Counts the number of stored struct/union members in the given struct/union.
*/
int struct_union_length( struct_union* su ) { PROFILE(STRUCT_UNION_LENGTH);

  su_member* curr;       /* Pointer to current struct/union member */
  int        count = 0;  /* Number of members counted */

  curr = su->mem_head;
  while( curr != NULL ) {
    count++;
    curr = curr->next;
  }

  return( count );

}

/*!
 \param su   Pointer to struct/union to add member to
 \param mem  Pointer to struct/union member to add

 Adds the given struct/union member to the given struct/union member list.
*/
void struct_union_add_member( struct_union* su, su_member* mem ) { PROFILE(STRUCT_UNION_ADD_MEMBER);

  if( su->mem_head == NULL ) {
    su->mem_head = su->mem_tail = mem;
  } else {
    su->mem_tail->next = mem;
    su->mem_tail       = mem;
  }

}

/*!
 \param su  Pointer to struct/union to add member to

 \return Returns a pointer to the newly allocated struct/union member.

 Allocates, initializes and adds a 'void' type struct/union member to the given struct/union.
*/
su_member* struct_union_add_member_void( struct_union* su ) { PROFILE(STRUCT_UNION_ADD_MEMBER_VOID);

  su_member* mem;  /* Pointer to newly created struct/union member */

  /* Allocate memory for the new member */
  mem = (su_member*)malloc_safe( sizeof( su_member ) ); 

  /* Initialize the contents of the member */
  mem->type     = SU_MEMTYPE_VOID;
  mem->pos      = struct_union_length( su );
  mem->elem.sig = NULL;

  return( mem );

}

/*!
 \param su  Pointer to struct/union to add member to

 \return Returns a pointer to the newly allocated struct/union member.

 Allocates, initializes and adds a 'signal' type struct/union member to the given struct/union.
*/
su_member* struct_union_add_member_sig( struct_union* su, vsignal* sig ) { PROFILE(STRUCT_UNION_ADD_MEMBER_SIG);

  su_member* mem;  /* Pointer to newly created struct/union member */

  /* Allocate memory for the new member */
  mem = (su_member*)malloc_safe( sizeof( su_member ) );

  /* Initialize the contents of the member */
  mem->type     = SU_MEMTYPE_SIG;
  mem->pos      = struct_union_length( su );
  mem->elem.sig = sig;

  return( mem );

}

/*!
 \param su  Pointer to struct/union to add member to

 \return Returns a pointer to the newly allocated struct/union member.

 Allocates, initializes and adds a 'typedef' type struct/union member to the given struct/union.
*/
su_member* struct_union_add_member_typedef( struct_union* su, typedef_item* tdi ) { PROFILE(STRUCT_UNION_ADD_MEMBER_TYPEDEF);

  su_member* mem;  /* Pointer to newly created struct/union member */

  /* Allocate memory for the new member */
  mem = (su_member*)malloc_safe( sizeof( su_member ) );

  /* Initialize the contents of the member */
  mem->type     = SU_MEMTYPE_TYPEDEF;
  mem->pos      = struct_union_length( su );
  mem->elem.tdi = tdi;

  return( mem );

}

/*!
 \param su  Pointer to struct/union to add member to

 \return Returns a pointer to the newly allocated struct/union member.

 Allocates, initializes and adds an 'enum' type struct/union member to the given struct/union.
*/
su_member* struct_union_add_member_enum( struct_union* su, enum_item* ei ) { PROFILE(STRUCT_UNION_ADD_MEMBER_ENUM);

  su_member* mem;  /* Pointer to newly created struct/union member */

  /* Allocate memory for the new member */
  mem = (su_member*)malloc_safe( sizeof( su_member ) );

  /* Initialize the contents of the member */
  mem->type    = SU_MEMTYPE_ENUM;
  mem->pos     = struct_union_length( su );
  mem->elem.ei = ei;

  return( mem );

}

/*!
 \param su  Pointer to struct/union to add member to

 \return Returns a pointer to the newly allocated struct/union member.

 Allocates, initializes and adds a 'struct/union' type struct/union member to the given struct/union.
*/
su_member* struct_union_add_member_struct_union( struct_union* su, struct_union* sui ) { PROFILE(STRUCT_UNION_ADD_MEMBER_STRUCT_UNION);

  su_member* mem;  /* Pointer to newly created struct/union member */
  
  /* Allocate memory for the new member */
  mem = (su_member*)malloc_safe( sizeof( su_member ) );

  /* Initialize the contents of the member */
  mem->type    = SU_MEMTYPE_SU;
  mem->pos     = struct_union_length( su );
  mem->elem.su = sui;
  
  return( mem ); 

}

/*!
 \param name  Name of struct/union being added
 \param range  Pointer to multi-dimensional range associated with this struct/union
 \param type   Specifies struct, union or tagged union type of this structure
 \param packed  Specifies if this struct/union should be handled as a packed structure or not
 \param is_signed  Specifies if this struct/union should be handled as signed or not
 \param funit      Pointer to functional unit that contains this struct/union

 \return Returns a pointer to the newly allocated struct/union.

 Allocates, intializes and adds a new struct/union structure to the given functional unit's list of 
 struct/union members.
*/
struct_union* struct_union_create( const char* name, sig_range* range, int type, bool packed, bool is_signed, func_unit* funit ) { PROFILE(STRUCT_UNION_CREATE);

  struct_union* su;  /* Pointer to newly allocated struct/union structure */

  /* Allocate memory */
  su = (struct_union*)malloc_safe( sizeof( struct_union ) );

  /* Initialize */
  su->name      = strdup_safe( name );
  su->type      = type;
  su->packed    = packed;
  su->is_signed = is_signed;
  su->mem_head  = NULL;
  su->mem_tail  = NULL;
  su->next      = NULL;

  /* Add the new struct/union to the given functional unit */
  if( funit->su_head == NULL ) {
    funit->su_head = funit->su_tail = su;
  } else {
    funit->su_tail->next = su;
    funit->su_tail       = su;
  }

  return( su );

}

/*!
 \param mem  Pointer to struct/union member to deallocate

 Deallocates all memory associated with the given struct/union member.
*/
void struct_union_member_dealloc( su_member* mem ) { PROFILE(STRUCT_UNION_MEMBER_DEALLOC);

  if( mem != NULL ) {

    switch( mem->type ) {
      case SU_MEMTYPE_VOID    :  break;
      case SU_MEMTYPE_SIG     :  vsignal_dealloc( mem->elem.sig );      break;
      //case SU_MEMTYPE_TYPEDEF :  typedef_dealloc( mem->elem.tdi );      break;
      case SU_MEMTYPE_ENUM    :  enumerate_dealloc( mem->elem.ei );     break;
      case SU_MEMTYPE_SU      :  struct_union_dealloc( mem->elem.su );  break;
      default                 :  assert( 0 );                           break;
    }

    /* Deallocate the member itself */
    free_safe( mem );
  
  }

}

/*!
 \param su  Pointer to struct/union to deallocate

 Deallocates all memory associated with the given struct/union member.
*/
void struct_union_dealloc( struct_union* su ) { PROFILE(STRUCT_UNION_DEALLOC);

  su_member* curr_mem;  /* Pointer to current member entry to deallocate */
  su_member* tmp_mem;   /* Temporary pointer to current member to deallocate */

  if( su != NULL ) {

    /* Deallocate the member list */
    curr_mem = su->mem_head;
    while( curr_mem != NULL ) {
      tmp_mem = curr_mem;
      curr_mem = curr_mem->next;
      struct_union_member_dealloc( tmp_mem );
    }

    /* Deallocate the name */
    free_safe( su->name );

    /* Deallocate the struct/union itself */
    free_safe( su );

  }

}

/*!
 \param funit  Pointer to functional unit to remove struct/union list from.

 Deallocates the entire list of struct/union structures in the given functional unit.
*/
void struct_union_dealloc_list( func_unit* funit ) { PROFILE(STRUCT_UNION_DEALLOC_LIST);

  struct_union* curr_su;  /* Pointer to current struct/union to deallocate */
  struct_union* tmp_su;   /* Temporary pointer to current struct/union to deallocate */

  /* Iterate through each struct/union in the list */
  curr_su = funit->su_head;
  while( curr_su != NULL ) {
    tmp_su  = curr_su;
    curr_su = curr_su->next;
    struct_union_dealloc( tmp_su );
  }

  /* Clear the list for future use if necessary */
  funit->su_head = funit->su_tail = NULL;

}


/*
 $Log$
 Revision 1.3  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.2  2007/09/14 06:22:12  phase1geo
 Filling in existing functions in struct_union.  Completed parser code for handling
 struct/union declarations.  Code compiles thus far.

 Revision 1.1  2007/09/13 22:50:46  phase1geo
 Initial creation of struct_union files.  Added initial parsing ability for
 structs and unions (though I don't believe this is complete at this time).

*/

