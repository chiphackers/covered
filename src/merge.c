/*!
 \file     merge.c
 \author   Trevor Williams  (trevorw@charter.net)
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

#include "db.h"
#include "defines.h"
#include "merge.h"
#include "util.h"
#include "info.h"


extern int merged_code;

/*!
 Specifies the output filename of the CDD file that contains the merged data.
*/
char* merged_file = NULL;

/*!
 Specifies the name of the input CDD file that will be read in first.  If the user
 does not specify an output CDD filename (i.e., no -o option is specified), the name
 of merge_in0 will be used for merged_file.
*/
char* merge_in0   = NULL;

/*!
 Specifies the name of the input CDD file that will be read in second (this data
 is merged into the first CDD input.
*/
char* merge_in1   = NULL;

extern char user_msg[USER_MSG_LENGTH];

/*!
 Outputs usage informaiton to standard output for merge command.
*/
void merge_usage() {

  printf( "\n" );
  printf( "Usage:  covered merge [<options>] <existing_database> <database_to_merge>\n" );
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

 \return Returns TRUE if argument parsing was successful; otherwise,
         returns FALSE.

 Parses the merge argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the merge operation, an error message is displayed to the
 user.
*/
bool merge_parse_args( int argc, int last_arg, char** argv ) {

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator                  */

  i = last_arg + 1;

  while( (i < argc) && retval ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      merge_usage();
      retval = FALSE;

    } else if( strncmp( "-o", argv[i], 2 ) == 0 ) {
    
      i++;
      if( is_directory( argv[i] ) ) {
        merged_file = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Illegal output file specified \"%s\"", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( (i + 2) == argc ) {

      /* Second to last argument.  This must be filename */
      if( file_exists( argv[i] ) ) {
        if( merged_file == NULL ) {
          merged_file = strdup_safe( argv[i], __FILE__, __LINE__ );
          merged_code = INFO_ONE_MERGED;
        } else {
          merged_code = INFO_TWO_MERGED;
        }
        merge_in0 = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "First CDD (%s) file does not exist", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else if( (i + 1) == argc ) {

      /* Last argument.  This must be filename */
      if( file_exists( argv[i] ) ) {
        merge_in1 = strdup_safe( argv[i], __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Second CDD (%s) file does not exist", argv[i] );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      }

    } else {

       snprintf( user_msg, USER_MSG_LENGTH, "Unknown merge command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
       print_output( user_msg, FATAL, __FILE__, __LINE__ );
       retval = FALSE;

    }

    i++;

  }

  return( retval );

}

/*!
 \param argc      Number of arguments in command-line to parse.
 \param last_arg  Index of last parsed argument from list.
 \param argv      List of arguments from command-line to parse.

 \return Returns 0 if merge is successful; otherwise, returns -1.

 Performs merge command functionality.
*/
int command_merge( int argc, int last_arg, char** argv ) {

  int retval = 0;  /* Return value of this function */

  /* Parse score command-line */
  if( merge_parse_args( argc, last_arg, argv ) ) {

    snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

    print_output( "Merging databases...", NORMAL, __FILE__, __LINE__ );

    /* Initialize all global information */
    info_initialize();

    /* Read in base database */
    db_read( merge_in0, READ_MODE_MERGE_NO_MERGE );

    /* Read in database to merge */
    db_read( merge_in1, READ_MODE_MERGE_INST_MERGE );

    /* Write out new database to output file */
    db_write( merged_file, FALSE );

    print_output( "\n***  Merging completed successfully!  ***", NORMAL, __FILE__, __LINE__ );

  }

  return( retval );

}

/*
 $Log$
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

