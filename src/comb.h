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

/*! \brief Collects all toggle expressions that match the specified coverage indication. */
bool combination_collect( const char* mod_name, expression*** covs, int* cov_cnt, expression*** uncovs, int* uncov_cnt );

// bool toggle_get_coverage( char* mod_name, char* sig_name, int* msb, int* lsb, char** tog01, char** tog10 );

bool combination_get_module_summary( char* mod_name, int* total, int* hit );

/*! \brief Generates report output for combinational logic coverage. */
void combination_report( FILE* ofile, bool verbose );


/*
 $Log$
 Revision 1.7  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

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
