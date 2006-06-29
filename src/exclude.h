#ifndef __EXCLUDE_H__
#define __EXCLUDE_H__

/*!
 \file     exclude.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/22/2006
 \brief    Contains functions for handling user-specified exclusion of coverage results.
*/

#include "defines.h"


/*! \brief Sets the excluded bit for all expressions in the given functional unit with the
           specified line number and recalculates the summary coverage information. */
bool exclude_set_line_exclude( char* funit_name, int funit_type, int line, int value );

/*! \brief Sets the excluded bit for the specified signal in the given functional unit and
           recalculates the summary coverage information. */
bool exclude_set_toggle_exclude( char* funit_name, int funit_type, char* sig_name, int value );

/*! \brief Sets the excluded bit for the specified expression in the given functional unit
           and recalculates the summary coverage information. */
bool exclude_set_comb_exclude( char* funit_name, int funit_type, int expr_id, int uline_id, int value );

/*! \brief Sets the excluded bit for the specified state transition in the given functional unit
           and recalculates the summary coverage information. */
bool exclude_set_fsm_exclude( char* funit_name, int funit_type, int expr_id, char* from_state, char* to_state, int value );

/*! \brief Sets the excluded bit for the specified expression in the given functional unit
           and recalculates the summary coverage information. */
bool exclude_set_assert_exclude( char* funit_name, int funit_type, char* inst_name, int expr_id, int value );


/*
 $Log$
 Revision 1.4  2006/06/26 22:49:00  phase1geo
 More updates for exclusion of combinational logic.  Also updates to properly
 support CDD saving; however, this change causes regression errors, currently.

 Revision 1.3  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.2  2006/06/23 04:03:30  phase1geo
 Updating build files and removing syntax errors in exclude.h and exclude.c
 (though this code doesn't do anything meaningful at this point).

 Revision 1.1  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

*/

#endif

