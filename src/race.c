/*!
 \file     race.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/15/2004

 \par
 The contents of this file contain functions to handle race condition checking and information
 handling regarding race conditions.  Since race conditions can cause the Covered simulator to not
 provide the same run-time order as the primary Verilog simulator, we need to be able to detect when
 race conditions are occurring within the design.  The following conditions are checked within the design
 during the parsing stage.

 \par
 -# All sequential logic uses non-blocking assignments.
 -# All latches use non-blocking assignments.
 -# All combinational logic in an always block uses blocking assignments.
 -# All mixed sequential and combinational logic in the same always block uses non-blocking assignments.
 -# Blocking and non-blocking assignments should not be used in the same always block.
 -# Assignments made to a variable should only be done within one always block.
 -# The $strobe system call should only be used to display variables that were assigned using non-blocking
    assignments.
 -# No #0 procedural assignments should exist.

 \par
 The failure of any one of these rules will cause Covered to either display warning type messages to the user when
 the race condition checking flag has not been set or error messages when the race condition checking flag has
 been set by the user.
*/


#include "defines.h"
#include "race.h"


extern bool race_flag = false;


bool race_find_lhs_sigs( expression* exp, statement* root, bool* blocking ) {

  bool found = FALSE;  /* Return value for this function */

  if( exp != NULL ) {

    /* If the current expression was an assignment operator, return if it was blocking/non-blocking */
    if( (SUPPL_OP( exp->suppl ) == EXP_OP_ASSIGN) ||
        (SUPPL_OP( exp->suppl ) == EXP_OP_BASSIGN) ) {
      *blocking = TRUE;
    }

    /* If the current expression is a signal that is on the LHS, add it to the stmt_sig array */
    if( (SUPPL_IS_LHS( exp->suppl ) == 1) &&
        ((SUPPL_OP( exp->suppl ) == EXP_OP_SIG     ) ||
         (SUPPL_OP( exp->suppl ) == EXP_OP_SBIT_SEL) ||
         (SUPPL_OP( exp->suppl ) == EXP_OP_MBIT_SEL)) ) {

      /* Create new stmt_sig structure */
      new_stmt_sig       = (stmt_sig*)malloc( sizeof( stmt_sig ) );
      new_stmt_sig->sig  = exp->sig;
      new_stmt_sig->stmt = root;
      new_stmt_sig->next = NULL;
      
      /* Add new stmt_sig structure to list */
      if( stmt_sig_head == NULL ) {
        stmt_sig_head = stmt_sig_tail = new_stmt_sig;
      } else {
        stmt_sig_tail->next = new_stmt_sig;
        stmt_sig_tail       = new_stmt_sig;
      }

    }

    /*
     TBD - We could make this code more efficient by first searching for an assignment operator and then
     searching for all LHS signals.
    */
    race_find_lhs_sigs( exp->left  );
    race_find_lhs_sigs( exp->right );

  } 

}

/*!
 \param stmt  Pointer to current statement to find signals for.
 \param root  Pointer to root statement in statement tree.

 Finds all signals within the given statement tree that are assigned within this tree.
 When this list of signals is found, new stmt_sig links are added to the stmt_sig linked
 list, setting the signal and top-level statement values into these links.  If a signal
 is found within this statement tree that is also in the current stmt_sig list and the
 top-level statements are found to be different, an error message is displayed to the user
 (depending on the race condition flag set in the score command).
*/
void race_find_and_add_stmt_sigs( statement* stmt, statement* root ) {

  expression* curr_expr;  /* Pointer to current expression in stmt expression tree */

  if( stmt != NULL ) {

    /* First, find all signals in the current statement that are labeled as LHS */
    curr_expr = stmt->exp;

    while( curr_expr != NULL ) {

    assert( curr_sigl->sig != NULL );
    assert( curr_sigl->sig->value != NULL );

    if( SUPPL_IS_LHS( curr_sigl->sig->value->suppl ) == 1 ) {

      /* This signal is assigned within this statement */
    }

    curr_sigl = curr_sigl->next;

  }

}


/*
 $Log$
 Revision 1.2  2004/12/16 15:22:01  phase1geo
 Adding race.c compilation to Makefile and adding documentation to race.c.

*/

