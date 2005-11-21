#ifndef __RACE_H__
#define __RACE_H__

/*!
 \file     race.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/15/2004
 \brief    Contains functions used to check for race conditions and proper signal use
           within the specified design.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Checks the current module for race conditions */
void race_check_modules();

/*! \brief Writes contents of specified race condition block to specified file output */
bool race_db_write( race_blk* head, FILE* file );

/*! \brief Reads contents from specified line for a race condition block and assigns the new block to the curr_mod */
bool race_db_read( char** line, func_unit* curr_mod );

/*! \brief Get statistic information for the specified race condition block list */
void race_get_stats( race_blk* curr, int* race_total, int type_total[][RACE_TYPE_NUM] );

/*! \brief Displays report information for race condition blocks in design */
void race_report( FILE* ofile, bool verbose );

/*! \brief Collects all of the lines in the specified module that were not verified due to race condition breach */
bool race_collect_lines( char* funit_name, int funit_type, int** lines, int** reasons, int* line_cnt );

/*! \brief Deallocates the specified race condition block from memory */
void race_blk_delete_list( race_blk* rb );


/*
 $Log$
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
