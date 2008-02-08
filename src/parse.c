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
 \file     parse.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
*/

#include <stdio.h>
#include <stdlib.h>

#include "binding.h"
#include "db.h"
#include "defines.h"
#include "fsm_var.h"
#include "info.h"
#include "link.h"
#include "lxt.h"
#include "parse.h"
#include "parser_misc.h"
#include "race.h"
#include "sim.h"
#include "stmt_blk.h"
#include "util.h"
#include "vcd.h"


extern void reset_lexer( str_link* file_list_head );
extern int VLparse();

extern str_link* use_files_head;
extern str_link* modlist_head;
extern str_link* modlist_tail;
extern char      user_msg[USER_MSG_LENGTH];
extern isuppl    info_suppl;
extern bool      flag_check_races;
extern sig_range curr_prange;
extern sig_range curr_urange;
extern bool      instance_specified;
extern char*     top_module;

/*!
 \param file  Pointer to file to read
 \param line  Read line from specified file.
 \param size  Maximum number of characters that can be stored in line.
 \return Returns the number of characters read from this line.

 Reads from specified file, one character at a time, until either a newline
 or EOF is encountered.  Returns the read line and the number of characters
 stored in the line.  No newline character is added to the line.
*/
int parse_readline( FILE* file, char* line, int size ) { PROFILE(PARSE_READLINE);

  int i = 0;  /* Loop iterator */

  while( (i < (size - 1)) && !feof( file ) && ((line[i] = fgetc( file )) != '\n') ) {
    i++;
  }

  if( !feof( file ) ) {
    line[i] = '\0';
  }

  if( i == size ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Line too long.  Must be less than %d characters.", size );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
  }

  return( !feof( file ) );

}

/*!
 \param top        Name of top-level module to score
 \param output_db  Name of output directory for generated scored files.

 Resets the lexer and parses all Verilog files specified in use_files list.
 After all design files are parsed, their information will be appropriately
 stored in the associated lists.
*/
void parse_design( char* top, char* output_db ) { PROFILE(PARSE_DESIGN);

  Try {

    (void)str_link_add( strdup_safe( top ), &modlist_head, &modlist_tail );

    if( use_files_head != NULL ) {

      /* Initialize lexer with first file */
      reset_lexer( use_files_head );

      /* Starting parser */
      if( (VLparse() != 0) || (error_count > 0) ) {
        print_output( "Error in parsing design", FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

      /* Deallocate any memory in curr_range variable */
      parser_dealloc_sig_range( &curr_urange, FALSE );
      parser_dealloc_sig_range( &curr_prange, FALSE );

#ifdef DEBUG_MODE
      print_output( "========  Completed design parsing  ========\n", DEBUG, __FILE__, __LINE__ );
#endif

      /* Check to make sure that the -t and -i options were specified correctly */
      if( db_check_for_top_module() ) {
        if( instance_specified ) {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Module specified with -t option (%s) is a top-level module.", top_module );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          print_output( "The -i option should not have been specified", FATAL_WRAP, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        if( !instance_specified ) {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Module specified with -t option (%s) is not a top-level module.", top_module );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          print_output( "The -i option must be specified to provide the hierarchy to the module specified with", FATAL_WRAP, __FILE__, __LINE__ );
          print_output( "the -t option.", FATAL_WRAP, __FILE__, __LINE__ );
          Throw 0;
        }
      }

      /* Perform all signal/expression binding */
      bind_perform( FALSE, 0 );
      fsm_var_bind();
  
      /* Perform race condition checking */
      if( flag_check_races ) {
        print_output( "\nChecking for race conditions...", NORMAL, __FILE__, __LINE__ );
        race_check_modules();
      } else {
        print_output( "The -rI option was specified in the command-line, causing Covered to skip race condition", WARNING, __FILE__, __LINE__ );
        print_output( "checking; therefore, coverage information may not be accurate if actual race conditions", WARNING_WRAP, __FILE__, __LINE__ );
        print_output( "do exist.  Proceed at your own risk!", WARNING_WRAP, __FILE__, __LINE__ );
      }

      /* Remove all statement blocks that cannot be considered for coverage */
      stmt_blk_remove();

#ifdef DEBUG_MODE
      print_output( "========  Completed race condition checking  ========\n", DEBUG, __FILE__, __LINE__ );
#endif

    } else {

      print_output( "No Verilog input files specified", FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

    /* Write contents to baseline database file. */
    db_write( output_db, TRUE, FALSE );

  } Catch_anonymous {
    sim_dealloc();
    db_close();
    Throw 0;
  }

  /* Deallocate simulator stuff */
  sim_dealloc();

  /* Close database */
  db_close();

#ifdef DEBUG_MODE
  {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "========  Design written to database %s successfully  ========\n\n", output_db );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

}

/*!
 \param db         Name of output database file to generate.
 \param dump_file  Name of dumpfile to parse for scoring.
 \param dump_mode  Type of dumpfile being used (see \ref dumpfile_fmt for legal values)

 Reads in specified CDD database file, reads in specified dumpfile in the specified format,
 performs re-simulation and writes the scored design back to the specified CDD database file
 for merging or reporting.
*/
void parse_and_score_dumpfile( char* db, char* dump_file, int dump_mode ) { PROFILE(PARSE_AND_SCORE_DUMPFILE);

  assert( dump_file != NULL );

  Try {

#ifdef DEBUG_MODE
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "========  Reading in database %s  ========\n", db );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Initialize all global information variables */
    info_initialize();

    /* Read in contents of specified database file */
    db_read( db, READ_MODE_MERGE_NO_MERGE );
  
    /* Bind expressions to signals/functional units */
    bind_perform( TRUE, 0 );

    /* Add static values to simulator */
    sim_initialize();

#ifdef DEBUG_MODE
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "========  Reading in VCD dumpfile %s  ========\n", dump_file );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif
  
    /* Perform the parse */
    switch( dump_mode ) {
      case DUMP_FMT_VCD :  vcd_parse( dump_file );  break;
      case DUMP_FMT_LXT :  lxt_parse( dump_file );  break;
      default           :  assert( (dump_mode == DUMP_FMT_VCD) || (dump_mode == DUMP_FMT_LXT) );
    }
    
    /* Flush any pending statement trees that are waiting for delay */
    db_do_timestep( 0, TRUE );

#ifdef DEBUG_MODE
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "========  Writing database %s  ========\n", db );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Indicate that this CDD contains scored information */
    info_suppl.part.scored = 1;

    /* Write contents to database file */
    db_write( db, FALSE, FALSE );

  } Catch_anonymous {
    sim_dealloc();
    Throw 0;
  }

  /* Deallocate simulator stuff */
  sim_dealloc();

}

/*
 $Log$
 Revision 1.55  2008/01/16 23:10:31  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.54  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.53  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.52  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.51  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.50  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.49  2007/04/10 03:56:18  phase1geo
 Completing majority of code to support new simulation core.  Starting to debug
 this though we still have quite a ways to go here.  Checkpointing.

 Revision 1.48  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.47  2006/11/03 23:36:36  phase1geo
 Fixing bug 1590104.  Updating regressions per this change.

 Revision 1.46  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.45  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.44  2006/08/06 05:02:59  phase1geo
 Documenting and adding warning message to parse.c for the -rI option.

 Revision 1.43  2006/08/06 04:36:20  phase1geo
 Fixing bugs 1533896 and 1533827.  Also added -rI option that will ignore
 the race condition check altogether (has not been verified to this point, however).

 Revision 1.42  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.41  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.40  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.39  2006/04/07 03:47:50  phase1geo
 Fixing run-time issues with VPI.  Things are running correctly now with IV.

 Revision 1.38  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.37  2006/02/01 19:58:28  phase1geo
 More updates to allow parsing of various parameter formats.  At this point
 I believe full parameter support is functional.  Regression has been updated
 which now completely passes.  A few new diagnostics have been added to the
 testsuite to verify additional functionality that is supported.

 Revision 1.36  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

 Revision 1.35  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.34  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

 Revision 1.33  2005/12/19 05:18:24  phase1geo
 Fixing memory leak problems with instance1.1.  Full regression has some segfaults
 that need to be looked at now.

 Revision 1.32  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.31  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.30  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.29  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.28  2005/02/01 05:11:18  phase1geo
 Updates to race condition checker to find blocking/non-blocking assignments in
 statement block.  Regression still runs clean.

 Revision 1.27  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.26  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.25  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.24  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.23  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.22  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.21  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.20  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.19  2003/08/10 00:05:16  phase1geo
 Fixing bug with posedge, negedge and anyedge expressions such that these expressions
 must be armed before they are able to be evaluated.  Fixing bug in vector compare function
 to cause compare to occur on smallest vector size (rather than on largest).  Updated regression
 files and added new diagnostics to test event fix.

 Revision 1.18  2003/02/18 20:17:02  phase1geo
 Making use of scored flag in CDD file.  Causing report command to exit early
 if it is working on a CDD file which has not been scored.  Updated testsuite
 for these changes.

 Revision 1.17  2003/01/03 05:48:04  phase1geo
 Reorganizing file opening/closing code in lexer.l and pplexer.l to make some
 sense out of the madness.

 Revision 1.16  2002/12/01 06:37:52  phase1geo
 Adding appropriate error output and causing score command to proper exit
 if parser errors are found.

 Revision 1.15  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.14  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.13  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.12  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.11  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.10  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.9  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.8  2002/07/20 21:34:58  phase1geo
 Separating ability to parse design and score dumpfile.  Now both or either
 can be done (allowing one to parse once and score multiple times).

 Revision 1.7  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.6  2002/07/13 04:09:18  phase1geo
 Adding support for correct implementation of `ifdef, `else, `endif
 directives.  Full regression passes.

 Revision 1.5  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.4  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

