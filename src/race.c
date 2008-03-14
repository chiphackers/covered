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
 \file     race.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/15/2004

 \par
 The contents of this file contain functions to handle race condition checking and information
 handling regarding race conditions.  Since race conditions can cause the Covered simulator to not
 provide the same run-time order as the primary Verilog simulator, we need to be able to detect when
 race conditions are occurring within the design.  The conditions that are checked within the design
 during the parsing stage can be found \ref race_condition_types.

 \par
 The failure of any one of these rules will cause Covered to either display warning type messages to the user when
 the race condition checking flag has not been set or error messages when the race condition checking flag has
 been set by the user.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "iter.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "race.h"
#include "statement.h"
#include "stmt_blk.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


/*!
 Pointer to array of statement blocks for the specified design.
*/
static stmt_blk* sb = NULL;

/*!
 Number of entries loaded in the sb array.
*/
static int sb_size;

/*!
 Tracks the number of race conditions that were detected during the race-condition checking portion of the
 scoring command.
*/
static int races_found = 0;

/*!
 This array is used to output the various race condition violation messages in both the parsing and report functions
 */
const char* race_msgs[RACE_TYPE_NUM] = { "Sequential statement block contains blocking assignment(s)",
                                         "Combinational statement block contains non-blocking assignment(s)",
                                         "Mixed statement block contains blocking assignment(s)",
					 "Statement block contains both blocking and non-blocking assignment(s)", 
					 "Signal assigned in two different statement blocks",
					 "Signal assigned both in statement block and via input/inout port",
					 "System call $strobe used to output signal assigned via blocking assignment",
					 "Procedural assignment with #0 delay performed" };

static void race_calc_assignments( statement*, int );

extern int         flag_race_check;
extern char        user_msg[USER_MSG_LENGTH];
extern funit_link* funit_head;
extern inst_link*  inst_head;
extern func_unit*  curr_funit;
extern isuppl      info_suppl;
extern int         stmt_conn_id;

/*!
 \param reason      Numerical reason for why race condition was detected.
 \param start_line  Starting line of race condition block.
 \param end_line    Ending line of race condition block.

 \return Returns a pointer to the newly allocated race condition block.

 Allocates and initializes memory for a race condition block.
*/
static race_blk* race_blk_create(
  int reason,
  int start_line,
  int end_line
) { PROFILE(RACE_BLK_CREATE);

  race_blk* rb;  /* Pointer to newly allocated race condition block */

  rb             = (race_blk*)malloc_safe( sizeof( race_blk ) );
  rb->reason     = reason;
  rb->start_line = start_line;
  rb->end_line   = end_line;
  rb->next       = NULL;

  PROFILE_END;

  return( rb );

}

/*!
 \param curr     Pointer to current statement to check
 \param to_find  Pointer to statement to find

 \return Returns TRUE if the specified to_find statement was found in the statement block specified
         by curr; otherwise, returns FALSE.

 Recursively iterates through specified statement block, searching for statement pointed to by to_find.
 If the statement is found, a value of TRUE is returned to the calling function; otherwise, a value of
 FALSE is returned.
*/
static bool race_find_head_statement_containing_statement_helper(
  statement* curr,
  statement* to_find
) { PROFILE(RACE_FIND_HEAD_STATEMENT_CONTAINING_STATEMENT_HELPER);

  bool retval = FALSE;  /* Return value for this function */

  if( (curr != NULL) && (curr->conn_id != stmt_conn_id) ) {

    curr->conn_id = stmt_conn_id;

    if( curr == to_find ) {

      retval = TRUE;

    } else {

      /* 
       If the current statement is a named block call, task call or fork statement, look for specified
       statement in its statement block.
      */
      if( (curr->exp->op == EXP_OP_NB_CALL)   ||
          (curr->exp->op == EXP_OP_TASK_CALL) ||
          (curr->exp->op == EXP_OP_FORK) ) {
        retval = race_find_head_statement_containing_statement_helper( curr->exp->elem.funit->first_stmt, to_find );
      }

      if( !retval && (curr->suppl.part.stop_true == 0) ) {
        retval = race_find_head_statement_containing_statement_helper( curr->next_true, to_find );
      }
   
      if( !retval && (curr->suppl.part.stop_false == 0) && (curr->next_true != curr->next_false) ) {
        retval = race_find_head_statement_containing_statement_helper( curr->next_false, to_find );
      }

    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param stmt  Pointer to statement to search for matching statement block

 \return Returns a pointer to the statement block containing the given statement (if one is found); otherwise,
         returns a value of NULL.

 Searches all statement blocks for one that contains the given statement within it.  Once it is found, a pointer
 to the head statement block is returned to the calling function.  If it is not found, a value of NULL is returned,
 indicating that the statement block could not be found for the given statement.
*/
static statement* race_find_head_statement_containing_statement(
  statement* stmt
) { PROFILE(RACE_FIND_HEAD_STATEMENT_CONTAINING_STATEMENT);

  int i = 0;   /* Loop iterator */

  while( (i < sb_size) && !race_find_head_statement_containing_statement_helper( sb[i].stmt, stmt ) ) {
    i++;
    stmt_conn_id++;
  }

  if( i < sb_size ) {
    stmt_conn_id++;
  }

  PROFILE_END;

  return( (i == sb_size) ? NULL : sb[i].stmt );

}

/*!
 \param expr  Pointer to root expression to find in statement list.

 \return Returns pointer to head statement of statement block containing the specified expression

 Finds the head statement of the statement block containing the expression specified in the parameter list.
 Verifies that the return value is never NULL (this would be an internal error if it existed).
*/
static statement* race_get_head_statement(
  expression* expr
) { PROFILE(RACE_GET_HEAD_STATEMENT);

  statement* curr_stmt;  /* Pointer to current statement containing the expression */

  /* First, find the statement associated with this expression */
  if( (curr_stmt = expression_get_root_statement( expr )) != NULL ) {

    curr_stmt = race_find_head_statement_containing_statement( curr_stmt );

  }

  PROFILE_END;

  return( curr_stmt );

}

/*!
 \param stmt  Pointer to statement to search for in statement block array

 \return Returns index of found head statement in statement block array if found; otherwise,
         returns a value of -1 to indicate that no such statement block exists.
*/
static int race_find_head_statement(
  statement* stmt
) { PROFILE(RACE_FIND_HEAD_STATEMENT);

  int i = 0;   /* Loop iterator */

  while( (i < sb_size) && (sb[i].stmt != stmt) ) {
    i++;
  }

  PROFILE_END;

  return( (i == sb_size) ? -1 : i );

}

/*!
 \param expr      Pointer to expression to check for statement block type (sequential/combinational)
 \param sb_index  Current statement block index containing the given expression.

 Recursively iterates down expression tree, searching for an expression which will indicate that
 its statement block is a sequential always block or a combinational always block.
*/
static void race_calc_stmt_blk_type( expression* expr, int sb_index ) { PROFILE(RACE_CALC_STMT_BLK_TYPE);

  if( expr != NULL ) {

    /* Go to children to calculate further */
    race_calc_stmt_blk_type( expr->left,  sb_index );
    race_calc_stmt_blk_type( expr->right, sb_index );

    if( (expr->op == EXP_OP_PEDGE) || (expr->op == EXP_OP_NEDGE) || (expr->op == EXP_OP_ALWAYS_LATCH) ) {
      sb[sb_index].seq = TRUE;
    }

    if( (expr->op == EXP_OP_AEDGE) || (expr->op == EXP_OP_ALWAYS_COMB) ) {
      sb[sb_index].cmb = TRUE;
    }

  }

  PROFILE_END;

}

/*!
 \param exp       Pointer to expression to check for expression assignment type
 \param sb_index  Current statement block index containing the given expression

 Checks the given expression for its assignment type and sets the given statement block
 structure to the appropriate assign type value.  If the given expression is a named block,
 iterate through that block searching for an assignment type.
*/
static void race_calc_expr_assignment( expression* exp, int sb_index ) { PROFILE(RACE_CALC_EXPR_ASSIGNMENT);

  switch( exp->op ) {
    case EXP_OP_ASSIGN    :
    case EXP_OP_DASSIGN   :  sb[sb_index].bassign = TRUE;  break;
    case EXP_OP_NASSIGN   :  sb[sb_index].nassign = TRUE;  break;
    case EXP_OP_BASSIGN   :  sb[sb_index].bassign = (exp->suppl.part.for_cntrl == 0);  break;
    case EXP_OP_TASK_CALL :
    case EXP_OP_FORK      :
    case EXP_OP_NB_CALL   :  race_calc_assignments( exp->elem.funit->first_stmt, sb_index );  break;
    default               :  break;
  }

  PROFILE_END;

}

/*!
 \param stmt      Pointer to statement to get assign type from
 \param sb_index  Current statement block index containing the given statement

 Recursively iterates through the given statement block, searching for all assignment types used
 within the block.
*/
void race_calc_assignments(
  statement* stmt,
  int        sb_index
) { PROFILE(RACE_CALC_ASSIGNMENTS);

  if( (stmt != NULL) && (stmt->conn_id != stmt_conn_id) ) {

    stmt->conn_id = stmt_conn_id;

    /* Calculate children statements */
    if( stmt->suppl.part.stop_true == 0 ) {
      race_calc_assignments( stmt->next_true, sb_index );
    }
    if( (stmt->suppl.part.stop_false == 0) && (stmt->next_true != stmt->next_false) ) {
      race_calc_assignments( stmt->next_false, sb_index );
    }

    /* Calculate assignment operator type */
    race_calc_expr_assignment( stmt->exp, sb_index );

  }

  PROFILE_END;

}

/*!
 \param expr    Pointer to expression containing signal that was found to be in a race condition.
 \param mod     Pointer to module containing detected race condition
 \param stmt    Pointer to expr's head statement
 \param base    Pointer to head statement block that was found to be in conflict with stmt
 \param reason  Specifies what type of race condition was being checked that failed

 Outputs necessary information to user regarding the race condition that was detected and performs any
 necessary memory cleanup to remove the statement block involved in the race condition.
*/
static void race_handle_race_condition(
  expression* expr,
  func_unit*  mod,
  statement*  stmt,
  statement*  base,
  int         reason
) { PROFILE(RACE_HANDLE_RACE_CONDITION);

  race_blk*    rb;         /* Pointer to race condition block to add to specified module */
  int          i;          /* Loop iterator */
  int          last_line;  /* Holds the line number of the last line in the specified statement */
  unsigned int rv;         /* Return value from snprintf calls */

  /* If the base pointer is NULL, the stmt refers to a statement block that conflicts with an input port */
  if( base == NULL ) {

    if( flag_race_check != NORMAL ) {

      print_output( "", (flag_race_check + 1), __FILE__, __LINE__ );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Possible race condition detected - %s", race_msgs[reason] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, flag_race_check, __FILE__, __LINE__ );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "  Signal assigned in file: %s, line: %d", obf_file( mod->filename ), expr->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );

      if( flag_race_check == WARNING ) {
        print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "    Statement block starting at file: %s, line: %d",
                       obf_file( mod->filename ), stmt->exp->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
      }
              
    }

  /* If the stmt and base pointers are pointing to different statements, we will output conflict for stmt */
  } else if( stmt != base ) { 

    if( flag_race_check != NORMAL ) {

      print_output( "", (flag_race_check + 1), __FILE__, __LINE__ );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Possible race condition detected - %s", race_msgs[reason] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, flag_race_check, __FILE__, __LINE__ );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "  Signal assigned in file: %s, line: %d", obf_file( mod->filename ), expr->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "  Signal also assigned in statement starting at file: %s, line: %d",
                     obf_file( mod->filename ), base->exp->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );

      if( flag_race_check == WARNING ) {
        print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "    Statement block starting at file: %s, line: %d",
                       obf_file( mod->filename ), stmt->exp->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
      }

    }

  /* If stmt and base are pointing to the same statement, just report that we are removing the base statement */
  } else {

    if( flag_race_check != NORMAL ) {

      if( reason != 6 ) {
	
        print_output( "", (flag_race_check + 1), __FILE__, __LINE__ );
	rv = snprintf( user_msg, USER_MSG_LENGTH, "Possible race condition detected - %s", race_msgs[reason] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, flag_race_check, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Statement block starting in file: %s, line: %d",
                       obf_file( mod->filename ), stmt->exp->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );
	if( flag_race_check == WARNING ) {
	  print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
	}

      } else {

	if( flag_race_check == WARNING ) {
          print_output( "", WARNING_WRAP, __FILE__, __LINE__ );
	  print_output( "* Safely removing statement block from coverage consideration", WARNING, __FILE__, __LINE__ );
          rv = snprintf( user_msg, USER_MSG_LENGTH, "  Statement block starting at file: %s, line: %d",
                         obf_file( mod->filename ), stmt->exp->line );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, WARNING_WRAP, __FILE__, __LINE__ );
	}

      }

    }

  }

  /* Calculate the last line of this statement so that we are not constantly doing it */
  last_line = statement_get_last_line( stmt );

  /* Search this module's race list to see if this statement block already exists */
  rb = mod->race_head;
  while( (rb != NULL) && (rb->start_line != stmt->exp->line) && (rb->end_line != last_line) ) {
    rb = rb->next;
  }  

  /* If a matching statement block was not found, go ahead and create it */
  if( rb == NULL ) {

    /* Create a race condition block and add it to current module */
    rb = race_blk_create( reason, stmt->exp->line, last_line );

    /* Add the newly created race condition block to the current module */
    if( mod->race_head == NULL ) {
      mod->race_head = mod->race_tail = rb;
    } else {
      mod->race_tail->next = rb;
      mod->race_tail       = rb;
    }

  }

  /* Set remove flag in stmt_blk array to remove this module from memory */
  i = race_find_head_statement( stmt );
  assert( i != -1 );
  sb[i].remove = TRUE;

  /* Increment races found flag */
  races_found++;

  PROFILE_END;

}

/*!
 \param mod  Pointer to functional unit to check assignment types for
*/
static void race_check_assignment_types(
  func_unit* mod
) { PROFILE(RACE_CHECK_ASSIGNMENT_TYPES);

  int i;  /* Loop iterator */

  for( i=0; i<sb_size; i++ ) {

    if( sb[i].stmt->suppl.part.ignore_rc == 0 ) {

      /* Check that a sequential logic block contains only non-blocking assignments */
      if( sb[i].seq && !sb[i].cmb && sb[i].bassign ) {

        race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_SEQ_USES_NON_BLOCK );

      /* Check that a combinational logic block contains only blocking assignments */
      } else if( !sb[i].seq && sb[i].cmb && sb[i].nassign ) {

        race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_CMB_USES_BLOCK );

      /* Check that mixed logic block contains only non-blocking assignments */
      } else if( sb[i].seq && sb[i].cmb && sb[i].bassign ) {

        race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_MIX_USES_NON_BLOCK );

      /* Check that a statement block doesn't contain both blocking and non-blocking assignments */
      } else if( sb[i].bassign && sb[i].nassign ) {

        race_handle_race_condition( sb[i].stmt->exp, mod, sb[i].stmt, sb[i].stmt, RACE_TYPE_HOMOGENOUS );

      }

    }

  }

  PROFILE_END;

}

/*!
 \param mod  Pointer to module containing signals to verify

 Verifies that every signal is assigned in only one statement block.  If a signal is
 assigned in more than one statement block, both statement block's need to be removed
 from coverage consideration and a possible warning/error message generated to the user.
*/
static void race_check_one_block_assignment(
  func_unit* mod
) { PROFILE(RACE_CHECK_ONE_BLOCK_ASSIGNMENT);

  sig_link*  sigl;                /* Pointer to current signal */
  exp_link*  expl;                /* Pointer to current expression */
  statement* curr_stmt;           /* Pointer to current statement */
  int        sig_stmt;            /* Index of base signal statement in statement block array */
  bool       race_found = FALSE;  /* Specifies if at least one race condition was found for this signal */
  bool       curr_race;           /* Set to TRUE if race condition was found in current iteration */
  int        lval;                /* Left expression value */
  int        rval;                /* Right expression value */
  int        exp_dim;             /* Current expression dimension */
  int        dim_lsb;             /* LSB of current dimension */
  bool       dim_be;              /* Big endianness of current dimension */
  int        dim_width;           /* Bit width of current dimension */
  int        intval;              /* Integer value */
  vector     vec;                 /* Temporary vector used for setting assigned bits in signal */
  vec_data*  vstart;              /* Starting address for current dimension */
  int        vwidth;              /* Width of bit range to work within for current dimension */

  sigl = mod->sig_head;
  while( sigl != NULL ) {

    sig_stmt = -1;

    /* Skip checking the expressions of genvar signals */
    if( (sigl->sig->suppl.part.type != SSUPPL_TYPE_GENVAR) && (sigl->sig->suppl.part.type != SSUPPL_TYPE_MEM) ) {

      /* Iterate through expressions */
      expl = sigl->sig->exp_head;
      while( expl != NULL ) {
					      
        /* Only look at expressions that are part of LHS and they are not part of a bit select */
        if( (ESUPPL_IS_LHS( expl->exp->suppl ) == 1) && !expression_is_bit_select( expl->exp ) && expression_is_last_select( expl->exp ) ) {

          /* Get head statement of current expression */
          curr_stmt = race_get_head_statement( expl->exp );

          /* If the head statement is being ignored from race condition checking, skip the rest */
          if( (curr_stmt != NULL) && (curr_stmt->suppl.part.ignore_rc == 0) ) {

            /* Get current dimension of the given expression */
            exp_dim = expression_get_curr_dimension( expl->exp );
  
            /* Calculate starting vector value bit and signal LSB/BE for LHS */
            if( (ESUPPL_IS_ROOT( expl->exp->suppl ) == 0) &&
                (expl->exp->parent->expr->op == EXP_OP_DIM) && (expl->exp->parent->expr->right == expl->exp) ) {
              vstart = expl->exp->parent->expr->left->value->value;
              vwidth = expl->exp->parent->expr->left->value->width;
            } else {
              /* Get starting vector bit from signal itself */
              vstart = sigl->sig->value->value;
              vwidth = sigl->sig->value->width;
            }
            if( sigl->sig->dim[exp_dim].lsb < sigl->sig->dim[exp_dim].msb ) {
              dim_lsb = sigl->sig->dim[exp_dim].lsb;
              dim_be  = FALSE;
            } else {
              dim_lsb = sigl->sig->dim[exp_dim].msb;
              dim_be  = TRUE;
            }
            dim_width = vsignal_calc_width_for_expr( expl->exp, sigl->sig );

            /*
             If the signal was a part select, set the appropriate misc bits to indicate what
             bits have been assigned.
            */
            switch( expl->exp->op ) {
              case EXP_OP_SIG :
                if( (ESUPPL_IS_ROOT( expl->exp->suppl ) == 0) && !expression_is_in_rassign( expl->exp ) ) {
                  curr_race = vector_set_assigned( sigl->sig->value, (sigl->sig->value->width - 1), 0 );
                }
  	      break;
              case EXP_OP_SBIT_SEL :
                if( expl->exp->left->op == EXP_OP_STATIC ) {
                  intval = (vector_to_int( expl->exp->left->value ) - dim_lsb) * dim_width;
                  if( (intval >= 0) && (intval < expl->exp->value->width) ) {
                    vector_init( &vec, NULL, FALSE, expl->exp->value->width, VTYPE_SIG );
                    if( dim_be ) {
                      vec.value = vstart + (vwidth - (intval + expl->exp->value->width));
                    } else {
                      vec.value = vstart + intval;
                    }
                    curr_race = vector_set_assigned( &vec, (vec.width - 1), 0 );
                  } else {
                    curr_race = FALSE;
                  }
                } else { 
                  curr_race = vector_set_assigned( sigl->sig->value, (sigl->sig->value->width - 1), 0 );
                }
  	      break;
              case EXP_OP_MBIT_SEL :
                if( (expl->exp->left->op == EXP_OP_STATIC) && (expl->exp->right->op == EXP_OP_STATIC) ) {
                  intval = ((dim_be ? vector_to_int( expl->exp->left->value ) : vector_to_int( expl->exp->right->value )) - dim_lsb) * dim_width;
                  vector_init( &vec, NULL, FALSE, expl->exp->value->width, VTYPE_SIG );
                  if( dim_be ) {
                    vec.value = vstart + (vwidth - (intval + expl->exp->value->width));
                  } else {
                    vec.value = vstart + intval;
                  }
                  curr_race = vector_set_assigned( &vec, (vec.width - 1), 0 );
                } else {
                  curr_race = vector_set_assigned( sigl->sig->value, (sigl->sig->value->width - 1), 0 );
                }
	        break;
              case EXP_OP_MBIT_POS :
                if( expl->exp->left->op == EXP_OP_STATIC ) {
                  lval = vector_to_int( expl->exp->left->value );
                  rval = vector_to_int( expl->exp->right->value );
                  curr_race = vector_set_assigned( sigl->sig->value, ((lval + rval) - sigl->sig->dim[exp_dim].lsb), (lval - sigl->sig->dim[exp_dim].lsb) );
                } else {
                  curr_race = vector_set_assigned( sigl->sig->value, (sigl->sig->value->width - 1), 0 );
                }
                break;
              case EXP_OP_MBIT_NEG :
                if( expl->exp->left->op == EXP_OP_STATIC ) {
                  lval = vector_to_int( expl->exp->left->value );
                  rval = vector_to_int( expl->exp->right->value );
                  curr_race = vector_set_assigned( sigl->sig->value, ((lval + 1) - sigl->sig->dim[exp_dim].lsb), (((lval - rval) + 1) - sigl->sig->dim[exp_dim].lsb) );
                } else {
                  curr_race = vector_set_assigned( sigl->sig->value, (sigl->sig->value->width - 1), 0 );
                }
                break;
              default :
                curr_race = FALSE;
	        break;	
            }
  
            /*
             Get expression's head statement and if the statement is not a register assignment, check for
             race conditions (the way that RASSIGNs are treated, they will not cause race conditions so omit
             them from being checked.
            */
            if( (curr_stmt != NULL) && (curr_stmt->exp->op != EXP_OP_RASSIGN) ) {

              /* Check to see if the current signal is already being assigned in another statement */
              if( sig_stmt == -1 ) {

                /* Get index of base signal statement in sb array */
                sig_stmt = race_find_head_statement( curr_stmt );
                assert( sig_stmt != -1 );
  
                /* Check to see if current signal is also an input port */ 
                if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_INPUT) ||
                    (sigl->sig->suppl.part.type == SSUPPL_TYPE_INOUT) || curr_race ) {
                  race_handle_race_condition( expl->exp, mod, curr_stmt, NULL, RACE_TYPE_ASSIGN_IN_ONE_BLOCK2 );
                  sb[sig_stmt].remove = TRUE;
                }
  
              } else if( (sb[sig_stmt].stmt != curr_stmt) && curr_race ) {
  
                race_handle_race_condition( expl->exp, mod, curr_stmt, sb[sig_stmt].stmt, RACE_TYPE_ASSIGN_IN_ONE_BLOCK1 );
                sb[sig_stmt].remove = TRUE;
                race_found = TRUE;
  
              }
  
            }
  
          }

        }

        expl = expl->next;

      }

    }

    sigl = sigl->next;

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if no race conditions were found or the user specified that we should continue
         to score the design.

 \throws anonymous Throw
*/
static void race_check_race_count() { PROFILE(RACE_CHECK_RACE_COUNT);

  /*
   If we were able to find race conditions and the user specified to check for race conditions
   and quit, display the number of race conditions found and return FALSE to cause everything to
   halt.
  */
  if( (races_found > 0) && (flag_race_check == FATAL) ) {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "%d race conditions were detected.  Exiting score command.", races_found );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    printf( "race Throw A\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \throws anonymous race_check_race_count funit_size_elements

 Performs race checking for the currently loaded module.  This function should be called when
 the endmodule keyword is detected in the current module.
*/
void race_check_modules() { PROFILE(RACE_CHECK_MODULES);

  int         sb_index;  /* Index to statement block array */
  stmt_iter   si;        /* Statement iterator */
  funit_link* modl;      /* Pointer to current module link */
  int         i;         /* Loop iterator */
  int         ignore;    /* Placeholder */
  funit_inst* inst;      /* Instance of this functional unit */

  modl = funit_head;

  while( modl != NULL ) {

    /* Get instance */
    ignore = 0;
    inst   = inst_link_find_by_funit( modl->funit, inst_head, &ignore );

    /* Only perform race condition checking for modules that are instantiated and are not OVL assertions */
    if( (modl->funit->type == FUNIT_MODULE) &&
        (inst != NULL) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( modl->funit )) ) {

      /* Size elements for the current module */
      funit_size_elements( modl->funit, inst, FALSE, FALSE );

      /* Clear statement block array size */
      sb_size = 0;

      /* First, get the size of the statement block array for this module */
      stmt_iter_reset( &si, modl->funit->stmt_tail );
      while( si.curr != NULL ) {
        if( (si.curr->stmt->suppl.part.head == 1) &&
            (si.curr->stmt->suppl.part.is_called == 0) ) {
          sb_size++;
        }
        stmt_iter_next( &si );
      }

      if( sb_size > 0 ) {

        /* Allocate memory for the statement block array and clear current index */
        sb       = (stmt_blk*)malloc_safe( sizeof( stmt_blk ) * sb_size );
        sb_index = 0;

        /* Second, populate the statement block array with pointers to the head statements */
        stmt_iter_reset( &si, modl->funit->stmt_tail );
        while( si.curr != NULL ) {
          if( (si.curr->stmt->suppl.part.head == 1) &&
              (si.curr->stmt->suppl.part.is_called == 0) ) {
            sb[sb_index].stmt    = si.curr->stmt;
            sb[sb_index].remove  = FALSE;
            sb[sb_index].seq     = FALSE;
	    sb[sb_index].cmb     = FALSE;
	    sb[sb_index].bassign = FALSE;
	    sb[sb_index].nassign = FALSE;
	    race_calc_stmt_blk_type( sb[sb_index].stmt->exp, sb_index );
	    race_calc_assignments( sb[sb_index].stmt, sb_index );
            sb_index++; 
            stmt_conn_id++;
          }
          stmt_iter_next( &si );
        }

        /* Perform checks #1 - #5 */
        race_check_assignment_types( modl->funit );

        /* Perform check #6 */
        race_check_one_block_assignment( modl->funit );

        /* Cleanup statements to be removed */
        stmt_iter_reverse( &si );
        curr_funit = modl->funit;
        for( i=0; i<sb_size; i++ ) {
          if( sb[i].remove ) {
#ifdef DEBUG_MODE
            print_output( "Removing statement block because it was found to have a race condition", DEBUG, __FILE__, __LINE__ );
#endif 
            stmt_blk_add_to_remove_list( sb[i].stmt );
          }
        }

        /* Deallocate stmt_blk list */
        free_safe( sb );

      }

    }

    modl = modl->next;

  }

  /* Handle output if any race conditions were found */
  race_check_race_count();

  PROFILE_END;

}

/*!
 \param rb    Pointer to race condition block to write to specified output file
 \param file  File handle of output stream to write.

 Writes contents of specified race condition block to the specified output stream.
*/
void race_db_write(
  race_blk* rb,
  FILE*     file
) { PROFILE(RACE_DB_WRITE);

  fprintf( file, "%d %d %d %d\n",
    DB_TYPE_RACE,
    rb->reason,
    rb->start_line,
    rb->end_line
  );

  PROFILE_END;

}

/*!
 \param line      Pointer to line containing information for a race condition block.
 \param curr_mod  Pointer to current module to store race condition block to.

 \throws anonymous Throw Throw

 Reads the specified line from the CDD file and parses it for race condition block
 information.
*/
void race_db_read(
  char**     line,
  func_unit* curr_mod
) { PROFILE(RACE_DB_READ);

  int       start_line;     /* Starting line for race condition block */
  int       end_line;       /* Ending line for race condition block */
  int       reason;         /* Reason for why the race condition block exists */
  int       chars_read;     /* Number of characters read via sscanf */
  race_blk* rb;             /* Pointer to newly created race condition block */

  if( sscanf( *line, "%d %d %d%n", &reason, &start_line, &end_line, &chars_read ) == 3 ) {

    *line = *line + chars_read;

    if( curr_mod == NULL ) {

      print_output( "Internal error:  race condition in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      printf( "race Throw B\n" );
      Throw 0;

    } else {

      /* Create the new race condition block */
      rb = race_blk_create( reason, start_line, end_line );

      /* Add the new race condition block to the current module */
      if( curr_mod->race_head == NULL ) {
        curr_mod->race_head = curr_mod->race_tail = rb;
      } else {
        curr_mod->race_tail->next = rb;
        curr_mod->race_tail       = rb;
      }

    }

  } else {

    print_output( "Unable to parse race condition line in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
    printf( "race Throw C\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param curr        Pointer to head of race condition block list.
 \param race_total  Pointer to value that will hold the total number of race conditions in this module.
 \param type_total  Pointer to array containing number of race conditions found for each violation type.

 Iterates through specified race condition block list, totaling the number of race conditions found as
 well as tallying each type of race condition.
*/
void race_get_stats(
  race_blk* curr,
  int*      race_total,
  int       type_total[][RACE_TYPE_NUM]
) { PROFILE(RACE_GET_STATS);

  int i;  /* Loop iterator */

  /* Clear totals */
  *race_total = 0;
  for( i=0; i<RACE_TYPE_NUM; i++ ) {
    (*type_total)[i] = 0;
  }

  /* Tally totals */
  while( curr != NULL ) {
    (*type_total)[curr->reason]++;
    (*race_total)++;
    curr = curr->next;
  }

  PROFILE_END;

}

/*!
 \param ofile  Pointer to output file to use
 \param head   Pointer to head of functional unit list to report

 \return Returns TRUE if any race conditions were found in the functional unit list

 Displays the summary report for race conditions for all functional units in design.
*/
static bool race_report_summary(
  FILE* ofile,
  funit_link* head
) { PROFILE(RACE_REPORT_SUMMARY);

  bool found = FALSE;  /* Return value for this function */

  while( head != NULL ) {

    if( (head->funit->type == FUNIT_MODULE) && (head->funit->stat != NULL) ) {

      found = (head->funit->stat->race_total > 0) ? TRUE : found;

      fprintf( ofile, "  %-20.20s    %-20.20s        %d\n", 
               funit_flatten_name( head->funit ),
  	       get_basename( obf_file( head->funit->filename ) ),
  	       head->funit->stat->race_total );

    }

    head = head->next;

  }

  PROFILE_END;

  return( found );

}

/*!
 \param ofile  Pointer to output file to use
 \param head   Pointer to head of functional unit list being reported

 Outputs a verbose race condition report to the specified output file specifying
 the line number and race condition reason for all functional units in the design.
*/
static void race_report_verbose(
  FILE* ofile,
  funit_link* head
) { PROFILE(RACE_REPORT_VERBOSE);

  race_blk* curr_race;  /* Pointer to current race condition block */

  while( head != NULL ) {

    if( (head->funit->stat != NULL) && (head->funit->stat->race_total > 0) ) {

      fprintf( ofile, "\n" );
      switch( head->funit->type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", obf_funit( funit_flatten_name( head->funit ) ), obf_file( head->funit->filename ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      fprintf( ofile, "      Starting Line #     Race Condition Violation Reason\n" );
      fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

      curr_race = head->funit->race_head;
      while( curr_race != NULL ) {
        fprintf( ofile, "              %7d:    %s\n", curr_race->start_line, race_msgs[curr_race->reason] );
	curr_race = curr_race->next;
      }

      fprintf( ofile, "\n" );

    }

    head = head->next;
											       
  }

  PROFILE_END;

}

/*!
 \param ofile    Output stream to display report information to.
 \param verbose  Specifies if summary or verbose output should be displayed.

 Generates the race condition report information and displays it to the specified
 output stream.
*/
void race_report(
  FILE* ofile,
  bool verbose
) { PROFILE(RACE_REPORT);

  bool found;  /* If set to TRUE, race conditions were found */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   RACE CONDITION VIOLATIONS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "Module                    Filename                 Number of Violations found\n" );
  fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

  found = race_report_summary( ofile, funit_head );

  if( verbose && found ) {
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    race_report_verbose( ofile, funit_head );
  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

/*!
 \param funit_name  Name of funtional unit to search for
 \param funit_type  Type of funtional unit to search for
 \param slines      Pointer to an array of starting line numbers that contain line numbers of race condition statements
 \param elines      Pointer to an array of ending line numbers that contain line numbers of race condition statements
 \param reasons     Pointer to an array of race condition reason integers, one for each line in the lines array
 \param line_cnt    Pointer to number of elements that exist in lines array

 \return Returns TRUE if the specified module name was found in the design; otherwise, returns FALSE.

 Collects all of the line numbers in the specified module that were ignored from coverage due to
 detecting a race condition.  This function is primarily used by the GUI for outputting purposes.
*/
bool race_collect_lines(
            const char* funit_name,
            int         funit_type,
  /*@out@*/ int**       slines,
  /*@out@*/ int**       elines,
  /*@out@*/ int**       reasons,
  /*@out@*/ int*       line_cnt
) { PROFILE(RACE_COLLECT_LINES);

  bool        retval    = TRUE;  /* Return value for this function */
  funit_link* modl;              /* Pointer to found module link containing specified module */
  race_blk*   curr_race = NULL;  /* Pointer to current race condition block */
  int         line_size = 20;    /* Current number of lines allocated in lines array */

  if( (modl = funit_link_find( funit_name, funit_type, funit_head )) != NULL ) {

    /* Begin by allocating some memory for the lines */
    *slines   = (int*)malloc_safe( sizeof( int ) * line_size );
    *elines   = (int*)malloc_safe( sizeof( int ) * line_size );
    *reasons  = (int*)malloc_safe( sizeof( int ) * line_size );
    *line_cnt = 0;

    curr_race = modl->funit->race_head;
    while( curr_race != NULL ) {
      if( *line_cnt == line_size ) {
        line_size += 20;
        *slines  = (int*)realloc( *slines,  (sizeof( int ) * line_size) );
        *elines  = (int*)realloc( *elines,  (sizeof( int ) * line_size) );
        *reasons = (int*)realloc( *reasons, (sizeof( int ) * line_size) );
      }
      (*slines)[*line_cnt]  = curr_race->start_line;
      (*elines)[*line_cnt]  = curr_race->end_line;
      (*reasons)[*line_cnt] = curr_race->reason;
      (*line_cnt)++;
      curr_race = curr_race->next;
    }

  } else { 

    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param rb  Pointer to race condition block to deallocate.

 Recursively deallocates the specified race condition block list.
*/
void race_blk_delete_list(
  race_blk* rb
) { PROFILE(RACE_BLK_DELETE_LIST);

  if( rb != NULL ) {

    /* Delete the next race condition block first */
    race_blk_delete_list( rb->next );

    /* Deallocate the memory for this race condition block */
    free_safe( rb );

  }

  PROFILE_END;

}

/*
 $Log$
 Revision 1.75  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.74  2008/02/28 03:53:17  phase1geo
 Code addition to support feature request 1902840.  Added race6 diagnostic and updated
 race5 diagnostics per this change.  For loop control assignments are now no longer
 considered when performing race condition checking.

 Revision 1.73  2008/02/25 20:43:49  phase1geo
 Checking in code to allow the use of racecheck pragmas.  Added new tests to
 regression suite to verify this functionality.  Still need to document in
 User's Guide and manpage.

 Revision 1.72  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.71  2008/02/11 14:00:09  phase1geo
 More updates for exception handling.  Regression passes.

 Revision 1.70  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.69  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.68  2008/01/23 20:48:03  phase1geo
 Fixing bug 1878134 and adding new diagnostics to regression suite to verify
 its behavior.  Full regressions pass.

 Revision 1.67  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.66  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.65  2008/01/15 23:01:15  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.64  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.63  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.62  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.61  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.60  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.59  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.58  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.57  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.56  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.55  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.54  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.53  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.52  2006/12/14 04:19:35  phase1geo
 More updates to parser and associated code to handle unnamed scopes and
 fixing more code to use functional unit pointers in expressions instead of
 statement pointers.  Still not fully compiling at this point.  Checkpointing.

 Revision 1.51  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.50  2006/10/04 22:04:16  phase1geo
 Updating rest of regressions.  Modified the way we are setting the memory rd
 vector data bit (should optimize the score command just a bit).  Also updated
 quite a bit of memory coverage documentation though I still need to finish
 documenting how to understand the report file for this metric.  Cleaning up
 other things and fixing a few more software bugs from regressions.  Added
 marray2* diagnostics to verify endianness in the unpacked dimension list.

 Revision 1.49  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.48  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.47  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.46  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.45  2006/09/05 21:00:45  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.44  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.43  2006/08/11 18:57:04  phase1geo
 Adding support for always_comb, always_latch and always_ff statement block
 types.  Added several diagnostics to regression suite to verify this new
 behavior.

 Revision 1.42  2006/07/21 20:12:46  phase1geo
 Fixing code to get generated instances and generated array of instances to
 work.  Added diagnostics to verify correct functionality.  Full regression
 passes.

 Revision 1.41  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.40  2006/04/13 22:17:47  phase1geo
 Adding the beginning of the OVL assertion extractor.  So far the -a option is
 parsed and the race condition checker is turned off for all detectable
 OVL assertion modules (we will trust that these modules don't have race conditions
 inherent in them).

 Revision 1.39  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.38  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.37  2006/03/27 17:37:23  phase1geo
 Fixing race condition output.  Some regressions may fail due to these changes.

 Revision 1.36  2006/02/10 16:44:29  phase1geo
 Adding support for register assignment.  Added diagnostic to regression suite
 to verify its implementation.  Updated TODO.  Full regression passes at this
 point.

 Revision 1.35  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.34  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.33  2005/12/23 20:59:34  phase1geo
 Fixing assertion error in race condition checker.  Full regression runs cleanly.

 Revision 1.32  2005/12/23 05:41:52  phase1geo
 Fixing several bugs in score command per bug report #1388339.  Fixed problem
 with race condition checker statement iterator to eliminate infinite looping (this
 was the problem in the original bug).  Also fixed expression assigment when static
 expressions are used in the LHS (caused an assertion failure).  Also fixed the race
 condition checker to properly pay attention to task calls, named blocks and fork
 statements to make sure that these are being handled correctly for race condition
 checking.  Fixed bug for signals that are on the LHS side of an assignment expression
 but is not being assigned (bit selects) so that these are NOT considered for race
 conditions.  Full regression is a bit broken now but the opened bug can now be closed.

 Revision 1.31  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.30  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.29  2005/11/25 22:03:20  phase1geo
 Fixing bugs in race condition checker when racing statement blocks are in
 different functional units.  Still some work to do here with what to do when
 conflicting statement block is in a task/function (I suppose we need to remove
 the calling statement block as well?)

 Revision 1.28  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

 Revision 1.27  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.26  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.25  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.24  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.23  2005/02/07 22:19:46  phase1geo
 Added code to output race condition reasons to informational bar.  Also added code to
 output toggle and combinational logic output to information bar when cursor is over
 an expression that, when clicked on, will take you to the detailed coverage window.

 Revision 1.22  2005/02/05 06:20:58  phase1geo
 Added ascii report output for race conditions.  There is a segmentation fault
 bug associated with instance reporting.  Need to look into further.

 Revision 1.21  2005/02/05 05:29:25  phase1geo
 Added race condition reporting to GUI.

 Revision 1.20  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.19  2005/02/04 23:55:53  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.18  2005/02/03 05:48:33  phase1geo
 Fixing bugs in race condition checker.  Adding race2.1 diagnostic.  Regression
 currently has some failures due to these changes.

 Revision 1.17  2005/02/03 04:59:13  phase1geo
 Fixing bugs in race condition checker.  Updated regression.

 Revision 1.16  2005/02/01 05:11:18  phase1geo
 Updates to race condition checker to find blocking/non-blocking assignments in
 statement block.  Regression still runs clean.

 Revision 1.15  2005/01/27 13:33:50  phase1geo
 Added code to calculate if statement block is sequential, combinational, both
 or none.

 Revision 1.14  2005/01/25 13:42:27  phase1geo
 Fixing segmentation fault problem with race condition checking.  Added race1.1
 to regression.  Removed unnecessary output statements from previous debugging
 checkins.

 Revision 1.13  2005/01/11 14:24:16  phase1geo
 Intermediate checkin.

 Revision 1.12  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.11  2005/01/10 13:44:58  phase1geo
 Fixing case where signal selects are being assigned by different statements that
 do not overlap.  We do not have race conditions in this case.

 Revision 1.10  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.9  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.8  2005/01/04 14:37:00  phase1geo
 New changes for race condition checking.  Things are uncompilable at this
 point.

 Revision 1.7  2004/12/20 04:12:00  phase1geo
 A bit more race condition checking code added.  Still not there yet.

 Revision 1.6  2004/12/18 16:23:18  phase1geo
 More race condition checking updates.

 Revision 1.5  2004/12/17 22:29:35  phase1geo
 More code added to race condition feature.  Still not usable.

 Revision 1.4  2004/12/17 14:27:46  phase1geo
 More code added to race condition checker.  This is in an unusable state at
 this time.

 Revision 1.3  2004/12/16 23:31:48  phase1geo
 More work done on race condition code.

 Revision 1.2  2004/12/16 15:22:01  phase1geo
 Adding race.c compilation to Makefile and adding documentation to race.c.

*/

