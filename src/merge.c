/*!
 \file     merge.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "db.h"
#include "defines.h"
#include "merge.h"
#include "util.h"


char* merged_file = NULL;
char* merge_in0   = NULL;
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
        merged_file = strdup( argv[i] );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Illegal output file specified \"%s\"", argv[i] );
        print_output( user_msg, FATAL );
        retval = FALSE;
      }

    } else if( (i + 2) == argc ) {

      /* Second to last argument.  This must be filename */
      if( file_exists( argv[i] ) ) {
        if( merged_file == NULL ) {
          merged_file = strdup( argv[i] );
        }
        merge_in0 = strdup( argv[i] );
      }

    } else if( (i + 1) == argc ) {

      /* Last argument.  This must be filename */
      if( file_exists( argv[i] ) ) {
        merge_in1 = strdup( argv[i] );
      }

    } else {

       snprintf( user_msg, USER_MSG_LENGTH, "Unknown merge command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
       print_output( user_msg, FATAL );
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

  int   retval = 0;  /* Return value of this function         */
  FILE* ofile;       /* Pointer to output stream              */
  FILE* dbfile;      /* Pointer to database file to read from */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );

  /* Parse score command-line */
  if( merge_parse_args( argc, last_arg, argv ) ) {

    snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
    print_output( user_msg, NORMAL );

    print_output( "Merging databases...", NORMAL );

    /* Read in base database */
    db_read( merge_in0, READ_MODE_MERGE_NO_MERGE );

    /* Read in database to merge */
    db_read( merge_in1, READ_MODE_MERGE_INST_MERGE );

    /* Write out new database to output file */
    db_write( merged_file, 0 );

    print_output( "\n***  Merging completed successfully!  ***", NORMAL );

  }

  return( retval );

}

/* $Log$
/* Revision 1.7  2002/10/11 05:23:21  phase1geo
/* Removing local user message allocation and replacing with global to help
/* with memory efficiency.
/*
/* Revision 1.6  2002/07/09 04:46:26  phase1geo
/* Adding -D and -Q options to covered for outputting debug information or
/* suppressing normal output entirely.  Updated generated documentation and
/* modified Verilog diagnostic Makefile to use these new options.
/*
/* Revision 1.5  2002/07/08 16:06:33  phase1geo
/* Updating help information.
/*
/* Revision 1.4  2002/07/03 03:31:11  phase1geo
/* Adding RCS Log strings in files that were missing them so that file version
/* information is contained in every source and header file.  Reordering src
/* Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
/* */
