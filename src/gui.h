#ifndef __GUI_H__
#define __GUI_H__

/*!
 \file     gui.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/24/2003
 \brief    Contains functions that are used by the GUI report viewer.
*/

#include "defines.h"

/*! \brief Collects array of uncovered/covered lines from given module. */
bool line_collect( const char* mod_name, int cov, int** lines, int* line_cnt );

/*! \brief Returns hit and total information for specified module. */
bool line_get_module_summary( char* mod_name, int* total, int* hit );

/*! \brief Collects array of module names from the design. */
bool module_get_list( char*** mod_list, int* mod_size );

/*! \brief Retrieves filename of given module. */
char* module_get_filename( const char* mod_name );

/*! \brief Retrieves starting and ending line numbers of the specified module. */
bool module_get_start_and_end_lines( const char* mod_name, int* start_line, int* end_line );

/*
 $Log$
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

