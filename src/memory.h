#ifndef __MEMORY_H__
#define __MEMORY_H__

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
 \file     memory.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     9/24/2006
 \brief    Contains functions for generating memory coverage reports
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Calculates the memory coverage numbers for a given memory signal */
void memory_get_stat(
            vsignal*      sig,
  /*@out@*/ unsigned int* wr_hit,
  /*@out@*/ unsigned int* rd_hit,
  /*@out@*/ unsigned int* ae_total,
  /*@out@*/ unsigned int* tog01_hit,
  /*@out@*/ unsigned int* tog10_hit,
  /*@out@*/ unsigned int* tog_total,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ bool*         cov_found,
            bool          ignore_excl
);

/*! \brief Calculates memory coverage numbers for the specified signal list. */
void memory_get_stats(
            func_unit*    funit,
  /*@out@*/ unsigned int* wr_hit,
  /*@out@*/ unsigned int* rd_hit,
  /*@out@*/ unsigned int* ae_total,
  /*@out@*/ unsigned int* tog01_hit,
  /*@out@*/ unsigned int* tog10_hit,
  /*@out@*/ unsigned int* tog_total,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ bool*         cov_found
);

/*! \brief Gets memory summary information for a GUI request */
void memory_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gets memory summary information for a GUI request */
void memory_get_inst_summary(
            funit_inst*   funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gets coverage information for the specified memory */
void memory_get_coverage(
            func_unit*  funit,
            const char* signame,
  /*@out@*/ char**      pdim_str,
  /*@out@*/ char**      pdim_array,
  /*@out@*/ char**      udim_str,
  /*@out@*/ char**      memory_info,
  /*@out@*/ int*        excluded,
  /*@out@*/ char**      reason
);

/*! \brief Collects all signals that are memories and match the given coverage metric for the given functional unit */
void memory_collect(
            func_unit*    funit,
            int           cov,
  /*@out@*/ vsignal***    sigs,
  /*@out@*/ unsigned int* sig_size,
  /*@out@*/ unsigned int* sig_no_rm_index
);

/*! \brief Generates report output for line coverage. */
void memory_report(
  FILE* ofile,
  bool  verbose
);

#endif

