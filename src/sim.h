#ifndef __SIM_H__
#define __SIM_H__

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
 \file     sim.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/20/2002
 \brief    Contains functions for simulation engine.
*/


#include "defines.h"


/*! \brief Displays the given thread to standard output (for debug purposes only). */
void sim_display_thread(
  const thread* thr,
  bool          show_queue,
  bool          endl
);

/*! \brief Displays the current state of the active queue (for debug purposes only). */
void sim_display_active_queue();

/*! \brief Displays the current state of the delay queue (for debug purposes only). */
void sim_display_delay_queue();

/*! \brief Displays the state of all threads */
void sim_display_all_list();

/*! \brief Returns a pointer to the current thread at the head of the active queue. */
thread* sim_current_thread();

/*! \brief Inserts the given thread into the delay queue at the given time slot */
void sim_thread_insert_into_delay_queue(
  thread*         thr,
  const sim_time* time
);

/*! \brief Adds specified expression's statement to pre-simulation statement queue. */
void sim_expr_changed(
  expression*     expr,
  const sim_time* time
);

/*! \brief Creates a thread for the given statement and adds it to the thread simulation queue. */
thread* sim_add_thread(
  thread*         parent,
  statement*      stmt,
  func_unit*      funit,
  const sim_time* time
);

/*! \brief Deallocates thread and removes it from parent and thread queue lists for specified functional unit */
void sim_kill_thread_with_funit(
  func_unit* funit
);

/*! \brief Pushes given thread onto the active queue */
void sim_thread_push(
  thread*         thr,
  const sim_time* time
);

/*! \brief Simulates a given expression tree, only performing needed operations as it traverses the tree. */
bool sim_expression(
  expression*     expr,
  thread*         thr,
  const sim_time* time,
  bool            lhs
);

/*! \brief Simulates one thread until it has either completed or enters a context switch */
void sim_thread(
  thread*         thr,
  const sim_time* time
);

/*! \brief Simulates current timestep. */
bool sim_simulate(
  const sim_time* time
);

/*! \brief Initializes the simulator */
void sim_initialize();

/*! \brief Causes the simulation to stop and enter into CLI mode */
void sim_stop();

/*! \brief Causes simulator to finish gracefully */
void sim_finish();

/*! \brief Updates the given non-blocking assign structure and adds it to the non-blocking assignment queue. */
void sim_add_nonblock_assign(
  nonblock_assign* nba,
  int              lhs_lsb,
  int              lhs_msb,
  int              rhs_lsb,
  int              rhs_msb
);

/*! \brief Performs non-blocking assignment for currently queued assignment items for the current timestep. */
void sim_perform_nba(
  const sim_time* time
);

/*! \brief Deallocates all memory for simulator */
void sim_dealloc();

#endif

