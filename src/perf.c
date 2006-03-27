/*!
 \file     perf.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/1/2006
*/

#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "perf.h"
#include "util.h"
#include "expr.h"


extern funit_link* funit_head;
extern funit_inst* instance_root;


/*!
 \param funit  Pointer to functional unit to generate performance statistics for

 \return Returns a pointer to the perf_stat structure containing the performance statistics
         for the given functional unit

 Allocates and initializes the contents of a performance structure to display the total number
 of expressions in the given functional unit and the number of times these expressions that
 were executed during simulation.
*/
perf_stat* perf_gen_stats( func_unit* funit ) {

  exp_link*  expl;   /* Pointer to current expression link */
  perf_stat* pstat;  /* Pointer to newly created performance stat structure */
  int        i;      /* Loop iterator */

  /* Create and initialize new perf_stat structure */
  pstat = (perf_stat*)malloc_safe( sizeof( perf_stat ), __FILE__, __LINE__ );
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
void perf_output_mod_stats( FILE* ofile, func_unit* funit ) {

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
      fprintf( ofile, "    %-10.10s %-8.0f   %-8d   %-8.1f\n",
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
void perf_output_inst_report_helper( FILE* ofile, funit_inst* root ) {

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
void perf_output_inst_report( FILE* ofile ) {

  funit_inst* curr;  /* Pointer to current functional unit instance */

  fprintf( ofile, "\nSIMULATION PERFORMANCE STATISTICS:\n\n" );

  perf_output_inst_report_helper( ofile, instance_root );

}

/*
 $Log$
 Revision 1.1  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

*/
