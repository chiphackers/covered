/*!
 \file     stat.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/4/2002
*/

#include "stat.h"
#include "defines.h"
#include "util.h"


/*!
 \return  Pointer to newly created/initialized statistic structure.

 Allocates new memory for a coverage statistic structure and initializes
 its values.
*/
statistic* statistic_create() {

  statistic* stat;   /* New statistic structure */

  stat = (statistic*)malloc_safe( sizeof( statistic ) );

  stat->line_total = 0;
  stat->line_hit   = 0;
  stat->tog_total  = 0;
  stat->tog01_hit  = 0;
  stat->tog10_hit  = 0;
  stat->comb_total = 0;
  stat->comb_hit   = 0;

  return( stat );

}

/*!
 \param stat_to    Statistic structure to merge information into.
 \param stat_from  Statistic structure to be merged.

 Adds the values of the stat_to structure to the contents of the
 stat_from structure.  The stat_from structure will then contain
 accumulated results.
*/
void statistic_merge( statistic* stat_to, statistic* stat_from ) {

  stat_to->line_total += stat_from->line_total;
  stat_to->line_hit   += stat_from->line_hit;
  stat_to->tog_total  += stat_from->tog_total;
  stat_to->tog01_hit  += stat_from->tog01_hit;
  stat_to->tog10_hit  += stat_from->tog10_hit;
  stat_to->comb_total += stat_from->comb_total;
  stat_to->comb_hit   += stat_from->comb_hit;

}

/*!
 \param stat  Pointer to statistic structure to deallocate from heap.

 Destroys the specified statistic structure from heap memory.
*/
void statistic_dealloc( statistic* stat ) {

  if( stat != NULL ) {
   
    /* Free up memory for entire structure */
    free_safe( stat );

  }

}
