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

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "codegen.h"
#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_iter.h"
#include "func_unit.h"
#include "generator.new.h"
#include "gen_item.h"
#include "instance.h"
#include "link.h"
#include "ovl.h"
#include "param.h"
#include "parser_misc.h"
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
extern unsigned int   inline_comb_depth;
extern char*          verilator_prefix;
extern bool           test_mode;
extern int*           fork_block_depth;
extern int            fork_depth;
extern int            block_depth;


struct fname_link_s;
typedef struct fname_link_s fname_link;
struct fname_link_s {
  char*       filename;    /*!< Filename associated with all functional units */
  func_unit*  next_funit;  /*!< Pointer to the next/current functional unit that will be parsed */
  funit_link* head;        /*!< Pointer to head of functional unit list */
  funit_link* tail;        /*!< Pointer to tail of functional unit list */
  fname_link* next;        /*!< Pointer to next filename list */
};

struct funit_str_s;
typedef struct funit_str_s funit_str;
struct funit_str_s {
  char*      str;    /*!< String containing inserted temporary registers for the given functional unit */
  func_unit* funit;  /*!< Functional unit that will contain the given string */
  funit_str* next;   /*!< Pointer to the next element in the funit_str list */
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
 Pointer to the top of the temporary register string stack.
*/
funit_str* tmp_regs_top = NULL;

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
 Specifies that we should handle the current functional unit as an assertion.
*/
bool handle_funit_as_assert = FALSE;

/*!
 Iterator for current functional unit.
*/
static func_iter fiter;

/*!
 Pointer to the current functional unit instance.
*/
static funit_inst* curr_inst;

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

/*!
 Index of last token added to the working buffer.
*/
static unsigned int last_token_index;

/*!
 Look-ahead buffer that stores last parsed token.
*/
static char lahead_buffer[4096];

/*!
 Name of the output database file.
*/
static const char* output_db;

static char* generator_gen_size(
  expression* exp,
  func_unit*  funit,
  int*        number
);

/*!
 \return Returns allocated string containing the difference in scope between the current functional unit
         and the specified child functional unit.
*/
static char* generator_get_relative_scope(
  func_unit* child  /*!< Pointer to child functional unit */
) { PROFILE(GENERATOR_GET_RELATIVE_SCOPE);

  char* back   = strdup_safe( child->name );
  char* relative_scope;
  int   i;

  assert( tmp_regs_top != NULL );
  
  scope_extract_scope( child->name, tmp_regs_top->funit->name, back );
  relative_scope = strdup_safe( back );

#ifdef OBSOLETE
  /* Replace the periods with a '/' character */
  for( i=0; i<strlen( relative_scope ); i++ ) {
    if( relative_scope[i] == '.' ) {
      relative_scope[i] = '/';
    }
  }
#endif

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
 \return Returns TRUE if the given functional unit is within a static-only function; otherwise, returns FALSE.
*/
static bool generator_is_static_function_only(
  func_unit* funit  /*!< Pointer to functional unit to check */
) { PROFILE(GENERATOR_IS_STATIC_FUNCTION_ONLY);

  func_unit* func = funit_get_curr_function( funit );

  PROFILE_END;

  return( (func != NULL) && (func->suppl.part.staticf == 1) && (func->suppl.part.normalf == 0) );

}

/*!
 \return Returns TRUE if the given functional unit is within a static function; otherwise, returns FALSE.
*/
bool generator_is_static_function(
  func_unit* funit  /*!< Pointer to functional unit to check */
) { PROFILE(GENERATOR_IS_STATIC_FUNCTION);

  bool retval = FALSE;

  if( funit != NULL ) {
    func_unit* func = funit_get_curr_function( funit );
    retval = (func != NULL) && (func->suppl.part.staticf == 1);
  }

  PROFILE_END;

  return( retval );

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

  // printf( "In generator_replace, str: %s, first_line: %u, first_column: %u, last_line: %u, last_column: %u\n", str, first_line, first_column, last_line, last_column );

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
          (void)str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
          strcpy( work_buffer, str );
        }

        /* Now append the end of the working buffer */
        if( (strlen( work_buffer ) + strlen( keep_end )) < 4095 ) {
          replace_first.word_ptr = work_buffer + strlen( work_buffer );
          strcat( work_buffer, keep_end );
        } else {
          (void)str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
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
        work_tail              = first_list_ptr;

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

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "In generator_push_reg_insert", DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Make sure that the hold buffer is added to the hold list */
  if( hold_buffer[0] != '\0' ) {
    strcat( hold_buffer, "\n" );
    (void)str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
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

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "In generator_pop_reg_insert", DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Save pointer to the top reg_insert structure and adjust reg_top */
  ri      = reg_top;
  reg_top = ri->next;

  /* Deallocate the reg_insert structure */
  free_safe( ri, sizeof( reg_insert ) );

}

#ifdef OBSOLETE
/*!
 \return Returns TRUE if the reg_top stack contains exactly one element.
*/
static bool generator_is_reg_top_one_deep() { PROFILE(GENERATOR_IS_BASE_REG_INSERT);

  bool retval = (reg_top != NULL) && (reg_top->next == NULL);

  PROFILE_END;

  return( retval );

}
#endif

/*!
 Inserts the given register instantiation string into the appropriate spot in the hold buffer.
*/
static void generator_insert_reg(
  const char* str,     /*!< Register instantiation string to insert */
  bool        tmp_reg  /*!< Set to TRUE if the added register/wire is an intermediate signal */
) { PROFILE(GENERATOR_INSERT_REG);

  assert( tmp_regs_top != NULL );

  /* If the signal is an intermediate signal, turn tracing off for this signal */
  if( tmp_reg ) {
    tmp_regs_top->str = generator_build( 3, tmp_regs_top->str, strdup_safe( "/* verilator tracing_off */" ), "\n" );
  }

  /* Create string link */
  tmp_regs_top->str = generator_build( 3, tmp_regs_top->str, strdup_safe( str ), "\n" );

  /* If the signal is an intermediate signal, turn tracing back on for this signal */
  if( tmp_reg ) {
    tmp_regs_top->str = generator_build( 3, tmp_regs_top->str, strdup_safe( "/* verilator tracing_on */" ), "\n" );
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
                (exp->op == EXP_OP_SURAND_RANGE) ||
                (exp->op == EXP_OP_SVALARGS);

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

  bool retval = (depth < inline_comb_depth) && (EXPR_IS_MEASURABLE( exp ) == 1) && !expression_is_static_only( exp );
          
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
    if( exp->suppl.part.eval_t ) {
      exp->suppl.part.eval_t    = 0;
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
  char         op[30];
  char         fline[30];
  char         lline[30];
  char         col[30];

  assert( exp != NULL );

  /* Create string versions of op, first line, last line and column information */
  rv = snprintf( op,    30, "%x", exp->op );       assert( rv < 30 );
  rv = snprintf( fline, 30, "%u", exp->ppfline );  assert( rv < 30 );
  rv = snprintf( lline, 30, "%u", exp->pplline );  assert( rv < 30 );
  rv = snprintf( col,   30, "%x", exp->col.all );  assert( rv < 30 );

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

    /* Only add modules that are not the $root "module" and are not assertion modules (if assertion coverage is off) */
    if( (funitl->funit->suppl.part.type == FUNIT_MODULE)  &&
        (strncmp( "$root", funitl->funit->name, 5 ) != 0) &&
        ((info_suppl.part.scored_assert == 1) || !ovl_is_assertion_module( funitl->funit )) ) {

      fname_link* fnamel      = *head;
      const char* funit_fname = (funitl->funit->incl_fname != NULL) ? funitl->funit->incl_fname : funitl->funit->orig_fname;

      /* Search for functional unit filename in our filename list */
      while( (fnamel != NULL) && (strcmp( fnamel->filename, funit_fname ) != 0) ) {
        fnamel = fnamel->next;
      }

      /* If the filename link was not found, create a new link */
      if( fnamel == NULL ) {

        /* Allocate and initialize the filename link */
        fnamel             = (fname_link*)malloc_safe( sizeof( fname_link ) );
        fnamel->filename   = strdup_safe( funit_fname );
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

#ifdef OBSOLETE
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
#endif

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

  // inst_link_display( db_list[curr_db]->inst_head );

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
      (void)str_link_add( strdup_safe( funitl->funit->name ), &modlist_head, &modlist_tail );
      funitl = funitl->next;
    }

    /* Open the output file for writing */
    if( (curr_ofile = fopen( filename, "w" )) != NULL ) {

      /* Parse the original code and output inline coverage code */
      reset_lexer_for_generation( head->filename, "covered/verilog" );
      (void)GENparse();

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
 \return Returns a string containing all of the Verilator instance ID overrides.

 Writes the instance ID assignments for Verilator simulation.
*/
static char* generator_verilator_inst_ids(
  funit_inst* root,        /*!< Pointer to root instance in instance tree to parse */
  char*       parent_name  /*!< Verilator version of parent's scope */
) { PROFILE(GENERATOR_WRITE_VERILATOR_INST_IDS);

  char* inst_str = NULL;

  assert( root != NULL );

  if( ((root->funit == NULL) || (root->funit->suppl.part.type != FUNIT_NO_SCORE)) && !root->suppl.ignore ) {

    funit_inst*  child = root->child_head;
    char         child_name[4096];
    char         name[256];
    char         index_str[30];
    int          index;
    unsigned int rv;

    strcpy( child_name, parent_name );

    if( sscanf( root->name, "%[^[][%d]", name, &index ) == 2 ) {
      strcat( child_name, name );
      strcat( child_name, "__BRA__" );
      rv = snprintf( index_str, 30, "%d", index );
      assert( rv < 30 );
      strcat( child_name, index_str );
      strcat( child_name, "__KET__" );
    } else if( sscanf( root->name, "u$%d", &index ) == 1 ) {
      strcat( child_name, "u__024" );
      rv = snprintf( index_str, 30, "%d", index );
      assert( rv < 30 );
      strcat( child_name, index_str );
    } else {
      strcat( child_name, root->name );
    }

    if( (root->funit == NULL) || (root->funit->suppl.part.type == FUNIT_MODULE) ) {
      strcat( child_name, "->" );
    } else {
      strcat( child_name, "__DOT__" );
    }

    /* Output the line to the file */
    if( (root->funit != NULL) &&
        ((root->funit->suppl.part.type == FUNIT_MODULE) ||
         (root->funit->suppl.part.type == FUNIT_NAMED_BLOCK) ||
         (root->funit->suppl.part.type == FUNIT_ANAMED_BLOCK)) ) {
      char         str[4096];
      unsigned int rv = snprintf( str, 4096, "%sCOVERED_INST_ID%d = %d;", child_name, root->funit->id, root->id );
      assert( rv < 4096 );
      inst_str = generator_build( 2, strdup_safe( str ), "\n" );
    }

    while( child != NULL ) {
      inst_str = generator_build( 2, inst_str, generator_verilator_inst_ids( child, child_name ) );
      child = child->next;
    }

  }

  PROFILE_END;

  return( inst_str );

}

/*!
 Outputs the covered portion of the design to the covered/verilog directory.
*/
void generator_output(
  const char* odb  /*!< Name of the output database file */
) { PROFILE(GENERATOR_OUTPUT);

  fname_link* fname_head = NULL;  /* Pointer to head of filename linked list */
  fname_link* fname_tail = NULL;  /* Pointer to tail of filename linked list */
  fname_link* fnamel;             /* Pointer to current filename link */

  /* Allow ourselves to reference the output database name */
  output_db = odb;

  /* Create the initial "covered" directory - TBD - this should be done prior to this function call */
  if( !directory_exists( "covered" ) ) {
    /*@-shiftimplementation@*/
    if( mkdir( "covered", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
    /*@=shiftimplementation@*/
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
  /*@-shiftimplementation@*/
  if( mkdir( "covered/verilog", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
  /*@=shiftimplementation@*/
    print_output( "Unable to create \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

#ifdef OBSOLETE
  /* If the "covered/include" directory exists, remove it */
  if( directory_exists( "covered/include" ) ) {
    if( system( "rm -rf covered/include" ) != 0 ) {
      print_output( "Unable to remove \"covered/include\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* Create the covered/include directory */
  /*@-shiftimplementation@*/
  if( mkdir( "covered/include", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
  /*@=shiftimplementation@*/
    print_output( "Unable to create \"covered/include\" directory", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }
#endif

  /* Initialize the work_buffer and hold_buffer arrays */
  work_buffer[0] = '\0';
  hold_buffer[0] = '\0';

  /* Initialize the functional unit iter */
  fiter.sls  = NULL;
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

  int ignore = 0;

  /* Capture the current instance */
  curr_inst = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore );

  /* Deallocate the functional unit iterator */
  func_iter_dealloc( &fiter );

  /* Initializes the functional unit iterator */
  func_iter_init( &fiter, funit, TRUE, FALSE, TRUE );

  /* Clear the current statement pointer */
  curr_stmt = NULL;

  /* Reset the replacement string information */
  generator_clear_replace_ptrs();

  /* Calculate if we need to handle this functional unit as an assertion or not */
  handle_funit_as_assert = (info_suppl.part.scored_assert == 1) && ovl_is_assertion_module( funit );

  /* Create a temporary register container */
  generator_create_tmp_regs();

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
      (void)str_link_add( strdup_safe( str ), &work_head, &work_tail );
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
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
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
               bool         from_code,     /*!< Specifies if the string came from the code directly */
  /*@unused@*/ const char*  file,          /*!< Filename that called this function */
  /*@unused@*/ unsigned int line           /*!< Line number where this function call was performed from */
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

    /* Only append the look-ahead buffer contents if the string is coming from the code */
    if( from_code ) {

      /* If something is stored in the look-ahead buffer, add it to the work buffer first */
      if( strlen( lahead_buffer ) > 0 ) {

        assert( (strlen( work_buffer ) + strlen( lahead_buffer)) < 4095 );
        strcat( work_buffer, lahead_buffer );
        lahead_buffer[0] = '\0';

      } else {

        if( work_buffer[0] == '\0' ) {

          /* Set the last_token index */
          last_token_index = 0;

        } else {

          /* Make sure any leading whitespace is included with a held token */
          char* ptr = work_buffer + (strlen( work_buffer ) - 1);
          while( (ptr > work_buffer) && ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\b')) ) ptr--;

          /* Set the last_token index */
          if( ptr == work_buffer ) {
            last_token_index = 0;
          } else {
            last_token_index = (ptr + 1) - work_buffer;
          }

        }

      }

    }

    /* I don't believe that a line will ever exceed 4K chars */
    assert( (strlen( work_buffer ) + strlen( str )) < 4095 );
    strcat( work_buffer, str );

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Adding to work code [%s] (fline: %u, fcol: %u, from_code: %d, file: %s, line: %u)",
                                  str, first_line, first_column, from_code, file, line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

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
  /*@unused@*/ const char*  file,  /*!< Filename that calls this function */
  /*@unused@*/ unsigned int line   /*!< Line number that calls this function */
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
    (void)str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
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
  if( strlen( lahead_buffer ) == 0 ) {
    generator_clear_replace_ptrs();
  } else {
    char* ptr = lahead_buffer;
    while( (*ptr != '\0') && ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\b')) ) ptr++;
    if( *ptr == '\0' ) {
      generator_clear_replace_ptrs();
    }
  }

  PROFILE_END;

}

/*!
 Adds the given code string to the "immediate" code generator.  This code can be output to the file
 immediately if there is no code in the sig_list and exp_list arrays.  If it cannot be output immediately,
 the code is added to the exp_list array.
*/
void generator_add_to_hold_code(
               const char*  str,   /*!< String to write */
  /*@unused@*/ const char*  file,  /*!< Filename of caller of this function */
  /*@unused@*/ unsigned int line   /*!< Line number of caller of this function */
) { PROFILE(GENERATOR_ADD_TO_HOLD_CODE);
 
  /* I don't believe that a line will ever exceed 4K chars */
  assert( (strlen( hold_buffer ) + strlen( str )) < 4095 );
  strcat( hold_buffer, str );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Adding to hold code [%s] (file: %s, line: %u)", str, file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* If we have hit a newline, add it to the hold list and clear the hold buffer */
  if( strcmp( str, "\n" ) == 0 ) {
    (void)str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
    hold_buffer[0] = '\0';
  }

  PROFILE_END;

}

/*!
 Outputs all held code to the output file.
*/
void generator_flush_hold_code1(
  /*@unused@*/ const char*  file,  /*!< Filename that calls this function */
  /*@unused@*/ unsigned int line   /*!< Line number that calls this function */
) { PROFILE(GENERATOR_FLUSH_HOLD_CODE1);

  str_link* strl = hold_head;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing hold code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* We shouldn't ever be flushing the hold code if the reg_top is more than one entry deep */
  assert( (reg_top == NULL) || (reg_top->next == NULL) );

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

  printf( "In generator_find_statement, line: %d, column: %d, funit_top: %s\n", first_line, first_column, funit_top->funit->name );

  if( (curr_stmt == NULL) || (curr_stmt->exp->ppfline != first_line) || (curr_stmt->exp->col.part.first != first_column) ) {

    stmt_link* stmtl = stmt_link_find_by_position( first_line, first_column, funit_top->funit->stmt_head );

    stmt_link_display( funit_top->funit->stmt_head );

    /* If we couldn't find it in the func_iter, look for it in the generate list */
    if( stmtl == NULL ) {
      statement* gen_stmt = generate_find_stmt_by_position( curr_funit, first_line, first_column );
      if( gen_stmt != NULL ) {
        curr_stmt = gen_stmt;
      }
    } else {
      curr_stmt = stmtl->stmt;
    }

  }

  if( (curr_stmt != NULL) && (curr_stmt->exp->ppfline == first_line) && (curr_stmt->exp->col.part.first == first_column) && (curr_stmt->exp->op != EXP_OP_FORK) ) {
    printf( "  FOUND (%s %x)!\n", expression_string( curr_stmt->exp ), curr_stmt->exp->col.part.first );
  } else {
    printf( "  NOT FOUND!\n" );
  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->ppfline != first_line) ||
          (curr_stmt->exp->col.part.first != first_column) || (curr_stmt->exp->op == EXP_OP_FORK)) ? NULL : curr_stmt );

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

//  printf( "In generator_find_case_statement, line: %d, column: %d\n", first_line, first_column );

  if( (curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->ppfline != first_line) || (curr_stmt->exp->left->col.part.first != first_column) ) {

    stmt_link* stmtl = funit_top->funit->stmt_head;
    while( (stmtl != NULL) && ((stmtl->stmt->exp->left == NULL) || (stmtl->stmt->exp->left->ppfline != first_line) || (stmtl->stmt->exp->left->col.part.first != first_column)) ) {
      stmtl = stmtl->next;
    }

    curr_stmt = (stmtl != NULL) ? stmtl->stmt : NULL;

  }

//  if( (curr_stmt != NULL) && (curr_stmt->exp->left != NULL) && (curr_stmt->exp->left->ppfline == first_line) && (curr_stmt->exp->left->col.part.first == first_column) ) {
//    printf( "  FOUND (%s %x)!\n", expression_string( curr_stmt->exp->left ), curr_stmt->exp->left->col.part.first );
//  } else {
//    printf( "  NOT FOUND!\n" );
//  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->ppfline != first_line) ||
          (curr_stmt->exp->left->col.part.first != first_column)) ? NULL : curr_stmt );

}

/*!
 \return Returns a string containing the line coverage information.

 Inserts line coverage information for the given statement.
*/
char* generator_line_cov_with_stmt(
  statement* stmt,      /*!< Pointer to statement to generate line coverage for */
  bool       semicolon  /*!< Specifies if a semicolon (TRUE) or a comma (FALSE) should be appended to the created line */
) { PROFILE(GENERATOR_INSERT_LINE_COV_WITH_STMT);

  char str[4096];

  if( (stmt != NULL) && ((info_suppl.part.scored_line && !handle_funit_as_assert) || (handle_funit_as_assert && ovl_is_coverage_point( stmt->exp ))) ) {

    unsigned int rv;
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;

    if( info_suppl.part.verilator ) {

      /* If the statement is not a head statement or not an event, add the line */
      if( !stmt->suppl.part.head || ((stmt->exp->op != EXP_OP_EOR) && (stmt->exp->op != EXP_OP_PEDGE) && (stmt->exp->op != EXP_OP_NEDGE) && (stmt->exp->op != EXP_OP_AEDGE)) ) {
        rv = snprintf( str, 4096, " $c( \"covered_line( \", COVERED_INST_ID%d, \", %d );\" )%c",
                       stmt->funit->id,
                       (expression_get_id( stmt->exp, TRUE ) - expression_get_id( stmt->funit->exps[0], TRUE )),
                       (semicolon ? ';' : ',') );
        assert( rv < 4096 );
      }

    } else {

      char  sig[4096];
      char* scope    = generator_get_relative_scope( stmt->funit ); 

      if( scope[0] == '\0' ) {
        rv = snprintf( sig, 4096, " \\covered$L%u_%u_%x ", stmt->exp->ppfline, stmt->exp->pplline, stmt->exp->col.all );
      } else {
        rv = snprintf( sig, 4096, " \\covered$L%u_%u_%x$%s ", stmt->exp->ppfline, stmt->exp->pplline, stmt->exp->col.all, scope );
      }
      assert( rv < 4096 );
      free_safe( scope, (strlen( scope ) + 1) );

      /* Create the register */
      rv = snprintf( str, 4096, "reg %s;", sig );
      assert( rv < 4096 );
      generator_insert_reg( str, FALSE );

      /* Prepend the line coverage assignment to the working buffer */
      rv = snprintf( str, 4096, " %s = 1'b1%c", sig, (semicolon ? ';' : ',') );
      assert( rv < 4096 );

    }

  }

  PROFILE_END;

  return( strdup_safe( str ) );

}

/*!
 \return Returns a pointer to the statement inserted (or NULL if no statement was inserted).

 Inserts line coverage information in string queues.
*/
char* generator_line_cov(
               unsigned int first_line,    /*!< First line to create line coverage for */
  /*@unused@*/ unsigned int last_line,     /*!< Last line to create line coverage for */
               unsigned int first_column,  /*!< First column of statement */
  /*@unused@*/ unsigned int last_column,   /*!< Last column of statement */
               bool         semicolon      /*!< Set to TRUE to create a semicolon after the line assignment; otherwise, adds a comma */
) { PROFILE(GENERATOR_INSERT_LINE_COV);

  statement* stmt;
  char*      str = NULL;

  if( ((stmt = generator_find_statement( first_line, first_column )) != NULL) &&
      !generator_is_static_function_only( stmt->funit ) &&
      ((info_suppl.part.scored_line && !handle_funit_as_assert) || (handle_funit_as_assert && ovl_is_coverage_point( stmt->exp ))) ) {

    str = generator_line_cov_with_stmt( stmt, semicolon );

  }

  PROFILE_END;

  return( str );

}

/*!
 \return Returns a string containing the event coverage information.

 Handles the insertion of event-type combinational logic code.
*/
char* generator_event_comb_cov(
  expression* exp,        /*!< Pointer to expression to output */
  func_unit*  funit,      /*!< Pointer to functional unit containing the expression */
  bool        reg_needed  /*!< If set to TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_EVENT_COMB_COV);

  char         name[4096];
  char         str[4096];
  unsigned int rv;
  expression*  root_exp = exp;
  char*        scope    = generator_get_relative_scope( funit );
  char*        cov_str  = NULL;

  /* Find the root event of this expression tree */
  while( (ESUPPL_IS_ROOT( root_exp->suppl ) == 0) && (EXPR_IS_EVENT( root_exp->parent->expr ) == 1) ) {
    root_exp = root_exp->parent->expr;
  }

  /* Create signal name */
  if( scope[0] == '\0' ) {
    rv = snprintf( name, 4096, " \\covered$E%u_%u_%x ", exp->ppfline, exp->pplline, exp->col.all );
  } else {
    rv = snprintf( name, 4096, " \\covered$E%u_%u_%x$%s ", exp->ppfline, exp->pplline, exp->col.all, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create register */
  if( reg_needed ) {
    rv = snprintf( str, 4096, "reg %s;", name );
    assert( rv < 4096 );
    generator_insert_reg( str, FALSE );
  }

  /*
   If the expression is also the root of its expression tree, it is the only event in the statement; therefore,
   the coverage string should just set the coverage variable to a value of 1 to indicate that the event was hit.
  */
  if( exp == root_exp ) {

    /* Create assignment and append it to the working code list */
    rv = snprintf( str, 4096, "%s = 1'b1;", name );
    assert( rv < 4096 );
    cov_str = generator_build( 2, strdup_safe( str ), "\n" );

  /*
   Otherwise, we need to save off the state of the temporary event variable and compare it after the event statement
   has triggered to see which events where hit in the event statement.
  */
  } else {

    char* tname     = generator_create_expr_name( exp );
    char* event_str = codegen_gen_expr_one_line( exp->right, funit, FALSE );
    bool  stmt_head = (root_exp->parent->stmt->suppl.part.head == 1);

    /* Handle the event */
    switch( exp->op ) {
      case EXP_OP_PEDGE :
        {
          if( reg_needed && (exp->suppl.part.eval_t == 0) ) {
            rv = snprintf( str, 4096, "reg %s;", tname );
            assert( rv < 4096 );
            generator_insert_reg( str, FALSE );
            exp->suppl.part.eval_t = 1;
          }
          rv = snprintf( str, 4096, " %s = (%s!==1'b1) & ((%s)===1'b1);", name, tname, event_str );
          assert( rv < 4096 );
          cov_str = strdup_safe( str );
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          cov_str = generator_build( 3, cov_str, strdup_safe( str ), "\n" );
        }
        break;

      case EXP_OP_NEDGE :
        {
          if( reg_needed && (exp->suppl.part.eval_t == 0) ) {
            rv = snprintf( str, 4096, "reg %s;", tname );
            assert( rv < 4096 );
            generator_insert_reg( str, FALSE );
            exp->suppl.part.eval_t = 1;
          }
          rv = snprintf( str, 4096, " %s = (%s!==1'b0) & ((%s)===1'b0);", name, tname, event_str );
          assert( rv < 4096 );
          cov_str = strdup_safe( str );
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          cov_str = generator_build( 3, cov_str, strdup_safe( str ), "\n" );
        }
        break;

      case EXP_OP_AEDGE :
        {
          if( reg_needed && (exp->suppl.part.eval_t == 0) ) {
            int   number;
            char* size = generator_gen_size( exp->right, funit, &number );
            if( number >= 0 ) {
              rv = snprintf( str, 4096, "reg [%d:0] %s;", (number - 1), tname );
            } else {
              rv = snprintf( str, 4096, "reg [((%s)-1):0] %s;", size, tname );
            }
            assert( rv < 4096 );
            generator_insert_reg( str, FALSE );
            free_safe( size, (strlen( size ) + 1) );
            exp->suppl.part.eval_t = 1;
          }
          rv = snprintf( str, 4096, " %s = (%s!==(%s));", name, tname, event_str );
          assert( rv < 4096 );
          cov_str = strdup_safe( str );
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          cov_str = generator_build( 3, cov_str, strdup_safe( str ), "\n" );
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

  return( cov_str );

}

/*!
 \return Returns string containing combinational coverage for the given unary expression.

 Inserts combinational logic coverage code for a unary expression.
*/
static char* generator_unary_comb_cov(
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
  char*        scope   = generator_get_relative_scope( funit );
  char*        cov_str = NULL;

  /* Create signal */
  if( scope[0] == '\0' ) {
    rv = snprintf( sig,  4096, " \\covered$%c%u_%u_%x ", (net ? 'u' : 'U'), exp->ppfline, exp->pplline, exp->col.all );
  } else {
    rv = snprintf( sig,  4096, " \\covered$U%u_%u_%x$%s ", exp->ppfline, exp->pplline, exp->col.all, scope );
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
      rv = snprintf( str, 4096, "reg %s;", sig );
      assert( rv < 4096 );
      generator_insert_reg( str, FALSE );
    }
  }

  /* Prepend the coverage assignment to the working buffer */
  if( exp->value->suppl.part.is_signed == 1 ) {
    rv = snprintf( str, 4096, "%s%s = (%s != 0);", prefix, sig, sigr );
  } else {
    rv = snprintf( str, 4096, "%s%s = (%s > 0);", prefix, sig, sigr );
  }
  assert( rv < 4096 );

  /* Deallocate temporary memory */
  free_safe( sigr, (strlen( sigr ) + 1) );

  if( net ) {
    generator_insert_reg( str, FALSE );
  } else {
    cov_str = generator_build( 2, strdup_safe( str ), "\n" );
  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns string containing combinational coverage information.

 Inserts AND/OR/OTHER-style combinational logic coverage code.
*/
static char* generator_comb_comb_cov(
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
  char*        scope    = generator_get_relative_scope( funit );
  char*        cov_str  = NULL;

  /* Create signal */
  if( scope[0] == '\0' ) {
    rv = snprintf( sig, 4096, " \\covered$%c%u_%u_%x ", (net ? 'c' : 'C'), exp->ppfline, exp->pplline, exp->col.all );
  } else {
    rv = snprintf( sig, 4096, " \\covered$C%u_%u_%x$%s ", exp->ppfline, exp->pplline, exp->col.all, scope );
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
    rv = snprintf( str, 4096, "reg [1:0] %s;", sig );
    assert( rv < 4096 );
    generator_insert_reg( str, FALSE );
  }

  /* Prepend the coverage assignment to the working buffer */
  if( exp->left->value->suppl.part.is_signed == 1 ) {
    if( exp->right->value->suppl.part.is_signed == 1 ) {
      rv = snprintf( str, 4096, "%s%s = {(%s != 0),(%s != 0)};", prefix, sig, sigl, sigr );
    } else {
      rv = snprintf( str, 4096, "%s%s = {(%s != 0),(%s > 0)};", prefix, sig, sigl, sigr );
    }
  } else {
    if( exp->right->value->suppl.part.is_signed == 1 ) {
      rv = snprintf( str, 4096, "%s%s = {(%s > 0),(%s != 0)};", prefix, sig, sigl, sigr );
    } else {
      rv = snprintf( str, 4096, "%s%s = {(%s > 0),(%s > 0)};", prefix, sig, sigl, sigr );
    }
  }
  assert( rv < 4096 );

  /* Deallocate memory */
  free_safe( sigl, (strlen( sigl ) + 1) );
  free_safe( sigr, (strlen( sigr ) + 1) );

  if( net ) {
    generator_insert_reg( str, FALSE );
  } else {
    cov_str = generator_build( 2, strdup_safe( str ), "\n" );
  }

  PROFILE_END;

  return( cov_str );

}

static char* generator_mbit_gen_value(
  expression* exp,
  func_unit*  funit,
  int*        number
) { PROFILE(GENERATOR_MBIT_GEN_VALUE);

  char* value = NULL;

  if( exp != NULL ) {

    if( exp->op == EXP_OP_STATIC ) {
      *number = vector_to_int( exp->value );
    } else { 
      value = codegen_gen_expr_one_line( exp, funit, FALSE );
    }
    
  }

  PROFILE_END;

  return( value );

}

/*!
 \return Returns TRUE if the specified expression is on the RHS of an assignment operation; otherwise,
         returns FALSE.
*/
static bool generator_is_rhs_of_assignment(
  expression* exp  /*!< Pointer to expression to check */
) { PROFILE(GENERATOR_IS_RHS_OF_ASSIGNMENT);

  bool retval = (ESUPPL_IS_ROOT( exp->suppl ) == 0) &&
                (ESUPPL_IS_LHS( exp->suppl )  == 0) &&
                ((exp->parent->expr->op == EXP_OP_ASSIGN)  ||
                 (exp->parent->expr->op == EXP_OP_DASSIGN) ||
                 (exp->parent->expr->op == EXP_OP_BASSIGN) ||
                 (exp->parent->expr->op == EXP_OP_NASSIGN) ||
                 (exp->parent->expr->op == EXP_OP_RASSIGN) ||
                 (exp->parent->expr->op == EXP_OP_DLY_OP));

  PROFILE_END;

  return( retval );

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

  *number = -1;

  if( exp != NULL ) {

    char*        lexp    = NULL;
    char*        rexp    = NULL;
    unsigned int rv;
    int          lnumber = 0;
    int          rnumber = 0;

    switch( exp->op ) {
      case EXP_OP_STATIC :
        *number = exp->value->width;
        break;
      case EXP_OP_LIST     :
      case EXP_OP_MULTIPLY :
        lexp = generator_gen_size( exp->left,  funit, &lnumber );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        if( (lexp == NULL) && (rexp == NULL) ) {
          *number = lnumber + rnumber;
        } else {
          unsigned int slen;
          if( lexp == NULL ) {
            lexp = convert_int_to_str( lnumber );
          } else if( rexp == NULL ) {
            rexp = convert_int_to_str( rnumber );
          }
          slen = 1 + strlen( lexp ) + 3 + strlen( rexp ) + 2;
          size = (char*)malloc_safe( slen );
          rv   = snprintf( size, slen, "(%s)+(%s)", lexp, rexp );
          assert( rv < slen );
          free_safe( lexp, (strlen( lexp ) + 1) );
          free_safe( rexp, (strlen( rexp ) + 1) );
        }
        break;
      case EXP_OP_CONCAT         :
      case EXP_OP_NEGATE         :
      case EXP_OP_COND           :
        size = generator_gen_size( exp->right, funit, number );
        break;
      case EXP_OP_MBIT_POS       :
      case EXP_OP_MBIT_NEG       :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        size = generator_mbit_gen_value( exp->right, funit, number );
        break;
      case EXP_OP_LSHIFT  :
      case EXP_OP_RSHIFT  :
      case EXP_OP_ALSHIFT :
      case EXP_OP_ARSHIFT :
        if( generator_is_rhs_of_assignment( exp ) ) {
          size = generator_gen_size( exp->parent->expr->left, funit, number );
        } else {
          size = generator_gen_size( exp->left, funit, number );
        }
        break;
      case EXP_OP_EXPAND :
        lexp = generator_mbit_gen_value( exp->left, funit, &lnumber );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        if( (lexp == NULL) && (rexp == NULL) ) {
          *number = (lnumber * rnumber) + 1;
        } else {
          unsigned int slen;
          if( lexp == NULL ) {
            lexp = convert_int_to_str( lnumber );
          } else if( rexp == NULL ) {
            rexp = convert_int_to_str( rnumber );
          }
          slen = 3 + strlen( lexp ) + 3 + strlen( rexp ) + 6;
          size = (char*)malloc_safe( slen );
          rv   = snprintf( size, slen, "(((%s)*(%s))+1)", lexp, rexp );
          assert( rv < slen );
          free_safe( lexp, (strlen( lexp ) + 1) );
          free_safe( rexp, (strlen( rexp ) + 1) );
        }
        break;
      case EXP_OP_STIME :
      case EXP_OP_SR2B  :
      case EXP_OP_SR2I  :
        *number = 64;
        break;
      case EXP_OP_SSR2B        :
      case EXP_OP_SRANDOM      :
      case EXP_OP_SURANDOM     :
      case EXP_OP_SURAND_RANGE :
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
        *number = 1;
        break;
      case EXP_OP_SBIT_SEL  :
        {
          unsigned int dimension = expression_get_curr_dimension( exp );
          if( (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM) && ((dimension + 1) < (exp->sig->udim_num + exp->sig->pdim_num)) ) {
            size = mod_parm_gen_size_code( exp->sig, (dimension + 1), funit_get_curr_module( funit ), number );
          } else {
            *number = 1;
          }
        }
        break;
      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
        lexp = generator_mbit_gen_value( exp->left,  funit, &lnumber );
        rexp = generator_mbit_gen_value( exp->right, funit, &rnumber );
        if( (lexp == NULL) && (rexp == NULL) ) {
          *number = ((exp->sig->suppl.part.big_endian == 1) ? (rnumber - lnumber) : (lnumber - rnumber)) + 1;
        } else {
          unsigned int slen;
          if( lexp == NULL ) {
            lexp = convert_int_to_str( lnumber );
          } else if( rexp == NULL ) {
            rexp = convert_int_to_str( rnumber );
          }
          slen = 2 + strlen( lexp ) + 3 + strlen( rexp ) + 5;
          size = (char*)malloc_safe( slen );
          if( exp->sig->suppl.part.big_endian == 1 ) {
            rv = snprintf( size, slen, "((%s)-(%s))+1", rexp, lexp );
          } else {
            rv = snprintf( size, slen, "((%s)-(%s))+1", lexp, rexp );
          }
          assert( rv < slen );
          free_safe( lexp, (strlen( lexp ) + 1) );
          free_safe( rexp, (strlen( rexp ) + 1) );
        }
        break;
      case EXP_OP_SIG       :
      case EXP_OP_PARAM     :
      case EXP_OP_FUNC_CALL :
        if( (exp->sig->suppl.part.type == SSUPPL_TYPE_GENVAR) || (exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_SREAL) ) {
          *number = 32;
        } else if( exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_REAL ) {
          *number = 64;
        } else {
          size = mod_parm_gen_size_code( exp->sig, expression_get_curr_dimension( exp ), funit_get_curr_module( funit ), number );
        }
        break;
      default :
        {
          bool set = FALSE;
          if( exp->left != NULL ) {
            lexp = generator_gen_size( exp->left, funit, &lnumber );
            if( exp->right == NULL ) {
              if( lexp == NULL ) {
                *number = lnumber;
              } else {
                size = lexp;
              }
              set = TRUE;
            }
          }
          if( exp->right != NULL ) {
            rexp = generator_gen_size( exp->right, funit, &rnumber );
            if( exp->left == NULL ) {
              if( rexp == NULL ) {
                *number = rnumber;
              } else {
                size = rexp;
              }
              set = TRUE;
            }
          }
          if( !set ) {
            if( (lexp == NULL) && (rexp == NULL) ) {
              *number = (lnumber > rnumber) ? lnumber : rnumber;
            } else {
              unsigned int slen;
              if( lexp == NULL ) {
                lexp = convert_int_to_str( lnumber );
              } else if( rexp == NULL ) {
                rexp = convert_int_to_str( rnumber );
              }
              slen = (strlen( lexp ) * 2) + (strlen( rexp ) * 2) + 14;
              size = (char*)malloc_safe( slen );
              rv   = snprintf( size, slen, "((%s)>(%s))?(%s):(%s)", lexp, rexp, lexp, rexp );
              assert( rv < slen );
              free_safe( lexp, (strlen( lexp ) + 1) );
              free_safe( rexp, (strlen( rexp ) + 1) );
            }
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
) { PROFILE(GENERATOR_CREATE_LHS);

  unsigned int rv;
  char*        name = generator_create_expr_name( exp );
  char*        size;
  int          number;
  unsigned int slen;
  char*        str  = NULL;

  /* Generate MSB string */
  size = generator_gen_size( exp, funit, &number );

  if( net ) {

    /* Create sized wire string */
    if( size == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      slen = 6 + strlen( tmp ) + 4 + strlen( name ) + 2;
      str  = (char*)malloc_safe( slen );
      rv   = snprintf( str, slen, "wire [%s:0] %s;", tmp, name );
    } else {
      slen = 7 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 2;
      str  = (char*)malloc_safe_nolimit( slen );
      rv   = snprintf( str, slen, "wire [(%s)-1:0] %s;", ((size != NULL) ? size : "1"), name );
    }

  /* Create sized register string */
  } else if( reg_needed && (exp->suppl.part.eval_t == 0) ) {

    if( size == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      if( exp->value->suppl.part.is_signed == 1 ) {
        slen = 30 + strlen( name ) + 20 + strlen( tmp ) + 4 + strlen( name ) + 9;
        str  = (char*)malloc_safe( slen );
        rv   = snprintf( str, slen, "`ifdef V1995_COV_MODE\ninteger %s;\n`else\nreg signed [%s:0] %s;\n`endif", name, tmp, name );
      } else {
        slen = 5 + strlen( tmp ) + 4 + strlen( name ) + 2;
        str  = (char*)malloc_safe( slen );
        rv   = snprintf( str, slen, "reg [%s:0] %s;", tmp, name );
      }
    } else {
      if( exp->value->suppl.part.is_signed == 1 ) {
        slen = 30 + strlen( name ) + 21 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 9;
        str  = (char*)malloc_safe_nolimit( slen );
        rv   = snprintf( str, slen, "`ifdef V1995_COV_MODE\ninteger %s;\n`else\nreg signed [(%s-1):0] %s;\n`endif", name, ((size != NULL) ? size : "1"), name );
      } else {
        slen = 6 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 2;
        str  = (char*)malloc_safe_nolimit( slen );
        rv   = snprintf( str, slen, "reg [(%s)-1:0] %s;", ((size != NULL) ? size : "1"), name );
      }
    }

    exp->suppl.part.eval_t = 1;

  }

  /* Add the wire/reg */
  if( str != NULL ) {
    assert( rv < slen );
    generator_insert_reg( str, TRUE );
  }

  /* Deallocate memory */
  free_safe( str,  (strlen( str )  + 1) );
  free_safe( size, (strlen( size ) + 1) );

  PROFILE_END;

  return( name );

}

/*!
 \return Returns the string from the given subexpression.

 Generates temporary subexpression for the given expression (not recursively)
*/
static char* generator_subexp(
  expression* exp,         /*!< Pointer to the current expression */
  func_unit*  funit,       /*!< Pointer to the functional unit that exp exists in */
  bool        net,         /*!< If TRUE, specifies that we are generating for a net */
  bool        reg_needed,  /*!< If TRUE, instantiates needed registers */
  bool        replace_exp  /*!< If TRUE, replaces the actual logic with this subexpression */
) { PROFILE(GENERATOR_INSERT_SUBEXP);

  char*        lhs_str  = NULL;
  char*        val_str;
  char*        str      = NULL;
  unsigned int slen;
  unsigned int rv;
  unsigned int i;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  char*        cov_str  = NULL;

  /* Create LHS portion of assignment */
  lhs_str = generator_create_lhs( exp, funit, net, reg_needed );

  /* Generate value string */
  if( EXPR_IS_OP_AND_ASSIGN( exp ) == 1 ) {
    char  op_str[4];
    char* lval_str = codegen_gen_expr_one_line( exp->left,  funit, !generator_expr_needs_to_be_substituted( exp->left ) );
    char* rval_str = codegen_gen_expr_one_line( exp->right, funit, !generator_expr_needs_to_be_substituted( exp->right ) );

    switch( exp->op ) {
      case EXP_OP_MLT_A :  strcpy( op_str, "*"  );   break;
      case EXP_OP_DIV_A :  strcpy( op_str, "/"  );   break;
      case EXP_OP_MOD_A :  strcpy( op_str, "%"  );   break;
      case EXP_OP_LS_A  :  strcpy( op_str, "<<" );   break;
      case EXP_OP_RS_A  :  strcpy( op_str, ">>" );   break;
      case EXP_OP_ALS_A :  strcpy( op_str, "<<<" );  break;
      case EXP_OP_ARS_A :  strcpy( op_str, ">>>" );  break;
      default :
        assert( 0 );
        break;
    }

    /* Construct the string */
    slen    = 1 + strlen( lval_str ) + 2 + strlen( op_str ) + 2 + strlen( rval_str ) + 2;
    val_str = (char*)malloc_safe( slen );
    rv      = snprintf( val_str, slen, "(%s) %s (%s)", lval_str, op_str, rval_str );
    assert( rv < slen );

    free_safe( lval_str, (strlen( lval_str ) + 1) );
    free_safe( rval_str, (strlen( rval_str ) + 1) );

  /* Otherwise, the value string is just the expression itself */
  } else {
    val_str = codegen_gen_expr_one_line( exp, funit, !generator_expr_needs_to_be_substituted( exp ) );

  }

  /* If this expression needs to be substituted, do it with the lhs_str value */
  if( replace_exp ) {
    generator_replace( lhs_str, exp->ppfline, exp->col.part.first, exp->pplline, exp->col.part.last );
  }

  /* Create expression string */
  if( net ) {
    slen = 8 + strlen( lhs_str ) + 3 + strlen( val_str ) + 2;
    str  = (char*)malloc_safe_nolimit( slen );
    rv   = snprintf( str, slen, " assign %s = %s;", lhs_str, val_str );
    assert( rv < slen );
    generator_insert_reg( str, FALSE );
    free_safe( str, slen );
  } else {
    slen = strlen( lhs_str ) + 3 + strlen( val_str ) + 2;
    str  = (char*)malloc_safe_nolimit( slen );
    rv   = snprintf( str, slen, "%s = %s;", lhs_str, val_str );
    assert( rv < slen );
    cov_str = generator_build( 2, str, "\n" );
  }

  /* Deallocate memory */
  free_safe( lhs_str, (strlen( lhs_str ) + 1) );
  free_safe( val_str, (strlen( val_str ) + 1) );

  /* Specify that this expression has an intermediate value assigned to it */
  exp->suppl.part.comb_cntd = 1;

  PROFILE_END;

  return( cov_str );

}

/*!
 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static char* generator_comb_cov_helper2(
  expression*  exp,           /*!< Pointer to expression tree to operate on */
  func_unit*   funit,         /*!< Pointer to current functional unit */
  exp_op_type  parent_op,     /*!< Parent expression operation (originally should be set to the same operation as exp) */
  int          parent_depth,  /*!< Current expression depth (originally set to 0) */
  bool         force_subexp,  /*!< Set to TRUE if a expression subexpression is required needed (originally set to FALSE) */
  bool         net,           /*!< If set to TRUE generate code for a net */
  bool         root,          /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed,    /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
  bool         replace_exp    /*!< If set to TRUE, will allow this expression to replace the original */
) { PROFILE(GENERATOR_COMB_COV_HELPER2);

  char* cov_str = NULL;

  if( exp != NULL ) {

    int  depth             = parent_depth + ((exp->op != parent_op) ? 1 : 0);
    bool expr_cov_needed   = generator_expr_cov_needed( exp, depth );
    bool child_replace_exp = replace_exp &&
                             !(force_subexp ||
                               generator_expr_needs_to_be_substituted( exp ) ||
                               (EXPR_IS_COMB( exp ) && !root && expr_cov_needed) ||
                               (!EXPR_IS_EVENT( exp ) && !EXPR_IS_COMB( exp ) && expr_cov_needed));
    
    /* Generate children expression trees (depth first search) */
    cov_str = generator_build( 2,
                               generator_comb_cov_helper2( exp->left,  funit, exp->op, depth, (expr_cov_needed & EXPR_IS_COMB( exp )), net, FALSE, reg_needed, (child_replace_exp && !EXPR_IS_OP_AND_ASSIGN( exp )) ),
                               generator_comb_cov_helper2( exp->right, funit, exp->op, depth, (expr_cov_needed & EXPR_IS_COMB( exp )), net, FALSE, reg_needed, child_replace_exp ) );

    /* Generate event combinational logic type */
    if( EXPR_IS_EVENT( exp ) ) {
      if( info_suppl.part.scored_events == 1 ) {
        if( expr_cov_needed ) {
          cov_str = generator_build( 2, cov_str, generator_event_comb_cov( exp, funit, reg_needed ) );
        }
        if( force_subexp || generator_expr_needs_to_be_substituted( exp ) ) {
          cov_str = generator_build( 2, cov_str, generator_subexp( exp, funit, net, reg_needed, (replace_exp && !EXPR_IS_OP_AND_ASSIGN( exp )) ) );
        }
      }

    /* Otherwise, generate binary combinational logic type */
    } else if( EXPR_IS_COMB( exp ) ) {
      if( info_suppl.part.scored_comb == 1 ) {
        if( !root && (expr_cov_needed || force_subexp || generator_expr_needs_to_be_substituted( exp )) ) {
          cov_str = generator_build( 2, cov_str, generator_subexp( exp, funit, net, reg_needed, replace_exp ) );
        }
        if( expr_cov_needed ) {
          cov_str = generator_build( 2, cov_str, generator_comb_comb_cov( exp, funit, net, reg_needed ) );
        }
      }

    /* Generate unary combinational logic type */
    } else {
      if( info_suppl.part.scored_comb == 1 ) {
        if( expr_cov_needed || force_subexp || generator_expr_needs_to_be_substituted( exp ) ) {
          cov_str = generator_build( 2, cov_str, generator_subexp( exp, funit, net, reg_needed, (replace_exp && !EXPR_IS_OP_AND_ASSIGN( exp )) ) );
        }
        if( expr_cov_needed ) {
          cov_str = generator_build( 2, cov_str, generator_unary_comb_cov( exp, funit, net, reg_needed ) );
        }
      }

    }

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns combinational coverage string.

 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static char* generator_comb_cov_helper(
  expression*  exp,        /*!< Pointer to expression tree to operate on */
  func_unit*   funit,      /*!< Pointer to current functional unit */
  exp_op_type  parent_op,  /*!< Parent expression operation (originally should be set to the same operation as exp) */
  bool         net,        /*!< If set to TRUE generate code for a net */
  bool         root,       /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed  /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER);

  /* Generate the code */
  char* cov_str = generator_comb_cov_helper2( exp, funit, parent_op, 0, FALSE, net, root, reg_needed, ((ESUPPL_IS_ROOT( exp->suppl ) == 1) || !EXPR_IS_EVENT( exp->parent->expr )) );

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns a string containing the memory index value to use for memory write/read coverage.

 Generates a memory index value for a given memory expression.
*/
static char* generator_gen_mem_index_helper(
  expression* exp,        /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,      /*!< Pointer to functional unit containing exp */
  int         dimension,  /*!< Current memory dimension (should be initially set to expression_get_curr_dimension( exp ) */
  char*       ldim_width  /*!< Bit width of the lower dimension in string form (should be NULL in first call) */
) { PROFILE(GENERATOR_GEN_MEM_INDEX_HELPER);

  char*        index;
  char*        str;
  char*        num;
  unsigned int slen;
  unsigned int rv;
  int          number;

  /* Calculate the index value */
  switch( exp->op ) {
    case EXP_OP_SBIT_SEL :
      index = codegen_gen_expr_one_line( exp->left, funit, FALSE );
      break;
    case EXP_OP_MBIT_SEL :
      {
        char* lstr = codegen_gen_expr_one_line( exp->left,  funit, FALSE );
        char* rstr = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        slen  = (strlen( lstr ) * 3) + (strlen( rstr ) * 3) + 24;
        index = (char*)malloc_safe( slen );
        rv    = snprintf( index, slen, "((%s)>(%s))?((%s)-(%s)):((%s)-(%s))", lstr, rstr, lstr, rstr, rstr, lstr );
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
        slen  = 2 + strlen( lstr ) + 3 + strlen( rstr ) + 5;
        index = (char*)malloc_safe( slen );
        rv    = snprintf( index, slen, "((%s)-(%s))+1", lstr, rstr );
        assert( rv < slen );
        free_safe( lstr, (strlen( lstr ) + 1) );
        free_safe( rstr, (strlen( rstr ) + 1) );
      }
      break;
    default :
      assert( 0 );
      break;
  }

  /* Get the LSB of the current dimension */
  num = mod_parm_gen_lsb_code( exp->sig, dimension, funit_get_curr_module( funit ), &number );

  /* Adjust the index to get the true index */
  {
    char* tmp_index = index;
    if( num == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", number );
      assert( rv < 50 );
      slen  = 1 + strlen( tmp_index ) + 3 + strlen( tmp ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s)-(%s)", tmp_index, tmp );
      assert( rv < slen );
    } else {
      slen  = 1 + strlen( tmp_index ) + 3 + strlen( num ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s)-(%s)", tmp_index, num );
      assert( rv < slen );
      free_safe( num, (strlen( num ) + 1) );
    }
    free_safe( tmp_index, (strlen( tmp_index ) + 1) );
  }

  /* Get the dimensional width for the current expression */
  num = mod_parm_gen_size_code( exp->sig, dimension, funit_get_curr_module( funit ), &number );

  /* If the current dimension is big endian, recalculate the index value */
  if( exp->elem.dim->dim_be ) {
    char* tmp_index = index;
    if( num == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      slen  = 1 + strlen( tmp ) + 3 + strlen( tmp_index ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s)-(%s)", tmp, tmp_index );
    } else {
      slen  = 2 + strlen( num ) + 5 + strlen( index ) + 1;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "((%s)-1)-%s", num, tmp_index );
    }
    assert( rv < slen );
    free_safe( tmp_index, (strlen( tmp_index ) + 1) );
  }

  /* Create the full string for this dimension */
  if( ldim_width != NULL ) {
    slen = 1 + strlen( index ) + 3 + strlen( ldim_width ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "(%s)*(%s)", index, ldim_width );
    assert( rv < slen );
  } else {
    str = strdup_safe( index );
  }

  if( dimension != 0 ) {

    char* width;

    /* Create the width of this dimension */
    if( num == NULL ) {
      num = convert_int_to_str( number );
    }

    if( ldim_width != NULL ) {
      slen  = 1 + strlen( ldim_width ) + 3 + strlen( num ) + 2;
      width = (char*)malloc_safe( slen );
      rv    = snprintf( width, slen, "(%s)*(%s)", ldim_width, num );
      assert( rv < slen );
    } else {
      width = strdup_safe( num );
    }

    /* Adding our generated value to the other dimensional information */
    {
      char* tmpstr = str;
      char* rest   = generator_gen_mem_index_helper( ((dimension == 1) ? exp->parent->expr->left : exp->parent->expr->left->right), funit, (dimension - 1), width );

      slen = 1 + strlen( tmpstr ) + 3 + strlen( rest ) + 2;
      str  = (char*)malloc_safe( slen );
      rv   = snprintf( str, slen, "(%s)+(%s)", tmpstr, rest );
      assert( rv < slen ); 

      free_safe( rest,   (strlen( rest )   + 1) );
      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
    }

    /* Deallocate memory */
    free_safe( width, (strlen( width ) + 1) );

  }

  /* Deallocate memory */
  free_safe( num,   (strlen( num )   + 1) );
  free_safe( index, (strlen( index ) + 1) );

  PROFILE_END;

  return( str );

}

/*!
 \return Returns a string containing the memory index value to use for memory write/read coverage.

 Generates a memory index value for a given memory expression.
*/
static char* generator_gen_mem_index(
  expression* exp,       /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,     /*!< Pointer to functional unit containing exp */
  int         dimension  /*!< Current memory dimension (should be initially set to expression_get_curr_dimension( exp ) */
) { PROFILE(GENERATOR_GEN_MEM_INDEX);

  char* ldim_width = NULL;
  char* str;

  /* If the dimension has not been set and its not the last dimension, calculate a last dimension value */
  if( dimension < ((exp->sig->udim_num + exp->sig->pdim_num) - 1) ) {
    int          number;
    int          slen;
    unsigned int rv;
    int          dim = (exp->sig->udim_num + exp->sig->pdim_num) - 1;
    char*        num = mod_parm_gen_size_code( exp->sig, dim--, funit_get_curr_module( funit ), &number );
    if( num == NULL ) {
      ldim_width = convert_int_to_str( number );
    } else {
      ldim_width = num;
    }
    for( ; dim>dimension; dim-- ) {
      char* tmp_str = ldim_width;
      num        = mod_parm_gen_size_code( exp->sig, dim, funit_get_curr_module( funit ), &number );
      if( num == NULL ) {
        num = convert_int_to_str( number );
      }
      slen       = 1 + strlen( tmp_str ) + 3 + strlen( num ) + 2;
      ldim_width = (char*)malloc_safe( slen );
      rv         = snprintf( ldim_width, slen, "(%s)*(%s)", tmp_str, num );
      assert( rv < slen );
      free_safe( tmp_str, (strlen( tmp_str ) + 1) );
      free_safe( num,     (strlen( num )     + 1) );
    }
  }

  /* Call the helper function to calculate the string */
  str = generator_gen_mem_index_helper( exp, funit, dimension, ldim_width );

  /* Deallocate memory */
  free_safe( ldim_width, (strlen( ldim_width ) + 1) );

  PROFILE_END;

  return( str );

}

#ifdef OBSOLETE
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
      slen += strlen( tmp ) + 6;
      size  = (char*)malloc_safe( slen );
      rv    = snprintf( size, slen, "(%s)*(%s)", tmpsize, tmp );
    } else {
      slen += strlen( curr_size ) + 6;
      size  = (char*)malloc_safe( slen );
      rv    = snprintf( size, slen, "(%s)*(%s)", tmpsize, curr_size );
    }
    assert( rv < slen );

    free_safe( curr_size, (strlen( curr_size ) + 1) );
    free_safe( tmpsize,   (strlen( tmpsize ) + 1) );

  }

  PROFILE_END;

  return( size );

}
#endif

/*!
 \return Returns a string containing the LSB of the RHS to use to assign to this LHS expression.
*/
static char* generator_get_lhs_lsb_helper(
  expression* exp,   /*!< Pointer to LHS expression to get LSB information for */
  func_unit*  funit  /*!< Functional unit containing the given expression */
) { PROFILE(GENERATOR_GET_LHS_LSB_HELPER);

  char* lsb;

  if( exp != NULL ) {

    char*        right;
    char*        size;
    int          number;
    unsigned int rv;
    unsigned int slen;

    /* Get the LSB information for the right expression */
    if( (ESUPPL_IS_ROOT( exp->parent->expr->parent->expr->suppl ) == 0) && (exp->parent->expr->parent->expr->op != EXP_OP_CONCAT) ) {
      right = generator_get_lhs_lsb_helper( exp->parent->expr->parent->expr->right, funit );
    } else {
      right = strdup_safe( "0" );
    }

    /* Calculate our width */
    size = generator_gen_size( exp, funit, &number );

    /* Add our size to the size of the right expression */
    if( number >= 0 ) {
      char num[50];
      rv = snprintf( num, 50, "%d", number );
      assert( rv < 50 );
      slen = 1 + strlen( num ) + 3 + strlen( right ) + 2;
      lsb  = (char*)malloc_safe( slen );
      rv   = snprintf( lsb, slen, "(%s)+(%s)", num, right );
      assert( rv < slen );
    } else {
      slen = 1 + strlen( size ) + 3 + strlen( right ) + 2;
      lsb  = (char*)malloc_safe( slen );
      rv   = snprintf( lsb, slen, "(%s)+(%s)", size, right );
      assert( rv < slen );
    }

    free_safe( size,  (strlen( size ) + 1) );
    free_safe( right, (strlen( right ) + 1) );

  } else {

    lsb = strdup_safe( "0" );

  }

  PROFILE_END;

  return( lsb );

}

/*!
 \return Returns a string containing the LSB of the RHS to use to assign to this LHS expression.
*/
static char* generator_get_lhs_lsb(
  expression* exp,   /*!< Pointer to LHS expression to get LSB information for */
  func_unit*  funit  /*!< Pointer to functional unit containing this expression */
) { PROFILE(GENERATOR_GET_LHS_LSB);

  char* lsb;

  if( (exp != NULL) && (ESUPPL_IS_ROOT( exp->parent->expr->suppl ) == 0) && (exp->parent->expr->op != EXP_OP_NASSIGN) ) {

    if( exp->parent->expr->left == exp ) {
      lsb = generator_get_lhs_lsb_helper( exp->parent->expr->right, funit );
    } else if( exp->parent->expr->parent->expr->op != EXP_OP_CONCAT ) {
      lsb = generator_get_lhs_lsb_helper( exp->parent->expr->parent->expr->right, funit );
    } else {
      lsb = strdup_safe( "0" );
    }

  } else {
    
    lsb = strdup_safe( "0" );

  }

  PROFILE_END;

  return( lsb );

}

/*!
 \return Returns string containing memory coverage information.

 Inserts memory coverage for the given expression.
*/
static char* generator_mem_cov(
  expression* exp,    /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,  /*!< Pointer to functional unit containing the given expression */
  bool        net,    /*!< If TRUE, creates the signal name for a net; otherwise, creates the signal name for a register */
  bool        write,  /*!< If TRUE, creates write logic; otherwise, creates read logic */
  expression* rhs     /*!< If the root expression is a non-blocking assignment, this pointer will point to the RHS
                           expression that is required to extract memory coverage.  If this pointer is NULL, handle
                           memory coverage normally. */
) { PROFILE(GENERATOR_MEM_COV);

  char         name[4096];
  char         range[4096];
  unsigned int rv;
  char*        idxstr  = generator_gen_mem_index( exp, funit, expression_get_curr_dimension( exp ) );
  char*        value;
  char*        str;
  char         num[50];
  char*        scope   = generator_get_relative_scope( funit );
  char*        cov_str = NULL;

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
      rv = snprintf( iname, 4096, " \\covered$%c%u_%u_%x$%s ", (net ? 'i' : 'I'), exp->ppfline, exp->pplline, exp->col.all, exp->name );
    } else {
      rv = snprintf( iname, 4096, " \\covered$%c%u_%u_%x$%s$%s ", (net ? 'i' : 'I'), exp->ppfline, exp->pplline, exp->col.all, exp->name, scope );
    }
    assert( rv < 4096 );

    if( net ) {

      unsigned int slen = 7 + strlen( num ) + 7 + strlen( name ) + 3 + strlen( idxstr ) + 2;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "wire [(%s)-1:0] %s = %s;", num, iname, idxstr );
      assert( rv < slen );

      generator_insert_reg( str, FALSE );

    } else {

      unsigned int slen = 6 + strlen( num ) + 7 + strlen( iname ) + 3;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "reg [(%s)-1:0] %s;", num, iname );
      assert( rv < slen );
      generator_insert_reg( str, FALSE );
      free_safe( str, (strlen( str ) + 1) );

      slen = 1 + strlen( iname ) + 3 + strlen( idxstr ) + 2;
      str  = (char*)malloc_safe( slen );
      rv   = snprintf( str, slen, " %s = %s;", iname, idxstr );
      assert( rv < slen );

      cov_str = str;

    }

    /* Generate size needed to store memory element */
    size = generator_gen_size( exp, funit, &number );

    /*
     If the rhs expression is not NULL, we are within a non-blocking assignment so calculate the value that will be stored
     in the memory element.
    */
    if( rhs != NULL ) {

      char* ename = generator_create_expr_name( rhs );
      char  rhs_reg[4096];
      char* rhs_str;
      char* lsb_str;
      char* msb_str;

      /*
       We are reusing the comb_cntd bit in the expression supplemental field to indicate that this expression
       has or has not been created.
      */
      if( rhs->suppl.part.eval_t == 0 ) {

        char* size;
        int   number;

        /* Generate size needed to store memory element */
        size = generator_gen_size( rhs, funit, &number );

        if( number >= 0 ) {
          rv = snprintf( rhs_reg, 4096, "reg [%d:0] %s;", (number - 1), ename );
        } else {
          rv = snprintf( rhs_reg, 4096, "reg [(%s)-1:0] %s;", size, ename );
        }
        assert( rv < 4096 );
        generator_insert_reg( rhs_reg, TRUE );

        /* Add the expression that will be assigned to the memory element */
        rhs_str = codegen_gen_expr_one_line( rhs, funit, FALSE );
        vlen    = strlen( ename ) + 3 + strlen( rhs_str ) + 2;
        value   = (char*)malloc_safe( vlen );
        rv      = snprintf( value, vlen, "%s = %s;", ename, rhs_str );
        assert( rv < vlen );
        free_safe( rhs_str, (strlen( rhs_str ) + 1) );

        /* Prepend the expression */
        cov_str = generator_build( 2, cov_str, value );

        /* Specify that the expression has been placed */
        rhs->suppl.part.eval_t = 1;

        free_safe( size, (strlen( size ) + 1) );

      }

      /* Generate the LSB of the RHS expression that needs to be assigned to this memory element */
      lsb_str = generator_get_lhs_lsb( exp, funit );

      /* Generate the MSB of the RHS expression that needs to be assigned to this memory element */
      if( number >= 0 ) {
        char num[50];
        rv = snprintf( num, 50, "%d", number );
        assert( rv < 50 );
        vlen    = 2 + strlen( num ) + 6 + strlen( lsb_str ) + 2;
        msb_str = (char*)malloc_safe( vlen );
        rv      = snprintf( msb_str, vlen, "((%s)-1)+(%s)", num, lsb_str );
        assert( rv < vlen );
      } else {
        vlen    = 2 + strlen( size ) + 6 + strlen( lsb_str ) + 2;
        msb_str = (char*)malloc_safe( vlen );
        rv      = snprintf( msb_str, vlen, "((%s)-1)+(%s)", size, lsb_str );
        assert( rv < vlen );
      }

      /* Generate the part select of the RHS expression to assign to this memory element */
      vlen   = strlen( ename ) + 1 + strlen( msb_str ) + 1 + strlen( lsb_str ) + 2;
      memstr = (char*)malloc_safe( vlen );
      rv     = snprintf( memstr, vlen, "%s[%s:%s]", ename, msb_str, lsb_str );

      free_safe( lsb_str, (strlen( lsb_str ) + 1) );
      free_safe( msb_str, (strlen( msb_str ) + 1) );
      free_safe( ename,   (strlen( ename )   + 1) );

    } else {

      memstr = codegen_gen_expr_one_line( first_exp, funit, FALSE );

    }

    /* Create name */
    if( scope[0] == '\0' ) {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s ", (net ? 'w' : 'W'), exp->ppfline, exp->pplline, exp->col.all, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s$%s ", (net ? 'w' : 'W'), exp->ppfline, exp->pplline, exp->col.all, exp->name, scope );
    }
    assert( rv < 4096 );

    /* Create the range information for the write */
    if( number >= 0 ) {
      rv = snprintf( range, 4096, "[%d+((%s)-1):0]", number, num );
    } else {
      rv = snprintf( range, 4096, "[(%s)+((%s)-1):0]", size, num );
    }
    assert( rv < 4096 );

    /* Create the value to assign */
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
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s ", (net ? 'r' : 'R'), exp->ppfline, exp->pplline, exp->col.all, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s$%s ", (net ? 'r' : 'R'), exp->ppfline, exp->pplline, exp->col.all, exp->name, scope );
    }
    assert( rv < 4096 );

    /* Create the range information for the read */
    rv = snprintf( range, 4096, "[(%s)-1:0]", num );
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

    generator_insert_reg( str, FALSE );

  /* Otherwise, create the assignment string for a register and create the register */
  } else {

    unsigned int slen = 3 + 1 + strlen( range ) + 1 + strlen( name ) + 3;

    str = (char*)malloc_safe( slen );
    rv  = snprintf( str, slen, "reg %s %s;", range, name );
    assert( rv < slen );

    /* Add the register to the register list */
    generator_insert_reg( str, FALSE );
    free_safe( str, (strlen( str ) + 1) );

    /* Now create the assignment string */
    slen = 1 + strlen( name ) + 3 + strlen( value ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, " %s = %s;", name, value );
    assert( rv < slen );

    /* Write coverage should append to the working buffer */
    cov_str = generator_build( 3, cov_str, strdup_safe( str ), "\n" );
#ifdef OBSOLETE
    if( write ) {
      generator_add_cov_to_work_code( str );
    } else {
      (void)str_link_add( strdup_safe( str ), &work_head, &work_tail );
    }
#endif

  }

  /* Deallocate temporary memory */
  free_safe( idxstr, (strlen( idxstr ) + 1) );
  free_safe( value,  (strlen( value )  + 1) );
  free_safe( str,    (strlen( str )    + 1) );
  free_safe( scope,  (strlen( scope )  + 1) );

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns string containing memory coverage information.

 Traverses the specified expression tree searching for memory accesses.  If any are found, the appropriate
 coverage code is inserted into the output buffers.
*/
static char* generator_mem_cov_helper(
  expression* exp,           /*!< Pointer to current expression */
  func_unit*  funit,         /*!< Pointer to functional unit containing the expression */
  bool        net,           /*!< Specifies if the code generator should produce code for a net or a register */
  bool        do_read,       /*!< If TRUE, performs memory read access for any memories found in the expression tree (by default, set it to FALSE) */
  bool        do_write,      /*!< If TRUE, performs memory write access for any memories found in the expression tree (by default, set it to FALSE) */
  expression* rhs            /*!< Set to the RHS expression if the root expression was a non-blocking assignment */
) { PROFILE(GENERATOR_MEM_COV_HELPER);

  char* cov_str = NULL;

  if( exp != NULL ) {

    /* Generate code to perform memory write/read access */
    if( (exp->sig != NULL) && (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM) && (exp->elem.dim != NULL) && exp->elem.dim->last ) {
      if( ((exp->suppl.part.lhs == 1) || do_write) && !do_read ) {
        cov_str = generator_mem_cov( exp, funit, net, TRUE, rhs );
      }
      if( (exp->suppl.part.lhs == 0) || do_read ) {
        cov_str = generator_build( 2, cov_str, generator_mem_cov( exp, funit, net, FALSE, rhs ) );
      }
    }

    /* Get memory coverage for left and right expressions */
    cov_str = generator_build( 3, cov_str,
                               generator_mem_cov_helper( exp->left,
                                     funit,
                                     net,
                                     ((exp->op == EXP_OP_SBIT_SEL) || (exp->op == EXP_OP_MBIT_SEL) || (exp->op == EXP_OP_MBIT_POS) ||
                                      (exp->op == EXP_OP_MBIT_NEG) || do_read),
                                     FALSE,
                                     rhs ),
                               generator_mem_cov_helper( exp->right,
                                     funit,
                                     net,
                                     ((exp->op == EXP_OP_MBIT_SEL) || do_read),
                                     ((exp->op == EXP_OP_SASSIGN) && (exp->parent->expr != NULL) && ((exp->parent->expr->op == EXP_OP_SRANDOM) || (exp->parent->expr->op == EXP_OP_SURANDOM))),
                                     rhs ) );

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns string containing combinational coverage.

 Insert combinational logic coverage code for the given expression (by file position).
*/
char* generator_comb_cov(
  unsigned int first_line,    /*!< First line of expression to generate for */
  unsigned int first_column,  /*!< First column of expression to generate for */
  bool         net,           /*!< If set to TRUE, generate code for a net; otherwise, generate code for a variable */
  bool         use_right,     /*!< If set to TRUE, use right-hand expression */
  bool         save_stmt,     /*!< If set to TRUE, saves the found statement to the statement stack */
  bool         reg_needed     /*!< If set to TRUE, creates registers for temporary variables */
) { PROFILE(GENERATOR_COMB_COV);

  statement* stmt    = NULL;
  char*      cov_str = NULL;

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_memory == 1) || (info_suppl.part.scored_events == 1)) &&
      !handle_funit_as_assert && ((stmt = generator_find_statement( first_line, first_column )) != NULL) && !generator_is_static_function_only( stmt->funit ) ) {

    /* Generate combinational coverage */
    if( (info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_events) ) {
      cov_str = generator_comb_cov_helper( (use_right ? stmt->exp->right : stmt->exp), stmt->funit, (use_right ? stmt->exp->right->op : stmt->exp->op), net, TRUE, reg_needed );
    }

    /* Generate memory coverage */
    if( info_suppl.part.scored_memory == 1 ) {
      cov_str = generator_build( 2, cov_str, generator_mem_cov_helper( stmt->exp, stmt->funit, net, FALSE, FALSE, ((stmt->exp->op == EXP_OP_NASSIGN) ? stmt->exp->right : NULL) ) );
    }

    /* Clear the comb_cntd bits in the expression tree */
    generator_clear_comb_cntd( stmt->exp );

  }

  /* If we need to save the found statement, do so now */
  if( save_stmt ) {

    stmt_loop_link* new_stmtl;

    assert( stmt != NULL );

    new_stmtl       = (stmt_loop_link*)malloc_safe( sizeof( stmt_loop_link ) );
    new_stmtl->stmt = stmt;
    new_stmtl->next = stmt_stack;
    new_stmtl->type = use_right ? 0 : 1;
    stmt_stack      = new_stmtl;

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns string containing combinational coverage.

 Inserts combinational coverage information from statement stack (and pop stack).
*/
char* generator_comb_cov_from_stmt_stack() { PROFILE(GENERATOR_INSERT_COMB_COV_FROM_STMT_STACK);

  statement* stmt    = NULL;
  char*      cov_str = NULL;

  if( ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_events == 1)) && !handle_funit_as_assert ) {

    stmt_loop_link* sll;
    expression*     exp;

    assert( stmt_stack != NULL );

    stmt = stmt_stack->stmt;
    sll  = stmt_stack;
    exp  = stmt_stack->type ? stmt->exp->right : stmt->exp;

    /* Generate combinational coverage information */
    if( !generator_is_static_function_only( stmt->funit ) ) {
      cov_str = generator_comb_cov_helper( exp, stmt->funit, exp->op, FALSE, TRUE, FALSE );
    }

    /* Clear the comb_cntd bits in the expression tree */
    generator_clear_comb_cntd( exp );

    /* Now pop the statement stack */
    stmt_stack = sll->next;
    free_safe( sll, sizeof( stmt_loop_link ) );

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns string containing combinational coverage information.

 Inserts combinational coverage information for the given statement.
*/
char* generator_comb_cov_with_stmt(
  statement* stmt,       /*!< Pointer to statement to generate combinational logic coverage for */
  bool       use_right,  /*!< Specifies if the right expression should be used in the statement */
  bool       reg_needed  /*!< If TRUE, instantiate necessary registers */
) { PROFILE(GENERATOR_INSERT_COMB_COV_WITH_STMT);

  char* cov_str = NULL;

  if( ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_events == 1)) &&
      !handle_funit_as_assert && (stmt != NULL) && !generator_is_static_function_only( stmt->funit ) ) {

    expression* exp = use_right ? stmt->exp->right : stmt->exp;

    /* Insert combinational coverage */
    cov_str = generator_comb_cov_helper( exp, stmt->funit, exp->op, FALSE, TRUE, reg_needed );

    /* Clear the comb_cntd bits in the expression tree */
    generator_clear_comb_cntd( exp );

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns combinational logic string.

 Handles combinational logic for an entire case block (and its case items -- not the case item logic blocks).
*/
char* generator_case_comb_cov(
  unsigned int first_line,   /*!< First line number of first statement in case block */
  unsigned int first_column  /*!< First column of first statement in case block */
) { PROFILE(GENERATOR_INSERT_CASE_COMB_COV);

  statement* stmt;
  char*      cov_str = NULL;

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_events == 1)) &&
      !handle_funit_as_assert && ((stmt = generator_find_case_statement( first_line, first_column )) != NULL) &&
      !generator_is_static_function_only( stmt->funit ) ) {

    cov_str = generator_comb_cov_helper( stmt->exp->left, stmt->funit, stmt->exp->left->op, FALSE, TRUE, TRUE );

    /* Clear the comb_cntd bits in the expression tree */
    generator_clear_comb_cntd( stmt->exp->left );

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 Inserts FSM coverage at the end of the module for the current module.
*/
char* generator_fsm_covs() { PROFILE(GENERATOR_FSM_COVS);

  char*        cov_str = NULL;
  int          slen;
  unsigned int rv;

  if( (info_suppl.part.scored_fsm == 1) && !handle_funit_as_assert && !generator_is_static_function_only( curr_funit ) ) {

    unsigned int i;

    for( i=0; i<curr_funit->fsm_size; i++ ) {

      fsm*  table    = curr_funit->fsms[i];
      char  idxstr[30];
      char  numstr[30];
      char* tcov_str = cov_str;

      /* Get the string value of the index */
      rv = snprintf( idxstr, 30, "%d", (i + 1) );
      assert( rv < 30 );

      if( table->from_state->id == table->to_state->id ) {

        int   number;
        char* size = generator_gen_size( table->from_state, curr_funit, &number );
        char* exp  = codegen_gen_expr_one_line( table->from_state, curr_funit, FALSE );
        if( number >= 0 ) {
          rv      = snprintf( numstr, 30, "%d", (number - 1) );  assert( rv < 30 );
          slen    = 6 + strlen( numstr ) + 14 + strlen( idxstr ) + 3 + strlen( exp ) + 2;
          cov_str = (char*)malloc_safe( slen );
          rv      = snprintf( cov_str, slen, "wire [%d:0] \\covered$F%u = %s;", (number - 1), (i + 1), exp );
          assert( rv < slen );
        } else {
          slen    = 7 + strlen( (size != NULL) ? size : "1" ) + 17 + strlen( idxstr ) + 3 + strlen( exp ) + 2;
          cov_str = (char*)malloc_safe( slen );
          rv      = snprintf( cov_str, slen, "wire [(%s)-1:0] \\covered$F%u = %s;", ((size != NULL) ? size : "1"), (i + 1), exp );
          assert( rv < slen );
        }
        free_safe( size, (strlen( size ) + 1) );
        free_safe( exp, (strlen( exp ) + 1) );

      } else {

        int   from_number;
        int   to_number;
        char* fsize = generator_gen_size( table->from_state, curr_funit, &from_number );
        char* fexp  = codegen_gen_expr_one_line( table->from_state, curr_funit, FALSE );
        char* tsize = generator_gen_size( table->to_state, curr_funit, &to_number );
        char* texp  = codegen_gen_expr_one_line( table->to_state, curr_funit, FALSE );
        if( from_number >= 0 ) {
          if( to_number >= 0 ) {
            rv      = snprintf( numstr, 30, "%d", ((from_number + to_number) - 1) );  assert( rv < 30 );
            slen    = 6 + strlen( numstr ) + 14 + strlen( idxstr ) + 4 + strlen( fexp ) + 1 + strlen( texp ) + 3;
            cov_str = (char*)malloc_safe( slen );
            rv      = snprintf( cov_str, slen, "wire [%d:0] \\covered$F%u = {%s,%s};", ((from_number + to_number) - 1), (i + 1), fexp, texp );
            printf( "cov_str: %p, %s\n", cov_str, cov_str );
          } else {
            rv      = snprintf( numstr, 30, "%d", from_number );  assert( rv < 30 );
            slen    = 7 + strlen( numstr ) + 2 + strlen( (tsize != NULL) ? tsize : "1" ) + 18 + strlen( idxstr ) + 4 + strlen( fexp ) + 1 + strlen( texp ) + 3;
            cov_str = (char*)malloc_safe( slen );
            rv      = snprintf( cov_str, slen, "wire [(%d+(%s))-1:0] \\covered$F%u = {%s,%s};", from_number, ((tsize != NULL) ? tsize : "1"), (i + 1), fexp, texp );
          }
        } else {
          if( to_number >= 0 ) {
            rv      = snprintf( numstr, 30, "%d", to_number );  assert( rv < 30 );
            slen    = 8 + strlen( (fsize != NULL) ? fsize : "1" ) + 2 + strlen( numstr ) + 17 + strlen( idxstr ) + 4 + strlen( fexp ) + 1 + strlen( texp ) + 3;
            cov_str = (char*)malloc_safe( slen );
            rv      = snprintf( cov_str, slen, "wire [((%s)+%d)-1:0] \\covered$F%u = {%s,%s};", ((fsize != NULL) ? fsize : "1"), to_number, (i + 1), fexp, texp );
          } else {
            slen    = 8 + strlen( (fsize != NULL) ? fsize : "1" ) + 3 + strlen( (tsize != NULL) ? tsize : "1" ) + 18 + strlen( idxstr ) + 4 + strlen( fexp ) + 1 + strlen( texp ) + 3;
            cov_str = (char*)malloc_safe( slen );
            rv      = snprintf( cov_str, slen, "wire [((%s)+(%s))-1:0] \\covered$F%u = {%s,%s};",
                                ((fsize != NULL) ? fsize : "1"), ((tsize != NULL) ? tsize : "1"), (i + 1), fexp, texp );
          }
        }
        free_safe( fsize, (strlen( fsize ) + 1) );
        free_safe( fexp,  (strlen( fexp )  + 1) );
        free_safe( tsize, (strlen( tsize ) + 1) );
        free_safe( texp,  (strlen( texp )  + 1) );

      }

      /* Concatenate strings */
      cov_str = generator_build( 2, tcov_str, cov_str );

    }

  }

  PROFILE_END;

  return( cov_str );

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

/*!
 Removes the last token from the working buffer and holds onto it, adding it back when the next token is parsed.
*/
void generator_hold_last_token() { PROFILE(GENERATOR_HOLD_LAST_TOKEN);

  /* Find the last token and store it into the look-ahead buffer */
  if( strlen( work_buffer ) > 0 ) {

    /* Skip whitespace */
    char* ptr = work_buffer + last_token_index;
    // while( (*ptr != '\0') && ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\r') || (*ptr == '\n')) ) ptr++;

    if( *ptr != '\0' ) {
      strcpy( lahead_buffer, ptr );
      work_buffer[last_token_index] = '\0';
      replace_last.word_ptr = work_buffer + (last_token_index - 1);
    }

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Holding last token [%s]", lahead_buffer );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

  }

  PROFILE_END;

}

/*!
 Flushes the held token (if a token is being held).
*/
void generator_flush_held_token1(
  /*@unused@*/ const char* file,  /*!< File name that called this function */
  /*@unused@*/ int         line   /*!< Line number of code that called this function */
) { PROFILE(GENERATOR_FLUSH_HELD_TOKEN);

  /* If something is stored in the look-ahead buffer, add it to the work buffer first */
  if( strlen( lahead_buffer ) > 0 ) {

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing held token (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

    assert( (strlen( work_buffer ) + strlen( lahead_buffer)) < 4095 );
    strcat( work_buffer, lahead_buffer );
    lahead_buffer[0] = '\0';

  }

  PROFILE_END;

}

/*!
 Inserts an instance ID parameter for the given functional unit.
*/
char* generator_inst_id_reg(
  func_unit* funit  /*!< Pointer to functional unit to add */
) { PROFILE(GENERATOR_INST_ID_PARAM);

  char* cov_str = NULL;

  if( funit != NULL ) {
    char         str[128];
    unsigned int rv = snprintf( str, 128, "reg [31:0] COVERED_INST_ID%d /* verilator public */;", funit->id );
    assert( rv < 128 );
    cov_str = generator_build( 2, strdup_safe( str ), "\n" );
  }

  PROFILE_END;

  return( cov_str );

}

/*!
 Writes the current instance tree to the parameter override file for instance IDs.
*/
static char* generator_inst_id_overrides_helper(
  funit_inst* root,                 /*!< Pointer to root instance to generate for */
  int         hier_chars_to_ignore  /*!< Number of characters in the scope name to ignore from output */
) { PROFILE(GENERATOR_INST_ID_OVERRIDES_HELPER);

  char* cov_str = NULL;

  if( root != NULL ) {

    funit_inst* child;
    bool        recursive = TRUE;

    /* Output ourselves */
    if( (root->funit != NULL) && (strcmp( root->name, "$root" ) != 0) ) {

      /* Don't continue the current tree if it is marked as a no score instance or it is to be ignored */
      if( (root->funit->suppl.part.type != FUNIT_NO_SCORE) && !root->suppl.ignore && !root->funit->suppl.part.fork ) {

        /* Only override modules and named blocks (tasks and functions will not have an ID to override) */
        if( (root->funit->suppl.part.type == FUNIT_MODULE) ||
            (root->funit->suppl.part.type == FUNIT_NAMED_BLOCK) ||
            (root->funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {

          char         str1[4096];
          char         str2[4096];
          unsigned int rv;

          /* Get the hierarchical reference */
          str1[0] = '\0';
          instance_gen_scope( str1, root, FALSE );

          /* Insert the code */
          rv = snprintf( str2, 4096, "%s%sCOVERED_INST_ID%d = %d;",
                         ((strlen( str1 ) == hier_chars_to_ignore) ? "" : (str1 + (hier_chars_to_ignore + 1))),
                         ((strlen( str1 ) == hier_chars_to_ignore) ? "" : "."),
                         root->funit->id, root->id );
          assert( rv < 4096 );

          cov_str = generator_build( 2, strdup_safe( str2 ), "\n" );

        }

      } else {

        recursive = FALSE;

      }

    }

    if( recursive ) {

      /* Output children */
      child = root->child_head;
      while( child != NULL ) {
        cov_str = generator_build( 2, cov_str, generator_inst_id_overrides_helper( child, hier_chars_to_ignore ) );
        child   = child->next;
      }

    }

  }

  PROFILE_END;

  return( cov_str );

}

/*!
 \return Returns the string that contains the instance ID overrides.

 Creates the parameter override file and populates it with the instance ID information.
*/
char* generator_inst_id_overrides() { PROFILE(GENERATOR_INST_ID_OVERRIDES);

  funit_inst*  top_inst;
  char         leading_hier[4096];
  unsigned int rv;
  char*        inst_str = NULL;

  /* Get the top-most module */
  leading_hier[0] = '\0';
  instance_get_leading_hierarchy( db_list[curr_db]->inst_tail->inst, leading_hier, &top_inst );

  if( info_suppl.part.verilator ) {

    if( top_inst->funit == curr_funit ) {

      /* Create initialization function */
      rv = snprintf( leading_hier, 4096, "void covered_initialize( %s* top, const char* cdd_name );", verilator_prefix );
      assert( rv < 4096 );
      inst_str = generator_build( 6, strdup_safe( "`systemc_header" ), "\n", strdup_safe( "#include \"covered_verilator.h\"" ), "\n",
                                  strdup_safe( leading_hier ), "\n" );

      rv = snprintf( leading_hier, 4096, "void covered_initialize( %s* top, const char* cdd_name ) {", verilator_prefix );
      assert( rv < 4096 );
      inst_str = generator_build( 5, inst_str, strdup_safe( "`systemc_implementation" ), "\n", strdup_safe( leading_hier ), "\n" );

      rv = snprintf( leading_hier, 4096, "covered_initialize_db( \"../%s\" );", output_db );
      assert( rv < 4096 );
      inst_str = generator_build( 6, inst_str, strdup_safe( leading_hier ), "\n", generator_verilator_inst_ids( top_inst, "top->" ),
                                  strdup_safe( "}" ), "\n" );

    } else {

      inst_str = generator_build( 2, strdup_safe( "`systemc_imp_header" ), "\n" );

      if( top_inst->funit != curr_funit ) {
        inst_str = generator_build( 3, inst_str, strdup_safe( "#define COVERED_METRICS_ONLY" ), "\n" );
      }

      inst_str = generator_build( 3, inst_str, strdup_safe( "#include \"covered_verilator.h\"" ), "\n" );

    }

    inst_str = generator_build( 3, inst_str, strdup_safe( "`verilog" ), "\n" );

  } else {

    /* If the current functional unit is the same as the top-most functional unit, insert the overrides */
    if( top_inst->funit == curr_funit ) {

      inst_link* instl = db_list[curr_db]->inst_head;

      inst_str = generator_build( 2, strdup_safe( "initial begin" ), "\n" );

      while( instl != NULL ) {
        inst_str = generator_build( 2, inst_str, generator_inst_id_overrides_helper( instl->inst, strlen( leading_hier ) ) );
        instl = instl->next;
      }

      inst_str = generator_build( 3, inst_str, strdup_safe( "end" ), "\n" );

    }

  }

  PROFILE_END;

  return( inst_str );

}

/*!
 Begins a statement that might be a part of a fork..join block, placing a "begin" prior to the block.
*/
void generator_begin_parallel_statement(
  unsigned int first_line,   /*!< First line of statement */
  unsigned int first_column  /*!< First column of statement */
) { PROFILE(GENERATOR_BEGIN_PARALLEL_STATEMENT);

  if( (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {

    func_unit* funit = db_get_tfn_by_position( first_line, first_column );

    assert( funit != NULL );

    /* If the block is not specified for forking, add the instance ID */
    if( funit->suppl.part.fork == 0 ) {

      char*        str;
      char*        back;
      char*        rest;
      unsigned int rv;
      unsigned int size;
      char         cstr[128];

      rv = snprintf( cstr, 128, " reg [31:0] COVERED_INST_ID%d /* verilator public */;", funit->id );
      assert( rv < 128 );

      generator_prepend_to_work_code( cstr );

      back = strdup_safe( funit->name );
      rest = strdup_safe( funit->name );
      scope_extract_back( funit->name, back, rest );

      size = strlen( back ) + 11;
      str  = (char*)malloc_safe( size );
      rv   = snprintf( str, size, " begin : %s ", back );
      assert( rv < size );

      generator_prepend_to_work_code( str );

      free_safe( back, (strlen( funit->name ) + 1) );
      free_safe( rest, (strlen( funit->name ) + 1) );
      free_safe( str,  size );

    }

  }

  PROFILE_END;

}

/*!
 Ends a statement that might be a part of a fork..join block, placing an "end" after the block.
*/
void generator_end_parallel_statement(
  unsigned int first_line,   /*!< First line of statement */
  unsigned int first_column  /*!< First column of statement */
) { PROFILE(GENERATOR_END_PARALLEL_STATEMENT);

  if( (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {

    func_unit* funit = db_get_tfn_by_position( first_line, first_column );

    assert( funit != NULL );

    if( funit->suppl.part.fork == 0 ) {
      generator_add_cov_to_work_code( " end " );
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a string containing all of the specified elements.
 Creates
*/
char* generator_build1(
  const char* file,  /*!< Name of file that this function was called from */
  int         line,  /*!< Line number of file that this function was called from */
  int         args,  /*!< Number of arguments to get from the rest of this function */
  ...
) { PROFILE(GENERATOR_BUILD);

  va_list ap;
  char*   str = NULL;
  int     len = 0;
  int     i;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In generator_build1, file: %s, line: %d, args: %d", file, line, args );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    va_start( ap, args );
    for( i=0; i<args; i++ ) {
      char* arg = va_arg( ap, char* );
      if( arg != NULL ) {
        if( strcmp( arg, "\n" ) == 0 ) {
          rv = snprintf( user_msg, USER_MSG_LENGTH, "%2d * \\n", i );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, DEBUG, __FILE__, __LINE__ );
        } else {
          rv = snprintf( user_msg, USER_MSG_LENGTH, "%2d * %s", i, arg );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, DEBUG, __FILE__, __LINE__ );
        }
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "%2d *", i );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, DEBUG, __FILE__, __LINE__ );
      }
    }
    va_end( ap );
    print_output( "", DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* First, get the length of the string */
  va_start( ap, args );
  for( i=0; i<args; i++ ) {
    char* arg = va_arg( ap, char* );
    if( arg != NULL ) {
      if( strcmp( arg, "\n" ) == 0 ) {
        /* TBD - We may want to represent the string as a string list */
        len += 1;
      } else {
        len += strlen( arg ) + 1;
      }
    }
  }
  va_end( ap );

  /* If the length is more than a character, allocate the memory */
  if( len > 0 ) {

    /* Allocate memory for the string */
    str    = (char*)malloc_safe_nolimit( len + 1 );
    str[0] = '\0';

    /* Now let's build that string... */
    va_start( ap, args );
    for( i=0; i<args; i++ ) {
      char* arg = va_arg( ap, char* );
      if( arg != NULL ) {
        if( strcmp( arg, "\n" ) == 0 ) {
          /* TBD - We may want to represent the string as a string list */
          strcat( str, "\n" );
        } else {
          strcat( str, arg );
          strcat( str, " " );
          free_safe( arg, (strlen( arg ) + 1) );
        }
      }
    }
    va_end( ap );

  }

  PROFILE_END;

  return( str );

}

/*!
 Allocates and populates a string/coverage structure.
*/
str_cov* generator_build2(
  char* cov,  /*!< Coverage string */
  char* str   /*!< Code string */
) { PROFILE(GENERATOR_BUILD2);

  str_cov* sc = (str_cov*)malloc_safe( sizeof( str_cov ) );

  sc->cov = cov;
  sc->str = str;

  PROFILE_END;

  return( sc );

}

/*!
 Deallocates a string/coverage structure.
*/
void generator_destroy2(
  str_cov* sc  /*!< Pointer to str_cov structure */
) { PROFILE(GENERATOR_DESTROY2);

  /* Deallocate the str_cov structure */
  free_safe( sc, sizeof( str_cov ) );

  PROFILE_END;

}

/*!
 \return Returns a string containing all of the temporary registers for the current scope.
*/
char* generator_tmp_regs() { PROFILE(GENERATOR_TMP_REGS);

  char*      str;
  funit_str* fstr = tmp_regs_top;

  /* Grab the string connected to the tail */
  str = tmp_regs_top->str;
  
  /* Adjust the tail and delete the old one */
  tmp_regs_top = tmp_regs_top->next;
  free_safe( fstr, sizeof( funit_str ) );
  
  PROFILE_END;

  return( str );

}

/*!
 Adds a new temporary register string to the stack.
*/
void generator_create_tmp_regs() { PROFILE(GENERATOR_CREATE_TMP_REGS);

  funit_str* fstr;

  /* Allocate the memory and initialize */
  fstr        = (funit_str*)malloc_safe( sizeof( funit_str ) );
  fstr->str   = NULL;
  fstr->funit = funit_top->funit;
  fstr->next  = tmp_regs_top;

  /* Point the top of the stack to the new string link */
  tmp_regs_top = fstr;

  PROFILE_END;

}

/*!
 Wrapper around the VLerror function (reuses the same error handler as the normal parser).
*/
void GENerror(
  char* str  /*!< Error string to output */
) { PROFILE(GENERROR);

  VLerror( str );

  PROFILE_END;

}

/*!
 Outputs the specified string to the output file and deallocates the memory for the string.
 This is called after an entire file has been parsed.
*/
void generator_write_to_file(
  char* str
) { PROFILE(GENERATOR_WRITE_TO_FILE);

  /* Output the string to the opened output file */
  fprintf( curr_ofile, "%s", str );

  /* Deallocate the string */
  free_safe( str, (strlen( str ) + 1) );

  PROFILE_END;

}

