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
  si->curr = (stmt_link*)((int)si->curr->ptr ^ (int)si->last);
  si->last = tmp;
  
}


/* $Log$ */

