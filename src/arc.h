#ifndef __ARC_H__
#define __ARC_H__

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
 \file     arc.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/25/2003
 \brief    Contains functions for handling FSM arc arrays.
*/

#include <stdio.h>

#include "defines.h"


/*!
 \brief Allocates and initializes new state transition array.
*/
fsm_table* arc_create();

/*! \brief Adds new state transition arc entry to specified table. */
void arc_add(
  fsm_table*    table,
  const vector* fr_st,
  const vector* to_st,
  int           hit,
  bool          exclude
);

/*! \brief Finds the specified FROM state in the given FSM table */
int arc_find_from_state(
            const fsm_table* table,
            const vector*    st
);

/*! \brief Finds the specified TO state in the given FSM table */
int arc_find_to_state(
            const fsm_table* table,
            const vector*    st
);

/*! \brief Finds the specified state transition in the given FSM table */
int arc_find_arc(
  const fsm_table* table,
  unsigned int     fr_index,
  unsigned int     to_index
);

/*! \brief Finds the specified state transition in the given FSM table by the exclusion ID */ 
int arc_find_arc_by_exclusion_id(
  const fsm_table* table,
  int              id
);

/*! \brief Calculates all state and state transition values for reporting purposes. */
void arc_get_stats(
            const fsm_table* table,
  /*@out@*/ int*             state_hits,
  /*@out@*/ int*             state_total,
  /*@out@*/ int*             arc_hits,
  /*@out@*/ int*             arc_total,
  /*@out@*/ int*             arc_excluded
);

/*! \brief Writes specified arc array to specified CDD file. */
void arc_db_write(
  const fsm_table* table,
  FILE*            file
);

/*! \brief Reads in arc array from CDD database string. */
void arc_db_read(
  fsm_table** table,
  char**      line
);

/*! \brief Merges contents of arc table from line to specified base array. */
void arc_db_merge(
  fsm_table* table,
  char**     line
);

/*! \brief Merges two FSM arcs, placing the results in the base arc. */
void arc_merge(
  fsm_table*       base,
  const fsm_table* other
);

/*! \brief Stores arc array state values to specified string array. */
void arc_get_states(
  char***          fr_states,
  unsigned int*    fr_state_size,
  char***          to_states,
  unsigned int*    to_state_size,
  const fsm_table* table,
  bool             hit,
  bool             any,
  unsigned int     fr_width,
  unsigned int     to_width
);

/*! \brief Outputs arc array state transition values to specified output stream. */
void arc_get_transitions(
  /*@out@*/ char***          from_states,
  /*@out@*/ char***          to_states,
  /*@out@*/ int**            ids,
  /*@out@*/ int**            excludes,
  /*@out@*/ char***          reasons,
  /*@out@*/ int*             arc_size,
            const fsm_table* table,
            func_unit*       funit,
            bool             hit,
            bool             any,
            unsigned int     fr_width,
            unsigned int     to_width
);

/*! \brief Specifies if any state transitions have been excluded from coverage. */
bool arc_are_any_excluded(
  const fsm_table* table
);

/*! \brief Deallocates memory for specified arcs array. */
void arc_dealloc(
  /*@only@*/ fsm_table* table
);

#endif

