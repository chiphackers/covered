#ifndef __RACE_H__
#define __RACE_H__

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
 \file     race.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/15/2004
 \brief    Contains functions used to check for race conditions and proper signal use
           within the specified design.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Checks the current module for race conditions */
void race_check_modules();

/*! \brief Writes contents of specified race condition block to specified file output */
void race_db_write( race_blk* head, FILE* file );

/*! \brief Reads contents from specified line for a race condition block and assigns the new block to the curr_mod */
void race_db_read( char** line, /*@null@*/func_unit* curr_mod );

/*! \brief Get statistic information for the specified race condition block list */
void race_get_stats(
            race_blk*     curr,
  /*@out@*/ unsigned int* race_total,
  /*@out@*/ unsigned int  type_total[][RACE_TYPE_NUM]
);

/*! \brief Displays report information for race condition blocks in design */
void race_report( FILE* ofile, bool verbose );

/*! \brief Collects all of the lines in the specified module that were not verified due to race condition breach */
void race_collect_lines(
            func_unit* funit,
  /*@out@*/ int**      slines,
  /*@out@*/ int**      elines,
  /*@out@*/ int**      reasons,
  /*@out@*/ int*       line_cnt
);

/*! \brief Deallocates the specified race condition block from memory */
void race_blk_delete_list( race_blk* rb );


/*
 $Log$
 Revision 1.23.4.2  2008/08/06 20:11:35  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.23.4.1  2008/07/10 22:43:54  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.24  2008/06/27 14:02:04  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.23  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.22  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.21  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.20  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.19  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.18  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.17  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.16  2006/03/27 17:37:23  phase1geo
 Fixing race condition output.  Some regressions may fail due to these changes.

 Revision 1.15  2005/12/23 05:41:52  phase1geo
 Fixing several bugs in score command per bug report #1388339.  Fixed problem
 with race condition checker statement iterator to eliminate infinite looping (this
 was the problem in the original bug).  Also fixed expression assigment when static
 expressions are used in the LHS (caused an assertion failure).  Also fixed the race
 condition checker to properly pay attention to task calls, named blocks and fork
 statements to make sure that these are being handled correctly for race condition
 checking.  Fixed bug for signals that are on the LHS side of an assignment expression
 but is not being assigned (bit selects) so that these are NOT considered for race
 conditions.  Full regression is a bit broken now but the opened bug can now be closed.

 Revision 1.14  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.13  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.12  2005/02/07 22:19:46  phase1geo
 Added code to output race condition reasons to informational bar.  Also added code to
 output toggle and combinational logic output to information bar when cursor is over
 an expression that, when clicked on, will take you to the detailed coverage window.

 Revision 1.11  2005/02/05 05:29:25  phase1geo
 Added race condition reporting to GUI.

 Revision 1.10  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.9  2005/02/04 23:55:54  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.8  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.7  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.6  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.5  2004/12/20 04:12:00  phase1geo
 A bit more race condition checking code added.  Still not there yet.

 Revision 1.4  2004/12/18 16:23:18  phase1geo
 More race condition checking updates.

 Revision 1.3  2004/12/17 22:29:36  phase1geo
 More code added to race condition feature.  Still not usable.

 Revision 1.2  2004/12/17 14:27:46  phase1geo
 More code added to race condition checker.  This is in an unusable state at
 this time.

 Revision 1.1  2004/12/16 13:52:58  phase1geo
 Starting to add support for race-condition detection and handling.

*/

#endif

