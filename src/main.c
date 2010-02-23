/*
 Copyright (c) 2006-2010 Trevor Williams

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
#include "exclude.h"
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
  printf( "Usage:  covered (-h | -v | (-D | -T | -Q) (-P [<file>]) (-B) <command> <command_options>))\n" );
#else
  printf( "Usage:  covered (-h | -v | (-D | -T | -Q) (-B) <command> <command_options>))\n" );
#endif
#else
#ifdef PROFILER
  printf( "Usage:  covered (-h | -v | (-T | -Q) (-P [<file>]) (-B) <command> <command_options>))\n" );
#else
  printf( "Usage:  covered (-h | -v | (-T | -Q) (-B) <command> <command_options>))\n" );
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
  printf( "      -T                      Terse mode.  Causes all output except for header information and warnings to be suppressed\n" );
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
  printf( "      exclude                 Excludes coverage points from a given CDD and saves the modified CDD for further commands.\n" );
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
  set_quiet( FALSE );
  set_terse( FALSE );
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
  
            set_quiet( TRUE );

          } else if( strncmp( "-T", argv[curr_arg], 2 ) == 0 ) {

            set_terse( TRUE );

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

          } else if( strncmp( "exclude", argv[curr_arg], 7 ) == 0 ) {

            command_exclude( argc, curr_arg, argv );
            cmd_found = TRUE;

          } else {

            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown command/global option \"%s\".  Please see \"covered -h\" for usage.", argv[curr_arg] );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;

          }

          curr_arg++;

        } while( (curr_arg < argc) && !cmd_found );

        if( !cmd_found ) {
 
          print_output( "Must specify a command (score, merge, report, rank, exclude, -v, or -h)", FATAL, __FILE__, __LINE__ );
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
    printf( "curr_malloc_size: %" FMT64 "d\n", curr_malloc_size );
    assert( curr_malloc_size == 0 );
  }
#endif

  return( retval );

}

