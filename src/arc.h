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


/*! \brief Writes specified arc array to specified CDD file. */
bool arc_db_write( char* arcs, FILE* file );

/*! \brief Reads in arc array from CDD database string. */
bool arc_db_read( char** arcs, char** line );

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc( char* arcs );

/*
 $Log$
 Revision 1.1  2003/08/26 12:53:35  phase1geo
 Added initial versions of arc.c and arc.h though they are not even close to
 being finished at this point.

*/

#endif

