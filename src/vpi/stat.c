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

  stat = (statistic*)malloc_safe( sizeof( statistic ), __FILE__, __LINE__ );

  stat->line_total  = 0;
  stat->line_hit    = 0;
  stat->tog_total   = 0;
  stat->tog01_hit   = 0;
  stat->tog10_hit   = 0;
  stat->comb_total  = 0;
  stat->comb_hit    = 0;
  stat->state_total = 0;
  stat->state_hit   = 0;
  stat->arc_total   = 0;
  stat->arc_hit     = 0;

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

  stat_to->line_total  += stat_from->line_total;
  stat_to->line_hit    += stat_from->line_hit;
  stat_to->tog_total   += stat_from->tog_total;
  stat_to->tog01_hit   += stat_from->tog01_hit;
  stat_to->tog10_hit   += stat_from->tog10_hit;
  stat_to->comb_total  += stat_from->comb_total;
  stat_to->comb_hit    += stat_from->comb_hit;
  if( (stat_to->state_total != -1) && (stat_from->state_total != -1) ) {
    stat_to->state_total += stat_from->state_total;
  } else {
    stat_to->state_total = -1;
  }
  stat_to->state_hit   += stat_from->state_hit;
  if( (stat_to->arc_total != -1) && (stat_from->arc_total != -1) ) {
    stat_to->arc_total += stat_from->arc_total;
  } else {
    stat_to->arc_total = -1;
  }
  stat_to->arc_hit     += stat_from->arc_hit;

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

/*
 $Log$
 Revision 1.6  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.5  2003/11/10 04:25:50  phase1geo
 Adding more FSM diagnostics to regression suite.  All major testing for
 current FSM code should be complete at this time.  A few bug fixes to files
 that were found during this regression testing.

 Revision 1.4  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.3  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

