#ifndef __ITER_H__
#define __ITER_H__

/*!
 \file     iter.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/24/2002
 \brief    Contains functions for dealing with iterators.
*/

#include "defines.h"


/*! Resets the specified statement iterator at start point. */
void stmt_iter_reset( stmt_iter* si, stmt_link* start );

/*! Moves to the next statement link. */
void stmt_iter_next( stmt_iter* si );


/*
 $Log$
 Revision 1.2  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.1  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.
*/

#endif

