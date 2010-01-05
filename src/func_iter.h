#ifndef __FUNC_ITER_H__
#define __FUNC_ITER_H__

/*
 Copyright (c) 2006-2010 Trevor Williams

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
  unsigned int  scopes;    /*!< The number of scopes iterated with this iteration (i.e., the allocated size of sls and sigs) */
  stmt_link**   sls;       /*!< Pointer to array of statement links (sorted by line number) for the given functional unit */
  unsigned int  sl_num;    /*!< Specifies the current index in the sls array to process */
  vsignal***    sigs;      /*!< Pointer to array of signal lists for the given functional unit */
  unsigned int* sig_size;  /*!< Array of sigs array sizes stored in sigs */
  unsigned int  sig_num;   /*!< Specifies the current index in the sigs array to process */
  unsigned int  curr_sig;  /*!< Index of current signal in the current sigs array element */
  func_unit*    funit;     /*!< Pointer to functional unit that this iterator represents */
} func_iter;


/*! \brief Initializes the values in the given structure */
void func_iter_init(
  /*@out@*/ func_iter* fi,
            func_unit* funit,
            bool       stmts,
            bool       sigs,
            bool       inc_all
);

/*! \brief Provides the next statement iterator in the functional unit statement iterator */
statement* func_iter_get_next_statement(
  func_iter* fi
);

/*! \brief Provides the next signal in the functional unit signal iterator */
vsignal* func_iter_get_next_signal(
  func_iter* fi
);

/*! \brief Deallocates functional unit iterator */
void func_iter_dealloc(
  func_iter* si
);

#endif

