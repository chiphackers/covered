#ifndef __ASSERTION_H__
#define __ASSERTION_H__

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
 \file     assertion.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/4/2006
 \brief    Contains functions for handling assertion coverage.
*/


#include <stdio.h>

#include "defines.h"


/*! \brief Parses -A command-line option to score command */
void assertion_parse( const char* arg );

/*! \brief Parses an in-line attribute for assertion coverage information */
void assertion_parse_attr(
  attr_param*      ap,
  const func_unit* funit,
  bool             exclude
);

/*! \brief Gather statistics for assertion coverage */
void assertion_get_stats(
            const func_unit* funit,
  /*@out@*/ unsigned int*    hit,
  /*@out@*/ unsigned int*    excluded,
  /*@out@*/ unsigned int*    total
);

/*! \brief Generates report output for assertion coverage */
void assertion_report( FILE* ofile, bool verbose );

/*! \brief Retrieves the total and hit counts of assertions for the specified functional unit */
void assertion_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Collects uncovered and covered assertion instance names for the given module */
void assertion_collect(
            func_unit*    funit,
            int           cov,
  /*@out@*/ char***       inst_names,
  /*@out@*/ int**         excludes,
  /*@out@*/ unsigned int* inst_size
);

/*! \brief Gets missed coverage point descriptions for the given assertion module */
void assertion_get_coverage(
            const func_unit* funit,
            const char*      inst_name,
  /*@out@*/ char**           assert_mod,
  /*@out@*/ str_link**       cp_head,
  /*@out@*/ str_link**       cp_tail
);

#endif

