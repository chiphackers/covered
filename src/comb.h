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
void combination_report( FILE* ofile, bool verbose );


/*
 $Log$
 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

#endif
