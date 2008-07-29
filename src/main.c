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
#include "merge.h"
#include "obfuscate.h"
#include "profiler.h"
#include "rank.h"
#include "report.h"
#include "score.h"
#include "util.h"


/*!
 Exception context structure used by cexcept.h for throwing and catching exceptions.
*/
struct exception_context the_exception_context[1];


extern char  user_msg[USER_MSG_LENGTH];
extern char* ppfilename;
extern int64 curr_malloc_size;
extern bool  test_mode;


/*!
 Displays usage information about this utility.
*/
static void usage() {

  printf( "\n" );
#ifdef DEBUG_MODE
#ifdef PROFILER
  printf( "Usage:  covered (-h | -v | (-D | -Q) (-P [<file>]) (-B) <command> <command_options>))\n" );
#else
  printf( "Usage:  covered (-h | -v | (-D | -Q) (-B) <command> <command_options>))\n" );
#endif
#else
#ifdef PROFILER
  printf( "Usage:  covered (-h | -v | (-Q) (-P [<file>]) (-B) <command> <command_options>))\n" );
#else
  printf( "Usage:  covered (-h | -v | (-Q) (-B) <command> <command_options>))\n" );
#endif
#endif
  printf( "\n" );
  printf( "   Options:\n" );
#ifdef DEBUG_MODE
  printf( "      -D                      Debug.  Display information helpful for debugging tool problems\n" );
#endif
#ifdef PROFILER
  printf( "      -P [<file>]             Profile.  Generate profiling information file from command.  Default output file is covered.prof\n" );
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
  printf( "      rank                    Generates ranked list of CDD files to run for optimal coverage in a regression run.\n" );
  printf( "\n" );
  printf( "   For individual help information for each of the above commands, enter:\n" );
  printf( "      covered <command> -h\n" );
  printf( "\n" );

}

/*!
 \param argc Number of arguments specified in argv parameter list.
 \param argv List of arguments passed to this process from the command-line.

 \return Returns EXIT_SUCCESS to indicate a successful return; otherwise, returns EXIT_FAILURE.

 Main function for the Covered utility.  Parses command-line arguments and calls
 the appropriate functions.
*/
int main( int argc, const char** argv ) {

  int  retval    = EXIT_SUCCESS;  /* Return value of this utility */
  int  curr_arg  = 1;             /* Current position in argument list */
  bool cmd_found = FALSE;         /* Set to TRUE when command found in arg list */

  /* Initialize the exception context */
  init_exception_context( the_exception_context );

  /* Initialize error suppression value */
  set_output_suppression( FALSE );
  set_debug( FALSE );
#ifdef TESTMODE
  set_testmode();
#endif
  obfuscate_set_mode( FALSE );
  profiler_set_mode( FALSE );

  Try {

    if( argc == 1 ) {

      print_output( "Must specify a command (score, merge, report, rank, -v, or -h)", FATAL, __FILE__, __LINE__ );
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
            print_output( "Global command -D can only be used when Covered is configured with the --enable-debug flag when being built", WARNING, __FILE__, __LINE__ );
#endif

          } else if( strncmp( "-P", argv[curr_arg], 2 ) == 0 ) {

#ifdef PROFILER
            profiler_set_mode( TRUE );
            curr_arg++;
            if( (curr_arg < argc) && (argv[curr_arg][0] != '-') &&
                (strncmp( "score",  argv[curr_arg], 5 ) != 0) &&
                (strncmp( "merge",  argv[curr_arg], 5 ) != 0) &&
                (strncmp( "report", argv[curr_arg], 6 ) != 0)) {
              profiler_set_filename( argv[curr_arg] );
            } else {
              curr_arg--;
              profiler_set_filename( PROFILING_OUTPUT_NAME );
            }
#else
            print_output( "Global command -P can only be used when Covered is configured with the --enable-profiling flag when being built", WARNING, __FILE__, __LINE__ );
#endif

          } else if( strncmp( "-B", argv[curr_arg], 2 ) == 0 ) {

            obfuscate_set_mode( TRUE );

          } else if( strncmp( "score", argv[curr_arg], 5 ) == 0 ) {

            command_score( argc, curr_arg, argv );
            cmd_found = TRUE;

          } else if( strncmp( "merge", argv[curr_arg], 5 ) == 0 ) {

            command_merge( argc, curr_arg, argv );
            cmd_found = TRUE;

          } else if( strncmp( "report", argv[curr_arg], 6 ) == 0 ) {

            command_report( argc, curr_arg, argv );
            cmd_found = TRUE;

          } else if( strncmp( "rank", argv[curr_arg], 4 ) == 0 ) {

            command_rank( argc, curr_arg, argv );
            cmd_found = TRUE;

          } else {

            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown command/global option \"%s\".  Please see \"covered -h\" for usage.", argv[curr_arg] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "main Throw A\n" );
            Throw 0;

          }

          curr_arg++;

        } while( (curr_arg < argc) && !cmd_found );

        if( !cmd_found ) {
 
          print_output( "Must specify a command (score, merge, report, rank, -v, or -h)", FATAL, __FILE__, __LINE__ );
          printf( "main Throw B\n" );
          Throw 0;

        }

      }

    }

  } Catch_anonymous {
    retval = EXIT_FAILURE;
  }

  /* Deallocate obfuscation tree */
  obfuscate_dealloc();

  /* Output profiling information, if necessary */
  profiler_report();

#ifdef TESTMODE
  /* Make sure that all of our allocate memory has been deallocated */
  if( test_mode ) {
    printf( "curr_malloc_size: %lld\n", curr_malloc_size );
    assert( curr_malloc_size == 0 );
  }
#endif

  return( retval );

}

/*
 $Log$
 Revision 1.35.2.2  2008/07/21 06:36:29  phase1geo
 Updating code from rank-devel-branch branch.

 Revision 1.35.2.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.36.2.1  2008/06/30 13:14:22  phase1geo
 Starting to work on new 'rank' command.  Checkpointing.

 Revision 1.36  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.35  2008/05/08 18:59:15  phase1geo
 Fixing bug 1950946.

 Revision 1.34  2008/04/08 23:58:17  phase1geo
 Fixing test mode code so that it works properly in regression and stand-alone
 runs.

 Revision 1.33  2008/04/01 23:08:21  phase1geo
 More updates for error diagnostic cleanup.  Full regression still not
 passing (but is getting close).

 Revision 1.32  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.31  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.30  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.29  2008/01/21 21:39:55  phase1geo
 Bug fix for bug 1876376.

 Revision 1.28  2008/01/17 04:35:12  phase1geo
 Updating regression per latest bug fixes and splint updates.

 Revision 1.27  2008/01/16 23:10:31  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.26  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.25  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.24  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.23  2007/12/20 04:47:50  phase1geo
 Fixing the last of the regression failures from previous changes.  Removing unnecessary
 output used for debugging.

 Revision 1.22  2007/12/12 07:53:00  phase1geo
 Separating debugging and profiling so that we can do profiling without all
 of the debug overhead.

 Revision 1.21  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.20  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.19  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.18  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

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

