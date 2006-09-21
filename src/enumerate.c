/*!
 \file     enumerate.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/29/2006
*/

#include "defines.h"
#include "enumerate.h"
#include "util.h"
#include "vector.h"
#include "param.h"
#include "static.h"


/*!
 \param enum_sig  Pointer to signal in database that represents an enumeration value
 \param value     Pointer to static expression that contains the value to be assigned to the signal during elaboration
 \param funit     Pointer to functional unit that this enumeration value will be added to

 Allocates, initializes and adds a new enumerated item to the given functional unit.
*/
void enumerate_add_item( vsignal* enum_sig, static_expr* value, func_unit* funit ) {

  enum_item* ei;  /* Pointer to newly allocated enumeration item */

  /* Allocate and initialize the enumeration item */
  ei = (enum_item*)malloc_safe( sizeof( enum_item ), __FILE__, __LINE__ );
  ei->sig   = enum_sig;
  ei->value = value;
  ei->last  = FALSE;
  ei->next  = NULL;

  /* Set the root bit on the static expression, if necessary */
  if( (value != NULL) && (value->exp != NULL) ) {
    value->exp->suppl.part.root = 1;
  }

  /* Add it to the current functional unit's enumeration list */
  if( funit->ei_head == NULL ) {
    funit->ei_head = funit->ei_tail = ei;
  } else {
    funit->ei_tail->next = ei;
    funit->ei_tail       = ei;
  }

}

/*!
 \param funit  Pointer to functional unit to close out enumerated list.

 Called after all enumerations have been parsed for this list.
*/
void enumerate_end_list( func_unit* funit ) {

  /* Make sure that we aren't calling this function when there is no existing enumerated list */
  assert( funit->ei_tail != NULL );

  /* Set the last bit of the tail enumerated item */
  funit->ei_tail->last = TRUE;

}

/*!
 \param inst  Pointer to functional unit instance to resolve all enumerated values

 Resolves all enumerated values for their value for the given instance.  This needs
 to be called during elaboration after all signals have been sized and parameters have
 been resolved.
*/
void enumerate_resolve( funit_inst* inst ) {

  enum_item* ei;            /* Pointer to current enumeration item in the given functional unit */
  int        last_value;    /* Value of last value for this enumeration */
  bool       first = TRUE;  /* Specifies if the current enumeration is the first */
  bool       is_signed;     /* Contains original value of signedness of signal */

  assert( inst != NULL );

  ei = inst->funit->ei_head;
  while( ei != NULL ) {

    assert( ei->sig->value != NULL );

    /* Store signedness */
    is_signed = ei->sig->value->suppl.part.is_signed;

    /* If no value was assigned, we need to assign one now */
    if( ei->value == NULL ) {

      if( first ) {
        vector_from_int( ei->sig->value, 0 );
      } else {
        vector_from_int( ei->sig->value, (last_value + 1) );
      }

    /* Otherwise, reduce the static_expr value to a number and assign it */
    } else {

      if( ei->value->exp == NULL ) {
        vector_from_int( ei->sig->value, ei->value->num );
      } else {
        param_expr_eval( ei->value->exp, inst );
        vector_from_int( ei->sig->value, vector_to_int( ei->value->exp->value ) );
      }

    }

    /* Put back the original signedness */
    ei->sig->value->suppl.part.is_signed = is_signed;
        
    /* Set the first value to indicate if the next value is the first in the list */
    first = ei->last;

    /* Set last_value to that of this this signal value */
    last_value = vector_to_int( ei->sig->value );

    ei = ei->next;

  }

}

/*!
 \param funit  Pointer to functional unit to remove enumeration list for

 Deallocates all memory associated with the enumeration list in the given functional unit
*/
void enumerate_dealloc_list( func_unit* funit ) {

  enum_item* tmp;  /* Temporary pointer to current link in list */

  while( funit->ei_head != NULL ) {

    tmp  = funit->ei_head;
    funit->ei_head = tmp->next;

    /* Deallocate static expression, if necessary */
    if( tmp->value != NULL ) {
      static_expr_dealloc( tmp->value, TRUE );
    }

    /* Deallocate ourself */
    free( tmp );

  }

  /* Set the tail pointer to NULL as well */
  funit->ei_tail = NULL;

}


/*
 $Log$
 Revision 1.2  2006/08/30 12:02:48  phase1geo
 Changing assertion in vcd.c that fails when the VCD file is improperly formatted
 to a user error message with a bit more meaning.  Fixing problem with signedness
 of enumeration resolution.  Added enum1.1 diagnostic to testsuite.

 Revision 1.1  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

*/

