/*!
 \file     diag.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/27/2006
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "diag.h"
#include "list.h"


extern list* plus_groups;


/*!
 Pointer to the head of the diagnostic list.
*/
diag* diag_head = NULL;

/*!
 Pointer to the tail of the diagnostic list.
*/
diag* diag_tail = NULL;


/*!
*/
diag* diag_add( char* name ) {

  diag* d;  /* Pointer to new diagnostic structure */

  /* Allocate and initialize the diagnostic structure */
  d = (diag*)malloc( sizeof( diag ) );
  d->name        = strdup( name );
  d->rc_head     = NULL;
  d->rc_tail     = NULL;
  d->ran         = 0;
  d->failed      = 0;
  d->failed_cmds = NULL;
  d->next        = NULL;

  /* Add the diagnostic to the list */
  if( diag_head == NULL ) {
    diag_head = diag_tail = d;
  } else {
    diag_tail->next = d;
    diag_tail       = d;
  }
  
  return( d );

}

/*!
 \param line  String containing line from a header comment.
 \param d     Pointer to current diagnostic structure

 \return Returns 1 if the header line was parsed correctly; otherwise, returns 0.

 Parses the given diagnostic header line.
*/
int parse_diag_header_line( char* line, diag* d ) {

  int  retval = 0;        /* Return value for this function */
  int  chars_read;        /* Number of characters read from the given line */
  char group_name[4096];  /* Name of current group being parsed */

  if( strstr( line, "GROUPS" ) != NULL ) {
    line = line + strlen( "GROUPS" );
    while( sscanf( line, "%s%n", group_name, &chars_read ) == 1 ) {
      line = line + chars_read;
      if( list_find_str( group_name, plus_groups ) != -1 ) {
        retval = 1;
      }
    }
  } else {
    run_cmd_add( line, &(d->rc_head), &(d->rc_tail) );
    retval = 1;
  }

  return( retval );

}

/*!
 \param d  Pointer to current diagnostic structure.

 \return Returns 1 if the diagnostic was read properly; otherwise, returns 0.

 Reads in the diagnostic information from the specified file.
*/
int read_diag_info( diag* d ) {

  FILE* diag_file;              /* Pointer to diagnostic file */
  char  diag_path[4096];        /* Full diagnostic pathname */
  int   retval = 1;             /* Return value for this function */
  char* line   = NULL;          /* Current line being read */
  int   chars;                  /* Number of characters allocated in line */
  int   chars_read;             /* Number of characters read from the current line */
  int   in_header = 0;          /* Specifies that we are in a header comment */
  int   in_output = 0;          /* Specifies that we are in an output comment */
  FILE* output_file;            /* Pointer to output file */
  char  str[4096];              /* Temporary string */
  char  output_filename[4096];  /* Name of output file */

  /* Create diagnostic pathname */
  snprintf( diag_path, 4096, "%s.v", d->name );

  if( (diag_file = fopen( diag_path, "r" )) != NULL ) {

    while( (getline( &line, &chars, diag_file ) > 0) && (retval == 1) ) {

      if( (in_header == 0) && (in_output == 0) ) {
   
        if( strstr( line, "/* HEADER" ) != NULL ) {
          in_header = 1;
        } else if( sscanf( line, "/* OUTPUT %s\n", str ) == 1 ) {
          in_output = 1;
          snprintf( output_filename, 4096, "%s.diag", str );
          if( (output_file = fopen( output_filename, "w" )) == NULL ) {
            retval = 0;
          }
        }

      } else if( strstr( line, "*/" ) != NULL ) {

        if( in_header == 1 ) {
          in_header = 0;
        } else if( in_output == 1 ) {
          in_output = 0;
          fclose( output_file );
        }

      } else if( in_header == 1 ) {

        retval = parse_diag_header_line( line, d );

      } else if( in_output == 1 ) {

        fprintf( output_file, "%s", line );

      }

      line = NULL;

    }

  } else {

    printf( "Unable to open diagnostic file %s\n", diag_path );
    retval = 0;

  }

  fclose( diag_file );

  return( retval );

}

/*!
 \param d   Pointer to current diagnostic to run
 \param rc  Pointer to current run command that needs to be executed

 \return Returns 1 if the run command was executed; otherwise, returns 0.
*/
int diag_execute_rc_reverse( diag* d, run_cmd* rc ) {

  run_cmd* next_rc;                /* Pointer to next run command */
  int      i;                      /* Loop iterator */
  int      next_index;             /* Index of next run command */
  int      prereqs_satisfied = 0;  /* Return value for this function */
  int      executed;               /* Set to 1 if reversed run-command was executed */

  printf( "In diag_execute_rc_reverse, cmd: %s, start: %d\n", rc->cmd, rc->start );

  /* Iterate through the list of inputs, executing all required commands */
  for( i=0; i<list_get_size( rc->inputs ); i++ ) {

    /* If this prerequisite has not been executed yet, do so now */
    if( list_get_num( i, rc->inputs ) == 0 ) {

      /* Find all commands that meet the needed input criteria */
      next_rc  = d->rc_head;
      executed = 0;
      while( (next_rc != NULL) && (prereqs_satisfied == i) ) {
        if( (next_rc->okay == 1) && (rc->start == 0) && ((next_index = list_find_str( list_get_str( i, rc->inputs ), next_rc->outputs )) != -1) ) {
          prereqs_satisfied += diag_execute_rc_reverse( d, next_rc );
        }
        next_rc = next_rc->next;
      }

    } else {

      prereqs_satisfied++;

    }

  }

  /* Finally, execute the current command */
  if( prereqs_satisfied == list_get_size( rc->inputs ) ) {
    run_cmd_execute( rc, d );
  }

  return( (prereqs_satisfied == list_get_size( rc->inputs )) ? 1 : 0 );

}

void diag_execute_rc_forward( diag* d, run_cmd* rc ) {

  run_cmd* next_rc;     /* Pointer to next run command */
  int      i;           /* Loop iterator */
  int      j;           /* Loop iterator */
  int      next_index;  /* List index of the next command to run */
  char     cmd[4096];   /* Command to execute */

  printf( "In diag_execute_rc_forward, cmd: %s, start: %d\n", rc->cmd, rc->start );

  /* First, execute the current command */
  if( run_cmd_execute( rc, d ) == 0 ) {

    /* Find all dependents of this command */
    next_rc = d->rc_head;
    while( next_rc != NULL ) {
      for( i=0; i<list_get_size( rc->outputs ); i++ ) {
        if( (next_index = list_find_str( list_get_str( i, rc->outputs ), next_rc->inputs )) != -1 ) {
          list_set_num( 1, next_index, next_rc->inputs );
          j = 0;
          while( (j < list_get_size( next_rc->inputs )) && (list_get_num( j, next_rc->inputs ) == 1) ) j++;
          if( j == list_get_size( next_rc->inputs ) ) {
            diag_execute_rc_forward( d, next_rc );    /* All prerequisites have been satisfied, run this command */
          } else if( next_rc->okay == 1 ) {
            diag_execute_rc_reverse( d, next_rc );    /* Go back and execute all preliminary commands first */
          } 
        }
      }
      next_rc = next_rc->next;
    }

  }

}

/*!
 \param d  Pointer to current diagnostic to remove output files for
*/
void diag_remove_outputs( diag* d ) {

  run_cmd* curr;  /* Pointer to current run command structure */

  curr = d->rc_head;
  while( curr != NULL ) {
    run_cmd_remove_outputs( curr );
    curr = curr->next;
  }

}

/*!
 \param d  Pointer to current diagnostic to run.

 Runs the current diagnostic, verifying that all outputs are correct.
*/
void run_current_diag( diag* d ) {

  run_cmd* rc;         /* Pointer to current run command */
  int      first = 1;  /* Specifies if this is the first start */

  rc = d->rc_head;
  while( rc != NULL ) {
    if( (rc->start == 1) && (rc->okay == 1) && (rc->executed == 0) ) {
      if( first == 1 ) {
        first = 0;
        printf( "Running %s\n", d->name );
      }
      diag_execute_rc_forward( d, rc );
    }
    rc = rc->next;
  }

  /* Remove all output files */
  diag_remove_outputs( d );

  /* Display PASS/FAIL message */
  if( d->ran == 1 ) {
    if( d->failed == 0 ) {
      printf( "  -- PASSED\n" );
    } else {
      printf( "  -- FAILED\n" );
    }
  }

}

/*!
 Outputs the final pass/fail information after all diagnostics have been run.
*/
void diag_output_results() {

  diag* curr;        /* Pointer to current diagnostic structure */
  int   passed = 0;  /* Number of diagnostics that passed */
  int   failed = 0;  /* Number of diagnostics that failed */

  /* First, just calculate the number of diagnostics that passed and failed */
  curr = diag_head;
  while( curr != NULL ) {
    if( curr->ran == 1 ) {
      if( curr->failed == 0 ) {
        passed++;
      } else {
        failed++;
      }
    }
    curr = curr->next;
  }
    
  printf( "\nPassed: %d,  Failed: %d\n", passed, failed );

  if( failed > 0 ) {

    printf( "\nFailed diagnostics:\n" );

    /* Second, display all the failed diagnostics and their commands */
    curr = diag_head;
    while( curr != NULL ) {
      if( (curr->ran == 1) && (curr->failed == 1) ) {
        printf( " - %s\n", curr->name );
        list_display( curr->failed_cmds );
      }
      curr = curr->next;
    }

  }

  printf( "\n" );

}

/*!
 Deallocates the entire diagnostic list.
*/
void diag_dealloc_list() {

  diag* curr;  /* Pointer to current diagnostic structure */
  diag* tmp;   /* Temporary pointer to diagnostic structure */

  curr = diag_head;
  while( curr != NULL ) {
    
    tmp  = curr;
    curr = curr->next;

    /* Free name */
    free( tmp->name );

    /* Free run command list */
    run_cmd_dealloc_list( tmp->rc_head );

    /* Free failing_cmds list */
    list_dealloc( tmp->failed_cmds );

    /* Free the structure itself */
    free( tmp );

  }

}

/*
 $Log$
 Revision 1.3  2006/03/15 22:48:27  phase1geo
 Updating run program.  Fixing bugs in statement_connect algorithm.  Updating
 regression files.

 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/

