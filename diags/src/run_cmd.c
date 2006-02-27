/*!
 \file     run_cmd.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/27/2006
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "run_cmd.h"
#include "list.h"


run_cmd* rc_head = NULL;
run_cmd* rc_tail = NULL;

void run_cmd_add_step_and_inputs( char** line, run_cmd* rc ) {

  char* str;          /* Pointer to step and inputs line */
  char  group[4096];  /* Name of group */
  int   chars_read;   /* Number of characters read from line */

  printf( "b line: %s\n", *line );
  str = strsep( line, ":" );
  printf( "a line: %s\n", *line );

  if( sscanf( str, "%s%n", group, &chars_read ) == 2 ) {

    str = str + chars_read;

    rc->step = strdup( group );

    while( sscanf( str, "%s%n", group, &chars_read ) == 2 ) {
      str = str + chars_read; 
      list_add( group, &(rc->inputs) );
    }

  }

}

void run_cmd_add_cmd( char** line, run_cmd* rc ) {

  char* str;  /* Pointer to command portion of line */

  printf( "line: %s\n", *line );
  str = strsep( line, ":" );
  printf( "str: %s\n", str );
  if( *str != '\0' ) {
    rc->cmd = strdup( str );
  }

}

void run_cmd_add_outputs( char** line, run_cmd* rc ) {

  char* str;           /* Pointer to command portion of line */
  char  output[4096];  /* Name of output file */
  int   chars_read;    /* Number of characters read from line */

  str = strsep( line, ":" );

  while( sscanf( str, "%s%n", output, &chars_read ) == 2 ) {
    str = str + chars_read;
    list_add( output, &(rc->outputs) );
  }

}

void run_cmd_add_error( char** line, run_cmd* rc ) {

  char* str;  /* Pointer to command portion of line */
  int   i;    /* Loop iterator */

  while( (i < strlen( *line )) && ((*line)[i] != '1') ) { i++; }

  if( i == strlen( *line ) ) {
    rc->error = 0;
  } else {
    rc->error = 1;
  }

}

void run_cmd_add( char* line ) {

  run_cmd* rc;  /* Pointer to current run command */

  rc = (run_cmd*)malloc( sizeof( run_cmd ) );
  rc->step    = NULL;
  rc->cmd     = NULL;
  rc->inputs  = NULL;
  rc->outputs = NULL;
  rc->next    = NULL;

  printf( "In run_cmd_add, line: %s\n", line );

  /* Populate the run command */
  run_cmd_add_step_and_inputs( &line, rc );
  run_cmd_add_cmd( &line, rc );
  run_cmd_add_outputs( &line, rc );
  run_cmd_add_error( &line, rc );

  /* Add the run command to the global list */
  if( rc_head == NULL ) {
    rc_head = rc_tail = rc;
  } else {
    rc_tail->next = rc;
    rc_tail       = rc;
  }

}

void run_cmd_dealloc_list() {

  run_cmd* curr;  /* Pointer to current run command element */
  run_cmd* tmp;   /* Temporary pointer to run command element */

  curr = rc_head;

  while( curr != NULL ) {
    
    tmp  = curr;
    curr = curr->next;

    /* Deallocate the tmp run command */
    if( tmp->step != NULL ) {
      free( tmp->step );
    }

    if( tmp->cmd != NULL ) {
      free( tmp->cmd );
    }

    list_dealloc( tmp->inputs );
    list_dealloc( tmp->outputs );

  }

}


/*
 $Log$
*/

