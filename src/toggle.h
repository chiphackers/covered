#ifndef __TOGGLE_H__
#define __TOGGLE_H__

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
 \file     toggle.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting toggle coverage.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Calculates the toggle coverage for the specifed expression and signal lists. */
void toggle_get_stats(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit01,
  /*@out@*/ unsigned int* hit10,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total,
  /*@out@*/ bool*         cov_found
);

/*! \brief Collects all toggle expressions that match the specified coverage indication. */
void toggle_collect(
            func_unit*    funit,
            int           cov,
  /*@out@*/ vsignal***    sigs,
  /*@out@*/ unsigned int* sig_size,
  /*@out@*/ unsigned int* size_no_rm_index
);

/*! \brief Gets toggle coverage information for a single signal in the specified functional unit */
void toggle_get_coverage(
            func_unit* funit,
            char*      sig_name,
  /*@out@*/ int*       msb,
  /*@out@*/ int*       lsb,
  /*@out@*/ char**     tog01,
  /*@out@*/ char**     tog10,
  /*@out@*/ int*       excluded,
  /*@out@*/ char**     reason
);

/*! \brief Gets total and hit toggle signal status for the specified functional unit */
void toggle_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gets total and hit toggle signal status for the specified functional unit instance */
void toggle_get_inst_summary(
            funit_inst*   inst,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Generates report output for toggle coverage. */
void toggle_report(
  FILE* ofile,
  bool  verbose
);

#endif

