#ifndef __EXCLUDE_H__
#define __EXCLUDE_H__

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
 \file     exclude.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/22/2006
 \brief    Contains functions for handling user-specified exclusion of coverage results.
*/

#include "defines.h"


/*! \brief Returns TRUE if the specified line is excluded in the given functional unit. */
bool exclude_is_line_excluded(
  func_unit* funit,
  int        line
);

/*! \brief Sets the excluded bit for all expressions in the given functional unit with the
           specified line number and recalculates the summary coverage information. */
void exclude_set_line_exclude(
            func_unit* funit,
            int        line,
            int        value,
            char*      reason,
            bool       rpt_comb,
            bool       rpt_event,
  /*@out@*/ statistic* stat
);

/*! \brief Returns TRUE if the specified signal is excluded in the given functional unit. */
bool exclude_is_toggle_excluded(
  func_unit* funit,
  char*      sig_name
);

/*! \brief Sets the excluded bit for the specified signal in the given functional unit and
           recalculates the summary coverage information. */
void exclude_set_toggle_exclude(
            func_unit*  funit,
            const char* sig_name,
            int         value,
            char        type,
            char*       reason,
  /*@out@*/ statistic*  stat
);

/*! \return Returns TRUE if the specified expression is excluded in the given functional unit. */
bool exclude_is_comb_excluded(
  func_unit* funit,
  int        expr_id,
  int        uline_id
);

/*! \brief Sets the excluded bit for the specified expression in the given functional unit
           and recalculates the summary coverage information. */
void exclude_set_comb_exclude(
            func_unit* funit,
            int        expr_id,
            int        uline_id,
            int        value,
            char*      reason,
            bool       rpt_comb,
            bool       rpt_event,
  /*@out@*/ statistic* stat
);

/*! \brief Returns TRUE if the specified FSM is excluded in the given functional unit. */
bool exclude_is_fsm_excluded(
  func_unit* funit,
  int        expr_id,
  char*      from_state,
  char*      to_state
);

/*! \brief Sets the excluded bit for the specified state transition in the given functional unit
           and recalculates the summary coverage information. */
void exclude_set_fsm_exclude(
            func_unit* funit,
            int        expr_id,
            char*      from_state,
            char*      to_state,
            int        value,
            char*      reason,
  /*@out@*/ statistic* stat
);

/*! \brief Returns TRUE if given assertion is excluded from coverage. */
bool exclude_is_assert_excluded(
  func_unit* funit,
  char*      inst_name,
  int        expr_id
);

/*! \brief Sets the excluded bit for the specified expression in the given functional unit
           and recalculates the summary coverage information. */
void exclude_set_assert_exclude(
            func_unit* funit,
            char*      inst_name,
            int        expr_id,
            int        value,
            char*      reason,
            bool       rpt_comb,
            bool       rpt_event,
  /*@out@*/ statistic* stat
);

/*! \brief Returns a pointer to the found exclude reason if one is found for the given type and ID; otherwise,
           returns NULL. */
exclude_reason* exclude_find_exclude_reason(
  char       type,
  int        id,
  func_unit* funit
);

/*! \brief Formats the reason string for storage purposes. */
char* exclude_format_reason(
  const char* old_str
);

/*! \brief Outputs the given exclude reason structure to the specified file stream. */
void exclude_db_write(
  exclude_reason* er,
  FILE*           ofile
);

/*! \brief Reads the given exclude reason from the specified line and stores its information in the curr_funit
           structure. */
void exclude_db_read(
  char**     line,
  func_unit* curr_funit
);

/*! \brief Reads the given exclude reason from the specified line and merges its information with the base functional unit. */
void exclude_db_merge(
  func_unit* base,
  char**     line
);

/*! \brief Performs exclusion reason merging. */
void exclude_merge(
  func_unit*      base,
  exclude_reason* er
);

/*! \brief Allows the user to exclude coverage points from reporting. */
void command_exclude(
  int          argc,
  int          last_arg,
  const char** argv
);

#endif

