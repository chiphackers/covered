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
 \file     info.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/12/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "info.h"
#include "link.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern str_link*    merge_in_head;
extern str_link*    merge_in_tail;
extern int          merge_in_num;
extern char*        merged_file;
extern uint64       num_timesteps;
extern char*        cdd_message;
extern char         user_msg[USER_MSG_LENGTH];


/*!
 Informational line for the CDD file.
*/
isuppl info_suppl = {0};

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
int cdd_version = CDD_VERSION;

/*!
 Specifes the pathname where the score command was originally run from.
*/
char score_run_path[4096];

/*!
 Array containing all of the score arguments.
*/
/*@null@*/ char** score_args = NULL;

/*!
 Number of valid elements in the score args array.
*/
int score_arg_num = 0;


/*!
 Sets the vector element size in the global info_suppl structure based on the current machine
 unsigned long byte size.
*/
void info_set_vector_elem_size() { PROFILE(INFO_SET_VECTOR_ELEM_SIZE);

  switch( sizeof( ulong ) ) {
    case 1 :  info_suppl.part.vec_ul_size = 0;  break;
    case 2 :  info_suppl.part.vec_ul_size = 1;  break;
    case 4 :  info_suppl.part.vec_ul_size = 2;  break;
    case 8 :  info_suppl.part.vec_ul_size = 3;  break;
    default:
      print_output( "Unsupported unsigned long size", FATAL, __FILE__, __LINE__ );
      Throw 0;
      /*@-unreachable@*/
      break;
      /*@=unreachable@*/
  }

  PROFILE_END;

}

/*!
 Writes information line to specified file.
*/
void info_db_write(
  FILE* file  /*!< Pointer to file to write information to */
) { PROFILE(INFO_DB_WRITE);

  int i;  /* Loop iterator */

  assert( db_list[curr_db]->leading_hier_num > 0 );

  /* Calculate vector element size */
  info_set_vector_elem_size();

  fprintf( file, "%d %x %x %llu %s\n",
           DB_TYPE_INFO,
           CDD_VERSION,
           info_suppl.all,
           num_timesteps,
           db_list[curr_db]->leading_hierarchies[0] );

  /* Display score arguments */
  fprintf( file, "%d %s", DB_TYPE_SCORE_ARGS, score_run_path );

  for( i=0; i<score_arg_num; i++ ) {
    fprintf( file, " %s", score_args[i] );
  }

  fprintf( file, "\n" );

  /* Display the CDD message, if there is one */
  if( cdd_message != NULL ) {
    fprintf( file, "%d %s\n", DB_TYPE_MESSAGE, cdd_message );
  }

  /* Display the merged CDD information, if there are any */
  if( db_list[curr_db]->leading_hier_num == merge_in_num ) {
    str_link* strl = merge_in_head;
    i = 0;
    while( strl != NULL ) {
      if( strcmp( strl->str, merged_file ) != 0 ) {
        fprintf( file, "%d %s %s\n", DB_TYPE_MERGED_CDD, strl->str, db_list[curr_db]->leading_hierarchies[i++] );
      } else {
        i++;
      }
      strl = strl->next; 
    }
  } else { 
    str_link* strl = merge_in_head;
    assert( (db_list[curr_db]->leading_hier_num - 1) == merge_in_num );
    i = 1; 
    while( strl != NULL ) {
      if( strcmp( strl->str, merged_file ) != 0 ) {
        fprintf( file, "%d %s %s\n", DB_TYPE_MERGED_CDD, strl->str, db_list[curr_db]->leading_hierarchies[i++] );
      } else {
        i++;
      }
      strl = strl->next;
    }
  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw

 Reads information line from specified string and stores its information.
*/
void info_db_read(
  /*@out@*/ char** line  /*!< Pointer to string containing information line to parse */
) { PROFILE(INFO_DB_READ);

  int          chars_read;  /* Number of characters scanned in from this line */
  uint32       scored;      /* Indicates if this file contains scored data */
  unsigned int version;     /* Contains CDD version from file */
  char         tmp[4096];   /* Temporary string */

  /* Save off original scored value */
  scored = info_suppl.part.scored;

  if( sscanf( *line, "%x%n", &version, &chars_read ) == 1 ) {

    *line = *line + chars_read;

    if( version != CDD_VERSION ) {
      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

    if( sscanf( *line, "%x %llu %s%n", &(info_suppl.all), &num_timesteps, tmp, &chars_read ) == 3 ) {

      *line = *line + chars_read;

      /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
      if( (db_list[curr_db]->leading_hier_num > 0) && (strcmp( db_list[curr_db]->leading_hierarchies[0], tmp ) != 0) ) {
        db_list[curr_db]->leading_hiers_differ = TRUE;
      }

      /* Assign this hierarchy to the leading hierarchies array */
      db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
      db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( tmp );
      db_list[curr_db]->leading_hier_num++;

      /* Set scored flag to correct value */
      if( info_suppl.part.scored == 0 ) {
        info_suppl.part.scored = scored;
      }

    } else {

      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads score command-line args line from specified string and stores its information.
*/
void args_db_read(
  char** line  /*!< Pointer to string containing information line to parse */
) { PROFILE(ARGS_DB_READ);

  int  chars_read;  /* Number of characters scanned in from this line */
  char tmp1[4096];  /* Temporary string */

  if( sscanf( *line, "%s%n", score_run_path, &chars_read ) == 1 ) {

    *line = *line + chars_read;

    /* Store score command-line arguments */
    while( sscanf( *line, "%s%n", tmp1, &chars_read ) == 1 ) {
      *line                     = *line + chars_read;
      score_args                = (char**)realloc_safe( score_args, (sizeof( char* ) * score_arg_num), (sizeof( char* ) * (score_arg_num + 1)) );
      score_args[score_arg_num] = strdup_safe( tmp1 );
      score_arg_num++;
    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Read user-specified message from specified string and stores its information.
*/
void message_db_read(
  char** line  /*!< Pointer to string containing information line to parse */
) { PROFILE(MESSAGE_DB_READ);

  /* All we need to do is copy the message */
  if( strlen( *line + 1 ) > 0 ) {
    cdd_message = strdup_safe( *line + 1 );
  }

  PROFILE_END;

}

/*!
 Parses given line for merged CDD information and stores this information in the appropriate global variables.
*/
void merged_cdd_db_read(
  char** line  /*!< Pointer to string containing merged CDD line to parse */
) { PROFILE(MERGED_CDD_DB_READ);

  char tmp1[4096];  /* Temporary string */
  char tmp2[4096];  /* Temporary string */
  int  chars_read;  /* Number of characters read */

  if( sscanf( *line, "%s %s%n", tmp1, tmp2, &chars_read ) == 2 ) {

    str_link* strl;

    *line = *line + chars_read;

    /* Add merged file */
    if( (strl = str_link_find( tmp1, merge_in_head)) == NULL ) {

      str_link_add( strdup_safe( tmp1 ), &merge_in_head, &merge_in_tail );
      merge_in_num++;

      /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
      if( strcmp( db_list[curr_db]->leading_hierarchies[0], tmp2 ) != 0 ) {
        db_list[curr_db]->leading_hiers_differ = TRUE;
      }

      /* Add its hierarchy */
      db_list[curr_db]->leading_hierarchies = (char**)realloc_safe( db_list[curr_db]->leading_hierarchies, (sizeof( char* ) * db_list[curr_db]->leading_hier_num), (sizeof( char* ) * (db_list[curr_db]->leading_hier_num + 1)) );
      db_list[curr_db]->leading_hierarchies[db_list[curr_db]->leading_hier_num] = strdup_safe( tmp2 );
      db_list[curr_db]->leading_hier_num++;

    } else if( merge_in_num > 0 ) {

      snprintf( user_msg, USER_MSG_LENGTH, "File %s in CDD file has been specified on the command-line", tmp1 );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Deallocates all memory associated with the database information section.  Needs to be called
 when the database is closed.
*/
void info_dealloc() { PROFILE(INFO_DEALLOC);

  int i;  /* Loop iterator */

  /* Free score arguments */
  for( i=0; i<score_arg_num; i++ ) {
    free_safe( score_args[i], (strlen( score_args[i] ) + 1) );
  }
  free_safe( score_args, (sizeof( char* ) * score_arg_num) );

  score_args    = NULL;
  score_arg_num = 0;

  /* Free merged arguments */
  str_link_delete_list( merge_in_head );
  merge_in_head = NULL;
  merge_in_tail = NULL;
  merge_in_num  = 0;

  /* Free user message */
  free_safe( cdd_message, (strlen( cdd_message ) + 1) );
  cdd_message = NULL;

  PROFILE_END;

}

/*
 $Log$
 Revision 1.38  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.32.2.7  2008/08/06 05:32:41  phase1geo
 Another fix for bug 2037791.  Also add new diagnostic to verify the fix for the bug.

 Revision 1.32.2.6  2008/08/05 03:56:45  phase1geo
 Completing fix for bug 2037791.  Added diagnostic to regression suite to verify
 the corrected behavior.

 Revision 1.32.2.5  2008/08/04 17:29:24  phase1geo
 Attempting to fix bug 2037791.

 Revision 1.32.2.4  2008/07/25 21:08:35  phase1geo
 Modifying CDD file format to remove the potential for memory allocation assertion
 errors due to a large number of merged CDD files.  Updating IV and Cver regressions per this
 change.

 Revision 1.32.2.3  2008/07/23 05:10:11  phase1geo
 Adding -d and -ext options to rank and merge commands.  Updated necessary files
 per this change and updated regressions.

 Revision 1.32.2.2  2008/07/21 21:35:07  phase1geo
 Fixing bug with deallocation of merge_in array and a later reallocation.  Needed
 to reset merge_in to NULL and merge_in_num to 0.

 Revision 1.32.2.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.36.2.2  2008/07/02 23:10:38  phase1geo
 Checking in work on rank function and addition of -m option to score
 function.  Added new diagnostics to verify beginning functionality.
 Checkpointing.

 Revision 1.36.2.1  2008/07/01 06:17:22  phase1geo
 More updates to rank command.  Updating IV/Cver regression for these changes (full
 regression not passing at this point).  Checkpointing.

 Revision 1.36  2008/06/27 14:02:02  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.35  2008/06/23 02:33:39  phase1geo
 Adding err9 diagnostic to regression suite.

 Revision 1.34  2008/06/22 22:02:01  phase1geo
 Fixing regression issues.

 Revision 1.33  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.32  2008/05/30 05:38:31  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.31.2.1  2008/05/28 22:12:31  phase1geo
 Adding further support for 32-/64-bit support.  Checkpointing.

 Revision 1.31  2008/03/31 21:40:23  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.30  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.29  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.28  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.27  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.26  2008/03/10 22:00:31  phase1geo
 Working on more exception handling (script is finished now).  Starting to work
 on code enhancements again :)  Checkpointing.

 Revision 1.25  2008/03/04 22:46:08  phase1geo
 Working on adding check_exceptions.pl script to help me make sure that all
 exceptions being thrown are being caught and handled appropriately.  Other
 code adjustments are made in regards to exception handling.

 Revision 1.24  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.23  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.22  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.21  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.20  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.19  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.18  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.17  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.16  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.15  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.14  2006/04/12 21:22:51  phase1geo
 Fixing problems with multi-file merging.  This now seems to be working
 as needed.  We just need to document this new feature.

 Revision 1.13  2006/04/12 18:06:24  phase1geo
 Updating regressions for changes that were made to support multi-file merging.
 Also fixing output of FSM state transitions to be what they were.
 Regressions now pass; however, the support for multi-file merging (beyond two
 files) has not been tested to this point.

 Revision 1.12  2006/04/12 13:28:37  phase1geo
 Fixing problem with memory allocation for merged files.

 Revision 1.11  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.10  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.9  2005/12/12 03:46:14  phase1geo
 Adding exclusion to score command to improve performance.  Updated regression
 which now fully passes.

 Revision 1.8  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.7  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.6  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.5  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.4  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.3  2003/10/17 02:12:38  phase1geo
 Adding CDD version information to info line of CDD file.  Updating regression
 for this change.

 Revision 1.2  2003/02/18 20:17:02  phase1geo
 Making use of scored flag in CDD file.  Causing report command to exit early
 if it is working on a CDD file which has not been scored.  Updated testsuite
 for these changes.

 Revision 1.1  2003/02/12 14:56:26  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

*/

