#ifndef __EXCLUDE_H__
#define __EXCLUDE_H__

/*!
 \file     exclude.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/22/2006
 \brief    Contains functions for handling user-specified exclusion of coverage results.
*/

#include "defines.h"


/*! \brief Sets the exclude bit of the specified expression and recalculates coverage */
void exclude_expr_assign_and_recalc( expression* expr, funit_inst* inst, bool excluded );

/*! \brief Sets the exclude bit of the specified signal and recalculates coverage */
void exclude_sig_assign_and_recalc( vsignal* sig, funit_inst* inst, bool excluded );

/*
 $Log$
 Revision 1.1  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

*/

#endif

