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


stmt_sig* stmt_sig_head = NULL;
stmt_sig* stmt_sig_tail = NULL;

// extern bool race_flag = false;


/*!
 \param ss    Pointer to current stmt_sig structure to search for in specified list
 \param curr  Pointer to head of stmt_sig list to search

 \return Returns TRUE if a matching stmt_sig structure exists in the specified list to
         the specified stmt_sig structure
*/
bool race_does_matching_stmt_sig_exist( stmt_sig* ss, stmt_sig* curr ) {

  while( (curr != NULL) && (curr->sig != ss->sig) && (curr->blocking != ss->blocking) ) {
    curr = curr->next;
  }

  return( curr != NULL );

}

/*!
 \param exp       Pointer to current expression to evaluate
 \param root      Pointer to root statement in statement tree
 \param blocking  Specifies if blocking expression was found in parent expression
 \param head      Pointer to head of stmt_sig list that was found for this statement
 \param tail      Pointer to tail of stmt_sig list that was found for this statement

 Recursively searches specified expression tree for LHS assigned signals.  When a
 signal is found that matches this criteria, it is added to the current stmt_sig list
 if a matching stmt_sig was not found already in this list.
*/
void race_find_lhs_sigs( expression* exp, statement* root, bool blocking, stmt_sig** head, stmt_sig** tail ) {

  stmt_sig* tmp_stmt_sig;  /* Pointer to temporary stmt_sig structure */

  if( exp != NULL ) {

    /* If the current expression was an assignment operator, return if it was blocking/non-blocking */
    if( (SUPPL_OP( exp->suppl ) == EXP_OP_ASSIGN) ||
        (SUPPL_OP( exp->suppl ) == EXP_OP_BASSIGN) ) {

      blocking = TRUE;

    /* If the current expression is a signal that is on the LHS, add it to the stmt_sig array */
    } else if( (SUPPL_OP( exp->suppl ) == EXP_OP_SIG     ) ||
               (SUPPL_OP( exp->suppl ) == EXP_OP_SBIT_SEL) ||
               (SUPPL_OP( exp->suppl ) == EXP_OP_MBIT_SEL) ) {

      if( SUPPL_IS_LHS( exp->suppl ) == 1 ) {

        /* Create new stmt_sig structure */
        tmp_stmt_sig           = (stmt_sig*)malloc( sizeof( stmt_sig ) );
        tmp_stmt_sig->sig      = exp->sig;
        tmp_stmt_sig->stmt     = root;
  	tmp_stmt_sig->blocking = blocking;
        tmp_stmt_sig->next     = NULL;
      
	/* If we did not find a match, go ahead and add it to the list */
	if( !race_does_matching_stmt_sig_exist( tmp_stmt_sig, *head ) ) {

          /* Add new stmt_sig structure to list */
          if( *head == NULL ) {
            *head = *tail = tmp_stmt_sig;
          } else {
            (*tail)->next = tmp_stmt_sig;
            *tail         = tmp_stmt_sig;
          }

        } else {

	  /* Deallocate the malloc'ed structure */ 
          race_stmt_sig_dealloc( tmp_stmt_sig );

        }

      }

    } else {

      /*
       TBD - We could make this code more efficient by first searching for an assignment operator and then
       searching for all LHS signals.
      */
      race_find_lhs_sigs( exp->left,  root, blocking, head, tail );
      race_find_lhs_sigs( exp->right, root, blocking, head, tail );

    } 

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

  stmt_sig* tmp_head;  /* Pointer to head of temporary stmt_sig list        */
  stmt_sig* tmp_tail;  /* Pointer to tail of temporary stmt_sig list        */
  stmt_sig* curr;      /* Pointer to current stmt_sig structure to evaluate */
  stmt_sig* last;      /* Pointer to last stmt_sig structure evaluated      */

  if( stmt != NULL ) {

    /* First, find all signals in the current statement that are labeled as LHS */
    race_find_lhs_sigs( stmt->exp, root, FALSE, &tmp_head, &tmp_tail );

    /* Next, remove all matching stmt_sig in temporary list */
    curr = tmp_head;
    last = NULL;
    while( curr != NULL ) {
      if( !race_does_matching_stmt_sig_exist( curr, stmt_sig_head ) ) {
	if( stmt_sig_head == NULL ) {
          stmt_sig_head = stmt_sig_tail = curr;
        } else {
          stmt_sig_tail->next = curr;
	  stmt_sig_tail       = curr;
        }
      } else {
        curr = curr->next;
      } 
    }

  }

}

/*!
 \param ss  Pointer to stmt_sig structure to deallocate

 Recursively deallocates specified stmt_sig structure/list from memory.
*/
void race_stmt_sig_dealloc( stmt_sig* ss ) {

  if( ss != NULL ) {

    /* Remove next stmt_sig before removing ourselves */
    race_stmt_sig_dealloc( ss->next );

    /* Deallocate ourselves now */
    free_safe( ss );

  }

}

/*
 $Log$
 Revision 1.3  2004/12/16 23:31:48  phase1geo
 More work done on race condition code.

 Revision 1.2  2004/12/16 15:22:01  phase1geo
 Adding race.c compilation to Makefile and adding documentation to race.c.

*/

