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
 \file     perf.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/1/2006
*/

#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "perf.h"
#include "util.h"
#include "expr.h"


extern funit_link* funit_head;
extern inst_link*  inst_head;


/*!
 \param funit  Pointer to functional unit to generate performance statistics for

 \return Returns a pointer to the perf_stat structure containing the performance statistics
         for the given functional unit

 Allocates and initializes the contents of a performance structure to display the total number
 of expressions in the given functional unit and the number of times these expressions that
 were executed during simulation.
*/
perf_stat* perf_gen_stats( func_unit* funit ) { PROFILE(PERF_GEN_STATS);

  exp_link*  expl;   /* Pointer to current expression link */
  perf_stat* pstat;  /* Pointer to newly created performance stat structure */
  int        i;      /* Loop iterator */

  /* Create and initialize new perf_stat structure */
  pstat = (perf_stat*)malloc_safe( sizeof( perf_stat ) );
  for( i=0; i<EXP_OP_NUM; i++ ) {
    pstat->op_exec_cnt[i] = 0;
    pstat->op_cnt[i]      = 0;
  }

  /* Populate performance structure */
  expl = funit->exp_head;
  while( expl != NULL ) {
    pstat->op_cnt[expl->exp->op]      += 1;
    pstat->op_exec_cnt[expl->exp->op] += expl->exp->exec_num;
    expl = expl->next;
  }

  return( pstat );

}

/*!
 \param ofile  Pointer to file to output performance results to
 \param funit  Pointer to functional unit to output performance metrics for

 Outputs the expression performance statistics to the given output stream.
*/
void perf_output_mod_stats( FILE* ofile, func_unit* funit ) { PROFILE(PERF_OUTPUT_MOD_STATS);

  perf_stat* pstat;     /* Pointer to performance statistic structure for this funit */
  int        i;         /* Loop iterator */
  float      avg_exec;  /* Calculated average execution count for the current expression op */

  /* Get performance structure */
  pstat = perf_gen_stats( funit );

  /* Output the structure */
  fprintf( ofile, "  %s: %s\n", get_funit_type( funit->type ), funit->name );
  fprintf( ofile, "    ExpOp      Cnt      / Executed / Avg. Executed\n" );

  for( i=0; i<EXP_OP_NUM; i++ ) {
    if( pstat->op_exec_cnt[i] > 0 ) {
      avg_exec = pstat->op_exec_cnt[i] / pstat->op_cnt[i]; 
      fprintf( ofile, "    %-10.10s %-8.0f   %-8u   %-8.1f\n",
               expression_string_op( i ), pstat->op_cnt[i], pstat->op_exec_cnt[i], avg_exec );
    }
  }

  fprintf( ofile, "\n" );

}

/*!
 \param ofile  File to output report information to.
 \param root   Pointer to current functional unit instance to output performance stats for

 Called by the perf_output_inst_report function to output a performance report on an
 instance basis.
*/
void perf_output_inst_report_helper( FILE* ofile, funit_inst* root ) { PROFILE(PERF_OUTPUT_INST_REPORT_HELPER);

  funit_inst* curr;  /* Pointer to current child instance to output */

  if( root != NULL ) {

    perf_output_mod_stats( ofile, root->funit );

    curr = root->child_head;
    while( curr != NULL ) {
      perf_output_inst_report_helper( ofile, curr );
      curr = curr->next;
    }

  }

}

/*!
 \param ofile  File to output report information to.

 Generates a performance report on an instance basis to the specified output file.
*/
void perf_output_inst_report( FILE* ofile ) { PROFILE(PERF_OUTPUT_INST_REPORT);

  inst_link* instl;  /* Pointer to current instance link */

  fprintf( ofile, "\nSIMULATION PERFORMANCE STATISTICS:\n\n" );

  instl = inst_head;
  while( instl != NULL ) {
    perf_output_inst_report_helper( ofile, instl->inst );
    instl = instl->next;
  }

}

/*
 $Log$
 Revision 1.7  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.6  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.5  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.4  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.3  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.2  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.1  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

*/
