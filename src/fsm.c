/*!
 \file     fsm.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#include <stdio.h>

#include "fsm.h"


/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 \param instance  Specifies to report by instance or module.

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the FSM coverage for each module encountered.  The parent module will
 specify its own FSM coverage along with a total FSM coverage including its 
 children.
*/
void fsm_report( FILE* ofile, bool verbose, bool instance ) {

  fprintf( ofile, "FSM COVERAGE RESULTS\n" );
  fprintf( ofile, "--------------------\n" );
  fprintf( ofile, "Not available at this time.\n\n" );

}
