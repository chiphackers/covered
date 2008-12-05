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


struct fname_link_s;
typedef struct fname_link_s fname_link;
struct fname_link_s {
  char*       filename;    /*!< Filename associated with all functional units */
  func_unit*  next_funit;  /*!< Pointer to the next/current functional unit that will be parsed */
  funit_link* head;        /*!< Pointer to head of functional unit list */
  funit_link* tail;        /*!< Pointer to tail of functional unit list */
  fname_link* next;        /*!< Pointer to next filename list */
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
 Maximum expression depth to generate coverage code for (from command-line).
*/
unsigned int generator_max_exp_depth = 1;

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
      generator_flush_all();

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
  } else {
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
void generator_flush_work_code() { PROFILE(GENERATOR_FLUSH_WORK_CODE);

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
void generator_flush_hold_code() { PROFILE(GENERATOR_FLUSH_HOLD_CODE);

  str_link* strl = hold_head;

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
 Shortcut for calling generator_flush_work_code() followed by generator_flush_hold_code().
*/
void generator_flush_all() { PROFILE(GENERATOR_FLUSH_ALL);

  generator_flush_work_code();
  generator_flush_hold_code();

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

  if( (stmt == NULL) || (stmt->exp->line != first_line) || (((stmt->exp->col >> 16) & 0xffff) != first_column) ) {

    /* Attempt to find the expression with the given position */
    while( ((stmt = func_iter_get_next_statement( &fiter )) != NULL) &&
           ((stmt->exp->line != first_line) || (((stmt->exp->col >> 16) & 0xffff) != first_column)) );

  }

  return( stmt );

}

/*!
 Inserts line coverage information in string queues.
*/
void generator_insert_line_cov(
  unsigned int first_line,   /*!< Line to create line coverage for */
  unsigned int first_column  /*!< First column of statement */
) { PROFILE(GENERATOR_INSERT_LINE_COV);

  if( generator_line && (generator_find_statement( first_line, first_column ) != NULL) ) {

    char         str[256];
    unsigned int rv;
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;

    /* Create the register */
    rv = snprintf( str, 256, "reg covered$l%d_%d = 1'b0;", first_line, first_column );
    assert( rv < 256 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );

    /* Prepend the line coverage assignment to the working buffer */
    rv = snprintf( str, 256, " covered$l%d_%d = 1'b1;", first_line, first_column );
    assert( rv < 256 );
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

static void generator_insert_event_comb_cov(
  expression* exp  /*!< Pointer to expression to output */
) { PROFILE(GENERATOR_INSERT_EVENT_COMB_COV);

  PROFILE_END;

}

/*!
 Inserts combinational logic coverage code for a unary expression.
*/
static void generator_insert_unary_comb_cov(
  expression*  exp,    /*!< Pointer to expression to output */
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
  rv = snprintf( sig,  80, "covered$e%d_%x", exp->line, exp->col );
  assert( rv < 80 );
  rv = snprintf( sigr, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->right->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->right->line, exp->right->col );
  assert( rv < 80 );

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
  rv = snprintf( str, 4096, "%s%s = %s%s;", prefix, sig, exp_op_info[exp->op].op_str, sigr );
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
 Inserts AND-style combinational logic coverage code.
*/
static void generator_insert_and_comb_cov(
  expression*  exp,    /*!< Pointer to expression to output */
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
  rv = snprintf( sig,  80, "covered$e%d_%x", exp->line, exp->col );
  assert( rv < 80 );
  rv = snprintf( sigl, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->left->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->left->line, exp->left->col );
  assert( rv < 80 );
  rv = snprintf( sigr, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->right->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->right->line, exp->right->col );
  assert( rv < 80 );

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [2:0] " );
  } else {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [2:0] %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = {~%s[0],~%s[0],(%s[0] %s %s[0])};",
                 prefix, sig, sigl, sigr, sigl, exp_op_info[exp->op].op_str, sigr );
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
 Inserts OR style combinational logic coverage code.
*/
static void generator_insert_or_comb_cov(
  expression*  exp,    /*!< Pointer to expression to output */
  unsigned int depth,  /*!< Current expression depth */
  bool         net     /*!< If set to TRUE generate code for a net */
) { PROFILE(GENERATOR_INSERT_OR_COMB_COV);

  char         prefix[16];
  char         sig[80];
  char         sigl[80];
  char         sigr[80];
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  
  /* Create signal */  
  rv = snprintf( sig,  80, "covered$e%d_%x", exp->line, exp->col );
  assert( rv < 80 );
  rv = snprintf( sigl, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->left->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->left->line, exp->left->col );
  assert( rv < 80 );
  rv = snprintf( sigr, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->right->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->right->line, exp->right->col );
  assert( rv < 80 );
  
  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [2:0] " );
  } else {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [2:0] %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
  }
  
  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = {%s[0],%s[0],(%s[0] %s %s[0])};",
                 prefix, sig, sigl, sigr, sigl, exp_op_info[exp->op].op_str, sigr );
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
 Inserts OTHER style combinational logic coverage code.
*/
static void generator_insert_other_comb_cov(
  expression*  exp,    /*!< Pointer to expression to output */
  unsigned int depth,  /*!< Current expression depth */
  bool         net     /*!< If set to TRUE generate code for a net */
) { PROFILE(GENERATOR_INSERT_OTHER_COMB_COV);

  char         prefix[16];
  char         sig[80];
  char         sigl[80];
  char         sigr[80];
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  
  /* Create signal */  
  rv = snprintf( sig,  80, "covered$e%d_%x", exp->line, exp->col );
  assert( rv < 80 );
  rv = snprintf( sigl, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->left->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->left->line, exp->left->col );
  assert( rv < 80 );
  rv = snprintf( sigr, 80, "covered$%c%d_%x", (((depth + ((exp->op != exp->right->op) ? 1 : 0)) < generator_max_exp_depth) ? 'e' : (net ? 'y' : 'x')),
                 exp->right->line, exp->right->col );
  assert( rv < 80 );
  
  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [3:0] " );
  } else {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [3:0] %s;", sig );
    assert( rv < 4096 );
    str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
  }
  
  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = {(%s[0] & %s[0]),(%s[0] & ~%s[0]),(~%s[0] & %s[0]),(~%s[0] & ~%s[0])};",
                 prefix, sig, sigl, sigr, sigl, sigr, sigl, sigr, sigl, sigr);
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
 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static void generator_insert_comb_cov_helper(
  expression*  exp,        /*!< Pointer to expression tree to operate on */
  func_unit*   funit,      /*!< Pointer to current functional unit */
  exp_op_type  parent_op,  /*!< Parent expression operation (originally should be set to the same operation as exp) */
  int          depth,      /*!< Current expression depth (originally set to 0) */
  bool         net         /*!< If set to TRUE generate code for a net */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER);

  if( exp != NULL ) {

    /* Only continue to traverse tree if we are within our depth limit */
    if( depth < generator_max_exp_depth ) {

      int child_depth = (depth + ((exp->op != parent_op) ? 1 : 0));

      /* Generate event combinational logic type */
      if( exp_op_info[exp->op].suppl.is_event ) {
        generator_insert_event_comb_cov( exp );

      /* Generate unary combinational logic type */
      } else if( exp_op_info[exp->op].suppl.is_unary ) {
        generator_insert_unary_comb_cov( exp, depth, net );

      /* Otherwise, generate binary combinational logic type */
      } else {
        switch( exp_op_info[exp->op].suppl.is_comb ) {
          case AND_COMB   :  generator_insert_and_comb_cov( exp, depth, net );    break;
          case OR_COMB    :  generator_insert_or_comb_cov( exp, depth, net );     break;
          case OTHER_COMB :  generator_insert_other_comb_cov( exp, depth, net );  break;
          default         :  break;
        }

      }

      /* Generate children expression trees */
      generator_insert_comb_cov_helper( exp->left,  funit, exp->op, child_depth, net );
      generator_insert_comb_cov_helper( exp->right, funit, exp->op, child_depth, net );

    } else {

      char         str[4096];
      char         prefix[8];
      char**       code;
      unsigned int code_depth;
      unsigned int i;
      str_link*    tmp_head = NULL;
      str_link*    tmp_tail = NULL;
      unsigned int rv;

      /* Generate code */
      codegen_gen_expr( exp, parent_op, &code, &code_depth, funit ); 

      if( net ) {
        strcpy( prefix, "wire " );
      } else {
        prefix[0] = '\0';
        rv = snprintf( str, 4096, "reg covered$x%d_%x;", exp->line, exp->col );
        assert( rv < 4096 );
        str_link_add( strdup_safe( str ), &reg_head, &reg_tail );
      }

      rv = snprintf( str, 4096, "%scovered$%c%d_%x = |(%s)%c", prefix, (net ? 'y' : 'x'), exp->line, exp->col, code[0], ((code_depth == 1) ? ';' : '\0') );
      assert( rv < 4096 );
      str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      free_safe( code[0], (strlen( code[0] ) + 1) );

      for( i=1; i<code_depth; i++ ) {
        str_link_add( code[i], &tmp_head, &tmp_tail );
      }
      free_safe( code, (sizeof( char* ) * code_depth) );

      /* Prepend code string */
      if( net ) {
        if( reg_head == NULL ) {
          reg_head = reg_tail = tmp_head;
        } else {
          tmp_tail->next = reg_head;
          reg_head       = tmp_head;
        }
      } else {
        if( work_head == NULL ) {
          work_head = work_tail = tmp_head;
        } else {
          tmp_tail->next = work_head;
          work_head      = tmp_head;
        }
      }

    }

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

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( generator_comb && ((stmt = generator_find_statement( first_line, first_column )) != NULL) ) {

    /* Generate combinational coverage */
    generator_insert_comb_cov_helper( (use_right ? stmt->exp->right : stmt->exp), db_get_curr_funit(), stmt->exp->op, 0, net );

  }

  PROFILE_END;

}


/*
 $Log$
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

