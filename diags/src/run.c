/*!
 \file     run.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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

    if( strncmp( "+", argvptr, 1 ) == 0 ) {

      list_add( (argvptr + 1), 0, &plus_groups );

    } else if( strncmp( "-", argvptr, 1 ) == 0 ) {

      list_add( (argvptr + 1), 0, &minus_groups );

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
  diag*          d;                  /* Newly allocated diagnostic structure */

  if( (dir_handle = opendir( "." )) == NULL ) {

    printf( "Unable to read directory ../verilog\n" );
    exit( 1 );

  } else {

    while( (dirp = readdir( dir_handle )) != NULL ) {

      if( (sscanf( dirp->d_name, "%[a-zA-Z0-9_]%s", diag_name, extension ) == 2) && (strcmp( ".v", extension ) == 0) ) {

        /* Create diagnostic structure */
        d = diag_add( diag_name );

        if( read_diag_info( d ) == 1 ) {

          run_current_diag( d );

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

  diag_output_results();

}

/*!
 Deallocates all allocated memory in this file.
*/
void cleanup() {

  list_dealloc( plus_groups );
  list_dealloc( minus_groups );
  diag_dealloc_list();

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
 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/
