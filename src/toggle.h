#ifndef __TOGGLE_H__
#define __TOGGLE_H__

/*!
 \file     toggle.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting toggle coverage.
*/

#include <stdio.h>

#include "defines.h"


//! Calculates the toggle coverage for the specifed expression and signal lists.
void toggle_get_stats( exp_link* expl, sig_link* sigl, float* total, int* hit01, int* hit10 );

//! Generates report output for toggle coverage.
void toggle_report( FILE* ofile, bool verbose, bool instance );

#endif
