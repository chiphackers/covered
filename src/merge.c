/*!
 \file     merge.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/29/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>

#include "binding.h"
#include "db.h"
#include "defines.h"
#include "info.h"
#include "merge.h"
#include "sim.h"
#include "util.h"


extern int merged_code;

/*!
 Specifies the output filename of the CDD file that contains the merged data.
*/
static char* merged_file = NULL;

/*!
 Specifies the names of the input CDD files.  If the user does not specify an output
 CDD filename (i.e., no -o option is specified), the name of merge_in[0] will be used
 for merged_file.
*/
char** merge_in = NULL;

/*!
 Specifies the number of valid entries in the merge_in array.
*/
int merge_in_num = 0;

extern char user_msg[USER_MSG_LENGTH];

/*!
 Outputs usage informaiton to standard output for merge command.
*/
static void merge_usage() {

  printf( "\n" );
  printf( "Usage:  covered merge [<options>] <existing_database> <database_to_merge>+\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -o <filename>           File to output new database to.  If this argument is not\n" );
  printf( "                              specified, the <existing_database> is used as the output\n" );
  printf( "                              database name.\n" );
  printf( "      -h                      Displays this help information.\n" );
  printf( "\n" );

}

/*!
 \param argc      Number of arguments in argument list argv.
 \param last_arg  Index of last parsed argument from list.
 \param argv      Argument list passed to this program.

 \throws anonymous Throw Throw Throw

 Parses the merge argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the merge operation, an error message is displayed to the
 user.
*/
static void merge_parse_args(
  int          argc,
  int          last_arg,
  const char** argv
) {

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator */

  i = last_arg + 1;

  while( i < argc ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      merge_usage();
      retval = FALSE;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {
    
      if( (retval = check_option_value( argc, argv, i )) ) {
        i++;
        if( is_legal_filename( argv[i] ) ) {
          merged_file = strdup_safe( argv[i] );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output file \"%s\" is not writable", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      }

    } else {

      /* The name of a file to merge */
      if( file_exists( argv[i] ) ) {

        /* If we have not specified a merge file explicitly, set it implicitly to the first CDD file found */
        if( (merge_in_num == 0) && (merged_file == NULL) ) {
          merged_file = strdup_safe( argv[i] );
        }

        /* Add the specified merge file to the list */
        merge_in               = (char**)realloc( merge_in, (sizeof( char* ) * (merge_in_num + 1)) );
        merge_in[merge_in_num] = strdup_safe( argv[i] );
        merge_in_num++;

      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "CDD file (%s) does not exist", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    }

    i++;

  }

  /* Check to make sure that the user specified at least two files to merge */
  if( merge_in_num < 2 ) {
    print_output( "Must specify at least two CDD files to merge", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

}

/*!
 \param argc      Number of arguments in command-line to parse.
 \param last_arg  Index of last parsed argument from list.
 \param argv      List of arguments from command-line to parse.

 Performs merge command functionality.
*/
void command_merge( int argc, int last_arg, const char** argv ) { PROFILE(COMMAND_MERGE);

  int          i;     /* Loop iterator */
  int          mnum;  /* Number of merge files to read */
  unsigned int rv;    /* Return value from snprintf calls */

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  Try {

    /* Parse score command-line */
    merge_parse_args( argc, last_arg, argv );

    /* Initialize all global information */
    info_initialize();

    /* Get a copy of the number of merge files to read */
    mnum = merge_in_num;

    /* Read in base database */
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Reading CDD file \"%s\"", merge_in[0] );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    db_read( merge_in[0], READ_MODE_MERGE_NO_MERGE );
    bind_perform( TRUE, 0 );

    /* Read in databases to merge */
    for( i=1; i<mnum; i++ ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Merging CDD file \"%s\"", merge_in[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      db_read( merge_in[i], READ_MODE_MERGE_INST_MERGE );
    }

    /* Write out new database to output file */
    db_write( merged_file, FALSE, FALSE );

    print_output( "\n***  Merging completed successfully!  ***", NORMAL, __FILE__, __LINE__ );

  } Catch_anonymous {}

  /* Close database */
  db_close();

  /* Deallocate memory */
  free_safe( merged_file );
  for( i=0; i<merge_in_num; i++ ) {
    free_safe( merge_in[i] );
  }
  free_safe( merge_in );

  PROFILE_END;

}

/*
 $Log$
 Revision 1.40  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.39  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.38  2008/01/21 21:39:55  phase1geo
 Bug fix for bug 1876376.

 Revision 1.37  2008/01/16 23:10:31  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.36  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.35  2008/01/09 23:54:15  phase1geo
 More splint updates.

 Revision 1.34  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.33  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.32  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.31  2007/04/10 03:56:18  phase1geo
 Completing majority of code to support new simulation core.  Starting to debug
 this though we still have quite a ways to go here.  Checkpointing.

 Revision 1.30  2007/03/13 22:12:59  phase1geo
 Merging changes to covered-0_5-branch to fix bug 1678931.

 Revision 1.29.2.1  2007/03/13 22:05:10  phase1geo
 Fixing bug 1678931.  Updated regression.

 Revision 1.29  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.28  2006/08/18 04:41:14  phase1geo
 Incorporating bug fixes 1538920 and 1541944.  Updated regressions.  Only
 event1.1 does not currently pass (this does not pass in the stable version
 yet either).

 Revision 1.27  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.26  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.25  2006/04/12 21:22:51  phase1geo
 Fixing problems with multi-file merging.  This now seems to be working
 as needed.  We just need to document this new feature.

 Revision 1.24  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.23  2006/04/07 03:47:50  phase1geo
 Fixing run-time issues with VPI.  Things are running correctly now with IV.

 Revision 1.22  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.21  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

 Revision 1.20  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.19  2005/12/13 23:15:15  phase1geo
 More fixes for memory leaks.  Regression fully passes at this point.

 Revision 1.18  2005/11/16 05:50:12  phase1geo
 Fixing binding with merge command.

 Revision 1.17  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.16  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.15  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.14  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.13  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.12  2003/02/11 05:20:52  phase1geo
 Fixing problems with merging constant/parameter vector values.  Also fixing
 bad output from merge command when the CDD files cannot be opened for reading.

 Revision 1.11  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.10  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.9  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.8  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.7  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.6  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.5  2002/07/08 16:06:33  phase1geo
 Updating help information.

 Revision 1.4  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

