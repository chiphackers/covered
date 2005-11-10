#ifndef __GUI_H__
#define __GUI_H__

/*!
 \file     gui.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/24/2003
 \brief    Contains functions that are used by the GUI report viewer.
*/

#include "defines.h"

/*! \brief Collects array of uncovered/covered lines from given functional unit. */
bool line_collect( char* funit_name, int funit_type, int cov, int** lines, int* line_cnt );

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

