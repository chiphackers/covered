#ifndef __ARC_H__
#define __ARC_H__

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
void arc_add( char** arcs, int width, vector* fr_st, vector* to_st, int hit );

/*! \brief Calculates all state and state transition values for reporting purposes. */
void arc_get_stats( char* arcs, float* state_total, int* state_hits, float* arc_total, int* arc_hits );

/*! \brief Writes specified arc array to specified CDD file. */
bool arc_db_write( char* arcs, FILE* file );

/*! \brief Reads in arc array from CDD database string. */
bool arc_db_read( char** arcs, char** line );

/*! \brief Merges contents of arc table from line to specified base array. */
bool arc_db_merge( char* arcs, char** line, bool same );

/*! \brief Outputs arc array state values to specified output stream. */
void arc_display_states( FILE* ofile, char* fstr, char* arcs, bool hit );

/*! \brief Outputs arc array state transition values to specified output stream. */
void arc_display_transitions( FILE* ofile, char* fstr, char* arcs, bool hit );

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc( char* arcs );

/*
 $Log$
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

