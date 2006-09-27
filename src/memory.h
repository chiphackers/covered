#ifndef __MEMORY_H__
#define __MEMORY_H__

/*!
 \file     memory.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/24/2006
 \brief    Contains functions for generating memory coverage reports
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Calculates the memory coverage numbers for a given memory signal */
void memory_get_stat( vsignal* sig, float* ae_total, int* wr_hit, int* rd_hit, float* tog_total, int* tog01_hit, int* tog10_hit );

/*! \brief Calculates memory coverage numbers for the specified signal list. */
void memory_get_stats( sig_link* sigl, float* ae_total, int* wr_hit, int* rd_hit, float* tog_total, int* tog01_hit, int* tog10_hit );

/*! \brief Gets memory summary information for a GUI request */
bool memory_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit );

/*! \brief Gets coverage information for the specified memory */
bool memory_get_coverage( char* funit_name, int funit_type, char* signame,
                          char** pdim_str, char** pdim_array, char** udim_str, char** memory_info, int* excluded );

/*! \brief Collects all signals that are memories and match the given coverage metric for the given functional unit */
bool memory_collect( char* funit_name, int funit_type, int cov, sig_link** head, sig_link** tail );

/*! \brief Generates report output for line coverage. */
void memory_report( FILE* ofile, bool verbose );


/*
 $Log$
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

