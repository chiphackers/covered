#ifndef __STAT_H__
#define __STAT_H__

/*
 Copyright (c) 2006-2010 Trevor Williams

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
 \file     stat.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/4/2002
 \brief    Contains functions for handling coverage statistics.
*/

#include "defines.h"


/*! \brief Creates and initializes a new statistic structure. */
void statistic_create( statistic** stat );

/*! \brief Returns TRUE if the given statistic structure contains no coverage information */
bool statistic_is_empty( statistic* stat );

/*! \brief Deallocates memory for a statistic structure. */
void statistic_dealloc( statistic* stat );

#endif

