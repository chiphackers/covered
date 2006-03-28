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

  stmt_iter lsi;                  /* Points to lowest statement iterator */
  int       lowest = 0x7fffffff;  /* Line number of the lowest statement */

  /* If the current statement is not a head, go back to the head */
  if( ESUPPL_IS_STMT_HEAD( si->curr->stmt->exp->suppl ) == 0 ) {
    stmt_iter_reverse( si );
    stmt_iter_find_head( si, FALSE );
  }

  /* Capture the lowest statement iterator */
  lsi.curr = NULL;
  lsi.last = NULL;

  /* Advance to the next statement */
  stmt_iter_next( si );
  
  /* Search for a statement that has not been traversed yet within this statement block */
  while( (si->curr != NULL) && (ESUPPL_IS_STMT_HEAD( si->curr->stmt->exp->suppl ) == 0) ) {
    if( (si->curr->stmt->exp->suppl.part.stmt_added == 0) &&
        (si->curr->stmt->exp->line != 0) &&
        (si->curr->stmt->exp->line < lowest) ) {
      lowest   = si->curr->stmt->exp->line;
      lsi.curr = si->curr;
      lsi.last = si->last;
    }
    stmt_iter_next( si );
  }

  /*
    If we were unable to find an untraversed statement, go to the next statement block,
    resetting the stmt_added supplemental value as we go.
  */
  if( (lsi.curr == NULL) && (lsi.last == NULL) ) {
    stmt_iter_reverse( si );
    while( (si->curr != NULL) && (ESUPPL_IS_STMT_HEAD( si->curr->stmt->exp->suppl ) == 0) ) {
      si->curr->stmt->exp->suppl.part.stmt_added = 0;
      stmt_iter_next( si );
    }
    stmt_iter_find_head( si, TRUE );
  } else {
    si->curr = lsi.curr;
    si->last = lsi.last;
    si->curr->stmt->exp->suppl.part.stmt_added = 1;
  }

}

/*
 $Log$
 Revision 1.11  2006/03/23 22:42:54  phase1geo
 Changed two variable combinational expressions that have a constant value in either the
 left or right expression tree to unary expressions.  Removed expressions containing only
 static values from coverage totals.  Fixed bug in stmt_iter_get_next_in_order for looping
 cases (the verbose output was not being emitted for these cases).  Updated regressions for
 these changes -- full regression passes.

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

