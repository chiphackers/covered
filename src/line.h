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

/*! \brief Calculates line coverage numbers for the specified expression list. */
void line_get_stats( stmt_link* stmtl, float* total, int* hit );

/*! \brief Gathers line numbers from specified module that were not hit during simulation. */
bool line_collect( const char* mod_name, int cov, int** lines, int* line_cnt );

/*! \brief Generates report output for line coverage. */
void line_report( FILE* ofile, bool verbose );


/*
 $Log$
 Revision 1.9  2003/11/22 20:44:58  phase1geo
 Adding function to get array of missed line numbers for GUI purposes.  Updates
 to report command for getting information ready when running the GUI.

 Revision 1.8  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.7  2002/10/31 23:13:55  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.4  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.3  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).
*/

#endif

