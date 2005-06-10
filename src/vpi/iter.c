/*!
 \file     iter.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/24/2002
*/

#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "iter.h"


/*!
 \param si     Pointer to statement iterator to reset.
 \param start  Specifies link to start iterating through (can be head or tail).
 
 Initializes the specified statement iterator to begin advancing.
*/
void stmt_iter_reset( stmt_iter* si, stmt_link* start ) {
  
  si->curr = start;
  si->last = NULL;
  
}

/*!
 \param si  Pointer to statement iterator to advance.
 
 \return Returns pointer to next statement link.
 
 Advances specified statement iterator to next statement
 link in statement list.
*/
void stmt_iter_next( stmt_iter* si ) {
  
  stmt_link* tmp;    /* Temporary holder for current statement link */
  
  tmp      = si->curr;
  si->curr = (stmt_link*)((long int)si->curr->ptr ^ (long int)si->last);
  si->last = tmp;
  
}

/*!
 \param si  Pointer to statement iterator to reverse.
 
 Reverses the direction of the iterator and changes the current pointer
 to point to the last statement before the reverse.
*/
void stmt_iter_reverse( stmt_iter* si ) {
  
  stmt_link* tmp;
  
  tmp      = si->curr;
  si->curr = si->last;
  si->last = tmp;
  
}

/*!
 \param si    Pointer to statement iterator to transform.
 \param skip  Specifies if we should skip to the next statement header.
 
 Iterates down statement list until a statement head is reached.  If a
 statement head is found, the iterator is reversed (with curr pointing to
 statement head).  Used for displaying statements in line order for reports.
*/
void stmt_iter_find_head( stmt_iter* si, bool skip ) {
  
  while( (si->curr != NULL) && ((ESUPPL_IS_STMT_HEAD( si->curr->stmt->exp->suppl ) == 0) || skip) ) {
    if( ESUPPL_IS_STMT_HEAD( si->curr->stmt->exp->suppl ) == 1 ) {
      skip = FALSE;
    }
    stmt_iter_next( si );
  }
  
  if( si->curr != NULL ) {
    stmt_iter_next( si );
    stmt_iter_reverse( si );
  }
  
}

/*!
 \param si  Pointer to statement iterator to transform.
 
 Iterates to next statement in list and compares this statement ID with
 the previous statement ID.  If the new statement ID is less than the previous
 statement ID, we need to reverse the iterator, find the second statement head
 and reverse the iterator again.  This function is used to order statements by
 line number in verbose reports.
*/
void stmt_iter_get_next_in_order( stmt_iter* si ) {

  stmt_iter_next( si );
  
  if( (si->curr == NULL) || (si->curr->stmt->exp->line < si->last->stmt->exp->line) || (si->curr->stmt->exp->line == 0) ) {
    stmt_iter_reverse( si );
    stmt_iter_find_head( si, TRUE );
  }

}

/*
 $Log$
 Revision 1.10  2005/01/24 13:21:44  phase1geo
 Modifying unlinking algorithm for statement links.  Still getting
 segmentation fault at this time.

 Revision 1.9  2005/01/11 14:24:16  phase1geo
 Intermediate checkin.

 Revision 1.8  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.7  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.6  2003/10/13 03:56:29  phase1geo
 Fixing some problems with new FSM code.  Not quite there yet.

 Revision 1.5  2003/01/27 16:06:10  phase1geo
 Fixing bug with line ordering where case statement lines were not being
 output to reports.  Updating regression to reflect fixes.

 Revision 1.4  2002/12/07 17:46:53  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.3  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.2  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.1  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.
*/

