#ifndef __ITER_H__
#define __ITER_H__

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
 \file     iter.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/24/2002
 \brief    Contains functions for dealing with iterators.
*/

#include "defines.h"


/*! \brief Resets the specified statement iterator at start point. */
void stmt_iter_reset(
  stmt_iter* si,
  stmt_link* start
);

/*! \brief Copies the given statement iterator */
void stmt_iter_copy(
  stmt_iter* si,
  stmt_iter* orig
);

/*! \brief Moves to the next statement link. */
void stmt_iter_next(
  stmt_iter* si
);

/*! \brief Reverses iterator flow and advances to next statement link. */
void stmt_iter_reverse(
  stmt_iter* si
);

/*! \brief Sets specified iterator to start with statement head for ordering. */
void stmt_iter_find_head(
  stmt_iter* si,
  bool       skip
);

/*! \brief Sets current iterator to next statement in order. */
void stmt_iter_get_next_in_order(
  stmt_iter* si
);

/*! \brief Sets current iterator to statement just prior to the given line number */
void stmt_iter_get_line_before(
  stmt_iter* si,
  int        lnum
);

#endif

