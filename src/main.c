/*!
 \file     main.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/26/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
  printf( "Usage:  covered (-h | -v | (-D) (-Q) <command> <command_options>))\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -D                      Debug.  Display information helpful for debugging tool problems\n" );
  printf( "      -Q                      Quiet mode.  Causes all output to be suppressed\n" );
  printf( "      -v                      Version.  Display current Covered version\n" );
  printf( "      -h                      Help.  Display this usage information\n" );
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

  int  retval    = 0;      /* Return value of this utility               */
  int  curr_arg  = 1;      /* Current position in argument list          */
  bool cmd_found = FALSE;  /* Set to TRUE when command found in arg list */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );
  set_debug( FALSE );

  if( argc == 1 ) {

    print_output( "Must specify a command (score, merge, report, -v, or -h)", FATAL );
    retval = -1;

  } else {

    if( strncmp( "-v", argv[1], 2 ) == 0 ) {

      /* Display version of Covered */
      print_output( COVERED_VERSION, NORMAL );

    } else if( strncmp( "-h", argv[1], 2 ) == 0 ) {

      usage();

    } else {

      do {

        if( strncmp( "-D", argv[curr_arg], 2 ) == 0 ) {

          set_debug( TRUE );

        } else if( strncmp( "-Q", argv[curr_arg], 2 ) == 0 ) {

          set_output_suppression( TRUE );

        } else if( strncmp( "score", argv[curr_arg], 5 ) == 0 ) {

          retval    = command_score( argc, curr_arg, argv );
          cmd_found = TRUE;

        } else if( strncmp( "merge", argv[curr_arg], 5 ) == 0 ) {

          retval    = command_merge( argc, curr_arg, argv );
          cmd_found = TRUE;

        } else if( strncmp( "report", argv[curr_arg], 6 ) == 0 ) {

          retval    = command_report( argc, curr_arg, argv );
          cmd_found = TRUE;

        } else {

          print_output( "Unknown command.  Please see \"covered -h\" for usage.", FATAL );
          retval = -1;

        }

        curr_arg++;

      } while( (curr_arg < argc) && !cmd_found );

      if( !cmd_found ) {
 
        print_output( "Must specify a command (score, merge, report, -v, or -h)", FATAL );
        retval = -1;

      }

    }

  }

  return( retval );

}

/*
 $Log$
 Revision 1.8  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.7  2002/07/11 19:12:38  phase1geo
 Fixing version number.  Fixing bug with score command if -t option was not
 specified to avoid a segmentation fault.

 Revision 1.6  2002/07/09 05:02:42  phase1geo
 Updating user documentation to reflect new command-line options.

 Revision 1.5  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.4  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

