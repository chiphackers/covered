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

/*! \brief Totals all states hit during simulation. */
int arc_state_hit_total( char* arcs );

/*! \brief Totals all state transitions hit during simulation. */
int arc_transition_hit_total( char* arcs );

/*! \brief Writes specified arc array to specified CDD file. */
bool arc_db_write( char* arcs, FILE* file );

/*! \brief Reads in arc array from CDD database string. */
bool arc_db_read( char** arcs, char** line );

/*! \brief Merges contents of arc table from line to specified base array. */
bool arc_db_merge( char* arcs, char** line, bool same );

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc( char* arcs );

/*
 $Log$
 Revision 1.2  2003/08/29 12:52:06  phase1geo
 Updating comments for functions.

 Revision 1.1  2003/08/26 12:53:35  phase1geo
 Added initial versions of arc.c and arc.h though they are not even close to
 being finished at this point.

*/

#endif

