/*!
 \file     parse.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
*/

#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "parse.h"
#include "link.h"
#include "util.h"
#include "db.h"
#include "binding.h"
#include "vcd.h"
#include "fsm_var.h"
#include "info.h"
#include "sim.h"
#include "race.h"


extern void reset_lexer( str_link* file_list_head );
extern int VLparse();

extern str_link* use_files_head;

extern str_link* modlist_head;
extern str_link* modlist_tail;
extern char user_msg[USER_MSG_LENGTH];
extern int  error_count;
extern bool flag_scored;
extern bool flag_race_check;

/*!
 \param file  Pointer to file to read
 \param line  Read line from specified file.
 \param size  Maximum number of characters that can be stored in line.
 \return Returns the number of characters read from this line.

 Reads from specified file, one character at a time, until either a newline
 or EOF is encountered.  Returns the read line and the number of characters
 stored in the line.  No newline character is added to the line.
*/
int parse_readline( FILE* file, char* line, int size ) {

  int i = 0;  /* Loop iterator */

  while( (i < (size - 1)) && !feof( file ) && ((line[i] = fgetc( file )) != '\n') ) {
    i++;
  }

  if( !feof( file ) ) {
    line[i] = '\0';
  }

  if( i == size ) {
    snprintf( user_msg, USER_MSG_LENGTH, "Line too long.  Must be less than %d characters.", size );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
  }

  return( !feof( file ) );

}

/*!
 \param top        Name of top-level module to score
 \param output_db  Name of output directory for generated scored files.

 \return Returns TRUE if parsing is successful; otherwise, returns FALSE.

 Resets the lexer and parses all Verilog files specified in use_files list.
 After all design files are parsed, their information will be appropriately
 stored in the associated lists.
*/
bool parse_design( char* top, char* output_db ) {

  bool retval = TRUE;  /* Return value of this function */

  str_link_add( strdup_safe( top, __FILE__, __LINE__ ), &modlist_head, &modlist_tail );

  if( use_files_head != NULL ) {

    /* Initialize lexer with first file */
    reset_lexer( use_files_head );

    /* Starting parser */
    if( (VLparse() != 0) || (error_count > 0) ) {
      print_output( "Error in parsing design", FATAL, __FILE__, __LINE__ );
      exit( 1 );
    }

#ifdef DEBUG_MODE
    print_output( "========  Completed design parsing  ========\n", DEBUG, __FILE__, __LINE__ );
#endif

    /* Perform all signal/expression binding */
    bind( FALSE );
    fsm_var_bind();
  
    /* Perform race condition checking */
    print_output( "\nChecking for race conditions...", NORMAL, __FILE__, __LINE__ );
    race_check_modules();

    /* Remove all statement blocks that cannot be considered for coverage */
    stmt_blk_remove();

#ifdef DEBUG_MODE
    print_output( "========  Completed race condition checking  ========\n", DEBUG, __FILE__, __LINE__ );
#endif

  } else {

    print_output( "No Verilog input files specified", FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  /* Write contents to baseline database file. */
  if( !db_write( output_db, TRUE ) ) {
    print_output( "Unable to write database file", FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  /* Close database */
  db_close();

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "========  Design written to database %s successfully  ========\n\n", output_db );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  return( retval );

}

/*!
 \param db   Name of output database file to generate.
 \param vcd  Name of VCD file to parse for scoring.
 \return Returns TRUE if VCD parsing and scoring is successful; otherwise,
         returns FALSE.

*/
bool parse_and_score_dumpfile( char* db, char* vcd ) {

  bool retval = TRUE;  /* Return value of this function */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "========  Reading in database %s  ========\n", db );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Initialize all global information variables */
  info_initialize();

  /* Read in contents of specified database file */
  if( !db_read( db, READ_MODE_MERGE_NO_MERGE ) ) {
    print_output( "Unable to read database file", FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }
  
  /* Bind expressions to signals/functional units */
  bind( TRUE );

  sim_add_statics();

  /* Read in contents of VCD file */
  if( vcd == NULL ) {
    print_output( "VCD file not specified on command line", FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "========  Reading in VCD dumpfile %s  ========\n", vcd );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
  
  /* Perform initialization simulation timestep */
  db_do_timestep( 0 );

  /* Perform the parse */
  vcd_parse( vcd );
    
  /* Flush any pending statement trees that are waiting for delay */
  db_do_timestep( -1 );

  /* Remove all remaining threads */
  sim_kill_all_threads();

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "========  Writing database %s  ========\n", db );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Indicate that this CDD contains scored information */
  flag_scored = TRUE;

  /* Write contents to database file */
  if( !db_write( db, FALSE ) ) {
    print_output( "Unable to write database file", FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  return( retval );

}

/*
 $Log$
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

