/*
 Copyright (c) 2006-2009 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "codegen.h"
#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_iter.h"
#include "generator.h"
#include "link.h"
#include "profiler.h"
#include "util.h"


extern void reset_lexer_for_generation(
  const char* in_fname,  /*!< Name of file to read */
  const char* out_dir    /*!< Output directory name */
);
extern int VLparse();

extern db**           db_list;
extern unsigned int   curr_db;
extern char           user_msg[USER_MSG_LENGTH];
extern str_link*      modlist_head;
extern str_link*      modlist_tail;
extern const exp_info exp_op_info[EXP_OP_NUM];
extern bool           debug_mode;
extern func_unit*     curr_funit;


struct fname_link_s;
typedef struct fname_link_s fname_link;
struct fname_link_s {
  char*       filename;    /*!< Filename associated with all functional units */
  func_unit*  next_funit;  /*!< Pointer to the next/current functional unit that will be parsed */
  funit_link* head;        /*!< Pointer to head of functional unit list */
  funit_link* tail;        /*!< Pointer to tail of functional unit list */
  fname_link* next;        /*!< Pointer to next filename list */
};

struct expf_link_s;
typedef struct expf_link_s expf_link;
struct expf_link_s {
  expression* exp;         /*!< Pointer to expression */
  func_unit*  funit;       /*!< Pointer to functional unit */
  expf_link*  next;        /*!< Pointer to next expression/functional unit structure in list */
};

struct event_link_s;
typedef struct event_link_s event_link;
struct event_link_s {
  char*       name;        /*!< Name of the event */
  func_unit*  funit;       /*!< Pointer to functional unit containing this event expression */
  expf_link*  expf_head;   /*!< Pointer to the head of the event expression list */
  expf_link*  expf_tail;   /*!< Pointer to the tail of the event expression list */
  event_link* next;        /*!< Pointer to the next event in this list */
};


/*!
 Pointer to the current output file.
*/
FILE* curr_ofile = NULL;

/*!
 Temporary holding buffer for code to be output.
*/
char work_buffer[4096];

/*!
 Pointer to head of hold buffer list.
*/
str_link* work_head = NULL;

/*!
 Pointer to tail of hold buffer list.
*/
str_link* work_tail = NULL;

/*!
 Temporary holding buffer for code to be output.
*/
char hold_buffer[4096];

/*!
 Pointer to head of hold buffer list.
*/
str_link* hold_head = NULL;

/*!
 Pointer to tail of hold buffer list.
*/
str_link* hold_tail = NULL;

/*!
 Pointer to head of register string list.
*/
str_link* reg_head = NULL;

/*!
 Pointer to tail of register string list.
*/
str_link* reg_tail = NULL;

/*!
 Pointer to head of event list.
*/
event_link* event_head = NULL;

/*!
 Pointer to tail of event list.
*/
event_link* event_tail = NULL;

/*!
 Maximum expression depth to generate coverage code for (from command-line).
*/
unsigned int generator_max_exp_depth = 0xffffffff;

/*!
 Specifies that we should perform line coverage code insertion.
*/
bool generator_line = TRUE;

/*!
 Specifies that we should perform combinational logic coverage code insertion.
*/
bool generator_comb = TRUE;

/*!
 Specifies that we should perform FSM coverage code insertion.
*/
bool generator_fsm = TRUE;

/*!
 Specifies that we should perform memory coverage code insertion.
*/
bool generator_mem = TRUE;

/*!
 Iterator for current functional unit.
*/
static func_iter fiter;

/*!
 Pointer to current statement (needs to be set to NULL at the beginning of each module).
*/
static statement* curr_stmt;

/*!
 Pointer to top of statement stack (statement stack's are used for saving found statements that need to
 be reused at a later time.  We are reusing the stmt_loop_link structure which does not match our needs
 exactly but is close enough.
*/
static stmt_loop_link* stmt_stack = NULL;


/*!
 Outputs the current state of the code lists to standard output (for debugging purposes only).
*/
void generator_display() { PROFILE(GENERATOR_DISPLAY);

  str_link* strl;

  printf( "Register code list:\n" );
  strl = reg_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }

  printf( "Holding code list:\n" );
  strl = hold_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }
  printf( "Holding buffer:\n  %s\n", hold_buffer );

  printf( "Working code list:\n" );
  strl = work_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }
  printf( "Working buffer:\n  %s\n", work_buffer );

  PROFILE_END;

}

/*!
 Populates the specified filename list with the functional unit list, sorting all functional units with the
 same filename together in the same link.
*/
static void generator_create_filename_list(
  funit_link*  funitl,  /*!< Pointer to the head of the functional unit linked list */
  fname_link** head,    /*!< Pointer to the head of the filename list to populate */
  fname_link** tail     /*!< Pointer to the tail of the filename list to populate */
) { PROFILE(GENERATOR_SORT_FUNIT_BY_FILENAME);

  while( funitl != NULL ) {

    /* Only add modules that are not the $root "module" */
    if( (funitl->funit->type == FUNIT_MODULE) && (strncmp( "$root", funitl->funit->name, 5 ) != 0) ) {

      fname_link* fnamel = *head;

      /* Search for functional unit filename in our filename list */
      while( (fnamel != NULL) && (strcmp( fnamel->filename, funitl->funit->filename ) != 0) ) {
        fnamel = fnamel->next;
      }

      /* If the filename link was not found, create a new link */
      if( fnamel == NULL ) {

        /* Allocate and initialize the filename link */
        fnamel             = (fname_link*)malloc_safe( sizeof( fname_link ) );
        fnamel->filename   = strdup_safe( funitl->funit->filename );
        fnamel->next_funit = funitl->funit;
        fnamel->head       = NULL;
        fnamel->tail       = NULL;
        fnamel->next       = NULL;

        /* Add the new link to the list */
        if( *head == NULL ) {
          *head = *tail = fnamel;
        } else {
          (*tail)->next = fnamel;
          *tail         = fnamel;
        }

      /*
       Otherwise, if the filename was found, compare the start_line of the next_funit to this functional unit's first_line.
       If the current functional unit's start_line is less, set next_funit to point to this functional unit.
      */
      } else {

        if( fnamel->next_funit->start_line > funitl->funit->start_line ) {
          fnamel->next_funit = funitl->funit;
        }

      }

      /* Add the current functional unit to the filename functional unit list */
      funit_link_add( funitl->funit, &(fnamel->head), &(fnamel->tail) );

    }

    /* Advance to the next functional unit */
    funitl = funitl->next;

  }
 
  PROFILE_END;

}

/*!
 \return Returns TRUE if next functional unit exists; otherwise, returns FALSE.

 This function should be called whenever we see an endmodule of a covered module.
*/
static bool generator_set_next_funit(
  fname_link* curr  /*!< Pointer to the current filename link being worked on */
) { PROFILE(GENERATOR_SET_NEXT_FUNIT);

  func_unit*  next_funit = NULL;
  funit_link* funitl     = curr->head;

  /* Find the next functional unit */
  while( funitl != NULL ) {
    if( (funitl->funit->start_line >= curr->next_funit->end_line) &&
        ((next_funit == NULL) || (funitl->funit->start_line < next_funit->start_line)) ) {
      next_funit = funitl->funit;
    }
    funitl = funitl->next;
  }

  /* Only change the next_funit if we found a next functional unit (we don't want this value to ever be NULL) */
  if( next_funit != NULL ) {
    curr->next_funit = next_funit;
  }

  PROFILE_END;

  return( next_funit != NULL );

}

/*!
 Deallocates all memory allocated for the given filename linked list.
*/
static void generator_dealloc_filename_list(
  fname_link* head  /*!< Pointer to filename list to deallocate */
) { PROFILE(GENERATOR_DEALLOC_FNAME_LIST);

  fname_link* tmp;

  while( head != NULL ) {
    
    tmp  = head;
    head = head->next;

    /* Deallocate the filename */
    free_safe( tmp->filename, (strlen( tmp->filename ) + 1) );

    /* Deallocate the functional unit list (without deallocating the functional units themselves) */
    funit_link_delete_list( &(tmp->head), &(tmp->tail), FALSE );

    /* Deallocate the link itself */
    free_safe( tmp, sizeof( fname_link ) );

  }

  PROFILE_END;

}

/*!
 Generates an instrumented version of a given functional unit.
*/
static void generator_output_funits(
  fname_link* head  /*!< Pointer to the head of the filename list */
) { PROFILE(GENERATOR_OUTPUT_FUNIT);

  while( head != NULL ) {

    char         filename[4096];
    unsigned int rv;
    funit_link*  funitl;

    /* Create the output file pathname */
    rv = snprintf( filename, 4096, "covered/verilog/%s", get_basename( head->filename ) );
    assert( rv < 4096 );

    /* Populate the modlist list */
    funitl = head->head;
    while( funitl != NULL ) {
      str_link_add( strdup_safe( funitl->funit->name ), &modlist_head, &modlist_tail );
      funitl = funitl->next;
    }

    /* Open the output file for writing */
    if( (curr_ofile = fopen( filename, "w" )) != NULL ) {

      /* Parse the original code and output inline coverage code */
      reset_lexer_for_generation( head->filename, "covered/verilog" );
      VLparse();

      /* Flush the work and hold buffers */
      generator_flush_all;

      /* Close the output file */
      rv = fclose( curr_ofile );
      assert( rv == 0 );

    } else {

      rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to create generated Verilog file \"%s\"", filename );
      assert( rv < USER_MSG_LENGTH );
      Throw 0;

    }

    /* Reset the modlist list */
    str_link_delete_list( modlist_head );
    modlist_head = modlist_tail = NULL;

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Outputs the covered portion of the design to the covered/verilog directory.
*/
void generator_output() { PROFILE(GENERATOR_OUTPUT);

  fname_link* fname_head = NULL;  /* Pointer to head of filename linked list */
  fname_link* fname_tail = NULL;  /* Pointer to tail of filename linked list */
  fname_link* fnamel;             /* Pointer to current filename link */

  /* Create the initial "covered" directory - TBD - this should be done prior to this function call */
  if( !directory_exists( "covered" ) ) {
    if( mkdir( "covered", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
      print_output( "Unable to create \"covered\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* If the "covered/verilog" directory exists, remove it */
  if( directory_exists( "covered/verilog" ) ) {
    if( system( "rm -rf covered/verilog" ) != 0 ) {
      print_output( "Unable to remove \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* Create "covered/verilog" directory */
  if( mkdir( "covered/verilog", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
    print_output( "Unable to create \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Initialize the work_buffer and hold_buffer arrays */
  work_buffer[0] = '\0';
  hold_buffer[0] = '\0';

  /* Initialize the functional unit iter */
  fiter.sis  = NULL;
  fiter.sigs = NULL;

  /* Create the filename list from the functional unit list */
  generator_create_filename_list( db_list[curr_db]->funit_head, &fname_head, &fname_tail );

  /* Iterate through the covered files, generating coverage output along with the design information */
  generator_output_funits( fname_head );

  /* Deallocate memory from filename list */
  generator_dealloc_filename_list( fname_head );

  /* Deallocate the functional unit iterator */
  func_iter_dealloc( &fiter );

  PROFILE_END;

}

/*!
 Initializes and resets the functional unit iterator.
*/
void generator_init_funit(
  func_unit* funit  /*!< Pointer to current functional unit */
) { PROFILE(GENERATOR_INIT_FUNIT);

  /* Deallocate the functional unit iterator */
  func_iter_dealloc( &fiter );

  /* Initializes the functional unit iterator */
  func_iter_init( &fiter, funit, TRUE, FALSE, FALSE, TRUE );

  /* Clear the current statement pointer */
  curr_stmt = NULL;

  /* Reset the structure */
  func_iter_reset( &fiter );

  PROFILE_END;

}

/*!
 Prepends the given string to the working code list/buffer.  This function is used by the parser
 to add code prior to the current working buffer without updating the holding buffer.
*/
void generator_prepend_to_work_code(
  const char* str  /*!< String to write */
) { PROFILE(GENERATOR_PREPEND_TO_WORK_CODE);

  char         tmpstr[4096];
  unsigned int rv;

  /* If the work list is empty, prepend to the work buffer */
  if( work_head == NULL ) {

    if( (strlen( work_buffer ) + strlen( str )) < 4095 ) {
      strcpy( tmpstr, work_buffer );
      rv = snprintf( work_buffer, 4096, "%s %s", str, tmpstr );
      assert( rv < 4096 );
    } else {
      str_link_add( strdup_safe( str ), &work_head, &work_tail );
    }

  /* Otherwise, prepend the string to the head string of the work list */
  } else {

    if( (strlen( work_head->str ) + strlen( str )) < 4095 ) {
      strcpy( tmpstr, work_head->str );
      rv = snprintf( tmpstr, 4096, "%s %s", str, work_head->str );
      assert( rv < 4096 );
      free_safe( work_head->str, (strlen( work_head->str ) + 1) );
      work_head->str = strdup_safe( tmpstr );
    } else {
      str_link* tmp_head = NULL;
      str_link* tmp_tail = NULL;
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      if( work_head == NULL ) {
        work_head = work_tail = tmp_head;
      } else {
        tmp_tail->next = work_head;
        work_head      = tmp_head;
      }
    }

  }

  PROFILE_END;

}

/*!
 Adds the given string to the working code buffer.
*/
void generator_add_to_work_code(
  const char* str,       /*!< String to write */
  bool        from_code  /*!< Specifies if the string came from the code directly */
) { PROFILE(GENERATOR_ADD_TO_WORK_CODE);

  static bool semi_from_code_just_seen  = FALSE;
  static bool semi_inject_just_seen     = FALSE;
  static bool begin_from_code_just_seen = FALSE;
  static bool begin_inject_just_seen    = FALSE;
  static bool default_just_seen         = FALSE;
  bool        add                       = TRUE;

  /* Make sure that we remove back-to-back semicolons */
  if( strcmp( str, ";" ) == 0 ) {
    if( ((semi_from_code_just_seen || begin_from_code_just_seen) && !from_code) ||
        ((semi_inject_just_seen || begin_inject_just_seen || default_just_seen) && from_code) ) {
      add = FALSE;
    }
    if( from_code ) {
      semi_from_code_just_seen  = TRUE;
      semi_inject_just_seen     = FALSE;
    } else {
      semi_inject_just_seen     = TRUE;
      semi_from_code_just_seen  = FALSE;
    }
    begin_from_code_just_seen = FALSE;
    begin_inject_just_seen    = FALSE;
    default_just_seen        = FALSE;
  } else if( strcmp( str, " begin" ) == 0 ) {
    if( from_code ) {
      begin_from_code_just_seen = TRUE;
      begin_inject_just_seen    = FALSE;
    } else {
      begin_inject_just_seen    = TRUE;
      begin_from_code_just_seen = FALSE;
    }
    semi_from_code_just_seen = FALSE;
    semi_inject_just_seen    = FALSE;
    default_just_seen        = FALSE;
  } else if( strcmp( str, "default" ) == 0 ) {
    default_just_seen = TRUE;
  } else if( (str[0] != ' ') && (str[0] != '\n') && (str[0] != '\t') && (str[0] != '\r') && (str[0] != '\b') ) {
    semi_from_code_just_seen  = FALSE;
    semi_inject_just_seen     = FALSE;
    begin_from_code_just_seen = FALSE;
    begin_inject_just_seen    = FALSE;
    default_just_seen         = FALSE;
  }

  if( add ) {

    /* I don't believe that a line will ever exceed 4K chars */
    assert( (strlen( work_buffer ) + strlen( str )) < 4095 );
    strcat( work_buffer, str );

    /* If we have hit a newline, add the working buffer to the working list and clear the working buffer */
    if( strcmp( str, "\n" ) == 0 ) {
      str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
      work_buffer[0] = '\0';
    }

  }

  PROFILE_END;

}

/*!
 Flushes the current working code to the holding code buffers.
*/
void generator_flush_work_code1(
  const char*  file,  /*!< Filename that calls this function */
  unsigned int line   /*!< Line number that calls this function */
) { PROFILE(GENERATOR_FLUSH_WORK_CODE1);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing work code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* If the hold_buffer is not empty, move it to the hold list */
  if( strlen( hold_buffer ) > 0 ) {
    str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
  }

  /* If the working list is not empty, append it to the holding code */
  if( work_head != NULL ) {

    /* Move the working code lists to the holding code lists */
    if( hold_head == NULL ) {
      hold_head = work_head;
    } else {
      hold_tail->next = work_head;
    }
    hold_tail = work_tail;
    work_head = work_tail = NULL;

  }

  /* Copy the working buffer to the holding buffer */
  strcpy( hold_buffer, work_buffer );

  /* Clear the work buffer */
  work_buffer[0] = '\0';

  PROFILE_END;

}

/*!
 Adds the given code string to the "immediate" code generator.  This code can be output to the file
 immediately if there is no code in the sig_list and exp_list arrays.  If it cannot be output immediately,
 the code is added to the exp_list array.
*/
void generator_add_to_hold_code(
  const char*  str  /*!< String to write */
) { PROFILE(GENERATOR_ADD_TO_HOLD_CODE);
 
  /* I don't believe that a line will ever exceed 4K chars */
  assert( (strlen( hold_buffer ) + strlen( str )) < 4095 );
  strcat( hold_buffer, str );

  /* If we have hit a newline, add it to the hold list and clear the hold buffer */
  if( strcmp( str, "\n" ) == 0 ) {
    str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
    hold_buffer[0] = '\0';
  }

  PROFILE_END;

}

/*!
 Outputs all held code to the output file.
*/
void generator_flush_hold_code1(
  const char*  file,  /*!< Filename that calls this function */
  unsigned int line   /*!< Line number that calls this function */
) { PROFILE(GENERATOR_FLUSH_HOLD_CODE1);

  str_link* strl = hold_head;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing hold code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Output the register buffer if it exists */
  strl = reg_head;
  while( strl != NULL ) {
    fprintf( curr_ofile, "%s\n", strl->str );
    strl = strl->next;
  }
  str_link_delete_list( reg_head );
  reg_head = reg_tail = NULL;

  /* Output the hold buffer list to the output file */
  strl = hold_head;
  while( strl != NULL ) {
    fprintf( curr_ofile, "%s", strl->str );
    strl = strl->next;
  }
  str_link_delete_list( hold_head );
  hold_head = hold_tail = NULL;

  /* If the hold buffer is not empty, output it as well */
  if( strlen( hold_buffer ) > 0 ) {
    fprintf( curr_ofile, "%s", hold_buffer );
    hold_buffer[0] = '\0';
  }

  PROFILE_END;

}

/*!
 Outputs the contents of the given event link to the Verilog code.
*/
static void generator_flush_event_comb(
  event_link* eventl  /*!< Pointer to event link to output */
) { PROFILE(GENERATOR_FLUSH_EVENT_COMB);

  expf_link*  expfl = eventl->expf_head;
  expf_link*  tmpefl;
  expression* exp;
  bool        begin_end_needed = FALSE;

  while( expfl != NULL ) {
    exp = expression_get_last_line_expr( expfl->exp );
    if( expfl->funit->type == FUNIT_MODULE ) {
      fprintf( curr_ofile, "reg \\covered$E%d_%d_%x ;\n", expfl->exp->line, exp->line, expfl->exp->col );
    } else {
      fprintf( curr_ofile, "reg \\covered$E%d_%d_%x$%s ;\n", expfl->exp->line, exp->line, expfl->exp->col, expfl->funit->name );
    }
    expfl = expfl->next;
  }

  /* If we have more than one event to assign, we need a begin..end block output */
  if( eventl->expf_head != eventl->expf_tail ) {
    begin_end_needed = TRUE;
  }

  if( eventl->name[0] == '@' ) {
    fprintf( curr_ofile, "always %s", eventl->name );
  } else {
    fprintf( curr_ofile, "always @(%s)", eventl->name );
  }
  if( begin_end_needed ) {
    fprintf( curr_ofile, " begin" );
  }

  /* Output the event assignments */
  expfl = eventl->expf_head;
  while( expfl != NULL ) {
    tmpefl = expfl;
    expfl  = expfl->next;
    exp    = expression_get_last_line_expr( tmpefl->exp );
    if( tmpefl->funit->type == FUNIT_MODULE ) {
      fprintf( curr_ofile, "  \\covered$E%d_%d_%x = 1'b1;\n", tmpefl->exp->line, exp->line, tmpefl->exp->col );
    } else {
      fprintf( curr_ofile, "  \\covered$E%d_%d_%x$%s = 1'b1;\n", tmpefl->exp->line, exp->line, tmpefl->exp->col, tmpefl->funit->name );
    }
    free_safe( tmpefl, sizeof( expf_link ) );
  }

  if( begin_end_needed ) {
    fprintf( curr_ofile, "end\n" );
  }

  fprintf( curr_ofile, "\n" );

  /* Deallocate the rest of the event link */
  free_safe( eventl->name, (strlen( eventl->name ) + 1) );
  free_safe( eventl, sizeof( event_link ) );

  PROFILE_END;

}

/*!
 This function should only be called just prior to an endmodule token.  It flushes the current contents of the
 event list to the module for the purposes of handling combinational logic event types.
*/
void generator_flush_event_combs1(
  const char*  file,  /*!< Filename where function is called */
  unsigned int line   /*!< Line number where function is called */
) { PROFILE(GENERATOR_FLUSH_EVENT_COMBS1);

  event_link* eventl = event_head;
  event_link* tmpl;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing event code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  while( eventl != NULL ) {
    tmpl   = eventl;
    eventl = eventl->next;
    generator_flush_event_comb( tmpl );
  }

  event_head = event_tail = NULL;

  PROFILE_END;

}


/*!
 Shortcut for calling generator_flush_work_code() followed by generator_flush_hold_code().
*/
void generator_flush_all1(
  const char*  file,  /*!< Filename where function is called */
  unsigned int line   /*!< Line number where function is called */
) { PROFILE(GENERATOR_FLUSH_ALL1);

  generator_flush_work_code1( file, line );
  generator_flush_hold_code1( file, line );

  PROFILE_END;

}

/*!
 \return Returns a pointer to the found statement (or if no statement was found, returns NULL).

 Searches the current functional unit for a statement that matches the given positional information.
*/
statement* generator_find_statement(
  unsigned int first_line,   /*!< First line of statement to find */
  unsigned int first_column  /*!< First column of statement to find */
) { PROFILE(GENERATOR_FIND_STATEMENT);

//  printf( "In generator_find_statement, line: %d, column: %d\n", first_line, first_column );

  if( (curr_stmt == NULL) || (curr_stmt->exp->line < first_line) ||
      ((curr_stmt->exp->line == first_line) && (((curr_stmt->exp->col >> 16) & 0xffff) < first_column)) ) {

//    func_iter_display( &fiter );

    /* Attempt to find the expression with the given position */
    while( ((curr_stmt = func_iter_get_next_statement( &fiter )) != NULL) &&
//           printf( "  statement %s %d\n", expression_string( curr_stmt->exp ), ((curr_stmt->exp->col >> 16) & 0xffff) ) &&
           ((curr_stmt->exp->line < first_line) || 
            ((curr_stmt->exp->line == first_line) && (((curr_stmt->exp->col >> 16) & 0xffff) < first_column)) ||
            (curr_stmt->exp->op == EXP_OP_FORK)) );

  }

//  if( (curr_stmt != NULL) && (curr_stmt->exp->line == first_line) && (((curr_stmt->exp->col >> 16) & 0xffff) == first_column) && (curr_stmt->exp->op != EXP_OP_FORK) ) {
//    printf( "  FOUND (%s %x)!\n", expression_string( curr_stmt->exp ), ((curr_stmt->exp->col >> 16) & 0xffff) );
//  } else {
//    printf( "  NOT FOUND!\n" );
//  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->line != first_line) ||
          (((curr_stmt->exp->col >> 16) & 0xffff) != first_column) || (curr_stmt->exp->op == EXP_OP_FORK)) ? NULL : curr_stmt );

}

/*!
 \return Returns a pointer to the found statement (or if no statement was found, returns NULL).

 Searches the current functional unit for a statement that contains the case expression matches the
 given positional information.
*/
static statement* generator_find_case_statement(
  unsigned int first_line,   /*!< First line of case expression to find */
  unsigned int first_column  /*!< First column of case expression to find */
) { PROFILE(GENERATOR_FIND_CASE_STATEMENT);

  if( (curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->line < first_line) ||
      ((curr_stmt->exp->left->line == first_line) && (((curr_stmt->exp->left->col >> 16) & 0xffff) < first_column)) ) {

    /* Attempt to find the expression with the given position */
    while( ((curr_stmt = func_iter_get_next_statement( &fiter )) != NULL) && 
           ((curr_stmt->exp->left == NULL) ||
            (curr_stmt->exp->left->line < first_line) ||
            ((curr_stmt->exp->left->line == first_line) && (((curr_stmt->exp->left->col >> 16) & 0xffff) < first_column))) );

  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->line != first_line) ||
          (((curr_stmt->exp->left->col >> 16) & 0xffff) != first_column)) ? NULL : curr_stmt );

}

/*!
 Inserts line coverage information for the given statement.
*/
void generator_insert_line_cov_with_stmt(
  statement* stmt,      /*!< Pointer to statement to generate line coverage for */
  bool       semicolon  /*!< Specifies if a semicolon (TRUE) or a comma (FALSE) should be appended to the created line */
) { PROFILE(GENERATOR_INSERT_LINE_COV_WITH_STMT);

  if( generator_line && (stmt != NULL) ) {

    expression*  last_exp = expression_get_last_line_expr( stmt->exp );
    char         str[4096];
    char         sig[4096];
    unsigned int rv;
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;

    if( stmt->funit->type == FUNIT_MODULE ) {
      rv = snprintf( sig, 4096, " \\covered$l%d_%d_%x ", stmt->exp->line, last_exp->line, stmt->exp->col );
    } else {
      rv = snprintf( sig, 4096, " \\covered$l%d_%d_%x$%s ", stmt->exp->line, last_exp->line, stmt->exp->col, stmt->funit->name );
    }
    assert( rv < 4096 );

    /* Create the register */
    rv = snprintf( str, 4096, "reg %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );

    /* Prepend the line coverage assignment to the working buffer */
    rv = snprintf( str, 4096, " %s = 1'b1%c", sig, (semicolon ? ';' : ',') );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the statement inserted (or NULL if no statement was inserted).

 Inserts line coverage information in string queues.
*/
statement* generator_insert_line_cov(
  unsigned int first_line,    /*!< First line to create line coverage for */
  unsigned int last_line,     /*!< Last line to create line coverage for */
  unsigned int first_column,  /*!< First column of statement */
  unsigned int last_column,   /*!< Last column of statement */
  bool         semicolon      /*!< Set to TRUE to create a semicolon after the line assignment; otherwise, adds a comma */
) { PROFILE(GENERATOR_INSERT_LINE_COV);

  statement* stmt = NULL;

  if( generator_line && ((stmt = generator_find_statement( first_line, first_column )) != NULL) ) {

    generator_insert_line_cov_with_stmt( stmt, semicolon );

  }

  PROFILE_END;

  return( stmt );

}

/*!
 Handles the insertion of event-type combinational logic code.
*/
void generator_insert_event_comb_cov(
  expression* exp,        /*!< Pointer to expression to output */
  func_unit*  funit,      /*!< Pointer to functional unit containing the expression */
  bool        reg_needed  /*!< If set to TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_EVENT_COMB_COV);

  char         name[4096];
  unsigned int rv;

  /*
   If the expression is a root of an expression tree and it is a single event, just insert the event coverage code
   immediately.
  */
  if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) && (exp->op != EXP_OP_EOR) ) {

    char        str[4096];
    expression* last_exp = expression_get_last_line_expr( exp );

    /* Create signal name */
    if( funit->type == FUNIT_MODULE ) {
      rv = snprintf( name, 4096, " \\covered$E%d_%d_%x ", exp->line, last_exp->line, exp->col );
    } else {
      rv = snprintf( name, 4096, " \\covered$E%d_%d_%x$%s ", exp->line, last_exp->line, exp->col, funit->name );
    }
    assert( rv < 4096 );

    /* Create register */
    if( reg_needed ) {
      rv = snprintf( str, 4096, "reg %s;", name );
      assert( rv < 4096 );
      str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
    }

    /* Create assignment and append it to the working code list */
    rv = snprintf( str, 4096, "%s = 1'b1;", name );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &work_head, &work_tail );

  /*
   Otherwise, we will store off the information to inject it at the end of the module.
  */
  } else {

    char**       code       = NULL;
    unsigned int code_depth = 0;
    char*        event_str;
    unsigned int i;
    event_link*  eventl;

    /* Create signal name */
    if( funit->type == FUNIT_MODULE ) {
      rv = snprintf( name, 4096, " \\covered$z%d_%x ", exp->line, exp->col );
    } else {
      rv = snprintf( name, 4096, " \\covered$z%d_%x$%s ", exp->line, exp->col, funit->name );
    }
    assert( rv < 4096 );

    /* Create the event string */
    event_str = codegen_gen_expr_one_line( exp, funit );

    /* Search through event list to see if this event has already been added */
    eventl = event_head;
    while( (eventl != NULL) && (strcmp( eventl->name, event_str ) != 0) ) {
      eventl = eventl->next;
    }

    /* Add the expression to the event list if a matching event link is found */
    if( eventl != NULL ) {
      expf_link* expfl = (expf_link*)malloc_safe( sizeof( expf_link ) );
      expfl->exp       = exp;
      expfl->funit     = funit;
      expfl->next      = NULL;
      if( eventl->expf_head == NULL ) {
        eventl->expf_head = eventl->expf_tail = NULL;
      } else {
        eventl->expf_tail->next = expfl;
        eventl->expf_tail       = expfl;
      }
      free_safe( event_str, (strlen( event_str ) + 1) );

    /* Otherwise, allocate and initialize a new event link and add it to the list */
    } else {
      expf_link* expfl  = (expf_link*)malloc_safe( sizeof( expf_link ) );
      expfl->exp        = exp;
      expfl->funit      = funit;
      expfl->next       = NULL;
      eventl            = (event_link*)malloc_safe( sizeof( event_link ) );
      eventl->name      = event_str;
      eventl->expf_head = expfl;
      eventl->expf_tail = expfl;
      eventl->next      = NULL;
      if( event_head == NULL ) {
        event_head = event_tail = eventl;
      } else {
        event_tail->next = eventl;
        event_tail       = eventl;
      }
    }

  }

  PROFILE_END;

}

/*!
 Inserts combinational logic coverage code for a unary expression.
*/
static void generator_insert_unary_comb_cov(
  expression*  exp,        /*!< Pointer to expression to output */
  func_unit*   funit,      /*!< Pointer to functional unit containing this expression */
  unsigned int depth,      /*!< Current expression depth */
  bool         net,        /*!< Set to TRUE if this expression is a net */
  bool         reg_needed  /*!< If TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_UNARY_COMB_COV);

  char         prefix[16];
  char         sig[80];
  char         sigr[80];
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  expression*  last_exp = expression_get_last_line_expr( exp );

  /* Create signal */
  if( net || (funit->type == FUNIT_MODULE) ) {
    rv = snprintf( sig,  80, " \\covered$%c%d_%d_%x ", (net ? 'u' : 'U'), exp->line, last_exp->line, exp->col );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$%c%d_%d_%x ", (net ? 'x' : 'X'), exp->line, last_exp->line, exp->col );
    assert( rv < 80 );
  } else {
    rv = snprintf( sig,  80, " \\covered$U%d_%d_%x$%s ", exp->line, last_exp->line, exp->col, funit->name );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$X%d_%d_%x$%s ", exp->line, last_exp->line, exp->col, funit->name );
    assert( rv < 80 );
  }

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire " );
  } else {
    prefix[0] = '\0';
    if( reg_needed ) {
      rv = snprintf( str, 4096, "reg %s;", sig );
      assert( rv < 4096 );
      str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
    }
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = (%s > 0);", prefix, sig, sigr );
  assert( rv < 4096 );
  str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );

  /* If this is a net, prepend to the register list */
  if( net ) {
    if( reg_head == NULL ) {
      reg_head = reg_tail = tmp_head;
    } else {
      tmp_tail->next = reg_head;
      reg_head       = tmp_head;
    }

  /* Otherwise, prepend to the working list */
  } else {
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }

  }

  PROFILE_END;

}

/*!
 Inserts AND/OR/OTHER-style combinational logic coverage code.
*/
static void generator_insert_comb_comb_cov(
  expression* exp,        /*!< Pointer to expression to output */
  func_unit*  funit,      /*!< Pointer to functional unit containing this expression */
  bool        net,        /*!< Set to TRUE if this expression is a net */
  bool        reg_needed  /*!< If set to TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_AND_COMB_COV);

  char         prefix[16];
  char         sig[80];
  char         sigl[80];
  char         sigr[80];
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head  = NULL;
  str_link*    tmp_tail  = NULL;
  expression*  last_exp  = expression_get_last_line_expr( exp );
  expression*  last_lexp = expression_get_last_line_expr( exp->left );

  /* Create signal */
  if( net || (funit->type == FUNIT_MODULE) ) {
    rv = snprintf( sig,  80, " \\covered$%c%d_%d_%x ", (net ? 'c' : 'C'), exp->line, last_exp->line, exp->col );
    assert( rv < 80 );
    rv = snprintf( sigl, 80, " \\covered$%c%d_%d_%x ", (net ? 'x' : 'X'), exp->left->line, last_lexp->line, exp->left->col );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$%c%d_%d_%x ", (net ? 'x' : 'X'), exp->right->line, last_exp->line, exp->right->col );
    assert( rv < 80 );
  } else {
    rv = snprintf( sig,  80, " \\covered$C%d_%d_%x$%s ", exp->line, last_exp->line, exp->col, funit->name );
    assert( rv < 80 );
    rv = snprintf( sigl, 80, " \\covered$X%d_%d_%x$%s ", exp->left->line, last_lexp->line, exp->left->col, funit->name );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$X%d_%d_%x$%s ", exp->right->line, last_exp->line, exp->right->col, funit->name );
    assert( rv < 80 );
  }

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [1:0] " );
  } else if( reg_needed ) {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [1:0] %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = {(%s > 0),(%s > 0)};", prefix, sig, sigl, sigr );
  assert( rv < 4096 );
  str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );

  /* If this is a net, prepend to the register list */
  if( net ) {
    if( reg_head == NULL ) {
      reg_head = reg_tail = tmp_head;
    } else {
      tmp_tail->next = reg_head;
      reg_head       = tmp_head;
    }

  /* Otherwise, prepend to the working list */
  } else {
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }
  }

  PROFILE_END;

}

/*!
 Generates MSB string to use for sizing subexpression values.
*/
static char* generator_gen_msb(
  expression* exp,   /*!< Pointer to subexpression to generate MSB value for */
  func_unit*  funit  /*!< Pointer to functional unit containing this subexpression */
) { PROFILE(GENERATOR_GEN_MSB);

  char* msb = NULL;

  if( exp != NULL ) {

    char*        lexp = NULL;
    char*        rexp = NULL;
    unsigned int rv;

    switch( exp->op ) {
      case EXP_OP_STATIC :
        {
          char tmp[50];
          rv = snprintf( tmp, 50, "%d", exp->value->width );
          assert( rv < 50 );
          msb = strdup_safe( tmp );
        }
        break;
      case EXP_OP_LIST     :
      case EXP_OP_MULTIPLY :
        lexp = generator_gen_msb( exp->left,  funit );
        rexp = generator_gen_msb( exp->right, funit );
        {
          unsigned int slen = strlen( lexp ) + strlen( rexp ) + 4;
          msb = (char*)malloc_safe( slen );
          rv  = snprintf( msb, slen, "(%s+%s)", lexp, rexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        break;
      case EXP_OP_CONCAT         :
      case EXP_OP_NEGATE         :
        msb = generator_gen_msb( exp->right, funit );
        break;
      case EXP_OP_MBIT_POS       :
      case EXP_OP_MBIT_NEG       :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        msb = codegen_gen_expr_one_line( exp->right, funit );
        break;
      case EXP_OP_LSHIFT  :
      case EXP_OP_RSHIFT  :
      case EXP_OP_ALSHIFT :
      case EXP_OP_ARSHIFT :
        msb = generator_gen_msb( exp->left, funit );
        break;
      case EXP_OP_EXPAND :
        lexp = codegen_gen_expr_one_line( exp->left, funit );
        rexp = generator_gen_msb( exp->right, funit );
        {
          unsigned int slen = strlen( lexp ) + strlen( rexp ) + 4;
          msb = (char*)malloc_safe( slen );
          rv  = snprintf( msb, slen, "(%s*%s)", lexp, rexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        break;
      case EXP_OP_STIME :
      case EXP_OP_SR2B  :
      case EXP_OP_SR2I  :
        msb = strdup_safe( "64" );
        break;
      case EXP_OP_SSR2B :
        msb = strdup_safe( "32" );
        break;
      case EXP_OP_LT        :
      case EXP_OP_GT        :
      case EXP_OP_EQ        :
      case EXP_OP_CEQ       :
      case EXP_OP_LE        :
      case EXP_OP_GE        :
      case EXP_OP_NE        :
      case EXP_OP_CNE       :
      case EXP_OP_LOR       :
      case EXP_OP_LAND      :
      case EXP_OP_UAND      :
      case EXP_OP_UNOT      :
      case EXP_OP_UOR       :
      case EXP_OP_UXOR      :
      case EXP_OP_UNAND     :
      case EXP_OP_UNOR      :
      case EXP_OP_UNXOR     :
      case EXP_OP_EOR       :
      case EXP_OP_NEDGE     :
      case EXP_OP_PEDGE     :
      case EXP_OP_AEDGE     :
      case EXP_OP_CASE      :
      case EXP_OP_CASEX     :
      case EXP_OP_CASEZ     :
      case EXP_OP_DEFAULT   :
      case EXP_OP_REPEAT    :
      case EXP_OP_RPT_DLY   :
      case EXP_OP_WAIT      :
      case EXP_OP_SFINISH   :
      case EXP_OP_SSTOP     :
      case EXP_OP_SSRANDOM  :
      case EXP_OP_STESTARGS :
      case EXP_OP_SVALARGS  :
      case EXP_OP_SBIT_SEL  :
      case EXP_OP_PARAM_SBIT :
        msb = strdup_safe( "1" );
        break;
      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
        lexp = codegen_gen_expr_one_line( exp->left,  funit );
        rexp = codegen_gen_expr_one_line( exp->right, funit );
        {
          unsigned int slen = (strlen( lexp ) * 3) + (strlen( rexp ) * 3) + 18;
          msb = (char*)malloc_safe( slen );
          rv  = snprintf( msb, slen, "(((%s>%s)?(%s-%s):(%s-%s))+1)", lexp, rexp, lexp, rexp, rexp, lexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        break;
      case EXP_OP_SIG       :
      case EXP_OP_PARAM     :
      case EXP_OP_FUNC_CALL :
        if( (exp->sig->suppl.part.type == SSUPPL_TYPE_GENVAR) || (exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_SREAL) ) {
          msb = strdup_safe( "32" );
        } else if( exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_REAL ) {
          msb = strdup_safe( "64" );
//        } else if( !exp->sig->suppl.part.implicit_size ) {
        } else {
          char tmp[50];
          rv = snprintf( tmp, 50, "%d", exp->sig->value->width );
          assert( rv < 50 );
          msb = strdup_safe( tmp );
//        } else {
//          /* TBD - We need to look at the signal size expression... */
//          *msb = strdup_safe( "???" );
        }
        break;
      default :
        lexp = generator_gen_msb( exp->left,  funit );
        rexp = generator_gen_msb( exp->right, funit );
        if( lexp != NULL ) {
          if( rexp != NULL ) {
            unsigned int slen = (strlen( lexp ) * 2) + (strlen( rexp ) * 2) + 8;
            msb = (char*)malloc_safe( slen );
            rv   = snprintf( msb, slen, "((%s>%s)?%s:%s)", lexp, rexp, lexp, rexp );
            assert( rv < slen );
            free_safe( lexp, (strlen( lexp ) + 1) );
            free_safe( rexp, (strlen( rexp ) + 1) );
          } else {
            msb = lexp;
          }
        } else {
          if( rexp != NULL ) {
            msb = rexp;
          } else {
            msb = NULL;
          }
        }
        break;
    }

  }

  PROFILE_END;

  return( msb );

}

/*!
 Generates the strings needed for the left-hand-side of the temporary expression.
*/
static char* generator_create_lhs(
  expression* exp,        /*!< Pointer to subexpression to create LHS string for */
  func_unit*  funit,      /*!< Functional unit containing the expression */
  bool        net,        /*!< If set to TRUE, generate code for a net; otherwise, generate for a register */
  bool        reg_needed  /*!< If TRUE, instantiates the needed registers */
) { PROFILE(GENERATOR_CREATE_RHS);

  unsigned int rv;
  char         name[4096];
  char         tmp[4096];
  char*        msb;
  char*        code;
  expression*  last_exp = expression_get_last_line_expr( exp );

  /* Generate signal name */
  if( net || (funit->type == FUNIT_MODULE) ) {
    rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x ", (net ? 'x' : 'X'), exp->line, last_exp->line, exp->col );
  } else {
    rv = snprintf( name, 4096, " \\covered$X%d_%d_%x$%s ", exp->line, last_exp->line, exp->col, funit->name );
  }
  assert( rv < 4096 );

  /* Generate MSB string */
  msb = generator_gen_msb( exp, funit );

  if( net ) {

    /* Create sized wire string */
    rv = snprintf( tmp, 4096, "wire [(%s-1):0] %s", ((msb != NULL) ? msb : "1"), name );
    assert( rv < 4096 );

    code = strdup_safe( tmp );

  } else {

    /* Create sized register string */
    if( reg_needed ) {
      rv = snprintf( tmp, 4096, "reg [(%s-1):0] %s;", ((msb != NULL) ? msb : "1"), name );
      assert( rv < 4096 );
      str_link_add( strdup_safe( tmp ), &reg_head, &reg_tail );
    }

    /* Set the name to the value of code */
    code = strdup_safe( name );

  }

  /* Deallocate memory */
  free_safe( msb, (strlen( msb ) + 1) );

  PROFILE_END;

  return( code );

}

/*!
 Creates the proper subexpression code string and stores it into the code array.
*/
static char* generator_create_subexp(
  expression*   exp,        /*!< Pointer to subexpression to generate code array for */
  exp_op_type   parent_op,  /*!< Operation of parent */
  func_unit*    funit,      /*!< Pointer to functional unit that contains the given expression */
  unsigned int  depth,      /*!< Current subexpression depth */
  bool          net         /*!< Set to TRUE if subexpression is a net */
) { PROFILE(GENERATOR_CREATE_SUBEXP);

  unsigned int rv;
  unsigned int i;
  char*        code = NULL;

  /* Generate left string */
  if( exp != NULL ) {

    if( (((exp->op != parent_op) ? (depth + 1) : depth) < generator_max_exp_depth) && EXPR_IS_MEASURABLE( exp ) && !expression_is_static_only( exp ) ) {

      char         num_str[50];
      unsigned int fline_len;
      unsigned int lline_len;
      unsigned int col_len;
      expression*  last_exp = expression_get_last_line_expr( exp );

      rv = snprintf( num_str, 50, "%d", exp->line );       assert( rv < 50 );  fline_len = strlen( num_str );
      rv = snprintf( num_str, 50, "%d", last_exp->line );  assert( rv < 50 );  lline_len = strlen( num_str );
      rv = snprintf( num_str, 50, "%x", exp->col );        assert( rv < 50 );  col_len   = strlen( num_str );
      if( net || (funit->type == FUNIT_MODULE) ) {
        unsigned int slen = 10 + 1 + fline_len + 1 + lline_len + 1 + col_len + 2;
        code = (char*)malloc_safe( slen );
        rv   = snprintf( code, slen, " \\covered$%c%d_%d_%x ", (net ? 'x' : 'X'), exp->line, last_exp->line, exp->col );
        assert( rv < slen );
      } else {
        unsigned int slen = 10 + 1 + fline_len + 1 + lline_len + 1 + col_len + 1 + strlen( funit->name ) + 2;
        code = (char*)malloc_safe( slen );
        rv   = snprintf( code, slen, " \\covered$%c%d_%d_%x$%s ", (net ? 'x' : 'X'), exp->line, last_exp->line, exp->col, funit->name );
        assert( rv < slen );
      }

    } else {

      code = codegen_gen_expr_one_line( exp, funit );

    }

  }

  return( code );

}

/*!
 Concatenates the given string values and appends them to the working code buffer.
*/
static void generator_concat_code(
             char* lhs,     /*!< LHS string */
  /*@null@*/ char* before,  /*!< Optional string that precedes the left subexpression string array */
  /*@null@*/ char* lstr,    /*!< String array from left subexpression */
  /*@null@*/ char* middle,  /*!< Optional string that is placed in-between the left and right subexpression array strings */
  /*@null@*/ char* rstr,    /*!< String array from right subexpression */
  /*@null@*/ char* after,   /*!< Optional string that is placed after the right subpexression string array */
             bool  net      /*!< If TRUE, specifies that this subexpression exists in net logic */
) { PROFILE(GENERATOR_CONCAT_CODE);

  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  char         str[4096];
  unsigned int i;
  unsigned int rv;

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s = ", lhs );
  assert( rv < 4096 );
  if( before != NULL ) {
    if( (strlen( str ) + strlen( before )) < 4095 ) {
      strcat( str, before );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, before );
    }
  }
  if( lstr != NULL ) {
    if( (strlen( str ) + strlen( lstr )) < 4095 ) {
      strcat( str, lstr );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      if( strlen( lstr ) < 4095 ) {
        strcpy( str, lstr );
      } else {
        str_link_add( strdup_safe( lstr ), &tmp_head, &tmp_tail );
        str[0] = '\0';
      }
    }
  }
  if( middle != NULL ) {
    if( (strlen( str ) + strlen( middle )) < 4095 ) {
      strcat( str, middle );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, middle );
    }
  }
  if( rstr != NULL ) {
    if( (strlen( str ) + strlen( rstr )) < 4095 ) {
      strcat( str, rstr );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      if( strlen( rstr ) < 4095 ) {
        strcpy( str, lstr );
      } else {
        str_link_add( strdup_safe( rstr ), &tmp_head, &tmp_tail );
        str[0] = '\0';
      }
    }
  }
  if( after != NULL ) {
    if( (strlen( str ) + strlen( after )) < 4095 ) {
      strcat( str, after );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, after );
    }
  }
  if( (strlen( str ) + 1) < 4095 ) {
    strcat( str, ";" );
  } else {
    str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
    strcpy( str, ";" );
  }
  str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      
  /* If this is a net, prepend to the register list */
  if( net ) {
    if( reg_head == NULL ) {
      reg_head = reg_tail = tmp_head;
    } else {
      tmp_tail->next = reg_head;
      reg_head       = tmp_head;
    }

  /* Otherwise, prepend to the working list */
  } else {
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }
  }

  PROFILE_END;

}

/*!
 Generates the combinational logic temporary expression string for the given expression.
*/
static void generator_create_exp(
  expression* exp,   /*!< Pointer to the expression to generation the expression for */
  char*       lhs,   /*!< String for left-hand-side of temporary expression */
  char*       lstr,  /*!< String for left side of RHS expression */
  char*       rstr,  /*!< String for right side of RHS expression */
  bool        net    /*!< Set to TRUE if we are generating for a net; set to FALSE for a register */
) { PROFILE(GENERATOR_CREATE_EXP);

  switch( exp->op ) {
    case EXP_OP_XOR        :  generator_concat_code( lhs, NULL, lstr, " ^ ", rstr, NULL, net );  break;
    case EXP_OP_MULTIPLY   :  generator_concat_code( lhs, NULL, lstr, " * ", rstr, NULL, net );  break;
    case EXP_OP_DIVIDE     :  generator_concat_code( lhs, NULL, lstr, " / ", rstr, NULL, net );  break;
    case EXP_OP_MOD        :  generator_concat_code( lhs, NULL, lstr, " % ", rstr, NULL, net );  break;
    case EXP_OP_ADD        :  generator_concat_code( lhs, NULL, lstr, " + ", rstr, NULL, net );  break;
    case EXP_OP_SUBTRACT   :  generator_concat_code( lhs, NULL, lstr, " - ", rstr, NULL, net );  break;
    case EXP_OP_AND        :  generator_concat_code( lhs, NULL, lstr, " & ", rstr, NULL, net );  break;
    case EXP_OP_OR         :  generator_concat_code( lhs, NULL, lstr, " | ", rstr, NULL, net );  break;
    case EXP_OP_NAND       :  generator_concat_code( lhs, NULL, lstr, " ~& ", rstr, NULL, net );  break;
    case EXP_OP_NOR        :  generator_concat_code( lhs, NULL, lstr, " ~| ", rstr, NULL, net );  break;
    case EXP_OP_NXOR       :  generator_concat_code( lhs, NULL, lstr, " ~^ ", rstr, NULL, net );  break;
    case EXP_OP_LT         :  generator_concat_code( lhs, NULL, lstr, " < ", rstr, NULL, net );  break;
    case EXP_OP_GT         :  generator_concat_code( lhs, NULL, lstr, " > ", rstr, NULL, net );  break;
    case EXP_OP_LSHIFT     :  generator_concat_code( lhs, NULL, lstr, " << ", rstr, NULL, net );  break;
    case EXP_OP_RSHIFT     :  generator_concat_code( lhs, NULL, lstr, " >> ", rstr, NULL, net );  break;
    case EXP_OP_EQ         :  generator_concat_code( lhs, NULL, lstr, " == ", rstr, NULL, net );  break;
    case EXP_OP_CEQ        :  generator_concat_code( lhs, NULL, lstr, " === ", rstr, NULL, net );  break;
    case EXP_OP_LE         :  generator_concat_code( lhs, NULL, lstr, " <= ", rstr, NULL, net );  break;
    case EXP_OP_GE         :  generator_concat_code( lhs, NULL, lstr, " >= ", rstr, NULL, net );  break;
    case EXP_OP_NE         :  generator_concat_code( lhs, NULL, lstr, " != ", rstr, NULL, net );  break;
    case EXP_OP_CNE        :  generator_concat_code( lhs, NULL, lstr, " !== ", rstr, NULL, net );  break;
    case EXP_OP_LOR        :  generator_concat_code( lhs, NULL, lstr, " || ", rstr, NULL, net );  break;
    case EXP_OP_LAND       :  generator_concat_code( lhs, NULL, lstr, " && ", rstr, NULL, net );  break;
    case EXP_OP_UINV       :  generator_concat_code( lhs, NULL, NULL, "~", rstr, NULL, net );  break;
    case EXP_OP_UAND       :  generator_concat_code( lhs, NULL, NULL, "&", rstr, NULL, net );  break;
    case EXP_OP_UNOT       :  generator_concat_code( lhs, NULL, NULL, "!", rstr, NULL, net );  break;
    case EXP_OP_UOR        :  generator_concat_code( lhs, NULL, NULL, "|", rstr, NULL, net );  break;
    case EXP_OP_UXOR       :  generator_concat_code( lhs, NULL, NULL, "^", rstr, NULL, net );  break;
    case EXP_OP_UNAND      :  generator_concat_code( lhs, NULL, NULL, "~&", rstr, NULL, net );  break;
    case EXP_OP_UNOR       :  generator_concat_code( lhs, NULL, NULL, "~|", rstr, NULL, net );  break;
    case EXP_OP_UNXOR      :  generator_concat_code( lhs, NULL, NULL, "~^", rstr, NULL, net );  break;
    case EXP_OP_ALSHIFT    :  generator_concat_code( lhs, NULL, lstr, " <<< ", rstr, NULL, net );  break;
    case EXP_OP_ARSHIFT    :  generator_concat_code( lhs, NULL, lstr, " >>> ", rstr, NULL, net );  break;
    case EXP_OP_EXPONENT   :  generator_concat_code( lhs, NULL, lstr, " ** ", rstr, NULL, net );  break;
    case EXP_OP_NEGATE     :  generator_concat_code( lhs, NULL, NULL, "-", rstr, NULL, net );  break;
    case EXP_OP_CASE       :  generator_concat_code( lhs, NULL, lstr, " == ", rstr, NULL, net );  break;
    case EXP_OP_CASEX      :  generator_concat_code( lhs, NULL, lstr, " === ", rstr, NULL, net );  break;
    case EXP_OP_CASEZ      :  generator_concat_code( lhs, NULL, lstr, " === ", rstr, NULL, net );  break;  /* TBD */
    case EXP_OP_CONCAT     :  generator_concat_code( lhs, NULL, NULL, "{", rstr, "}", net );  break;
    case EXP_OP_EXPAND     :  generator_concat_code( lhs, "{", lstr, "{", rstr, "}}", net );  break;
    case EXP_OP_LIST       :  generator_concat_code( lhs, NULL, lstr, ",", rstr, NULL, net );  break;
    case EXP_OP_COND       :  generator_concat_code( lhs, NULL, lstr, " ? ", rstr, NULL, net );  break;
    case EXP_OP_COND_SEL   :  generator_concat_code( lhs, NULL, lstr, " : ", rstr, NULL, net );  break;
    case EXP_OP_SIG        :
    case EXP_OP_PARAM      :  generator_concat_code( lhs, exp->name, NULL, NULL, NULL, NULL, net );  break;
    case EXP_OP_FUNC_CALL  :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s(", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, ")", NULL, NULL, net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_SBIT_SEL   :
    case EXP_OP_PARAM_SBIT :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, "]", NULL, NULL, net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_SEL   :
    case EXP_OP_PARAM_MBIT :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, ":", rstr, "]", net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_POS       :
    case EXP_OP_PARAM_MBIT_POS :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, "+:", rstr, "]", net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_NEG       :
    case EXP_OP_PARAM_MBIT_NEG :
      { 
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, "-:", rstr, "]", net );
        free_safe( str, slen );
      }
      break;
    default :
      break;
  }

  PROFILE_END;

}

/*!
 Generates temporary subexpression for the given expression (not recursively)
*/
static void generator_insert_subexp(
  expression* exp,        /*!< Pointer to the current expression */
  func_unit*  funit,      /*!< Pointer to the functional unit that exp exists in */
  int         depth,      /*!< Current subexpression depth */
  bool        net,        /*!< If TRUE, specifies that we are generating for a net */
  bool        reg_needed  /*!< If TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_SUBEXP);

  char*        lhs_str = NULL;
  char*        lstr;
  char*        rstr;
  unsigned int rv;
  unsigned int i;

  /* Create LHS portion of assignment */
  lhs_str = generator_create_lhs( exp, funit, net, reg_needed );

  /* Generate left and right subexpression code string */
  lstr = generator_create_subexp( exp->left,  exp->op, funit, depth, net );
  rstr = generator_create_subexp( exp->right, exp->op, funit, depth, net );

  /* Create output expression and add it to the working list */
  generator_create_exp( exp, lhs_str, lstr, rstr, net );

  /* Deallocate strings */
  free_safe( lhs_str, (strlen( lhs_str ) + 1) );
  free_safe( lstr, (strlen( lstr ) + 1) );
  free_safe( rstr, (strlen( rstr ) + 1) );

  PROFILE_END;

}

/*!
 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static void generator_insert_comb_cov_helper(
  expression*  exp,        /*!< Pointer to expression tree to operate on */
  func_unit*   funit,      /*!< Pointer to current functional unit */
  exp_op_type  parent_op,  /*!< Parent expression operation (originally should be set to the same operation as exp) */
  int          depth,      /*!< Current expression depth (originally set to 0) */
  bool         net,        /*!< If set to TRUE generate code for a net */
  bool         root,       /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed  /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER);

  if( exp != NULL ) {

    int child_depth = (depth + ((exp->op != parent_op) ? 1 : 0));

    // printf( "In generator_insert_comb_cov_helper, expr: %s\n", expression_string( exp ) );

    /* Only continue to traverse tree if we are within our depth limit */
    if( (depth < generator_max_exp_depth) && (EXPR_IS_MEASURABLE( exp ) == 1) && !expression_is_static_only( exp ) ) {

      /* Generate event combinational logic type */
      if( EXPR_IS_EVENT( exp ) ) {
        generator_insert_event_comb_cov( exp, funit, reg_needed );

      /* Otherwise, generate binary combinational logic type */
      } else if( EXPR_IS_COMB( exp ) ) {
        generator_insert_comb_comb_cov( exp, funit, net, reg_needed );
        if( (exp->left != NULL) && ((child_depth >= generator_max_exp_depth) || !EXPR_IS_MEASURABLE( exp->left ) || expression_is_static_only( exp->left )) ) {
          generator_insert_subexp( exp->left,  funit, depth, net, reg_needed );
        }
        if( (exp->right != NULL) && ((child_depth >= generator_max_exp_depth) || !EXPR_IS_MEASURABLE( exp->right ) || expression_is_static_only( exp->right )) ) {
          generator_insert_subexp( exp->right, funit, depth, net, reg_needed );
        }
        if( !root ) {
          generator_insert_subexp( exp, funit, depth, net, reg_needed );
        }

      /* Generate unary combinational logic type */
      } else {
        generator_insert_unary_comb_cov( exp, funit, depth, net, reg_needed );
        generator_insert_subexp( exp, funit, depth, net, reg_needed );

      }

    }

    /* Generate children expression trees */
    generator_insert_comb_cov_helper( exp->left,  funit, exp->op, child_depth, net, FALSE, reg_needed );
    generator_insert_comb_cov_helper( exp->right, funit, exp->op, child_depth, net, FALSE, reg_needed );

  }

  PROFILE_END;

}

/*!
 Generates a memory index value for a given memory expression.
*/
static char* generator_gen_mem_index(
  expression* exp,     /*!< Pointer to expression accessign memory signal */
  func_unit*  funit,   /*!< Pointer to functional unit containing exp */
  bool        is_dim0  /*!< Set to TRUE if the current expression is dimension 0 */
) { PROFILE(GENERATOR_GEN_MEM_INDEX);

  char*        index = codegen_gen_expr_one_line( exp->left, funit );
  char*        str;
  char         num[50];
  unsigned int slen;
  unsigned int rv;

  printf( "In generator_gen_mem_index, exp: %s\n", expression_string( exp ) );

  rv = snprintf( num, 50, "%d", exp->elem.dim->dim_width );
  assert( rv < 50 );

  slen = 1 + strlen( index ) + 3 + strlen( num ) + 2;
  str  = (char*)malloc_safe( slen );
  rv   = snprintf( str, slen, "(%s * %s)", index, num );
  assert( rv < slen );

  if( !exp->elem.dim->last ) {

    char* tmpstr = str;
    char* rest   = generator_gen_mem_index( (is_dim0 ? exp->parent->expr->right : exp->parent->expr->parent->expr->right), funit, FALSE );

    slen = strlen( tmpstr ) + 3 + strlen( rest ) + 1;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "%s + %s", tmpstr, rest );
    assert( rv < slen ); 

    free_safe( rest,   (strlen( rest )   + 1) );
    free_safe( tmpstr, (strlen( tmpstr ) + 1) );

  }

  free_safe( index, (strlen( index ) + 1) );

  PROFILE_END;

  return( str );

}

/*!
 Inserts memory coverage for the given expression.
*/
static void generator_insert_mem_cov(
  expression* exp,    /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,  /*!< Pointer to functional unit containing the given expression */
  bool        net,    /*!< If TRUE, creates the signal name for a net; otherwise, creates the signal name for a register */
  bool        write   /*!< If TRUE, creates write logic; otherwise, creates read logic */
) { PROFILE(GENERATOR_INSERT_MEM_COV);

  char         name[4096];
  char         range[4096];
  unsigned int rv;
  char*        idxstr   = generator_gen_mem_index( exp, funit, TRUE );
  char*        value;
  char*        str;
  expression*  last_exp = expression_get_last_line_expr( exp );

  /* Now insert the code to store the index and memory */
  if( write ) {

    char*        msb;
    char*        memstr;
    unsigned int vlen;
    char         num[50];
    char         iname[4096];
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;

    /* First, create the wire/register to hold the index */
    if( net || (funit->type == FUNIT_MODULE) ) {
      rv = snprintf( iname, 4096, " \\covered$%c%d_%d_%x$%s ", (net ? 'i' : 'I'), exp->line, last_exp->line, exp->col, exp->name );
    } else {
      rv = snprintf( iname, 4096, " \\covered$%c%d_%d_%x$%s$%s ", (net ? 'i' : 'I'), exp->line, last_exp->line, exp->col, exp->name, funit->name );
    }
    assert( rv < 4096 );

    /* Figure out the size of the dimensional width */
    rv = snprintf( num, 50, "%d", (exp->elem.dim->dim_width - 1) );
    assert( rv < 50 );

    if( net ) {

      unsigned int slen = 6 + strlen( num ) + 4 + strlen( name ) + 3 + strlen( idxstr ) + 2;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "wire [%s:0] %s = %s;", num, iname, idxstr );
      assert( rv < slen );

    } else {

      unsigned int slen = 5 + strlen( num ) + 4 + strlen( iname ) + 2;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "reg [%s:0] %s;", num, iname );
      assert( rv < slen );
      str_link_add( str, &reg_head, &reg_tail );

      slen = 1 + strlen( iname ) + 3 + strlen( idxstr ) + 2;
      str  = (char*)malloc_safe( slen );
      rv   = snprintf( str, slen, " %s = %s;", iname, idxstr );
      assert( rv < slen );

    }

    /* Prepend the index */
    str_link_add( str, &tmp_head, &tmp_tail );
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }

    /* Create name */
    if( net || (funit->type == FUNIT_MODULE) ) {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s ", (net ? 'w' : 'W'), exp->line, last_exp->line, exp->col, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s$%s ", (net ? 'w' : 'W'), exp->line, last_exp->line, exp->col, exp->name, funit->name );
    }
    assert( rv < 4096 );

    /* Generate msb */
    msb = generator_gen_msb( exp, funit );

    /* Create the range information for the write */
    rv = snprintf( range, 4096, "[(%s+%d):0]", msb, exp->elem.dim->dim_width );
    assert( rv < 4096 );

    /* Create the value to assign */
    memstr = codegen_gen_expr_one_line( exp, funit );
    vlen   = 1 + strlen( memstr ) + 1 + strlen( iname ) + 2;
    value  = (char*)malloc_safe( vlen );
    rv = snprintf( value, vlen, "{%s,%s}", memstr, iname );
    assert( rv < vlen );

    /* Deallocate temporary strings */
    free_safe( msb, (strlen( msb ) + 1) );
    free_safe( memstr, (strlen( memstr ) + 1) );

  } else {

    /* Create name */
    if( net || (funit->type == FUNIT_MODULE) ) {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s ", (net ? 'r' : 'R'), exp->line, last_exp->line, exp->col, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s$%s ", (net ? 'r' : 'R'), exp->line, last_exp->line, exp->col, exp->name, funit->name );
    }
    assert( rv < 4096 );

    /* Create the range information for the read */
    rv = snprintf( range, 4096, "[%d:0]", (exp->elem.dim->dim_width - 1) );
    assert( rv < 4096 );

    /* Create the value to assign */
    value  = idxstr;
    idxstr = NULL;

  }

  /* Create the assignment string for a net */
  if( net ) {

    unsigned int slen = 4 + 1 + strlen( range ) + 1 + strlen( name ) + 3 + strlen( value ) + 2;

    str = (char*)malloc_safe( slen );
    rv  = snprintf( str, slen, "wire %s %s = %s;", range, name, value );
    assert( rv < slen );

  /* Otherwise, create the assignment string for a register and create the register */
  } else {

    unsigned int slen = 3 + 1 + strlen( range ) + 1 + strlen( name ) + 2;

    str = (char*)malloc_safe( slen );
    rv  = snprintf( str, slen, "reg %s %s;", range, name );
    assert( rv < slen );

    /* Add the register to the register list */
    str_link_add( str, &reg_head, &reg_tail );

    /* Now create the assignment string */
    slen = 1 + strlen( name ) + 3 + strlen( value ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, " %s = %s;", name, value );
    assert( rv < slen );

  }

  /* Append the line coverage assignment to the working buffer */
  generator_add_to_work_code( str, FALSE );

  /* Deallocate temporary memory */
  free_safe( idxstr, (strlen( idxstr ) + 1) );
  free_safe( value,  (strlen( value )  + 1) );
  free_safe( str,    (strlen( str )    + 1) );

  PROFILE_END;

}

/*!
 \return Returns TRUE if the memory was handled in this call to the function; otherwise, returns FALSE.
*/
static bool generator_insert_mem_cov_helper2(
  expression* exp,
  func_unit*  funit,
  bool        net,
  bool        treat_as_rhs
) { PROFILE(GENERATOR_INSERT_MEM_COV_HELPER2);

  bool retval = FALSE;

  if( exp != NULL ) {

    if( (exp->right != NULL) && (exp->right->op != EXP_OP_DIM) ) {
      if( exp->right->elem.dim->set_mem_rd ) {
        generator_insert_mem_cov( exp->right, funit, net, ((exp->suppl.part.lhs == 1) && !treat_as_rhs) );
        retval = TRUE;
      } else if( (exp->left != NULL) && (exp->left->op != EXP_OP_DIM) && exp->left->elem.dim->set_mem_rd ) {
        generator_insert_mem_cov( exp->left,  funit, net, ((exp->suppl.part.lhs == 1) && !treat_as_rhs) );
        retval = TRUE;
      }
    } else if( (exp->left != NULL) && exp->left->elem.dim->set_mem_rd && !generator_insert_mem_cov_helper2( exp->right, funit, net, treat_as_rhs ) ) {
      generator_insert_mem_cov( exp->left, funit, net, ((exp->suppl.part.lhs == 1) && !treat_as_rhs) );
      retval = TRUE;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 Traverses the specified expression tree searching for memory accesses.  If any are found, the appropriate
 coverage code is inserted into the output buffers.
*/
static void generator_insert_mem_cov_helper(
  expression* exp,          /*!< Pointer to current expression */
  func_unit*  funit,        /*!< Pointer to functional unit containing the expression */
  bool        net,          /*!< Specifies if the code generator should produce code for a net or a register */
  bool        treat_as_rhs  /*!< If TRUE, treats any memory access as a memory read regardless of position (by default, set to FALSE) */
) { PROFILE(GENERATOR_INSERT_MEM_COV_HELPER);

  if( exp != NULL ) {

    if( (exp->op == EXP_OP_DIM) || ((exp->sig != NULL) && (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM)) ) {
      generator_insert_mem_cov_helper2( exp, funit, net, treat_as_rhs );
    } else {
      generator_insert_mem_cov_helper( exp->left,  funit, net, treat_as_rhs );
      generator_insert_mem_cov_helper( exp->right, funit, net, treat_as_rhs );
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the statement inserted (or NULL if no statement was inserted).

 Insert combinational logic coverage code for the given expression (by file position).
*/
statement* generator_insert_comb_cov(
  unsigned int first_line,    /*!< First line of expression to generate for */
  unsigned int first_column,  /*!< First column of expression to generate for */
  bool         net,           /*!< If set to TRUE, generate code for a net; otherwise, generate code for a variable */
  bool         use_right,     /*!< If set to TRUE, use right-hand expression */
  bool         save_stmt      /*!< If set to TRUE, saves the found statement to the statement stack */
) { PROFILE(GENERATOR_INSERT_COMB_COV);

  statement* stmt = NULL;

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( (generator_comb || generator_mem) && ((stmt = generator_find_statement( first_line, first_column )) != NULL) ) {

    /* Generate combinational coverage */
    if( generator_comb ) {
      generator_insert_comb_cov_helper( (use_right ? stmt->exp->right : stmt->exp), stmt->funit, (use_right ? stmt->exp->right->op : stmt->exp->op), 0, net, TRUE, TRUE );
    }

    /* Generate memory coverage */
    if( generator_mem ) {
      generator_insert_mem_cov_helper( stmt->exp, stmt->funit, net, FALSE );
    }

  }

  /* If we need to save the found statement, do so now */
  if( save_stmt ) {

    stmt_loop_link* new_stmtl;

    assert( stmt != NULL );

    new_stmtl            = (stmt_loop_link*)malloc_safe( sizeof( stmt_loop_link ) );
    new_stmtl->stmt      = stmt;
    new_stmtl->next      = stmt_stack;
    new_stmtl->next_true = use_right;
    stmt_stack           = new_stmtl;

  }

  PROFILE_END;

  return( stmt );

}

/*!
 \return Returns a pointer to the inserted statement (or NULL if no statement was inserted).

 Inserts combinational coverage information from statement stack (and pop stack).
*/
statement* generator_insert_comb_cov_from_stmt_stack() { PROFILE(GENERATOR_INSERT_COMB_COV_FROM_STMT_STACK);

  statement* stmt = NULL;

  if( generator_comb ) {

    stmt_loop_link* sll;
    expression*     exp;

    assert( stmt_stack != NULL );

    stmt = stmt_stack->stmt;
    sll  = stmt_stack;
    exp  = stmt_stack->next_true ? stmt->exp->right : stmt->exp;

    /* Generate combinational coverage information */
    generator_insert_comb_cov_helper( exp, stmt->funit, exp->op, 0, FALSE, TRUE, FALSE );

    /* Now pop the statement stack */
    stmt_stack = sll->next;
    free_safe( sll, sizeof( stmt_loop_link ) );

  }

  PROFILE_END;

  return( stmt );

}

/*!
 Inserts combinational coverage information for the given statement.
*/
void generator_insert_comb_cov_with_stmt(
  statement* stmt,       /*!< Pointer to statement to generate combinational logic coverage for */
  bool       use_right,  /*!< Specifies if the right expression should be used in the statement */
  bool       reg_needed  /*!< If TRUE, instantiate necessary registers */
) { PROFILE(GENERATOR_INSERT_COMB_COV_WITH_STMT);

  if( generator_comb && (stmt != NULL) ) {

    expression* exp = use_right ? stmt->exp->right : stmt->exp;

    /* Insert combinational coverage */
    generator_insert_comb_cov_helper( exp, stmt->funit, exp->op, 0, FALSE, TRUE, reg_needed );

  }

  PROFILE_END;

}

/*!
 Handles combinational logic for an entire case block (and its case items -- not the case item logic blocks).
*/
void generator_insert_case_comb_cov(
  unsigned int first_line,   /*!< First line number of first statement in case block */
  unsigned int first_column  /*!< First column of first statement in case block */
) { PROFILE(GENERATOR_INSERT_CASE_COMB_COV);

  statement* stmt;

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( generator_comb && ((stmt = generator_find_case_statement( first_line, first_column )) != NULL) ) {

    generator_insert_comb_cov_helper( stmt->exp->left, stmt->funit, stmt->exp->left->op, 0, FALSE, TRUE, TRUE );

#ifdef FUTURE_ENHANCEMENT
    /* Generate covered for the current case item */
    generator_insert_comb_cov_helper( stmt->exp, stmt->funit, stmt->exp->op, 0, FALSE, TRUE, TRUE );

    /* If the current statement is a case item type, handle it; otherwise, we are done */
    while( !stmt->suppl.part.stop_false &&
           ((stmt->next_false->exp->op == EXP_OP_CASE) || (stmt->next_false->exp->op == EXP_OP_CASEX) || (stmt->next_false->exp->op == EXP_OP_CASEZ)) ) {

      /* Move statement to next false statement */
      stmt = stmt->next_false;

      /* Generate covered for the current case item */
      generator_insert_comb_cov_helper( stmt->exp, stmt->funit, stmt->exp->op, 0, FALSE, TRUE, TRUE );

    }
#endif

  }

  PROFILE_END;

}

/*!
 Inserts FSM coverage at the end of the module for the current module.
*/
void generator_insert_fsm_covs() { PROFILE(GENERATOR_INSERT_FSM_COVS);

  if( generator_fsm ) {

    fsm_link*    fsml = curr_funit->fsm_head;
    unsigned int id   = 1;

    while( fsml != NULL ) {

      if( fsml->table->from_state->id == fsml->table->to_state->id ) {

        char* msb = generator_gen_msb( fsml->table->from_state, curr_funit );
        char* exp = codegen_gen_expr_one_line( fsml->table->from_state, curr_funit );
        fprintf( curr_ofile, "wire [(%s-1):0] \\covered$F%d = %s;\n", ((msb != NULL) ? msb : "1"), id, exp );
        free_safe( msb, (strlen( msb ) + 1) );
        free_safe( exp, (strlen( exp ) + 1) );

      } else {

        char* fmsb = generator_gen_msb( fsml->table->from_state, curr_funit );
        char* fexp = codegen_gen_expr_one_line( fsml->table->from_state, curr_funit );
        char* tmsb = generator_gen_msb( fsml->table->to_state, curr_funit );
        char* texp = codegen_gen_expr_one_line( fsml->table->to_state, curr_funit );
        fprintf( curr_ofile, "wire [((%s+%s)-1):0] \\covered$F%d = {%s,%s};\n",
                 ((fmsb != NULL) ? fmsb : "1"), ((tmsb != NULL) ? tmsb : "1"), id, fexp, texp );
        free_safe( fmsb, (strlen( fmsb ) + 1) );
        free_safe( fexp, (strlen( fexp ) + 1) );
        free_safe( tmsb, (strlen( tmsb ) + 1) );
        free_safe( texp, (strlen( texp ) + 1) );

      }

      fsml = fsml->next;
      id++;

    }

  }

  PROFILE_END;

}


/*
 $Log$
 Revision 1.50  2009/01/01 07:24:44  phase1geo
 Checkpointing work on memory coverage.  Simple testing now works but still need
 to do some debugging here.

 Revision 1.49  2009/01/01 00:19:40  phase1geo
 Adding memory coverage insertion code.  Still need to add memory coverage handling
 code during runtime.  Checkpointing.

 Revision 1.48  2008/12/29 05:40:09  phase1geo
 Regression updates.  Starting to add memory coverage (not even close to being
 finished at this point).  Checkpointing.

 Revision 1.47  2008/12/28 19:39:17  phase1geo
 Fixing the handling of wait statements.  Updated regressions as necessary.
 Checkpointing.

 Revision 1.46  2008/12/28 04:14:54  phase1geo
 Fixing support for case statement combinational coverage.  Updating regressions.
 We are now down to 41 failures in regression.  Checkpointing.

 Revision 1.45  2008/12/27 21:05:55  phase1geo
 Updating CDD version and regressions per this change.  Checkpointing.

 Revision 1.44  2008/12/27 04:47:47  phase1geo
 Updating regressions.  Added code to support while loops; however, the new code does
 not support FOR loops as I was hoping so I might end up reverting these changes somewhat
 in the end.  Checkpointing.

 Revision 1.43  2008/12/24 21:48:15  phase1geo
 Fixing issue with naming of FSM register.

 Revision 1.42  2008/12/24 21:19:01  phase1geo
 Initial work at getting FSM coverage put in (this looks to be working correctly
 to this point).  Updated regressions per fixes.  Checkpointing.

 Revision 1.41  2008/12/20 06:35:26  phase1geo
 Fixing more IV regression bugs and adding regression Makefile support for finding
 library files in the covered/verilog directory.  About 104 diagnostics are now
 failing.  Checkpointing.

 Revision 1.40  2008/12/20 06:04:25  phase1geo
 IV regression fixes for inlined code coverage (around 117 tests are still
 failing at this point).  Checkpointing.

 Revision 1.39  2008/12/19 19:07:01  phase1geo
 More regression updates.

 Revision 1.38  2008/12/19 06:55:07  phase1geo
 A few code fixes.  Checkpointing.

 Revision 1.37  2008/12/19 00:05:12  phase1geo
 IV regression updates.  Checkpointing.

 Revision 1.36  2008/12/17 22:31:31  phase1geo
 Fixing unary and combination expression coverage bug.  Adding support for do..while.
 Checkpointing.

 Revision 1.35  2008/12/17 18:17:18  phase1geo
 Checkpointing inlined code coverage work.

 Revision 1.34  2008/12/17 15:23:02  phase1geo
 More updates for inlined code coverage.  Updating regressions per these changes.
 Checkpointing.

 Revision 1.33  2008/12/17 00:02:57  phase1geo
 More work on inlined coverage code.  Making good progress through the regression
 suite.  Checkpointing.

 Revision 1.32  2008/12/16 04:56:39  phase1geo
 More updates for inlined code generation feature.  Updates to regression per
 these changes.  Checkpointing.

 Revision 1.31  2008/12/16 00:18:08  phase1geo
 Checkpointing work on for2 diagnostic.  Adding initial support for fork..join
 blocks -- more work to do here.  Starting to add support for FOR loop control
 logic although I'm thinking that I may want to pull this coverage support from
 the current coverage handling structures.

 Revision 1.30  2008/12/15 06:48:52  phase1geo
 More updates to code generator for regressions.  Checkpointing.  Updates to
 regressions per these changes.

 Revision 1.29  2008/12/14 06:56:02  phase1geo
 Making some code modifications to set the stage for supporting case statements
 with the new inlined code coverage methodology.  Updating regressions per this
 change (IV and Cver fully pass).

 Revision 1.28  2008/12/13 00:17:28  phase1geo
 Fixing more regression bugs.  Updated some original tests to make them comparable to the inlined method output.
 Checkpointing.

 Revision 1.27  2008/12/12 05:57:50  phase1geo
 Checkpointing work on code coverage injection.  We are making decent progress in
 getting regressions back to a working state.  Lots to go, but good progress at
 this point.

 Revision 1.26  2008/12/12 00:17:30  phase1geo
 Fixing some bugs, creating some new ones...  Checkpointing.

 Revision 1.25  2008/12/11 05:53:32  phase1geo
 Fixing some bugs in the combinational logic code coverage generator.  Checkpointing.

 Revision 1.24  2008/12/10 23:37:02  phase1geo
 Working on handling event combinational logic cases.  This does not fully work
 at this point.  Fixed issues with combinational logic generation for IF statements.
 Checkpointing.

 Revision 1.23  2008/12/10 06:25:38  phase1geo
 More work on LHS signal sizing (not complete yet but almost).  Fixed several issues
 found with regression runs.  Still working on always1.v failures.  Checkpointing.

 Revision 1.22  2008/12/10 00:19:23  phase1geo
 Fixing issues with aedge1 diagnostic (still need to handle events but this will
 be worked on a later time).  Working on sizing temporary subexpression LHS signals.
 This is not complete and does not compile at this time.  Checkpointing.

 Revision 1.21  2008/12/09 06:17:24  phase1geo
 More work on inline code generator.  Also working on verilog/Makefile for regression
 runs.  Checkpointing.

 Revision 1.20  2008/12/09 00:20:14  phase1geo
 More work on code generator.  Also made first changes to verilog Makefile for
 regression testing (more work to do here).

 Revision 1.19  2008/12/07 07:20:08  phase1geo
 Checkpointing work.  I have an end-to-end run now working with test.v in
 the testsuite.  The results are not accurate at this point but it's progress.
 I have updated the regression suite per these changes (minor), added an "-inline"
 option to the score command to control this behavior.  IV regressions have one
 failing diagnostic at this point.

 Revision 1.18  2008/12/06 06:35:19  phase1geo
 Adding first crack at handling coverage-related information from dumpfile.
 This code is untested.

 Revision 1.17  2008/12/05 06:11:56  phase1geo
 Fixing always block handling and generator_find_statement functionality.
 Improving statement link display output.

 Revision 1.16  2008/12/05 05:48:30  phase1geo
 More work on code coverage insertion.  Checkpointing.

 Revision 1.15  2008/12/05 04:39:14  phase1geo
 Checkpointing.  Updating regressions.

 Revision 1.14  2008/12/05 00:22:41  phase1geo
 More work completed on code coverage generator.  Currently working on bug in
 statement finder.  Checkpointing.

 Revision 1.13  2008/12/04 14:19:50  phase1geo
 Fixing bug in code generator.  Checkpointing.

 Revision 1.12  2008/12/04 07:14:39  phase1geo
 Doing more work on the code generator to handle combination logic output.
 Still more coding and debugging work to do here.  Need to clear the added
 bit in the statement lists to get current code working correctly.  Checkpointing.

 Revision 1.11  2008/12/03 23:29:07  phase1geo
 Finished getting line coverage insertion working.  Starting to work on combinational logic
 coverage.  Checkpointing.

 Revision 1.10  2008/12/03 17:15:10  phase1geo
 Code to output coverage file is now working from an end-to-end perspective.  Checkpointing.
 We are now ready to start injecting actual coverage information into this file.

 Revision 1.9  2008/12/03 07:27:01  phase1geo
 Made initial pass through the parser to add parse_mode.  Things are quite broken
 in regression at this point and we have conflicts in the resultant parser.
 Checkpointing.

 Revision 1.8  2008/12/02 23:43:21  phase1geo
 Reimplementing inlined code generation code.  Added this code to Verilog lexer and parser.
 More work to do here.  Checkpointing.

 Revision 1.7  2008/12/01 06:02:55  phase1geo
 More updates to get begin/end code injection to work.  This is still not fully working
 at this point.  Checkpointing.

 Revision 1.6  2008/11/30 20:48:09  phase1geo
 More work on instrumenting Verilog code.  Added statement iterator and fixed a few
 issues with the code.  Checkpointing.

 Revision 1.5  2008/11/29 04:27:07  phase1geo
 More work on inlined coverage code insertion.  Net assigns and procedural assigns
 seem to be working at a most basic level.  Currently, I have an issue that I need
 to solve where non-head statements are being ignored.  Checkpointing.

 Revision 1.4  2008/11/27 00:24:44  phase1geo
 Fixing problems with previous version of generator.  Things work as expected at this point.
 Checkpointing.

 Revision 1.3  2008/11/27 00:01:50  phase1geo
 More work on coverage generator.  Checkpointing.

 Revision 1.2  2008/11/26 05:34:48  phase1geo
 More work on Verilog generator file.  We are now able to create the needed
 directories and output a non-instrumented version of a module to the appropriate
 directory.  Checkpointing.

 Revision 1.1  2008/11/25 23:53:07  phase1geo
 Adding files for Verilog generator functions.

*/

