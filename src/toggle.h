#ifndef __TOGGLE_H__
#define __TOGGLE_H__

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
 \file     toggle.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting toggle coverage.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Calculates the toggle coverage for the specifed expression and signal lists. */
void toggle_get_stats(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit01,
  /*@out@*/ unsigned int* hit10,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total,
  /*@out@*/ bool*         cov_found
);

/*! \brief Collects all toggle expressions that match the specified coverage indication. */
void toggle_collect(
            func_unit*  funit,
            int         cov,
  /*@out@*/ sig_link**  sig_head,
  /*@out@*/ sig_link**  sig_tail
);

/*! \brief Gets toggle coverage information for a single signal in the specified functional unit */
void toggle_get_coverage(
            func_unit* funit,
            char*      sig_name,
  /*@out@*/ int*       msb,
  /*@out@*/ int*       lsb,
  /*@out@*/ char**     tog01,
  /*@out@*/ char**     tog10,
  /*@out@*/ int*       excluded
);

/*! \brief Gets total and hit toggle signal status for the specified functional unit */
void toggle_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gets total and hit toggle signal status for the specified functional unit instance */
void toggle_get_inst_summary(
            funit_inst*   inst,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Generates report output for toggle coverage. */
void toggle_report(
  FILE* ofile,
  bool  verbose
);


/*
 $Log$
 Revision 1.21  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.17.6.3  2008/08/07 06:39:11  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.17.6.2  2008/08/06 20:11:35  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.17.6.1  2008/07/10 22:43:55  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.19  2008/06/27 14:02:04  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.18  2008/06/19 05:52:36  phase1geo
 Fixing bug 1997423.  Added report coverage diagnostics.

 Revision 1.17  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.16  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.15  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.14  2006/06/29 22:44:57  phase1geo
 Fixing newly introduced bug in FSM report handler.  Also adding pointers back
 to main text window when exclusion properties are changed.  Fixing toggle
 coverage retension.  This is partially working but doesn't seem to want to
 save/restore properly at this point.

 Revision 1.13  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.12  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.11  2006/01/19 00:01:09  phase1geo
 Lots of changes/additions.  Summary report window work is now complete (with the
 exception of adding extra features).  Added support for parsing left and right
 shift operators and the exponent operator in static expression scenarios.  Fixed
 issues related to GUI (due to recent changes in the score command).  Things seem
 to be generally working as expected with the GUI now.

 Revision 1.10  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.9  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.8  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.7  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.6  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:14:30  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:51  phase1geo
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

