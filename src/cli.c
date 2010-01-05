#ifdef DEBUG_MODE

/*
 Copyright (c) 2006-2010 Trevor Williams

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
#include "vector.h"
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
extern bool         debug_mode;


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
static unsigned int cli_replay_index = 0;

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
static unsigned int history_index = 0;

/*!
 The currently allocated size of the history array.
*/
static unsigned int history_size = 0;

/*!
 The current find vector (signal or expression in the design).
*/
static vector* cli_goto_vec1 = NULL;

/*!
 Comparison operator for the find vectors.
*/
static exp_op_type cli_goto_op;

/*!
 User-specified value to compare against.  If this value is NULL, no comparison is performed.  If the compare occurs and
 returns TRUE, this vector MUST be deallocated.
*/
static vector* cli_goto_vec2 = NULL;

/*!
 One-bit target vector to store comparison results into.
*/
static vector* cli_goto_vec = NULL;

/*
 Specifies the name of the file containing the line number to simulate to.  If this value is not NULL, we
 should be simulating until the line number matches.
*/
static char* cli_goto_filename = NULL;

/*!
 Specifies the line number to find.  The value is valid if cli_goto_filename is not NULL.
*/
static int cli_goto_linenum;


/*!
 Displays CLI usage information to standard output.
*/
static void cli_usage() {

  printf( "\n" );
  printf( "Covered score command CLI usage:\n" );
  printf( "\n" );
  printf( "  step [<num>]\n" );
  printf( "      Advances to the next statement if <num> is not specified; otherwise, advances <num> statements\n" );
  printf( "      before returning to the CLI prompt.\n\n" );
  printf( "  next [<num>]\n" );
  printf( "      Advances to the next timestep if <num> is not specified; otherwise, advances <num> timesteps\n" );
  printf( "      before returning to the CLI prompt.\n\n" );
  printf( "  goto time <num>\n" );
  printf( "      Simulates to the given timestep (or the next timestep after the given value if the timestep is\n" );
  printf( "      not executed) specified by <num>.\n\n" );
  printf( "  goto line [<filename>:]<num>\n" );
  printf( "      Simulates until the given line number is about to be executed.  If <filename> is specified, the\n" );
  printf( "      specified filename is used for searching; otherwise, the currently executing filename is used.\n\n" );
  printf( "  goto expr (<name>|<num>) (!=|!==|==|===|<|>|<=|>=) <num>\n" );
  printf( "      Simulates to the point where the given signal (<name>) or expression <num> evaluates to be true\n" );
  printf( "      with the given value and operation.\n\n" );
  printf( "  run\n" );
  printf( "      Runs the simulation.\n\n" );
  printf( "  continue\n" );
  printf( "      Continues running the simulation.\n\n" );
  printf( "  thread active\n" );
  printf( "      Displays the current state of the active simulation queue.\n\n" );
  printf( "  thread delayed\n" );
  printf( "      Displays the current state of the delayed simulation queue.\n\n" );
  printf( "  thread all\n" );
  printf( "      Displays the list of all threads.\n\n" );
  printf( "  current\n" );
  printf( "      Displays the current scope, block, filename and line number.\n\n" );
  printf( "  time\n" );
  printf( "      Displays the current simulation time.\n\n" );
  printf( "  signal <name>\n" );
  printf( "      Displays the current value of the given net/variable.\n\n" );
  printf( "  expr <num>\n" );
  printf( "      Displays the given expression and its current value where <num> is the ID of the expression to output.\n\n" );
  printf( "  debug [less | more | off]\n" );
  printf( "      Turns verbose debug output from simulator on or off.  If 'less' is specified, only the executed expression\n" );
  printf( "      is output.  If 'more' is specified, the full debug information from internal debug statements in Covered is\n" );
  printf( "      is output.  If 'less', 'more' or 'off' is not specified, displays the current debug mode.\n\n" );
  printf( "  list [<num>]\n" );
  printf( "      Lists the contents of the file where the current statement is to be executed.  If <num> is specified,\n" );
  printf( "      outputs the given number of lines; otherwise, outputs 10 lines.\n\n" );
  printf( "  savehist <file>\n" );
  printf( "      Saves the current history to the specified file.\n\n" );
  printf( "  history [(<num> | all)]\n" );
  printf( "      Displays the last 10 lines of command-line history.  If 'all' is specified, the entire history contents\n" );
  printf( "      will be displayed.  If <num> is specified, the last <num> commands will be displayed.\n\n" );
  printf( "  !<num>\n" );
  printf( "      Executes the command at the <num> position in history.\n\n" );
  printf( "  !!\n" );
  printf( "      Executes the last valid command.\n\n" );
  printf( "  help\n" );
  printf( "      Displays this usage message.\n\n" );
  printf( "  quit\n" );
  printf( "      Ends simulation.\n" );
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
static void cli_display_current_stmt(
  statement* curr_stmt  /*!< Pointer to current statement */
) {

  thread*      curr_thr;    /* Pointer to current thread in queue */
  char**       code;        /* Pointer to code string from code generator */
  unsigned int code_depth;  /* Depth of code array */
  unsigned int i;           /* Loop iterator */

  /* Get current thread from simulator */
  curr_thr = sim_current_thread();

  assert( curr_thr != NULL );
  assert( curr_thr->funit != NULL );

  /* Generate the logic */
  codegen_gen_expr( curr_stmt->exp, curr_thr->funit, &code, &code_depth );

  /* Output the full expression */
  for( i=0; i<code_depth; i++ ) {
    printf( "    %7d:    %s\n", curr_stmt->exp->line, code[i] );
    free_safe( code[i], (strlen( code[i] ) + 1) );
  }

  if( code_depth > 0 ) {
    free_safe( code, (sizeof( char* ) * code_depth) );
  }

}

/*!
 Outputs the scope, block name, filename and line number of the current thread in the active queue to standard output.
*/
static void cli_display_current(
  statement* curr_stmt  /*!< Pointer to current statement */
) {

  thread* curr;         /* Pointer to current thread */
  char    scope[4096];  /* String containing scope of given functional unit */
  int     ignore = 0;   /* Specifies that we should not ignore a matching functional unit */

  /* Get current thread from simulator */
  curr = sim_current_thread();

  assert( curr != NULL );
  assert( curr->funit != NULL );

  /* Get the scope of the functional unit represented by the current thread */
  scope[0] = '\0';
  instance_gen_scope( scope, inst_link_find_by_funit( curr->funit, db_list[curr_db]->inst_head, &ignore ), TRUE );

  /* Output the given scope */
  printf( "    SCOPE: %s, BLOCK: %s, FILE: %s\n", scope, funit_flatten_name( curr->funit ), curr->funit->orig_fname );

  /* Display current statement */
  cli_display_current_stmt( curr_stmt );

}

/*!
 \return Returns TRUE if signal was found; otherwise, returns FALSE.

 Outputs the given signal value to standard output.
*/
static bool cli_display_signal(
  char* name  /*!< Name of signal to display */
) {

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
 \param Returns TRUE if expression was found; otherwise, returns FALSE.

 Outputs the given expression and its value to standard output.
*/
static bool cli_display_expression(
  int id  /*!< Expression ID of expression to display */
) {

  bool       retval = TRUE;  /* Return value for this function */
  func_unit* funit;          /* Pointer to functional unit that contains the given expression */

  /* Find the functional unit that contains this expression ID */
  if( (funit = funit_find_by_id( id )) != NULL ) {

    char**       code       = NULL;  /* Code to output */
    unsigned int code_depth = 0;     /* Number of elements in code array */
    unsigned int i;                  /* Loop iterator */
    expression*  exp;                /* Pointer to found expression */

    /* Find the expression */
    exp = exp_link_find( id, funit->exps, funit->exp_size );
    assert( exp != NULL );

    /* Output the expression */
    codegen_gen_expr( exp, funit, &code, &code_depth );
    assert( code_depth > 0 );
    for( i=0; i<code_depth; i++ ) {
      printf( "    %s\n", code[i] );
      free_safe( code[i], (strlen( code[i] ) + 1) );
    }
    free_safe( code, (sizeof( char* ) * code_depth) );

    /* Output the expression value */
    printf( "\n  " );
    expression_display( exp );

  } else {

    cli_print_error( "Unable to find specified expression", TRUE );
    retval = FALSE;

  }

  return( retval );

}

/*!
 Starting at the current statement line, outputs the next num lines to standard output.
*/
static void cli_display_lines(
  unsigned num  /*!< Maximum number of lines to display */
) {

  thread*      curr;         /* Pointer to current thread in simulation */
  FILE*        vfile;        /* File pointer to Verilog file */
  char*        line = NULL;  /* Pointer to current line */
  unsigned int line_size;    /* Allocated size of the current line */
  unsigned int lnum = 1;     /* Current line number */
  unsigned int start_line;   /* Starting line */

  /* Get the current thread from the simulator */
  curr = sim_current_thread();

  assert( curr != NULL );
  assert( curr->funit != NULL );
  assert( curr->curr != NULL );

  if( (vfile = fopen( curr->funit->orig_fname, "r" )) != NULL ) {

    /* Get the starting line number */
    start_line = curr->curr->exp->line;

    /* Read the Verilog file and output lines when we are in range */
    while( util_readline( vfile, &line, &line_size ) ) {
      if( (lnum >= start_line) && (lnum < (start_line + num)) ) {
        printf( "    %7d:  %s\n", lnum, line );
      }
      lnum++;
    }

    fclose( vfile );

    /* Deallocate memory */
    free_safe( line, line_size );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open current file: %s", curr->funit->orig_fname );
    assert( rv < USER_MSG_LENGTH );
    cli_print_error( user_msg, TRUE );

  }

}

/*!
 \return Returns TRUE if the user specified a valid command; otherwise, returns FALSE

 Parses the given command from the user.
*/
static bool cli_parse_input(
  char*           line,       /*!< User-specified command line to parse */
  bool            perform,    /*!< Set to TRUE if we should perform the specified command */
  bool            replaying,  /*!< Set to TRUE if we are calling this due to replaying the history */
  const sim_time* time,       /*!< Pointer to current simulation time */
  statement*      curr_stmt   /*!< Pointer to current statement */
) {

  char     arg[4096];         /* Holder for user argument */
  bool     valid_cmd = TRUE;  /* Specifies if the given command was valid */
  int      chars_read;        /* Specifies the number of characters that was read from the string */
  unsigned num;               /* Unsigned integer value from user */
  FILE*    hfile;             /* History file to read or to write */
  thread*  curr_thr;          /* Pointer to current thread */

  /* Resize the history if necessary */
  if( history_index == history_size ) {
    history_size = (history_size == 0) ? 50 : (history_size * 2);
    history      = (char**)realloc_safe( history, (sizeof( char* ) * (history_size / 2)), (sizeof( char* ) * history_size) );
  }

  /* Store this command in the history buffer if we are not in replay mode */
  if( !replaying ) {
    history[history_index] = strdup_safe( line );
  }

  /* Parse first string */
  if( sscanf( line, "%s%n", arg, &chars_read ) == 1 ) {

    line += chars_read;

    if( arg[0] == '!' ) {

      line = arg;
      line++;
      if( arg[1] == '!' ) {
        free_safe( history[history_index], (strlen( history[history_index] ) + 1) );
        (bool)cli_parse_input( history[history_index-1], perform, replaying, time, curr_stmt );
        history_index--;
        cli_replay_index--;
      } else if( sscanf( line, "%d", &num ) == 1 ) {
        if( num < (history_index + 1) ) {
          free_safe( history[history_index], (strlen( history[history_index] ) + 1) );
          cli_parse_input( history[num-1], perform, replaying, time, curr_stmt );
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

      if( sscanf( line, "%s%n", arg, &chars_read ) == 1 ) {
        line += chars_read;
        if( strncmp( "time", arg, 4 ) == 0 ) {
          if( perform ) {
            uint64 timestep;
            if( sscanf( line, "%" FMT64 "u", &timestep ) != 1 ) {
              cli_print_error( "No timestep specified for goto command", perform );
              valid_cmd = FALSE;
            } else {
              goto_timestep.lo   = (timestep & 0xffffffffLL);
              goto_timestep.hi   = ((timestep >> 32) & 0xffffffffLL);
              goto_timestep.full = timestep;
            }
          }
        } else if( strncmp( "expr", arg, 4 ) == 0 ) {
          if( perform ) {
            vsignal*   sig;
            func_unit* funit;
            int        id;
            int        base;
            if( sscanf( line, "%s%n", arg, &chars_read ) == 1 ) {
              line += chars_read;
              curr_thr = sim_current_thread();
              if( scope_find_signal( arg, curr_thr->funit, &sig, &funit, 0 ) ) {
                cli_goto_vec1 = sig->value;
              } else {
                cli_print_error( "Unable to find signal in find expression", perform );
                valid_cmd = FALSE;
              }
            } else if( sscanf( line, "%d%n", &id, &chars_read ) == 1 ) {
              if( (funit = funit_find_by_id( id )) != NULL ) {
                expression* exp = exp_link_find( id, funit->exps, funit->exp_size );
                assert( exp != NULL );
                cli_goto_vec1 = exp->value;
              } else {
                cli_print_error( "Unable to find expression ID in find expression", perform );
                valid_cmd = FALSE;
              }
            } else {
              cli_print_error( "Illegal find expression specified", perform );
              valid_cmd = FALSE;
            }
            if( valid_cmd ) {
              if( sscanf( line, "%s%n", arg, &chars_read ) == 1 ) {
                line += chars_read;
                if( strcmp( "==", arg ) == 0 ) {
                  cli_goto_op = EXP_OP_EQ;
                } else if( strcmp( "===", arg ) == 0 ) {
                  cli_goto_op = EXP_OP_CEQ;
                } else if( strcmp( "!=", arg ) == 0 ) {
                  cli_goto_op = EXP_OP_NE;
                } else if( strcmp( "!==", arg ) == 0 ) {
                  cli_goto_op = EXP_OP_CNE;
                } else if( strcmp( "<=", arg ) == 0 ) {
                  cli_goto_op = EXP_OP_LE;
                } else if( strcmp( "<",  arg ) == 0 ) {
                  cli_goto_op = EXP_OP_LT;
                } else if( strcmp( ">=", arg ) == 0 ) {
                  cli_goto_op = EXP_OP_GE;
                } else if( strcmp( ">",  arg ) == 0 ) {
                  cli_goto_op = EXP_OP_GT;
                } else {
                  cli_print_error( "Illegal expression operator specified", perform );
                  valid_cmd = FALSE;
                }
              } else {
                cli_print_error( "Illegal find expression specified", perform );
                valid_cmd = FALSE;
              }
            }
            if( valid_cmd ) {
              if( sscanf( line, "%s", arg ) == 1 ) {
                char* tmpstr = arg;
                vector_from_string( &tmpstr, FALSE, &cli_goto_vec2, &base );
                if( cli_goto_vec2 != NULL ) {
                  cli_goto_vec = vector_create( 1, VTYPE_VAL, VDATA_UL, TRUE );
                } else {
                  cli_print_error( "Illegal value string specified", perform );
                  valid_cmd = FALSE;
                }
              } else {
                cli_print_error( "Illegal find expression specified", perform );
                valid_cmd = FALSE;
              }
            }
          }
        } else if( strncmp( "line", arg, 4 ) == 0 ) {
          if( perform ) {
            if( sscanf( line, "%d", &cli_goto_linenum ) == 1 ) {
              curr_thr = sim_current_thread();
              cli_goto_filename = strdup_safe( curr_thr->funit->orig_fname );
            } else if( sscanf( line, "%s", arg ) == 1 ) {
              char targ[4096];
              strcpy( targ, arg );
              if( sscanf( targ, "%[^:]:%d", arg, &cli_goto_linenum ) == 2 ) {
                cli_goto_filename = strdup_safe( arg );
              } else {
                cli_print_error( "Illegal line number specified", perform );
                valid_cmd = FALSE;
              }
            } else {
              cli_print_error( "Illegal line number specified", perform );
              valid_cmd = FALSE;
            }
          }
        } else {
          cli_print_error( "Unknown goto type specified", perform );
          valid_cmd = FALSE;
        }
      } else {
        cli_print_error( "No goto type was specified", perform );
        valid_cmd = FALSE;
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
        cli_display_current( curr_stmt );
      }

    } else if( strncmp( "time", arg, 4 ) == 0 ) {

      if( perform ) {
        printf( "    TIME: %" FMT64 "u\n", time->full );
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
        if( strncmp( "less", arg, 4 ) == 0 ) {
          if( perform ) {
            cli_debug_mode = TRUE;
            set_debug( FALSE );
          }
        } else if( strncmp( "more", arg, 4 ) == 0 ) {
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
      }
      if( perform ) {
        if( cli_debug_mode ) {
          if( debug_mode ) { 
            printf( "Current debug mode is 'more'.\n" );
          } else {
            printf( "Current debug mode is 'less'.\n" );
          }
        } else {
          printf( "Current debug mode is 'off'.\n" );
        }
      }

    } else if( strncmp( "history", arg, 7 ) == 0 ) {

      int i = (history_index - 9);
      if( sscanf( line, "%d", &num ) == 1 ) {
        i = (history_index - (num - 1));
      } else if( sscanf( line, "%s", arg ) == 1 ) {
        if( strncmp( "all", arg, 3 ) == 0 ) {
          i = 0;
        }
      }
      if( perform ) {
        printf( "\n" );
        for( i=((i<0)?0:i); i<=(int)history_index; i++ ) {
          printf( "%7d  %s\n", (i + 1), history[i] );
        }
      }

    } else if( strncmp( "savehist", arg, 8 ) == 0 ) {

      if( sscanf( line, "%s", arg ) == 1 ) {
        if( perform ) {
          if( (hfile = fopen( arg, "w" )) != NULL ) {
            unsigned int i;
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
  const sim_time* time,      /*!< Pointer to current simulation time */
  statement*      curr_stmt  /*!< Pointer to current statement to be executed */
) {

  do {

    /* If the history buffer still needs to be replayed, do so instead of prompting the user */
    if( cli_replay_index < history_index ) {

      printf( "\ncli %d> %s\n", (cli_replay_index + 1), history[cli_replay_index] );

      (void)cli_parse_input( history[cli_replay_index], TRUE, TRUE, time, curr_stmt );

    } else {

      char*        line;       /* Read line from user */
      unsigned int line_size;  /* Allocated byte size of read line from user */

      /* Prompt the user for input */
      printf( "\ncli %d> ", (history_index + 1) ); 
      fflush( stdout );

      /* Read the user-specified command */
      (void)util_readline( stdin, &line, &line_size );

      /* Parse the command line */
      (void)cli_parse_input( line, TRUE, FALSE, time, curr_stmt );

      /* Deallocate the memory allocated for the read line */
      free_safe( line, line_size );

    }

  } while( (stmts_left == 0) && (timesteps_left == 0) && (cli_goto_vec == NULL) && (cli_goto_filename == NULL) && TIME_CMP_GE(*time, goto_timestep) && !dont_stop );

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
  cli_goto_vec1       = NULL;
  vector_dealloc( cli_goto_vec2 );
  cli_goto_vec2       = NULL;
  vector_dealloc( cli_goto_vec );
  cli_goto_vec        = NULL;
  free_safe( cli_goto_filename, (strlen( cli_goto_filename ) + 1) );
  cli_goto_filename   = NULL;

}

/*!
 Signal handler for Ctrl-C event.
*/
void cli_ctrl_c(
  int sig  /*!< Signal that was received */
) {

  thread* curr_thr = sim_current_thread();

  /* Display a message to the user */
  printf( "\nCtrl-C interrupt encountered.  Stopping current command.\n" );

  /* Reset the CLI - this will cause the CLI prompt to be displayed ASAP */
  cli_reset( &curr_thr->curr_time );

}

/*!
 \param time   Pointer to current simulation time.
 \param force  Forces us to provide a CLI prompt.

 Performs CLI prompting if necessary.
*/
void cli_execute(
  const sim_time* time,      /*!< Pointer to current simulation time */
  bool            force,     /*!< Forces us to provide a CLI prompt */
  statement*      curr_stmt  /*!< Pointer to statement that is about to be executed */
) {

  static sim_time last_timestep = {0,0,0,FALSE};
  bool            new_timestep  = FALSE;

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

      if( TIME_CMP_NE(last_timestep, *time) ) {

        new_timestep = TRUE;

        /* Decrement timesteps_left it is set and the last timestep differs from the current simulation time */
        if( timesteps_left > 0 ) {
          timesteps_left--;
        }

        /* Record the last timestep seen */
        last_timestep = *time;

      }

    }

    /* If the find expression was in progress, check to see if it evaluates to TRUE */
    if( cli_goto_vec != NULL ) {
      vector tgt;
      switch( cli_goto_op ) {
        case EXP_OP_EQ  :  vector_op_eq(  cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_CEQ :  vector_op_ceq( cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_NE  :  vector_op_ne(  cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_CNE :  vector_op_cne( cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_LE  :  vector_op_le(  cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_LT  :  vector_op_lt(  cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_GE  :  vector_op_ge(  cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
        case EXP_OP_GT  :  vector_op_gt(  cli_goto_vec, cli_goto_vec1, cli_goto_vec2 );  break;
      }
      if( !vector_is_unknown( cli_goto_vec ) && vector_is_not_zero( cli_goto_vec ) ) {
        vector_dealloc( cli_goto_vec2 );  cli_goto_vec2 = NULL;
        vector_dealloc( cli_goto_vec );   cli_goto_vec  = NULL;
      }
    }

    /* If we are looking for a line number, check for it */
    if( cli_goto_filename != NULL ) {
      thread* thr = sim_current_thread();
      if( (strcmp( thr->funit->orig_fname, cli_goto_filename ) == 0) && (cli_goto_linenum == curr_stmt->exp->line) ) {
        free_safe( cli_goto_filename, (strlen( cli_goto_filename ) + 1) );
        cli_goto_filename = NULL;
      }
    }

    /*
     If we have no more statements to execute, timesteps to execute, comparisons to execute, and we
     are not supposed to continuely run, prompt the user.
    */
    if( (stmts_left == 0) && (timesteps_left == 0) && (cli_goto_vec2 == NULL) && (cli_goto_filename == NULL) && TIME_CMP_GE(*time, goto_timestep) && !dont_stop ) {

      /* Erase the status bar */
      cli_erase_status_bar( TRUE );

      /* Display current line that will be executed if we are not replaying */
      if( cli_replay_index == history_index ) {
        cli_display_current_stmt( curr_stmt );
      }

      /* Get the next instruction from the user */
      cli_prompt_user( time, curr_stmt );

    /* Otherwise, potentially display a status bar */
    } else {

      /* Only draw the status bar if we are not in debug mode */
      if( cli_debug_mode ) {
        if( !debug_mode && (cli_replay_index == history_index) ) {
          if( new_timestep ) {
            printf( "  TIME: %" FMT64 "u\n", time->full );
          }
          cli_display_current_stmt( curr_stmt );
        }
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

}

/*!
 \param fname  Name of history file to read in

 Reads the contents of the CLI history file and parses its information, storing it in a replay
 buffer when simulation starts.
*/
void cli_read_hist_file( const char* fname ) {

  FILE* hfile;      /* File containing history file */

  /* Make sure that this function was not called twice */
  assert( (cli_replay_index == 0) && !flag_use_command_line_debug );

  /* Read the contents of the history file, storing the read lines into the history array */
  if( (hfile = fopen( fname, "r" )) != NULL ) {

    Try {

      sim_time     time;         /* Temporary simulation time holder */
      char*        line = NULL;  /* Holds current line read from history file */
      unsigned int line_size;    /* Allocated bytes for read line */

      while( util_readline( hfile, &line, &line_size ) ) {
        if( !cli_parse_input( line, FALSE, FALSE, &time, NULL ) ) {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Specified -cli file \"%s\" is not a valid CLI history file", fname );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      }

      /* Deallocate memory */
      free_safe( line, line_size );

    } Catch_anonymous {
      unsigned int rv = fclose( hfile );
      assert( rv == 0 );
      Throw 0;
    }

    fclose( hfile );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to read history file \"%s\"", fname );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

}

#endif

