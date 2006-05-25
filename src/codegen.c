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
 \author   Trevor Williams  (trevorw@charter.net)
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


extern bool flag_use_line_width;
extern int  line_width;
extern char user_msg[USER_MSG_LENGTH];


/*!
 \param code             Array of strings containing generated code lines.
 \param code_index       Index to current string in code array to set.
 \param first            String value coming before left expression string.
 \param left             Array of strings for left expression code.
 \param left_depth       Number of strings contained in left array.
 \param first_same_line  Set to TRUE if left expression is on the same line as the current expression.
 \param middle           String value coming after left expression string.
 \param right            Array of strings for right expression code.
 \param right_depth      Number of strings contained in right array.
 \param last_same_line   Set to TRUE if right expression is on the same line as the left expression.
 \param last             String value coming after right expression string.

 Generates multi-line expression code strings from current, left, and right expressions.
*/
void codegen_create_expr_helper( char** code,
                                 int    code_index,
                                 char*  first,
                                 char** left,
                                 int    left_depth,
                                 bool   first_same_line,
                                 char*  middle,
                                 char** right,
                                 int    right_depth,
                                 bool   last_same_line,
                                 char*  last ) {

  char* tmpstr;         /* Temporary string holder */
  int   code_size = 0;  /* Length of current string to generate */
  int   i;              /* Loop iterator */

  assert( left_depth > 0 );

/*
  printf( "code_index: %d, first: %s, left_depth: %d, first_same: %d, middle: %s, right_depth: %d, last_same: %d, last: %s\n",
          code_index, first, left_depth, first_same_line, middle, right_depth, last_same_line, last );
  printf( "left code:\n" );
  for( i=0; i<left_depth; i++ ) {
    printf( "  %s\n", left[i] );
  }
  printf( "right_code:\n" );
  for( i=0; i<right_depth; i++ ) {
    printf( "  %s\n", right[i] );
  }
*/

  if( first != NULL ) {
    code_size += strlen( first );
  }
  if( first_same_line ) {
    code_size += strlen( left[0] );
    if( (left_depth == 1) && (middle != NULL) ) {
      code_size += strlen( middle );
    }
  }
  if( code[code_index] != NULL ) {
    free_safe( code[code_index] );
  }
  code[code_index]    = (char*)malloc_safe( (code_size + 1), __FILE__, __LINE__ );
  code[code_index][0] = '\0';

  if( first != NULL ) {
    snprintf( code[code_index], (code_size + 1), "%s", first );
  }
  if( first_same_line ) {
    tmpstr = strdup_safe( code[code_index], __FILE__, __LINE__ );
    snprintf( code[code_index], (code_size + 1), "%s%s", tmpstr, left[0] );
    free_safe( tmpstr );
    free_safe( left[0] );
    if( (left_depth == 1) && (middle != NULL) ) {
      code_size = strlen( code[code_index] ) + strlen( middle );
      tmpstr    = (char*)malloc_safe( (code_size + 1), __FILE__, __LINE__ );
      snprintf( tmpstr, (code_size + 1), "%s%s", code[code_index], middle );
      if( right_depth > 0 ) {
        codegen_create_expr_helper( code, code_index, tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
        free_safe( tmpstr );
      } else {
        free_safe( code[code_index] );
        code[code_index] = tmpstr;
      }
    } else {
      if( middle != NULL ) {
        for( i=1; i<(left_depth - 1); i++ ) {
          code[code_index+i] = left[i];
        }
        code_size = strlen( left[i] ) + strlen( middle );
        tmpstr    = (char*)malloc_safe( (code_size + 1), __FILE__, __LINE__ );
        snprintf( tmpstr, (code_size + 1), "%s%s", left[i], middle );
        free_safe( left[i] );
        if( right_depth > 0 ) {
          codegen_create_expr_helper( code, (code_index + i), tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
          free_safe( tmpstr );
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
      tmpstr    = (char*)malloc_safe( (code_size + 1), __FILE__, __LINE__ );
      snprintf( tmpstr, (code_size + 1), "%s%s", left[i], middle );
      free_safe( left[i] );
      if( right_depth > 0 ) {
        codegen_create_expr_helper( code, (code_index + i), tmpstr, right, right_depth, last_same_line, last, NULL, 0, FALSE, NULL );
        free_safe( tmpstr );
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
 \param code         Pointer to an array of strings which will contain generated code lines.
 \param code_depth   Pointer to number of strings contained in code array.
 \param curr_line    Line number of current expression.
 \param first        String value coming before left expression string.
 \param left         Array of strings for left expression code.
 \param left_depth   Number of strings contained in left array.
 \param left_exp     Pointer to left expression
 \param middle       String value coming after left expression string.
 \param right        Array of strings for right expression code.
 \param right_depth  Number of strings contained in right array.
 \param right_exp    Pointer to right expression
 \param last         String value coming after right expression string.

 Allocates enough memory in code array to store all code lines for the current expression.
 Calls the helper function to actually generate code lines (to populate the code array).
*/
void codegen_create_expr( char***     code,
                          int*        code_depth,
                          int         curr_line,
                          char*       first,
                          char**      left,
                          int         left_depth,
                          expression* left_exp,
                          char*       middle,
                          char**      right,
                          int         right_depth,
                          expression* right_exp,
                          char*       last ) {

  int         total_len = 0;    /* Total length of first, left, middle, right, and last strings */
  int         i;                /* Loop iterator */ 
  expression* last_exp;         /* Last expression of left expression tree */
  expression* first_exp;        /* First expression of right expression tree */
  int         left_line_start;  /* Line number of first expression of left expression tree */
  int         left_line_end;    /* Line number of last expression of left expression tree */
  int         right_line;       /* Line number of first expression of right expression tree */

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

    *code = (char**)malloc_safe( (sizeof( char* ) * (*code_depth)), __FILE__, __LINE__ );
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
void codegen_gen_expr( expression* expr, int parent_op, char*** code, int* code_depth, func_unit* funit ) {

  char**     right_code;            /* Pointer to the code that is generated by the right side of the expression */
  char**     left_code;             /* Pointer to the code that is generated by the left side of the expression */
  int        left_code_depth  = 0;
  int        right_code_depth = 0;
  char       code_format[20];       /* Format for creating my_code string */
  char*      tmpstr;                /* Temporary string holder */
  char*      before;
  char*      after;
  func_unit* tfunit;
  char*      pname;                 /* Printable version of signal name */

  if( expr != NULL ) {

    /* Only traverse left and right expression trees if we are not an SLIST */
    if( expr->op != EXP_OP_SLIST ) {

      codegen_gen_expr( expr->left,  expr->op, &left_code,  &left_code_depth,  funit );
      codegen_gen_expr( expr->right, expr->op, &right_code, &right_code_depth, funit );

    }

    if( (expr->op == EXP_OP_LAST) || (expr->op == EXP_OP_NB_CALL) ) {

      /* Do nothing. */

    } else if( expr->op == EXP_OP_STATIC ) {

      *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
      *code_depth = 1;

      if( expr->value->suppl.part.base == DECIMAL ) {

        snprintf( code_format, 20, "%d", vector_to_int( expr->value ) );
        if( (strlen( code_format ) == 1) && (expr->parent->expr->op == EXP_OP_NEGATE) ) {
          strcat( code_format, " " );
        }
        (*code)[0] = strdup_safe( code_format, __FILE__, __LINE__ );

      } else if( expr->value->suppl.part.base == QSTRING ) {

        tmpstr = vector_to_string( expr->value );
        (*code)[0] = (char*)malloc_safe( (strlen( tmpstr ) + 3), __FILE__, __LINE__ );
        snprintf( (*code)[0], (strlen( tmpstr ) + 3), "\"%s\"", tmpstr );
        free_safe( tmpstr );

      } else { 

        (*code)[0] = vector_to_string( expr->value );

      }

    } else if( (expr->op == EXP_OP_SIG) || (expr->op == EXP_OP_PARAM) ) {

#ifdef OBSOLETE
      assert( expr->sig != NULL );

      if( expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
        tmpstr = scope_gen_printable( expr->sig->name );
      } else {
#endif
        tmpstr = scope_gen_printable( expr->name );
#ifdef OBSOLETE
      }
#endif

      switch( strlen( tmpstr ) ) {
        case 0 :  assert( strlen( tmpstr ) > 0 );  break;
        case 1 :
          *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
          (*code)[0]  = (char*)malloc_safe( 4, __FILE__, __LINE__ );
          *code_depth = 1;
          snprintf( (*code)[0], 4, " %s ", tmpstr );
          break;
        case 2 :
          *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
          (*code)[0]  = (char*)malloc_safe( 4, __FILE__, __LINE__ );
          *code_depth = 1;
          snprintf( (*code)[0], 4, " %s", tmpstr );
          break;
        default :
          *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
          (*code)[0]  = strdup_safe( tmpstr, __FILE__, __LINE__ );
          *code_depth = 1;
          break;
      }

      free_safe( tmpstr );

    } else if( (expr->op == EXP_OP_SBIT_SEL) || (expr->op == EXP_OP_PARAM_SBIT) ) {

#ifdef OBSOLETE
      assert( expr->sig != NULL );

      if( expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
        pname = scope_gen_printable( expr->sig->name );
      } else {
#endif
        pname = scope_gen_printable( expr->name );
#ifdef OBSOLETE
      }
#endif

      tmpstr = (char*)malloc_safe( (strlen( pname ) + 2), __FILE__, __LINE__ );
      snprintf( tmpstr, (strlen( pname ) + 2), "%s[", pname );

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth,
                           expr->left, "]", NULL, 0, NULL, NULL );

      free_safe( tmpstr );
      free_safe( pname );

    } else if( (expr->op == EXP_OP_MBIT_SEL) || (expr->op == EXP_OP_PARAM_MBIT) ) {

#ifdef OBSOLETE
      assert( expr->sig != NULL );

      if( expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
        pname = scope_gen_printable( expr->sig->name );
      } else {
#endif
        pname = scope_gen_printable( expr->name );
#ifdef OBSOLETE
      }
#endif

      tmpstr = (char*)malloc_safe( (strlen( pname ) + 2), __FILE__, __LINE__ );
      snprintf( tmpstr, (strlen( pname ) + 2), "%s[", pname );

      if( ESUPPL_WAS_SWAPPED( expr->suppl ) ) {
        codegen_create_expr( code, code_depth, expr->line, tmpstr,
                             right_code, right_code_depth, expr->right, ":",
                             left_code, left_code_depth, expr->left, "]" );
      } else {
        codegen_create_expr( code, code_depth, expr->line, tmpstr,
                             left_code, left_code_depth, expr->left, ":",
                             right_code, right_code_depth, expr->right, "]" );
      }

      free_safe( tmpstr );
      free_safe( pname );

    } else if( (expr->op == EXP_OP_MBIT_POS) || (expr->op == EXP_OP_PARAM_MBIT_POS) ) {

#ifdef OBSOLETE
      assert( expr->sig != NULL );

      if( expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
        pname = scope_gen_printable( expr->sig->name );
      } else {
#endif
        pname = scope_gen_printable( expr->name );
#ifdef OBSOLETE
      }
#endif

      tmpstr = (char*)malloc_safe( (strlen( pname ) + 2), __FILE__, __LINE__ );
      snprintf( tmpstr, (strlen( pname ) + 2), "%s[", pname );

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left, "+:",
                           right_code, right_code_depth, expr->right, "]" );

      free_safe( tmpstr );
      free_safe( pname );

    } else if( (expr->op == EXP_OP_MBIT_NEG) || (expr->op == EXP_OP_PARAM_MBIT_NEG) ) {

#ifdef OBSOLETE
      assert( expr->sig != NULL );

      if( expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
        pname = scope_gen_printable( expr->sig->name );
      } else {
#endif
        pname = scope_gen_printable( expr->name );
#ifdef OBSOLETE
      }
#endif

      tmpstr = (char*)malloc_safe( (strlen( pname ) + 2), __FILE__, __LINE__ );
      snprintf( tmpstr, (strlen( pname ) + 2), "%s[", pname );

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left, "-:",
                           right_code, right_code_depth, expr->right, "]" );

      free_safe( tmpstr );
      free_safe( pname );

    } else if( (expr->op == EXP_OP_FUNC_CALL) || (expr->op == EXP_OP_TASK_CALL) ) {

      assert( expr->stmt != NULL );

      if( (tfunit = funit_find_by_id( expr->stmt->exp->id )) != NULL ) {
        after = (char*)malloc_safe( (strlen( tfunit->name ) + 1), __FILE__, __LINE__ );
        scope_extract_back( tfunit->name, after, user_msg );
        pname = scope_gen_printable( after );
        if( (expr->op == EXP_OP_TASK_CALL) && (expr->left == NULL) ) {
          *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
          (*code)[0]  = strdup_safe( pname, __FILE__, __LINE__ );
          *code_depth = 1;
        } else {
          tmpstr = (char*)malloc_safe( (strlen( pname ) + 3), __FILE__, __LINE__ );
          snprintf( tmpstr, (strlen( pname ) + 3), "%s( ", pname );
          codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left, " )", NULL, 0, NULL, NULL );
          free_safe( tmpstr );
        }
        free_safe( after );
        free_safe( pname );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Unable to find statement %d in module %s's task/function list",
                  expr->stmt->exp->id, funit->name );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }

    } else if( expr->op == EXP_OP_TRIGGER ) {

      assert( expr->sig != NULL );

      pname  = scope_gen_printable( expr->name );
      tmpstr = (char*)malloc_safe( (strlen( pname ) + 3), __FILE__, __LINE__ );
      snprintf( tmpstr, (strlen( pname ) + 3), "->%s", pname );

      *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
      (*code)[0]  = strdup_safe( tmpstr, __FILE__, __LINE__ );
      *code_depth = 1;

      free_safe( tmpstr );
      free_safe( pname );

    } else if( expr->op == EXP_OP_DEFAULT ) {

      *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
      (*code)[0]  = strdup_safe( "default :", __FILE__, __LINE__ );
      *code_depth = 1;

    } else if( expr->op == EXP_OP_SLIST ) {

      *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
      (*code)[0]  = strdup_safe( "@*", __FILE__, __LINE__ );
      *code_depth = 1;

    } else {

      if( parent_op == expr->op ) {
        before = NULL;
        after  = NULL;
      } else {
        before = strdup_safe( "(", __FILE__, __LINE__ );
        after  = strdup_safe( ")", __FILE__, __LINE__ );
      }


      switch( expr->op ) {
        case EXP_OP_XOR      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " ^ ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_MULTIPLY :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " * ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_DIVIDE   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " / ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_MOD      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " %% ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ADD      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " + ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_SUBTRACT :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " - ",
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
        case EXP_OP_OR       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " | ",
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
        case EXP_OP_ALSHIFT  :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " <<< ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_RSHIFT   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >> ",
                               right_code, right_code_depth, expr->right, after );
          break;
        case EXP_OP_ARSHIFT   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left, " >>> ",
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
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(posedge ", right_code, right_code_depth, expr->right, ")",
                                 NULL, 0, NULL, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, "posedge ", right_code, right_code_depth, expr->right, NULL,
                                 NULL, 0, NULL, NULL );
          }
          break;
        case EXP_OP_NEDGE    :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(negedge ", right_code, right_code_depth, expr->right, ")",
                                 NULL, 0, NULL, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, "negedge ", right_code, right_code_depth, expr->right, NULL,
                                 NULL, 0, NULL, NULL );
          }
          break;
        case EXP_OP_AEDGE    :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(", right_code, right_code_depth, expr->right, ")",
                                 NULL, 0, NULL, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, NULL, right_code, right_code_depth, expr->right, NULL,
                                 NULL, 0, NULL, NULL );
          }
          break;
        case EXP_OP_EOR      :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
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
        case EXP_OP_NEGATE   :
          codegen_create_expr( code, code_depth, expr->line, "-", right_code, right_code_depth, expr->right, NULL,
                               NULL, 0, NULL, NULL );
          break;
        default:  break;
      }

      if( parent_op != expr->op ) {
        free_safe( before );
        free_safe( after );
      }

    }

    if( right_code_depth > 0 ) {
      free_safe( right_code );
    }

    if( left_code_depth > 0 ) {
      free_safe( left_code );
    }

  }

}


/*
 $Log$
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
