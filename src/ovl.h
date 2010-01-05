#ifndef __OVL_H__
#define __OVL_H__

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
 \file     ovl.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     04/13/2006
 \brief    Contains functions for handling OVL assertion extraction.
*/


#include "defines.h"


/*! \brief Returns TRUE if specified functional unit is an OVL assertion module. */
bool ovl_is_assertion_module( const func_unit* funit );

/*! \brief Returns TRUE if specified expression corresponds to a functional coverage point. */
bool ovl_is_coverage_point( const expression* exp );

/*! \brief Adds all assertion modules to no score list */
void ovl_add_assertions_to_no_score_list( bool rm_tasks );

/*! \brief Gathers the OVL assertion coverage summary statistics for the given functional unit. */
void ovl_get_funit_stats(
            const func_unit* funit,
  /*@out@*/ unsigned int*    hit,
  /*@out@*/ unsigned int*    excludes,
  /*@out@*/ unsigned int*    total
);

/*! \brief Displays the verbose hit/miss information to the given output file for the given functional unit. */
void ovl_display_verbose(
  FILE*            ofile,
  const func_unit* funit,
  rpt_type         rtype
);

/*! \brief Finds the instance names of all uncovered and covered assertions in the specified functional unit. */
void ovl_collect(
                 func_unit*    funit,
                 int           cov,
  /*@null out@*/ char***       inst_names,
  /*@out@*/      int**         excludes,
  /*@out@*/      unsigned int* inst_size
);

/*! \brief Gets missed coverage points for the given assertion */
void ovl_get_coverage(
            const func_unit* funit,
            const char*      inst_name,
  /*@out@*/ char**           assert_mod,
  /*@out@*/ str_link**       cp_head,
  /*@out@*/ str_link**       cp_tail
);

#endif

