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


/*! \brief Calculates combination logic statistics for summary output */
void combination_get_stats( exp_link* expl, float* total, int* hit );

/*! \brief Generates report output for combinational logic coverage. */
void combination_report( FILE* ofile, bool verbose );


/*
 $Log$
 Revision 1.6  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.5  2002/10/31 23:13:23  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

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
