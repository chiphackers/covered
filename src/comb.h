#ifndef __COMB_H__
#define __COMB_H__

/*!
 \file     comb.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting combinational logic coverage.
*/

#include <stdio.h>

#include "defines.h"

//! Generates report output for combinational logic coverage.
void combination_report( FILE* ofile, bool verbose, bool instance );

#endif
