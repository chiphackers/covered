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


/*
 $Log$
 Revision 1.2  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.1  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.
*/

