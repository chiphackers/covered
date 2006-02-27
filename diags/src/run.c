/*!
 \file     run.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/27/2006
 \brief    Run script for running individual diagnostics or regressions
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "diag.h"
#include "list.h"


/*!
 User-specified array of plusarg groups to run.
*/
list* plus_groups = NULL;

/*!
 User-specified array of minusarg groups to not run.
*/
list* minus_groups = NULL;

/*!
 List of diagnostics that failed along with the command that caused it to fail.
*/
list* failed_diags = NULL;

/*!
 The number of diagnostics that passed.
*/
int    passed_diags_size;


/*!
 Displays usage information to standard output.
*/
void usage() {

  printf( "Usage:  run +<group_name>* -<group_name>*\n" );

}

/*!
 \param argc  Number of elements in the argv array.
 \param argv  Array of strings containing user-specified arguments to this script

 Parses the command-line arguments for plus and minus arguments and stores these values in arrays.
*/
void parse_args( int argc, char** argv ) {

  char* argvptr;  /* Pointer to current string in argv array */
  int   i = 1;    /* Loop iterator */

  while( i < argc ) {

    argvptr = argv[i];
    printf( "argvptr: %s\n", argvptr );

    if( strncmp( "+", argvptr, 1 ) == 0 ) {

      list_add( (argvptr + 1), &plus_groups );

    } else if( strncmp( "-", argvptr, 1 ) == 0 ) {

      list_add( (argvptr + 1), &minus_groups );

    } else {

      usage();
      exit( 1 );

    }

    i++;

  }

}

/*!
 Runs all diagnostics that match the user-specified criteria in the ../verilog directory.  Stores pass and fail
 status of each diagnostic that was run for later output.
*/
void run_diags() {

  DIR*           dir_handle;         /* Pointer to opened directory */
  struct dirent* dirp;               /* Pointer to current directory entry */
  char           diag_name[256];     /* Name of diagnostic */
  char           extension[256];     /* Diagnostic extension information */
  int            chars_read;         /* Number of characters read from line */
  int            ran;                /* Specifies if diagnostic was run */
  int            failed;             /* Specifies if diagnostic was run and failed */
  char           failing_cmd[4096];  /* If failed is set to 1, this will contain the command that failed */
  char           failing_msg[4096];  /* Diagnostic failure message */

  if( (dir_handle = opendir( "../verilog" )) == NULL ) {

    printf( "Unable to read directory ../verilog\n" );
    exit( 1 );

  } else {

    while( (dirp = readdir( dir_handle )) != NULL ) {

      if( (sscanf( dirp->d_name, "%[a-zA-Z0-9_]%s", diag_name, extension ) == 2) && (strcmp( ".v", extension ) == 0) ) {

        if( read_diag_info( diag_name ) == 1 ) {

          run_current_diag( diag_name, &ran, &failed, &failing_cmd );

          if( ran == 1 ) {
            if( failed == 1 ) {
              printf( "  -- FAILED\n" );
              snprintf( failing_msg, 4096, "%s  (%s)", diag_name, failing_cmd );
              list_add( failing_msg, &failed_diags );
            } else {
              printf( "  -- PASSED\n" );
              passed_diags_size++;
            }
          }

        }

      }

    }

    closedir( dir_handle );

  }

}

/*!
 Outputs run results to standard output.
*/
void output_results() {

  printf( "\nPassed: %d,  Failed: %d\n", passed_diags_size, failed_diags->num );

  if( failed_diags != NULL ) {
    printf( "\nFailing diagnostics:\n" );
    list_display( failed_diags );
  }
    
  printf( "\n" );

}

/*!
 Deallocates all allocated memory in this file.
*/
void cleanup() {

  list_dealloc( plus_groups );
  list_dealloc( minus_groups );
  list_dealloc( failed_diags );

}

/*!
 \param argc  Number of elements in the argv array.
 \param argv  Array containing command-line argument values.

 Main routine.
*/
main( int argc, char** argv ) {

  /* Parse command-line arguments */
  parse_args( argc, argv );

  /* Run diagnostics */
  run_diags();

  /* Generate output */
  output_results();

  /* Cleanup memory */
  cleanup();

}


/*
 $Log$
*/
