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
#include "db.h"
#include "util.h"
#include "vsignal.h"


stmt_blk* stmt_blk_head = NULL;
stmt_blk* stmt_blk_tail = NULL;
int       races_found   = 0;

extern bool      flag_race_check;
extern char      user_msg[USER_MSG_LENGTH];
extern mod_link* mod_head;


statement* race_get_head_statement( module* mod, expression* expr ) {

  stmt_iter  si;         /* Statement iterator                                     */
  statement* curr_stmt;  /* Pointer to current statement containing the expression */

  /* First, find the statement associated with this expression */
  while( (expr != NULL) && (expr->suppl.part.root == 0) ) {
    expr = expr->parent->expr;
  }
  curr_stmt = expr->parent->stmt;

  /* Second, find the head statement that contains the expression's statement */
  stmt_iter_reset( &si, mod->stmt_head );

  while( (si.curr != NULL) && (si.curr->stmt != curr_stmt) ) {
    stmt_iter_next( &si );
  }

  assert( si.curr != NULL );

  /* Back up to the head statement once we have found the matching statement */
  stmt_iter_find_head( &si, FALSE );

  assert( si.curr != NULL );

  return( si.curr->stmt );

}

void race_handle_race_condition( statement* stmt, statement* base ) {

  /* First, output the information, if specified */
  printf( "Found race condition problem with stmt %d\n", stmt->exp->id );

  exit( 1 );

  /* Increment races found flag */
  races_found++;

}

void race_check_one_block_assignment( module* mod ) {

  sig_link*  sigl;                /* Pointer to current signal                                          */
  exp_link*  expl;                /* Pointer to current expression                                      */
  statement* curr_stmt;           /* Pointer to current statement                                       */
  statement* sig_stmt;            /* Pointer to base signal statement                                   */
  bool       race_found = FALSE;  /* Specifies if at least one race condition was found for this signal */
  bool       prev_assigned;       /* Set to TRUE if signal was previously assigned                      */
  bool       curr_race;           /* Set to TRUE if race condition was found in current iteration       */

  sigl = mod->sig_head;
  while( sigl != NULL ) {

    sig_stmt = NULL;

    /* Iterate through expressions */
    expl = sigl->sig->exp_head;
    while( expl != NULL ) {
					      
      /* Only look at expressions that are part of LHS */
      if( expl->exp->suppl.part.lhs == 1 ) {      

	/*
	 If the signal was a part select, set the appropriate misc bits to indicate what
	 bits have been assigned.
        */
	switch( expl->exp->op ) {
          case EXP_OP_SIG :
            curr_race = vsignal_set_assigned( sigl->sig, ((sigl->sig->value->width - 1) + sigl->sig->lsb), sigl->sig->lsb );
	    break;
          case EXP_OP_SBIT_SEL :
            if( expl->exp->left->op == EXP_OP_STATIC ) {
              curr_race = vsignal_set_assigned( sigl->sig, vector_to_int( expl->exp->left->value ), vector_to_int( expl->exp->left->value ) );
	    } else { 
              curr_race = vsignal_set_assigned( sigl->sig, ((sigl->sig->value->width - 1) + sigl->sig->lsb), sigl->sig->lsb );
            }
	    break;
          case EXP_OP_MBIT_SEL :
            if( (expl->exp->left->op == EXP_OP_STATIC) && (expl->exp->right->op == EXP_OP_STATIC) ) {
              curr_race = vsignal_set_assigned( sigl->sig, vector_to_int( expl->exp->left->value ), vector_to_int( expl->exp->right->value ) );
            } else {
              curr_race = vsignal_set_assigned( sigl->sig, ((sigl->sig->value->width - 1) + sigl->sig->lsb), sigl->sig->lsb );
            }
	    break;
          default :
            curr_race = FALSE;
	    break;	
        }
          
        /* Get expression's head statement */
        curr_stmt = race_get_head_statement( mod, expl->exp );

        /* Check to see if the current signal is already being assigned in another statement */
        if( sig_stmt == NULL ) {

          sig_stmt = curr_stmt;

          /* Check to see if current signal is also an input port */ 
          if( (sigl->sig->value->suppl.part.inport == 1) || curr_race ) {
            race_handle_race_condition( curr_stmt, NULL );
  	    race_found = TRUE; 
          }

        } else if( (sig_stmt != curr_stmt) || curr_race ) {

          race_handle_race_condition( curr_stmt, sig_stmt );
	  race_found = TRUE;

        }

      }

      expl = expl->next;

    }

    sigl = sigl->next;

  }

  /* Finally, if we found a race condition and sig_stmt is not NULL, we need to handle this statement */
  if( race_found && (sig_stmt != NULL) ) {
    race_handle_race_condition( sig_stmt, sig_stmt );
  } 

}

/*!
 Performs race checking for the currently loaded module.  This function should be called when
 the endmodule keyword is detected in the current module.
*/
void race_check_modules() {

  stmt_blk*  sb;        /* Pointer to statement block array            */
  int        sb_size;   /* Number of elements in statement block array */
  int        sb_index;  /* Index to statement block array              */
  stmt_iter  si;        /* Statement iterator                          */
  mod_link*  modl;      /* Pointer to current module link              */

  modl = mod_head;

  while( modl != NULL ) {

    /* Clear statement block array size */
    sb_size = 0;

    /* First, get the size of the statement block array */
    stmt_iter_reset( &si, modl->mod->stmt_head );
    while( si.curr != NULL ) {
      if( si.curr->stmt->exp->suppl.part.stmt_head == 1 ) {
        sb_size++;
      }
      stmt_iter_next( &si );
    }

    if( sb_size > 0 ) {

      /* Allocate memory for the statement block array and clear current index */
      sb       = (stmt_blk*)malloc_safe( (sizeof( stmt_blk ) * sb_size), __FILE__, __LINE__ );
      sb_index = 0;

      /* Second, populate the statement block array with pointers to the head statements */
      stmt_iter_reset( &si, modl->mod->stmt_head );
      while( si.curr != NULL ) {
        if( si.curr->stmt->exp->suppl.part.stmt_head == 1 ) {
          sb[sb_index].stmt = si.curr->stmt;
          sb_index++; 
        }
        stmt_iter_next( &si );
      }

      /* Perform other checks here - TBD */

      /* Deallocate stmt_blk list */
      free_safe( sb );

    }

    /* Perform check #6 */
    race_check_one_block_assignment( modl->mod );

    modl = modl->next;

  }

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
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}

/*
 $Log$
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

