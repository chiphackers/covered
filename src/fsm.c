/*!
 \file     fsm.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 
 \par How are FSMs handled?
 In some fashion (either by manual input or automatic FSM extraction), an FSM state
 variable is named.  When the parser parses a module that contains this state variable,
 the size of the state variable is used to construct a two-dimensional state transition
 table which is the size of the state variable squared.  Each element of the table is
 one bit in size.  If the bit is set in an element, it is known that the state variable
 transitioned from row value to column value.  The table can be output to the CDD file
 in any way that uses the least amount of space.
 
 \par What information can be extracted from an FSM?
 Because of the history saving nature of the FSM table, at least two basic statistics
 can be drawn from it.  The first is basically, "Which states did the state machine get
 to?".  This information can be calculated by parsing the table for set bits.  When a set
 bit is found, both the row and column values are reported as achieved states.
 
 \par
 The second statistic that can be drawn from a state machine table are state transitions.
 This statistic answers the question, "Which state transition arcs in the state transition
 diagram were traversed?".  The table format is formulated to specifically calculate the
 answer to this question.  When a bit is found to be set in the table, we know which
 state (row) transitioned to which other state (column).
 
 \par What is contained in this file?
 This file contains the functions necessary to perform the following:
 -# Create the required FSM table and attach it to a signal
 -# Set the appropriate bit in the table during simulation
 -# Write/read an FSM to/from the CDD file
 -# Generate FSM report output
*/

#include <stdio.h>

#include "fsm.h"


extern bool report_instance;


/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 
 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the FSM coverage for each module encountered.  The parent module will
 specify its own FSM coverage along with a total FSM coverage including its 
 children.
*/
void fsm_report( FILE* ofile, bool verbose ) {

  fprintf( ofile, "FSM COVERAGE RESULTS\n" );
  fprintf( ofile, "--------------------\n" );
  fprintf( ofile, "Not available at this time.\n\n" );

}

/*
 $Log$
 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

