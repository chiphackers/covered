/*!
 \file     comb.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#include <stdio.h>

#include "comb.h"


/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 \param instance  Specifies to report by instance or module.

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the combinational logic coverage for each module encountered.  The parent 
 module will specify its own combinational logic coverage along with a total combinational
 logic coverage including its children.
*/
void combination_report( FILE* ofile, bool verbose, bool instance ) {

  fprintf( ofile, "COMBINATIONAL LOGIC COVERAGE RESULTS\n" );
  fprintf( ofile, "------------------------------------\n" );



}
