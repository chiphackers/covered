#ifndef __SYS_TASKS_H__
#define __SYS_TASKS_H__

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
 \file      sys_tasks.h
 \author    Trevor Williams  (phase1geo@gmail.com)
 \date      10/2/2008
 \brief     Contains functions that handle various system tasks.
 \note      The contents of this file came from Icarus Verilog source code and should be attributed
            to this group.  Only the format of this code has been modified.
*/ 


/*! \brief Performs $srandom system task call. */
long sys_task_srandom(
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

/*
 $Log$
 Revision 1.2  2008/10/03 21:47:32  phase1geo
 Checkpointing more system task work (things might be broken at the moment).

 Revision 1.1  2008/10/02 06:46:33  phase1geo
 Initial $random support added.  Added random1 and random1.1 diagnostics to regression
 suite.  random1.1 is currently failing.  Checkpointing.

*/

#endif
