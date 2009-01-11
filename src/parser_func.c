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

/*!
 \file    parser_func.c
 \author  Trevor Williams  (phase1geo@gmail.com)
 \date    12/1/2008
*/

#include "db.h"
#include "defines.h"
#include "parser_func.h"
#include "parser_misc.h"
#include "profiler.h"
#include "static.h"


/*!
 If set to a value > 0, specifies that we are currently parsing code that should be ignored by
 Covered; otherwise, evaluate parsed code.
*/
int ignore_mode = 0;

/*!
 Specifies the current signal/port type for a variable list.
*/
int curr_sig_type = SSUPPL_TYPE_DECL_NET;

/*!
 Contains the last explicitly or implicitly defines signed indication.
*/
bool curr_signed = FALSE;

/*!
 Contains the last explicitly or implicitly defined packed range information.  This is needed because lists
 of signals/parameters can be made using a previously set range.
*/
sig_range curr_prange;

/*!
 Contains the last explicitly or implicitly defined unpacked range information.  This is needed because lists
 of signals/parameters can be made using a previously set range.
*/
sig_range curr_urange;

/*!
 If set to TRUE, specifies that we are currently parsing syntax on the left-hand-side of an
 assignment.
*/
bool lhs_mode = FALSE;



/*!
 Checks the validity of the (*...*) syntax.
*/
void parser_check_pstar() { PROFILE(PARSER_CHECK_PSTAR);

  if( (ignore_mode == 0) && !parser_check_generation( GENERATION_2001 ) ) {
    VLerror( "Attribute syntax found in block that is specified to not allow Verilog-2001 syntax" );
    ignore_mode++;
  }

  PROFILE_END;

}

/*!
 Parses the given attribute list for a valid Covered attribute and handles it accordingly.
*/
void parser_check_attribute(
  attr_param* ap  /*!< Pointer to attribute parameter */
) { PROFILE(PARSER_CHECK_ATTRIBUTE);

  if( !parser_check_generation( GENERATION_2001 ) ) {
    ignore_mode--;
  } else if( ignore_mode == 0 ) {
    db_parse_attribute( ap );
  }

  PROFILE_END;

}

/*!
 \return Returns pointer to newly created attribute parameter list.
*/
attr_param* parser_append_to_attr_list(
  attr_param* ap_list,  /*!< Existing attribute parameter list to append to */
  attr_param* attr      /*!< Pointer to attribute to append */
) { PROFILE(PARSER_CREATE_ATTR_LIST);

  attr_param* retval = NULL;

  if( (ignore_mode == 0) && (attr != NULL) && (ap_list != NULL) ) {
    attr->next    = ap_list;
    ap_list->prev = attr;
    attr->index   = ap_list->index + 1; 
    retval        = attr;  
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns pointer to newly create attribute parameter.
*/
attr_param* parser_create_attr(
  char*       name,  /*!< Name of attribute */
  expression* expr   /*!< Expression value of attribute */
) { PROFILE(PARSER_CREATE_ATTR);

  attr_param* ap = NULL;

  if( ignore_mode == 0 ) {
    ap = db_create_attr_param( name, expr );
  }

  free_safe( name, (strlen( name ) + 1) );

  PROFILE_END;

  return( ap );

}

/*!
 Creates a task declaration (scope).
*/
void parser_create_task_decl(
  bool         automatic,    /*!< If set to TRUE, specifies that this task is an automatic task */
  char*        name,         /*!< Name of task */
  char*        filename,     /*!< Name of file that contains this task */
  unsigned int first_line,   /*!< Starting line number of first line of task */
  unsigned int first_column  /*!< Starting column number of first line of task */
) { PROFILE(PARSER_CREATE_TASK_DECL);

  if( ignore_mode == 0 ) {
    Try {
      if( !db_add_function_task_namedblock( (automatic ? FUNIT_ATASK : FUNIT_TASK), name, filename, first_line, first_column ) ) {
        ignore_mode++;
      }
    } Catch_anonymous {
      error_count++;
      ignore_mode++;
    }
  }

  free_safe( name, (strlen( name ) + 1) );

  PROFILE_END;

}

/*!
 Creates the body of the current task.
*/
void parser_create_task_body(
  statement*   stmt,          /*!< Statement (list) containing task body */
  unsigned int first_line,    /*!< First line of statement (list) */
  unsigned int first_column,  /*!< First column of current statement (list) */
  unsigned int last_column    /*!< Last column of current statement (list) */
) { PROFILE(PARSER_CREATE_TASK_BODY);

  if( ignore_mode == 0 ) {
    if( stmt == NULL ) {
      stmt = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, FALSE, first_line, first_column, (last_column - 1), NULL ) );
    }
    stmt->suppl.part.head      = 1;
    stmt->suppl.part.is_called = 1;
    db_add_statement( stmt, stmt );
  }

  PROFILE_END;

}

/*!
 Creates a function declaration (scope).
*/
void parser_create_function_decl(
  bool         automatic,    /*!< Set to TRUE if the function is an automatic function */
  char*        name,         /*!< Name of function */
  char*        filename,     /*!< Filename containing function */
  unsigned int first_line,   /*!< First line of function */
  unsigned int first_column  /*!< First column of function */
) { PROFILE(PARSER_CREATE_FUNCTION_DECL);

  if( ignore_mode == 0 ) {
    Try {
      if( db_add_function_task_namedblock( (automatic ? FUNIT_AFUNCTION : FUNIT_FUNCTION), name, filename, first_line, first_column ) ) {
        db_add_signal( name, curr_sig_type, &curr_prange, NULL, curr_signed, FALSE, first_line, first_column, TRUE );
      } else {
        ignore_mode++;
      }
    } Catch_anonymous {
      error_count++;
      ignore_mode++;
    }
    free_safe( name, (strlen( name ) + 1) );
  }

  PROFILE_END;

}

/*!
 Creates the body of the current function.
*/
void parser_create_function_body(
  statement* stmt  /*!< Pointer to statement (list) */
) { PROFILE(PARSER_CREATE_FUNCTION_BODY);

  if( (ignore_mode == 0) && (stmt != NULL) ) {
    stmt->suppl.part.head      = 1;
    stmt->suppl.part.is_called = 1;
    db_add_statement( stmt, stmt );
  }

  PROFILE_END;

}

/*!
 Finishes the instantiation of the current function/task.
*/
void parser_end_task_function(
  unsigned int last_line  /*!< Last line of task/function */
) { PROFILE(PARSER_END_TASK_FUNCTION);

  if( ignore_mode == 0 ) {
    db_end_function_task_namedblock( last_line );
  } else {
    ignore_mode--;
  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the newly created port information structure.
*/
port_info* parser_create_inline_port(
  char*        name,         /*!< Name of port to create */
  int          sig_type,     /*!< Signal type to create */
  unsigned int first_line,   /*!< First line that port name exists on */
  unsigned int first_column  /*!< First column that port name exists */
) { PROFILE(PARSER_CREATE_PORT);

  port_info* pi = NULL;

  if( !parser_check_generation( GENERATION_2001 ) ) {

    VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
    free_safe( name, (strlen( name ) + 1) );

  } else {

    if( ignore_mode == 0 ) {

      db_add_signal( name, sig_type, &curr_prange, NULL, curr_signed, FALSE, first_line, first_column, TRUE );

      pi            = (port_info*)malloc_safe( sizeof( port_info ) );
      pi->type      = sig_type;
      pi->is_signed = curr_signed;
      pi->prange    = parser_copy_curr_range( TRUE );
      pi->urange    = parser_copy_curr_range( FALSE );

      free_safe( name, (strlen( name ) + 1) );

    }

  }

  PROFILE_END;
 
  return( pi );

}

/*!
 \return Returns NULL.

 Handles error conditions pertaining to inline port errors.
*/
port_info* parser_handle_inline_port_error() { PROFILE(PARSER_HANDLE_INLINE_PORT_ERROR);

  if( !parser_check_generation( GENERATION_2001 ) ) {
    VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
  } else if( ignore_mode == 0 ) {
    VLerror( "Invalid variable list in port declaration" );
  }

  PROFILE_END;

  return( NULL );

}

/*!
 \return Returns vector and base type of given number.

 Creates a simple number.
*/
const_value parser_create_simple_number(
  char* str_num  /*!< String version of the number */
) { PROFILE(PARSER_CREATE_SIMPLE_NUMBER);

  const_value num;
  char*       tmp = str_num;

  vector_from_string( &str_num, FALSE, &(num.vec), &(num.base) );

  free_safe( tmp, (strlen( tmp ) + 1) );

  PROFILE_END;

  return( num );

}

/*!
 \return Returns vector and base type of given number.

 Creates a complex number.
*/
const_value parser_create_complex_number(
  char* str_num1,  /*!< String version of first part of number */
  char* str_num2   /*!< String version of second part of number */
) { PROFILE(PARSER_CREATE_COMPLEX_NUMBER);

  const_value  num;
  int          slen     = strlen( str_num1 ) + strlen( str_num2 ) + 1;
  char*        combined = (char*)malloc_safe( slen );
  char*        tmp      = combined;
  unsigned int rv       = snprintf( combined, slen, "%s%s", str_num1, str_num2 );

  assert( rv < slen );

  vector_from_string( &combined, FALSE, &(num.vec), &(num.base) );

  free_safe( str_num1, (strlen( str_num1 ) + 1) );
  free_safe( str_num2, (strlen( str_num2 ) + 1) );
  free_safe( tmp, slen );

  PROFILE_END;

  return( num );

}

/*!
 \return Returns a pointer to the newly creating static expression port list.
*/
static_expr* parser_append_se_port_list(
  static_expr* sel,               /*!< Pointer to existing static expression port list */
  static_expr* se,                /*!< Pointer to static expression to make into a port and append it to the list */
  unsigned int sel_first_line,    /*!< First line of static expression port list */
  unsigned int sel_first_column,  /*!< First column of static expression port list */
  unsigned int se_first_line,     /*!< First line of static expression */
  unsigned int se_first_column,   /*!< First column of static expression */
  unsigned int se_last_column     /*!< Last column of static expression */
) { PROFILE(PARSER_APPEND_SE_PORT_LIST);

  static_expr* retval = NULL;

  if( ignore_mode == 0 ) {
    if( se != NULL ) {
      Try {
        se = static_expr_gen_unary( se, EXP_OP_PASSIGN, se_first_line, se_first_column, (se_last_column - 1) );
      } Catch_anonymous {
        error_count++;
      }
      Try {
        se = static_expr_gen( se, sel, EXP_OP_PLIST, sel_first_line, sel_first_column, (se_last_column - 1), NULL );
      } Catch_anonymous {
        error_count++;
      }
      retval = se;
    } else {
      retval = sel;
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the newly created static expression port.
*/
static_expr* parser_create_se_port_list(
  static_expr* se,            /*!< Pointer to static expression to make into a port */
  unsigned int first_line,    /*!< First line of static expression */
  unsigned int first_column,  /*!< First column of static expression */
  unsigned int last_column    /*!< Last column of static expression */
) { PROFILE(PARSER_CREATE_SE_PORT_LIST);

  static_expr* retval = NULL;

  if( ignore_mode == 0 ) {
    if( se != NULL ) {
      Try {
        se = static_expr_gen_unary( se, EXP_OP_PASSIGN, first_line, first_column, (last_column - 1) );
      } Catch_anonymous {
        error_count++;
      }
    }
    retval = se;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns unary static expression.
*/
static_expr* parser_create_unary_se(
  static_expr* se,            /*!< Pointer to child static expression */
  exp_op_type  op,            /*!< Specifies unary operation */
  unsigned int first_line,    /*!< First line of unary static expression */
  unsigned int first_column,  /*!< First column of unary static expression */
  unsigned int last_column    /*!< Last column of unary static expression */
) { PROFILE(PARSER_CREATE_UNARY_SE);

  static_expr* retval = NULL;

  Try {
    retval = static_expr_gen_unary( se, op, first_line, first_column, (last_column - 1) );
  } Catch_anonymous {
    error_count++;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns pointer to newly created static expression.

 Creates a static expression system call.
*/
static_expr* parser_create_syscall_se(
  exp_op_type  op,            /*!< Expression operation type */
  unsigned int first_line,    /*!< First line of static expression */
  unsigned int first_column,  /*!< First column of static expression */
  unsigned int last_column    /*!< Last column of static expression */
) { PROFILE(PARSER_CREATE_SYSCALL_SE);

  static_expr* se = NULL;

  if( ignore_mode == 0 ) {
    se = (static_expr*)malloc_safe( sizeof( static_expr ) );
    se->num = -1;
    Try {
      se->exp = db_create_expression( NULL, NULL, op, lhs_mode, first_line, first_column, (last_column - 1), NULL );
    } Catch_anonymous {
      error_count++;
      static_expr_dealloc( se, FALSE );
      se = NULL;
    }
  }

  PROFILE_END;

  return( se );

}

/*!
 \return Returns pointer to newly created expression.

 Creates a unary expression.
*/
expression* parser_create_unary_exp(
  expression*  exp,           /*!< Pointer to child expression */
  exp_op_type  op,            /*!< Specifies unary operation */
  unsigned int first_line,    /*!< First line of unary expression */
  unsigned int first_column,  /*!< First column of unary expression */
  unsigned int last_column    /*!< Last column of unary expression */
) { PROFILE(PARSER_CREATE_UNARY_EXP);

  expression* retval = NULL;

  if( (ignore_mode == 0) && (exp != NULL) ) {
    Try {
      retval = db_create_expression( exp, NULL, op, lhs_mode, first_line, first_column, (last_column - 1), NULL );
    } Catch_anonymous {
      error_count++;
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns pointer to newly created expression.

 Creates a binary expression from two child expressions.
*/
expression* parser_create_binary_exp(
  expression* lexp,            /*!< Pointer to child expression on the left */
  expression* rexp,            /*!< Pointer to child expression on the right */
  exp_op_type op,              /*!< Expression operation type */
  unsigned int first_line,     /*!< First line of expression */
  unsigned int first_column,   /*!< First column of expression */
  unsigned int last_column     /*!< Last column of expression */
) { PROFILE(PARSER_CREATE_BINARY_EXP);

  expression* retval = NULL;

  if( (ignore_mode == 0) && (lexp != NULL) && (rexp != NULL) ) {
    Try {
      retval = db_create_expression( rexp, lexp, op, lhs_mode, first_line, first_column, (last_column - 1), NULL );
    } Catch_anonymous {
      error_count++;
    }
  } else {
    expression_dealloc( lexp, FALSE );
    expression_dealloc( rexp, FALSE );
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the newly created expression.

 Creates an expression for an op-and-assign expression.
*/
expression* parser_create_op_and_assign_exp(
  char*        name,           /*!< Name of signal */
  exp_op_type  op,             /*!< Expression operation type */
  unsigned int first_line1,    /*!< First line of signal */
  unsigned int first_column1,  /*!< First column of signal */
  unsigned int last_column1,   /*!< Last column of signal */
  unsigned int last_column2    /*!< Last column of full expression */
) { PROFILE(PARSER_CREATE_OP_AND_ASSIGN_EXP);

  expression* retval = NULL;

  if( ignore_mode == 0 ) {
    Try {
      expression* tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, first_line1, first_column1, (last_column1 - 1), name );
      retval = db_create_expression( NULL, tmp, op, lhs_mode, first_line1, first_column1, (last_column2 - 1), NULL );
    } Catch_anonymous {
      error_count++;
    }
  }
  free_safe( name, (strlen( name ) + 1) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the newly created expression system call.

 Creates an expression for a system call.
*/
expression* parser_create_syscall_exp(
  exp_op_type  op,            /*!< Expression operation type */
  unsigned int first_line,    /*!< First line of expression */
  unsigned int first_column,  /*!< First column of expression */
  unsigned int last_column    /*!< Last column of expression */
) { PROFILE(PARSER_CREATE_SYSCALL_EXP);

  expression* retval = NULL;

  if( ignore_mode == 0 ) {
    Try {
      retval = db_create_expression( NULL, NULL, op, lhs_mode, first_line, first_column, (last_column - 1), NULL );
    } Catch_anonymous {
      error_count++;
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the newly created expression.

 Creates an expression for a system call with a parameter list.
*/
expression* parser_create_syscall_w_params_exp(
  exp_op_type  op,            /*!< Expression operation type */
  expression*  plist,         /*!< Parameter list expression */
  unsigned int first_line,    /*!< First line of expression */
  unsigned int first_column,  /*!< First column of expression */
  unsigned int last_column    /*!< Last column of expression */
) { PROFILE(PARSER_CREATE_SYSCALL_W_PARAMS_EXP);

  expression* retval = NULL;

  if( (ignore_mode == 0) && (plist != NULL) ) {
    Try {
      retval = db_create_expression( NULL, plist, op, lhs_mode, first_line, first_column, (last_column - 1), NULL );
    } Catch_anonymous {
      expression_dealloc( plist, FALSE );
      error_count++;
    }
  } else {
    expression_dealloc( plist, FALSE );
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the newly created expression.

 Creates an op-and-assign expression.
*/
expression* parser_create_op_and_assign_w_dim_exp(
  char*        name,          /*!< Name of signal */
  exp_op_type  op,            /*!< Expression operation type */
  expression*  dim_exp,       /*!< Dimensional expression */
  unsigned int first_line,    /*!< First line of expression */
  unsigned int first_column,  /*!< First column of expression */
  unsigned int last_column    /*!< Last column of expression */
) { PROFILE(PARSER_CREATE_OP_AND_ASSIGN_W_DIM_EXP);

  expression* retval = NULL;

  if( (ignore_mode == 0) && (dim_exp != NULL) ) {
    db_bind_expr_tree( dim_exp, name );
    dim_exp->line = first_line;
    dim_exp->col  = ((first_column & 0xffff) << 16) | (dim_exp->col & 0xffff);
    Try {
      retval = db_create_expression( NULL, dim_exp, op, lhs_mode, first_line, first_column, (last_column - 1), NULL );
    } Catch_anonymous {
      error_count++;
    }
  } else {
    expression_dealloc( dim_exp, FALSE );
  }
  free_safe( name, (strlen( name ) + 1) );

  PROFILE_END;

  return( retval );

}


/*
 $Log$
 Revision 1.3  2009/01/09 21:25:01  phase1geo
 More generate block fixes.  Updated all copyright information source code files
 for the year 2009.  Checkpointing.

 Revision 1.2  2008/12/02 06:14:09  phase1geo
 More changes to parser.y to move its code to parser_func.c for cleanup purposes.
 Regression still passes.  Checkpointing.

 Revision 1.1  2008/12/02 00:17:07  phase1geo
 Adding missing files.

*/

