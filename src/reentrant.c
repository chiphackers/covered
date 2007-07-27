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
 \param funit  Pointer to current function to count the bits of

 \return Returns the total number of bits in all signals in this functional unit and all parents
         within the same reentrant task/function.

 Recursively iterates up the functional unit tree keeping track of the total number of bits needed
 to store all information in the current reentrant task/function.
*/
int reentrant_count_afu_bits( func_unit* funit ) {

  sig_link* sigl;      /* Pointer to current signal link */
  int       bits = 0;  /* Number of bits in this functional unit and all parent functional units in the reentrant task/function */

  if( (funit->type == FUNIT_ATASK) || (funit->type == FUNIT_AFUNCTION) || (funit->type == FUNIT_ANAMED_BLOCK) ) {

    /* Count the number of bits in this functional unit */
    sigl = funit->sig_head;
    while( sigl != NULL ) {
      bits += sigl->sig->value->width;
      sigl = sigl->next;
    }

    /* If the current functional unit is a named block, gather the bits in the parent functional unit */
    if( funit->type == FUNIT_ANAMED_BLOCK ) {
      bits += reentrant_count_afu_bits( funit->parent );
    }

  }

  return( bits );

}

/*!
 \param funit     Pointer to current functional unit to traverse
 \param ren       Pointer to reentrant structure to populate
 \param curr_bit  Current bit to store (should be started at a value of 0)

 Recursively gathers all signal data bits to store and stores them in the given reentrant
 structure.
*/
void reentrant_store_data_bits( func_unit* funit, reentrant* ren, int curr_bit ) {

  sig_link* sigl;  /* Pointer to current signal link in current functional unit */
  int       i;     /* Loop iterator */

  if( (funit->type == FUNIT_ATASK) || (funit->type == FUNIT_AFUNCTION) || (funit->type == FUNIT_ANAMED_BLOCK) ) {

    /* Walk through the signal list in the reentrant functional unit, compressing and saving vector values */
    sigl = funit->sig_head;
    while( sigl != NULL ) {
      for( i=0; i<sigl->sig->value->width; i++ ) {
        ren->data[((curr_bit%4)==0)?(curr_bit/4):((curr_bit/4)+1)] |= (sigl->sig->value->value[i].part.val.value << (curr_bit % 4));
        curr_bit++;
      }
      sigl = sigl->next;
    }

    /* If the current functional unit is a named block, store the bits in the parent functional unit */
    if( funit->type == FUNIT_ANAMED_BLOCK ) {
      reentrant_store_data_bits( funit->parent, ren, curr_bit );
    }

  }

}

/*!
 \param funit     Pointer to current functional unit to restore
 \param ren       Pointer to reentrant structure containing bits to restore
 \param curr_bit  Current bit in reentrant structure to restore
 \param sim_time  Current simulation time

 Recursively restores the signal values of the functional units in a reentrant task/function.
*/
void reentrant_restore_data_bits( func_unit* funit, reentrant* ren, int curr_bit, uint64 sim_time ) {

  sig_link* sigl;  /* Pointer to current signal link */
  int       i;     /* Loop iterator */

  if( (funit->type == FUNIT_ATASK) || (funit->type == FUNIT_AFUNCTION) || (funit->type == FUNIT_ANAMED_BLOCK) ) {

    /* Walk through each bit in the compressed data array and assign it back to its signal */
    sigl = funit->sig_head;
    while( sigl != NULL ) {
      for( i=0; i<sigl->sig->value->width; i++ ) {
        sigl->sig->value->value[i].part.val.value = (ren->data[((curr_bit%4)==0)?(curr_bit/4):((curr_bit/4)+1)] >> (curr_bit % 4));
        curr_bit++;
      }
      vsignal_propagate( sigl->sig, sim_time );
      sigl = sigl->next;
    }

    /*
     If the current functional unit is a named block, restore the rest of the bits for the parent functional units
     in this reentrant task/function.
    */
    if( funit->type == FUNIT_ANAMED_BLOCK ) {
      reentrant_restore_data_bits( funit->parent, ren, curr_bit, sim_time );
    }

  }

}

/*!
 \param funit  Pointer to functional unit to create a new reentrant structure for.

 \return Returns a pointer to the newly created reentrant structure.

 Allocates and initializes the reentrant structure for the given functional unit,
 compressing and packing the bits into the given data structure.
*/
reentrant* reentrant_create( func_unit* funit ) {

  reentrant* ren  = NULL;  /* Pointer to newly created reentrant structure */
  int        data_size;    /* Number of nibbles needed to store the given functional unit */
  int        bits = 0;     /* Number of bits needed to store signal values */

  /* Get size needed to store data */
  bits = reentrant_count_afu_bits( funit );

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
    reentrant_store_data_bits( funit, ren, 0 );

  }

  return( ren );

}

/*!
 \param ren       Pointer to the reentrant structure to deallocate from memory.
 \param funit     Pointer to functional unit associated with this reentrant structure.
 \param sim_time  Current timestep being simulated.

 Pops data back into the given functional unit and deallocates all memory associated
 with the given reentrant structure.
*/
void reentrant_dealloc( reentrant* ren, func_unit* funit, uint64 sim_time ) {

  sig_link* sigl;     /* Pointer to current signal link in list */
  int       i;        /* Loop iterator */
  int       bit = 0;  /* Current bit in compressed bit array being assigned */

  if( ren != NULL ) {

    /* If we have data being stored, pop it */
    if( ren->data_size > 0 ) {

      /* Walk through each bit in the compressed data array and assign it back to its signal */
      reentrant_restore_data_bits( funit, ren, 0, sim_time );

      /* Deallocate the data nibble array */
      free_safe( ren->data );

    }

    /* Deallocate memory allocated for this reentrant structure */
    free_safe( ren );

  }

}

/*
 $Log$
 Revision 1.5  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

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

