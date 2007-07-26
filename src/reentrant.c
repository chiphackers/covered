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
 \param stmt   Pointer to current statement that is causing the context swith.

 \return Returns a pointer to the newly created reentrant structure.

 Allocates and initializes the reentrant structure for the given functional unit,
 compressing and packing the bits into the given data structure.
*/
reentrant* reentrant_create( func_unit* funit, statement* stmt ) {

  reentrant* ren  = NULL;  /* Pointer to newly created reentrant structure */
  int        data_size;    /* Number of nibbles needed to store the given functional unit */
  sig_link*  sigl;         /* Pointer to current signal in the given functional unit */
  int        bits = 0;     /* Number of bits needed to store signal values */
  int        bit  = 0;     /* Current bit position of compressed data */
  int        i;            /* Loop iterator */

  /* Get size needed to store data */
  sigl = funit->sig_head;
  while( sigl != NULL ) {
    bits += sigl->sig->value->width;
    sigl = sigl->next;
  }

  /* Calculate data size */
  data_size = ((bits % 4) == 0) ? (bits / 4) : ((bits / 4) + 1);

  /* If there is data to store, allocate the needed memory and populate it */
  if( data_size > 0 ) {

    /* Allocate the structure */
    ren = (reentrant*)malloc_safe( sizeof( reentrant ), __FILE__, __LINE__ );

    /* Set the data size */
    ren->data_size = data_size;

    /* Allocate memory for data */
    ren->data = (nibble*)malloc_safe( (sizeof( nibble ) * ren->data_size), __FILE__, __LINE__ );

    /* Walk through the signal list in the reentrant functional unit, compressing and saving vector values */
    sigl = funit->sig_head;
    while( sigl != NULL ) {
      for( i=0; i<sigl->sig->value->width; i++ ) {
        ren->data[((bit%4)==0)?(bit/4):((bit/4)+1)] |= (sigl->sig->value->value[i].part.val.value << (bit % 4));
        bit++;
      }
      sigl = sigl->next;
    }

  }

  return( ren );

}

/*!
 \param ren       Pointer to the reentrant structure to deallocate from memory.
 \param funit     Pointer to functional unit associated with this reentrant structure.
 \param stmt      Pointer to statement associated with this reentrant structure.
 \param sim_time  Current timestep being simulated.

 Pops data back into the given functional unit and deallocates all memory associated
 with the given reentrant structure.
*/
void reentrant_dealloc( reentrant* ren, func_unit* funit, statement* stmt, uint64 sim_time ) {

  sig_link* sigl;     /* Pointer to current signal link in list */
  int       i;        /* Loop iterator */
  int       bit = 0;  /* Current bit in compressed bit array being assigned */

  if( ren != NULL ) {

    /* If we have data being stored, pop it */
    if( ren->data_size > 0 ) {

      /* Walk through each bit in the compressed data array and assign it back to its signal */
      sigl = funit->sig_head;
      while( sigl != NULL ) {
        for( i=0; i<sigl->sig->value->width; i++ ) {
          sigl->sig->value->value[i].part.val.value = (ren->data[((bit%4)==0)?(bit/4):((bit/4)+1)] >> (bit % 4));
          bit++;
        }
        vsignal_propagate( sigl->sig, sim_time );
        sigl = sigl->next;
      }

      /* Deallocate the data nibble array */
      free_safe( ren->data );

    }

    /* Deallocate memory allocated for this reentrant structure */
    free_safe( ren );

  }

}

/*
 $Log$
 Revision 1.4  2007/04/20 22:56:46  phase1geo
 More regression updates and simulator core fixes.  Still a ways to go.

 Revision 1.3  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.2  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.1  2006/12/11 23:29:16  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.
*/

