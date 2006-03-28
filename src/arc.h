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
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     8/25/2003
 \brief    Contains functions for handling FSM arc arrays.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Allocates and initializes new state transition array. */
char* arc_create( int width );

/*! \brief Adds new state transition arc entry to specified table. */
void arc_add( char** arcs, vector* fr_st, vector* to_st, int hit );

/*! \brief Gets supplemental field value from specified arc entry table. */
int arc_get_suppl( const char* arcs, int type );

/*! \brief Calculates all state and state transition values for reporting purposes. */
void arc_get_stats( char* arcs, float* state_total, int* state_hits, float* arc_total, int* arc_hits );

/*! \brief Writes specified arc array to specified CDD file. */
bool arc_db_write( const char* arcs, FILE* file );

/*! \brief Reads in arc array from CDD database string. */
bool arc_db_read( char** arcs, char** line );

/*! \brief Merges contents of arc table from line to specified base array. */
bool arc_db_merge( char** arcs, char** line, bool same );

/*! \brief Replaces contents of arc table from line to specified base array. */
bool arc_db_replace( char** arcs, char** line );

/*! \brief Outputs arc array state values to specified output stream. */
void arc_display_states( FILE* ofile, const char* fstr, const char* arcs, bool hit );

/*! \brief Outputs arc array state transition values to specified output stream. */
void arc_display_transitions( FILE* ofile, const char* fstr, const char* arcs, bool hit );

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc( char* arcs );

/*
 $Log$
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

