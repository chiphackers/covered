/*!
 \file     merge.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "defines.h"
#include "merge.h"


char* merged_file = NULL;
char* merge_in0   = NULL;
char* merge_in1   = NULL;


/*!
 \param argc  Number of arguments in argument list argv.
 \param argv  Argument list passed to this program.

 \return Returns TRUE if argument parsing was successful; otherwise,
         returns FALSE.

 Parses the merge argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the merge operation, an error message is displayed to the
 user.
*/
bool merge_parse_args( int argc, char** argv ) {

  bool retval = TRUE;    /* Return value for this function */
  int  i;                /* Loop iterator                  */
  char err_msg[4096];    /* Error message for user         */

  i = 2;

  while( (i < argc) && retval ) {

    if( strncmp( "-o", argv[i], 2 ) == 0 ) {
    
      i++;
      if( is_directory( argv[i] ) ) {
        merged_file = strdup( argv[i] );
      } else {
  	snprintf( err_msg, 4096, "Illegal output file specified \"%s\"", argv[i] );
        print_output( err_msg, FATAL );
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

       snprintf( err_msg, 4096, "Unknown merge command option \"%s\".  See \"covered -h\" for more information.", argv[i] );
       print_output( err_msg, FATAL );
       retval = FALSE;

    }

    i++;

  }

  return( retval );

}

/*!
 \param argc  Number of arguments in command-line to parse.
 \param argv  List of arguments from command-line to parse.
 \return Returns 0 if merge is successful; otherwise, returns -1.

*/
int command_merge( int argc, char** argv ) {

  int   retval = 0;    /* Return value of this function         */
  FILE* ofile;         /* Pointer to output stream              */
  FILE* dbfile;        /* Pointer to database file to read from */
  char  msg[4096];     /* Error message for user                */

  /* Initialize error suppression value */
  set_output_suppression( FALSE );

  /* Parse score command-line */
  if( !merge_parse_args( argc, argv ) ) {
    exit( 1 );
  }

  print_output( "Merging databases...", NORMAL );

  /* Read in base database */
  db_read( merge_in0, READ_MODE_MERGE_NO_MERGE );

  /* Read in database to merge */
  db_read( merge_in1, READ_MODE_MERGE_INST_MERGE );

  /* Write out new database to output file */
  db_write( merged_file );

  printf( "\n***  Merging completed successfully!  ***\n" );

  return( retval );

}

