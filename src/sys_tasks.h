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


/*! \brief Performs $random system task call. */
long sys_task_dist_uniform(
  long* seed,
  long  start,
  long  end
);

/*
 $Log$
*/

#endif
