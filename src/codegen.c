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
 \file     codegen.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/11/2002
 
 \par
 This file and its functions are used solely by the Covered report command.  When the user
 specifies that Covered should report detailed or verbose reports, it is necessary to output
 the Verilog code associated with line and combinational logic coverage.  However, the actual
 Verilog code read from the file is not stored internally anywhere inside of the Covered.  By
 doing this, Covered can do two things:  (1) save on memory that otherwise need to be stored
 or save on processing (have to reparse the code for reports) and (2) allows Covered to "clean
 up the source code making it easier to read (hopefully) and easier to underline missed
 combinational logic cases.
 
 \par
 What the code generator does then is to recreate the source Verilog from the internally
 stored expression tree.  This allows associated Verilog code with uncovered logic to quickly
 and efficiently be display to the user.  The code generator is used in combination with the
 underlining facility (located in the comb.c file).
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "codegen.h"
#include "util.h"
#include "vector.h"
#include "func_unit.h"
#include "expr.h"
#include "obfuscate.h"


extern bool           flag_use_line_width;
extern int            line_width;
extern char           user_msg[USER_MSG_LENGTH];
extern const exp_info exp_op_info[EXP_OP_NUM];


/*!
 Generates multi-line expression code strings from current, left, and right expressions.
*/
static void codegen_create_expr_helper(
  /*@out@*/  char**       code,             /*!< Array of strings containing generated code lines */
             int          code_index,       /*!< Index to current string in code array to set */
             char*        first,            /*!< String value coming before left expression string */
  /*@null@*/ char**       left,             /*!< Array of strings for left expression code */
             unsigned int left_depth,       /*!< Number of strings contained in left array */
             bool         first_same_line,  /*!< Set to TRUE if left expression is on the same line as the current expression */
             char*        middle,           /*!< String value coming after left expression string */
  /*@null@*/ char**       right,            /*!< Array of strings for right expression code */
             unsigned int right_depth,      /*!< Number of strings contained in right array */
             bool         last_same_line,   /*!< Set to TRUE if right expression is on the same line as the left expression */
  /*@null@*/ char*        last              /*!< String value coming after right expression string */
) { PROFILE(CODEGEN_CREATE_EXPR_HELPER);

  char*        tmpstr;         /* Temporary string holder */
  unsigned int code_size = 0;  /* Length of current string to generate */
  unsigned int i;              /* Loop iterator */
  unsigned int rv;             /* Return value from snprintf calls */

  assert( left_depth > 0 );

/*
  // TEMPORARY
  printf( "code_index: %d, first: %s, left_depth: %d, first_same: %d, middle: %s, right_depth: %d, last_same: %d, last: %s\n",
          code_index, first, left_depth, first_same_line, middle, right_depth, last_same_line, last );
  printf( "left code:\n" );
  for( i=0; i<left_depth; i++ ) {
    printf( "  .%s.\n", left[i] );
  }
  printf( "right_code:\n" );
  for( i=0; i<right_depth; i++ ) {
    printf( "  .%s.\n", right[i] );
  }
*/

  if( first != NULL ) {
    code_size += strlen( first );
    // printf( "code size after first: %d\n", code_size );
  }
  if( first_same_line ) {
    code_size += strlen( left[0] );
    // printf( "code size after left[0]: %d\n", code_size );
  //  if( (left_depth == 1) && (middle != NULL) && (right_depth == 0) ) {
  //    code_size += strlen( middle );
  //    printf( "code size after middle: %d\n", code_size );
  //  }
  }
  if( code[code_index] != NULL ) {
    free_safe( code[code_index], (strlen( code[code_index] ) + 1) );
  }
  code[code_index]    = (char*)malloc_safe( code_size + 1 );
  //printf( "Allocated %d bytes for code[%d]\n", (code_size + 1), code_index );
  code[code_index][0] = '\0';

  if( first != NULL ) {
    rv = snprintf( code[code_index], (code_size + 1), "%s", first );
    assert( rv < (code_size + 1) );
  }
  if( first_same_line ) {
    tmpstr = strdup_safe( code[code_index] );
    rv = snprintf( code[code_index], (code_size + 1), "%s%s", tmpstr, left[0] );
    assert( rv < (code_size + 1) );
    free_safe( tmpstr, (strlen( tmpstr ) + 1) );
    free_safe( left[0], (strlen( left[0] ) + 1) );
    if( (left_depth == 1) && (middle != NULL) ) {
      code_size = strlen( code[code_index] ) + strlen( middle );
      tmpstr    = (char*)malloc_safe( code_size + 1 );
      rv = snprintf( tmpstr, (code_size + 1), "%s%s", code[code_index], middle );
      assert( rv < (code_size + 1) );
      if( right_depth > 0 ) {
        // printf( "A code[%d]:%s.\n", code_index, code[code_index] );
        codegen_create_expr_helper( code, code_index, tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
        free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      } else {
        free_safe( code[code_index], (strlen( code[code_index] ) + 1) );
        code[code_index] = tmpstr;
      }
    } else {
      if( middle != NULL ) {
        for( i=1; i<(left_depth - 1); i++ ) {
          code[code_index+i] = left[i];
        }
        code_size = strlen( left[i] ) + strlen( middle );
        tmpstr    = (char*)malloc_safe( code_size + 1 );
        rv = snprintf( tmpstr, (code_size + 1), "%s%s", left[i], middle );
        assert( rv < (code_size + 1) );
        free_safe( left[i], (strlen( left[i] ) + 1) );
        if( right_depth > 0 ) {
          // printf( "B code[%d+%d]:%s.\n", code_index, i, code[code_index] );
          codegen_create_expr_helper( code, (code_index + i), tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
          free_safe( tmpstr, (strlen( tmpstr ) + 1) );
        } else {
          code[code_index+i] = tmpstr;
        }
      } else {
        for( i=1; i<left_depth; i++ ) {
          code[code_index+i] = left[i];
        }
      }
    }
  } else {
    if( middle != NULL ) {
      for( i=0; i<(left_depth - 1); i++ ) {
        code[code_index+1+i] = left[i];
      }
      code_size = strlen( left[i] ) + strlen( middle );
      tmpstr    = (char*)malloc_safe( code_size + 1 );
      rv = snprintf( tmpstr, (code_size + 1), "%s%s", left[i], middle );
      assert( rv < (code_size + 1) );
      free_safe( left[i], (strlen( left[i] ) + 1) );
      if( right_depth > 0 ) {
        // printf( "C code[%d+%d]:%s.\n", code_index, i, code[code_index] );
        codegen_create_expr_helper( code, (code_index + i), tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
        free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      } else {
        code[code_index+i+1] = tmpstr;
      }
    } else {
      for( i=0; i<left_depth; i++ ) {
        code[code_index+i+1] = left[i];
      }
    }
  }

}

/*!
 Allocates enough memory in code array to store all code lines for the current expression.
 Calls the helper function to actually generate code lines (to populate the code array).
*/
static void codegen_create_expr(
  /*@out@*/  char***       code,        /*!< Pointer to an array of strings which will contain generated code lines */
             unsigned int* code_depth,  /*!< Pointer to number of strings contained in code array */
             int           curr_line,   /*!< Line number of current expression */
  /*@null@*/ char*         first,       /*!< String value coming before left expression string */
             char**        left,        /*!< Array of strings for left expression code */
             unsigned int  left_depth,  /*!< Number of strings contained in left array */
             expression*   left_exp,    /*!< Pointer to left expression */
  /*@null@*/ char*         middle,      /*!< String value coming after left expression string */
  /*@null@*/ char**        right,       /*!< Array of strings for right expression code */
             unsigned int  right_depth, /*!< Number of strings contained in right array */
  /*@null@*/ expression*   right_exp,   /*!< Pointer to right expression */
  /*@null@*/ char*         last         /*!< String value coming after right expression string */
) { PROFILE(CODEGEN_CREATE_EXPR);

  int          total_len = 0;    /* Total length of first, left, middle, right, and last strings */
  unsigned int i;                /* Loop iterator */ 
  expression*  last_exp;         /* Last expression of left expression tree */
  expression*  first_exp;        /* First expression of right expression tree */
  int          left_line_start;  /* Line number of first expression of left expression tree */
  int          left_line_end;    /* Line number of last expression of left expression tree */
  int          right_line;       /* Line number of first expression of right expression tree */

/*
  printf( "-----------------------------------------------------------------\n" );
  printf( "codegen_create_expr, curr_line: %d, first: %s, middle: %s, last: %s\n", curr_line, first, middle, last );
*/

  *code_depth = 0;

  if( (left_depth > 0) || (right_depth > 0) ) {

    /* Allocate enough storage in code array for these lines */
    if( left_depth > 0 ) {
      *code_depth += left_depth;
    }
    if( right_depth > 0 ) {
      *code_depth += right_depth;
    }

    /* Calculate last and first expression values */
    if( left_exp != NULL ) {
      first_exp       = expression_get_first_line_expr( left_exp );
      left_line_start = first_exp->line;
      last_exp        = expression_get_last_line_expr( left_exp );
      left_line_end   = last_exp->line;
    } else {
      left_line_start = 0;
      left_line_end   = 0;
    }
    if( right_exp != NULL ) {
      first_exp  = expression_get_first_line_expr( right_exp );
      right_line = first_exp->line;
    } else {
      right_line = 0;
    }

    if( flag_use_line_width ) {

      if( first != NULL   ) { total_len += strlen( first );                }
      if( left_depth > 0  ) { total_len += strlen( left[left_depth - 1] ); }
      if( middle != NULL  ) { total_len += strlen( middle );               }
      if( right_depth > 0 ) { total_len += strlen( right[0] );             }
      if( last != NULL    ) { total_len += strlen( last );                 }

      if( (left_depth > 0) && (right_depth > 0) && (total_len <= line_width) ) {
        *code_depth -= 1;
      }

    } else {

      if( (left_depth > 0) && (right_depth > 0) && (left_line_end == right_line) ) {
        *code_depth -= 1;
      }
      if( (left_depth > 0) && (left_line_start > curr_line) ) {
        *code_depth += 1;
      }
      if( (left_depth == 0) && (right_depth > 0) && (right_line != curr_line) ) {
        *code_depth += 1;
      }

    }

    *code = (char**)malloc_safe( sizeof( char* ) * (*code_depth) );
    for( i=0; i<(*code_depth); i++ ) {
      (*code)[i] = NULL;
    }

    /* Now generate expression string array */
    if( flag_use_line_width ) {
      codegen_create_expr_helper( *code, 0, first, left, left_depth, TRUE, middle,
                                  right, right_depth, (total_len <= line_width), last );
    } else {
      codegen_create_expr_helper( *code, 0, first, left, left_depth, (curr_line >= left_line_start), middle,
                                  right, right_depth, (left_line_end == right_line), last );
    }

/*
    printf( "CODE:\n" );
    for( i=0; i<(*code_depth); i++ ) {
      printf( "%s\n", (*code)[i] );
    }
*/

  }

}

/*!
 \param expr        Pointer to root of expression tree to generate.
 \param parent_op   Operation of parent.  If our op is the same, no surrounding parenthesis is needed.
 \param code        Pointer to array of strings that will contain code lines for the supplied expression.
 \param code_depth  Pointer to number of strings contained in code array.
 \param funit       Pointer to functional unit containing the specified expression.

 Generates Verilog code from specified expression tree.  This Verilog
 snippet is used by the verbose coverage reporting functions for showing
 Verilog examples that were missed during simulation.  This code handles multi-line
 Verilog output by storing its information into the code and code_depth parameters.
*/
void codegen_gen_expr(
            expression*   expr,        /*!< Pointer to root of expression tree to generate */
            exp_op_type   parent_op,   /*!< Operation of parent.  If our op is the same, no surrounding parenthesis is needed */
  /*@out@*/ char***       code,        /*!< Pointer to array of strings that will contain code lines for the supplied expression */
  /*@out@*/ unsigned int* code_depth,  /*!< Pointer to number of strings contained in code array */
            func_unit*    funit        /*!< Pointer to functional unit containing the specified expression */
) { PROFILE(CODEGEN_GEN_EXPR);

  char**       right_code;               /* Pointer to the code that is generated by the right side of the expression */
  char**       left_code;                /* Pointer to the code that is generated by the left side of the expression */
  unsigned int left_code_depth  = 0;     /* Depth of left code string array */
  unsigned int right_code_depth = 0;     /* Depth of right code string array */
  char         code_format[20];          /* Format for creating my_code string */
  char*        tmpstr;                   /* Temporary string holder */
  char*        before;                   /* String before operation */
  char*        after;                    /* String after operation */
  func_unit*   tfunit;                   /* Temporary pointer to functional unit */
  char*        pname            = NULL;  /* Printable version of signal name */
  unsigned int rv;                       /* Return value from calls to snprintf */

  if( expr != NULL ) {

    /* Only traverse left and right expression trees if we are not an SLIST-type */
    if( (expr->op != EXP_OP_SLIST) && (expr->op != EXP_OP_ALWAYS_COMB) && (expr->op != EXP_OP_ALWAYS_LATCH) ) {

      codegen_gen_expr( expr->left,  expr->op, &left_code,  &left_code_depth,  funit );
      codegen_gen_expr( expr->right, expr->op, &right_code, &right_code_depth, funit );

    }

    if( (expr->op == EXP_OP_LAST) || (expr->op == EXP_OP_NB_CALL) || (expr->op == EXP_OP_JOIN) || (expr->op == EXP_OP_FORK) ||
        ((parent_op == EXP_OP_REPEAT) && (expr->parent->expr->left == expr)) ) {

      /* Do nothing. */
      *code_depth = 0;

    } else if( expr->op == EXP_OP_STATIC ) {

      *code       = (char**)malloc_safe( sizeof( char* ) );
      *code_depth = 1;

      if( ESUPPL_STATIC_BASE( expr->suppl ) == DECIMAL ) {

        rv = snprintf( code_format, 20, "%d", vector_to_int( expr->value ) );
        assert( rv < 20 );
        if( (strlen( code_format ) == 1) && (expr->parent->expr->op == EXP_OP_NEGATE) ) {
          strcat( code_format, " " );
        }
        (*code)[0] = strdup_safe( code_format );

      } else if( ESUPPL_STATIC_BASE( expr->suppl ) == QSTRING ) {

        unsigned int slen;
        tmpstr = vector_to_string( expr->value, QSTRING, FALSE );
        slen   = strlen( tmpstr ) + 3;
        (*code)[0] = (char*)malloc_safe( slen );
        rv = snprintf( (*code)[0], slen, "\"%s\"", tmpstr );
        assert( rv < slen );
        free_safe( tmpstr, (strlen( tmpstr ) + 1) );

      } else { 

        (*code)[0] = vector_to_string( expr->value, ESUPPL_STATIC_BASE( expr->suppl ), FALSE );

      }

    } else if( (expr->op == EXP_OP_SIG) || (expr->op == EXP_OP_PARAM) ) {

      tmpstr = scope_gen_printable( expr->name );

      switch( strlen( tmpstr ) ) {
        case 0 :  assert( strlen( tmpstr ) > 0 );  break;
        case 1 :
          *code       = (char**)malloc_safe( sizeof( char* ) );
          (*code)[0]  = (char*)malloc_safe( 4 );
          *code_depth = 1;
          rv = snprintf( (*code)[0], 4, " %s ", tmpstr );
          assert( rv < 4 );
          break;
        case 2 :
          *code       = (char**)malloc_safe( sizeof( char* ) );
          (*code)[0]  = (char*)malloc_safe( 4 );
          *code_depth = 1;
          rv = snprintf( (*code)[0], 4, " %s", tmpstr );
          assert( rv < 4 );
          break;
        default :
          *code       = (char**)malloc_safe( sizeof( char* ) );
          (*code)[0]  = strdup_safe( tmpstr );
          *code_depth = 1;
          break;
      }

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );

    } else if( (expr->op == EXP_OP_SBIT_SEL) || (expr->op == EXP_OP_PARAM_SBIT) ) {

      if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) &&
          (expr->parent->expr->op == EXP_OP_DIM) &&
          (expr->parent->expr->right == expr) ) {
        tmpstr = (char*)malloc_safe( 2 );
        rv = snprintf( tmpstr, 2, "[" );
        assert( rv < 2 );
      } else {
        unsigned int slen;
        pname  = scope_gen_printable( expr->name );
        slen   = strlen( pname ) + 2;
        tmpstr = (char*)malloc_safe( slen );
        rv = snprintf( tmpstr, slen, "%s[", pname );
        assert( rv < slen );
      }

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth,
                           expr->left, "]", NULL, 0, NULL, NULL );

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( (expr->op == EXP_OP_MBIT_SEL) || (expr->op == EXP_OP_PARAM_MBIT) ) {

      if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) &&
          (expr->parent->expr->op == EXP_OP_DIM) &&
          (expr->parent->expr->right == expr) ) {
        tmpstr = (char*)malloc_safe( 2 );
        rv = snprintf( tmpstr, 2, "[" );
        assert( rv < 2 );
      } else {
        unsigned int slen;
        pname  = scope_gen_printable( expr->name );
        slen   = strlen( pname ) + 2;
        tmpstr = (char*)malloc_safe( slen );
        rv = snprintf( tmpstr, slen, "%s[", pname );
        assert( rv < slen );
      }

      if( ESUPPL_WAS_SWAPPED( expr->suppl ) ) {
        codegen_create_expr( code, code_depth, expr->line, tmpstr,
                             right_code, right_code_depth, expr->right, ":",
                             left_code, left_code_depth, expr->left, "]" );
      } else {
        codegen_create_expr( code, code_depth, expr->line, tmpstr,
                             left_code, left_code_depth, expr->left, ":",
                             right_code, right_code_depth, expr->right, "]" );
      }

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( (expr->op == EXP_OP_MBIT_POS) || (expr->op == EXP_OP_PARAM_MBIT_POS) ) {

      if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) &&
          (expr->parent->expr->op == EXP_OP_DIM) &&
          (expr->parent->expr->right == expr) ) {
        tmpstr = (char*)malloc_safe( 2 );
        rv = snprintf( tmpstr, 2, "[" );
        assert( rv < 2 );
      } else {
        unsigned int slen;
        pname  = scope_gen_printable( expr->name );
        slen   = strlen( pname ) + 2;
        tmpstr = (char*)malloc_safe( slen );
        rv = snprintf( tmpstr, slen, "%s[", pname );
        assert( rv < slen );
      }

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left, "+:",
                           right_code, right_code_depth, expr->right, "]" );

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( (expr->op == EXP_OP_MBIT_NEG) || (expr->op == EXP_OP_PARAM_MBIT_NEG) ) {

      if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) &&
          (expr->parent->expr->op == EXP_OP_DIM) &&
          (expr->parent->expr->right == expr) ) {
        tmpstr = (char*)malloc_safe( 2 );
        rv = snprintf( tmpstr, 2, "[" );
        assert( rv < 2 );
      } else {
        unsigned int slen;
        pname  = scope_gen_printable( expr->name );
        slen   = strlen( pname ) + 2;
        tmpstr = (char*)malloc_safe( slen );
        rv = snprintf( tmpstr, slen, "%s[", pname );
        assert( rv < slen );
      }

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left, "-:",
                           right_code, right_code_depth, expr->right, "]" );

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( (expr->op == EXP_OP_FUNC_CALL) || (expr->op == EXP_OP_TASK_CALL) ) {

      assert( expr->elem.funit != NULL );

      tfunit = expr->elem.funit;
      after = (char*)malloc_safe( strlen( tfunit->name ) + 1 );
      scope_extract_back( tfunit->name, after, user_msg );
      pname = scope_gen_printable( after );
      if( (expr->op == EXP_OP_TASK_CALL) && (expr->left == NULL) ) {
        *code       = (char**)malloc_safe( sizeof( char* ) );
        (*code)[0]  = strdup_safe( pname );
        *code_depth = 1;
      } else {
        unsigned int slen;
        tmpstr = (char*)malloc_safe( strlen( pname ) + 3 );
        slen   = strlen( pname ) + 3;
        rv = snprintf( tmpstr, slen, "%s( ", pname );
        assert( rv < slen );
        codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left, " )", NULL, 0, NULL, NULL );
        free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      }
      free_safe( after, (strlen( tfunit->name ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( expr->op == EXP_OP_TRIGGER ) {
      unsigned int slen;
      assert( expr->sig != NULL );
      pname  = scope_gen_printable( expr->name );
      slen   = strlen( pname ) + 3;
      tmpstr = (char*)malloc_safe( slen );
      rv = snprintf( tmpstr, slen, "->%s", pname );
      assert( rv < slen );

      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = strdup_safe( tmpstr );
      *code_depth = 1;

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( expr->op == EXP_OP_DISABLE ) {
      unsigned int slen;
      assert( expr->elem.funit != NULL );
      pname  = scope_gen_printable( expr->name );
      slen   = strlen( pname ) + 9;
      tmpstr = (char*)malloc_safe( slen );
      rv = snprintf( tmpstr, slen, "disable %s", pname );
      assert( rv < slen );

      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = strdup_safe( tmpstr );
      *code_depth = 1;

      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
      free_safe( pname, (strlen( pname ) + 1) );

    } else if( expr->op == EXP_OP_DEFAULT ) {

      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = strdup_safe( "default :" );
      *code_depth = 1;

    } else if( expr->op == EXP_OP_SLIST ) {

      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = strdup_safe( "@*" );
      *code_depth = 1;

    } else if( expr->op == EXP_OP_ALWAYS_COMB ) {
 
      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = strdup_safe( "always_comb" );
      *code_depth = 1;

    } else if( expr->op == EXP_OP_ALWAYS_LATCH ) {

      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = strdup_safe( "always_latch" );
      *code_depth = 1;

    } else {

      if( parent_op == expr->op ) {
        before = NULL;
        after  = NULL;
      } else {
        before = strdup_safe( "(" );
        after  = strdup_safe( ")" );
      }

      switch( expr->op ) {
        case EXP_OP_XOR      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ^ ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_XOR_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ^= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_MULTIPLY :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " * ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_MLT_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " *= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_DIVIDE   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " / ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_DIV_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " /= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_MOD      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " % ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_MOD_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " %= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ADD      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " + ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ADD_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " += ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_SUBTRACT :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " - ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_SUB_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " -= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_EXPONENT :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ** ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_AND      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " & ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_AND_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " &= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_OR       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " | ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_OR_A     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " |= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_NAND     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ~& ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_NOR      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ~| ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_NXOR     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ~^ ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_LT       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " < ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_GT       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " > ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_LSHIFT   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " << ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_LS_A     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " <<= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ALSHIFT  :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " <<< ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ALS_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " <<<= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_RSHIFT   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >> ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_RS_A     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >>= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ARSHIFT  :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >>> ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ARS_A    :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >>>= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_EQ       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " == ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_CEQ      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " === ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_LE       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " <= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_GE       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >= ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_NE       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " != ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_CNE      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " !== ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_LOR      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " || ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_LAND     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " && ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_COND     :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " ? ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_COND_SEL :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " : ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_UINV     :
          codegen_create_expr( code, code_depth, expr->line, "~", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UAND     :
          codegen_create_expr( code, code_depth, expr->line, "&", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UNOT     :
          codegen_create_expr( code, code_depth, expr->line, "!", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UOR      :
          codegen_create_expr( code, code_depth, expr->line, "|", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UXOR     :
          codegen_create_expr( code, code_depth, expr->line, "^", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UNAND    :
          codegen_create_expr( code, code_depth, expr->line, "~&", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UNOR     :
          codegen_create_expr( code, code_depth, expr->line, "~|", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_UNXOR    :
          codegen_create_expr( code, code_depth, expr->line, "~^", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_EXPAND   :
          codegen_create_expr( code, code_depth, expr->line, "{", left_code, left_code_depth, expr->left, "{",
                               right_code, right_code_depth, expr->right, "}}" );
          break;
        case EXP_OP_LIST     :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, ", ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_CONCAT   :
          codegen_create_expr( code, code_depth, expr->line, "{", right_code, right_code_depth, expr->right, "}",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_PEDGE    :
          if( (ESUPPL_IS_ROOT( expr->suppl ) == 1)       ||
              (expr->parent->expr->op == EXP_OP_RPT_DLY) || 
              (expr->parent->expr->op == EXP_OP_DLY_OP) ) {
            codegen_create_expr( code, code_depth, expr->line, "@(posedge ", right_code, right_code_depth, expr->right, ")",
                                 NULL, 0, NULL, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, "posedge ", right_code, right_code_depth, expr->right, NULL,
                                 NULL, 0, NULL, NULL );
          }
          break;
        case EXP_OP_NEDGE    :
          if( (ESUPPL_IS_ROOT( expr->suppl ) == 1)       ||
              (expr->parent->expr->op == EXP_OP_RPT_DLY) ||
              (expr->parent->expr->op == EXP_OP_DLY_OP) ) {
            codegen_create_expr( code, code_depth, expr->line, "@(negedge ", right_code, right_code_depth, expr->right, ")",
                                 NULL, 0, NULL, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, "negedge ", right_code, right_code_depth, expr->right, NULL,
                                 NULL, 0, NULL, NULL );
          }
          break;
        case EXP_OP_AEDGE    :
          if( (ESUPPL_IS_ROOT( expr->suppl ) == 1)       ||
              (expr->parent->expr->op == EXP_OP_RPT_DLY) ||
              (expr->parent->expr->op == EXP_OP_DLY_OP) ) {
            codegen_create_expr( code, code_depth, expr->line, "@(", right_code, right_code_depth, expr->right, ")",
                                 NULL, 0, NULL, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, NULL, right_code, right_code_depth, expr->right, NULL,
                                 NULL, 0, NULL, NULL );
          }
          break;
        case EXP_OP_EOR      :
          if( (ESUPPL_IS_ROOT( expr->suppl ) == 1)       ||
              (expr->parent->expr->op == EXP_OP_RPT_DLY) ||
              (expr->parent->expr->op == EXP_OP_DLY_OP) ) {
            codegen_create_expr( code, code_depth, expr->line, "@(", left_code, left_code_depth, expr->left, " or ",
                                 right_code, right_code_depth, expr->right, ")" );
          } else {
            codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " or ",
                                 right_code, right_code_depth, expr->right, NULL );
          }
          break;
        case EXP_OP_CASE     :
          codegen_create_expr( code, code_depth, expr->line, "case( ", left_code, left_code_depth, expr->left, " ) ",
                               right_code, right_code_depth, expr->right, " :" );
          break;
        case EXP_OP_CASEX    :
          codegen_create_expr( code, code_depth, expr->line, "casex( ", left_code, left_code_depth, expr->left, " ) ",
                               right_code, right_code_depth, expr->right, " :" );
          break;
        case EXP_OP_CASEZ    :
          codegen_create_expr( code, code_depth, expr->line, "casez( ", left_code, left_code_depth, expr->left, " ) ",
                               right_code, right_code_depth, expr->right, " :" );
          break;
        case EXP_OP_DELAY    :
          codegen_create_expr( code, code_depth, expr->line, "#(", right_code, right_code_depth, expr->right, ")",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_ASSIGN   :
          codegen_create_expr( code, code_depth, expr->line, "assign ", left_code, left_code_depth, expr->left, " = ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_DASSIGN  :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " = ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_BASSIGN  :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " = ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_NASSIGN  :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " <= ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_PASSIGN  :
          *code            = right_code;
          *code_depth      = right_code_depth;
          right_code_depth = 0;
          break;
        case EXP_OP_DLY_ASSIGN :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " = ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_DLY_OP   :
        case EXP_OP_RPT_DLY  :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " ",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        case EXP_OP_IF       :
          codegen_create_expr( code, code_depth, expr->line, "if( ", right_code, right_code_depth, expr->right, " )",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_REPEAT   :
          codegen_create_expr( code, code_depth, expr->line, "repeat( ", right_code, right_code_depth, expr->right, " )",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_WHILE    :
          codegen_create_expr( code, code_depth, expr->line, "while( ", right_code, right_code_depth, expr->right, " )",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_WAIT     :
          codegen_create_expr( code, code_depth, expr->line, "wait( ", right_code, right_code_depth, expr->right, " )",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_NEGATE   :
          codegen_create_expr( code, code_depth, expr->line, "-", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_IINC     :
          codegen_create_expr( code, code_depth, expr->line, "++", left_code, left_code_depth, expr->left, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_PINC     :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, "++",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_IDEC     :
          codegen_create_expr( code, code_depth, expr->line, "--", left_code, left_code_depth, expr->left, NULL,
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_PDEC     :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, "--",
                               NULL, 0, NULL, NULL );
          break;
        case EXP_OP_DIM      :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, "",
                               right_code, right_code_depth, expr->right, NULL );
          break;
        default:  break;
      }

      if( parent_op != expr->op ) {
        free_safe( before, (strlen( before ) + 1) );
        free_safe( after, (strlen( after ) + 1) );
      }

    }

    if( right_code_depth > 0 ) {
      free_safe( right_code, (sizeof( char* ) * right_code_depth) );
    }

    if( left_code_depth > 0 ) {
      free_safe( left_code, (sizeof( char* ) * left_code_depth) );
    }

  }

}


/*
 $Log$
 Revision 1.96  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.95.2.2  2008/05/07 21:09:10  phase1geo
 Added functionality to allow to_string to output full vector bits (even
 non-significant bits) for purposes of reporting for FSMs (matches original
 behavior).

 Revision 1.95.2.1  2008/05/05 19:49:59  phase1geo
 Updating regressions, fixing bugs and added new diagnostics.  Checkpointing.

 Revision 1.95  2008/04/09 18:00:33  phase1geo
 Fixing op-and-assign operation and updated regression files appropriately.
 Also modified verilog/Makefile to compile lib or src directory as needed
 according to the VPI environment variable being set or not.  Full regression
 passes.

 Revision 1.94  2008/04/08 22:45:10  phase1geo
 Optimizations for op-and-assign expressions.  This is an untested checkin
 at this point but it does compile cleanly.  Checkpointing.

 Revision 1.93  2008/04/06 21:31:12  phase1geo
 Fixing more regression failures.  Last of regression updates.

 Revision 1.92  2008/04/05 05:30:18  phase1geo
 Fixing report bug from regressions and updating more regression runs.

 Revision 1.91  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.90  2008/03/18 03:56:44  phase1geo
 More updates for memory checking (some "fixes" here as well).

 Revision 1.89  2008/03/17 22:02:30  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.88  2008/03/17 05:26:15  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.87  2008/01/10 04:59:03  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.86  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.85  2008/01/07 05:01:57  phase1geo
 Cleaning up more splint errors.

 Revision 1.84  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.83  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.82  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.81  2006/12/12 06:20:22  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.80  2006/11/20 21:36:21  phase1geo
 Fixing bug 1599869.  When a disable statement is found to not be covered, the
 codegen_gen_expr function returns the proper output (instead of unitialized output).

 Revision 1.79  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.78  2006/10/05 21:43:17  phase1geo
 Added support for increment and decrement operators in expressions.  Also added
 proper parsing and handling support for immediate and postponed increment/decrement.
 Added inc3, inc3.1, dec3 and dec3.1 diagnostics to verify this new functionality.
 Still need to run regressions.

 Revision 1.77  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.76  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.75  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.74  2006/08/22 21:46:03  phase1geo
 Updating from stable branch.

 Revision 1.73  2006/08/21 22:49:59  phase1geo
 Adding more support for delayed assignments.  Added dly_assign1 to testsuite
 to verify the #... type of delayed assignment.  This seems to be working for
 this case but has a potential issue in the report generation.  Checkpointing
 work.

 Revision 1.72  2006/08/20 03:20:58  phase1geo
 Adding support for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, <<<=, >>>=, ++
 and -- operators.  The op-and-assign operators are currently good for
 simulation and code generation purposes but still need work in the comb.c
 file for proper combinational logic underline and reporting support.  The
 increment and decrement operations should be fully supported with the exception
 of their use in FOR loops (I'm not sure if this is supported by SystemVerilog
 or not yet).  Also started adding support for delayed assignments; however, I
 need to rework this completely as it currently causes segfaults.  Added lots of
 new diagnostics to verify this new functionality and updated regression for
 these changes.  Full IV regression now passes.

 Revision 1.71  2006/08/18 22:07:44  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.70  2006/08/11 18:57:03  phase1geo
 Adding support for always_comb, always_latch and always_ff statement block
 types.  Added several diagnostics to regression suite to verify this new
 behavior.

 Revision 1.69  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.68  2006/05/25 12:10:57  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.67.8.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.67.8.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.67  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.66  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.65  2006/03/22 21:34:26  phase1geo
 Fixing code generation bug.

 Revision 1.64  2006/03/20 16:43:38  phase1geo
 Fixing code generator to properly display expressions based on lines.  Regression
 still needs to be updated for these changes.

 Revision 1.63  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.62  2006/02/06 05:07:26  phase1geo
 Fixed expression_set_static_only function to consider static expressions
 properly.  Updated regression as a result of this change.  Added files
 for signed3 diagnostic.  Documentation updates for GUI (these are not quite
 complete at this time yet).

 Revision 1.61  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.60  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.59  2006/01/25 16:51:26  phase1geo
 Fixing performance/output issue with hierarchical references.  Added support
 for hierarchical references to parser.  Full regression passes.

 Revision 1.58  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.57  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.56  2006/01/13 04:01:04  phase1geo
 Adding support for exponential operation.  Added exponent1 diagnostic to verify
 but Icarus does not support this currently.

 Revision 1.55  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.54  2006/01/10 05:12:48  phase1geo
 Added arithmetic left and right shift operators.  Added ashift1 diagnostic
 to verify their correct operation.

 Revision 1.53  2005/12/17 05:47:36  phase1geo
 More memory fault fixes.  Regression runs cleanly and we have verified
 no memory faults up to define3.v.  Still have a ways to go.

 Revision 1.52  2005/12/16 23:26:41  phase1geo
 Last set of memory leak fixes to get all assign diagnostics to run cleanly.

 Revision 1.51  2005/12/14 23:25:50  phase1geo
 Checkpointing some more memory fault fixes.

 Revision 1.50  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.49  2005/12/13 23:15:14  phase1geo
 More fixes for memory leaks.  Regression fully passes at this point.

 Revision 1.48  2005/12/08 22:50:58  phase1geo
 Adding support for while loops.  Added while1 and while1.1 to regression suite.
 Ran VCS on regression suite and updated.  Full regression passes.

 Revision 1.47  2005/12/07 21:50:50  phase1geo
 Added support for repeat blocks.  Added repeat1 to regression and fixed errors.
 Full regression passes.

 Revision 1.46  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.45  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.44  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.43  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.42  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.41  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.40  2005/01/07 23:30:08  phase1geo
 Adding ability to handle strings in expressions.  Added string1.v diagnostic
 to verify this functionality.  Updated regressions for this change.

 Revision 1.39  2005/01/07 23:00:05  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.38  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.37  2004/03/17 13:25:00  phase1geo
 Fixing some more report-related bugs.  Added new diagnostics to regression
 suite to test for these.

 Revision 1.36  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.35  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.34  2003/12/18 15:21:18  phase1geo
 Fixing bug in code generator that caused incorrect data to be output and
 also segfaulted.  Full regression runs clean and line width support should
 be complete at this time.

 Revision 1.33  2003/12/16 23:22:07  phase1geo
 Adding initial code for line width specification for report output.

 Revision 1.32  2003/12/13 05:52:02  phase1geo
 Removed verbose output and updated development documentation for new code.

 Revision 1.31  2003/12/13 03:18:16  phase1geo
 Fixing bug found in difference between Linux and Irix snprintf() algorithm.
 Full regression now passes.

 Revision 1.30  2003/12/12 17:16:25  phase1geo
 Changing code generator to output logic based on user supplied format.  Full
 regression fails at this point due to mismatching report files.

 Revision 1.29  2003/11/30 05:46:44  phase1geo
 Adding IF report outputting capability.  Updated always9 diagnostic for these
 changes and updated rest of regression CDD files accordingly.

 Revision 1.28  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.27  2003/10/11 05:15:07  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.26  2002/12/05 14:45:17  phase1geo
 Removing assertion error from instance6.1 failure; however, this case does not
 work correctly according to instance6.2.v diagnostic.  Added @(...) output in
 report command for edge-triggered events.  Also fixed bug where a module could be
 parsed more than once.  Full regression does not pass at this point due to
 new instance6.2.v diagnostic.

 Revision 1.25  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.24  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.23  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.22  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.21  2002/10/25 03:44:39  phase1geo
 Fixing bug in comb.c that caused statically allocated string to be exceeded
 which caused memory corruption problems.  Full regression now passes.

 Revision 1.20  2002/10/24 23:19:38  phase1geo
 Making some fixes to report output.  Fixing bugs.  Added long_exp1.v diagnostic
 to regression suite which finds a current bug in the report underlining
 functionality.  Need to look into this.

 Revision 1.19  2002/10/23 03:39:06  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.18  2002/10/01 13:21:24  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.17  2002/07/16 00:05:31  phase1geo
 Adding support for replication operator (EXPAND).  All expressional support
 should now be available.  Added diagnostics to test replication operator.
 Rewrote binding code to be more efficient with memory use.

 Revision 1.16  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.15  2002/07/10 16:27:17  phase1geo
 Fixing output for single/multi-bit select signals in reports.

 Revision 1.14  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.13  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.12  2002/07/05 00:37:37  phase1geo
 Small update to CASE handling in scope to avoid future errors.

 Revision 1.11  2002/07/05 00:10:18  phase1geo
 Adding report support for case statements.  Everything outputs fine; however,
 I want to remove CASE, CASEX and CASEZ expressions from being reported since
 it causes redundant and misleading information to be displayed in the verbose
 reports.  New diagnostics to check CASE expressions have been added and pass.

 Revision 1.10  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.9  2002/07/03 19:54:35  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.8  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.7  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.6  2002/06/27 21:18:48  phase1geo
 Fixing report Verilog output.  simple.v verilog diagnostic now passes.

 Revision 1.5  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).
*/
