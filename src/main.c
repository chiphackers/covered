/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     main.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/26/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "devel_doc.h"
#include "defines.h"
#include "score.h"
#include "merge.h"
#include "report.h"
#include "util.h"
#include "obfuscate.h"


extern char  user_msg[USER_MSG_LENGTH];
extern char* ppfilename;


/*!
 Displays usage information about this utility.
*/
void usage() {

  printf( "\n" );
#ifdef DEBUG_MODE
  printf( "Usage:  covered (-h | -v | (-D | -Q) (-B) <command> <command_options>))\n" );
#else
  printf( "Usage:  covered (-h | -v | (-Q) (-B) <command> <command_options>))\n" );
#endif
  printf( "\n" );
  printf( "   Options:\n" );
#ifdef DEBUG_MODE
  printf( "      -D                      Debug.  Display information helpful for debugging tool problems\n" );
#endif
  printf( "      -Q                      Quiet mode.  Causes all output to be suppressed\n" );
  printf( "      -B                      Obfuscate.  Obfuscates design-sensitive names in all user-readable output\n" );
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
 Function called at the end of execution which takes care of cleaning up state and temporary files.
*/
void covered_cleanup( void ) {

  /* Remove temporary pre-processor file (if it still exists) */
  if( ppfilename != NULL ) {
    unlink( ppfilename );
    free_safe( ppfilename );
  }

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
  obfuscate_set_mode( FALSE );

  /* Setup function to be called at exit */
  assert( atexit( covered_cleanup ) == 0 );

  if( argc == 1 ) {

    print_output( "Must specify a command (score, merge, report, -v, or -h)", FATAL, __FILE__, __LINE__ );
    retval = -1;

  } else {

    if( strncmp( "-v", argv[1], 2 ) == 0 ) {

      /* Display version of Covered */
      print_output( COVERED_VERSION, NORMAL, __FILE__, __LINE__ );

    } else if( strncmp( "-h", argv[1], 2 ) == 0 ) {

      usage();

    } else {

      do {

        if( strncmp( "-Q", argv[curr_arg], 2 ) == 0 ) {

          set_output_suppression( TRUE );

        } else if( strncmp( "-D", argv[curr_arg], 2 ) == 0 ) {

#ifdef DEBUG_MODE
          set_debug( TRUE );
#else
          print_output( "Global command -D can only be used when Covered is configured with the --enable-debug flag when being built", FATAL, __FILE__, __LINE__ );
#endif

        } else if( strncmp( "-B", argv[curr_arg], 2 ) == 0 ) {

          obfuscate_set_mode( TRUE );

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

          snprintf( user_msg, USER_MSG_LENGTH, "Unknown command/global option \"%s\".  Please see \"covered -h\" for usage.", argv[curr_arg] );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          exit( 1 );

        }

        curr_arg++;

      } while( (curr_arg < argc) && !cmd_found );

      if( !cmd_found ) {
 
        print_output( "Must specify a command (score, merge, report, -v, or -h)", FATAL, __FILE__, __LINE__ );
        retval = -1;

      }

    }

  }

  /* Deallocate obfuscation tree */
  obfuscate_dealloc();

  return( retval );

}

/*
 $Log$
 Revision 1.17  2006/08/18 22:19:54  phase1geo
 Fully integrated obfuscation into the development release.

 Revision 1.16  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.15  2006/01/09 18:58:15  phase1geo
 Updating regression for VCS runs.  Added cleanup function at exit to remove the
 tmp* file (if it exists) regardless of the internal state of Covered at the time
 of exit (removes the need for the user to remove this file when things go awry).
 Documentation updates for this feature.

 Revision 1.14  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.13  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.12  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.11  2002/11/27 15:53:50  phase1geo
 Checkins for next release (20021127).  Updates to documentation, configuration
 files and NEWS.

 Revision 1.10  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.9  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

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

