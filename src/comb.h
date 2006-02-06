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


/*! \brief Resets combination counted bits in expression list */
void combination_reset_counted_exprs( exp_link* expl );

/*! \brief Calculates combination logic statistics for summary output */
void combination_get_stats( exp_link* expl, float* total, int* hit );

/*! \brief Collects all toggle expressions that match the specified coverage indication. */
bool combination_collect( char* funit_name, int funit_type, expression*** covs, int* cov_cnt, expression*** uncovs, int* uncov_cnt );

/*! \brief Gets combinational logic summary statistics for specified module. */
bool combination_get_module_summary( char* funit_name, int funit_type, int* total, int* hit );

/*! \brief Gets output for specified expression including underlines and code */
bool combination_get_expression( char* funit_name, int funit_type, int expr_id, char*** code, int** uline_groups, int* code_size, char*** ulines, int* uline_size );

/*! \brief Gets output for specified expression including coverage information */
bool combination_get_coverage( char* funit_name, int funit_type, int exp_id, int uline_id, char*** info, int* info_size );

/*! \brief Generates report output for combinational logic coverage. */
void combination_report( FILE* ofile, bool verbose );


/*
 $Log$
 Revision 1.13  2005/11/10 19:28:22  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.12  2004/09/07 03:17:13  phase1geo
 Fixing bug that did not allow combinational logic to be revisited in GUI properly.
 Also removing comments from bgerror function in Tcl code.

 Revision 1.11  2004/08/17 15:23:37  phase1geo
 Added combinational logic coverage output to GUI.  Modified comb.c code to get this
 to work that impacts ASCII coverage output; however, regression is fully passing with
 these changes.  Combinational coverage for GUI is mostly complete regarding information
 and usability.  Possibly some cleanup in output and in code is needed.

 Revision 1.10  2004/08/17 04:43:57  phase1geo
 Updating unary and binary combinational expression output functions to create
 string arrays instead of immediately sending the information to standard output
 to support GUI interface as well as ASCII interface.

 Revision 1.9  2004/08/13 20:45:05  phase1geo
 More added for combinational logic verbose reporting.  At this point, the
 code is being output with underlines that can move up and down the expression
 tree.  No expression reporting is being done at this time, however.

 Revision 1.8  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

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
