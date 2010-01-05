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
 \file     enumerate.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/29/2006
*/

#include <stdlib.h>

#include "defines.h"
#include "enumerate.h"
#include "obfuscate.h"
#include "param.h"
#include "static.h"
#include "util.h"
#include "vector.h"


extern char user_msg[USER_MSG_LENGTH];


/*!
 Allocates, initializes and adds a new enumerated item to the given functional unit.
*/
void enumerate_add_item(
  vsignal*     enum_sig,  /*!< Pointer to signal in database that represents an enumeration value */
  static_expr* value,     /*!< Pointer to static expression that contains the value to be assigned to the signal during elaboration */
  func_unit*   funit      /*!< Pointer to functional unit that this enumeration value will be added to */
) { PROFILE(ENUMERATE_ADD_ITEM);

  enum_item* ei;  /* Pointer to newly allocated enumeration item */

  /* Allocate and initialize the enumeration item */
  ei = (enum_item*)malloc_safe( sizeof( enum_item ) );
  ei->sig   = enum_sig;
  ei->value = value;
  ei->last  = FALSE;
  ei->next  = NULL;

  /* Add it to the current functional unit's enumeration list */
  if( funit->ei_head == NULL ) {
    funit->ei_head = funit->ei_tail = ei;
  } else {
    funit->ei_tail->next = ei;
    funit->ei_tail       = ei;
  }

  PROFILE_END;

}

/*!
 Called after all enumerations have been parsed for this list.
*/
void enumerate_end_list(
  func_unit* funit  /*!< Pointer to functional unit to close out enumerated list */
) { PROFILE(ENUMERATE_END_LIST);

  /* Make sure that we aren't calling this function when there is no existing enumerated list */
  assert( funit->ei_tail != NULL );

  /* Set the last bit of the tail enumerated item */
  funit->ei_tail->last = TRUE;

  PROFILE_END;

}

/*!
 \throws anonymous Throw param_expr_eval

 Resolves all enumerated values for their value for the given instance.  This needs
 to be called during elaboration after all signals have been sized and parameters have
 been resolved.
*/
void enumerate_resolve(
  funit_inst* inst  /*!< Pointer to functional unit instance to resolve all enumerated values */
) { PROFILE(ENUMERATE_RESOLVE);

  enum_item* ei;                 /* Pointer to current enumeration item in the given functional unit */
  int        last_value = 0;     /* Value of last value for this enumeration */
  bool       first      = TRUE;  /* Specifies if the current enumeration is the first */
  bool       is_signed;          /* Contains original value of signedness of signal */

  assert( inst != NULL );

  ei = inst->funit->ei_head;
  while( ei != NULL ) {

    assert( ei->sig->value != NULL );

    /* Store signedness */
    is_signed = ei->sig->value->suppl.part.is_signed;

    /* If no value was assigned, we need to assign one now */
    if( ei->value == NULL ) {

      if( first ) {
        (void)vector_from_int( ei->sig->value, 0 );
      } else if( last_value == -1 ) {
        unsigned int rv;
        print_output( "Implicit enumerate assignment cannot follow an X or Z value", FATAL, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "File: %s, Line: %d", obf_file( inst->funit->orig_fname ), ei->sig->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );
        Throw 0;
      } else {
        (void)vector_from_int( ei->sig->value, (last_value + 1) );
      }

    /* Otherwise, reduce the static_expr value to a number and assign it */
    } else {

      if( ei->value->exp == NULL ) {
        (void)vector_from_int( ei->sig->value, ei->value->num );
      } else {
        param_expr_eval( ei->value->exp, inst );
        (void)vector_set_value_ulong( ei->sig->value, ei->value->exp->value->value.ul, ei->sig->value->width );
      }

    }

    /* Put back the original signedness */
    ei->sig->value->suppl.part.is_signed = is_signed;
        
    /* Set the first value to indicate if the next value is the first in the list */
    first = ei->last;

    /* Set last_value to that of this this signal value */
    if( !vector_is_unknown( ei->sig->value ) ) {
      last_value = vector_to_int( ei->sig->value );
    } else {
      last_value = -1;
    }

    ei = ei->next;

  }

  PROFILE_END;

}

/*!
 Deallocates all memory associated with the given enumeration.
*/
void enumerate_dealloc(
  enum_item* ei  /*!< Pointer to enumeration to deallocate */
) { PROFILE(ENUMERATE_DEALLOC);

  if( ei != NULL ) {

    /* Deallocate static expression, if necessary */
    if( ei->value != NULL ) {
      static_expr_dealloc( ei->value, TRUE );
    }

    /* Deallocate ourself */
    free_safe( ei, sizeof( enum_item ) );

  }

  PROFILE_END;

}

/*!
 Deallocates all memory associated with the enumeration list in the given functional unit
*/
void enumerate_dealloc_list(
  func_unit* funit  /*!< Pointer to functional unit to remove enumeration list for */
) { PROFILE(ENUMERATE_DEALLOC_LIST);

  enum_item* tmp;  /* Temporary pointer to current link in list */

  while( funit->ei_head != NULL ) {
    tmp  = funit->ei_head;
    funit->ei_head = tmp->next;
    enumerate_dealloc( tmp );
  }

  /* Set the tail pointer to NULL as well */
  funit->ei_tail = NULL;

  PROFILE_END;

}

