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
  printf( "Usage:  covered (-h | -v | <command> <command_options>)\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -v                      Display current Covered version\n" );
  printf( "      -h                      Display this usage information\n" );
  printf( "\n" );
  printf( "   Commands:\n" );
  printf( "      score                   Parses Verilog files and VCD dumpfiles to create database file used\n" );
  printf( "                                for merging and reporting.\n" );
  printf( "      merge                   Merges two database files into one.\n" );
  printf( "      report                  Generates human-readable coverage reports from database file.\n" );
  printf( "\n" );
  printf( "   For individual help information for each of the above commands, enter:\n" );
  printf( "      covered <command> -h\n" );
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

      retval = command_score( argc, argv );

    } else if( strncmp( "merge", argv[1], 5 ) == 0 ) {

      retval = command_merge( argc, argv );

    } else if( strncmp( "report", argv[1], 6 ) == 0 ) {

      retval = command_report( argc, argv );

    } else {

      print_output( "Unknown command.  Please see \"covered -h\" for more usage.", FATAL );
      retval = -1;

    }

  }

  return( retval );

}

