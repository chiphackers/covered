#ifndef __SYS_TASKS_H__
#define __SYS_TASKS_H__

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
 \file      sys_tasks.h
 \author    Trevor Williams  (phase1geo@gmail.com)
 \date      10/2/2008
 \brief     Contains functions that handle various system tasks.
 \note      The contents of this file came from Icarus Verilog source code and should be attributed
            to this group.  Only the format of this code has been modified.
*/ 


/*! \brief Performs $srandom system task call. */
void sys_task_srandom(
  long seed
);

/*! \brief Performs $random system task call. */
long sys_task_random(
  long* seed
);

/*! \brief Performs $urandom system task call. */
unsigned long sys_task_urandom(
  long* seed
);

/*! \brief Performs $urandom_range system task call. */
unsigned long sys_task_urandom_range(
  unsigned long max,
  unsigned long min
);

/*! \brief Converts a 64-bit real value to a 64-bit unsigned value. */
uint64 sys_task_realtobits(
  double real
);

/*! \brief Converts a 64-bit value to a 64-bit real. */
double sys_task_bitstoreal(
  uint64 u64
);

/*! \brief Converts a 32-bit real value to a 32-bit unsigned value. */
uint32 sys_task_shortrealtobits(
  float real
);

/*! \brief Converts a 32-bit value to a 32-bit real. */
float sys_task_bitstoshortreal(
  uint32 u32
);

/*! \brief Returns a real representation of the specified integer value. */
double sys_task_itor(
  int ival
);

/*! \brief Returns an integer representation of the specified real value (value is truncated). */
int sys_task_rtoi(
  double real
);

/*! \brief Scans command-line for plusargs */
void sys_task_store_plusarg(
  const char* arg
);

/*! \brief Returns 1 if the specified plusarg was found; otherwise, returns 0. */
ulong sys_task_test_plusargs(
  const char* arg
);

/*!
 \brief Parses command-line for value plusargs, assigns the specified vector the found value and
        returns 1 (if found).
*/ 
ulong sys_task_value_plusargs(
  const char* arg,
  vector*     vec
);

/*! \brief Deallocates any memory allocated within this file. */
void sys_task_dealloc();

#endif

