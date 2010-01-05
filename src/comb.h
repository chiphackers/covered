#ifndef __COMB_H__
#define __COMB_H__

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
 \file     comb.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting combinational logic coverage.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Resets combination counted bits in expression tree */
void combination_reset_counted_expr_tree( expression* exp );

/*! \brief Calculates combination logic statistics for a single expression tree */
void combination_get_tree_stats(
            expression*   exp,
            bool          rpt_comb,
            bool          rpt_event,
            int*          ulid,
            unsigned int  curr_depth,
            bool          excluded,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excludes,
  /*@out@*/ unsigned int* total );

/*! \brief Calculates combination logic statistics for summary output */
void combination_get_stats(
            func_unit*    funit,
            bool          rpt_comb,
            bool          rpt_event,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Collects all toggle expressions that match the specified coverage indication. */
void combination_collect(
            func_unit*    funit,
            int           cov,
  /*@out@*/ expression*** exprs,
  /*@out@*/ unsigned int* exp_cnt,
  /*@out@*/ int**         excludes
);

/*! \brief Gets combinational logic summary statistics for specified functional unit */
void combination_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gets combinational logic summary statistics for specified functional unit instance */
void combination_get_inst_summary(
            funit_inst*   inst,  
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total  
);

/*! \brief Gets output for specified expression including underlines and code */
void combination_get_expression(
            int           expr_id,
  /*@out@*/ char***       code,
  /*@out@*/ int**         uline_groups,
  /*@out@*/ unsigned int* code_size,
  /*@out@*/ char***       ulines,
  /*@out@*/ unsigned int* uline_size,
  /*@out@*/ int**         excludes,
  /*@out@*/ char***       reasons,
  /*@out@*/ unsigned int* exclude_size
);

/*! \brief Gets output for specified expression including coverage information */
void combination_get_coverage(
            int        exp_id,
            int        uline_id,
  /*@out@*/ char***    info,
  /*@out@*/ int*       info_size
);

/*! \brief Generates report output for combinational logic coverage. */
void combination_report(
  FILE* ofile,
  bool  verbose
);

#endif

