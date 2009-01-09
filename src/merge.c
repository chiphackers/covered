/*
 Copyright (c) 2006-2009 Trevor Williams

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


extern db**         db_list;
extern unsigned int curr_db;
extern int          merged_code;
extern char         user_msg[USER_MSG_LENGTH];
extern char*        cdd_message;


/*!
 Specifies the output filename of the CDD file that contains the merged data.
*/
char* merged_file = NULL;

/*!
 Pointer to head of list containing names of the input CDD files.
*/
str_link* merge_in_head = NULL;

/*!
 Pointer to tail of list containing names of the input CDD files.
*/
str_link* merge_in_tail = NULL;

/*!
 Pointer to last input name from command-line.
*/
str_link* merge_in_cl_last = NULL;

/*!
 Specifies the number of merged CDD files.
*/
int merge_in_num  = 0;

/*!
 Specifies the value of the -er option.
*/
int merge_er_value = MERGE_ER_NONE;


/*!
 Outputs usage informaiton to standard output for merge command.
*/
static void merge_usage() {

  printf( "\n" );
  printf( "Usage:  covered merge (-h | [<options>] <existing_database> <database_to_merge>+)\n" );
  printf( "\n" );
  printf( "   -h                         Displays this help information.\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -o <filename>           File to output new database to.  If this argument is not\n" );
  printf( "                                specified, the <existing_database> is used as the output\n" );
  printf( "                                database name.\n" );
  printf( "      -f <filename>           Name of file containing additional arguments to parse.\n" );
  printf( "      -d <directory>          Directory to search for CDD files to include.  This option is used in\n" );
  printf( "                                conjunction with the -ext option which specifies the file extension\n" );
  printf( "                                to use for determining which files in the directory are CDD files.\n" );
  printf( "      -er <value>             Specifies how to handle exclusion reason resolution.  If two or more CDD files being\n" );
  printf( "                                merged have exclusion reasons specified for the same coverage point, the exclusion\n" );
  printf( "                                reason needs to be resolved (unless it is the same string value).  If this option\n" );
  printf( "                                is not specified and a conflict is found, Covered will interactively request input\n" );
  printf( "                                for each exclusion as to how to handle it.  If this option is specified, it tells\n" );
  printf( "                                Covered how to handle all exclusion reason conflicts.  The values are as follows:\n" );
  printf( "                                  first - CDD file that contained the first exclusion reason is used.\n" );
  printf( "                                  last  - CDD file that contained the last exclusion reason is used.\n" );
  printf( "                                  all   - All exclusion reasons are used (concatenated).\n" );
  printf( "                                  new   - Use the newest exclusion reason specified.\n" );
  printf( "                                  old   - Use the oldest exclusion reason specified.\n" );
  printf( "      -ext <extension>        Used in conjunction with the -d option.  If no -ext options are specified\n" );
  printf( "                                on the command-line, the default value of '.cdd' is used.  Note that\n" );
  printf( "                                a period (.) should be specified.\n" );
  printf( "      -m <message>            Allows the user to specify information about this CDD file.  This information\n" );
  printf( "                                can be anything (messages with whitespace should be surrounded by double-quotation\n" );
  printf( "                                marks), but may include something about the simulation arguments to more easily\n" );
  printf( "                                link the CDD file to its simulation for purposes of recreating the CDD file.\n" );
  printf( "\n" );

}

/*!
 \throws anonymous Throw Throw Throw

 Parses the merge argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the merge operation, an error message is displayed to the
 user.
*/
static void merge_parse_args(
  int          argc,      /*!< Number of arguments in argument list argv */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Argument list passed to this program */
) {

  int       i;
  str_link* strl;
  str_link* ext_head     = NULL;
  str_link* ext_tail     = NULL;
  str_link* dir_head     = NULL;
  str_link* dir_tail     = NULL;

  i = last_arg + 1;

  while( i < argc ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      merge_usage();
      Throw 0;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {
    
      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( is_legal_filename( argv[i] ) ) {
          merged_file = strdup_safe( argv[i] );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output file \"%s\" is not writable", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        char**       arg_list = NULL;
        int          arg_num  = 0;
        unsigned int j;
        i++;
        Try {
          read_command_file( argv[i], &arg_list, &arg_num );
          merge_parse_args( arg_num, -1, (const char**)arg_list );
        } Catch_anonymous {
          for( j=0; j<arg_num; j++ ) {
            free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
          }
          free_safe( arg_list, (sizeof( char* ) * arg_num) );
          Throw 0;
        }
        for( j=0; j<arg_num; j++ ) {
          free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
        }
        free_safe( arg_list, (sizeof( char* ) * arg_num) );
      } else {
        Throw 0;
      }

    } else if( strncmp( "-d", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( directory_exists( argv[i] ) ) {
          (void)str_link_add( strdup_safe( argv[i] ), &dir_head, &dir_tail );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Specified -d directory (%s) does not exist", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-er", argv[i], 3 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( strncmp( "first", argv[i], 5 ) == 0 ) {
          merge_er_value = MERGE_ER_FIRST;
        } else if( strncmp( "last", argv[i], 4 ) == 0 ) {
          merge_er_value = MERGE_ER_LAST;
        } else if( strncmp( "all", argv[i], 3 ) == 0 ) {
          merge_er_value = MERGE_ER_ALL;
        } else if( strncmp( "new", argv[i], 3 ) == 0 ) {
          merge_er_value = MERGE_ER_NEW;
        } else if( strncmp( "old", argv[i], 3 ) == 0 ) {
          merge_er_value = MERGE_ER_OLD;
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal value to use for the -er option (%s).  Valid values are: first, last, all, new, old", argv[i] );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } else {
        Throw 0;
      }

    } else if( strncmp( "-ext", argv[i], 4 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        (void)str_link_add( strdup_safe( argv[i] ), &ext_head, &ext_tail );
      } else {
        Throw 0;
      } 

    } else if( strncmp( "-m", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        i++;
        if( cdd_message != NULL ) {
          print_output( "Only one -m option is allowed on the merge command-line.  Using first value...", WARNING, __FILE__, __LINE__ );
        } else {
          cdd_message = strdup_safe( argv[i] );
        }
      } else {
        Throw 0;
      }

    } else {

      /* The name of a file to merge */
      if( file_exists( argv[i] ) ) {

        /* Create absolute filename */
        char* file = get_absolute_path( argv[i] );

        /* If we have not specified a merge file explicitly, set it implicitly to the first CDD file found */
        if( (merge_in_head == NULL) && (merged_file == NULL) ) {
          merged_file = strdup_safe( file );
        }

        /* Add the specified merge file to the list */
        (void)str_link_add( file, &merge_in_head, &merge_in_tail );

      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "CDD file (%s) does not exist", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    }

    i++;

  }

  Try {

    /* Load any merge files found in specified directories */
    strl = dir_head;
    while( strl != NULL ) {
      directory_load( strl->str, ext_head, &merge_in_head, &merge_in_tail );
      strl = strl->next;
    }

    /* Deallocate the temporary lists */
    str_link_delete_list( ext_head );
    str_link_delete_list( dir_head );

  } Catch_anonymous {
    str_link_delete_list( ext_head );
    str_link_delete_list( dir_head );
    Throw 0;
  }

  /* Set the last command-line pointer to the current tail */
  merge_in_cl_last = merge_in_tail;

  /* Make sure that we have at least 2 files to merge */
  strl = merge_in_head;
  while( strl != NULL ) {
    merge_in_num++;
    strl = strl->next;
  }

  /* Check to make sure that the user specified at least two files to merge */
  if( merge_in_num < 2 ) {
    print_output( "Must specify at least two CDD files to merge", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /*
   If no -o option was specified and no merge files were specified, don't presume that the first file found in
   the directory will be that file.
  */
  if( merged_file == NULL ) {
    print_output( "Must specify the -o option or a specific CDD file for containing the merged results", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

}

/*!
 Performs merge command functionality.
*/
void command_merge(
  int          argc,      /*!< Number of arguments in command-line to parse */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< List of arguments from command-line to parse */
) { PROFILE(COMMAND_MERGE);

  int          i;     /* Loop iterator */
  unsigned int rv;    /* Return value from snprintf calls */

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  Try {

    str_link* strl;
    bool      stop_merging;
    int       curr_leading_hier_num = 0;

    /* Parse score command-line */
    merge_parse_args( argc, last_arg, argv );

    /* Read in base database */
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Reading CDD file \"%s\"", merge_in_head->str );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    db_read( merge_in_head->str, READ_MODE_MERGE_NO_MERGE );

    /* If the currently read CDD didn't contain any merged CDDs it is a leaf CDD so mark it as such */
    if( (db_list[curr_db]->leading_hier_num - curr_leading_hier_num) == 1 ) {
      merge_in_head->suppl = 1;
    }
    curr_leading_hier_num = db_list[curr_db]->leading_hier_num;

    /* Read in databases to merge */
    strl         = merge_in_head->next;
    stop_merging = (strl == merge_in_head);
    while( (strl != NULL) && !stop_merging ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Merging CDD file \"%s\"", strl->str );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
      db_read( strl->str, READ_MODE_MERGE_NO_MERGE );

      /* If we have not merged any CDD files from this CDD, this is a leaf CDD so mark it as such */
      if( (db_list[curr_db]->leading_hier_num - curr_leading_hier_num) == 1 ) {
        strl->suppl = 1;
      }
      curr_leading_hier_num = db_list[curr_db]->leading_hier_num;

      stop_merging = (strl == merge_in_cl_last);
      strl         = strl->next;
    }

    /* Perform the tree merges */
    db_merge_instance_trees();

    /* Bind */
    bind_perform( TRUE, 0 );

    /* Write out new database to output file */
    db_write( merged_file, FALSE, TRUE, FALSE );

    print_output( "\n***  Merging completed successfully!  ***", NORMAL, __FILE__, __LINE__ );

  } Catch_anonymous {}

  /* Close database */
  db_close();

  /* Deallocate other memory */
  str_link_delete_list( merge_in_head );
  free_safe( merged_file, (strlen( merged_file ) + 1) );

  PROFILE_END;

}

/*
 $Log$
 Revision 1.64  2008/11/12 19:57:07  phase1geo
 Fixing the rest of the issues from regressions in regards to the merge changes.
 Updating regression files.  IV and Cver regressions now pass.

 Revision 1.63  2008/11/11 05:36:40  phase1geo
 Checkpointing merge code.

 Revision 1.62  2008/10/31 22:01:34  phase1geo
 Initial code changes to support merging two non-overlapping CDD files into
 one.  This functionality seems to be working but needs regression testing to
 verify that nothing is broken as a result.

 Revision 1.61  2008/09/22 22:15:04  phase1geo
 Initial code for supporting the merging and resolution of exclusion reasons.
 This code is completely untested at this point but does compile.  Checkpointing.

 Revision 1.60  2008/09/22 05:04:50  phase1geo
 Adding parsing support for new -er option.

 Revision 1.59  2008/09/22 04:19:57  phase1geo
 Fixing bug 2122019.  Also adding exclusion reason timestamp support to CDD files.

 Revision 1.58  2008/09/17 04:55:46  phase1geo
 Integrating new get_absolute_path and get_relative_path functions and
 updating regressions.  Also fixed a few coding bugs with these new functions.
 IV and Cver regressions fully pass at the moment.

 Revision 1.57  2008/09/15 22:42:07  phase1geo
 Fixing bug 2112509.

 Revision 1.56  2008/09/15 22:04:42  phase1geo
 Fixing bug 2112858.  Also added new exclude10.3.1 diagnostic which recreates
 bug 2112613 (this bug is not fixed yet, however).

 Revision 1.55  2008/09/15 03:43:49  phase1geo
 Cleaning up splint warnings.

 Revision 1.54  2008/09/04 21:34:20  phase1geo
 Completed work to get exclude reason support to work with toggle coverage.
 Ground-work is laid for the rest of the coverage metrics.  Checkpointing.

 Revision 1.53  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.47.2.4  2008/08/06 05:32:41  phase1geo
 Another fix for bug 2037791.  Also add new diagnostic to verify the fix for the bug.

 Revision 1.47.2.3  2008/08/05 03:56:45  phase1geo
 Completing fix for bug 2037791.  Added diagnostic to regression suite to verify
 the corrected behavior.

 Revision 1.47.2.2  2008/07/23 05:10:11  phase1geo
 Adding -d and -ext options to rank and merge commands.  Updated necessary files
 per this change and updated regressions.

 Revision 1.47.2.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.51  2008/06/22 22:02:01  phase1geo
 Fixing regression issues.

 Revision 1.50  2008/06/20 14:19:20  phase1geo
 Updating merge.c and report.c to remove unnecessary code and output.

 Revision 1.49  2008/06/18 13:22:33  phase1geo
 Adding merge diagnostics and updating more regression scripting files.
 Full regression passes.

 Revision 1.48  2008/06/17 14:40:54  phase1geo
 Adding merge_err1 diagnostic to regression suite (which is a new script style
 of diagnostic running) and adding support for script-based regression runs
 to diagnostic Makefile structures.

 Revision 1.47  2008/05/30 05:38:31  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.46.2.1  2008/05/24 05:36:21  phase1geo
 Fixing bitwise coverage functionality and updating regression files.  Added
 new bitwise1 and err5.1 diagnostics to regression suite.  Removing output
 for uncovered exceptions in command-line parsers.

 Revision 1.46  2008/03/31 21:40:23  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.45  2008/03/31 18:39:08  phase1geo
 Fixing more regression issues related to latest code modifications.  Checkpointing.

 Revision 1.44  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.43  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.42  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.41  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

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

