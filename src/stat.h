#ifndef __STAT_H__
#define __STAT_H__

/*!
 \file     stat.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/4/2002
 \brief    Contains functions for handling coverage statistics.
*/

#include "defines.h"


//! Creates and initializes a new statistic structure.
statistic* statistic_create();

//! Merges the results of the stat_from to the stat_to
void statistic_merge( statistic* stat_to, statistic* stat_from );

//! Deallocates memory for a statistic structure.
void statistic_dealloc( statistic* stat );

/*
 $Log$
 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

