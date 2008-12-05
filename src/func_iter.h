#ifndef __FUNC_ITER_H__
#define __FUNC_ITER_H__

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
 \file     func_iter.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/2/2007
 \brief    Contains functions for dealing with functional unit iterators.
*/

#include "defines.h"


/*!
 Structure for iterating through a functional unit and its unnamed scopes.
*/
typedef struct func_iter_s {
  unsigned int scopes;     /*!< The number of scopes iterated with this iteration (i.e., the allocated size of sis and sigs) */
  stmt_iter**  sis;        /*!< Pointer to array of statement iterators (sorted by line number) for the given functional unit */
  unsigned int si_num;     /*!< Specifies the current index in the sis array to process */
  sig_link**   sigs;       /*!< Pointer to array of signal lists for the given functional unit */
  unsigned int sig_num;    /*!< Specifies the current index in the sigs array to process */
  sig_link*    curr_sigl;  /*!< Pointer to current sig_link element in the given sigs array element */
} func_iter;


/*! \brief Initializes the values in the given structure */
void func_iter_init(
  /*@out@*/ func_iter* fi,
            func_unit* funit,
            bool       stmts,
            bool       sigs,
            bool       use_tail,
            bool       inc_all
);

/*! \brief Resets the functional iterator structure (call after initializing) */
void func_iter_reset(
  func_iter* fi  /*!< Pointer to functional unit to reset */
);

/*! \brief Provides the next statement iterator in the functional unit statement iterator */
statement* func_iter_get_next_statement( func_iter* fi );

/*! \brief Provides the next signal in the functional unit signal iterator */
vsignal* func_iter_get_next_signal( func_iter* fi );

/*! \brief Deallocates functional unit iterator */
void func_iter_dealloc( func_iter* si );


/*
 $Log$
 Revision 1.6  2008/12/04 14:19:50  phase1geo
 Fixing bug in code generator.  Checkpointing.

 Revision 1.5  2008/11/27 00:24:44  phase1geo
 Fixing problems with previous version of generator.  Things work as expected at this point.
 Checkpointing.

 Revision 1.4  2008/08/22 20:56:35  phase1geo
 Starting to make updates for proper unnamed scope report handling (fix for bug 2054686).
 Not complete yet.  Also making updates to documentation.  Checkpointing.

 Revision 1.3  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.2  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.1  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

*/

#endif

