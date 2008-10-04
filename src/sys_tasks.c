/*
 Copyright (c) 2006-2008 Trevor Williams

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
 \file      sys_tasks.c
 \author    Trevor Williams  (phase1geo@gmail.com)
 \date      10/2/2008
 \note      The contents of this file came from Icarus Verilog source code and should be attributed
            to this group.  Only the format of this code has been modified.
*/ 

#include <limits.h>

#include "profiler.h"

#if ULONG_MAX > 4294967295UL
#define UNIFORM_MAX INT_MAX
#define UNIFORM_MIN INT_MIN
#else
#define UNIFORM_MAX LONG_MAX
#define UNIFORM_MIN LONG_MIN
#endif


/*!
 Random seed value.
*/
static long random_seed = 0;


/*!
 \return Returns a randomly generated number that is uniformly distributed.
*/
static double sys_task_uniform(
  long* seed,   /*!< Pointer to seed value to use and store new seed in */
  long  start,  /*!< Beginning range */
  long  end     /*!< Ending range */
) { PROFILE(SYS_TASK_UNIFORM);

  double        d = 0.00000011920928955078125;
  double        a, b, c;
  unsigned long oldseed, newseed;

  oldseed = *seed;

  if( oldseed == 0 )
    oldseed = 259341593;

  if( start >= end ) {
    a = 0.0;
    b = 2147483647.0;
  } else {
    a = (double)start;
    b = (double)end;
  }

  /* Original routine used signed arithmetic, and the (frequent)
   * overflows trigger "Undefined Behavior" according to the
   * C standard (both c89 and c99).  Using unsigned arithmetic
   * forces a conforming C implementation to get the result
   * that the IEEE-1364-2001 committee wants.
   */
  newseed = 69069 * oldseed + 1;

  /* Emulate a 32-bit unsigned long, even if the native machine
   * uses wider words.
   */
#if ULONG_MAX > 4294967295UL
  newseed = newseed & 4294967295UL;
#endif
  *seed = newseed;

  /* Equivalent conversion without assuming IEEE 32-bit float */
  /* constant is 2^(-23) */
  c = 1.0 + (newseed >> 9) * 0.00000011920928955078125;
  c = c + (c*d);
  c = ((b - a) * (c - 1.0)) + a;

  PROFILE_END;

  return( c );

}

/*!
 \return Returns a randomly generated integer value that is uniformly distributed.

 \note
 Copied from IEEE1364-2001, with slight modifications for 64bit machines.  This code is taken from
 Icarus Verilog.
*/
long sys_task_dist_uniform(
  long* seed,   /*!< Pointer to seed value to use and store new seed value to */
  long  start,  /*!< Starting range of random value */
  long  end     /*!< Ending range of random value */
) { PROFILE(SYS_TASK_RTL_DIST_UNIFORM);

  double r;
  long   i;

  if( start >= end ) {

    i = start;

  } else {

    /*
     NOTE: The cast of r to i can overflow and generate strange
     values, so cast to unsigned long first. This eliminates
     the underflow and gets the twos complement value. That in
     turn can be cast to the long value that is expected.
    */

    if( end != UNIFORM_MAX ) {

      end++;
      r = sys_task_uniform( seed, start, end );
      if( r >= 0 ) {
        i = (unsigned long) r;
      } else {
        i = - ( (unsigned long) (-(r - 1)) );
      }
      if( i < start ) i = start;
      if( i >= end ) i = end - 1;

    } else if( start != UNIFORM_MIN ) {

      start--;
      r = sys_task_uniform( seed, start, end ) + 1.0;
      if( r >= 0 ) {
        i = (unsigned long) r;
      } else {
        i = - ( (unsigned long) (-(r - 1)) );
      }
      if( i <= start ) i = start + 1;
      if( i > end ) i = end;

    } else {

      r = (sys_task_uniform( seed, start, end ) + 2147483648.0) / 4294967295.0;
      r = r * 4294967296.0 - 2147483648.0;

      if( r >= 0 ) {
        i = (unsigned long) r;
      } else {
        /*
         At least some compilers will notice that (r-1)
         is <0 when castling to unsigned long and
         replace the result with a zero. This causes
         much wrongness, so do the casting to the
         positive version and invert it back.
        */
        i = - ( (unsigned long) (-(r - 1)) );
      }

    }

  }

  PROFILE_END;

  return( i );

}


/*!
 Sets the global seed value to the specified value.
*/
void sys_task_srandom(
  long seed  /*!< User-specified seed to set global seed value to */
) { PROFILE(SYS_TASK_SRANDOM);

  random_seed = seed;

  PROFILE_END;

}

/*!
 \return Returns a randomly generated signed value
*/
long sys_task_random(
  long* seed  /*!< Pointer to seed value to use and store new information to */
) { PROFILE(SYS_TASK_RANDOM);

  long result;
  
  if( seed != NULL ) {
    random_seed = *seed;
  }

  result = sys_task_dist_uniform( &random_seed, INT_MIN, INT_MAX );

  if( seed != NULL ) {
    *seed = random_seed;
  }

  PROFILE_END;

  return( result );

}

/*!
 \return Returns a randomly generated unsigned value within a given range

 \note
 From System Verilog 3.1a.  This code is from the Icarus Verilog project.
*/
unsigned long sys_task_urandom(
  long* seed  /*!< Pointer to seed value to use and store new information to */
) { PROFILE(SYS_TASK_URANDOM);

  unsigned long result;

  if( seed != NULL ) {
    random_seed = *seed;
  }

  result = sys_task_dist_uniform( &random_seed, INT_MIN, INT_MAX ) - INT_MIN;

  if( seed != NULL ) {
    *seed = random_seed;
  }

  return( result );

}

/*!
 \return Returns a randomly generated unsigned value within a given range

 \note
 From System Verilog 3.1a.  This code is from the Icarus Verilog project.
*/
unsigned long sys_task_urandom_range(
  unsigned long max,   /*!< Maximum range value */
  unsigned long min    /*!< Minimum range value */
) { PROFILE(SYS_TASK_URANDOM_RANGE);

  unsigned long result;
  long          max_i, min_i;

  max_i = max + INT_MIN;
  min_i = min + INT_MIN;

  result = sys_task_dist_uniform( &random_seed, min_i, max_i ) - INT_MIN;

  return( result );

}


/*
 $Log$
 Revision 1.2  2008/10/03 21:47:32  phase1geo
 Checkpointing more system task work (things might be broken at the moment).

 Revision 1.1  2008/10/02 06:46:33  phase1geo
 Initial $random support added.  Added random1 and random1.1 diagnostics to regression
 suite.  random1.1 is currently failing.  Checkpointing.

*/
