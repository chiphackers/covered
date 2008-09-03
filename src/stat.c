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

/*
 $Log$
 Revision 1.17  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.13.4.3  2008/08/07 06:39:11  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.13.4.2  2008/08/06 20:11:35  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.13.4.1  2008/07/10 22:43:54  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.15  2008/06/27 14:02:04  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.14  2008/06/19 05:52:36  phase1geo
 Fixing bug 1997423.  Added report coverage diagnostics.

 Revision 1.13  2008/03/17 05:26:17  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.12  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.11  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.10  2006/09/25 04:15:04  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

 Revision 1.9  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.8  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.7  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

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

