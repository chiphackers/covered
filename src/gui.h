#ifndef __GUI_H__
#define __GUI_H__

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
 \file     gui.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/24/2003
 \brief    Contains functions that are used by the GUI report viewer.
*/

#include "defines.h"

/*! \brief Collects array of uncovered/covered lines from given functional unit. */
bool line_collect( char* funit_name, int funit_type, int cov, int** lines, int** excludes, int* line_cnt );

/*! \brief Returns hit and total information for specified functional unit. */
bool line_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit );

/*! \brief Collects array of functional unit names/types from the design. */
bool funit_get_list( char*** funit_names, char*** funit_types, int* funit_size );

/*! \brief Retrieves filename of given functional unit. */
char* funit_get_filename( const char* funit_name, int funit_type );

/*! \brief Retrieves starting and ending line numbers of the specified functional unit. */
bool funit_get_start_and_end_lines( const char* funit_name, int funit_type, int* start_line, int* end_line );

/*
 $Log$
 Revision 1.7  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.6  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.5  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.4  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.3  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.2  2003/12/01 23:27:16  phase1geo
 Adding code for retrieving line summary module coverage information for
 GUI.

 Revision 1.1  2003/11/24 17:48:56  phase1geo
 Adding gui.c/.h files for functions related to the GUI interface.  Updated
 Makefile.am for the inclusion of these files.

*/

#endif

