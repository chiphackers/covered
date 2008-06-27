#ifndef __MEMORY_H__
#define __MEMORY_H__

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
  /*@out@*/ unsigned int* ae_total,
  /*@out@*/ unsigned int* wr_hit,
  /*@out@*/ unsigned int* rd_hit,
  /*@out@*/ unsigned int* tog_total,
  /*@out@*/ unsigned int* tog01_hit,
  /*@out@*/ unsigned int* tog10_hit,
            bool          ignore_excl
);

/*! \brief Calculates memory coverage numbers for the specified signal list. */
void memory_get_stats(
            sig_link*     sigl,
  /*@out@*/ unsigned int* ae_total,
  /*@out@*/ unsigned int* wr_hit,
  /*@out@*/ unsigned int* rd_hit,
  /*@out@*/ unsigned int* tog_total,
  /*@out@*/ unsigned int* tog01_hit,
  /*@out@*/ unsigned int* tog10_hit
);

/*! \brief Gets memory summary information for a GUI request */
bool memory_get_funit_summary(
            const char* funit_name,
            int         funit_type,
  /*@out@*/ int*        total,
  /*@out@*/ int*        hit
);

/*! \brief Gets coverage information for the specified memory */
bool memory_get_coverage(
            const char* funit_name,
            int         funit_type,
            const char* signame,
  /*@out@*/ char**      pdim_str,
  /*@out@*/ char**      pdim_array,
  /*@out@*/ char**      udim_str,
  /*@out@*/ char**      memory_info,
  /*@out@*/ int*        excluded
);

/*! \brief Collects all signals that are memories and match the given coverage metric for the given functional unit */
bool memory_collect(
            const char* funit_name,
            int         funit_type,
            int         cov,
  /*@out@*/ sig_link**  head,
  /*@out@*/ sig_link**  tail
);

/*! \brief Generates report output for line coverage. */
void memory_report(
  FILE* ofile,
  bool  verbose
);


/*
 $Log$
 Revision 1.10  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.9  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.8  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.7  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.6  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.5  2006/10/02 22:41:00  phase1geo
 Lots of bug fixes to memory coverage functionality for GUI.  Memory coverage
 should now be working correctly.  We just need to update the GUI documentation
 as well as other images for the new feature add.

 Revision 1.4  2006/09/27 21:38:35  phase1geo
 Adding code to interract with data in memory coverage verbose window.  Majority
 of code is in place; however, this has not been thoroughly debugged at this point.
 Adding mem3 diagnostic for GUI debugging purposes and checkpointing.

 Revision 1.3  2006/09/26 22:36:38  phase1geo
 Adding code for memory coverage to GUI and related files.  Lots of work to go
 here so we are checkpointing for the moment.

 Revision 1.2  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.1  2006/09/25 04:15:04  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

*/

#endif

