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
 \file     reentrant.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/11/2006
*/

#include <assert.h>

#include "defines.h"
#include "reentrant.h"
#include "util.h"


/*!
 \param funit  Pointer to functional unit to create a new reentrant structure for.

 \return Returns a pointer to the newly created reentrant structure.

 Allocates and initializes the reentrant structure for the given functional unit.
*/
reentrant* reentrant_create( func_unit* funit ) {

  reentrant* ren;       /* Pointer to newly created reentrant structure */
  sig_link*  sigl;      /* Pointer to current signal in the given functional unit */
  int        bits = 0;  /* Number of bits needed to store signal values */

  /* Allocate the structure */
  ren = (reentrant*)malloc_safe( sizeof( reentrant ), __FILE__, __LINE__ );

  /* Initialize the structure */
  ren->stack     = NULL;
  ren->funit     = funit;

  /* Get size needed to store data */
  sigl = funit->sig_head;
  while( sigl != NULL ) {
    printf( "In reentrant_create, signal: %s, width: %d\n", sigl->sig->name, sigl->sig->value->width );
    bits += sigl->sig->value->width;
    sigl = sigl->next;
  }

  /* Calculate data size */
  ren->data_size = ((bits % 4) == 0) ? (bits / 4) : ((bits / 4) + 1);
  printf( "  data_size: %d\n", ren->data_size );

  return( ren );

}

/*!
 \param ren  Pointer to reentrant structure containing stack to push.

 Iterates through the functional unit's signal list, compressing and storing its values into a newly
 allocated stack entry.  Adds the new stack entry to the stack contained in the given reentrant structure.
*/
void reentrant_stack_push( reentrant* ren ) {

  sig_link*     sigl;     /* Pointer to current signal link in list */
  rstack_entry* rse;      /* Pointer to newly allocated reentrant stack entry */
  int           bit = 0;  /* Index into the packed data structure */
  int           i;        /* Loop iterator */

  assert( ren != NULL );

  printf( "In reentrant_stack_push...\n" );

  if( ren->data_size > 0 ) {

    /* Allocate memory for stack entry and its data */
    rse       = (rstack_entry*)malloc_safe( sizeof( rstack_entry ), __FILE__, __LINE__ );
    printf( "*****  Allocating data for rse: %p\n", rse );
    rse->data = (nibble*)malloc_safe( (sizeof( nibble ) * ren->data_size), __FILE__, __LINE__ );

    /* Walk through the signal list in the reentrant functional unit, compressing and saving vector values */
    sigl = ren->funit->sig_head;
    while( sigl != NULL ) {
      for( i=0; i<sigl->sig->value->width; i++ ) {
        rse->data[((bit%4)==0)?(bit/4):((bit/4)+1)] |= (sigl->sig->value->value[i].part.val.value << (bit % 4));
        bit++;
      }
      sigl = sigl->next;
    }

    /* Move stack pointer to point to this newly created stack */
    rse->prev  = ren->stack;
    ren->stack = rse;

  }

}

/*!
 \param ren  Pointer to reentrant structure containing stack to pop.

 Iterates through each stored bit in the given reentrant stack top, assigning it back to its original signal
 value.  Calls the vsignal_propagate function for each signal to indicate that the value has changed from its
 previous value and deallocates/pops the top of the reentrant stack.
*/
void reentrant_stack_pop( reentrant* ren ) {

  sig_link*     sigl;     /* Pointer to current signal link in list */
  rstack_entry* rse;      /* Tempoary pointer to reentrant stack entry */
  int           bit = 0;  /* Index into the packed data structure */
  int           i;        /* Loop iterator */

  assert( ren != NULL );

  if( ren->data_size > 0 ) {

    printf( "Attempting to pop data from rse: %p\n", ren->stack );

    /* Walk through each bit in the compressed data array and assign it back to its signal */
    sigl = ren->funit->sig_head;
    while( sigl != NULL ) {
      for( i=0; i<sigl->sig->value->width; i++ ) {
        sigl->sig->value->value[i].part.val.value = (ren->stack->data[((bit%4)==0)?(bit/4):((bit/4)+1)] >> (bit % 4));
        bit++;
      }
      vsignal_propagate( sigl->sig, 0 );
      sigl = sigl->next;
    }

    /* Now pop the entry */
    rse        = ren->stack;
    ren->stack = ren->stack->prev;
    
    /* Deallocate the last entry */
    printf( "***** Deallocating data for rse: %p\n", rse );
    free_safe( rse->data );
    free_safe( rse );

  }

}

/*!
 \param ren  Pointer to the reentrant structure to deallocate from memory.

 Deallocates all memory associated with the given reentrant structure.
*/
void reentrant_dealloc( reentrant* ren ) {

  if( ren != NULL ) {

    /* If the stack still exists, pop it (it should be the final entry) */
    if( ren->stack != NULL ) {
      reentrant_stack_pop( ren );
    }

    /* Verify that the stack has been deallocated */
    assert( ren->stack == NULL );

    /* Deallocate memory allocated for this reentrant structure */
    free_safe( ren );

  }

}

/*
 $Log$
 Revision 1.1  2006/12/11 23:29:16  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.
*/

