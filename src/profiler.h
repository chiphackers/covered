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
#define PROFILE(index)     unsigned int profile_index = index;  profiles[index].calls++;
#define PROFILE_TIME_START timer_start(&profiles[profile_index].time_in);
#define PROFILE_TIME_STOP  timer_stop(&profiles[profile_index].time_in);
#define MALLOC_CALL(index) profiles[index].mallocs++;
#define FREE_CALL(index)   profiles[index].frees++;
#endif
#endif


/*!
 Initializes all of the profiling variables.
*/
void profiler_initialize();


/*
 $Log$
*/

#endif
