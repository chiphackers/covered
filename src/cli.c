#ifdef DEBUG_MODE

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
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/11/2007
*/


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cli.h"
#include "codegen.h"
#include "expr.h"
#include "func_unit.h"
#include "instance.h"
#include "link.h"
#include "scope.h"
#include "sim.h"
#include "util.h"
#include "vsignal.h"


/*!
 Specifies the maximum number of dashes to draw for a status bar (Note: This value
 should not exceed 100!)
*/
#define CLI_NUM_DASHES 50


extern db**         db_list;
extern unsigned int curr_db;
extern char         user_msg[USER_MSG_LENGTH];
extern bool         flag_use_command_line_debug;


/*!
 Specifies the number of statements left to execute before returning to the CLI prompt.
*/
static unsigned int stmts_left = 0;

/*!
 Specifies the number of statements to execute (provided by the user).
*/
static unsigned int stmts_specified = 0;

/*!
 Specifies the number of timesteps left to execute before returning to the CLI prompt.
*/
static unsigned int timesteps_left = 0;

/*!
 Specifies the number of timesteps to execute (provided by the user).
*/
static unsigned int timesteps_specified = 0;

/*!
 Specifies the timestep to jump to from here.
*/
static sim_time goto_timestep = {0,0,0,FALSE};

/*!
 Specifies if we should run without stopping (ignore stmts_left value)
*/
static bool dont_stop = FALSE;

/*!
 Specifies if the history buffer needs to be replayed prior to CLI prompting.
*/
static int cli_replay_index = 0;

/*!
 Specifies if simulator debug information should be output during CLI operation.
*/
bool cli_debug_mode = FALSE;

/*!
 Array of user-specified command strings.
*/
static char** history = NULL;

/*!
 Index of current command string in history array to store.
*/
static int history_index = 0;

/*!
 The currently allocated size of the history array.
*/
static int history_size = 0;


/*!
 Displays CLI usage information to standard output.
*/
static void cli_usage() {

  printf( "\n" );
  printf( "Covered score command CLI usage:\n" );
  printf( "\n" );
  printf( "  step [<num>]            Advances to the next statement if <num> is not\n" );
  printf( "                            specified; otherwise, advances <num> statements\n" );
  printf( "                            before returning to the CLI prompt.\n" );
  printf( "  next [<num>]            Advances to the next timestep if <num> is not\n" );
  printf( "                            specified; otherwise, advances <num> timesteps\n" );
  printf( "                            before returning to the CLI prompt.\n" );
  printf( "  goto <num>              Advances to the given timestep (or the next timestep after the\n" );
  printf( "                            given value if the timestep is not executed) specified by <num>.\n" );
  printf( "  run                     Runs the simulation.\n" );
  printf( "  continue                Continues running the simulation.\n" );
  printf( "  thread active            Displays the current state of the active simulation queue.\n" );
  printf( "  thread delayed           Displays the current state of the delayed simulation queue.\n" );
  printf( "  thread all              Displays the list of all threads.\n" );
  printf( "  current                 Displays the current scope, block, filename and line number.\n" );
  printf( "  time                    Displays the current simulation time.\n" );
  printf( "  signal <name>           Displays the current value of the given net/variable.\n" );
  printf( "  expr <num>              Displays the given expression and its current value where <num>\n" );
  printf( "                            is the ID of the expression to output.\n" );
  printf( "  debug [on | off]        Turns verbose debug output from simulator on\n" );
  printf( "                            or off.  If 'on' or 'off' is not specified,\n" );
  printf( "                            displays the current debug mode.\n" );
  printf( "  list [<num>]            Lists the contents of the file where the\n" );
  printf( "                            current statement is to be executed.  If\n" );
  printf( "                            <num> is specified, outputs the given number\n" );
  printf( "                            of lines; otherwise, outputs 10 lines.\n" );
  printf( "  savehist <file>         Saves the current history to the specified file.\n" );
  printf( "  history [(<num> | all)] Displays the last 10 lines of command-line\n" );
  printf( "                            history.  If 'all' is specified, the entire\n" );
  printf( "                            history contents will be displayed.  If <num>\n" );
  printf( "                            is specified, the last <num> commands will be\n" );
  printf( "                            displayed.\n" );
  printf( "  !<num>                  Executes the command at the <num> position in history.\n" );
  printf( "  !!                      Executes the last valid command.\n" );
  printf( "  help                    Displays this usage message.\n" );
  printf( "  quit                    Ends simulation.\n" );
  printf( "\n" );

}

/*!
 \param msg       Message to display as error message.
 \param standard  If set to TRUE, output to standard output.

 Displays the specified error message.
*/
static void cli_print_error( char* msg, bool standard ) {

  if( standard ) {
    printf( "%s.  Type 'help' for usage information.\n", msg );
  }

}

/*!
 Erases a previously drawn status bar from the screen.
*/
static void cli_erase_status_bar( bool clear ) {

  unsigned int i;   /* Loop iterator */
  unsigned int rv;  /* Return value from fflush */

  /* If the user needs the line completely cleared, do so now */
  if( clear ) {
    for( i=0; i<(CLI_NUM_DASHES+2); i++ ) {
      printf( "\b" );
    }
    for( i=0; i<(CLI_NUM_DASHES+2); i++ ) {
      printf( " " );
    }
  }

  for( i=0; i<(CLI_NUM_DASHES+2); i++ ) {
    printf( "\b" );
  }

  rv = fflush( stdout );
  assert( rv == 0 );

}

/*!
 \param percent  Specifies the percentage of completion to show.

 Erases and redraws status bar for a given command (that takes time).
*/
static void cli_draw_status_bar(
  unsigned int percent
) {

  static unsigned int last_percent = 100;  /* Last percent value given */
  unsigned int        i;                   /* Loop iterator */
  unsigned int        rv;                  /* Return value from fflush */

  /* Only redisplay status bar if it needs to be updated */
  if( last_percent != percent ) {

    cli_erase_status_bar( FALSE );

    printf( "|" );

    for( i=0; i<CLI_NUM_DASHES; i++ ) {
      if( percent <= ((100 / CLI_NUM_DASHES) * i) ) {
        printf( " " );
      } else {
        printf( "-" );
      }
    }

    printf( "|" );

    rv = fflush( stdout );
    assert( rv == 0 );

    last_percent = percent;

  }

}

/*!
 Displays the current statement to standard output.
*/
static void cli_display_current_stmt() {

  thread* curr;        /* Pointer to current thread in queue */
  char**  code;        /* Pointer to code string from code generator */
  int     code_depth;  /* Depth of code array */
  int     i;           /* Loop iterator */

  /* Get current thread from simulator */
  curr = sim_current_thread();

  assert( curr != NULL );
  assert( curr->funit != NULL );
  assert( curr->curr != NULL );

  /* Generate the logic */
  codegen_gen_expr( curr->curr->exp, curr->curr->exp->op, &code, &code_depth, curr->funit );

  /* Output the full expression */
  for( i=0; i<code_depth; i++ ) {
    printf( "    %7d:    %s\n", curr->curr->exp->line, code[i] );
    free_safe( code[i], (strlen( code[i] ) + 1) );
  }

  if( code_depth > 0 ) {
    free_safe( code, (sizeof( char* ) * code_depth) );
  }

}

/*!
 Outputs the scope, block name, filename and line number of the current thread in the active queue to standard output.
*/
static void cli_display_current() {

  thread* curr;         /* Pointer to current thread */
  char    scope[4096];  /* String containing scope of given functional unit */
  int     ignore = 0;   /* Specifies that we should not ignore a matching functional unit */

  /* Get current thread from simulator */
  curr = sim_current_thread();

  assert( curr != NULL );
  assert( curr->funit != NULL );
  assert( curr->curr != NULL );

  /* Get the scope of the functional unit represented by the current thread */
  scope[0] = '\0';
  instance_gen_scope( scope, inst_link_find_by_funit( curr->funit, db_list[curr_db]->inst_head, &ignore ), TRUE );

  /* Output the given scope */
  printf( "    SCOPE: %s, BLOCK: %s, FILE: %s\n", scope, funit_flatten_name( curr->funit ), curr->funit->filename );

  /* Display current statement */
  cli_display_current_stmt();

}

/*!
 \param name  Name of signal to display

 \return Returns TRUE if signal was found; otherwise, returns FALSE.

 Outputs the given signal value to standard output.
*/
static bool cli_display_signal( char* name ) {

  bool       retval = TRUE;  /* Return value for this function */
  thread*    curr;           /* Pointer to current thread in simulator */
  vsignal*   sig;            /* Pointer to found signal */
  func_unit* funit;          /* Pointer to found functional unit */

  /* Get current thread from simulator */
  curr = sim_current_thread();

  assert( curr != NULL );
  assert( curr->funit != NULL );
  assert( curr->curr != NULL );

  /* Find the given signal name based on the current scope */
  if( scope_find_signal( name, curr->funit, &sig, &funit, 0 ) ) {

    /* Output the signal contents to standard output */
    printf( "  " );
    vsignal_display( sig );

  } else {

    cli_print_error( "Unable to find specified signal", TRUE );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param id  Expression ID of expression to display

 \param Returns TRUE if expression was found; otherwise, returns FALSE.

 Outputs the given expression and its value to standard output.
*/
static bool cli_display_expression( int id ) {

  bool       retval = TRUE;  /* Return value for this function */
  func_unit* funit;          /* Pointer to functional unit that contains the given expression */

  /* Find the functional unit that contains this expression ID */
  if( (funit = funit_find_by_id( id )) != NULL ) {

    char**       code       = NULL;  /* Code to output */
    int          code_depth = 0;     /* Number of elements in code array */
    unsigned int i;                  /* Loop iterator */
    exp_link*    expl;               /* Pointer to found expression */

    /* Find the expression */
    expl = exp_link_find( id, funit->exp_head );
    assert( expl != NULL );
    assert( expl->exp != NULL );

    /* Output the expression */
    codegen_gen_expr( expl->exp, expl->exp->op, &code, &code_depth, funit );
    assert( code_depth > 0 );
    for( i=0; i<code_depth; i++ ) {
      printf( "    %s\n", code[i] );
      free_safe( code[i], (strlen( code[i] ) + 1) );
    }
    free_safe( code, (sizeof( char* ) * code_depth) );

    /* Output the expression value */
    printf( "\n  " );
    expression_display( expl->exp );

  } else {

    cli_print_error( "Unable to find specified expression", TRUE );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param num  Maximum number of lines to display

 Starting at the current statement line, outputs the next num lines to standard output.
*/
static void cli_display_lines( unsigned num ) {

  thread* curr;        /* Pointer to current thread in simulation */
  FILE*   vfile;       /* File pointer to Verilog file */
  char*   line;        /* Pointer to current line */
  int     line_size;   /* Allocated size of the current line */
  int     lnum = 1;    /* Current line number */
  int     start_line;  /* Starting line */

  /* Get the current thread from the simulator */
  curr = sim_current_thread();

  assert( curr != NULL );
  assert( curr->funit != NULL );
  assert( curr->curr != NULL );

  if( (vfile = fopen( curr->funit->filename, "r" )) != NULL ) {

    /* Get the starting line number */
    start_line = curr->curr->exp->line;

    /* Read the Verilog file and output lines when we are in range */
    while( util_readline( vfile, &line, &line_size ) ) {
      if( (lnum >= start_line) && (lnum < (start_line + num)) ) {
        printf( "    %7d:  %s\n", lnum, line );
      }
      free_safe( line, line_size );
      lnum++;
    }

    fclose( vfile );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open current file: %s", curr->funit->filename );
    assert( rv < USER_MSG_LENGTH );
    cli_print_error( user_msg, TRUE );

  }

}

/*!
 \param line       User-specified command line to parse
 \param perform    Set to TRUE if we should perform the specified command
 \param replaying  Set to TRUE if we are calling this due to replaying the history
 \param time       Pointer to current simulation time

 \return Returns TRUE if the user specified a valid command; otherwise, returns FALSE

 Parses the given command from the user.
*/
static bool cli_parse_input( char* line, bool perform, bool replaying, const sim_time* time ) {

  char     arg[4096];         /* Holder for user argument */
  bool     valid_cmd = TRUE;  /* Specifies if the given command was valid */
  int      chars_read;        /* Specifies the number of characters that was read from the string */
  int      i;                 /* Iterator */
  unsigned num;               /* Unsigned integer value from user */
  FILE*    hfile;             /* History file to read or to write */

  /* Resize the history if necessary */
  if( history_index == history_size ) {
    history_size = (history_size == 0) ? 50 : (history_size * 2);
    history      = (char**)realloc_safe( history, (sizeof( char* ) * (history_size / 2)), (sizeof( char* ) * history_size) );
  }

  /* Store this command in the history buffer if we are not in replay mode */
  if( !replaying ) {
    history[history_index] = line;
  }

  /* Parse first string */
  if( sscanf( line, "%s%n", arg, &chars_read ) == 1 ) {

    line += chars_read;

    if( arg[0] == '!' ) {

      line = arg;
      line++;
      if( arg[1] == '!' ) {
        free_safe( history[history_index], (strlen( history[history_index] ) + 1) );
        (bool)cli_parse_input( strdup_safe( history[history_index-1] ), perform, replaying, time );
        history_index--;
        cli_replay_index--;
      } else if( sscanf( line, "%d", &num ) == 1 ) {
        if( num < (history_index + 1) ) {
          free_safe( history[history_index], (strlen( history[history_index] ) + 1) );
          cli_parse_input( strdup_safe( history[num-1] ), perform, replaying, time );
          history_index--;
          cli_replay_index--;
        } else {
          cli_print_error( "Illegal history number", perform );
          valid_cmd = FALSE;
        }
      } else {
        cli_print_error( "Illegal value to the right of '!'", perform );
        valid_cmd = FALSE;
      }
            
    } else if( strncmp( "help", arg, 4 ) == 0 ) {

      if( perform ) {
        cli_usage();
      }

    } else if( strncmp( "step", arg, 4 ) == 0 ) {

      if( perform ) {
        if( sscanf( line, "%u", &stmts_left ) != 1 ) {
          stmts_left = 1;
        }
        stmts_specified = stmts_left;
      }

    } else if( strncmp( "next", arg, 4 ) == 0 ) {

      if( perform ) {
        if( sscanf( line, "%u", &timesteps_left ) != 1 ) {
          timesteps_left = 1;
        }
        timesteps_specified = timesteps_left;
      }

    } else if( strncmp( "goto", arg, 4 ) == 0 ) {

      if( perform ) {
        uint64 timestep;
        if( sscanf( line, "%llu", &timestep ) != 1 ) {
          cli_print_error( "No timestep specified for goto command", perform );
          valid_cmd = FALSE;
        } else {
          goto_timestep.lo   = (timestep & 0xffffffffLL);
          goto_timestep.hi   = ((timestep >> 32) & 0xffffffffLL);
          goto_timestep.full = timestep;
        }
      }

    } else if( strncmp( "run", arg, 3 ) == 0 ) {
  
      if( perform ) {
        /* TBD - We should probably allow a way to restart the simulation??? */
        dont_stop = TRUE;
      }

    } else if( strncmp( "continue", arg, 8 ) == 0 ) {

      if( perform ) {
        dont_stop = TRUE;
      }

    } else if( strncmp( "thread", arg, 6 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( strncmp( "active", arg, 6 ) == 0 ) {
          if( perform ) {
            sim_display_active_queue();
          }
        } else if( strncmp( "delayed", arg, 7 ) == 0 ) {
          if( perform ) {
            sim_display_delay_queue();
          }
        } else if( strncmp( "all", arg, 3 ) == 0 ) {
          if( perform ) {
            sim_display_all_list();
          }
        } else {
          cli_print_error( "Illegal thread type specified", perform );
          valid_cmd = FALSE;
        }
      } else {
        cli_print_error( "Type information missing from thread command", perform );
        valid_cmd = FALSE;
      }

    } else if( strncmp( "current", arg, 7 ) == 0 ) {

      if( perform ) {
        cli_display_current();
      }

    } else if( strncmp( "time", arg, 4 ) == 0 ) {

      if( perform ) {
        printf( "    TIME: %lld\n", time->full );
      }

    } else if( strncmp( "signal", arg, 6 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( perform ) {
          (void)cli_display_signal( arg );
        }
      } else {
        cli_print_error( "No signal name specified", perform );
        valid_cmd = FALSE;
      }

    } else if (strncmp( "expr", arg, 4 ) == 0 ) {

      int id;
      if( sscanf( line, "%d", &id ) == 1 ) {
        if( perform ) {
          (void)cli_display_expression( id );
        }
      } else {
        cli_print_error( "No expression ID specified", perform );
        valid_cmd = FALSE;
      }

    } else if( strncmp( "quit", arg, 4 ) == 0 ) {

      if( perform ) {
        dont_stop = TRUE;
        sim_finish();
      }
 
    } else if( strncmp( "debug", arg, 5 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( strncmp( "on", arg, 2 ) == 0 ) {
          if( perform ) {
            cli_debug_mode = TRUE;
            set_debug( TRUE );
          }
        } else if( strncmp( "off", arg, 3 ) == 0 ) {
          if( perform ) {
            cli_debug_mode = FALSE;
          }
        } else {
          cli_print_error( "Unknown debug command parameter", perform );
          valid_cmd = FALSE; 
        }
      } else {
        if( perform ) {
          if( cli_debug_mode ) {
            printf( "Current debug mode is on.\n" );
          } else {
            printf( "Current debug mode is off.\n" );
          }
        }
      }

    } else if( strncmp( "history", arg, 7 ) == 0 ) {

      i = (history_index - 9);
      if( sscanf( line, "%d", &num ) == 1 ) {
        i = (history_index - (num - 1));
      } else if( sscanf( line, "%s", arg ) == 1 ) {
        if( strncmp( "all", arg, 3 ) == 0 ) {
          i = 0;
        }
      }
      if( perform ) {
        printf( "\n" );
        for( i=((i<0)?0:i); i<=history_index; i++ ) {
          printf( "%7d  %s\n", (i + 1), history[i] );
        }
      }

    } else if( strncmp( "savehist", arg, 8 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( perform ) {
          if( (hfile = fopen( arg, "w" )) != NULL ) {
            for( i=0; i<history_index; i++ ) {
              fprintf( hfile, "%s\n", history[i] );
            }
            fclose( hfile );
            printf( "History saved to file '%s'\n", arg );
          } else {
            cli_print_error( "Unable to write history file", perform );
            valid_cmd = FALSE;
          }
        }
      } else {
        cli_print_error( "Filename not specified", perform );
        valid_cmd = FALSE;
      }

    } else if( strncmp( "list", arg, 4 ) == 0 ) {

      if( perform ) {
        if( sscanf( line, "%u", &num ) == 1 ) {
          cli_display_lines( num );
        } else {
          cli_display_lines( 10 );
        }
      }

    } else {

      cli_print_error( "Unknown command", perform );
      valid_cmd = FALSE; 

    }

  } else {

    valid_cmd = FALSE;

  }

  /* If the command was valid, increment the history index */
  if( !replaying ) {
    if( valid_cmd ) {
      history_index++;
 
    /* Otherwise, deallocate the command from the history buffer */
    } else {
      free_safe( history[history_index], (sizeof( history[history_index] ) + 1) );
    }
  }
  
  /* Always increment the replay index if we are performing */
  if( perform ) {
    cli_replay_index++;  
  }

  return( valid_cmd );

}

/*!
 Takes care of either replaying the history buffer or prompting the user for the next command
 to be issued.
*/
static void cli_prompt_user(
  const sim_time* time  /*!< Pointer to current simulation time */
) {

  char* line;       /* Read line from user */
  int   line_size;  /* Allocated byte size of read line from user */

  do {

    /* If the history buffer still needs to be replayed, do so instead of prompting the user */
    if( cli_replay_index < history_index ) {

      printf( "\ncli %d> %s\n", (cli_replay_index + 1), history[cli_replay_index] );

      (void)cli_parse_input( history[cli_replay_index], TRUE, TRUE, time );

    } else {

      /* Prompt the user for input */
      printf( "\ncli %d> ", (history_index + 1) ); 
      fflush( stdout );

      /* Read the user-specified command */
      (void)util_readline( stdin, &line, &line_size );

      /* Parse the command line */
      (void)cli_parse_input( line, TRUE, FALSE, time );

      /* Deallocate the memory allocated for the read line */
      free_safe( line, line_size );

    }

  } while( (stmts_left == 0) && (timesteps_left == 0) && TIME_CMP_GE(*time, goto_timestep) && !dont_stop );

}

/*!
 Resets CLI conditions to pre-simulation values.
*/
void cli_reset(
  const sim_time* time
) {

  stmts_left          = 0;
  timesteps_left      = 0;
  goto_timestep.lo    = time->lo;
  goto_timestep.hi    = time->hi;
  goto_timestep.full  = time->full;
  goto_timestep.final = time->final;
  dont_stop           = FALSE;

}

/*!
 \param time   Pointer to current simulation time.
 \param force  Forces us to provide a CLI prompt.

 Performs CLI prompting if necessary.
*/
void cli_execute(
  const sim_time* time,
  bool            force
) {

  static sim_time last_timestep = {0,0,0,FALSE};

  if( flag_use_command_line_debug || force ) {

    /* If we are forcing a CLI prompt, reset all outstanding counters, times, etc. */
    if( force ) {
      cli_reset( time );
    }

    /* Decrement stmts_left if it is set */
    if( stmts_left > 0 ) {
      stmts_left--;
    }

    /* If the given time is not 0, possibly decrement the number of timesteps left */
    if( (time->hi!=0) || (time->lo!=0) ) {

      /* Decrement timesteps_left it is set and the last timestep differs from the current simulation time */
      if( (timesteps_left > 0) && TIME_CMP_NE(last_timestep, *time) ) {
        timesteps_left--;
      }

      /* Record the last timestep seen */
      last_timestep = *time;

    }

    /* If we have no more statements to execute and we are not supposed to continuely run, prompt the user */
    if( (stmts_left == 0) && (timesteps_left == 0) && TIME_CMP_GE(*time, goto_timestep) && !dont_stop ) {

      /* Erase the status bar */
      cli_erase_status_bar( TRUE );

      /* Display current line that will be executed if we are not replaying */
      if( cli_replay_index == history_index ) {
        cli_display_current_stmt();
      }

      /* Get the next instruction from the user */
      cli_prompt_user( time );

    /* Otherwise, potentially display a status bar */
    } else {

      if( stmts_left > 0 ) {
        cli_draw_status_bar( ((stmts_specified - stmts_left) * 100) / stmts_specified );
      } else if ( timesteps_left > 0 ) {
        cli_draw_status_bar( ((timesteps_specified - timesteps_left) * 100) / timesteps_specified );
      } else if ( TIME_CMP_GT(goto_timestep, *time) ) {
        cli_draw_status_bar( 100 - (((goto_timestep.full - time->full) * 100) / goto_timestep.full) );
      }

    }

  }

}

/*!
 \param fname  Name of history file to read in

 Reads the contents of the CLI history file and parses its information, storing it in a replay
 buffer when simulation starts.
*/
void cli_read_hist_file( const char* fname ) {

  char* line;       /* Holds current line read from history file */
  int   line_size;  /* Allocated bytes for read line */
  FILE* hfile;      /* File containing history file */

  /* Make sure that this function was not called twice */
  assert( (cli_replay_index == 0) && !flag_use_command_line_debug );

  /* Read the contents of the history file, storing the read lines into the history array */
  if( (hfile = fopen( fname, "r" )) != NULL ) {

    Try {

      sim_time time;

      while( util_readline( hfile, &line, &line_size ) ) {
        if( !cli_parse_input( line, FALSE, FALSE, &time ) ) {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Specified -cli file \"%s\" is not a valid CLI history file", fname );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          printf( "cli Throw A\n" );
          Throw 0;
        }
        free_safe( line, line_size );
      }

    } Catch_anonymous {
      unsigned int rv = fclose( hfile );
      assert( rv == 0 );
      printf( "cli Throw B\n" );
      Throw 0;
    }

    fclose( hfile );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to read history file \"%s\"", fname );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    printf( "cli Throw C\n" );
    Throw 0;

  }

}

/*
 $Log$
 Revision 1.24  2008/04/15 20:37:07  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.23  2008/03/17 22:02:30  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.22  2008/03/17 05:26:15  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.21  2008/03/14 22:00:17  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.20  2008/02/29 00:08:31  phase1geo
 Completed optimization code in simulator.  Still need to verify that code
 changes enhanced performances as desired.  Checkpointing.

 Revision 1.19  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.18  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.17  2008/01/21 21:39:55  phase1geo
 Bug fix for bug 1876376.

 Revision 1.16  2008/01/20 04:39:42  phase1geo
 Renaming commands in CLI to make things easier to remember and shorter to type.
 Added CLI expr command that outputs a given expression and its current state/value.
 Updated documentation per CLI changes.  Also updated build scripts and user
 documentation for upcoming development release.

 Revision 1.15  2008/01/18 23:37:27  phase1geo
 Fixing some issues surrounding the use of the !! command.

 Revision 1.14  2008/01/18 05:19:01  phase1geo
 Fixing output glitch from status bar.

 Revision 1.13  2008/01/18 05:03:14  phase1geo
 Fixing bug in CLI that didn't stop the CLI prompt at the right location.
 Added "goto" command to allow us to simply simulate to a specific timestep.
 Added status bar to indicate to the user how much we have gotten to our
 simulation goal for the CLI.

 Revision 1.12  2008/01/15 23:04:27  phase1geo
 A few more updates.

 Revision 1.11  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.10  2007/12/19 22:54:34  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.9  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.8  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.7  2007/04/18 22:34:58  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.6  2007/04/13 21:47:12  phase1geo
 More simulation debugging.  Added 'display all_list' command to CLI to output
 the list of all threads.  Updated regressions though we are not fully passing
 at the moment.  Checkpointing.

 Revision 1.5  2007/04/12 20:54:55  phase1geo
 Adding cli > output when replaying and adding back all of the functions (since
 the cli > prompt helps give it context.  Fixing bugs in simulation core.
 Checkpointing.

 Revision 1.4  2007/04/12 14:28:35  phase1geo
 Adding ability to display the last <num> lines of history to the history
 command.  Changed behavior of 'display active_queue' and 'display delayed_queue'
 commands to output when replaying history.  Also modified look of the help
 output to hopefully make it easier to read.

 Revision 1.3  2007/04/12 04:15:40  phase1geo
 Adding history all command, added list command and updated the display current
 command to include statement output.

 Revision 1.2  2007/04/12 03:46:30  phase1geo
 Fixing bugs with CLI.  History and history file saving/loading is implemented
 and working as desired.

 Revision 1.1  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

*/

#endif
