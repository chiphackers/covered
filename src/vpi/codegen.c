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


extern bool flag_use_line_width;
extern int  line_width;


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

  char* tmpstr;         /* Temporary string holder              */
  int   code_size = 0;  /* Length of current string to generate */
  int   i;              /* Loop iterator                        */

  assert( left_depth > 0 );

/*
  printf( "code_index: %d, first: %s, left_depth: %d, first_same: %d, middle: %s, right_depth: %d, last_same: %d, last: %s\n",
          code_index, first, left_depth, first_same_line, middle, right_depth, last_same_line, last );
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
 \param code             Pointer to an array of strings which will contain generated code lines.
 \param code_depth       Pointer to number of strings contained in code array.
 \param curr_line        Line number of current expression.
 \param first            String value coming before left expression string.
 \param left             Array of strings for left expression code.
 \param left_depth       Number of strings contained in left array.
 \param left_line        Line number of left expression.
 \param middle           String value coming after left expression string.
 \param right            Array of strings for right expression code.
 \param right_depth      Number of strings contained in right array.
 \param right_line       Line number of right expression.
 \param last             String value coming after right expression string.

 Allocates enough memory in code array to store all code lines for the current expression.
 Calls the helper function to actually generate code lines (to populate the code array).
*/
void codegen_create_expr( char*** code,
                          int*    code_depth,
                          int     curr_line,
                          char*   first,
                          char**  left,
                          int     left_depth,
                          int     left_line,
                          char*   middle,
                          char**  right,
                          int     right_depth,
                          int     right_line,
                          char*   last ) {

  int total_len = 0;  /* Total length of first, left, middle, right, and last strings */
  int i;              /* Loop iterator                                                */ 

  *code_depth = 0;

  if( (left_depth > 0) || (right_depth > 0) ) {

    /* Allocate enough storage in code array for these lines */
    if( left_depth > 0 ) {
      *code_depth += left_depth;
    }
    if( right_depth > 0 ) {
      *code_depth += right_depth;
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

      if( (left_depth > 0) && (right_depth > 0) && (left_line == right_line) ) {
        *code_depth -= 1;
      }
      if( (left_depth > 0) && (left_line > curr_line) ) {
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
      codegen_create_expr_helper( *code, 0, first, left, left_depth, (curr_line >= left_line), middle,
                                  right, right_depth, (left_line == right_line), last );
    }

  }

}

/*!
 \param expr        Pointer to root of expression tree to generate.
 \param parent_op   Operation of parent.  If our op is the same, no surrounding parenthesis is needed.
 \param code        Pointer to array of strings that will contain code lines for the supplied expression.
 \param code_depth  Pointer to number of strings contained in code array.

 Generates Verilog code from specified expression tree.  This Verilog
 snippet is used by the verbose coverage reporting functions for showing
 Verilog examples that were missed during simulation.  This code handles multi-line
 Verilog output by storing its information into the code and code_depth parameters.
*/
void codegen_gen_expr( expression* expr, int parent_op, char*** code, int* code_depth ) {

  char** right_code;                /* Pointer to the code that is generated by the right side of the expression */
  char** left_code;                 /* Pointer to the code that is generated by the left side of the expression  */
  int    left_code_depth  = 0;
  int    right_code_depth = 0;
  char   code_format[20];           /* Format for creating my_code string                                        */
  char*  tmpstr;                    /* Temporary string holder                                                   */
  char*  before;
  char*  after;

  if( expr != NULL ) {

    codegen_gen_expr( expr->left,  expr->op, &left_code,  &left_code_depth  );
    codegen_gen_expr( expr->right, expr->op, &right_code, &right_code_depth );

    if( expr->op == EXP_OP_LAST ) {

      /* Do nothing. */

    } else if( expr->op == EXP_OP_STATIC ) {

      *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
      *code_depth = 1;

      if( expr->value->suppl.part.base == DECIMAL ) {

        snprintf( code_format, 20, "%d", vector_to_int( expr->value ) );
        (*code)[0] = strdup_safe( code_format, __FILE__, __LINE__ );

      } else if( expr->value->suppl.part.base == QSTRING ) {

        tmpstr = vector_to_string( expr->value );
        (*code)[0] = (char*)malloc_safe( (strlen( tmpstr ) + 3), __FILE__, __LINE__ );
        snprintf( (*code)[0], (strlen( tmpstr ) + 3), "\"%s\"", tmpstr );

      } else { 

        (*code)[0] = vector_to_string( expr->value );

      }

    } else if( (expr->op == EXP_OP_SIG) || (expr->op == EXP_OP_PARAM) ) {

      assert( expr->sig != NULL );

      if( expr->sig->name[0] == '#' ) {
        tmpstr = expr->sig->name + 1;
      } else {
        tmpstr = expr->sig->name;
      }

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

    } else if( (expr->op == EXP_OP_SBIT_SEL) || (expr->op == EXP_OP_PARAM_SBIT) ) {

      assert( expr->sig != NULL );

      if( expr->sig->name[0] == '#' ) {
        tmpstr = (char*)malloc_safe( (strlen( expr->sig->name ) + 1), __FILE__, __LINE__ );
        snprintf( tmpstr, (strlen( expr->sig->name ) + 1), "%s[", (expr->sig->name + 1) );
      } else {
        tmpstr = (char*)malloc_safe( (strlen( expr->sig->name ) + 2), __FILE__, __LINE__ );
        snprintf( tmpstr, (strlen( expr->sig->name ) + 2), "%s[", expr->sig->name );
      }

      codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left->line, "]", NULL, 0, 0, NULL );

      free_safe( tmpstr );

    } else if( (expr->op == EXP_OP_MBIT_SEL) || (expr->op == EXP_OP_PARAM_MBIT) ) {

      assert( expr->sig != NULL );

      if( expr->sig->name[0] == '#' ) {
        tmpstr = (char*)malloc_safe( (strlen( expr->sig->name ) + 1), __FILE__, __LINE__ );
        snprintf( tmpstr, (strlen( expr->sig->name ) + 1), "%s[", (expr->sig->name + 1) );
      } else {
        tmpstr = (char*)malloc_safe( (strlen( expr->sig->name ) + 2), __FILE__, __LINE__ );
        snprintf( tmpstr, (strlen( expr->sig->name ) + 2), "%s[", expr->sig->name );
      }

      if( ESUPPL_WAS_SWAPPED( expr->suppl ) ) {
        codegen_create_expr( code, code_depth, expr->line, tmpstr, right_code, right_code_depth, expr->right->line, ":",
                             left_code, left_code_depth, expr->left->line, "]" );
      } else {
        codegen_create_expr( code, code_depth, expr->line, tmpstr, left_code, left_code_depth, expr->left->line, ":",
                             right_code, right_code_depth, expr->right->line, "]" );
      }

    } else if( expr->op == EXP_OP_DEFAULT ) {

      *code       = (char**)malloc_safe( sizeof( char* ), __FILE__, __LINE__ );
      (*code)[0]  = strdup_safe( "default :", __FILE__, __LINE__ );
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
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " ^ ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_MULTIPLY :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " * ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_DIVIDE   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " / ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_MOD      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " %% ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_ADD      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " + ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_SUBTRACT :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " - ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_AND      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " & ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_OR       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " | ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_NAND     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " ~& ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_NOR      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " ~| ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_NXOR     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " ~^ ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_LT       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " < ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_GT       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " > ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_LSHIFT   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " << ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_RSHIFT   :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " >> ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_EQ       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " == ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_CEQ      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " === ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_LE       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " <= ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_GE       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " >= ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_NE       :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " != ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_CNE      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " !== ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_LOR      :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " || ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_LAND     :
          codegen_create_expr( code, code_depth, expr->line, before, left_code, left_code_depth, expr->left->line, " && ",
                               right_code, right_code_depth, expr->right->line, after );
          break;
        case EXP_OP_COND     :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left->line, " ? ",
                               right_code, right_code_depth, expr->right->line, NULL );
          break;
        case EXP_OP_COND_SEL :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left->line, " : ",
                               right_code, right_code_depth, expr->right->line, NULL );
          break;
        case EXP_OP_UINV     :
          codegen_create_expr( code, code_depth, expr->line, "~", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UAND     :
          codegen_create_expr( code, code_depth, expr->line, "&", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UNOT     :
          codegen_create_expr( code, code_depth, expr->line, "!", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UOR      :
          codegen_create_expr( code, code_depth, expr->line, "|", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UXOR     :
          codegen_create_expr( code, code_depth, expr->line, "^", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UNAND    :
          codegen_create_expr( code, code_depth, expr->line, "~&", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UNOR     :
          codegen_create_expr( code, code_depth, expr->line, "~|", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_UNXOR    :
          codegen_create_expr( code, code_depth, expr->line, "~^", right_code, right_code_depth, expr->right->line, NULL,
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_EXPAND   :
          codegen_create_expr( code, code_depth, expr->line, "{", left_code, left_code_depth, expr->left->line, "{",
                               right_code, right_code_depth, expr->right->line, "}}" );
          break;
        case EXP_OP_LIST     :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left->line, ", ",
                               right_code, right_code_depth, expr->right->line, NULL );
          break;
        case EXP_OP_CONCAT   :
          codegen_create_expr( code, code_depth, expr->line, "{", right_code, right_code_depth, expr->right->line, "}",
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_PEDGE    :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(posedge ", right_code, right_code_depth, expr->right->line, ")",
                                 NULL, 0, 0, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, "posedge ", right_code, right_code_depth, expr->right->line, NULL,
                                 NULL, 0, 0, NULL );
          }
          break;
        case EXP_OP_NEDGE    :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(negedge ", right_code, right_code_depth, expr->right->line, ")",
                                 NULL, 0, 0, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, "negedge ", right_code, right_code_depth, expr->right->line, NULL,
                                 NULL, 0, 0, NULL );
          }
          break;
        case EXP_OP_AEDGE    :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(", right_code, right_code_depth, expr->right->line, ")",
                                 NULL, 0, 0, NULL );
          } else {
            codegen_create_expr( code, code_depth, expr->line, NULL, right_code, right_code_depth, expr->right->line, NULL,
                                 NULL, 0, 0, NULL );
          }
          break;
        case EXP_OP_EOR      :
          if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
            codegen_create_expr( code, code_depth, expr->line, "@(", left_code, left_code_depth, expr->left->line, " or ",
                                 right_code, right_code_depth, expr->right->line, ")" );
          } else {
            codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left->line, " or ",
                                 right_code, right_code_depth, expr->right->line, NULL );
          }
          break;
        case EXP_OP_CASE     :
          codegen_create_expr( code, code_depth, expr->line, "case( ", left_code, left_code_depth, expr->left->line, " ) ",
                               right_code, right_code_depth, expr->right->line, " :" );
          break;
        case EXP_OP_CASEX    :
          codegen_create_expr( code, code_depth, expr->line, "casex( ", left_code, left_code_depth, expr->left->line, " ) ",
                               right_code, right_code_depth, expr->right->line, " :" );
          break;
        case EXP_OP_CASEZ    :
          codegen_create_expr( code, code_depth, expr->line, "casez( ", left_code, left_code_depth, expr->left->line, " ) ",
                               right_code, right_code_depth, expr->right->line, " :" );
          break;
        case EXP_OP_DELAY    :
          codegen_create_expr( code, code_depth, expr->line, "#(", right_code, right_code_depth, expr->right->line, ")",
                               NULL, 0, 0, NULL );
          break;
        case EXP_OP_ASSIGN   :
          codegen_create_expr( code, code_depth, expr->line, "assign ", left_code, left_code_depth, expr->left->line, " = ",
                               right_code, right_code_depth, expr->right->line, NULL );
          break;
        case EXP_OP_BASSIGN  :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left->line, " = ",
                               right_code, right_code_depth, expr->right->line, NULL );
          break;
        case EXP_OP_NASSIGN  :
          codegen_create_expr( code, code_depth, expr->line, NULL, left_code, left_code_depth, expr->left->line, " <= ",
                               right_code, right_code_depth, expr->right->line, NULL );
          break;
        case EXP_OP_IF       :
          codegen_create_expr( code, code_depth, expr->line, "if( ", right_code, right_code_depth, expr->right->line, " )",
                               NULL, 0, 0, NULL );
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
