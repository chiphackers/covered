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


extern db**         db_list;
extern unsigned int curr_db;


/*!
 \return Returns a pointer to the perf_stat structure containing the performance statistics
         for the given functional unit

 Allocates and initializes the contents of a performance structure to display the total number
 of expressions in the given functional unit and the number of times these expressions that
 were executed during simulation.
*/
static perf_stat* perf_gen_stats(
  func_unit* funit  /*!< Pointer to functional unit to generate performance statistics for */
) { PROFILE(PERF_GEN_STATS);

  perf_stat*   pstat;  /* Pointer to newly created performance stat structure */
  unsigned int i;

  /* Create and initialize new perf_stat structure */
  pstat = (perf_stat*)malloc_safe( sizeof( perf_stat ) );
  for( i=0; i<EXP_OP_NUM; i++ ) {
    pstat->op_exec_cnt[i] = 0;
    pstat->op_cnt[i]      = 0;
  }

  /* Populate performance structure */
  for( i=0; i<funit->exp_size; i++ ) {
    pstat->op_cnt[funit->exps[i]->op]      += 1;
    pstat->op_exec_cnt[funit->exps[i]->op] += funit->exps[i]->exec_num;
  }

  PROFILE_END;

  return( pstat );

}

/*!
 Outputs the expression performance statistics to the given output stream.
*/
static void perf_output_mod_stats(
  FILE*      ofile,  /*!< Pointer to file to output performance results to */
  func_unit* funit   /*!< Pointer to functional unit to output performance metrics for */
) { PROFILE(PERF_OUTPUT_MOD_STATS);

  perf_stat* pstat;     /* Pointer to performance statistic structure for this funit */
  int        i;         /* Loop iterator */
  float      avg_exec;  /* Calculated average execution count for the current expression op */

  /* Get performance structure */
  pstat = perf_gen_stats( funit );

  /* Output the structure */
  fprintf( ofile, "  %s: %s\n", get_funit_type( funit->suppl.part.type ), funit->name );
  fprintf( ofile, "    ExpOp      Cnt      / Executed / Avg. Executed\n" );

  for( i=0; i<EXP_OP_NUM; i++ ) {
    if( pstat->op_exec_cnt[i] > 0 ) {
      avg_exec = pstat->op_exec_cnt[i] / pstat->op_cnt[i]; 
      fprintf( ofile, "    %-10.10s %-8.0f   %-8u   %-8.1f\n",
               expression_string_op( i ), pstat->op_cnt[i], pstat->op_exec_cnt[i], avg_exec );
    }
  }

  fprintf( ofile, "\n" );

  PROFILE_END;

}

/*!
 Called by the perf_output_inst_report function to output a performance report on an
 instance basis.
*/
static void perf_output_inst_report_helper(
  FILE*       ofile,  /*!< File to output report information to */
  funit_inst* root    /*!< Pointer to current functional unit instance to output performance stats for */
) { PROFILE(PERF_OUTPUT_INST_REPORT_HELPER);

  funit_inst* curr;  /* Pointer to current child instance to output */

  if( root != NULL ) {

    perf_output_mod_stats( ofile, root->funit );

    curr = root->child_head;
    while( curr != NULL ) {
      perf_output_inst_report_helper( ofile, curr );
      curr = curr->next;
    }

  }

  PROFILE_END;

}

/*!
 Generates a performance report on an instance basis to the specified output file.
*/
void perf_output_inst_report(
  FILE* ofile  /*!< File to output report information to */
) { PROFILE(PERF_OUTPUT_INST_REPORT);

  inst_link* instl;  /* Pointer to current instance link */

  fprintf( ofile, "\nSIMULATION PERFORMANCE STATISTICS:\n\n" );

  instl = db_list[curr_db]->inst_head;
  while( instl != NULL ) {
    perf_output_inst_report_helper( ofile, instl->inst );
    instl = instl->next;
  }

  PROFILE_END;

}

