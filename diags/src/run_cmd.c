/*!
 \file     run_cmd.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/27/2006
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "run_cmd.h"
#include "list.h"
#include "diag.h"


extern list* plus_groups;
extern list* minus_groups;


char* run_cmd_get_block( char** line ) {

  char* start;  /* Pointer to start of current line */
  char* ptr;    /* Pointer to start of separating string */

  start = *line;

  if( (ptr = strstr( *line, " : " )) != NULL ) {
    *ptr = '\0';
    *line = ptr + 3;
  } else {
    *line = NULL;
  }

  return( start );

}

void run_cmd_add_step_and_inputs( char** line, run_cmd* rc ) {

  char* str;          /* Pointer to step and inputs line */
  char  group[4096];  /* Name of group */
  int   chars_read;   /* Number of characters read from line */

  str = run_cmd_get_block( line );

  if( sscanf( str, "%s%n", group, &chars_read ) == 1 ) {

    str = str + chars_read;

    if( strcmp( "SIM", group ) == 0 ) {
      rc->step = 1;
    } else if( strcmp( "SCORE", group ) == 0 ) {
      rc->step = 2;
    } else if( strcmp( "MERGE", group ) == 0 ) {
      rc->step = 3;
    } else if( strcmp( "REPORT", group ) == 0 ) {
      rc->step = 4;
    }

    while( sscanf( str, "%s%n", group, &chars_read ) == 1 ) {
      str = str + chars_read; 
      list_add( group, 0, &(rc->inputs) );
      if( list_find_str( group, plus_groups ) != -1 ) {
        rc->start = 1;
      }
      if( list_find_str( group, minus_groups ) != -1 ) {
        rc->okay = 0;
      }
    }

  }

}

void run_cmd_add_cmd( char** line, run_cmd* rc ) {

  char* str;  /* Pointer to command portion of line */

  str = run_cmd_get_block( line );

  if( *str != '\0' ) {
    rc->cmd = strdup( str );
  }

}

void run_cmd_add_outputs( char** line, run_cmd* rc ) {

  char* str;           /* Pointer to command portion of line */
  char  output[4096];  /* Name of output file */
  int   chars_read;    /* Number of characters read from line */

  str = run_cmd_get_block( line );

  while( sscanf( str, "%s%n", output, &chars_read ) == 1 ) {
    str = str + chars_read;
    list_add( output, 0, &(rc->outputs) );
  }

}

void run_cmd_add_error( char** line, run_cmd* rc ) {

  char* str;  /* Pointer to command portion of line */
  int   i;    /* Loop iterator */

  if( *line == NULL ) {

    rc->error = 0;

  } else {

    while( (i < strlen( *line )) && ((*line)[i] != '1') ) { i++; }

    if( i == strlen( *line ) ) {
      rc->error = 0;
    } else {
      rc->error = 1;
    }

  }

}

/*!
 \param line     Pointer to current command line to parse
 \param rc_head  Pointer to head of run command list
 \param rc_tail  Pointer to tail of run command list

 Parses the specified line for command information and stores the information in the given diagnostic
 structure.
*/
void run_cmd_add( char* line, run_cmd** rc_head, run_cmd** rc_tail ) {

  run_cmd* rc;  /* Pointer to current run command */

  rc = (run_cmd*)malloc( sizeof( run_cmd ) );
  rc->step     = 0;
  rc->cmd      = NULL;
  rc->inputs   = NULL;
  rc->outputs  = NULL;
  rc->error    = 0;
  rc->start    = 0;
  rc->okay     = 1;
  rc->executed = 0;
  rc->next     = NULL;

  /* Populate the run command */
  run_cmd_add_step_and_inputs( &line, rc );
  run_cmd_add_cmd( &line, rc );
  run_cmd_add_outputs( &line, rc );
  run_cmd_add_error( &line, rc );

  /* Add the run command to the global list */
  if( *rc_head == NULL ) {
    *rc_head = *rc_tail = rc;
  } else {
    (*rc_tail)->next = rc;
    *rc_tail         = rc;
  }

}

/*!
 \param rc  Pointer to run command to execute
 \param d   Pointer to current diagnostic

 \return Returns 0 if command was run without error and valid outputs were verified to be correct.

 Executes the given run command, performing any needed output file checks on this command.  If
 any failures occur, the failed flag is set in the diag structure as well as the failing command.
*/
int run_cmd_execute( run_cmd* rc, diag* d ) {

  int         retval = 0; /* Return value for this function */
  char        cmd[4096];  /* Command to execute */
  char*       gflag;      /* Global flag value from environment */
  int         i;          /* Loop iterator */
  struct stat filestat;   /* Statistics of specified file */

  /* Get global flag value from the environment */
  if( (gflag = getenv( "COVERED_GFLAG" )) == NULL ) {
    gflag = "-Q";
  }

  switch( rc->step ) {
    case 1 :  /* SIM */
      strncpy( cmd, rc->cmd, 4096 );
      break;
    case 2 :  /* SCORE */
      snprintf( cmd, 4096, "../../src/covered %s score %s", gflag, rc->cmd );
      break;
    case 3 :  /* MERGE */
      snprintf( cmd, 4096, "../../src/covered %s merge %s", gflag, rc->cmd );
      break;
    case 4 :  /* REPORT */
      snprintf( cmd, 4096, "../../src/covered %s report %s", gflag, rc->cmd );
      break;
    default:
      exit( 1 );
      break;
  }

  /* Specify that this diagnostic was run and this run command was executed */
  d->ran       = 1;
  rc->executed = 1;

  /* Run system command and if it is successful, do the output compare (if necessary) */
  if( (system( cmd ) >> 8) == rc->error ) {

    for( i=0; i<list_get_size( rc->outputs ); i++ ) {

      /* Calculate diagnostic-provided output filename */
      snprintf( cmd, 4096, "%s.diag", list_get_str( i, rc->outputs ) );

      /* If this file exists in diagnostic form, perform the diff command */
      if( (stat( cmd, &filestat ) == 0) && S_ISREG( filestat.st_mode ) ) {

        /* Create diff command */
        snprintf( cmd, 4096, "diff %s %s.diag", list_get_str( i, rc->outputs ), list_get_str( i, rc->outputs ) );

        /* Perform diff */
        if( system( cmd ) == 0 ) {

          /* Specify that this output file should be removed */
          list_set_num( 1, i, rc->outputs );

        } else {

          list_add( cmd, 0, &(d->failed_cmds) );
          d->failed = 1;
          retval    = 1;

        }

      } else {

        /* Specify that this output file should be removed */
        list_set_num( 1, i, rc->outputs );

      }

    }

  } else {

    list_add( cmd, 0, &(d->failed_cmds) );
    d->failed = 1;
    retval    = 1;
 
  }

  return( retval );

}

/*!
 \param rc  Pointer to run command to remove output files for.

 Removes all output files generated from the specified run command if they exist.
*/
void run_cmd_remove_outputs( run_cmd* rc ) {

  int         i;           /* Loop iterator */
  int         j;           /* Loop iterator */
  char        name[4096];  /* Name of file to remove */
  struct stat filestat;    /* Statistics of specified file */

  for( i=0; i<list_get_size( rc->outputs ); i++ ) {

    for( j=0; j<2; j++ ) {

      /* Generate filename to remove */
      if( j == 0 ) {
        snprintf( name, 4096, "%s.diag", list_get_str( i, rc->outputs ) );
      } else if( list_get_num( i, rc->outputs ) == 1 ) {
        strncpy( name, list_get_str( i, rc->outputs ), 4096 );
      }

      /* If the file exists, remove it */
      if( (stat( name, &filestat ) == 0) && S_ISREG( filestat.st_mode ) ) {
        unlink( name );
      }

    }

  }

}

/*!
 \param rc_head  Pointer to head of run command list to deallocate.

 Deallocates all memory allocated for a given diagnostic run command list.
*/
void run_cmd_dealloc_list( run_cmd* rc_head ) {

  run_cmd* curr;  /* Pointer to current run command element */
  run_cmd* tmp;   /* Temporary pointer to run command element */

  curr = rc_head;

  while( curr != NULL ) {
    
    tmp  = curr;
    curr = curr->next;

    if( tmp->cmd != NULL ) {
      free( tmp->cmd );
    }

    list_dealloc( tmp->inputs );
    list_dealloc( tmp->outputs );

  }

}


/*
 $Log$
 Revision 1.4  2006/03/15 22:48:28  phase1geo
 Updating run program.  Fixing bugs in statement_connect algorithm.  Updating
 regression files.

 Revision 1.3  2006/03/06 22:55:47  phase1geo
 Fixing command-line parser.

 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/

