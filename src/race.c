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

#include <stdio.h>

#include "defines.h"
#include "race.h"


stmt_sig* stmt_sig_head = NULL;
stmt_sig* stmt_sig_tail = NULL;
int       races_found   = 0;

extern bool flag_race_check;
extern char user_msg[USER_MSG_LENGTH];


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

 Recursively searches specified expression tree for LHS assigned signals.  When a
 signal is found that matches this criteria, it is added to the current stmt_sig list
 if a matching stmt_sig was not found already in this list.
*/
void race_find_lhs_sigs( expression* exp, statement* root, bool blocking ) {

  stmt_sig* tmpss;  /* Pointer to temporary stmt_sig structure */

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

        /* Check to see if any elements in stmt_sig list match this element */
        tmpss = stmt_sig_head;
        while( (tmpss != NULL) && (exp->sig != tmpss->sig) && (blocking != tmpss->blocking) && (root != tmpss->stmt) ) {
          tmpss = tmpss->next;
        }

        /* If we don't have a match, go ahead and create the stmt_sig structure and add it to the list */
        if( tmpss == NULL ) {
        
          /* Create new stmt_sig structure */
          tmpss           = (stmt_sig*)malloc_safe( sizeof( stmt_sig ) );
          tmpss->sig      = exp->sig;
          tmpss->stmt     = root;
  	  tmpss->blocking = blocking;
          tmpss->next     = NULL;
      
          /* Add new stmt_sig structure to list */
          if( stmt_sig_head == NULL ) {
            stmt_sig_head = stmt_sig_tail = tmpss;
          } else {
            stmt_sig_tail->next = tmpss;
            stmt_sig_tail       = tmpss;
          }

        }

      }

    } else {

      /*
       TBD - We could make this code more efficient by first searching for an assignment operator and then
       searching for all LHS signals.
      */
      race_find_lhs_sigs( exp->left,  root, blocking );
      race_find_lhs_sigs( exp->right, root, blocking );

    } 

  } 

}

/*!
 \param sig  Pointer to input port signal.

 Creates a new stmt_sig structure to hold the specified signal information and adds it to the stmt_sig
 list for future processing.  This should be called by the parser whenever a new input port signal has
 been found.
*/
void race_add_inport_sig( vsignal* sig ) {

  stmt_sig* ss;  /* Pointer to newly created stmt_sig structure */

  /* The specified signal should never be a NULL value */
  assert( sig != NULL );

  /* Create new stmt_sig for this port -- setting root stmt to NULL to indicate that this is a port */
  ss = (stmt_sig*)malloc_safe( sizeof( stmt_sig ) );
  ss->sig      = sig;
  ss->stmt     = NULL;  /* Indicates that this is an input port */
  ss->blocking = TRUE;  /* Assume that this port has been assigned in a blocking fashion */
  ss->next     = NULL;

  /* Add the new signal to the list -- we shouldn't have duplicates here so don't check */
  if( stmt_sig_head == NULL ) {
    stmt_sig_head = stmt_sig_tail = ss;
  } else {
    stmt_sig_tail->next = ss;
    stmt_sig_tail       = ss;
  }

  printf( "Added input port\n" );
  race_stmt_sig_display();

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

  stmt_sig* curr;      /* Pointer to current stmt_sig structure to evaluate */
  stmt_sig* last;      /* Pointer to last stmt_sig structure evaluated      */

  assert( stmt != NULL );
  assert( root != NULL );

  if( stmt != NULL ) {

    /* Find all LHS assigned signals in current statement expression tree */
    race_find_lhs_sigs( stmt->exp, root, FALSE );

    /* Traverse TRUE and FALSE paths if STOP and CONTINUOUS bits are not set */
    if( (SUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0) &&
        (SUPPL_IS_STMT_CONTINUOUS( stmt->exp->suppl ) == 0) ) {

      race_find_and_add_stmt_sigs( stmt->next_true,  root );

      /* Traverse FALSE path if it differs from TRUE path */
      if( stmt->next_true != stmt->next_false ) {
        race_find_and_add_stmt_sigs( stmt->next_false, root );
      }

    }

    race_stmt_sig_display();

  }

}

/*!
 Displays contents of stmt_sig structure array to standard output.  For debugging purposes only.
*/
void race_stmt_sig_display() {

  stmt_sig* curr;  /* Pointer to current stmt_sig structure in list */

  printf( "Statement-signal array:\n" );

  curr = stmt_sig_head;
  while( curr != NULL ) {
    if( curr->stmt == NULL ) {
      printf( "  sig_name: %s, input, blocking: %d\n", curr->sig->name, curr->blocking );
    } else {
      printf( "  sig_name: %s, stmt_id: %d, blocking: %d\n", curr->sig->name, curr->stmt->exp->id, curr->blocking );
    } 
    curr = curr->next;
  }

}

/*!
 Performs race checking for the currently loaded module.  At this point, the stmt_sig array will be fully
 populated.  After race checking has been completed, the stmt_sig array will be completely deallocated.  This
 function should be called when the endmodule keyword is detected in the current module.
*/
void race_check_module() {

  stmt_sig* comp;  /* Pointer to stmt_sig to compare to    */
  stmt_sig* curr;  /* Pointer to current stmt_sig to check */

  comp = stmt_sig_head;

  while( comp != NULL ) {

    curr = comp->next;

    while( curr != NULL ) {

      /* If the signal to compare was assigned in more than one place, keep checking that signal */
      if( comp->sig == curr->sig ) {

	/* If the signal was assigned in the same statement, checking blocking value */
	if( comp->stmt == curr->stmt ) {

          /* If we have mixed assignment types in the same statement */
          if( comp->blocking != curr->blocking ) {
            if( flag_race_check ) {
	    }
	  }
	}

      }

      curr = curr->next;

    }

    comp = comp->next;

  }

  /* Deallocate stmt_sig list */
  race_stmt_sig_dealloc();

}

/*!
 \return Returns TRUE if no race conditions were found or the user specified that we should continue
         to score the design.
*/
bool race_check_race_count() {

  bool retval = TRUE;  /* Return value for this function */

  /*
   If we were able to find race conditions and the user specified to check for race conditions
   and quit, display the number of race conditions found and return FALSE to cause everything to
   halt.
  */
  if( (races_found > 0) && flag_race_check ) {

    snprintf( user_msg, USER_MSG_LENGTH, "%d race conditions were detected.  Exiting score command.", races_found );
    print_output( user_msg, FATAL );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param ss  Pointer to stmt_sig structure to deallocate

 Deallocates stmt_sig structure list from memory.
*/
void race_stmt_sig_dealloc() {

  stmt_sig* curr;  /* Pointer to current stmt_sig element to deallocate   */
  stmt_sig* tmp;   /* Temporary pointer to stmt_sig element to deallocate */

  /* Deallocate all elements in stmt_sig list */
  curr = stmt_sig_head;
  while( curr != NULL ) {
    tmp  = curr;
    curr = curr->next;
    free_safe( tmp );
  }

  /* Set head and tail to NULL */
  stmt_sig_head = NULL;
  stmt_sig_tail = NULL;

}

/*
 $Log$
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

