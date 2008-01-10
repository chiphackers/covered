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


#define PROFILE(index) int foobar
#define PROFILE_START(index)
#define PROFILE_END    foobar = 0
#define MALLOC_CALL
#define FREE_CALL

#ifdef PROFILER
#ifdef HAVE_SYS_TIME_H

#undef PROFILE
#undef PROFILE_START
#undef PROFILE_END
#undef MALLOC_CALL
#undef FREE_CALL

#define PROFILE(index)     unsigned int profile_index = index;  if(profiling_mode) profiler_enter(index);
#define PROFILE_END        if(profiling_mode) profiler_exit(profile_index);
#define MALLOC_CALL(index) if(profiling_mode) profiles[index].mallocs++;
#define FREE_CALL(index)   if(profiling_mode) profiles[index].frees++;

#endif
#endif


/*@-exportlocal@*/
extern bool profiling_mode;
/*@=exportlocal@*/


/*! \brief Sets the current profiling mode to the given value. */
void profiler_set_mode( bool value );

/*! \brief Sets the profiling output file to the given value. */
void profiler_set_filename( const char* fname );

/*! \brief Function to be called whenever a new function is entered. */
void profiler_enter( unsigned int index );

/*! \brief Function to be called whenever a timed function is exited. */
void profiler_exit( unsigned int index );

/*! \brief Output profiler report. */
void profiler_report();


/*
 $Log$
 Revision 1.6  2008/01/07 05:01:58  phase1geo
 Cleaning up more splint errors.

 Revision 1.5  2007/12/12 07:53:00  phase1geo
 Separating debugging and profiling so that we can do profiling without all
 of the debug overhead.

 Revision 1.4  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.3  2007/12/11 15:07:35  phase1geo
 More modifications.

 Revision 1.2  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.1  2007/12/10 23:16:22  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

*/

#endif
