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
#include "func_unit.h"
#include "generator.h"
#include "gen_item.h"
#include "link.h"
#include "ovl.h"
#include "param.h"
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
extern int            generate_mode;
extern isuppl         info_suppl;


struct fname_link_s;
typedef struct fname_link_s fname_link;
struct fname_link_s {
  char*       filename;    /*!< Filename associated with all functional units */
  func_unit*  next_funit;  /*!< Pointer to the next/current functional unit that will be parsed */
  funit_link* head;        /*!< Pointer to head of functional unit list */
  funit_link* tail;        /*!< Pointer to tail of functional unit list */
  fname_link* next;        /*!< Pointer to next filename list */
};

struct replace_info_s;
typedef struct replace_info_s replace_info;
struct replace_info_s {
  char*     word_ptr;
  str_link* list_ptr;
};

struct reg_insert_s;
typedef struct reg_insert_s reg_insert;
struct reg_insert_s {
  str_link*   ptr;         /*!< Pointer to string link to insert register information after */
  reg_insert* next;        /*!< Pointer to the next reg_insert structure in the stack */
};


/*!
 Pointer to functional unit stack.
*/
funit_link* funit_top = NULL;

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
 Pointer to the top of the register insertion stack.
*/
reg_insert* reg_top = NULL;

/*!
 Pointer to head of list for combinational coverage lines.
*/
str_link* comb_head = NULL;

/*!
 Pointer to tail of list for combinational coverage lines.
*/
str_link* comb_tail = NULL;

/*!
 Maximum expression depth to generate coverage code for (from command-line).
*/
unsigned int generator_max_exp_depth = 0xffffffff;

/*!
 Specifies that we should handle the current functional unit as an assertion.
*/
bool handle_funit_as_assert = FALSE;

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
 Stores replacement string information for the first character of the potential replacement string.
*/
static replace_info replace_first;

/*!
 Stores replacement string information for the last character of the potential replacement string.
*/
static replace_info replace_last;

/*!
 If replace_ptr is not set to NULL, this value contains the first line of the replacement string.
*/
static unsigned int replace_first_line;

/*!
 If replace_ptr is not set to NULL, this value contains the first column of the replacement string.
*/
static unsigned int replace_first_col;


static char* generator_gen_size(
  expression* exp,
  func_unit*  funit,
  int*        number
);

/*!
 Outputs the current state of the code lists to standard output (for debugging purposes only).
*/
void generator_display() { PROFILE(GENERATOR_DISPLAY);

  str_link* strl;

  printf( "Holding code list (%p %p):\n", hold_head, hold_tail );
  strl = hold_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }
  printf( "Holding buffer:\n  %s\n", hold_buffer );

  printf( "Working code list (%p %p):\n", work_head, work_tail );
  strl = work_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }
  printf( "Working buffer:\n  %s\n", work_buffer );

  PROFILE_END;

}

/*!
 \return Returns allocated string containing the difference in scope between the current functional unit
         and the specified child functional unit.
*/
static char* generator_get_relative_scope(
  func_unit* child  /*!< Pointer to child functional unit */
) { PROFILE(GENERATOR_GET_RELATIVE_SCOPE);

  char* back = strdup_safe( child->name );
  char* relative_scope;

  scope_extract_scope( child->name, funit_top->funit->name, back );
  relative_scope = strdup_safe( back );

  free_safe( back, (strlen( child->name ) + 1) );

  PROFILE_END;

  return( relative_scope );

}

/*!
 Clears the first and last replace information pointers.
*/
void generator_clear_replace_ptrs() { PROFILE(GENERATOR_CLEAR_REPLACE_PTRS);

  /* Clear the first pointers */
  replace_first.word_ptr = NULL;
  replace_first.list_ptr = NULL;

  /* Clear the last pointers */
  replace_last.word_ptr = NULL;
  replace_last.list_ptr = NULL;

  PROFILE_END;

}

/*!
 Replaces a portion (as specified by the line and column inputs) of the currently
 marked code segment (marking occurs automatically) with the given string.
*/
void generator_replace(
  const char*  str,           /*!< String to replace the original */
  unsigned int first_line,    /*!< Line number of start of code to replace */
  unsigned int first_column,  /*!< Column number of start of code to replace */
  unsigned int last_line,     /*!< Line number of end of code to replace */
  unsigned int last_column    /*!< Column number of end of code to replace */
) { PROFILE(GENERATOR_REPLACE);

  /* We can only perform the replacement if something has been previously marked for replacement */
  if( replace_first.word_ptr != NULL ) {

    /* Go to starting line */
    while( first_line > replace_first_line ) {
      replace_first.list_ptr = replace_first.list_ptr->next;
      if( replace_first.list_ptr == NULL ) {
        assert( first_line == (replace_first_line + 1) );
        replace_first.word_ptr = work_buffer;
      } else {
        replace_first.word_ptr = replace_first.list_ptr->str;
      }
      replace_first_col = 0;
      replace_first_line++;
    }

    /* Remove original code */
    if( first_line == last_line ) {

      /* If the line exists in the work buffer, replace the working buffer */
      if( replace_first.list_ptr == NULL ) {

        char keep_end[4096];
        strcpy( keep_end, (replace_first.word_ptr + (last_column - replace_first_col) + 1) );

        /* Chop the working buffer */
        *(replace_first.word_ptr + (first_column - replace_first_col)) = '\0';

        /* Append the new string */
        if( (strlen( work_buffer ) + strlen( str )) < 4095 ) {
          strcat( work_buffer, str );
        } else {
          str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
          strcpy( work_buffer, str );
        }

        /* Now append the end of the working buffer */
        if( (strlen( work_buffer ) + strlen( keep_end )) < 4095 ) {
          replace_first.word_ptr = work_buffer + strlen( work_buffer );
          strcat( work_buffer, keep_end );
        } else {
          str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
          strcpy( work_buffer, keep_end );
          replace_first.word_ptr = work_buffer;
        }

        /* Adjust the first replacement column to match the end of the newly inserted string */
        replace_first_col += (first_column - replace_first_col) + ((last_column - first_column) + 1);

      /* Otherwise, the line exists in the working list, so replace it there */
      } else {

        unsigned int keep_begin_len = (replace_first.word_ptr + (first_column - replace_first_col)) - replace_first.list_ptr->str;
        char*        keep_begin_str = (char*)malloc_safe( keep_begin_len + 1 );
        char*        keep_end_str   = strdup_safe( replace_first.word_ptr + (last_column - replace_first_col) + 1 );

        strncpy( keep_begin_str, replace_first.list_ptr->str, keep_begin_len );
        keep_begin_str[keep_begin_len] = '\0';
        free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
        replace_first.list_ptr->str = (char*)malloc_safe( keep_begin_len + strlen( str ) + strlen( keep_end_str ) + 1 );
        strcpy( replace_first.list_ptr->str, keep_begin_str );
        strcat( replace_first.list_ptr->str, str );
        strcat( replace_first.list_ptr->str, keep_end_str );

        free_safe( keep_begin_str, (strlen( keep_begin_str ) + 1) );
        free_safe( keep_end_str,   (strlen( keep_end_str ) + 1) );

      }
    
    } else {

      unsigned int keep_len       = (replace_first.word_ptr + (first_column - replace_first_col)) - replace_first.list_ptr->str;
      char*        keep_str       = (char*)malloc_safe( keep_len + strlen( str ) + 1 );
      str_link*    first_list_ptr = replace_first.list_ptr; 

      /*
       First, let's concat the kept code on the current line with the new code and replace
       the current line with the new string.
      */
      strncpy( keep_str, replace_first.list_ptr->str, keep_len );
      keep_str[keep_len] = '\0';
      strcat( keep_str, str );
      free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
      replace_first.list_ptr->str = keep_str;

      /* Now remove the rest of the replaced code and adjust the replacement pointers as needed */
      first_list_ptr         = replace_first.list_ptr;
      replace_first.list_ptr = replace_first.list_ptr->next;
      replace_first_line++;
      while( replace_first_line < last_line ) {
        str_link* next = replace_first.list_ptr->next;
        free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
        free_safe( replace_first.list_ptr, sizeof( str_link ) );
        replace_first.list_ptr = next;
        replace_first_line++;
      }
      first_list_ptr->next = replace_first.list_ptr;

      /* Remove the last line portion from the buffer if the last line is there */
      if( replace_first.list_ptr == NULL ) {
        char tmp_buffer[4096];
        strcpy( tmp_buffer, (work_buffer + last_column + 1) );
        strcpy( work_buffer, tmp_buffer );
        replace_first.word_ptr = work_buffer;
        replace_first_col      = last_column + 1;

      /* Otherwise, remove the last line portion from the list */
      } else {
        char* tmp_str = strdup_safe( replace_first.list_ptr->str + last_column + 1 );
        free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
        replace_first.list_ptr->str = tmp_str;
        replace_first.word_ptr      = tmp_str;
        replace_first_col           = last_column + 1;

      }

    }

  }

  PROFILE_END;

}

/*!
 Allocates and initializes a new structure for the purposes of coverage register insertion.
 Called by parser. 
*/
void generator_push_reg_insert() { PROFILE(GENERATOR_PUSH_REG_INSERT);

  reg_insert* ri;

  /* Make sure that the hold buffer is added to the hold list */
  if( hold_buffer[0] != '\0' ) {
    strcat( hold_buffer, "\n" );
    str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
    hold_buffer[0] = '\0';
  }

  /* Allocate and initialize the new register insertion structure */
  ri       = (reg_insert*)malloc_safe( sizeof( reg_insert ) );
  ri->ptr  = hold_tail;
  ri->next = reg_top;

  /* Point the register stack top to the newly created register insert structure */
  reg_top = ri;

  PROFILE_END;

}

/*!
 Pops the head of the register insertion stack.  Called by parser.
*/
void generator_pop_reg_insert() { PROFILE(GENERATOR_POP_REG_INSERT);

  reg_insert* ri;

  /* Save pointer to the top reg_insert structure and adjust reg_top */
  ri      = reg_top;
  reg_top = ri->next;

  /* Deallocate the reg_insert structure */
  free_safe( ri, sizeof( reg_insert ) );

}

/*!
 \return Returns TRUE if the reg_top stack contains exactly one element.
*/
static bool generator_is_reg_top_one_deep() { PROFILE(GENERATOR_IS_BASE_REG_INSERT);

  bool retval = (reg_top != NULL) && (reg_top->next == NULL);

  PROFILE_END;

  return( retval );

}

/*!
 Inserts the given register instantiation string into the appropriate spot in the hold buffer.
*/
static void generator_insert_reg(
  const char* str  /*!< Register instantiation string to insert */
) { PROFILE(GENERATOR_INSERT_REG);

  str_link* tmp_head = NULL;
  str_link* tmp_tail = NULL;

  assert( reg_top != NULL );

  /* Create string link */
  str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );

  /* Insert it at the insertion point */
  if( reg_top->ptr == NULL ) {
    tmp_head->next = hold_head;
    if( hold_head == NULL ) {
      hold_head = hold_tail = tmp_head;
    } else {
      hold_head = tmp_head;
    }
  } else {
    tmp_head->next     = reg_top->ptr->next;
    reg_top->ptr->next = tmp_head;
    if( hold_tail == reg_top->ptr ) {
      hold_tail = tmp_head;
    }
  }
 
  PROFILE_END;

}

/*!
 Pushes the given functional unit to the top of the functional unit stack.
*/
void generator_push_funit(
  func_unit* funit  /*!< Pointer to the functional unit to push onto the stack */
) { PROFILE(GENERATOR_PUSH_FUNIT);

  funit_link* tmp_head = NULL;
  funit_link* tmp_tail = NULL;

  /* Create a functional unit link */
  funit_link_add( funit, &tmp_head, &tmp_tail );

  /* Set the new functional unit link to the top of the stack */
  tmp_head->next = funit_top;
  funit_top      = tmp_head;

  PROFILE_END;

}

/*!
 Pops the top of the functional unit stack and deallocates it.
*/
void generator_pop_funit() { PROFILE(GENERATOR_POP_FUNIT);

  funit_link* tmp = funit_top;

  assert( tmp != NULL );

  funit_top = funit_top->next;

  free_safe( tmp, sizeof( funit_link ) );

  PROFILE_END;

}
  

/*!
 \return Returns TRUE if the specified expression is one that requires a substitution.
*/
bool generator_expr_needs_to_be_substituted(
  expression* exp  /*!< Pointer to expression to evaluate */
) { PROFILE(GENERATOR_EXPR_NEEDS_TO_BE_SUBSTITUTED);

  bool retval = (exp->op == EXP_OP_SRANDOM)      ||
                (exp->op == EXP_OP_SURANDOM)     ||
                (exp->op == EXP_OP_SURAND_RANGE);

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the given expression requires coverage output to be generated.
*/
static bool generator_expr_cov_needed(
  expression*  exp,   /*!< Pointer to expression to evaluate */
  unsigned int depth  /*!< Expression depth of the given expression */
) { PROFILE(GENERATOR_EXPR_COV_NEEDED);

  bool retval = (depth < generator_max_exp_depth) && (EXPR_IS_MEASURABLE( exp ) == 1) && !expression_is_static_only( exp );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the current expression needs to be substituted with an expression name.
*/
bool generator_expr_name_needed(
  expression* exp  /*!< Pointer to expression to evaluate */
) {

  return( exp->suppl.part.comb_cntd == 1 );

}

/*!
 Recursively clears the comb_cntd bits in the given expression tree for all expressions that do not require
 substitution.
*/
void generator_clear_comb_cntd(
  expression* exp  /*!< Pointer to expression to clear */
) { PROFILE(GENERATOR_CLEAR_COMB_CNTD);

  if( exp != NULL ) {

    generator_clear_comb_cntd( exp->left );
    generator_clear_comb_cntd( exp->right );

    /* Only clear the comb_cntd bit if it does not require substitution */
    if( !generator_expr_needs_to_be_substituted( exp ) ) {
      exp->suppl.part.comb_cntd = 0;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns the name of the given expression in a string form that represents the physical attributes
         of the given expression for the purposes of lookup.  Each name is guaranteed to be unique.  The
         returned string is allocated and must be deallocated by the calling function.
*/
char* generator_create_expr_name(
  expression*  exp  /*!< Pointer to expression */
) { PROFILE(GENERATOR_CREATE_EXPR_NAME);

  char*        name;
  unsigned int slen;
  unsigned int rv;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char         op[30];
  char         fline[30];
  char         lline[30];
  char         col[30];

  assert( exp != NULL );

  /* Create string versions of op, first line, last line and column information */
  rv = snprintf( op,    30, "%x", exp->op );           assert( rv < 30 );
  rv = snprintf( fline, 30, "%u", exp->ppline );       assert( rv < 30 );
  rv = snprintf( lline, 30, "%u", last_exp->ppline );  assert( rv < 30 );
  rv = snprintf( col,   30, "%x", exp->col );          assert( rv < 30 );

  /* Allocate and create the unique name */
  slen = 11 + strlen( op ) + 1 + strlen( fline ) + 1 + strlen( lline ) + 1 + strlen( col ) + 2;
  name = (char*)malloc_safe( slen );
  rv   = snprintf( name, slen, " \\covered$X%s_%s_%s_%s ", op, fline, lline, col );
  assert( rv < slen );

  PROFILE_END;

  return( name );

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

    /* Output the name to standard output */
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Generating inlined coverage file \"%s\"", filename );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

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

  /* Reset the replacement string information */
  generator_clear_replace_ptrs();

  /* Calculate if we need to handle this functional unit as an assertion or not */
  handle_funit_as_assert = (info_suppl.part.scored_assert == 1) && ovl_is_assertion_module( funit );

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
  const char*  str,           /*!< String to write */
  unsigned int first_line,    /*!< First line of string from file */
  unsigned int first_column,  /*!< First column of string from file */
  bool         from_code      /*!< Specifies if the string came from the code directly */
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

    long replace_offset = strlen( work_buffer );

    /* I don't believe that a line will ever exceed 4K chars */
    assert( (strlen( work_buffer ) + strlen( str )) < 4095 );
    strcat( work_buffer, str );

    /* If we have hit a newline, add the working buffer to the working list and clear the working buffer */
    if( strcmp( str, "\n" ) == 0 ) {
      str_link* tmp_tail = work_tail;
      str_link* strl     = str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );

      /*
       If the current word pointer is pointing at the buffer, we need to adjust it to point at the associated
       character in the new work list entry.
      */
      if( (replace_first.word_ptr != NULL) && (replace_first.list_ptr == NULL) ) {
        replace_first.word_ptr = strl->str + (replace_first.word_ptr - work_buffer); 
        replace_first.list_ptr = strl;
      }

      /* Set the replace_first or replace_last strucutures if they have not been setup previously */
      if( from_code ) {
        if( replace_first.word_ptr == NULL ) {
          replace_first.word_ptr = strl->str + replace_offset;
          replace_first.list_ptr = strl;
          replace_first_line     = first_line;
          replace_first_col      = first_column;
        }
      } else {
        if( (replace_first.word_ptr != NULL) && (replace_last.word_ptr == NULL) ) {
          if( replace_offset == 0 ) {
            replace_last.list_ptr = tmp_tail;
            replace_last.word_ptr = tmp_tail->str + (strlen( tmp_tail->str ) - 1);
          } else {
            replace_last.list_ptr = strl;
            replace_last.word_ptr = strl->str + (replace_offset - 1);
          }
        }
      }
      work_buffer[0] = '\0';
    } else {
      if( from_code ) {
        if( replace_first.word_ptr == NULL ) {
          replace_first.word_ptr = work_buffer + replace_offset;
          replace_first_line     = first_line;
          replace_first_col      = first_column;
        }
      } else {
        if( (replace_first.word_ptr != NULL) && (replace_last.word_ptr == NULL) ) {
          replace_last.word_ptr = work_buffer + (replace_offset - 1);
        }
      }
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
//    generator_display();
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

  /* Clear replacement pointers */
  generator_clear_replace_ptrs();

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

  /* We shouldn't ever by flushing the hold code if the reg_top is more than one entry deep */
  assert( (reg_top == NULL) || (reg_top != NULL) && (reg_top->next == NULL) );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing hold code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
//    generator_display();
  }
#endif

  fprintf( curr_ofile, "\n" );

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

  /* Reset the register insertion pointer (no need to push/pop) */
  if( reg_top != NULL ) {
    reg_top->ptr = NULL;
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
statement* generator_find_statement(
  unsigned int first_line,   /*!< First line of statement to find */
  unsigned int first_column  /*!< First column of statement to find */
) { PROFILE(GENERATOR_FIND_STATEMENT);

//  printf( "In generator_find_statement, line: %d, column: %d\n", first_line, first_column );

  if( (curr_stmt == NULL) || (curr_stmt->exp->ppline < first_line) ||
      ((curr_stmt->exp->ppline == first_line) && (((curr_stmt->exp->col >> 16) & 0xffff) < first_column)) ) {

//    func_iter_display( &fiter );

    /* Attempt to find the expression with the given position */
    while( ((curr_stmt = func_iter_get_next_statement( &fiter )) != NULL) &&
//           printf( "  statement %s %d %u\n", expression_string( curr_stmt->exp ), ((curr_stmt->exp->col >> 16) & 0xffff), curr_stmt->exp->ppline ) &&
           ((curr_stmt->exp->ppline < first_line) || 
            ((curr_stmt->exp->ppline == first_line) && (((curr_stmt->exp->col >> 16) & 0xffff) < first_column)) ||
            (curr_stmt->exp->op == EXP_OP_FORK)) );

    /* If we couldn't find it in the func_iter, look for it in the generate list */
    if( curr_stmt == NULL ) {
      statement* gen_stmt = generate_find_stmt_by_position( curr_funit, first_line, first_column );
      if( gen_stmt != NULL ) {
        curr_stmt = gen_stmt;
      }
    }

  }

//  if( (curr_stmt != NULL) && (curr_stmt->exp->ppline == first_line) && (((curr_stmt->exp->col >> 16) & 0xffff) == first_column) && (curr_stmt->exp->op != EXP_OP_FORK) ) {
//    printf( "  FOUND (%s %x)!\n", expression_string( curr_stmt->exp ), ((curr_stmt->exp->col >> 16) & 0xffff) );
//  } else {
//    printf( "  NOT FOUND!\n" );
//  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->ppline != first_line) ||
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

  if( (curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->ppline < first_line) ||
      ((curr_stmt->exp->left->ppline == first_line) && (((curr_stmt->exp->left->col >> 16) & 0xffff) < first_column)) ) {

    if( generate_mode > 0 ) {

      generate_find_stmt_by_position( curr_funit, first_line, first_column );

    } else {

      /* Attempt to find the expression with the given position */
      while( ((curr_stmt = func_iter_get_next_statement( &fiter )) != NULL) && 
             ((curr_stmt->exp->left == NULL) ||
              (curr_stmt->exp->left->ppline < first_line) ||
              ((curr_stmt->exp->left->ppline == first_line) && (((curr_stmt->exp->left->col >> 16) & 0xffff) < first_column))) );

    }

  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->ppline != first_line) ||
          (((curr_stmt->exp->left->col >> 16) & 0xffff) != first_column)) ? NULL : curr_stmt );

}

/*!
 Inserts line coverage information for the given statement.
*/
void generator_insert_line_cov_with_stmt(
  statement* stmt,      /*!< Pointer to statement to generate line coverage for */
  bool       semicolon  /*!< Specifies if a semicolon (TRUE) or a comma (FALSE) should be appended to the created line */
) { PROFILE(GENERATOR_INSERT_LINE_COV_WITH_STMT);

  if( (stmt != NULL) && ((info_suppl.part.scored_line && !handle_funit_as_assert) || (handle_funit_as_assert && ovl_is_coverage_point( stmt->exp ))) ) {

    expression*  last_exp = expression_get_last_line_expr( stmt->exp );
    char         str[4096];
    char         sig[4096];
    unsigned int rv;
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;
    char*        scope    = generator_get_relative_scope( stmt->funit ); 

    if( scope[0] == '\0' ) {
      rv = snprintf( sig, 4096, " \\covered$L%d_%d_%x ", stmt->exp->ppline, last_exp->ppline, stmt->exp->col );
    } else {
      rv = snprintf( sig, 4096, " \\covered$L%d_%d_%x$%s ", stmt->exp->ppline, last_exp->ppline, stmt->exp->col, scope );
    }
    assert( rv < 4096 );
    free_safe( scope, (strlen( scope ) + 1) );

    /* Create the register */
    rv = snprintf( str, 4096, "reg %s;\n", sig );
    assert( rv < 4096 );
    generator_insert_reg( str );

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

  if( ((stmt = generator_find_statement( first_line, first_column )) != NULL) &&
      ((info_suppl.part.scored_line && !handle_funit_as_assert) || (handle_funit_as_assert && ovl_is_coverage_point( stmt->exp ))) ) {

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
  char         str[4096];
  unsigned int rv;
  expression*  last_exp = expression_get_last_line_expr( exp );
  expression*  root_exp = exp;
  char*        scope    = generator_get_relative_scope( funit );

  /* Find the root event of this expression tree */
  while( (ESUPPL_IS_ROOT( root_exp->suppl ) == 0) && (EXPR_IS_EVENT( root_exp->parent->expr ) == 1) ) {
    root_exp = root_exp->parent->expr;
  }

  /* Create signal name */
  if( scope[0] == '\0' ) {
    rv = snprintf( name, 4096, " \\covered$E%d_%d_%x ", exp->ppline, last_exp->ppline, exp->col );
  } else {
    rv = snprintf( name, 4096, " \\covered$E%d_%d_%x$%s ", exp->ppline, last_exp->ppline, exp->col, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create register */
  if( reg_needed ) {
    rv = snprintf( str, 4096, "reg %s;\n", name );
    assert( rv < 4096 );
    generator_insert_reg( str );
  }

  /*
   If the expression is also the root of its expression tree, it is the only event in the statement; therefore,
   the coverage string should just set the coverage variable to a value of 1 to indicate that the event was hit.
  */
  if( exp == root_exp ) {

    /* Create assignment and append it to the working code list */
    rv = snprintf( str, 4096, "%s = 1'b1;", name );
    assert( rv < 4096 );
    generator_add_cov_to_work_code( str );

  /*
   Otherwise, we need to save off the state of the temporary event variable and compare it after the event statement
   has triggered to see which events where hit in the event statement.
  */
  } else {

    char* tname     = generator_create_expr_name( exp );
    char* event_str = codegen_gen_expr_one_line( exp->right, funit, FALSE );

    /* Handle the event */
    switch( exp->op ) {
      case EXP_OP_PEDGE :
        {
          if( reg_needed ) {
            rv = snprintf( str, 4096, "reg %s;\n", tname );
            assert( rv < 4096 );
            generator_insert_reg( str );
          }
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          generator_prepend_to_work_code( str );
          rv = snprintf( str, 4096, " %s = (%s!==1'b1) & (%s===1'b1);", name, tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
        }
        break;

      case EXP_OP_NEDGE :
        {
          if( reg_needed ) {
            rv = snprintf( str, 4096, "reg %s;\n", tname );
            assert( rv < 4096 );
            generator_insert_reg( str );
          }
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          generator_prepend_to_work_code( str );
          rv = snprintf( str, 4096, " %s = (%s!==1'b0) & (%s===1'b0);", name, tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
        }
        break;

      case EXP_OP_AEDGE :
        {
          if( reg_needed ) {
            int   number;
            char* size = generator_gen_size( exp->right, funit, &number );
            if( number >= 0 ) {
              rv = snprintf( str, 4096, "reg [%d:0] %s;\n", (number - 1), tname );
            } else {
              rv = snprintf( str, 4096, "reg [(%s-1):0] %s;\n", size, tname );
            }
            assert( rv < 4096 );
            generator_insert_reg( str );
            free_safe( size, (strlen( size ) + 1) );
          }
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          generator_prepend_to_work_code( str );
          rv = snprintf( str, 4096, " %s = (%s!==%s);", name, tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
        }
        break;

      default :
        break;
    }

    /* Deallocate memory */
    free_safe( event_str, (strlen( event_str ) + 1) );
    free_safe( tname,     (strlen( tname )     + 1) );

  }

  PROFILE_END;

}

/*!
 Inserts combinational logic coverage code for a unary expression.
*/
static void generator_insert_unary_comb_cov(
  expression*  exp,        /*!< Pointer to expression to output */
  func_unit*   funit,      /*!< Pointer to functional unit containing this expression */
  bool         net,        /*!< Set to TRUE if this expression is a net */
  bool         reg_needed  /*!< If TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_UNARY_COMB_COV);

  char         prefix[16];
  char         sig[4096];
  char*        sigr;
  char         str[4096];
  unsigned int rv;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char*        scope    = generator_get_relative_scope( funit );

  /* Create signal */
  if( scope[0] == '\0' ) {
    rv = snprintf( sig,  4096, " \\covered$%c%d_%d_%x ", (net ? 'u' : 'U'), exp->ppline, last_exp->ppline, exp->col );
  } else {
    rv = snprintf( sig,  4096, " \\covered$U%d_%d_%x$%s ", exp->ppline, last_exp->ppline, exp->col, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create right signal name */
  sigr = generator_create_expr_name( exp );

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire " );
  } else {
    prefix[0] = '\0';
    if( reg_needed ) {
      rv = snprintf( str, 4096, "reg %s;\n", sig );
      assert( rv < 4096 );
      generator_insert_reg( str );
    }
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = (%s > 0);", prefix, sig, sigr );
  assert( rv < 4096 );
  str_link_add( strdup_safe( str ), &comb_head, &comb_tail );

  /* Deallocate temporary memory */
  free_safe( sigr, (strlen( sigr ) + 1) );

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
  char         sig[4096];
  char*        sigl;
  char*        sigr;
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char*        scope    = generator_get_relative_scope( funit );

  /* Create signal */
  if( scope[0] == '\0' ) {
    rv = snprintf( sig, 4096, " \\covered$%c%d_%d_%x ", (net ? 'c' : 'C'), exp->ppline, last_exp->ppline, exp->col );
  } else {
    rv = snprintf( sig, 4096, " \\covered$C%d_%d_%x$%s ", exp->ppline, last_exp->ppline, exp->col, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create left and right expression names */
  sigl = generator_create_expr_name( exp->left );
  sigr = generator_create_expr_name( exp->right );

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [1:0] " );
  } else if( reg_needed ) {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [1:0] %s;\n", sig );
    assert( rv < 4096 );
    generator_insert_reg( str );
  }

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s%s = {(%s > 0),(%s > 0)};", prefix, sig, sigl, sigr );
  assert( rv < 4096 );
  str_link_add( strdup_safe( str ), &comb_head, &comb_tail );

  /* Deallocate memory */
  free_safe( sigl, (strlen( sigl ) + 1) );
  free_safe( sigr, (strlen( sigr ) + 1) );

  PROFILE_END;

}

/*!
 Generates MSB string to use for sizing subexpression values.
*/
static char* generator_gen_size(
  expression* exp,    /*!< Pointer to subexpression to generate MSB value for */
  func_unit*  funit,  /*!< Pointer to functional unit containing this subexpression */
  int*        number  /*!< Pointer to value that is set to the number of the returned string represents numbers only */
) { PROFILE(GENERATOR_GEN_SIZE);

  char* size = NULL;

  if( exp != NULL ) {

    char*        lexp = NULL;
    char*        rexp = NULL;
    unsigned int rv;
    int          lnumber;
    int          rnumber;

    switch( exp->op ) {
      case EXP_OP_STATIC :
        {
          char tmp[50];
          rv = snprintf( tmp, 50, "%d", exp->value->width );
          assert( rv < 50 );
          size    = strdup_safe( tmp );
          *number = exp->value->width;
        }
        break;
      case EXP_OP_LIST     :
      case EXP_OP_MULTIPLY :
        lexp = generator_gen_size( exp->left,  funit, &lnumber );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        {
          unsigned int slen = strlen( lexp ) + strlen( rexp ) + 4;
          size = (char*)malloc_safe( slen );
          rv   = snprintf( size, slen, "(%s+%s)", lexp, rexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        *number = ((lnumber >= 0) && (rnumber >= 0)) ? (lnumber + rnumber) : -1;
        break;
      case EXP_OP_CONCAT         :
      case EXP_OP_NEGATE         :
        size = generator_gen_size( exp->right, funit, number );
        break;
      case EXP_OP_MBIT_POS       :
      case EXP_OP_MBIT_NEG       :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        size    = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        *number = -1;
        break;
      case EXP_OP_LSHIFT  :
      case EXP_OP_RSHIFT  :
      case EXP_OP_ALSHIFT :
      case EXP_OP_ARSHIFT :
        size = generator_gen_size( exp->left, funit, number );
        break;
      case EXP_OP_EXPAND :
        lexp = codegen_gen_expr_one_line( exp->left, funit, FALSE );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        {
          unsigned int slen = strlen( lexp ) + strlen( rexp ) + 4;
          size = (char*)malloc_safe( slen );
          rv  = snprintf( size, slen, "(%s*%s)", lexp, rexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        *number = -1;
        break;
      case EXP_OP_STIME :
      case EXP_OP_SR2B  :
      case EXP_OP_SR2I  :
        size    = strdup_safe( "64" );
        *number = 64;
        break;
      case EXP_OP_SSR2B        :
      case EXP_OP_SRANDOM      :
      case EXP_OP_SURANDOM     :
      case EXP_OP_SURAND_RANGE :
        size    = strdup_safe( "32" );
        *number = 32;
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
      case EXP_OP_PARAM_SBIT :
        size    = strdup_safe( "1" );
        *number = 1;
        break;
      case EXP_OP_SBIT_SEL  :
        {
          unsigned int dimension = expression_get_curr_dimension( exp );
          if( (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM) && ((dimension + 1) < (exp->sig->udim_num + exp->sig->pdim_num)) ) {
            size = mod_parm_gen_size_code( exp->sig, (dimension + 1), funit_get_curr_module( funit ), number );
          } else {
            size    = strdup_safe( "1" );
            *number = 1;
          }
        }
        break;
      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
        lexp = codegen_gen_expr_one_line( exp->left,  funit, FALSE );
        rexp = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        {
          unsigned int slen = (strlen( lexp ) * 3) + (strlen( rexp ) * 3) + 18;
          size = (char*)malloc_safe( slen );
          rv  = snprintf( size, slen, "(((%s>%s)?(%s-%s):(%s-%s))+1)", lexp, rexp, lexp, rexp, rexp, lexp );
          assert( rv < slen );
        }
        free_safe( lexp, (strlen( lexp ) + 1) );
        free_safe( rexp, (strlen( rexp ) + 1) );
        *number = -1;
        break;
      case EXP_OP_SIG       :
      case EXP_OP_PARAM     :
      case EXP_OP_FUNC_CALL :
        if( (exp->sig->suppl.part.type == SSUPPL_TYPE_GENVAR) || (exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_SREAL) ) {
          size    = strdup_safe( "32" );
          *number = 32;
        } else if( exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_REAL ) {
          size    = strdup_safe( "64" );
          *number = 64;
        } else {
          size = mod_parm_gen_size_code( exp->sig, expression_get_curr_dimension( exp ), funit_get_curr_module( funit ), number );
        }
        break;
      default :
        lexp = generator_gen_size( exp->left,  funit, &lnumber );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        if( lexp != NULL ) {
          if( rexp != NULL ) {
            if( (lnumber >= 0) && (rnumber >= 0) ) {
              char num[50];
              *number = ((lnumber > rnumber) ? lnumber : rnumber);
              rv = snprintf( num, 50, "%d", *number );
              assert( rv < 50 );
              size = strdup_safe( num );
            } else {
              unsigned int slen = (strlen( lexp ) * 2) + (strlen( rexp ) * 2) + 8;
              size = (char*)malloc_safe_nolimit( slen );
              rv   = snprintf( size, slen, "((%s>%s)?%s:%s)", lexp, rexp, lexp, rexp );
              assert( rv < slen );
              *number = -1;
            }
            free_safe( lexp, (strlen( lexp ) + 1) );
            free_safe( rexp, (strlen( rexp ) + 1) );
          } else {
            size    = lexp;
            *number = lnumber;
          }
        } else {
          if( rexp != NULL ) {
            size    = rexp;
            *number = rnumber;
          } else {
            size    = NULL;
            *number = 1;
          }
        }
        break;
    }

  } 

  PROFILE_END;

  return( size );

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
  char*        name = generator_create_expr_name( exp );
  char*        size;
  char*        code;
  int          number;

  /* Generate MSB string */
  size = generator_gen_size( exp, funit, &number );

  if( net ) {

    unsigned int slen;

    /* Create sized wire string */
    if( number >= 0 ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      slen = 6 + strlen( tmp ) + 4 + strlen( name ) + 1;
      code = (char*)malloc_safe( slen );
      rv   = snprintf( code, slen, "wire [%s:0] %s", tmp, name );
    } else {
      slen = 7 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 1;
      code = (char*)malloc_safe_nolimit( slen );
      rv   = snprintf( code, slen, "wire [(%s-1):0] %s", ((size != NULL) ? size : "1"), name );
    }

    assert( rv < slen );

  } else {

    /* Create sized register string */
    if( reg_needed ) {

      unsigned int slen;
      char*        str;
      
      if( number >= 0 ) {
        char tmp[50];
        rv = snprintf( tmp, 50, "%d", (number - 1) );
        assert( rv < 50 );
        slen = 5 + strlen( tmp ) + 4 + strlen( name ) + 3;
        str  = (char*)malloc_safe( slen );
        rv   = snprintf( str, slen, "reg [%s:0] %s;\n", tmp, name );
      } else {
        slen = 6 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 3;
        str  = (char*)malloc_safe_nolimit( slen );
        rv   = snprintf( str, slen, "reg [(%s-1):0] %s;\n", ((size != NULL) ? size : "1"), name );
      }

      assert( rv < slen );
      generator_insert_reg( str );
      free_safe( str, (strlen( str ) + 1) );

    }

    /* Set the name to the value of code */
    code = strdup_safe( name );

  }

  /* Deallocate memory */
  free_safe( name, (strlen( name ) + 1) );
  free_safe( size, (strlen( size ) + 1) );

  PROFILE_END;

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
      
  /* Prepend to the working list */
  if( work_head == NULL ) {
    work_head = work_tail = tmp_head;
  } else {
    tmp_tail->next = work_head;
    work_head      = tmp_head;
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
    case EXP_OP_DIM        :  generator_concat_code( lhs, NULL, lstr, NULL, rstr, NULL, net );  break;
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
  bool        net,        /*!< If TRUE, specifies that we are generating for a net */
  bool        reg_needed  /*!< If TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_SUBEXP);

  char*        lhs_str  = NULL;
  char*        val_str;
  char*        str;
  unsigned int slen;
  unsigned int rv;
  unsigned int i;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;

  /* Create LHS portion of assignment */
  lhs_str = generator_create_lhs( exp, funit, net, reg_needed );

  /* Generate value string */
  val_str = codegen_gen_expr_one_line( exp, funit, !generator_expr_needs_to_be_substituted( exp ) );

  /* If this expression needs to be substituted, do it with the lhs_str value */
  if( generator_expr_needs_to_be_substituted( exp ) ) {
    expression* last_exp = expression_get_last_line_expr( exp );
    generator_replace( lhs_str, exp->ppline, ((exp->col >> 16) & 0xffff), last_exp->ppline, (last_exp->col & 0xffff) );
  }

  /* Create expression string */
  slen = strlen( lhs_str ) + 3 + strlen( val_str ) + 2;
  str  = (char*)malloc_safe_nolimit( slen );
  rv   = snprintf( str, slen, "%s = %s;", lhs_str, val_str );
  assert( rv < slen );

  /* Prepend the string to the register/working code */
  str_link_add( str, &comb_head, &comb_tail );

  /* Deallocate memory */
  free_safe( lhs_str, (strlen( lhs_str ) + 1) );
  free_safe( val_str, (strlen( val_str ) + 1) );

  /* Specify that this expression has an intermediate value assigned to it */
  exp->suppl.part.comb_cntd = 1;

  PROFILE_END;

}

/*!
 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static void generator_insert_comb_cov_helper2(
  expression*  exp,        /*!< Pointer to expression tree to operate on */
  func_unit*   funit,      /*!< Pointer to current functional unit */
  exp_op_type  parent_op,  /*!< Parent expression operation (originally should be set to the same operation as exp) */
  int          depth,      /*!< Current expression depth (originally set to 0) */
  bool         net,        /*!< If set to TRUE generate code for a net */
  bool         root,       /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed  /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER2);

  if( exp != NULL ) {

    int child_depth = (depth + ((exp->op != parent_op) ? 1 : 0));

    /* Generate children expression trees (depth first search) */
    generator_insert_comb_cov_helper2( exp->left,  funit, exp->op, child_depth, net, FALSE, reg_needed );
    generator_insert_comb_cov_helper2( exp->right, funit, exp->op, child_depth, net, FALSE, reg_needed );

    /* Generate event combinational logic type */
    if( EXPR_IS_EVENT( exp ) ) {
      if( generator_expr_cov_needed( exp, depth ) ) {
        generator_insert_event_comb_cov( exp, funit, reg_needed );
      }
      if( generator_expr_needs_to_be_substituted( exp ) ) {
        generator_insert_subexp( exp, funit, net, reg_needed );
      }

    /* Otherwise, generate binary combinational logic type */
    } else if( EXPR_IS_COMB( exp ) ) {
      if( (exp->left != NULL) && (!generator_expr_cov_needed( exp->left, child_depth ) || generator_expr_needs_to_be_substituted( exp->left )) ) {
        generator_insert_subexp( exp->left,  funit, net, reg_needed );
      }
      if( (exp->right != NULL) && (!generator_expr_cov_needed( exp->right, child_depth ) || generator_expr_needs_to_be_substituted( exp->right )) ) {
        generator_insert_subexp( exp->right, funit, net, reg_needed );
      }
      if( !root && (generator_expr_cov_needed( exp, depth ) || generator_expr_needs_to_be_substituted( exp )) ) {
        generator_insert_subexp( exp, funit, net, reg_needed );
      }
      if( generator_expr_cov_needed( exp, depth ) ) {
        generator_insert_comb_comb_cov( exp, funit, net, reg_needed );
      }

    /* Generate unary combinational logic type */
    } else {
      if( generator_expr_cov_needed( exp, depth ) || generator_expr_needs_to_be_substituted( exp ) ) {
        generator_insert_subexp( exp, funit, net, reg_needed );
      }
      if( generator_expr_cov_needed( exp, depth ) ) {
        generator_insert_unary_comb_cov( exp, funit, net, reg_needed );
      }

    }

#ifdef SKIP
    /* Generate children expression trees (depth first search) */
    generator_insert_comb_cov_helper2( exp->left,  funit, exp->op, child_depth, net, FALSE, reg_needed );
    generator_insert_comb_cov_helper2( exp->right, funit, exp->op, child_depth, net, FALSE, reg_needed );
#endif

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
  bool         net,        /*!< If set to TRUE generate code for a net */
  bool         root,       /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed  /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER);

  /* Generate the code */
  generator_insert_comb_cov_helper2( exp, funit, parent_op, depth, net, root, reg_needed );

  /* Output the generated code */
  if( comb_head != NULL ) {

    /* Prepend to the working list */
    if( work_head == NULL ) {
      work_head = comb_head;
      work_tail = comb_tail;
    } else {
      comb_tail->next = work_head;
      work_head       = comb_head;
    }

    /* Clear the comb head/tail pointers for reuse */
    comb_head = comb_tail = NULL;

  }

  /* Clear the comb_cntd bits in the expression tree */
  generator_clear_comb_cntd( exp );

}

/*!
 Generates a memory index value for a given memory expression.
*/
static char* generator_gen_mem_index(
  expression* exp,       /*!< Pointer to expression accessign memory signal */
  func_unit*  funit,     /*!< Pointer to functional unit containing exp */
  int         dimension  /*!< Current memory dimension (should be initially set to expression_get_curr_dimension( exp ) */
) { PROFILE(GENERATOR_GEN_MEM_INDEX);

  char*        index;
  char*        str;
  char*        num;
  unsigned int slen;
  unsigned int rv;
  int          number;

  if( dimension != 0 ) {

    char* tmpstr = str;
    char* rest   = generator_gen_mem_index( ((dimension == 1) ? exp->parent->expr->left : exp->parent->expr->left->right), funit, (dimension - 1) );

    slen = strlen( tmpstr ) + 1 + strlen( rest ) + 1;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "%s+%s", tmpstr, rest );
    assert( rv < slen ); 

    free_safe( rest,   (strlen( rest )   + 1) );
    free_safe( tmpstr, (strlen( tmpstr ) + 1) );

  }

  /* Calculate the index value */
  switch( exp->op ) {
    case EXP_OP_SBIT_SEL :
      index = codegen_gen_expr_one_line( exp->left, funit, FALSE );
      break;
    case EXP_OP_MBIT_SEL :
      {
        char* lstr = codegen_gen_expr_one_line( exp->left,  funit, FALSE );
        char* rstr = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        slen  = (strlen( lstr ) * 3) + (strlen( rstr ) * 3) + 14;
        index = (char*)malloc_safe( slen );
        rv    = snprintf( index, slen, "((%s>%s)?(%s-%s):(%s-%s))", lstr, rstr, lstr, rstr, rstr, lstr );
        assert( rv < slen );
        free_safe( lstr, (strlen( lstr ) + 1) );
        free_safe( rstr, (strlen( rstr ) + 1) );
      }
      break;
    case EXP_OP_MBIT_POS :
      index = codegen_gen_expr_one_line( exp->left, funit, FALSE );
      break;
    case EXP_OP_MBIT_NEG :
      {
        char* lstr = codegen_gen_expr_one_line( exp->left,  funit, FALSE );
        char* rstr = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        slen  = 2 + strlen( lstr ) + 1 + strlen( rstr ) + 5;
        index = (char*)malloc_safe( slen );
        rv    = snprintf( index, slen, "((%s-%s)+1)", lstr, rstr );
        assert( rv < slen );
        free_safe( lstr, (strlen( lstr ) + 1) );
        free_safe( rstr, (strlen( rstr ) + 1) );
      }
      break;
    default :
      assert( 0 );
      break;
  }

  /* Get the dimensional width for the current expression */
  num = mod_parm_gen_size_code( exp->sig, dimension, funit_get_curr_module( funit ), &number );

  /* If the current dimension is big endian, recalculate the index value */
  if( exp->elem.dim->dim_be ) {
    char* tmp_index = index;
    if( number >= 0 ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      slen  = 1 + strlen( tmp ) + 1 + strlen( tmp_index ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s-%s)", tmp, tmp_index );
    } else {
      slen  = 2 + strlen( num ) + 4 + strlen( index ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "((%s-1)-%s)", num, tmp_index );
    }
    assert( rv < slen );
    free_safe( tmp_index, (strlen( tmp_index ) + 1) );
  }

  /* Deallocate memory */
  free_safe( num, (strlen( num ) + 1) );
  num = NULL;

  /* Get the next dimensional width for the current expression */
  if( (dimension + 1) >= (exp->sig->udim_num + exp->sig->pdim_num) ) {
    number = 1;
  } else {
    num = mod_parm_gen_size_code( exp->sig, (dimension + 1), funit_get_curr_module( funit ), &number );
  }

  if( number >= 0 ) {
    char tmp[50];
    rv = snprintf( tmp, 50, "%d", number );
    assert( rv < 50 );
    slen = 1 + strlen( index ) + 1 + strlen( tmp ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "(%s*%s)", index, tmp );
  } else {
    slen = 1 + strlen( index ) + 1 + strlen( num ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "(%s*%s)", index, num );
  }
  assert( rv < slen );

  /* Deallocate memory */
  free_safe( num, (strlen( num ) + 1) );

  if( dimension != 0 ) {

    char* tmpstr = str;
    char* rest   = generator_gen_mem_index( ((dimension == 1) ? exp->parent->expr->left : exp->parent->expr->left->right), funit, (dimension - 1) );

    slen = strlen( tmpstr ) + 1 + strlen( rest ) + 1;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "%s+%s", tmpstr, rest );
    assert( rv < slen ); 

    free_safe( rest,   (strlen( rest )   + 1) );
    free_safe( tmpstr, (strlen( tmpstr ) + 1) );

  }

  /* Deallocate memory */
  free_safe( index, (strlen( index ) + 1) );

  PROFILE_END;

  return( str );

}

/*!
 \return Returns the string form of the overall size of the given memory.
*/
static char* generator_gen_mem_size(
  vsignal*   sig,  /*!< Pointer to signal to generate memory size information for */
  func_unit* mod   /*!< Pointer to module containing the given signal parameter sizers (module that signal exists in) */
) { PROFILE(GENERATOR_GEN_MEM_SIZE);

  unsigned int i;
  char*        curr_size;
  char*        size = NULL;
  unsigned int slen = 0;
  unsigned int rv;

  for( i=0; i<(sig->udim_num + sig->pdim_num); i++ ) {

    char* tmpsize = size;
    int   number;

    curr_size = mod_parm_gen_size_code( sig, i, mod, &number );

    if( number >= 0 )  {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", number );
      assert( rv < 50 );
      slen += strlen( tmp ) + 2;
      size  = (char*)malloc_safe( slen );
      rv    = snprintf( size, slen, "%s*%s", tmpsize, tmp );
    } else {
      slen += strlen( curr_size ) + 2;
      size  = (char*)malloc_safe( slen );
      rv    = snprintf( size, slen, "%s*%s", tmpsize, curr_size );
    }
    assert( rv < slen );

    free_safe( curr_size, (strlen( curr_size ) + 1) );
    free_safe( tmpsize,   (strlen( tmpsize ) + 1) );

  }

  PROFILE_END;

  return( size );

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
  char*        idxstr   = generator_gen_mem_index( exp, funit, expression_get_curr_dimension( exp ) );
  char*        value;
  char*        str;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char         num[50];
  char*        scope    = generator_get_relative_scope( funit );

  /* Figure out the size to store the number of dimensions */
  strcpy( num, "32" );

  /* Now insert the code to store the index and memory */
  if( write ) {

    char*        size;
    char*        memstr;
    unsigned int vlen;
    char         iname[4096];
    str_link*    tmp_head  = NULL;
    str_link*    tmp_tail  = NULL;
    int          number;
    expression*  first_exp = expression_get_first_select( exp );

    /* First, create the wire/register to hold the index */
    if( scope[0] == '\0' ) {
      rv = snprintf( iname, 4096, " \\covered$%c%d_%d_%x$%s ", (net ? 'i' : 'I'), exp->ppline, last_exp->ppline, exp->col, exp->name );
    } else {
      rv = snprintf( iname, 4096, " \\covered$%c%d_%d_%x$%s$%s ", (net ? 'i' : 'I'), exp->ppline, last_exp->ppline, exp->col, exp->name, scope );
    }
    assert( rv < 4096 );

    if( net ) {

      unsigned int slen = 7 + strlen( num ) + 7 + strlen( name ) + 3 + strlen( idxstr ) + 2;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "wire [(%s-1):0] %s = %s;", num, iname, idxstr );
      assert( rv < slen );

    } else {

      unsigned int slen = 6 + strlen( num ) + 7 + strlen( iname ) + 3;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "reg [(%s-1):0] %s;\n", num, iname );
      assert( rv < slen );
      generator_insert_reg( str );
      free_safe( str, (strlen( str ) + 1) );

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
    if( scope[0] == '\0' ) {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s ", (net ? 'w' : 'W'), exp->ppline, last_exp->ppline, exp->col, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s$%s ", (net ? 'w' : 'W'), exp->ppline, last_exp->ppline, exp->col, exp->name, scope );
    }
    assert( rv < 4096 );

    /* Generate size */
    size = generator_gen_size( exp, funit, &number );

    /* Create the range information for the write */
    if( number >= 0 ) {
      rv = snprintf( range, 4096, "[(%d+(%s-1)):0]", number, num );
    } else {
      rv = snprintf( range, 4096, "[(%s+(%s-1)):0]", size, num );
    }
    assert( rv < 4096 );

    /* Create the value to assign */
    memstr = codegen_gen_expr_one_line( first_exp, funit, FALSE );
    vlen   = 1 + strlen( memstr ) + 1 + strlen( iname ) + 2;
    value  = (char*)malloc_safe( vlen );
    rv = snprintf( value, vlen, "{%s,%s}", memstr, iname );
    assert( rv < vlen );

    /* Deallocate temporary strings */
    free_safe( size, (strlen( size ) + 1) );
    free_safe( memstr, (strlen( memstr ) + 1) );

  } else {

    /* Create name */
    if( scope[0] == '\0' ) {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s ", (net ? 'r' : 'R'), exp->ppline, last_exp->ppline, exp->col, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%d_%d_%x$%s$%s ", (net ? 'r' : 'R'), exp->ppline, last_exp->ppline, exp->col, exp->name, scope );
    }
    assert( rv < 4096 );

    /* Create the range information for the read */
    rv = snprintf( range, 4096, "[(%s-1):0]", num );
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

    unsigned int slen = 3 + 1 + strlen( range ) + 1 + strlen( name ) + 3;

    str = (char*)malloc_safe( slen );
    rv  = snprintf( str, slen, "reg %s %s;\n", range, name );
    assert( rv < slen );

    /* Add the register to the register list */
    generator_insert_reg( str );
    free_safe( str, (strlen( str ) + 1) );

    /* Now create the assignment string */
    slen = 1 + strlen( name ) + 3 + strlen( value ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, " %s = %s;", name, value );
    assert( rv < slen );

  }

  /* Append the line coverage assignment to the working buffer */
  generator_add_cov_to_work_code( str );

  /* Deallocate temporary memory */
  free_safe( idxstr, (strlen( idxstr ) + 1) );
  free_safe( value,  (strlen( value )  + 1) );
  free_safe( str,    (strlen( str )    + 1) );
  free_safe( scope, (strlen( scope ) + 1) );

  PROFILE_END;

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

    if( (exp->sig != NULL) && (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM) && exp->elem.dim->last ) {
      generator_insert_mem_cov( exp, funit, net, ((exp->suppl.part.lhs == 1) && !treat_as_rhs) );
    }

    generator_insert_mem_cov_helper( exp->left,  funit, net, treat_as_rhs );
    generator_insert_mem_cov_helper( exp->right, funit, net, treat_as_rhs );

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
  if( ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_memory == 1)) && !handle_funit_as_assert ) {

    if( (stmt = generator_find_statement( first_line, first_column )) != NULL ) {

      /* Generate combinational coverage */
      if( info_suppl.part.scored_comb == 1 ) {
        generator_insert_comb_cov_helper( (use_right ? stmt->exp->right : stmt->exp), stmt->funit, (use_right ? stmt->exp->right->op : stmt->exp->op), 0, net, TRUE, TRUE );
      }

      /* Generate memory coverage */
      if( info_suppl.part.scored_memory == 1 ) {
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

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert ) {

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

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert && (stmt != NULL) ) {

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
  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert && ((stmt = generator_find_case_statement( first_line, first_column )) != NULL) ) {

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

  if( (info_suppl.part.scored_fsm == 1) && !handle_funit_as_assert ) {

    fsm_link*    fsml = curr_funit->fsm_head;
    unsigned int id   = 1;

    while( fsml != NULL ) {

      if( fsml->table->from_state->id == fsml->table->to_state->id ) {

        int   number;
        char* size = generator_gen_size( fsml->table->from_state, curr_funit, &number );
        char* exp  = codegen_gen_expr_one_line( fsml->table->from_state, curr_funit, FALSE );
        if( number >= 0 ) {
          fprintf( curr_ofile, "wire [%d:0] \\covered$F%d = %s;\n", (number - 1), id, exp );
        } else {
          fprintf( curr_ofile, "wire [(%s-1):0] \\covered$F%d = %s;\n", ((size != NULL) ? size : "1"), id, exp );
        }
        free_safe( size, (strlen( size ) + 1) );
        free_safe( exp, (strlen( exp ) + 1) );

      } else {

        int   from_number;
        int   to_number;
        char* fsize = generator_gen_size( fsml->table->from_state, curr_funit, &from_number );
        char* fexp  = codegen_gen_expr_one_line( fsml->table->from_state, curr_funit, FALSE );
        char* tsize = generator_gen_size( fsml->table->to_state, curr_funit, &to_number );
        char* texp  = codegen_gen_expr_one_line( fsml->table->to_state, curr_funit, FALSE );
        if( from_number >= 0 ) {
          if( to_number >= 0 ) {
            fprintf( curr_ofile, "wire [%d:0] \\covered$F%d = {%s,%s};\n", ((from_number + to_number) - 1), id, fexp, texp );
          } else {
            fprintf( curr_ofile, "wire [((%d+%s)-1):0] \\covered$F%d = {%s,%s};\n", from_number, ((tsize != NULL) ? tsize : "1"), id, fexp, texp );
          }
        } else {
          if( to_number >= 0 ) {
            fprintf( curr_ofile, "wire [((%s+%d)-1):0] \\covered$F%d = {%s,%s};\n", ((fsize != NULL) ? fsize : "1"), to_number, id, fexp, texp );
          } else {
            fprintf( curr_ofile, "wire [((%s+%s)-1):0] \\covered$F%d = {%s,%s};\n",
                     ((fsize != NULL) ? fsize : "1"), ((tsize != NULL) ? tsize : "1"), id, fexp, texp );
          }
        }
        free_safe( fsize, (strlen( fsize ) + 1) );
        free_safe( fexp,  (strlen( fexp )  + 1) );
        free_safe( tsize, (strlen( tsize ) + 1) );
        free_safe( texp,  (strlen( texp )  + 1) );

      }

      fsml = fsml->next;
      id++;

    }

  }

  PROFILE_END;

}

/*!
 Replaces "event" types with "reg" types when performing combinational logic coverage.  This behavior
 is needed because events are impossible to discern coverage from when multiple events are used within a wait
 statement.
*/
void generator_handle_event_type( 
  unsigned int first_line,   /*!< First line of "event" type specifier */
  unsigned int first_column  /*!< First column of "event" type specifier */
) { PROFILE(GENERATOR_HANDLE_EVENT_TYPE);

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert ) {
    generator_replace( "reg", first_line, first_column, first_line, (first_column + 4) );
  }

  PROFILE_END;

}

/*!
 Transforms the event trigger to a register inversion (converts X to 0 if the identifier has not been initialized).
 This behavior is needed because events are impossible to discern coverage from when multiple events are used within a wait
 statement.
*/
void generator_handle_event_trigger(
  const char*  identifier,    /*!< Name of trigger identifier */
  unsigned int first_line,    /*!< First line of the '->' specifier */
  unsigned int first_column,  /*!< First column of the '->' specifier */
  unsigned int last_line,     /*!< Last line which contains the trigger identifier (not including the semicolon) */
  unsigned int last_column    /*!< Last column which contains the trigger identifier (not including the semicolon) */
) { PROFILE(GENERATOR_HANDLE_EVENT_TRIGGER);

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert ) {

    char         str[4096];
    unsigned int rv = snprintf( str, 4096, "%s = (%s === 1'bx) ? 1'b0 : ~%s", identifier, identifier, identifier ); 
   
    /* Perform the replacement */
    generator_replace( str, first_line, first_column, last_line, last_column );

  }

  PROFILE_END;

}


/*
 $Log$
 Revision 1.84  2009/01/20 14:48:17  phase1geo
 Fixing issue that comes up when combinational logic coverage is not being generated.

 Revision 1.83  2009/01/20 05:27:55  phase1geo
 Fixing bug with multiple replacements.  Added random4 diagnostic to verify this
 bug fix.  Checkpointing.

 Revision 1.82  2009/01/19 21:51:33  phase1geo
 Added -inlined-metrics score command option and hooked up its functionality.  Regressions
 pass with these changes; however, I have not been able to verify using this option yet.
 Checkpointing.

 Revision 1.81  2009/01/19 03:50:24  phase1geo
 Fixing multiply and concatenation size calculations.  Full IV regression
 passes.

 Revision 1.80  2009/01/18 05:27:44  phase1geo
 Fixing valgrind errors.

 Revision 1.79  2009/01/17 06:25:47  phase1geo
 Adding code to reduce reg/wire sizing output.  Fixing unary codegen handling when
 they are surrounded by parenthesis.  Updating regression output.  Checkpointing.

 Revision 1.78  2009/01/16 15:02:02  phase1geo
 Updates for support of problems found in covering real code.  Checkpointing.

 Revision 1.77  2009/01/16 00:03:54  phase1geo
 Fixing last issue with IV/Cver regressions (OVL assertions).  Updating
 regressions per needed changes to support this functionality.  Now only
 VCS regression needs to be updated.

 Revision 1.76  2009/01/14 23:53:38  phase1geo
 Updates for assertion coverage.  Still need to handle file inclusion renumbering for
 this to work.  Checkpointing.

 Revision 1.75  2009/01/14 21:01:35  phase1geo
 Fixing last remaining issues with generate blocks.  Checkpointing.

 Revision 1.74  2009/01/14 14:57:33  phase1geo
 Initial addition of the functional unit stack for inlined coverage code.
 Checkpointing.

 Revision 1.73  2009/01/13 23:37:31  phase1geo
 Adding support for register insertion when generating inlined coverage code.
 Still have one more issue to resolve before generate blocks work.  Checkpointing.

 Revision 1.72  2009/01/11 19:59:35  phase1geo
 More fixes for support of generate statements.  Getting close but not quite
 there yet.  Checkpointing.

 Revision 1.71  2009/01/10 00:24:10  phase1geo
 More work on support for generate blocks (the new changes don't quite work yet).
 Checkpointing.

 Revision 1.70  2009/01/09 21:25:00  phase1geo
 More generate block fixes.  Updated all copyright information source code files
 for the year 2009.  Checkpointing.

 Revision 1.69  2009/01/09 06:06:25  phase1geo
 Another fix and removing unnecessary output.  Checkpointing.

 Revision 1.68  2009/01/09 05:53:39  phase1geo
 More work on generate handling.  Still not working but Verilog output looks correct
 for IF generate statements now.  Checkpointing.

 Revision 1.67  2009/01/08 23:44:08  phase1geo
 Updating VCS regressions.  Fixing issues in regards to PDEC, PINC, IINC and IDEC
 operations.  Checkpointing.

 Revision 1.66  2009/01/08 16:21:57  phase1geo
 Working on support for generate blocks.  Checkpointing.

 Revision 1.65  2009/01/08 16:13:59  phase1geo
 Completing work on substitution support.  Updated regressions.

 Revision 1.64  2009/01/08 14:12:34  phase1geo
 Fixing some issues from last checkin.  Still a few more issues to iron out but
 this is close.  Checkpointing.

 Revision 1.63  2009/01/08 08:00:21  phase1geo
 Closest thing to working code for code substitution.  Not quite finished yet
 but close.  Checkpointing.

 Revision 1.62  2009/01/07 23:40:46  phase1geo
 Updates to support intermediate expression substitution.  Not done yet.  Checkpointing.

 Revision 1.61  2009/01/06 23:51:56  phase1geo
 Cleaning up unnecessary output.

 Revision 1.60  2009/01/06 23:11:00  phase1geo
 Completed and debugged new generator_replace functionality.  Fixed issue with
 event coverage handling.  Added new event2 diagnostic to verify the event
 coverage handling.  Checkpointing.

 Revision 1.59  2009/01/06 14:35:18  phase1geo
 Starting work on generator_replace functionality.  Not quite complete yet
 but I need to checkpoint.

 Revision 1.58  2009/01/06 06:59:22  phase1geo
 Adding initial support for string replacement.  More work to do here.
 Checkpointing.

 Revision 1.57  2009/01/05 20:15:26  phase1geo
 Fixing issue with memory coverage.  Checkpointing (20 diags fail currently).

 Revision 1.56  2009/01/04 20:11:19  phase1geo
 Completed initial work on event handling.

 Revision 1.55  2009/01/03 21:21:25  phase1geo
 Fixing combinational logic coverage issue with dimension operators.  Update
 to regressions.  Checkpointing (30 diagnostics failing in IV regression).

 Revision 1.54  2009/01/03 20:41:14  phase1geo
 Fixing lastest issue with memory coverage.  Need to fix a combinational logic
 coverage issue next.  Checkpointing.

 Revision 1.53  2009/01/03 08:03:53  phase1geo
 Adding more code to support memory coverage.  Added to code to handle parameterized
 signal sizing.  Updated regressions.  Checkpointing.

 Revision 1.52  2009/01/03 03:49:56  phase1geo
 Checkpointing memory coverage work.

 Revision 1.51  2009/01/02 06:00:26  phase1geo
 More updates for memory coverage (this is still not working however).  Currently
 segfaults.  Checkpointing.

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

