#ifndef __OVL_H__
#define __OVL_H__

/*
 Copyright (c) 2006 Trevor Williams

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
  /*@out@*/ unsigned int*    total,
  /*@out@*/ unsigned int*    hit );

/*! \brief Displays the verbose hit/miss information to the given output file for the given functional unit. */
void ovl_display_verbose(
  FILE*            ofile,
  const func_unit* funit
);

/*! \brief Finds the instance names of all uncovered and covered assertions in the specified functional unit. */
void ovl_collect(
                 func_unit*    funit,
  /*@null out@*/ char***       uncov_inst_names,
  /*@out@*/      int**         excludes,
  /*@out@*/      unsigned int* uncov_inst_size,
  /*@out@*/      char***       cov_inst_names,
  /*@out@*/      unsigned int* cov_inst_size
);

/*! \brief Gets missed coverage points for the given assertion */
void ovl_get_coverage(
            const func_unit* funit,
            const char*      inst_name,
  /*@out@*/ char**           assert_mod,
  /*@out@*/ str_link**       cp_head,
  /*@out@*/ str_link**       cp_tail
);

/*
 $Log$
 Revision 1.14  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.13  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.12  2008/01/08 13:27:46  phase1geo
 More splint updates.

 Revision 1.11  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.10  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.9  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.8  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.7  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.6  2006/05/01 13:19:07  phase1geo
 Enhancing the verbose assertion window.  Still need to fix a few bugs and add
 a few more enhancements.

 Revision 1.5  2006/04/29 05:12:14  phase1geo
 Adding initial version of assertion verbose window.  This is currently working; however,
 I think that I want to enhance this window a bit more before calling it good.

 Revision 1.4  2006/04/28 17:10:19  phase1geo
 Adding GUI support for assertion coverage.  Halfway there.

 Revision 1.3  2006/04/21 22:03:58  phase1geo
 Adding ovl1 and ovl1.1 diagnostics to testsuite.  ovl1 passes while ovl1.1
 currently fails due to a problem with outputting parameters to the CDD file
 (need to look into this further).  Added OVL assertion verbose output support
 which seems to be working for the moment.

 Revision 1.2  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.1  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

*/

#endif

