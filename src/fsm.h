#ifndef __FSM_H__
#define __FSM_H__

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
 \file     fsm.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting FSM coverage.
*/

#include <stdio.h>

#include "defines.h"

/*! \brief Creates and initializes new FSM structure. */
fsm* fsm_create(
  expression* from_state,
  expression* to_state,
  bool        exclude
);

/*! \brief Adds new FSM arc structure to specified FSMs arc list. */
void fsm_add_arc(
  fsm*        table,
  expression* from_state,
  expression* to_state
);

/*! \brief Sets sizes of tables in specified FSM structure. */
void fsm_create_tables(
  fsm* table
);

/*! \brief Outputs contents of specified FSM to CDD file. */
void fsm_db_write(
  fsm*  table,
  FILE* file,
  bool  ids_issued
);

/*! \brief Reads in contents of specified FSM. */
void fsm_db_read(
             char**     line,
  /*@null@*/ func_unit* funit
);

/*! \brief Reads and merges two FSMs, placing result into base FSM. */
void fsm_db_merge(
  fsm*   base,
  char** line
);

/*! \brief Merges two FSMs, placing the result into the base FSM. */
void fsm_merge(
  fsm* base,
  fsm* other
);

/*! \brief Sets the bit in set table based on the values of last and curr. */
void fsm_table_set(
  expression*     expr,
  const sim_time* time
);

/*! \brief Assigns the given value to the specified FSM and evaluates the FSM for coverage information. */
void fsm_vcd_assign(
  fsm*  table,
  char* value
);

/*! \brief Gathers statistics about the current FSM */
void fsm_get_stats(
            fsm**        table,
            unsigned int table_size,
  /*@out@*/ int*         state_hit,
  /*@out@*/ int*         state_total,
  /*@out@*/ int*         arc_hit,
  /*@out@*/ int*         arc_total,
  /*@out@*/ int*         arc_excluded
);

/*! \brief Retrieves the FSM summary information for the specified functional unit. */
void fsm_get_funit_summary(
            func_unit* funit,
  /*@out@*/ int*       hit,
  /*@out@*/ int*       excluded,
  /*@out@*/ int*       total
);

/*! \brief Retrieves the FSM summary information for the specified functional unit. */
void fsm_get_inst_summary(
            funit_inst* inst,
  /*@out@*/ int*        hit,
  /*@out@*/ int*        excluded,
  /*@out@*/ int*        total
);

/*! \brief Retrieves covered or uncovered FSMs from the specified functional unit. */
void fsm_collect(
            func_unit*    funit,
            int           cov,
  /*@out@*/ vsignal***    sigs,
  /*@out@*/ unsigned int* sig_size,
  /*@out@*/ unsigned int* sig_no_rm_index,
  /*@out@*/ int**         expr_ids,
  /*@out@*/ int**         excludes
);

/*! \brief Collects all coverage information for the specified FSM */
void fsm_get_coverage(
            func_unit*    funit,
            int           expr_id,
  /*@out@*/ char***       total_fr_states,
  /*@out@*/ unsigned int* total_fr_state_num,
  /*@out@*/ char***       total_to_states,
  /*@out@*/ unsigned int* total_to_state_num,
  /*@out@*/ char***       hit_fr_states,
  /*@out@*/ unsigned int* hit_fr_state_num,
  /*@out@*/ char***       hit_to_states,
  /*@out@*/ unsigned int* hit_to_state_num,
  /*@out@*/ char***       total_from_arcs,
  /*@out@*/ char***       total_to_arcs,
  /*@out@*/ int**         total_ids,
  /*@out@*/ int**         excludes,
  /*@out@*/ char***       reasons,
  /*@out@*/ int*          total_arc_num,
  /*@out@*/ char***       hit_from_arcs,
  /*@out@*/ char***       hit_to_arcs,
  /*@out@*/ int*          hit_arc_num,
  /*@out@*/ char***       input_state,
  /*@out@*/ unsigned int* input_size,
  /*@out@*/ char***       output_state,
  /*@out@*/ unsigned int* output_size
);

/*! \brief Generates report output for FSM coverage. */
void fsm_report(
  FILE* ofile,
  bool verbose
);

/*! \brief Deallocates specified FSM structure. */
void fsm_dealloc(
  fsm* table
);

#endif

