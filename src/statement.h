#ifndef __STATEMENT_H__
#define __STATEMENT_H__

/*!
 \file     statement.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     5/1/2002
 \brief    Contains functions to create, manipulate and deallocate statements.
*/

#include "defines.h"


/*! \brief Creates new statement structure. */
statement* statement_create( expression* exp, sig_link* head, sig_link* tail );

/*! \brief Writes specified statement to the specified output file. */
void statement_db_write( statement* stmt, FILE* ofile, char* scope );

/*! \brief Reads in statement line from specified string and stores statement in specified module. */
bool statement_db_read( char** line, module* curr_mod, int read_mode );

/*! \brief Connects statement sequence to next statement. */
void statement_connect( statement* curr_stmt, statement* next_stmt );

/*! \brief Sets stop bits in specified statement tree. */
void statement_set_stop( statement* stmt, statement* post, bool true_path, bool both );

/*! \brief Recursively deallocates specified statement tree. */
void statement_dealloc_recursive( statement* stmt );

/*! \brief Deallocates statement memory and associated expression tree from the heap. */
void statement_dealloc( statement* stmt );


/*
 $Log$
 Revision 1.13  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.12  2002/10/31 23:14:27  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.11  2002/10/30 06:07:11  phase1geo
 First attempt to handle expression trees/statement trees that contain
 unsupported code.  These are removed completely and not evaluated (because
 we can't guarantee that our results will match the simulator).  Added real1.1.v
 diagnostic that verifies one case of this scenario.  Full regression passes.

 Revision 1.10  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.9  2002/07/03 21:30:53  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.8  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.7  2002/06/30 22:23:20  phase1geo
 Working on fixing looping in parser.  Statement connector needs to be revamped.

 Revision 1.6  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.5  2002/06/27 12:36:47  phase1geo
 Fixing bugs with scoring.  I think I got it this time.

 Revision 1.4  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.3  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.2  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.1  2002/05/02 03:27:42  phase1geo
 Initial creation of statement structure and manipulation files.  Internals are
 still in a chaotic state.
*/

#endif

