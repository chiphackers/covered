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


/*! \brief Returns the width of the specified arc. */
unsigned int arc_get_width(
  const char* arcs
);

/*! \brief Allocates and initializes new state transition array. */
char* arc_create(
  int width
);

/*! \brief Adds new state transition arc entry to specified table. */
void arc_add(
  char**        arcs,
  const vector* fr_st,
  const vector* to_st,
  int           hit,
  bool          exclude
);

/*! \brief Gets supplemental field value from specified arc entry table. */
unsigned int arc_get_suppl(
  const char*  arcs,
  unsigned int type
);

/*! \brief Sets supplemental field value to specified arc entry. */
void arc_set_entry_suppl(
  char*        arcs,
  int          curr,
  unsigned int type,
  char         val
);

/*! \brief Gets supplemental field value from specified arc entry. */
int arc_get_entry_suppl(
  const char*  arcs,
  int          curr,
  unsigned int type
);

/*! \brief Finds the specified state transition in the given arc array */
int arc_find(
  const char*   arcs,
  const vector* from_st,
  const vector* to_st,
  int*          ptr
);

/*! \brief Calculates all state and state transition values for reporting purposes. */
void arc_get_stats(
  char* arcs,
  int*  state_total,
  int*  state_hits,
  int*  arc_total,
  int*  arc_hits
);

/*! \brief Writes specified arc array to specified CDD file. */
void arc_db_write(
  const char* arcs,
  FILE*       file
);

/*! \brief Reads in arc array from CDD database string. */
void arc_db_read(
  char** arcs,
  char** line
);

/*! \brief Merges contents of arc table from line to specified base array. */
void arc_db_merge(
  char** arcs,
  char** line,
  bool   same
);

/*! \brief Merges two FSM arcs, placing the results in the base arc. */
void arc_merge(
  char** base,
  char*  other
);

/*! \brief Stores arc array state values to specified string array. */
void arc_get_states(
  char***     states,
  int*        state_size,
  const char* arcs,
  bool        hit,
  bool        any
);

/*! \brief Outputs arc array state transition values to specified output stream. */
void arc_get_transitions(
  char***     from_states,
  char***     to_states,
  int**       excludes,
  int*        arc_size,
  const char* arcs,
  bool        hit,
  bool        any
);

/*! \brief Specifies if any state transitions have been excluded from coverage. */
bool arc_are_any_excluded(
  const char* arcs
);

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc(
  /*@only@*/ char* arcs
);

/*
 $Log$
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

