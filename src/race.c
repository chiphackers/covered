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
const char* race_msgs[RACE_TYPE_NUM] = {
  "Sequential statement block contains blocking assignment(s)",
  "Combinational statement block contains non-blocking assignment(s)",
  "Mixed statement block contains blocking assignment(s)",
  "Statement block contains both blocking and non-blocking assignment(s)", 
  "Signal assigned in two different statement blocks",
  "Signal assigned both in statement block and via input/inout port",
  "System call $strobe used to output signal assigned via blocking assignment",
  "Procedural assignment with #0 delay performed"
};

static void race_calc_assignments( statement*, int );

extern int          flag_race_check;
extern char         user_msg[USER_MSG_LENGTH];
extern db**         db_list;
extern unsigned int curr_db;
extern func_unit*   curr_funit;
extern isuppl       info_suppl;
extern int          stmt_conn_id;
extern bool         debug_mode;
extern bool         debug_mode;
extern str_link*    race_ignore_mod_head;
extern str_link*    race_ignore_mod_tail;

/*!
 \return Returns a pointer to the newly allocated race condition block.

 Allocates and initializes memory for a race condition block.
*/
static race_blk* race_blk_create(
  int reason,      /*!< Numerical reason for why race condition was detected */
  int start_line,  /*!< Starting line of race condition block */
  int end_line     /*!< Ending line of race condition block */
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

#ifndef RUNLIB
/*!
 \return Returns TRUE if the specified to_find statement was found in the statement block specified
         by curr; otherwise, returns FALSE.

 Recursively iterates through specified statement block, searching for statement pointed to by to_find.
 If the statement is found, a value of TRUE is returned to the calling function; otherwise, a value of
 FALSE is returned.
*/
static bool race_find_head_statement_containing_statement_helper(
  statement* curr,    /*!< Pointer to current statement to check */
  statement* to_find  /*!< Pointer to statement to find */
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
 \return Returns a pointer to the statement block containing the given statement (if one is found); otherwise,
         returns a value of NULL.

 Searches all statement blocks for one that contains the given statement within it.  Once it is found, a pointer
 to the head statement block is returned to the calling function.  If it is not found, a value of NULL is returned,
 indicating that the statement block could not be found for the given statement.
*/
static statement* race_find_head_statement_containing_statement(
  statement* stmt  /*!< Pointer to statement to search for matching statement block */
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
 \return Returns pointer to head statement of statement block containing the specified expression

 Finds the head statement of the statement block containing the expression specified in the parameter list.
 Verifies that the return value is never NULL (this would be an internal error if it existed).
*/
static statement* race_get_head_statement(
  expression* expr  /*!< Pointer to root expression to find in statement list */
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
 \return Returns index of found head statement in statement block array if found; otherwise,
         returns a value of -1 to indicate that no such statement block exists.
*/
static int race_find_head_statement(
  statement* stmt  /*!< Pointer to statement to search for in statement block array */
) { PROFILE(RACE_FIND_HEAD_STATEMENT);

  int i = 0;   /* Loop iterator */

  while( (i < sb_size) && (sb[i].stmt != stmt) ) {
    i++;
  }

  PROFILE_END;

  return( (i == sb_size) ? -1 : i );

}

/*!
 Recursively iterates down expression tree, searching for an expression which will indicate that
 its statement block is a sequential always block or a combinational always block.
*/
static void race_calc_stmt_blk_type(
  expression* expr,     /*!< Pointer to expression to check for statement block type (sequential/combinational) */
  int         sb_index  /*!< Current statement block index containing the given expression */
) { PROFILE(RACE_CALC_STMT_BLK_TYPE);

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
 Checks the given expression for its assignment type and sets the given statement block
 structure to the appropriate assign type value.  If the given expression is a named block,
 iterate through that block searching for an assignment type.
*/
static void race_calc_expr_assignment( 
  expression* exp,      /*!< Pointer to expression to check for expression assignment type */
  int         sb_index  /*!< Current statement block index containing the given expression */
) { PROFILE(RACE_CALC_EXPR_ASSIGNMENT);

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
 Recursively iterates through the given statement block, searching for all assignment types used
 within the block.
*/
void race_calc_assignments(
  statement* stmt,     /*!< Pointer to statement to get assign type from */
  int        sb_index  /*!< Current statement block index containing the given statement */
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
 Outputs necessary information to user regarding the race condition that was detected and performs any
 necessary memory cleanup to remove the statement block involved in the race condition.
*/
static void race_handle_race_condition(
  expression* expr,   /*!< Pointer to expression containing signal that was found to be in a race condition */
  func_unit*  mod,    /*!< Pointer to module containing detected race condition */
  statement*  stmt,   /*!< Pointer to expr's head statement */
  statement*  base,   /*!< Pointer to head statement block that was found to be in conflict with stmt */
  int         reason  /*!< Specifies what type of race condition was being checked that failed */
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
      rv = snprintf( user_msg, USER_MSG_LENGTH, "  Signal assigned in file: %s, line: %u", obf_file( mod->orig_fname ), expr->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );

      if( flag_race_check == WARNING ) {
        print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "    Statement block starting at file: %s, line: %u",
                       obf_file( mod->orig_fname ), stmt->exp->line );
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
      rv = snprintf( user_msg, USER_MSG_LENGTH, "  Signal assigned in file: %s, line: %u", obf_file( mod->orig_fname ), expr->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "  Signal also assigned in statement starting at file: %s, line: %u",
                     obf_file( mod->orig_fname ), base->exp->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );

      if( flag_race_check == WARNING ) {
        print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "    Statement block starting at file: %s, line: %u",
                       obf_file( mod->orig_fname ), stmt->exp->line );
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
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Statement block starting in file: %s, line: %u",
                       obf_file( mod->orig_fname ), stmt->exp->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, (flag_race_check + 1), __FILE__, __LINE__ );
	if( flag_race_check == WARNING ) {
	  print_output( "  * Safely removing statement block from coverage consideration", WARNING_WRAP, __FILE__, __LINE__ );
	}

      } else {

	if( flag_race_check == WARNING ) {
          print_output( "", WARNING_WRAP, __FILE__, __LINE__ );
	  print_output( "* Safely removing statement block from coverage consideration", WARNING, __FILE__, __LINE__ );
          rv = snprintf( user_msg, USER_MSG_LENGTH, "  Statement block starting at file: %s, line: %u",
                         obf_file( mod->orig_fname ), stmt->exp->line );
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
 Performs the race condition assignment checks for each statement block.
*/
static void race_check_assignment_types(
  func_unit* mod  /*!< Pointer to functional unit to check assignment types for */
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
 Verifies that every signal is assigned in only one statement block.  If a signal is
 assigned in more than one statement block, both statement block's need to be removed
 from coverage consideration and a possible warning/error message generated to the user.
*/
static void race_check_one_block_assignment(
  func_unit* mod  /*!< Pointer to module containing signals to verify */
) { PROFILE(RACE_CHECK_ONE_BLOCK_ASSIGNMENT);

  statement*   curr_stmt;           /* Pointer to current statement */
  bool         race_found = FALSE;  /* Specifies if at least one race condition was found for this signal */
  bool         curr_race  = FALSE;  /* Set to TRUE if race condition was found in current iteration */
  int          lval;                /* Left expression value */
  int          rval;                /* Right expression value */
  int          exp_dim;             /* Current expression dimension */
  int          dim_lsb;             /* LSB of current dimension */
  bool         dim_be;              /* Big endianness of current dimension */
  int          dim_width;           /* Bit width of current dimension */
  int          intval;              /* Integer value */
  unsigned int i;

  for( i=0; i<mod->sig_size; i++ ) {

    vsignal* sig      = mod->sigs[i];
    int      sig_stmt = -1;

    /* Skip checking the expressions of genvar signals */
    if( (sig->suppl.part.type != SSUPPL_TYPE_GENVAR) && (sig->suppl.part.type != SSUPPL_TYPE_MEM) ) {

      unsigned int j;

      /* Iterate through expressions */
      for( j=0; j<sig->exp_size; j++ ) {

        expression* exp = sig->exps[j];
					      
        /* Only look at expressions that are part of LHS and they are not part of a bit select */
        if( (ESUPPL_IS_LHS( exp->suppl ) == 1) && !expression_is_bit_select( exp ) && expression_is_last_select( exp ) ) {

          /* Get head statement of current expression */
          curr_stmt = race_get_head_statement( exp );

          /* If the head statement is being ignored from race condition checking, skip the rest */
          if( (curr_stmt != NULL) && (curr_stmt->suppl.part.ignore_rc == 0) ) {

            vector* src;
            int     vwidth;

            /* Get current dimension of the given expression */
            exp_dim = expression_get_curr_dimension( exp );
  
            /* Calculate starting vector value bit and signal LSB/BE for LHS */
            if( (ESUPPL_IS_ROOT( exp->suppl ) == 0) &&
                (exp->parent->expr->op == EXP_OP_DIM) && (exp->parent->expr->right == exp) ) {
              src    = exp->parent->expr->left->value;
              vwidth = src->width;
            } else {
              /* Get starting vector bit from signal itself */
              src    = sig->value;
              vwidth = src->width;
            }
            if( sig->dim[exp_dim].lsb < sig->dim[exp_dim].msb ) {
              dim_lsb = sig->dim[exp_dim].lsb;
              dim_be  = FALSE;
            } else {
              dim_lsb = sig->dim[exp_dim].msb;
              dim_be  = TRUE;
            }
            dim_width = vsignal_calc_width_for_expr( exp, sig );

            /*
             If the signal was a part select, set the appropriate misc bits to indicate what
             bits have been assigned.
            */
            switch( exp->op ) {
              case EXP_OP_SIG :
                if( (ESUPPL_IS_ROOT( exp->suppl ) == 0) && !expression_is_in_rassign( exp ) ) {
                  curr_race = vector_set_assigned( sig->value, (sig->value->width - 1), 0 );
                }
  	      break;
              case EXP_OP_SBIT_SEL :
                if( exp->left->op == EXP_OP_STATIC ) {
                  intval = (vector_to_int( exp->left->value ) - dim_lsb) * dim_width;
                  if( (intval >= 0) && ((unsigned int)intval < exp->value->width) ) {
                    if( dim_be ) {
                      int lsb = (vwidth - (intval + exp->value->width));
                      curr_race = vector_set_assigned( sig->value, ((exp->value->width - 1) + lsb), lsb );
                    } else {
                      curr_race = vector_set_assigned( sig->value, ((exp->value->width - 1) + intval), intval );
                    }
                  } else {
                    curr_race = FALSE;
                  }
                } else { 
                  curr_race = vector_set_assigned( sig->value, (sig->value->width - 1), 0 );
                }
  	      break;
              case EXP_OP_MBIT_SEL :
                if( (exp->left->op == EXP_OP_STATIC) && (exp->right->op == EXP_OP_STATIC) ) {
                  intval = ((dim_be ? vector_to_int( exp->left->value ) : vector_to_int( exp->right->value )) - dim_lsb) * dim_width;
                  if( dim_be ) {
                    int lsb = (vwidth - (intval + exp->value->width));
                    curr_race = vector_set_assigned( sig->value, ((exp->value->width - 1) + lsb), lsb );
                  } else {
                    curr_race = vector_set_assigned( sig->value, ((exp->value->width - 1) + intval), intval );
                  }
                } else {
                  curr_race = vector_set_assigned( sig->value, (sig->value->width - 1), 0 );
                }
	        break;
              case EXP_OP_MBIT_POS :
                if( exp->left->op == EXP_OP_STATIC ) {
                  lval = vector_to_int( exp->left->value );
                  rval = vector_to_int( exp->right->value );
                  curr_race = vector_set_assigned( sig->value, ((lval + rval) - sig->dim[exp_dim].lsb), (lval - sig->dim[exp_dim].lsb) );
                } else {
                  curr_race = vector_set_assigned( sig->value, (sig->value->width - 1), 0 );
                }
                break;
              case EXP_OP_MBIT_NEG :
                if( exp->left->op == EXP_OP_STATIC ) {
                  lval = vector_to_int( exp->left->value );
                  rval = vector_to_int( exp->right->value );
                  curr_race = vector_set_assigned( sig->value, ((lval + 1) - sig->dim[exp_dim].lsb), (((lval - rval) + 1) - sig->dim[exp_dim].lsb) );
                } else {
                  curr_race = vector_set_assigned( sig->value, (sig->value->width - 1), 0 );
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
                if( (sig->suppl.part.type == SSUPPL_TYPE_INPUT_NET) ||
                    (sig->suppl.part.type == SSUPPL_TYPE_INPUT_REG) ||
                    (sig->suppl.part.type == SSUPPL_TYPE_INOUT_NET) ||
                    (sig->suppl.part.type == SSUPPL_TYPE_INOUT_REG) || curr_race ) {
                  race_handle_race_condition( exp, mod, curr_stmt, NULL, RACE_TYPE_ASSIGN_IN_ONE_BLOCK2 );
                  sb[sig_stmt].remove = TRUE;
                }
  
              } else if( (sb[sig_stmt].stmt != curr_stmt) && curr_race ) {
  
                race_handle_race_condition( exp, mod, curr_stmt, sb[sig_stmt].stmt, RACE_TYPE_ASSIGN_IN_ONE_BLOCK1 );
                sb[sig_stmt].remove = TRUE;
                race_found = TRUE;
  
              }
  
            }

          }

        }

      }

    }

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
  stmt_link*  curr;      /* Statement iterator */
  funit_link* modl;      /* Pointer to current module link */
  int         i;         /* Loop iterator */
  int         ignore;    /* Placeholder */
  funit_inst* inst;      /* Instance of this functional unit */

  modl = db_list[curr_db]->funit_head;

  while( modl != NULL ) {

    /* Get instance */
    ignore = 0;
    inst   = inst_link_find_by_funit( modl->funit, db_list[curr_db]->inst_head, &ignore );

    /* Only perform race condition checking for modules that are instantiated and are not OVL assertions and are not ignored */
    if( (modl->funit->suppl.part.type == FUNIT_MODULE) &&
        (inst != NULL) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( modl->funit )) &&
        (str_link_find( modl->funit->name, race_ignore_mod_head ) == NULL) ) {

      /* Size elements for the current module */
      funit_size_elements( modl->funit, inst, FALSE, FALSE );

      /* Clear statement block array size */
      sb_size = 0;

      /* First, get the size of the statement block array for this module */
      curr = modl->funit->stmt_head;
      while( curr != NULL ) {
        if( (curr->stmt->suppl.part.head == 1) &&
            (curr->stmt->suppl.part.is_called == 0) ) {
          sb_size++;
        }
        curr = curr->next;
      }

      if( sb_size > 0 ) {

        /* Allocate memory for the statement block array and clear current index */
        sb       = (stmt_blk*)malloc_safe( sizeof( stmt_blk ) * sb_size );
        sb_index = 0;

        /* Second, populate the statement block array with pointers to the head statements */
        curr = modl->funit->stmt_head;
        while( curr != NULL ) {
          if( (curr->stmt->suppl.part.head == 1) &&
              (curr->stmt->suppl.part.is_called == 0) ) {
            sb[sb_index].stmt    = curr->stmt;
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
          curr = curr->next;
        }

        /* Perform checks #1 - #5 */
        race_check_assignment_types( modl->funit );

        /* Perform check #6 */
        race_check_one_block_assignment( modl->funit );

        /* Cleanup statements to be removed */
        curr_funit = modl->funit;
        for( i=0; i<sb_size; i++ ) {
          if( sb[i].remove ) {
#ifdef DEBUG_MODE
            if( debug_mode ) {
              print_output( "Removing statement block because it was found to have a race condition", DEBUG, __FILE__, __LINE__ );
            }
#endif 
            stmt_blk_add_to_remove_list( sb[i].stmt );
          }
        }

        /* Deallocate stmt_blk list */
        free_safe( sb, (sizeof( stmt_blk ) * sb_size) );

      }

    }

    modl = modl->next;

  }

  /* Handle output if any race conditions were found */
  race_check_race_count();

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Writes contents of specified race condition block to the specified output stream.
*/
void race_db_write(
  race_blk* rb,   /*!< Pointer to race condition block to write to specified output file */
  FILE*     file  /*!< File handle of output stream to write */
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
 \throws anonymous Throw Throw

 Reads the specified line from the CDD file and parses it for race condition block
 information.
*/
void race_db_read(
  /*@out@*/ char**     line,     /*!< Pointer to line containing information for a race condition block */
            func_unit* curr_mod  /*!< Pointer to current module to store race condition block to */
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
    Throw 0;

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Iterates through specified race condition block list, totaling the number of race conditions found as
 well as tallying each type of race condition.
*/
void race_get_stats(
            race_blk*     curr,                        /*!< Pointer to head of race condition block list */
  /*@out@*/ unsigned int* race_total,                  /*!< Pointer to value that will hold the total number of race conditions in this module */
  /*@out@*/ unsigned int  type_total[][RACE_TYPE_NUM]  /*!< Pointer to array containing number of race conditions found for each violation type */
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
 \return Returns TRUE if any race conditions were found in the functional unit list

 Displays the summary report for race conditions for all functional units in design.
*/
static bool race_report_summary(
  FILE*       ofile,  /*!< Pointer to output file to use */
  funit_link* head    /*!< Pointer to head of functional unit list to report */
) { PROFILE(RACE_REPORT_SUMMARY);

  bool found = FALSE;  /* Return value for this function */

  while( head != NULL ) {

    if( (head->funit->suppl.part.type == FUNIT_MODULE) && (head->funit->stat != NULL) ) {

      found = (head->funit->stat->race_total > 0) ? TRUE : found;

      fprintf( ofile, "  %-20.20s    %-20.20s        %u\n", 
               funit_flatten_name( head->funit ),
  	       get_basename( obf_file( head->funit->orig_fname ) ),
  	       head->funit->stat->race_total );

    }

    head = head->next;

  }

  PROFILE_END;

  return( found );

}

/*!
 Outputs a verbose race condition report to the specified output file specifying
 the line number and race condition reason for all functional units in the design.
*/
static void race_report_verbose(
  FILE*       ofile,  /*!< Pointer to output file to use */
  funit_link* head    /*!< Pointer to head of functional unit list being reported */
) { PROFILE(RACE_REPORT_VERBOSE);

  race_blk* curr_race;  /* Pointer to current race condition block */

  while( head != NULL ) {

    if( (head->funit->stat != NULL) && (head->funit->stat->race_total > 0) ) {

      fprintf( ofile, "\n" );
      switch( head->funit->suppl.part.type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", obf_funit( funit_flatten_name( head->funit ) ), obf_file( head->funit->orig_fname ) );
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
 Generates the race condition report information and displays it to the specified
 output stream.
*/
void race_report(
  FILE* ofile,   /*!< Output stream to display report information to */
  bool  verbose  /*!< Specifies if summary or verbose output should be displayed */
) { PROFILE(RACE_REPORT);

  bool found;  /* If set to TRUE, race conditions were found */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   RACE CONDITION VIOLATIONS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "Module                    Filename                 Number of Violations found\n" );
  fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

  found = race_report_summary( ofile, db_list[curr_db]->funit_head );

  if( verbose && found ) {
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    race_report_verbose( ofile, db_list[curr_db]->funit_head );
  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

/*!
 Collects all of the line numbers in the specified module that were ignored from coverage due to
 detecting a race condition.  This function is primarily used by the GUI for outputting purposes.
*/
void race_collect_lines(
            func_unit* funit,    /*!< Pointer to functional unit */
  /*@out@*/ int**      slines,   /*!< Pointer to an array of starting line numbers that contain line numbers of race condition statements */
  /*@out@*/ int**      elines,   /*!< Pointer to an array of ending line numbers that contain line numbers of race condition statements */
  /*@out@*/ int**      reasons,  /*!< Pointer to an array of race condition reason integers, one for each line in the lines array */
  /*@out@*/ int*       line_cnt  /*!< Pointer to number of elements that exist in lines array */
) { PROFILE(RACE_COLLECT_LINES);

  race_blk* curr_race = NULL;  /* Pointer to current race condition block */

  /* Begin by allocating some memory for the lines */
  *slines   = NULL;
  *elines   = NULL;
  *reasons  = NULL;
  *line_cnt = 0;

  curr_race = funit->race_head;
  while( curr_race != NULL ) {

    *slines  = (int*)realloc_safe( *slines,  (sizeof( int ) * (*line_cnt)), (sizeof( int ) * (*line_cnt + 1)) );
    *elines  = (int*)realloc_safe( *elines,  (sizeof( int ) * (*line_cnt)), (sizeof( int ) * (*line_cnt + 1)) );
    *reasons = (int*)realloc_safe( *reasons, (sizeof( int ) * (*line_cnt)), (sizeof( int ) * (*line_cnt + 1)) );

    (*slines)[*line_cnt]  = curr_race->start_line;
    (*elines)[*line_cnt]  = curr_race->end_line;
    (*reasons)[*line_cnt] = curr_race->reason;
    (*line_cnt)++;

    curr_race = curr_race->next;

  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Recursively deallocates the specified race condition block list.
*/
void race_blk_delete_list(
  race_blk* rb  /*!< Pointer to race condition block to deallocate */
) { PROFILE(RACE_BLK_DELETE_LIST);

  if( rb != NULL ) {

    /* Delete the next race condition block first */
    race_blk_delete_list( rb->next );

    /* Deallocate the memory for this race condition block */
    free_safe( rb, sizeof( race_blk ) );

  }

  PROFILE_END;

}
