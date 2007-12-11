#ifndef __PROFILER_H__
#define __PROFILER_H__

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
 \file    profiler.h
 \author  Trevor Williams  (phase1geo@gmail.com)
 \date    12/10/2007
 \brief   Contains defines and functions used for profiling Covered commands.
*/

#include "defines.h"
#include "genprof.h"
#include "util.h"


#define PROFILE(index)
#define PROFILE_TIME_START
#define PROFILE_TIME_STOP
#define MALLOC_CALL
#define FREE_CALL

#ifdef DEBUG
#ifdef HAVE_SYS_TIMES_H
#undef PROFILE
#undef PROFILE_TIME_START
#undef PROFILE_TIME_STOP
#undef MALLOC_CALL
#undef FREE_CALL
#define PROFILE(index)     unsigned int profile_index = index;  if(profiling_mode) profiles[index].calls++;
#define PROFILE_TIME_START if(profiling_mode) timer_start(&profiles[profile_index].time_in);
#define PROFILE_TIME_STOP  if(profiling_mode) timer_stop(&profiles[profile_index].time_in);
#define MALLOC_CALL(index) if(profiling_mode) profiles[index].mallocs++;
#define FREE_CALL(index)   if(profiling_mode) profiles[index].frees++;
#endif
#endif


extern bool profiling_mode;


/*! \brief Sets the current profiling mode to the given value. */
void profiler_set_mode( bool value );

/*! \brief Sets the profiling output file to the given value. */
void profiler_set_filename( const char* fname );

/*! \brief Output profiler report. */
void profiler_report();


/*
 $Log$
 Revision 1.2  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.1  2007/12/10 23:16:22  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

*/

#endif
