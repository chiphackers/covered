/*!
 \file     main.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/26/2001
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "devel_doc.h"
#include "defines.h"
#include "score.h"
#include "merge.h"
#include "report.h"

/*!
 Displays usage information about this utility.
*/
void usage() {

  printf( "\n" );
  printf( "Usage:\n" );
  printf( "\n" );
  printf( "  General Covered information:\n" );
  printf( "    covered -v       Display current Covered version\n" );
  printf( "    covered -h       Display this usage information\n" );
  printf( "\n" );
  printf( "  To generate coverage database:\n" );
  printf( "    covered score -t <top-level_module_name> -vcd <dumpfile> [<options>]\n" );
  printf( "\n" );
  printf( "    Options:\n" );
  printf( "      -o <database_filename>  Name of database to write coverage information to.\n" );
  printf( "      -I <directory>          Directory to find included Verilog files\n" );
  printf( "      -f <filename>           Name of file containing additional arguments to parse\n" );
  printf( "      -y <directory>          Directory to find unspecified Verilog files\n" );
  printf( "      -v <filename>           Name of specific Verilog file to score\n" );
  printf( "      -e <module_name>        Name of module to not score\n" );
  printf( "      -q                      Suppresses output to standard output\n" );
  printf( "\n" );
  printf( "      +libext+.<extension>(+.<extension>)+\n" );
  printf( "                              Extensions of Verilog files to allow in scoring\n" );
  printf( "\n" );
  printf( "    Note:\n" );
  printf( "      The top-level module specifies the module to begin scoring.  All\n" );
  printf( "      modules beneath this module in the hierarchy will also be scored\n" );
  printf( "      unless these modules are explicitly stated to not be scored using\n" );
  printf( "      the -e flag.\n" );
  printf( "\n" );
  printf( "  To merge a database with an existing database:\n" );
  printf( "    covered merge [<options>] <existing_database> <database_to_merge>\n" );
  printf( "\n" );
  printf( "    Options:\n" );
  printf( "      -o <filename>           File to output new database to.  If this argument is not\n" );
  printf( "                              specified, the <existing_database> is used as the output\n" );
  printf( "                              database name.\n" );
  printf( "\n" );
  printf( "  To generate a coverage report:\n" );
  printf( "    covered report [<options>] <database_file>\n" );
  printf( "\n" );
  printf( "    Options:\n" );
  printf( "      -m [l][t][c][f]         Type(s) of metrics to report.  Default is ltc.\n" );
  printf( "      -v                      Provide verbose information in report.  Default is summarize.\n" );
  printf( "      -i                      Provides coverage information for instances instead of module.\n" );
  printf( "      -o <filename>           File to output report information to.  Default is standard output.\n" );
  printf( "\n" );

}

/*!
 \param argc Number of arguments specified in argv parameter list.
 \param argv List of arguments passed to this process from the command-line.
 \return Returns 0 to indicate a successful return; otherwise, returns a non-zero value.

 Main function for the Covered utility.  Parses command-line arguments and calls
 the appropriate functions.
*/
int main( int argc, char** argv ) {

  int retval     = 0;    /* Return value of this utility */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );

  if( argc == 1 ) {

    print_output( "Must specify a command (score, merge, report, -v, or -h)", FATAL );
    retval = -1;

  } else {

    if( strncmp( "-v", argv[1], 2 ) == 0 ) {

      /* Display version of Covered */
      print_output( COVERED_VERSION, NORMAL );

    } else if( strncmp( "-h", argv[1], 2 ) == 0 ) {

      usage();

    } else if( strncmp( "score", argv[1], 5 ) == 0 ) {

      printf( COVERED_HEADER );
      retval = command_score( argc, argv );

    } else if( strncmp( "merge", argv[1], 5 ) == 0 ) {

      printf( COVERED_HEADER );
      retval = command_merge( argc, argv );

    } else if( strncmp( "report", argv[1], 6 ) == 0 ) {

      printf( COVERED_HEADER );
      retval = command_report( argc, argv );

    } else {

      print_output( "Unknown command.  Please see \"covered -h\" for more usage.", FATAL );
      retval = -1;

    }

  }

  return( retval );

}

