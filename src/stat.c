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
 \file     stat.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/4/2002
*/

#include "stat.h"
#include "defines.h"
#include "util.h"


/*!
 Allocates new memory for a coverage statistic structure and initializes
 its values.
*/
void statistic_create(
  statistic** stat  /*!< Pointer to newly create/initialized statistic structure */
) { PROFILE(STATISTIC_CREATE);

  if( *stat == NULL ) {
    *stat = (statistic*)malloc_safe( sizeof( statistic ) );
  }

  (*stat)->line_hit        = 0;
  (*stat)->line_excluded   = 0;
  (*stat)->line_total      = 0;
  (*stat)->tog01_hit       = 0;
  (*stat)->tog10_hit       = 0;
  (*stat)->tog_excluded    = 0;
  (*stat)->tog_total       = 0;
  (*stat)->tog_cov_found   = FALSE;
  (*stat)->comb_hit        = 0;
  (*stat)->comb_excluded   = 0;
  (*stat)->comb_total      = 0;
  (*stat)->state_total     = 0;
  (*stat)->state_hit       = 0;
  (*stat)->arc_total       = 0;
  (*stat)->arc_hit         = 0;
  (*stat)->arc_excluded    = 0;
  (*stat)->assert_hit      = 0;
  (*stat)->assert_excluded = 0;
  (*stat)->assert_total    = 0;
  (*stat)->mem_wr_hit      = 0;
  (*stat)->mem_rd_hit      = 0;
  (*stat)->mem_ae_total    = 0;
  (*stat)->mem_tog01_hit   = 0;
  (*stat)->mem_tog10_hit   = 0;
  (*stat)->mem_tog_total   = 0;
  (*stat)->mem_cov_found   = FALSE;
  (*stat)->mem_excluded    = 0;
  (*stat)->show            = TRUE;

  PROFILE_END;

}

/*!
 \return Returns TRUE if the given statistic structure contains values of 0 for all of its
         metrics.
*/
bool statistic_is_empty(
  statistic* stat  /*!< Pointer to statistic structure to check */
) { PROFILE(STATISTIC_IS_EMPTY);

  bool retval;  /* Return value for this function */

  assert( stat != NULL );

  retval = (stat->line_total    == 0) &&
           (stat->tog_total     == 0) &&
           (stat->comb_total    == 0) &&
           (stat->state_total   == 0) &&
           (stat->arc_total     == 0) &&
           (stat->assert_total  == 0) &&
           (stat->mem_ae_total  == 0) &&
           (stat->mem_tog_total == 0);

  PROFILE_END;

  return( retval );

}

/*!
 Destroys the specified statistic structure from heap memory.
*/
void statistic_dealloc(
  statistic* stat  /*!< Pointer to statistic structure to deallocate from heap */
) { PROFILE(STATISTIC_DEALLOC);

  if( stat != NULL ) {
   
    /* Free up memory for entire structure */
    free_safe( stat, sizeof( statistic ) );

  }

  PROFILE_END;

}

