#ifndef __RACE_H__
#define __RACE_H__

/*!
 \file     race.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/15/2004
 \brief    Contains functions used to check for race conditions and proper signal use
           within the specified design.
*/

#include "defines.h"


/*! \brief Adds specified input port signal to stmt_sig list */
void race_add_inport_sig( vsignal* sig );

/*! \brief Parses entire statement tree pointed to by stmt, adding all found signals within it to the stmt_sig list */
void race_find_and_add_stmt_sigs( statement* stmt, statement* root );

/*! \brief Displays contents of stmt_sig array to standard output */
void race_stmt_blk_display();

/*! \brief Checks the current module for race conditions */
void race_check_module();

/*! \brief Checks the number of race conditions that were detected in the design */
bool race_check_race_count();

/*! \brief Deallocates specified stmt_sig structure from memory */
void race_stmt_blk_dealloc();


/*
 $Log$
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

