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
 \file     cli.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     4/11/2007
*/


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cli.h"
#include "sim.h"
#include "util.h"


extern bool   flag_use_command_line_debug;
extern uint64 curr_sim_time;


/*!
 Specifies the number of statements left to execute before returning to the CLI prompt.
*/
static int    stmts_left = 0;

/*!
 Specifies the number of timesteps left to execute before returning to the CLI prompt.
*/
static int    timesteps_left = 0;

/*!
 Records the last timestep that was seen by the CLI.
*/
static uint64 last_timestep;

/*!
 Specifies if we should run without stopping (ignore stmts_left value)
*/
static bool   dont_stop = FALSE;

/*!
 Specifies if simulator debug information should be output during CLI operation.
*/
bool          cli_debug_mode = FALSE;

/*!
 Array of user-specified command strings.
*/
static char** history = NULL;

/*!
 Index of current command string in history array to store.
*/
static int    history_index = 0;

/*!
 The currently allocated size of the history array.
*/
static int    history_size = 0;


/*!
 Displays CLI usage information to standard output.
*/
void cli_usage() {

  printf( "\n" );
  printf( "Covered score command CLI usage:\n" );
  printf( "\n" );
  printf( "  help             Displays this usage message.\n" );
  printf( "  step [<num>]     Advances to the next statement if <num> is not specified;\n" );
  printf( "                   otherwise, advances <num> statements before returning to\n" );
  printf( "                   the CLI prompt.\n" );
  printf( "  next [<num>]     Advances to the next timestep if <num> is not specified;\n" );
  printf( "                   otherwise, advances <num> timesteps before returning to\n" );
  printf( "                   the CLI prompt.\n" );
  printf( "  run              Runs the simulation.\n" );
  printf( "  continue         Continues running the simulation.\n" );
  printf( "  display <type>   Displays the current state of the given type.  Valid types:\n" );
  printf( "    active_queue     Displays the current state of the active simulation queue.\n" );
  printf( "    delayed_queue    Displays the current state of the delayed simulation queue.\n" );
  printf( "    current          Displays the current scope, block, filename and line number.\n" );
  printf( "    time             Displays the current simulation time.\n" );
  printf( "  debug [on | off] Turns verbose debug output from simulator on or off.  If 'on'\n" );
  printf( "                   or 'off' is not specified, displays the current debug mode.\n" );
  printf( "  history          Displays the command-line history.\n" );
  printf( "  !<num>           Executes the command at the <num> position in history.\n" );
  printf( "  !!               Executes the last valid command.\n" );
  printf( "  savehist <file>  Saves the current history to the specified file.\n" );
  printf( "  readhist <file>  Reads and executes the history stored in the specified file.\n" );
  printf( "  quit             Ends simulation.\n" );
  printf( "\n" );

}

/*!
 \param msg  Message to display as error message.

 Displays the specified error message.
*/
void cli_print_error( char* msg ) {

  printf( "%s.  Type 'help' for usage information.\n", msg );

}

/*!
 \return Returns TRUE if the user specified a valid command; otherwise, returns FALSE

 Parses the given command from the user.
*/
void cli_parse_input( char* line ) {

  char  arg[4096];         /* Holder for user argument */
  bool  valid_cmd = TRUE;  /* Specifies if the given command was valid */
  int   chars_read;        /* Specifies the number of characters that was read from the string */
  int   i;                 /* Iterator */
  FILE* hfile;             /* History file to read or to write */

  /* Resize the history if necessary */
  if( history_index == history_size ) {
    history_size = (history_size == 0) ? 50 : (history_size * 2);
    history      = (char**)realloc( history, (sizeof( char* ) * (history_size * 2)) );
  }

  /* Store this command in the history buffer */
  history[history_index] = line;

  /* Parse first string */
  if( sscanf( line, "%s%n", arg, &chars_read ) == 1 ) {

    line += chars_read;

    printf( "arg: %s\n", arg );
    if( arg[0] == '!' ) {

      line = arg;
      line++;
      if( arg[1] == '!' ) {
        free_safe( history[history_index] );
        cli_parse_input( strdup( history[history_index-1] ) );
      } else if( sscanf( line, "%d", &i ) == 1 ) {
        if( i < (history_index + 1) ) {
          free_safe( history[history_index] );
          cli_parse_input( strdup( history[i-1] ) );
        } else {
          cli_print_error( "Illegal history number" );
          valid_cmd = FALSE;
        }
      } else {
        cli_print_error( "Illegal value to the right of '!'" );
        valid_cmd = FALSE;
      }
            
    } else if( strncmp( "help", arg, 4 ) == 0 ) {

      cli_usage();

    } else if( strncmp( "step", arg, 4 ) == 0 ) {

      if( sscanf( line, "%d", &stmts_left ) != 1 ) {
        stmts_left = 1;
      }

    } else if( strncmp( "next", arg, 4 ) == 0 ) {

      if( sscanf( line, "%d", &timesteps_left ) != 1 ) {
        timesteps_left = 1;
      }

    } else if( strncmp( "run", arg, 3 ) == 0 ) {
  
      /* TBD - We should probably allow a way to restart the simulation??? */
      dont_stop = TRUE;

    } else if( strncmp( "continue", arg, 8 ) == 0 ) {

      dont_stop = TRUE;

    } else if( strncmp( "display", arg, 7 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( strncmp( "active_queue", arg, 12 ) == 0 ) {
          sim_display_active_queue();
        } else if( strncmp( "delayed_queue", arg, 13 ) == 0 ) {
          sim_display_delay_queue();
        } else if( strncmp( "current", arg, 5 ) == 0 ) {
          sim_display_current();
        } else if( strncmp( "time", arg, 4 ) == 0 ) {
          printf( "%lld\n", curr_sim_time );
        } else {
          cli_print_error( "Unknown display type" );
          valid_cmd = FALSE; 
        }
      } else {
        cli_print_error( "Type information missing from display command" );
        valid_cmd = FALSE; 
      }

    } else if( strncmp( "quit", arg, 4 ) == 0 ) {

      exit( 0 );
 
    } else if( strncmp( "debug", arg, 5 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( strncmp( "on", arg, 2 ) == 0 ) {
          cli_debug_mode = TRUE;
        } else if( strncmp( "off", arg, 3 ) == 0 ) {
          cli_debug_mode = FALSE;
        } else {
          cli_print_error( "Unknown debug command parameter" );
          valid_cmd = FALSE; 
        }
      } else {
        if( cli_debug_mode ) {
          printf( "Current debug mode is on.\n" );
        } else {
          printf( "Current debug mode is off.\n" );
        }
      }

    } else if( strncmp( "history", arg, 7 ) == 0 ) {

      printf( "\n" );
      for( i=0; i<=history_index; i++ ) {
        printf( "%7d  %s\n", (i + 1), history[i] );
      }

    } else if( strncmp( "savehist", arg, 8 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( (hfile = fopen( arg, "w" )) != NULL ) {
          for( i=0; i<history_index; i++ ) {
            fprintf( hfile, "%s\n", history[i] );
          }
          fclose( hfile );
        } else {
          cli_print_error( "Unable to write history file" );
          valid_cmd = FALSE;
        }
      } else {
        cli_print_error( "Filename not specified" );
        valid_cmd = FALSE;
      }

    } else if( strncmp( "readhist", arg, 8 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( (hfile = fopen( arg, "r" )) != NULL ) {
          while( util_readline( hfile, &line ) ) {
            cli_parse_input( line );
          }
          fclose( hfile );
        } else {
          cli_print_error( "Unable to read history file" );
          valid_cmd = FALSE;
        }
      } else {
        cli_print_error( "Filename not specified" );
        valid_cmd = FALSE;
      }

    } else {

      cli_print_error( "Unknown command" );
      valid_cmd = FALSE; 

    }

  } else {

    valid_cmd = FALSE;

  }

  /* If the command was valid, increment the history index */
  if( valid_cmd ) {
    history_index++;
 
  /* Otherwise, deallocate the command from the history buffer */
  } else {
    free_safe( history[history_index] );
  }

}

void cli_prompt_user() {

  char* line;        /* Read line from user */
  char  arg[4096];   /* Holder for user argument */
  bool  valid_cmd;   /* Specifies if the given command was valid */
  int   chars_read;  /* Specifies the number of characters that was read from the string */
  int   i;           /* Iterator */

  do {

    /* Prompt the user for input */
    printf( "\ncli %d> ", (history_index + 1) ); 
    fflush( stdout );

    /* Read the user-specified command */
    (void)util_readline( stdin, &line );

    /* Parse the command line */
    cli_parse_input( line );

  } while( (stmts_left == 0) && (timesteps_left == 0) && !dont_stop );

}
/*!
 Performs CLI prompting if necessary.
*/
void cli_execute() {

  char args[3][4096];  /* User-specified command-line args */
  int  argc;           /* Number of arguments specified by the user */
  bool simulate;       /* Specifies if we should continue with simulation */
  int  i;              /* Loop iterator */

  if( flag_use_command_line_debug ) {

    /* Decrement stmts_left if it is set */
    if( stmts_left > 0 ) {
      stmts_left--;
    }

    /* Decrement timesteps_left it is set and the last timestep differs from the current simulation time */
    if( (timesteps_left > 0) && (last_timestep != curr_sim_time) ) {
      timesteps_left--;
    }

    /* Record the last timestep seen */
    last_timestep = curr_sim_time;

    /* If we have no more statements to execute and we are not supposed to continuely run, prompt the user */
    if( (stmts_left == 0) && (timesteps_left == 0) && !dont_stop ) {

      /* Display current line that will be executed */
      sim_display_current_stmt();

      /* Get the next instruction from the user */
      cli_prompt_user();

    }

  }

}

/*
 $Log$
*/

