#ifndef __ARC_H__
#define __ARC_H__

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
 \file     arc.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/25/2003
 \brief    Contains functions for handling FSM arc arrays.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Allocates and initializes new state transition array. */
fsm_table* arc_create();

/*! \brief Adds new state transition arc entry to specified table. */
void arc_add(
  fsm_table*    table,
  const vector* fr_st,
  const vector* to_st,
  int           hit,
  bool          exclude
);

/*! \brief Finds the specified FROM state in the given FSM table */
int arc_find_from_state(
            const fsm_table* table,
            const vector*    st
);

/*! \brief Finds the specified TO state in the given FSM table */
int arc_find_to_state(
            const fsm_table* table,
            const vector*    st
);

/*! \brief Finds the specified state transition in the given FSM table */
int arc_find_arc(
  const fsm_table* table,
  unsigned int     fr_index,
  unsigned int     to_index
);

/*! \brief Finds the specified state transition in the given FSM table by the exclusion ID */ 
int arc_find_arc_by_exclusion_id(
  const fsm_table* table,
  int              id
);

/*! \brief Calculates all state and state transition values for reporting purposes. */
void arc_get_stats(
            const fsm_table* table,
  /*@out@*/ int*             state_hits,
  /*@out@*/ int*             state_total,
  /*@out@*/ int*             arc_hits,
  /*@out@*/ int*             arc_total,
  /*@out@*/ int*             arc_excluded
);

/*! \brief Writes specified arc array to specified CDD file. */
void arc_db_write(
  const fsm_table* table,
  FILE*            file
);

/*! \brief Reads in arc array from CDD database string. */
void arc_db_read(
  fsm_table** table,
  char**      line
);

/*! \brief Merges contents of arc table from line to specified base array. */
void arc_db_merge(
  fsm_table* table,
  char**     line
);

/*! \brief Merges two FSM arcs, placing the results in the base arc. */
void arc_merge(
  fsm_table*       base,
  const fsm_table* other
);

/*! \brief Stores arc array state values to specified string array. */
void arc_get_states(
  char***          fr_states,
  unsigned int*    fr_state_size,
  char***          to_states,
  unsigned int*    to_state_size,
  const fsm_table* table,
  bool             hit,
  bool             any
);

/*! \brief Outputs arc array state transition values to specified output stream. */
void arc_get_transitions(
  /*@out@*/ char***          from_states,
  /*@out@*/ char***          to_states,
  /*@out@*/ int**            ids,
  /*@out@*/ int**            excludes,
  /*@out@*/ int*             arc_size,
            const fsm_table* table,
            bool             hit,
            bool             any
);

/*! \brief Specifies if any state transitions have been excluded from coverage. */
bool arc_are_any_excluded(
  const fsm_table* table
);

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc(
  /*@only@*/ fsm_table* table
);

/*
 $Log$
 Revision 1.32  2008/08/29 05:38:36  phase1geo
 Adding initial pass of FSM exclusion ID output.  Need to fix issues with the -e
 option usage for all metrics, I believe (certainly for FSM).  Checkpointing.

 Revision 1.31  2008/08/18 23:07:25  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.28.2.2  2008/08/07 06:39:10  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.28.2.1  2008/07/10 22:43:46  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.29  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.28  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.27.2.3  2008/05/08 23:12:41  phase1geo
 Fixing several bugs and reworking code in arc to get FSM diagnostics
 to pass.  Checkpointing.

 Revision 1.27.2.2  2008/05/08 03:56:38  phase1geo
 Updating regression files and reworking arc_find and arc_add functionality.
 Checkpointing.

 Revision 1.27.2.1  2008/05/02 22:06:10  phase1geo
 Updating arc code for new data structure.  This code is completely untested
 but does compile and has been completely rewritten.  Checkpointing.

 Revision 1.27  2008/04/15 06:08:46  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.26  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.25  2008/02/01 06:37:07  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.24  2008/01/16 06:40:33  phase1geo
 More splint updates.

 Revision 1.23  2008/01/16 05:01:21  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.22  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.21  2008/01/07 05:01:57  phase1geo
 Cleaning up more splint errors.

 Revision 1.20  2008/01/04 23:07:58  phase1geo
 More splint updates.

 Revision 1.19  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.18  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.17  2006/10/12 22:48:45  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.16  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.15  2006/06/29 04:26:02  phase1geo
 More updates for FSM coverage.  We are getting close but are just not to fully
 working order yet.

 Revision 1.14  2006/06/28 22:15:19  phase1geo
 Adding more code to support FSM coverage.  Still a ways to go before this
 is completed.

 Revision 1.13  2006/06/23 19:45:26  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.12  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.11  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.10  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.9  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.8  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.7  2003/09/19 13:25:28  phase1geo
 Adding new FSM diagnostics including diagnostics to verify FSM merging function.
 FSM merging code was modified to work correctly.  Full regression passes.

 Revision 1.6  2003/09/14 01:09:20  phase1geo
 Added verbose output for FSMs.

 Revision 1.5  2003/09/13 19:53:59  phase1geo
 Adding correct way of calculating state and state transition totals.  Modifying
 FSM summary reporting to reflect these changes.  Also added function documentation
 that was missing from last submission.

 Revision 1.4  2003/09/13 02:59:34  phase1geo
 Fixing bugs in arc.c created by extending entry supplemental field to 5 bits
 from 3 bits.  Additional two bits added for calculating unique states.

 Revision 1.3  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.2  2003/08/29 12:52:06  phase1geo
 Updating comments for functions.

 Revision 1.1  2003/08/26 12:53:35  phase1geo
 Added initial versions of arc.c and arc.h though they are not even close to
 being finished at this point.

*/

#endif

