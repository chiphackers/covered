#ifndef __ASSERTION_H__
#define __ASSERTION_H__

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


/*
 $Log$
 Revision 1.13.4.3  2008/08/07 06:39:10  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.13.4.2  2008/08/06 20:11:33  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.13.4.1  2008/07/10 22:43:49  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.14  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.13  2008/02/01 06:37:07  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.12  2008/01/16 05:01:22  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.11  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.10  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.9  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.8  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

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

 Revision 1.3  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.2  2006/04/06 22:30:03  phase1geo
 Adding VPI capability back and integrating into autoconf/automake scheme.  We
 are getting close but still have a ways to go.

 Revision 1.1  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

*/

#endif

