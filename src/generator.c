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
#include "defines.h"
#include "expr.h"
#include "func_iter.h"
#include "generator.h"
#include "link.h"
#include "profiler.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern char         user_msg[USER_MSG_LENGTH];


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
 Pointer to current statement stack.
*/
static statement** stmt_stack = NULL;

/*!
 Number of statement pointers in statement stack.
*/
static int stmt_stack_size = 0;

/*!
 Index of statement stack top.
*/
static int stmt_stack_ptr = -1;


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
 \return Returns a pointer to the next statement to execute (or NULL if there are no more statements in this functional unit).
*/
static statement* generator_get_next_stmt(
  func_iter* fi  /*!< Pointer to functional unit iterator structure */
) { PROFILE(GENERATOR_GET_NEXT_STMT);

  statement* curr_stmt = NULL;

  /* Get current statement from stack if it exists */
  if( stmt_stack != NULL ) {
    curr_stmt = stmt_stack[stmt_stack_ptr];
  }

  /* If the current statement is the last statement to traverse in the block, pop the stack */
  if( (curr_stmt != NULL) && curr_stmt->suppl.part.stop_true && curr_stmt->suppl.part.stop_false ) {

    do {
      curr_stmt = stmt_stack[--stmt_stack_ptr];
    } while( (stmt_stack_ptr > 0) && curr_stmt->suppl.part.stop_false );
    curr_stmt = curr_stmt->next_false;

  /* Otherwise, push to the stack */
  } else {

    /*
     If the statement stack is unallocated or the current statement is a nested block, get the next
     statement from the functional unit iterator.
    */
    if( (stmt_stack == NULL) || (curr_stmt->exp->op == EXP_OP_NB_CALL) ) {
      if( (stmt_stack_ptr + 1) == stmt_stack_size ) {
        stmt_stack = (statement**)realloc_safe( stmt_stack, (sizeof( statement* ) * stmt_stack_size), (sizeof( statement* ) * (stmt_stack_size + 1)) );
        stmt_stack_size++;
      }
      curr_stmt = stmt_stack[++stmt_stack_ptr] = func_iter_get_next_statement( fi );

    /*
     Otherwise, if the current statement is an IF statement, traverse the TRUE path statement list.
    */
    } else if( curr_stmt->exp->op == EXP_OP_IF ) {
      if( (stmt_stack_ptr + 1) == stmt_stack_size ) {
        stmt_stack = (statement**)realloc_safe( stmt_stack, (sizeof( statement* ) * stmt_stack_size), (sizeof( statement* ) * (stmt_stack_size + 1)) );
        stmt_stack_size++;
      }
      curr_stmt = stmt_stack[++stmt_stack_ptr] = curr_stmt->next_true;

    /*
     Otherwise, traverse the TRUE path.
    */
    } else {
      curr_stmt = stmt_stack[stmt_stack_ptr] = curr_stmt->next_true;

    }

  }

  PROFILE_END;

  return( curr_stmt );

}

/*!
 Generates code for a leaf expression.
*/
static void generator_create_leaf(
            expression* exp,        /*!< Pointer to expression to generate */
            func_unit*  funit,      /*!< Pointer to functional unit containing this expression */
            exp_op_type parent_op,  /*!< Parent operation type */
            bool        net,        /*!< Specifies if we need to generate code for a net or a variable */
  /*@out@*/ str_link**  code_head,  /*!< Pointer to head of code list to populate */
  /*@out@*/ str_link**  code_tail   /*!< Pointer to tail of code list to populate */
) { PROFILE(GENERATOR_CREATE_LEAF);

  char**       code;
  unsigned int code_depth;
  unsigned int i;
  unsigned int rv;
  char         str[4096];

  /* Generate the code */
  codegen_gen_expr( exp, parent_op, &code, &code_depth, funit );

  /* Create the expression */
  if( net ) {
    rv = snprintf( str, 4096, "wire [0:0] covered$e%d = %s%c", exp->id, code[0], ((code_depth == 1) ? ';' : '\0') );
    assert( rv < 4096 );
    (void)str_link_add( strdup_safe( str ), code_head, code_tail );
  } else {
    str_link* tmp;
    rv = snprintf( str, 4096, "covered$e%d = %s%c", exp->id, code[0], ((code_depth == 1) ? ';' : '\0') );
    assert( rv < 4096 );
    tmp = str_link_add( strdup_safe( str ), code_head, code_tail );
    rv = snprintf( str, 4069, "reg [0:0] covered$e%d;", exp->id );
    assert( rv < 4096 );
    tmp->str2 = strdup_safe( str );
  }
  free_safe( code[0], (strlen( code[0] ) + 1) );

  /* Add any additional lines to the code */
  for( i=1; i<(code_depth - 1); i++ ) {
    (void)str_link_add( code[i], code_head, code_tail );
  }

  /* Add last line if necessary */
  if( code_depth > 1 ) {
    rv = snprintf( str, 4096, "%s;", code[i] );
    assert( rv < 4096 );
    (void)str_link_add( strdup_safe( str ), code_head, code_tail );
    free_safe( code[i], (strlen( code[i] ) + 1) );
  }

  free_safe( code, (sizeof( char* ) * code_depth) );

  PROFILE_END;

}

/*!
 Generates combinational logic coverage code for a given expression tree.
*/
static void generator_create_comb_coverage(
            expression*  exp,        /*!< Pointer to expression tree to generate code coverage Verilog for */
            func_unit*   funit,      /*!< Pointer to the functional unit containing the given exp */
            exp_op_type  parent_op,  /*!< Parent operator (initialize to the same operation type as exp) */
            bool         net,        /*!< If set to TRUE, generate code for a net; otherwise, generate code for a procedure */
            unsigned int depth,      /*!< Specifies the current expression depth (initialize to a value of 0) */
  /*@out@*/ str_link**   code_head,  /*!< Pointer to head of code list to populate */
  /*@out@*/ str_link**   code_tail   /*!< Pointer to tail of code list to populate */
) { PROFILE(GENERATOR_CREATE_COMB_COVERAGE);

  unsigned int max_comb_depth = 0xffffffff;  /* TBD - This needs to be global and come from the score command parser */

  if( exp != NULL ) {

    char         str[4096];
    unsigned int rv;
    str_link*    tmp;

    /* Increase the depth if we are different from our parent */
    if( exp->op != parent_op ) {
      depth++;
    }

    if( depth < max_comb_depth ) {

      /* Generate children first */
      generator_create_comb_coverage( exp->left,  funit, exp->op, net, depth, code_head, code_tail );
      generator_create_comb_coverage( exp->right, funit, exp->op, net, depth, code_head, code_tail );

      /* Now generate our code */
      switch( exp->op ) {
        case EXP_OP_AND :
          if( net ) {
            rv = snprintf( str, 4096, "wire [2:0] covered$e%d = {~covered$e%d[0],~covered$e%d[0],|(covered$e%d[0] & covered$e%d[0])};",
                           exp->id, exp->left->id, exp->right->id, exp->left->id, exp->right->id );
            assert( rv < 4096 );
            (void)str_link_add( strdup_safe( str ), code_head, code_tail );
          } else {
            rv = snprintf( str, 4096, "covered$e%d = {~covered$e%d[0],~covered$e%d[0],|(covered$e%d[0] & covered$e%d[0])};",
                           exp->id, exp->left->id, exp->right->id, exp->left->id, exp->right->id );
            assert( rv < 4096 );
            tmp = str_link_add( strdup_safe( str ), code_head, code_tail );
            rv  = snprintf( str, 4096, "reg [2:0] covered$e%d;", exp->id );
            assert( rv < 4096 );
            tmp->str2 = strdup_safe( str );
          }
          break;
        default :
          generator_create_leaf( exp, funit, parent_op, net, code_head, code_tail );
          break;
      }

    } else {

      generator_create_leaf( exp, funit, parent_op, net, code_head, code_tail );

    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if this is the last net assignment in a comma-separated list; otherwise, returns FALSE.

 Handles code coverage injection for a net assignment.
*/
static bool generator_handle_net_assign(
            statement*    stmt,       /*!< Pointer to current statement to generate coverage information for */
            func_unit*    funit,      /*!< Pointer to functional unit containing this statement */
            char*         line,       /*!< Original line from Verilog source */
  /*@out@*/ unsigned int* curr_char,  /*!< Current character position in the line */
  /*@out@*/ str_link**    code_head,  /*!< Pointer to head of code to output */
  /*@out@*/ str_link**    code_tail   /*!< Pointer to tail of code to output */
) { PROFILE(GENERATOR_HANDLE_NET_ASSIGN);

  unsigned int orig_len;
  char*        orig;
  unsigned int i       = (stmt->exp->col & 0xffff);
  unsigned int str_len = strlen( line );
  bool         retval  = FALSE;

  /* We need to create the information necessary to get statement coverage (no line coverage) */
  generator_create_comb_coverage( stmt->exp->right, funit, stmt->exp->op, TRUE, 0, code_head, code_tail );

  /* Scan the rest of the line for a semicolon or comma */
  while( (i < str_len) && (line[i] != ',') && (line[i] != ';') ) i++;
  orig_len = ((i + 1) - *curr_char);

  /* Don't create logic at this time if there are more declared assigns in this list */
  if( line[i] != ',' ) {

    /* Allocate memory for the original line and populate it */
    orig = (char*)malloc_safe( orig_len + 1 );
    for( i=0; i<orig_len; i++ ) {
      orig[i] = line[(*curr_char)++];
    }
    orig[i] = '\0';
    (void)str_link_add( orig, code_head, code_tail );
    retval = TRUE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 Handles a procedural assignment (blocking and non-blocking).
*/
static void generator_handle_proc_assign(
            statement*    stmt,       /*!< Pointer to statement to handle */
            func_unit*    funit,      /*!< Pointer to functional unit containing the specified statement */
            char*         line,       /*!< Line containing original Verilog code */
  /*@out@*/ unsigned int* curr_char,  /*!< Pointer to current character */
  /*@out@*/ str_link**    code_head,  /*!< Pointer to head of code list to populate */
  /*@out@*/ str_link**    code_tail   /*!< Pointer to tail of code list to populate */
) { PROFILE(GENERATOR_HANDLE_PROC_ASSIGN);

  unsigned int orig_len;
  char*        orig;
  unsigned int i       = (stmt->exp->col & 0xffff);
  unsigned int str_len = strlen( line );

  /* We need to create the information necessary to get statement coverage (no line coverage) */
  generator_create_comb_coverage( stmt->exp->right, funit, stmt->exp->op, FALSE, 0, code_head, code_tail );

  /* Scan the rest of the line for a semicolon or comma */
  while( (i < str_len) && (line[i] != ';') ) i++;
  orig_len = ((i + 1) - *curr_char);

  /* Allocate memory for the original line and populate it */
  orig = (char*)malloc_safe( orig_len + 1 );
  for( i=0; i<orig_len; i++ ) {
    orig[i] = line[(*curr_char)++];
  }
  orig[i] = '\0';
  (void)str_link_add( orig, code_head, code_tail );
  str_link_display( *code_head );

  PROFILE_END;

}

/*!
 Handles a nested block (begin..end) statement.
*/
static void generator_handle_nb_call(
            statement*    stmt,       /*!< Pointer to statement to handle */
            char*         line,       /*!< Line containing original Verilog code */
  /*@out@*/ unsigned int* curr_char,  /*!< Pointer to current character */
  /*@out@*/ str_link**    code_head,  /*!< Pointer to head of code list to populate */
  /*@out@*/ str_link**    code_tail   /*!< Pointer to tail of code list to populate */
) { PROFILE(GENERATOR_HANDLE_NB_CALL);

  char*        orig;
  unsigned int orig_len = ((stmt->exp->col & 0xffff) + 1) - *curr_char;
  unsigned int i;

  /* Allocate memory for the original line and populate it */
  orig = (char*)malloc_safe( orig_len + 1 );
  for( i=0; i<orig_len; i++ ) {
    orig[i] = line[(*curr_char)++];
  }
  orig[i] = '\0';
  (void)str_link_add( orig, code_head, code_tail );
  str_link_display( *code_head );

  PROFILE_END;

}

/*!
 Handle code coverage injection for an IF statement.
*/
static void generator_handle_if(
            statement*    stmt,       /*!< Pointer to the IF statement */
            char*         line,       /*!< Original Verilog string being parsed */
  /*@out@*/ unsigned int* curr_char,  /*!< Pointer to current character position */
  /*@out@*/ str_link**    code_head,  /*!< Pointer to head of code to generate */
  /*@out@*/ str_link**    code_tail   /*!< Pointer to tail of code to generate */
) { PROFILE(GENERATOR_HANDLE_IF);

  unsigned int orig_len = ((stmt->exp->col & 0xffff) - *curr_char);
  char*        orig     = (char*)malloc_safe( orig_len + 1 );
  unsigned int i;

  /* Copy the contents of the original string to the code list */
  for( i=0; i<orig_len; i++ ) {
    orig[i] = line[(*curr_char)++];
  }
  orig[i] = '\0';
  (void)str_link_add( orig, code_head, code_tail );

  /* If the true case is not a begin..end block, create one. */
  if( stmt->next_true->exp->op != EXP_OP_NB_CALL ) {
    (void)str_link_add( strdup_safe( " begin " ), code_head, code_tail );
  } 

  PROFILE_END;

}

static void generator_handle_delay(
            statement*    stmt,  
            char*         line,
  /*@out@*/ unsigned int* curr_char,
  /*@out@*/ str_link**    code_head,
  /*@out@*/ str_link**    code_tail
) { PROFILE(GENERATOR_HANDLE_DELAY);

  unsigned int orig_len;
  char*        orig;
  unsigned int i       = (stmt->exp->col & 0xffff);
  unsigned int str_len = strlen( line );

  /* See if delay is a single statement or if it is associated with an assignment */
  while( (i < str_len) && ((line[i] == ' ') || (line[i] == '\t')) ) i++;

  /* If it is its own statement copy everything from curr_char to this position */
  if( i == ';' ) {

    orig_len = ((i + 1) - *curr_char);
    orig     = (char*)malloc_safe( orig_len + 1 );
    for( i=0; i<orig_len; i++ ) {
      orig[i] = line[(*curr_char)++];
    }
    orig[i] = '\0';

  } else {

    orig_len = (((stmt->exp->col & 0xffff) + 1) - *curr_char);
    orig     = (char*)malloc_safe( orig_len + 2 );
    for( i=0; i<orig_len; i++ ) {
      orig[i] = line[(*curr_char)++];
    }
    orig[i++] = ';';
    orig[i]   = '\0';

  }

  /* Add the string to the code list */
  (void)str_link_add( orig, code_head, code_tail );

  PROFILE_END;

}

/*!
 Generates the needed code for line coverage information.
*/
static void generator_add_line_coverage(
            unsigned int stmt_index,  /*!< Statement index */
  /*@out@*/ str_link**   code_head,   /*!< Pointer to head of code segment list */
  /*@out@*/ str_link**   code_tail    /*!< Pointer to tail of code segment list */
) { PROFILE(GENERATOR_ADD_LINE_COVERAGE);

  str_link*    tmp;
  char         str[128];
  unsigned int rv;

  /* Create line coverage assignment and add it to list */
  rv = snprintf( str, 128, "covered$l%d = 1'b1;", stmt_index );
  assert( rv < 128 );
  tmp = str_link_add( strdup_safe( str ), code_head, code_tail );

  /* Create line coverage register assignment - TBD - We may want to make this Verilog-1995 compliant */
  rv = snprintf( str, 128, "reg covered$l%d = 1'b0; ", stmt_index );
  assert( rv < 128 );
  
  /* Add register assignment to string link */
  tmp->str2 = strdup_safe( str );

  PROFILE_END;

}

/*!
 Parses the given statement and injects appropriate coverage code.
*/
static void generator_handle_stmt(
  statement*    stmt,       /*!< Pointer to statement to generate coverage code for */
  func_unit*    funit,      /*!< Pointer to functional unit containing this statement */
  char*         line,       /*!< Pointer to the original Verilog line */
  unsigned int* curr_char,  /*!< Pointer to the current character position */
  FILE*         ofile       /*!< Pointer to the file stream to write */
) { PROFILE(GENERATOR_HANDLE_STMT);

  static str_link*    code_head;
  static str_link*    code_tail;
  static unsigned int depth       = 0; 
  bool                output_code = FALSE;

  /* Handle net assignments */
  if( (stmt->exp->op == EXP_OP_ASSIGN) || (stmt->exp->op == EXP_OP_DASSIGN) ) { 
    output_code = generator_handle_net_assign( stmt, funit, line, curr_char, &code_head, &code_tail );

  /* Handle procedural statements */
  } else {

    /* Handle begin..end */
    if( stmt->exp->op == EXP_OP_NB_CALL ) {
      generator_handle_nb_call( stmt, line, curr_char, &code_head, &code_tail );
      depth++;

    /* Otherwise, we will not be switching contexts so handle the rest of the needed statements */
    } else {

      /* Insert code for line coverage */
      generator_add_line_coverage( stmt->exp->id, &code_head, &code_tail );

      /* Handle procedural assignments */
      if( (stmt->exp->op == EXP_OP_BASSIGN) || (stmt->exp->op == EXP_OP_NASSIGN) ) {
        generator_handle_proc_assign( stmt, funit, line, curr_char, &code_head, &code_tail );

      /* Handle if..then..else */
      } else if( stmt->exp->op == EXP_OP_IF ) {
        generator_handle_if( stmt, line, curr_char, &code_head, &code_tail );

      /* Handle delay statement */
      } else if( stmt->exp->op == EXP_OP_DELAY ) {
        generator_handle_delay( stmt, line, curr_char, &code_head, &code_tail );

      }

      /* If we have reached the end of a nested block, reduce the depth by one */
      if( (stmt->next_false == NULL) && (stmt->next_true == NULL) ) {
        depth--;
      }

      /* If our depth is 0 and we have reached the last statement in this block, output the Verilog. - TBD */
      if( depth == 0 ) {
        output_code = TRUE;
      }

    }

  }

  /* If we need to output our code, do so now. */
  if( output_code ) {
    str_link* curr = code_head;
    while( curr != NULL ) {
      if( curr->str2 != NULL ) {
        fprintf( ofile, "%s\n", curr->str2 );
      }
      curr = curr->next;
    }
    curr = code_head;
    while( curr != NULL ) {
      fprintf( ofile, "%s\n", curr->str );
      curr = curr->next;
    }
    str_link_delete_list( code_head );
    code_head = code_tail = NULL;
  }

  PROFILE_END;

}

/*!
 Takes the current line, outputting both original Verilog and injected covered code.
*/
static void generator_handle_line(
  char*        line,      /*!< Current line */
  unsigned int line_num,  /*!< Current line number */
  fname_link*  fnamel,    /*!< Pointer to current filename link */
  FILE*        ofile      /*!< Pointer to output file */
) { PROFILE(GENERATOR_HANDLE_LINE);

  static func_iter  fi;
  static statement* stmt      = NULL;
  bool              end_found;
  unsigned int      curr_char = 0;

  do {
    end_found = FALSE;
    if( (line_num >= fnamel->next_funit->start_line) && (line_num <= fnamel->next_funit->end_line) ) {
      if( line_num == fnamel->next_funit->start_line ) {
        func_iter_init( &fi, fnamel->next_funit, TRUE, FALSE, FALSE );
        func_iter_display( &fi );
        stmt = generator_get_next_stmt( &fi );
      }
      if( (stmt != NULL) && (stmt->exp->line == line_num) ) {
        while( (stmt != NULL) && (stmt->exp->line == line_num) ) {
          generator_handle_stmt( stmt, fnamel->next_funit, line, &curr_char, ofile );
          stmt = generator_get_next_stmt( &fi );
        }
        fprintf( ofile, "%s\n", (line + curr_char) );
      } else {
        fprintf( ofile, "%s\n", line );
      }
      if( line_num == fnamel->next_funit->end_line ) {
        func_iter_dealloc( &fi );
        end_found = generator_set_next_funit( fnamel );
      }
    }
  } while( end_found );

  PROFILE_END;

}
  
/*!
 Generates an instrumented version of a given functional unit.
*/
static void generator_output_funits(
  fname_link* head  /*!< Pointer to the head of the filename list */
) { PROFILE(GENERATOR_OUTPUT_FUNIT);

  while( head != NULL ) {

    FILE*        ofile;
    char         filename[4096];
    unsigned int rv;

    rv = snprintf( filename, 4096, "covered/verilog/%s", get_basename( head->filename ) );
    assert( rv < 4096 );

    /* Open the output file for writing */
    if( (ofile = fopen( filename, "w" )) != NULL ) {

      FILE* ifile;

      /* Open the original file for reading */
      if( (ifile = fopen( head->filename, "r" )) != NULL ) {

        func_iter    fi;
        char*        line;
        unsigned int line_size;
        unsigned int line_num = 1;

        /* Read in each line */
        while( util_readline( ifile, &line, &line_size ) ) {
          generator_handle_line( line, line_num, head, ofile );
          free_safe( line, line_size );
          line_num++;
        }

        /* Close the input file */
        rv = fclose( ifile );
        assert( rv == 0 );

      } else {

        rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to open \"%s\" for reading", head->filename );
        assert( rv < USER_MSG_LENGTH );
        fclose( ofile );
        Throw 0;

      }

      /* Close output file */
      rv = fclose( ofile );
      assert( rv == 0 );

    } else {

      rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to create generated Verilog file \"%s\"", filename );
      assert( rv < USER_MSG_LENGTH );
      Throw 0;

    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Outputs the covered portion of the design to the covered/verilog directory.
*/
void generator_output() { PROFILE(GENERATOR_OUTPUT);

  fname_link* fname_head;  /* Pointer to head of filename linked list */
  fname_link* fname_tail;  /* Pointer to tail of filename linked list */
  fname_link* fnamel;      /* Pointer to current filename link */

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

  /* Create the filename list from the functional unit list */
  generator_create_filename_list( db_list[curr_db]->funit_head, &fname_head, &fname_tail );

  /* Iterate through the covered files, generating coverage output along with the design information */
  generator_output_funits( fname_head );

  /* Deallocate memory from filename list */
  generator_dealloc_filename_list( fname_head );

  /* Deallocate statement stack */
  free_safe( stmt_stack, (sizeof( statement* ) * stmt_stack_size) );

  PROFILE_END;

}

/*
 $Log$
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

