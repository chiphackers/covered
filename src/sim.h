#ifndef __SIM_H__
#define __SIM_H__

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
void sim_thread_insert_into_delay_queue( thread* thr, const sim_time* time );

/*! \brief Adds specified expression's statement to pre-simulation statement queue. */
void sim_expr_changed( expression* expr, const sim_time* time );

/*! \brief Creates a thread for the given statement and adds it to the thread simulation queue. */
thread* sim_add_thread( thread* parent, statement* stmt, func_unit* funit, const sim_time* time );

/*! \brief Deallocates thread and removes it from parent and thread queue lists for specified functional unit */
void sim_kill_thread_with_funit( func_unit* funit );

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
void sim_thread( thread* thr, const sim_time* time );

/*! \brief Simulates current timestep. */
bool sim_simulate( const sim_time* time );

/*! \brief Initializes the simulator */
void sim_initialize();

/*! \brief Causes the simulation to stop and enter into CLI mode */
void sim_stop();

/*! \brief Causes simulator to finish gracefully */
void sim_finish();

/*! \brief Deallocates all memory for simulator */
void sim_dealloc();


/*
 $Log$
 Revision 1.38  2008/09/30 23:13:32  phase1geo
 Checkpointing (TOT is broke at this point).

 Revision 1.37  2008/03/30 05:14:32  phase1geo
 Optimizing sim_expr_changed functionality and fixing bug 1928475.

 Revision 1.36  2008/02/29 00:08:31  phase1geo
 Completed optimization code in simulator.  Still need to verify that code
 changes enhanced performances as desired.  Checkpointing.

 Revision 1.35  2008/02/28 07:54:09  phase1geo
 Starting to add functionality for simulation optimization in the sim_expr_changed
 function (feature request 1897410).

 Revision 1.34  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.33  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.32  2007/12/19 22:54:35  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.31  2007/12/19 04:27:52  phase1geo
 More fixes for compiler errors (still more to go).  Checkpointing.

 Revision 1.30  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.29  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.28  2007/04/18 22:35:02  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.27  2007/04/13 21:47:12  phase1geo
 More simulation debugging.  Added 'display all_list' command to CLI to output
 the list of all threads.  Updated regressions though we are not fully passing
 at the moment.  Checkpointing.

 Revision 1.26  2007/04/12 20:54:55  phase1geo
 Adding cli > output when replaying and adding back all of the functions (since
 the cli > prompt helps give it context.  Fixing bugs in simulation core.
 Checkpointing.

 Revision 1.25  2007/04/12 04:15:40  phase1geo
 Adding history all command, added list command and updated the display current
 command to include statement output.

 Revision 1.24  2007/04/11 22:29:49  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.23  2007/04/10 03:56:18  phase1geo
 Completing majority of code to support new simulation core.  Starting to debug
 this though we still have quite a ways to go here.  Checkpointing.

 Revision 1.22  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.21  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.20  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.19  2006/11/29 23:15:46  phase1geo
 Major overhaul to simulation engine by including an appropriate delay queue
 mechanism to handle simulation timing for delay operations.  Regression not
 fully passing at this moment but enough is working to checkpoint this work.

 Revision 1.18  2006/11/27 04:11:42  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.17  2006/10/06 17:18:13  phase1geo
 Adding support for the final block type.  Added final1 diagnostic to regression
 suite.  Full regression passes.

 Revision 1.16  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.15  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.14  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.13  2005/12/05 22:02:24  phase1geo
 Added initial support for disable expression.  Added test to verify functionality.
 Full regression passes.

 Revision 1.12  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.11  2005/11/17 05:34:44  phase1geo
 Initial work on supporting blocking assignments.  Added new diagnostic to
 check that this initial work is working correctly.  Quite a bit more work to
 do here.

 Revision 1.10  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.9  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.8  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.7  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.6  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:14:25  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.2  2002/06/22 21:08:23  phase1geo
 Added simulation engine and tied it to the db.c file.  Simulation engine is
 currently untested and will remain so until the parser is updated correctly
 for statements.  This will be the next step.

 Revision 1.1  2002/06/21 05:55:05  phase1geo
 Getting some codes ready for writing simulation engine.  We should be set
 now.
*/

#endif

