#ifndef __LINE_H__
#define __LINE_H__

/*!
 \file     line.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting line coverage.
*/

#include <stdio.h>

#include "defines.h"

//! Calculates line coverage numbers for the specified expression list.
void line_get_stats( exp_link* expl, float* total, int* hit );

//! Generates report output for line coverage.
void line_report( FILE* ofile, bool verbose, bool instance );

#endif
