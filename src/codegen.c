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
#include "generator.h"
#include "obfuscate.h"


extern bool           flag_use_line_width;
extern int            line_width;
extern char           user_msg[USER_MSG_LENGTH];
extern const exp_info exp_op_info[EXP_OP_NUM];


/*!
 Set this value to TRUE to cause signal names to be used as is.
*/
bool use_actual_names = FALSE;


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
        codegen_create_expr_helper( code, (code_index + i + 1), tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
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
 Generates Verilog code from specified expression tree.  This Verilog
 snippet is used by the verbose coverage reporting functions for showing
 Verilog examples that were missed during simulation.  This code handles multi-line
 Verilog output by storing its information into the code and code_depth parameters.
*/
static void codegen_gen_expr1(
            expression*   expr,        /*!< Pointer to root of expression tree to generate */
            exp_op_type   parent_op,   /*!< Operation of parent.  If our op is the same, no surrounding parenthesis is needed */
  /*@out@*/ char***       code,        /*!< Pointer to array of strings that will contain code lines for the supplied expression */
  /*@out@*/ unsigned int* code_depth,  /*!< Pointer to number of strings contained in code array */
            func_unit*    funit,       /*!< Pointer to functional unit containing the specified expression */
            bool          inline_exp   /*!< If set to TRUE, generates a unique expression name for any subexpression that will have its
                                            value calculated by intermediate code (only valid for combinational coverage inlined output) */
) { PROFILE(CODEGEN_GEN_EXPR1);

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

    /* If the current expression will be calculated intermediately, just output the intermediate expression name */
    if( inline_exp && generator_expr_name_needed( expr ) ) {
       
      *code       = (char**)malloc_safe( sizeof( char* ) );
      (*code)[0]  = generator_create_expr_name( expr );
      *code_depth = 1;

    /* Otherwise, calculate the value as is deemed necessary */
    } else {

      /* Only traverse left and right expression trees if we are not an SLIST-type */
      if( (expr->op != EXP_OP_SLIST) && (expr->op != EXP_OP_ALWAYS_COMB) && (expr->op != EXP_OP_ALWAYS_LATCH) ) {

        codegen_gen_expr1( expr->left,  expr->op, &left_code,  &left_code_depth,  funit, inline_exp );
        codegen_gen_expr1( expr->right, expr->op, &right_code, &right_code_depth, funit, inline_exp );

      }

      if( (expr->op == EXP_OP_LAST) || (expr->op == EXP_OP_NB_CALL) || (expr->op == EXP_OP_JOIN) || (expr->op == EXP_OP_FORK) ||
          ((parent_op == EXP_OP_REPEAT) && (expr->parent->expr->left == expr)) ) {

        /* Do nothing. */
        *code_depth = 0;

      } else if( expr->op == EXP_OP_STATIC ) {

        unsigned int data_type = expr->value->suppl.part.data_type;

        *code       = (char**)malloc_safe( sizeof( char* ) );
        *code_depth = 1;

        if( data_type == VDATA_R64 ) {

          assert( expr->value->value.r64->str != NULL );
          (*code)[0] = strdup_safe( expr->value->value.r64->str );

        } else if( data_type == VDATA_R32 ) {

          assert( expr->value->value.r32->str != NULL );
          (*code)[0] = strdup_safe( expr->value->value.r32->str );

        } else {

          if( ESUPPL_STATIC_BASE( expr->suppl ) == DECIMAL ) {

            rv = snprintf( code_format, 20, "%d", vector_to_int( expr->value ) );
            assert( rv < 20 );
            if( (strlen( code_format ) == 1) && (expr->parent->expr->op == EXP_OP_NEGATE) ) {
              strcat( code_format, " " );
            }
            (*code)[0] = strdup_safe( code_format );

          } else if( ESUPPL_STATIC_BASE( expr->suppl ) == QSTRING ) {

            unsigned int slen;
            tmpstr = vector_to_string( expr->value, QSTRING, FALSE, 0 );
            slen   = strlen( tmpstr ) + 3;
            (*code)[0] = (char*)malloc_safe( slen );
            rv = snprintf( (*code)[0], slen, "\"%s\"", tmpstr );
            assert( rv < slen );
            free_safe( tmpstr, (strlen( tmpstr ) + 1) );

          } else { 

            (*code)[0] = vector_to_string( expr->value, ESUPPL_STATIC_BASE( expr->suppl ), FALSE, 0 );

          }
   
        }

      } else if( (expr->op == EXP_OP_SIG) || (expr->op == EXP_OP_PARAM) ) {

        if( use_actual_names ) {
          tmpstr = strdup_safe( obf_sig( expr->name ) );
        } else {
          tmpstr = scope_gen_printable( expr->name );
        }

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
          if( use_actual_names ) {
            pname = strdup_safe( obf_sig( expr->name ) ); 
          } else {
            pname = scope_gen_printable( expr->name );
          }
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
          if( use_actual_names ) {
            pname = strdup_safe( obf_sig( expr->name ) );
          } else {
            pname = scope_gen_printable( expr->name );
          }
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
          if( use_actual_names ) {
            pname = strdup_safe( obf_sig( expr->name ) );
          } else {
            pname = scope_gen_printable( expr->name );
          } 
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
          if( use_actual_names ) {
            pname = strdup_safe( obf_sig( expr->name ) );
          } else {
            pname = scope_gen_printable( expr->name );
          }
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
        if( use_actual_names ) {
          pname = strdup_safe( obf_sig( after ) );
        } else {
          pname = scope_gen_printable( after );
        }
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
        if( use_actual_names ) {
          pname = strdup_safe( obf_sig( expr->name ) );
        } else {
          pname = scope_gen_printable( expr->name );
        }
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
        if( use_actual_names ) {
          pname = strdup_safe( obf_sig( expr->name ) );
        } else {
          pname = scope_gen_printable( expr->name );
        }
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

      } else if( expr->op == EXP_OP_STIME ) {
   
        *code       = (char**)malloc_safe( sizeof( char* ) );
        (*code)[0]  = strdup_safe( "$time" );
        *code_depth = 1;

      } else if( (expr->op == EXP_OP_SRANDOM) && (expr->left == NULL) ) {

        *code       = (char**)malloc_safe( sizeof( char* ) );
        (*code)[0]  = strdup_safe( "$random" );
        *code_depth = 1;

      } else if( (expr->op == EXP_OP_SURANDOM) && (expr->left == NULL) ) {
 
        *code       = (char**)malloc_safe( sizeof( char* ) );
        (*code)[0]  = strdup_safe( "$urandom" );
        *code_depth = 1;

      } else {

        if( expr->suppl.part.parenthesis ) {
          before = strdup_safe( "(" );
          after  = strdup_safe( ")" );
        } else {
          before = NULL;
          after  = NULL;
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
            before = expr->suppl.part.parenthesis ? strdup_safe( "(~" ) : strdup_safe( "~" );
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(~" : "~"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UAND     :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(&" : "&"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UNOT     :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(!" : "!"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UOR      :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(|" : "|"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UXOR     :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(^" : "^"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UNAND    :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(~&" : "~&"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UNOR     :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(~|" : "~|"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_UNXOR    :
            codegen_create_expr( code, code_depth, expr->line, (expr->suppl.part.parenthesis ? "(~^" : "~^"), right_code, right_code_depth, expr->right, after,
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_EXPAND   :
            codegen_create_expr( code, code_depth, expr->line, (inline_exp ? "{1'b0,{" : "{"), left_code, left_code_depth, expr->left, "{",
                                 right_code, right_code_depth, expr->right, (inline_exp ? "}}}" : "}}") );
            break;
          case EXP_OP_LIST     :
          case EXP_OP_PLIST    :
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
          case EXP_OP_RASSIGN  :
          case EXP_OP_BASSIGN  :
            codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " = ",
                                 right_code, right_code_depth, expr->right, NULL );
            break;
          case EXP_OP_NASSIGN  :
            codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left, " <= ",
                                 right_code, right_code_depth, expr->right, NULL );
            break;
          case EXP_OP_PASSIGN  :
          case EXP_OP_SASSIGN  :
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
          case EXP_OP_SSIGNED  :
            codegen_create_expr( code, code_depth, expr->line, "$signed( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SUNSIGNED  :
            codegen_create_expr( code, code_depth, expr->line, "$unsigned( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SCLOG2 :
            codegen_create_expr( code, code_depth, expr->line, "$clog2( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SRANDOM  :
            codegen_create_expr( code, code_depth, expr->line, "$random( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SURANDOM :
            codegen_create_expr( code, code_depth, expr->line, "$urandom( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SURAND_RANGE :
            codegen_create_expr( code, code_depth, expr->line, "$urandom_range( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SSRANDOM :
            codegen_create_expr( code, code_depth, expr->line, "$srandom( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SB2R :
            codegen_create_expr( code, code_depth, expr->line, "$bitstoreal( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SR2B :
            codegen_create_expr( code, code_depth, expr->line, "$realtobits( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SI2R :
            codegen_create_expr( code, code_depth, expr->line, "$itor( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SR2I :
            codegen_create_expr( code, code_depth, expr->line, "$rtoi( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SSR2B :
            codegen_create_expr( code, code_depth, expr->line, "$shortrealtobits( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SB2SR :
            codegen_create_expr( code, code_depth, expr->line, "$bitstoshortreal( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_STESTARGS :
            codegen_create_expr( code, code_depth, expr->line, "$test$plusargs( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          case EXP_OP_SVALARGS :
            codegen_create_expr( code, code_depth, expr->line, "$value$plusargs( ", left_code, left_code_depth, expr->left, " )",
                                 NULL, 0, NULL, NULL );
            break;
          default:  break;
        }

        /* Deallocate before and after strings */
        free_safe( before, (strlen( before ) + 1) );
        free_safe( after, (strlen( after ) + 1) );

      }

      if( right_code_depth > 0 ) {
        free_safe( right_code, (sizeof( char* ) * right_code_depth) );
      }

      if( left_code_depth > 0 ) {
        free_safe( left_code, (sizeof( char* ) * left_code_depth) );
      }

    }

  }

}

/*!
 Generates the code segment for the given expression tree.  This output maintains the original formatting unless
 the user specified the -w option to the command-line (for report command only).
*/
void codegen_gen_expr(
            expression*   expr,       /*!< Pointer to root of expression tree to generate */
            func_unit*    funit,      /*!< Pointer to functional unit containing the specified expression */
  /*@out@*/ char***       code,       /*!< Pointer to array of strings that will contain code lines for the supplied expression */
  /*@out@*/ unsigned int* code_depth  /*!< Pointer to number of strings contained in code array */
) { PROFILE(CODEGEN_GEN_EXPR);

  if( expr != NULL ) {
    codegen_gen_expr1( expr, expr->op, code, code_depth, funit, FALSE ); 
  }

  PROFILE_END;

}

/*!
 \return Returns generated expression string from the given expression.

 Wrapper function around the codegen_gen_expr function that emits one line for the entire expression.
*/
char* codegen_gen_expr_one_line(
  expression*  expr,       /*!< Pointer to the expression to generate code for */
  func_unit*   funit,      /*!< Pointer to functional unit containing expr */
  bool         inline_exp  /*!< If set to TRUE, generates inlined expression names in place of expressions that will be
                                calculated as intermediate expressions (this only occurs when generating combinational
                                logic coverage for inlined code coverage output. */
) { PROFILE(CODEGEN_GEN_EXPR_ONE_LINE);

  char* code_str = NULL;

  if( expr != NULL ) {

    bool         orig_flag_use_line_width = flag_use_line_width;
    int          orig_line_width          = line_width;
    bool         orig_use_actual_names    = use_actual_names;
    char**       code_array;
    unsigned int code_depth;

    /* Set the line width to the maximum value */
    flag_use_line_width = TRUE;
    line_width          = 0x7fffffff;
    use_actual_names    = TRUE;

    /* Generate the code */
    codegen_gen_expr1( expr, expr->op, &code_array, &code_depth, funit, inline_exp );

    /* The number of elements in the code_array array should only be one */
    assert( code_depth == 1 );

    /* Save the line for returning purposes */
    code_str = code_array[0];

    /* Deallocate the string array */
    free_safe( code_array, (sizeof( char* ) * code_depth) );

    /* Restore the original values of flag_use_line_width and line_width */
    flag_use_line_width = orig_flag_use_line_width;
    line_width          = orig_line_width;
    use_actual_names    = orig_use_actual_names;

  }

  PROFILE_END;
 
  return( code_str );

}

