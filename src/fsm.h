#ifndef __FSM_H__
#define __FSM_H__

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
 \file     fsm.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting FSM coverage.
*/

#include <stdio.h>

#include "defines.h"

/*! \brief Creates and initializes new FSM structure. */
fsm* fsm_create( expression* from_state, expression* to_state );

/*! \brief Adds new FSM arc structure to specified FSMs arc list. */
void fsm_add_arc( fsm* table, expression* from_state, expression* to_state );

/*! \brief Sets sizes of tables in specified FSM structure. */
void fsm_create_tables( fsm* table );

/*! \brief Outputs contents of specified FSM to CDD file. */
bool fsm_db_write( fsm* table, FILE* file, bool parse_mode );

/*! \brief Reads in contents of specified FSM. */
bool fsm_db_read( char** line, /*@null@*/func_unit* funit );

/*! \brief Reads and merges two FSMs, placing result into base FSM. */
bool fsm_db_merge( fsm* base, char** line, bool same );

/*! \brief Sets the bit in set table based on the values of last and curr. */
void fsm_table_set( fsm* table );

/*! \brief Gathers statistics about the current FSM */
void fsm_get_stats( fsm_link* table, float* state_total, int* state_hit, float* arc_total, int* arc_hit );

/*! \brief Retrieves the FSM summary information for the specified functional unit. */
bool fsm_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit );

/*! \brief Retrieves covered and uncovered FSMs from the specified functional unit. */
bool fsm_collect( char* funit_name, int funit_type, sig_link** cov_head, sig_link** cov_tail,
                  sig_link** uncov_head, sig_link** uncov_tail, int** expr_ids, int** excludes );

/*! \brief Collects all coverage information for the specified FSM */
bool fsm_get_coverage( char* funit_name, int funit_type, int expr_id, int* width,
                       char*** total_states, int* total_state_num,
                       char*** hit_states, int* hit_state_num,
                       char*** total_from_arcs, char*** total_to_arcs, int** excludes, int* total_arc_num,
                       char*** hit_from_arcs, char*** hit_to_arcs, int* hit_arc_num,
                       char*** input_state, int* input_size, char*** output_state, int* output_size );

/*! \brief Generates report output for FSM coverage. */
void fsm_report( FILE* ofile, bool verbose );

/*! \brief Deallocates specified FSM structure. */
void fsm_dealloc( fsm* table );

/*
 $Log$
 Revision 1.21  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.20  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.19  2006/06/29 04:26:02  phase1geo
 More updates for FSM coverage.  We are getting close but are just not to fully
 working order yet.

 Revision 1.18  2006/06/28 22:15:19  phase1geo
 Adding more code to support FSM coverage.  Still a ways to go before this
 is completed.

 Revision 1.17  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.16  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.15  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.14  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.13  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.12  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.11  2003/10/13 12:27:25  phase1geo
 More fixes to FSM stuff.

 Revision 1.10  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.9  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.8  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.7  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.6  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:13:50  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

