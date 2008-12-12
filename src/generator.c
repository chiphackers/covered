/*
 Copyright (c) 2006-2008 Trevor Williams

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
 Iterator for current functional unit.
*/
static func_iter fiter;


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

  /* Reset the structure */
  func_iter_reset( &fiter );

  PROFILE_END;

}

/*!
 Adds the given string to the working code buffer.
*/
void generator_add_to_work_code(
  const char*  str  /*!< String to write */
) { PROFILE(GENERATOR_ADD_TO_WORK_CODE);

  static bool semi_just_seen = FALSE;
  bool        add            = TRUE;

  /* Make sure that we remove back-to-back semicolons */
  if( strcmp( str, ";" ) == 0 ) {
    if( semi_just_seen ) {
      add = FALSE;
    }
    semi_just_seen = TRUE;
  } else if( (str[0] != ' ') && (str[0] != '\n') && (str[0] != '\t') && (str[0] != '\r') && (str[0] != '\b') ) {
    semi_just_seen = FALSE;
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

  expf_link* expfl      = eventl->expf_head;
  expf_link* tmpefl;
  bool       end_needed = FALSE;

  while( expfl != NULL ) {
    if( expfl->funit->type == FUNIT_MODULE ) {
      fprintf( curr_ofile, "reg \\covered$E%d_%x ;\n", expfl->exp->line, expfl->exp->col );
    } else {
      fprintf( curr_ofile, "reg \\covered$E%d_%x$%s ;\n", expfl->exp->line, expfl->exp->col, expfl->funit->name );
    }
    expfl = expfl->next;
  }

  if( eventl->name[0] == '@' ) {
    fprintf( curr_ofile, "always %s", eventl->name );
  } else {
    fprintf( curr_ofile, "always @(%s)", eventl->name );
  }
  if( (eventl->expf_head != NULL) && (eventl->expf_head->next != NULL) ) {
    fprintf( curr_ofile, " begin\n" );
    end_needed = TRUE;
  }

  /* Output the event assignments */
  expfl = eventl->expf_head;
  while( expfl != NULL ) {
    tmpefl = expfl;
    expfl  = expfl->next;
    if( tmpefl->funit->type == FUNIT_MODULE ) {
      fprintf( curr_ofile, "  \\covered$E%d_%x = 1'b1;\n", tmpefl->exp->line, tmpefl->exp->col );
    } else {
      fprintf( curr_ofile, "  \\covered$E%d_%x$%s = 1'b1;\n", tmpefl->exp->line, tmpefl->exp->col, tmpefl->funit->name );
    }
    free_safe( tmpefl, sizeof( expf_link ) );
  }

  if( end_needed ) {
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
static statement* generator_find_statement(
  unsigned int first_line,   /*!< First line of statement to find */
  unsigned int first_column  /*!< First column of statement to find */
) { PROFILE(GENERATOR_FIND_STATEMENT);

  static statement* stmt = NULL;

//  printf( "In generator_find_statement, line: %d, column: %d\n", first_line, first_column );

  if( (stmt == NULL) || (stmt->exp->line < first_line) || ((stmt->exp->line == first_line) && (((stmt->exp->col >> 16) & 0xffff) < first_column)) ) {

    /* Attempt to find the expression with the given position */
    while( ((stmt = func_iter_get_next_statement( &fiter )) != NULL) &&
//           printf( "  statement %s %d\n", expression_string( stmt->exp ), ((stmt->exp->col >> 16) & 0xffff) ) &&
           ((stmt->exp->line < first_line) || ((stmt->exp->line == first_line) && (((stmt->exp->col >> 16) & 0xffff) < first_column))) );

  }

//  if( (stmt != NULL) && (stmt->exp->line == first_line) && (((stmt->exp->col >> 16) & 0xffff) == first_column) ) {
//    printf( "  FOUND (%s %x)!\n", expression_string( stmt->exp ), ((stmt->exp->col >> 16) & 0xffff) );
//  } else {
//    printf( "  NOT FOUND!\n" );
//  }

  PROFILE_END;

  return( ((stmt == NULL) || (stmt->exp->line != first_line) || (((stmt->exp->col >> 16) & 0xffff) != first_column)) ? NULL : stmt );

}

/*!
 Inserts line coverage information in string queues.
*/
void generator_insert_line_cov(
  unsigned int first_line,    /*!< Line to create line coverage for */
  unsigned int first_column,  /*!< First column of statement */
  unsigned int last_column    /*!< Last column of statement */
) { PROFILE(GENERATOR_INSERT_LINE_COV);

  statement* stmt;

  if( generator_line && ((stmt = generator_find_statement( first_line, first_column )) != NULL) ) {

    char         str[4096];
    char         sig[4096];
    unsigned int rv;
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;

    if( stmt->funit->type == FUNIT_MODULE ) {
      rv = snprintf( sig, 4096, " \\covered$l%d_%x ", first_line, (((first_column & 0xffff) << 16) | (last_column & 0xffff)) );
    } else {
      rv = snprintf( sig, 4096, " \\covered$l%d_%x$%s ", first_line, (((first_column & 0xffff) << 16) | (last_column & 0xffff)), stmt->funit->name );
    }
    assert( rv < 4096 );

    /* Create the register */
    rv = snprintf( str, 4096, "reg %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );

    /* Prepend the line coverage assignment to the working buffer */
    rv = snprintf( str, 4096, " %s = 1'b1;", sig );
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
 Handles the insertion of event-type combinational logic code.
*/
static void generator_insert_event_comb_cov(
  expression* exp,   /*!< Pointer to expression to output */
  func_unit*  funit  /*!< Pointer to functional unit containing the expression */
) { PROFILE(GENERATOR_INSERT_EVENT_COMB_COV);

  /*
   If the expression is a root of an expression tree and it is a single event, just insert the event coverage code
   immediately.
  */
  if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) && ((exp->op == EXP_OP_PEDGE) || (exp->op == EXP_OP_NEDGE) || (exp->op == EXP_OP_AEDGE)) ) {

    char         str[4096];
    char         name[4096];
    unsigned int rv;

    /* Create signal name */
    if( funit->type == FUNIT_MODULE ) {
      rv = snprintf( name, 4096, " \\covered$E%d_%x ", exp->line, exp->col );
    } else {
      rv = snprintf( name, 4096, " \\covered$E%d_%x$%s ", exp->line, exp->col, funit->name );
    }
    assert( rv < 4096 );

    /* Create register */
    rv = snprintf( str, 4096, "reg %s;", name );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );

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

    /* Create the event string */
    codegen_gen_expr( exp, exp->op, &code, &code_depth, funit );

    /* Change the event array into a single string */
    event_str = code[0];
    for( i=1; i<code_depth; i++ ) {
      event_str = (char*)realloc_safe( event_str, (strlen( event_str ) + 1), (strlen( event_str ) + strlen( code[i] ) + 1) );
      strcat( event_str, code[i] );
      free_safe( code[i], (strlen( code[i] ) + 1) );
    }
    free_safe( code, (sizeof( char* ) * code_depth) );

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
  expression*  exp,    /*!< Pointer to expression to output */
  func_unit*   funit,  /*!< Pointer to functional unit containing this expression */
  unsigned int depth,  /*!< Current expression depth */
  bool         net     /*!< Set to TRUE if this expression is a net */
) { PROFILE(GENERATOR_INSERT_UNARY_COMB_COV);

  char         prefix[16];
  char         sig[80];
  char         sigr[80];
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;

  /* Create signal */
  if( net || (funit->type == FUNIT_MODULE) ) {
    rv = snprintf( sig,  80, " \\covered$%c%d_%x ", (net ? 'e' : 'E'), exp->line, exp->col );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$%c%d_%x ", (net ? 'y' : 'x'), exp->line, exp->col );
    assert( rv < 80 );
  } else {
    rv = snprintf( sig,  80, " \\covered$E%d_%x$%s ", exp->line, exp->col, funit->name );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$x%d_%x$%s ", exp->line, exp->col, funit->name );
    assert( rv < 80 );
  }

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire " );
  } else {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = |(%s);", prefix, sig, sigr );
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
  expression*  exp,    /*!< Pointer to expression to output */
  func_unit*   funit,  /*!< Pointer to functional unit containing this expression */
  unsigned int depth,  /*!< Current expression depth */
  bool         net     /*!< Set to TRUE if this expression is a net */
) { PROFILE(GENERATOR_INSERT_AND_COMB_COV);

  char         prefix[16];
  char         sig[80];
  char         sigl[80];
  char         sigr[80];
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;

  /* Create signal */
  if( net || (funit->type == FUNIT_MODULE) ) {
    rv = snprintf( sig,  80, " \\covered$%c%d_%x ", (net ? 'e' : 'E'), exp->line, exp->col );
    assert( rv < 80 );
    rv = snprintf( sigl, 80, " \\covered$%c%d_%x ",
                   ((((depth + ((exp->op != exp->left->op) ? 1 : 0)) < generator_max_exp_depth) && EXPR_IS_MEASURABLE( exp->left ) && !expression_is_static_only( exp->left )) ? (net ? 'e' : 'E') : (net ? 'y' : 'x')),
                   exp->left->line, exp->left->col );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$%c%d_%x ",
                   ((((depth + ((exp->op != exp->right->op) ? 1 : 0)) < generator_max_exp_depth) && EXPR_IS_MEASURABLE( exp->right ) && !expression_is_static_only( exp->right )) ? (net ? 'e' : 'E') : (net ? 'y' : 'x')),
                   exp->right->line, exp->right->col );
    assert( rv < 80 );
  } else {
    rv = snprintf( sig,  80, " \\covered$E%d_%x$%s ", exp->line, exp->col, funit->name );
    assert( rv < 80 );
    rv = snprintf( sigl, 80, " \\covered$%c%d_%x$%s ",
                   ((((depth + ((exp->op != exp->left->op) ? 1 : 0)) < generator_max_exp_depth) && EXPR_IS_MEASURABLE( exp->left ) && !expression_is_static_only( exp->left )) ? 'E' : 'x'),
                   exp->left->line, exp->left->col, funit->name );
    assert( rv < 80 );
    rv = snprintf( sigr, 80, " \\covered$%c%d_%x$%s ",
                   ((((depth + ((exp->op != exp->right->op) ? 1 : 0)) < generator_max_exp_depth) && EXPR_IS_MEASURABLE( exp->right ) && !expression_is_static_only( exp->right )) ? 'E' : 'x'),
                   exp->right->line, exp->right->col, funit->name );
    assert( rv < 80 );
  }

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [1:0] " );
  } else {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [1:0] %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = {%s,%s};", prefix, sig, sigl, sigr );
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
static void generator_gen_msb(
  expression* exp,    /*!< Pointer to subexpression to generate MSB value for */
  func_unit*  funit,  /*!< Pointer to functional unit containing this subexpression */
  char**      msb     /*!< String version of MSB */
) { PROFILE(GENERATOR_GEN_MSB);

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
          *msb = strdup_safe( tmp );
        }
        break;
      case EXP_OP_LIST     :
      case EXP_OP_MULTIPLY :
        generator_gen_msb( exp->left,  funit, &lexp );
        generator_gen_msb( exp->right, funit, &rexp );
        {
          unsigned int slen = strlen( lexp ) + strlen( rexp ) + 4;
          *msb = (char*)malloc_safe( slen );
          rv   = snprintf( *msb, slen, "(%s+%s)", lexp, rexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        break;
      case EXP_OP_NEGATE         :
      case EXP_OP_CONCAT         :
      case EXP_OP_MBIT_POS       :
      case EXP_OP_MBIT_NEG       :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        generator_gen_msb( exp->right, funit, &rexp );
        *msb = rexp;
        break;
      case EXP_OP_LSHIFT  :
      case EXP_OP_RSHIFT  :
      case EXP_OP_ALSHIFT :
      case EXP_OP_ARSHIFT :
        generator_gen_msb( exp->left, funit, &lexp );
        *msb = lexp;
        break;
      case EXP_OP_EXPAND :
        generator_gen_msb( exp->right, funit, &rexp );
        {
          char         tmp[50];
          unsigned int slen;
          rv = snprintf( tmp, 50, "%d", vector_to_int( exp->left->value ) );
          assert( rv < 50 );
          slen = strlen( tmp ) + strlen( rexp ) + 4;
          *msb = (char*)malloc_safe( slen );
          rv = snprintf( *msb, slen, "(%s*%s)", tmp, rexp );
          assert( rv < slen );
        }
        free_safe( rexp, (sizeof( rexp ) + 1) );
        break;
      case EXP_OP_STIME :
      case EXP_OP_SR2B  :
      case EXP_OP_SR2I  :
        *msb = (char*)malloc_safe( 3 );
        strcpy( *msb, "64" );
        break;
      case EXP_OP_SSR2B :
        *msb = (char*)malloc_safe( 3 );
        strcpy( *msb, "32" );
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
        *msb = strdup_safe( "1" );
        break;
      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
        generator_gen_msb( exp->left,  funit, &lexp );
        generator_gen_msb( exp->right, funit, &rexp );
        {
          unsigned int slen = (strlen( lexp ) * 3) + (strlen( rexp ) * 3) + 13;
          *msb = (char*)malloc_safe( slen );
          rv   = snprintf( *msb, slen, "((%s>%s)?(%s-%s):(%s-%s)", lexp, rexp, lexp, rexp, rexp, lexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        break;
      case EXP_OP_SIG       :
      case EXP_OP_PARAM     :
      case EXP_OP_FUNC_CALL :
        if( (exp->sig->suppl.part.type == SSUPPL_TYPE_GENVAR) || (exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_SREAL) ) {
          *msb = strdup_safe( "32" );
        } else if( exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_REAL ) {
          *msb = strdup_safe( "64" );
//        } else if( !exp->sig->suppl.part.implicit_size ) {
        } else {
          char tmp[50];
          rv = snprintf( tmp, 50, "%d", exp->sig->value->width );
          assert( rv < 50 );
          *msb = strdup_safe( tmp );
//        } else {
//          /* TBD - We need to look at the signal size expression... */
//          *msb = strdup_safe( "???" );
        }
        break;
      default :
        generator_gen_msb( exp->left,  funit, &lexp );
        generator_gen_msb( exp->right, funit, &rexp );
        if( lexp != NULL ) {
          if( rexp != NULL ) {
            unsigned int slen = (strlen( lexp ) * 2) + (strlen( rexp ) * 2) + 8;
            *msb = (char*)malloc_safe( slen );
            rv   = snprintf( *msb, slen, "((%s>%s)?%s:%s)", lexp, rexp, lexp, rexp );
            assert( rv < slen );
            free_safe( lexp, (strlen( lexp ) + 1) );
            free_safe( rexp, (strlen( rexp ) + 1) );
          } else {
            *msb = lexp;
          }
        } else {
          if( rexp != NULL ) {
            *msb = rexp;
          } else {
            *msb = NULL;
          }
        }
        break;
    }

  }

  PROFILE_END;

}

/*!
 Generates the strings needed for the left-hand-side of the temporary expression.
*/
static void generator_create_lhs(
  expression* exp,    /*!< Pointer to subexpression to create LHS string for */
  func_unit*  funit,  /*!< Functional unit containing the expression */
  bool        net,    /*!< If set to TRUE, generate code for a net; otherwise, generate for a register */
  char**      code    /*!< Pointer to string to populate */
) { PROFILE(GENERATOR_CREATE_RHS);

  unsigned int rv;
  char         name[4096];
  char         tmp[4096];
  char*        msb;

  /* Generate signal name */
  if( net || (funit->type == FUNIT_MODULE) ) {
    rv = snprintf( name, 4096, " \\covered$x%d_%x ", exp->line, exp->col );
  } else {
    rv = snprintf( name, 4096, " \\covered$x%d_%x$%s ", exp->line, exp->col, funit->name );
  }
  assert( rv < 4096 );

  /* Generate MSB string */
  generator_gen_msb( exp, funit, &msb );

  if( net ) {

    /* Create sized wire string */
    rv = snprintf( tmp, 4096, "wire [(%s-1):0] %s", ((msb != NULL) ? msb : "1"), name );
    assert( rv < 4096 );

    *code = strdup_safe( tmp );

  } else {

    /* Create sized register string */
    rv = snprintf( tmp, 4096, "reg [(%s-1):0] %s;", ((msb != NULL) ? msb : "1"), name );
    assert( rv < 4096 );
    str_link_add( strdup_safe( tmp ), &reg_head, &reg_tail );

    /* Set the name to the value of code */
    *code = strdup_safe( name );

  }

  /* Deallocate memory */
  free_safe( msb, (strlen( msb ) + 1) );

  PROFILE_END;

}

/*!
 Creates the proper subexpression code string and stores it into the code array.
*/
static char* generator_create_subexp(
            expression*   exp,        /*!< Pointer to subexpression to generate code array for */
            exp_op_type   parent_op,  /*!< Operation of parent */
            func_unit*    funit,      /*!< Pointer to functional unit that contains the given expression */
            unsigned int  depth,      /*!< Current subexpression depth */
            bool          net,        /*!< Set to TRUE if subexpression is a net */
  /*@out@*/ char***       code,       /*!< Pointer to string array with stored code */
  /*@out@*/ unsigned int* code_depth  /*!< Pointer to value containing the number of elements in the code array */
) { PROFILE(GENERATOR_CREATE_SUBEXP);

  unsigned int rv;
  unsigned int i;

  /* Initialize the code array */
  *code       = NULL;
  *code_depth = 0;

  /* Generate left string */
  if( exp != NULL ) {
    if( (((exp->op != parent_op) ? (depth + 1) : depth) < generator_max_exp_depth) && EXPR_IS_MEASURABLE( exp ) && !expression_is_static_only( exp ) ) {
      char num_str[50];
      unsigned int line_len;
      unsigned int col_len;
      rv = snprintf( num_str, 50, "%d", exp->line );  assert( rv < 50 );  line_len = strlen( num_str );
      rv = snprintf( num_str, 50, "%x", exp->col );   assert( rv < 50 );  col_len  = strlen( num_str );
      *code       = (char**)malloc_safe( sizeof( char* ) );
      *code_depth = 1;
      if( net || (funit->type == FUNIT_MODULE) ) {
        unsigned int slen = 10 + 1 + line_len + 1 + col_len + 2;
        (*code)[0] = (char*)malloc_safe( slen );
        rv = snprintf( (*code)[0], slen, " \\covered$%c%d_%x ", (net ? 'y' : 'x'), exp->line, exp->col );
        assert( rv < slen );
      } else {
        unsigned int slen = 10 + 1 + line_len + 1 + col_len + 1 + strlen( funit->name ) + 2;
        (*code)[0] = (char*)malloc_safe( slen );
        rv = snprintf( (*code)[0], slen, " \\covered$%c%d_%x$%s ", (net ? 'y' : 'x'), exp->line, exp->col, funit->name );
        assert( rv < slen );
      }
    } else {
      codegen_gen_expr( exp, exp->op, code, code_depth, funit );
    }
  }

}

/*!
 Concatenates the given string values and appends them to the working code buffer.
*/
static void generator_concat_code(
             char*        lhs,         /*!< LHS string */
  /*@null@*/ char*        before,      /*!< Optional string that precedes the left subexpression string array */
  /*@null@*/ char**       lstr,        /*!< String array from left subexpression */
             unsigned int lstr_depth,  /*!< Number of valid elements in the lstr array */
  /*@null@*/ char*        middle,      /*!< Optional string that is placed in-between the left and right subexpression array strings */
  /*@null@*/ char**       rstr,        /*!< String array from right subexpression */
             unsigned int rstr_depth,  /*!< Number of valid elements in the rstr array */
  /*@null@*/ char*        after,       /*!< Optional string that is placed after the right subpexression string array */
             bool         net          /*!< If TRUE, specifies that this subexpression exists in net logic */
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
  if( lstr_depth > 0 ) {
    if( (strlen( str ) + strlen( lstr[0] )) < 4095 ) {
      strcat( str, lstr[0] );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, lstr[0] );
    }
    if( lstr_depth > 1 ) {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
    }
    for( i=1; i<lstr_depth; i++ ) {
      str_link_add( strdup_safe( lstr[i] ), &tmp_head, &tmp_tail );
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
  if( rstr_depth > 0 ) {
    if( ((strlen( str ) + strlen( rstr[0] )) < 4095) && (lstr_depth == 1) && (rstr_depth == 1) ) {
      strcat( str, rstr[0] );
    } else {
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      for( i=0; i<(rstr_depth-1); i++ ) {
        str_link_add( strdup_safe( rstr[i] ), &tmp_head, &tmp_tail );
      }
      strcpy( str, rstr[i] );
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
  expression*  exp,         /*!< Pointer to the expression to generation the expression for */
  char*        lhs,         /*!< String for left-hand-side of temporary expression */
  char**       lstr,        /*!< String array for left side of RHS expression */
  unsigned int lstr_depth,  /*!< Number of elements in the lstr array */
  char**       rstr,        /*!< String array for right side of RHS expression */
  unsigned int rstr_depth,  /*!< Number of elements in the rstr array */
  bool         net          /*!< Set to TRUE if we are generating for a net; set to FALSE for a register */
) { PROFILE(GENERATOR_CREATE_EXP);

  switch( exp->op ) {
    case EXP_OP_XOR        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " ^ ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_MULTIPLY   :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " * ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_DIVIDE     :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " / ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_MOD        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " % ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_ADD        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " + ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_SUBTRACT   :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " - ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_AND        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " & ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_OR         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " | ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_NAND       :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " ~& ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_NOR        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " ~| ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_NXOR       :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " ~^ ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_LT         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " < ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_GT         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " > ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_LSHIFT     :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " << ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_RSHIFT     :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " >> ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_EQ         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " == ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_CEQ        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " === ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_LE         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " <= ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_GE         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " >= ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_NE         :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " != ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_CNE        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " !== ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_LOR        :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " || ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_LAND       :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " && ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UINV       :  generator_concat_code( lhs, NULL, NULL, 0, "~", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UAND       :  generator_concat_code( lhs, NULL, NULL, 0, "&", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UNOT       :  generator_concat_code( lhs, NULL, NULL, 0, "!", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UOR        :  generator_concat_code( lhs, NULL, NULL, 0, "|", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UXOR       :  generator_concat_code( lhs, NULL, NULL, 0, "^", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UNAND      :  generator_concat_code( lhs, NULL, NULL, 0, "~&", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UNOR       :  generator_concat_code( lhs, NULL, NULL, 0, "~|", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_UNXOR      :  generator_concat_code( lhs, NULL, NULL, 0, "~^", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_ALSHIFT    :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " <<< ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_ARSHIFT    :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " >>> ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_EXPONENT   :  generator_concat_code( lhs, NULL, lstr, lstr_depth, " ** ", rstr, rstr_depth, NULL, net );  break;
    case EXP_OP_NEGATE     :  generator_concat_code( lhs, NULL, NULL, 0, "-", rstr, rstr_depth, NULL, net );  break;
    // case EXP_OP_COND       :
    // case EXP_OP_COND_SEL   :
    case EXP_OP_SIG        :
    case EXP_OP_PARAM      :  generator_concat_code( lhs, exp->sig->name, NULL, 0, NULL, NULL, 0, NULL, net );  break;
    case EXP_OP_SBIT_SEL   :
    case EXP_OP_PARAM_SBIT :
      {
        unsigned int slen = strlen( exp->sig->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->sig->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, lstr_depth, "]", NULL, 0, NULL, net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_SEL   :
    case EXP_OP_PARAM_MBIT :
      {
        unsigned int slen = strlen( exp->sig->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->sig->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, lstr_depth, ":", rstr, rstr_depth, "]", net );
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
  expression* exp,    /*!< Pointer to the current expression */
  func_unit*  funit,  /*!< Pointer to the functional unit that exp exists in */
  int         depth,  /*!< Current subexpression depth */
  bool        net     /*!< If TRUE, specifies that we are generating for a net */
) { PROFILE(GENERATOR_INSERT_SUBEXP);

  char*        lhs_str = NULL;
  char**       left_str;
  unsigned int left_str_depth;
  char**       right_str;
  unsigned int right_str_depth;
  unsigned int rv;
  unsigned int i;

  /* Create LHS portion of assignment */
  generator_create_lhs( exp, funit, net, &lhs_str );

  /* Generate left and right subexpression code string */
  generator_create_subexp( exp->left,  exp->op, funit, depth, net, &left_str,  &left_str_depth );
  generator_create_subexp( exp->right, exp->op, funit, depth, net, &right_str, &right_str_depth );

  /* Create output expression and add it to the working list */
  generator_create_exp( exp, lhs_str, left_str, left_str_depth, right_str, right_str_depth, net );

  /* Deallocate right-hand-side expression string */
  free_safe( lhs_str, (strlen( lhs_str ) + 1) );

  /* Deallocate left string */
  for( i=0; i<left_str_depth; i++ ) {
    free_safe( left_str[i], (strlen( left_str[i] ) + 1) );
  }
  free_safe( left_str, (sizeof( char* ) * left_str_depth) );

  /* Deallocate right string */
  for( i=0; i<right_str_depth; i++ ) {
    free_safe( right_str[i], (strlen( right_str[i] ) + 1) );
  }
  free_safe( right_str, (sizeof( char* ) * right_str_depth) );

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
  bool         root        /*!< Set to TRUE only for the "root" expression in the tree */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER);

  if( exp != NULL ) {

    int child_depth = (depth + ((exp->op != parent_op) ? 1 : 0));

//    printf( "In generator_insert_comb_cov_helper, expr: %s\n", expression_string( exp ) );

    /* Only continue to traverse tree if we are within our depth limit */
    if( (depth < generator_max_exp_depth) && (EXPR_IS_MEASURABLE( exp ) == 1) && !expression_is_static_only( exp ) ) {

      /* Generate event combinational logic type */
      if( EXPR_IS_EVENT( exp ) ) {
        generator_insert_event_comb_cov( exp, funit );

      /* Generate unary combinational logic type */
      } else if( EXPR_IS_UNARY( exp ) ) {
        generator_insert_unary_comb_cov( exp, funit, depth, net );
        generator_insert_subexp( exp, funit, depth, net );

      /* Otherwise, generate binary combinational logic type */
      } else if( EXPR_IS_COMB( exp ) ) {
        generator_insert_comb_comb_cov( exp, funit, depth, net );
        if( (exp->left != NULL) && ((child_depth >= generator_max_exp_depth) || !EXPR_IS_MEASURABLE( exp->left ) || expression_is_static_only( exp->left )) ) {
          generator_insert_subexp( exp->left,  funit, depth, net );
        }
        if( (exp->right != NULL) && ((child_depth >= generator_max_exp_depth) || !EXPR_IS_MEASURABLE( exp->right ) || expression_is_static_only( exp->right )) ) {
          generator_insert_subexp( exp->right, funit, depth, net );
        }

      }

    }

    /* Generate children expression trees */
    generator_insert_comb_cov_helper( exp->left,  funit, exp->op, child_depth, net, FALSE );
    generator_insert_comb_cov_helper( exp->right, funit, exp->op, child_depth, net, FALSE );

  }

  PROFILE_END;

}

/*!
 Insert combinational logic coverage code for the given expression (by file position).
*/
void generator_insert_comb_cov(
  bool         net,          /*!< If set to TRUE, generate code for a net; otherwise, generate code for a variable */
  bool         use_right,    /*!< If set to TRUE, use right-hand expression */
  unsigned int first_line,   /*!< First line of expression to generate for */
  unsigned int first_column  /*!< First column of expression to generate for */
) { PROFILE(GENERATOR_INSERT_COMB_COV);

  statement* stmt;

//  printf( "Calling generator_insert_comb_cov, first_line: %u, first_column: %u\n", first_line, first_column );

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( generator_comb && ((stmt = generator_find_statement( first_line, first_column )) != NULL) ) {

    /* Generate combinational coverage */
    generator_insert_comb_cov_helper( (use_right ? stmt->exp->right : stmt->exp), stmt->funit, (use_right ? stmt->exp->right->op : stmt->exp->op), 0, net, TRUE );

  }

  PROFILE_END;

}


/*
 $Log$
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

