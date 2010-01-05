#ifndef __LINE_H__
#define __LINE_H__

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
 \file     line.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting line coverage.
*/

#include <stdio.h>

#include "defines.h"

/*! \brief Calculates line coverage numbers for the specified expression list. */
void line_get_stats(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gathers line numbers from specified functional unit that were not hit during simulation. */
void line_collect(
            func_unit* funit,
            int        cov,
  /*@out@*/ int**      lines,
  /*@out@*/ int**      excludes,
  /*@out@*/ char***    reasons,
  /*@out@*/ int*       line_cnt,
  /*@out@*/ int*       line_size
);

/*! \brief Returns hit and total information for specified functional unit. */
void line_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Returns hit and total information for specified functional unit instance */
void line_get_inst_summary(
            funit_inst*   inst,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Generates report output for line coverage. */
void line_report(
  FILE* ofile,
  bool  verbose
);

#endif

