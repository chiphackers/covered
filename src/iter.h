#ifndef __ITER_H__
#define __ITER_H__

/*!
 \file     iter.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/24/2002
 \brief    Contains functions for dealing with iterators.
*/

#include "defines.h"


//! Resets the specified statement iterator at start point.
void stmt_iter_reset( stmt_iter* si, stmt_link* start );

//! Moves to the next statement link.
void stmt_iter_next( stmt_iter* si );


/* $Log$ */

#endif

