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
void toggle_report( FILE* ofile, bool verbose );

/* $Log$
/* Revision 1.2  2002/07/03 03:31:11  phase1geo
/* Adding RCS Log strings in files that were missing them so that file version
/* information is contained in every source and header file.  Reordering src
/* Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
/* */

#endif
