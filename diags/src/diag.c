/*!
 \file     diag.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/27/2006
*/

#include <stdlib.h>
#include <stdio.h>

#include "diag.h"
#include "list.h"


extern list* plus_groups;
extern list* minus_groups;


/*!
 List of the groups that need to be run.
*/
list* groups_to_run = NULL;


/*!
 \param line  String containing line from a header comment.

 \return Returns 1 if the header line was parsed correctly; otherwise, returns 0.

 Parses the given diagnostic header line.
*/
int parse_diag_header_line( char* line ) {

  int  retval = 0;        /* Return value for this function */
  int  chars_read;        /* Number of characters read from the given line */
  char group_name[4096];  /* Name of current group being parsed */

  if( strstr( line, "GROUPS" ) != NULL ) {
    line = line + strlen( "GROUPS" );
    while( sscanf( line, "%s%n", group_name, &chars_read ) == 1 ) {
      line = line + chars_read;
      printf( "group_name: %s\n", group_name );
      if( list_find( group_name, plus_groups ) == 1 ) {
        printf( "HERE\n" );
        list_add( group_name, &groups_to_run );        
        printf( "HERE\n" );
        retval = 1;
      }
    }
  } else {
    run_cmd_add( &line );
    retval = 1;
  }

  return( retval );

}

/*!
 \param diag_name  Name of diagnostic to read in from ../verilog directory.

 \return Returns 1 if the diagnostic was read properly; otherwise, returns 0.

 Reads in the diagnostic information from the specified file.
*/
int read_diag_info( char* diag_name ) {

  FILE* diag;                   /* Pointer to diagnostic file */
  char  diag_path[4096];        /* Full diagnostic pathname */
  int   retval = 1;             /* Return value for this function */
  char* line   = NULL;          /* Current line being read */
  int   chars;                  /* Number of characters allocated in line */
  int   chars_read;             /* Number of characters read from the current line */
  int   in_header = 0;          /* Specifies that we are in a header comment */
  int   in_output = 0;          /* Specifies that we are in an output comment */
  FILE* output_file;            /* Pointer to output file */
  char  output_filename[4096];  /* Name of output file */

  /* Create diagnostic pathname */
  snprintf( diag_path, 4096, "../verilog/%s.v", diag_name );

  printf( "Reading diagnostic information for %s\n", diag_path );

  if( (diag = fopen( diag_path, "r" )) != NULL ) {

    while( (getline( &line, &chars, diag ) > 0) && (retval == 1) ) {

      printf( "line: %s", line );

      if( (in_header == 0) && (in_output == 0) ) {
   
        if( strstr( line, "/* HEADER" ) != NULL ) {
          in_header = 1;
        } else if( sscanf( line, "/* OUTPUT %s%n\n", output_filename ) == 1 ) {
          in_output = 1;
          if( (output_file = fopen( output_filename, "w" )) == NULL ) {
            retval = 0;
          }
        }

      } else if( sscanf( line, "*/%n", &chars_read ) && (chars_read > 0) ) {

        if( in_header == 1 ) {
          in_header = 0;
        } else if( in_output == 1 ) {
          in_output = 0;
          fclose( output_file );
        }

      } else if( in_header == 1 ) {

        retval = parse_diag_header_line( line );

      } else if( in_output == 1 ) {

        fprintf( output_file, "%s", line );

      }

      line = NULL;

    }

  } else {

    printf( "Unable to open diagnostic file %s\n", diag_path );
    retval = 0;

  }

  fclose( diag );

}

void run_current_diag( char* diag_name, int* ran, int* failed, char* failing_cmd ) {

  printf( "In run_current_diag...\n" );

}


/*
 $Log$
*/

