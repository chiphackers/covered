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

#endif
