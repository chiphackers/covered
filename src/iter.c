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
 \file     iter.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/24/2002
*/

#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "iter.h"
#include "profiler.h"


/*!
 Initializes the specified statement iterator to begin advancing.
*/
void stmt_iter_reset(
  stmt_iter* si,    /*!< Pointer to statement iterator to reset */
  stmt_link* start  /*!< Specifies link to start iterating through (can be head or tail) */
) { PROFILE(STMT_ITER_RESET);
  
  si->curr = start;
  si->last = NULL;

  PROFILE_END;
  
}

/*!
 Copies the given statement iterator to the given iterator.
*/
void stmt_iter_copy(
  stmt_iter* si,   /*!< Pointer to statement iterator containing the copied information */
  stmt_iter* orig  /*!< Pointer to original statementer iterator being copied */
) { PROFILE(STMT_ITER_COPY);

  si->curr = orig->curr;
  si->last = orig->last;

  PROFILE_END;

}

/*!
 \return Returns pointer to next statement link.
 
 Advances specified statement iterator to next statement
 link in statement list.
*/
void stmt_iter_next(
  stmt_iter* si  /*!< Pointer to statement iterator to advance */
) { PROFILE(STMT_ITER_NEXT);
  
  stmt_link* tmp;    /* Temporary holder for current statement link */
  
  tmp      = si->curr;
  si->curr = (stmt_link*)((long int)si->curr->ptr ^ (long int)si->last);
  si->last = tmp;

  PROFILE_END;
  
}

/*!
 Reverses the direction of the iterator and changes the current pointer
 to point to the last statement before the reverse.
*/
void stmt_iter_reverse(
  stmt_iter* si  /*!< Pointer to statement iterator to reverse */
) { PROFILE(STMT_ITER_REVERSE);
  
  stmt_link* tmp;
  
  tmp      = si->curr;
  si->curr = si->last;
  si->last = tmp;

  PROFILE_END;
  
}

/*!
 Iterates down statement list until a statement head is reached.  If a
 statement head is found, the iterator is reversed (with curr pointing to
 statement head).  Used for displaying statements in line order for reports.
*/
void stmt_iter_find_head(
  stmt_iter* si,   /*!< Pointer to statement iterator to transform */
  bool       skip  /*!< Specifies if we should skip to the next statement header */
) { PROFILE(STMT_ITER_FIND_HEAD);
  
  while( (si->curr != NULL) && ((si->curr->stmt->suppl.part.head == 0) || skip) ) {
    if( si->curr->stmt->suppl.part.head == 1 ) {
      skip = FALSE;
    }
    stmt_iter_next( si );
  }
  
  if( si->curr != NULL ) {
    stmt_iter_next( si );
    stmt_iter_reverse( si );
  }

  PROFILE_END;
  
}

/*!
 Iterates to next statement in list and compares this statement ID with
 the previous statement ID.  If the new statement ID is less than the previous
 statement ID, we need to reverse the iterator, find the second statement head
 and reverse the iterator again.  This function is used to order statements by
 line number in verbose reports.
*/
void stmt_iter_get_next_in_order(
  stmt_iter* si  /*!< Pointer to statement iterator to transform */
) { PROFILE(STMT_ITER_GET_NEXT_IN_ORDER);

  stmt_iter lsi;                    /* Points to lowest statement iterator */
  int       low_line = 0x7fffffff;  /* Line number of the lowest statement */
  int       low_col  = 0x7fffffff;  /* Column number of the lowest statement */

  /* If the current statement is not a head, go back to the head */
  if( si->curr->stmt->suppl.part.head == 0 ) {
    stmt_iter_reverse( si );
    stmt_iter_find_head( si, FALSE );
  }

  /* Capture the lowest statement iterator */
  lsi.curr = NULL;
  lsi.last = NULL;

  /* Advance to the next statement */
  stmt_iter_next( si );
  
  /* Search for a statement that has not been traversed yet within this statement block */
  while( (si->curr != NULL) && (si->curr->stmt->suppl.part.head == 0) ) {
    if( (si->curr->stmt->suppl.part.added == 0) &&
        (si->curr->stmt->exp->ppline != 0) &&
        ((si->curr->stmt->exp->ppline < low_line) ||
         ((si->curr->stmt->exp->ppline == low_line) && (((si->curr->stmt->exp->col >> 16) & 0xffff) < low_col))) ) {
      low_line = si->curr->stmt->exp->ppline;
      low_col  = ((si->curr->stmt->exp->col >> 16) & 0xffff);
      lsi.curr = si->curr;
      lsi.last = si->last;
    }
    stmt_iter_next( si );
  }

  /*
    If we were unable to find an untraversed statement, go to the next statement block,
    resetting the added supplemental value as we go.
  */
  if( (lsi.curr == NULL) && (lsi.last == NULL) ) {
    stmt_iter_reverse( si );
    while( (si->curr != NULL) && (si->curr->stmt->suppl.part.head == 0) ) {
      si->curr->stmt->suppl.part.added = 0;
      stmt_iter_next( si );
    }
    stmt_iter_find_head( si, TRUE );
  } else {
    si->curr = lsi.curr;
    si->last = lsi.last;
    si->curr->stmt->suppl.part.added = 1;
  }

  PROFILE_END;

}

/*!
 Searches the given statement iterator for the statement whose line number comes just
 before the given line number.  Places curr on this statement and places last on the
 statement whose line number is either equal to or less than curr.
*/
void stmt_iter_get_line_before(
  stmt_iter* si,   /*!< Pointer to statement iterator to search */
  int        lnum  /*!< Line number to compare against */
) { PROFILE(STMT_ITER_GET_LINE_BEFORE);

  if( si->curr != NULL ) {

    if( si->curr->stmt->exp->ppline < lnum ) {
      while( (si->curr != NULL) && (si->curr->stmt->exp->ppline < lnum) ) {
        stmt_iter_next( si );
      }
    } else {
      while( (si->curr != NULL) && (si->curr->stmt->exp->ppline > lnum) ) {
        stmt_iter_next( si );
      }
    }

  }

  PROFILE_END;

}

