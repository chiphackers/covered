#ifndef __STAT_H__
#define __STAT_H__

/*!
 \file     stat.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/4/2002
 \brief    Contains functions for handling coverage statistics.
*/

#include "defines.h"


/*! \brief Creates and initializes a new statistic structure. */
statistic* statistic_create();

/*! \brief Merges the results of the stat_from to the stat_to */
void statistic_merge( statistic* stat_to, statistic* stat_from );

/*! \brief Deallocates memory for a statistic structure. */
void statistic_dealloc( statistic* stat );


/*
 $Log$
 Revision 1.4  2002/10/31 23:14:26  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.3  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

