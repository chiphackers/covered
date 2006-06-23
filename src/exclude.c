/*!
 \file     exclude.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/22/2006
*/

#include "defines.h"
#include "exclude.h"
#include "expr.h"
#include "line.h"
#include "toggle.h"
#include "comb.h"
#include "fsm.h"
#include "assertion.h"
#include "ovl.h"


extern isuppl info_suppl;


/*!
 \param expr      Pointer to current expression to evaluate
 \param inst      Pointer to functional unit instance containing this expression
 \param excluded  Specifies if expression is being excluded or included

 Recursively iterates down expression tree, recalculating combinational expression
 numbers.
*/
void exclude_recalc_comb( expression* expr, funit_inst* inst, bool excluded ) {

  if( (expr != NULL) && (ESUPPL_EXCLUDED( expr->suppl ) == 0) ) {

    /* Recalculate combinational logic */

  }

}

/*!
 \param expr      Pointer to expression that is being excluded/included.
 \param inst      Pointer to functional unit instance that contains this expression.
 \param excluded  Specifies if expression is being excluded or included.

 Sets the specified expression's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
void exclude_expr_assign_and_recalc( expression* expr, funit_inst* inst, bool excluded ) {

  /* Now recalculate the coverage information for all metrics if this module is not an OVL module */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( inst->funit ) ) {

    /* If this expression is a root expression, recalculate line coverage */
    if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
      inst->stat->line_total = 0;
      inst->stat->line_hit   = 0;
      line_get_stats( inst->funit->stmt_head, &(inst->stat->line_total), &(inst->stat->line_hit) );
    }

    /* Always recalculate combinational coverage */
    inst->stat->comb_total = 0;
    inst->stat->comb_hit   = 0;
    combination_get_stats( inst->funit->exp_head, &(inst->stat->comb_total), &(inst->stat->comb_hit) );

    /* If this expression was connected to an FSM, recalculate its coverage */
    if( expr->table != NULL ) {
      inst->stat->state_total = 0;
      inst->stat->state_hit   = 0;
      inst->stat->arc_total   = 0;
      inst->stat->arc_hit     = 0;
      fsm_get_stats( inst->funit->fsm_head, &(inst->stat->state_total), &(inst->stat->state_hit),
                     &(inst->stat->arc_total), &(inst->stat->arc_hit) );
    }

  }

  /* Set the exclude bit in the expression supplemental field */
  expr->suppl.part.excluded = excluded ? 1 : 0;

}

/*!
 \param sig       Pointer to signal that is being excluded/included.
 \param inst      Pointer to functional unit instance that contains this expression.
 \param excluded  Specifies if expression is being excluded or included.

 Sets the specified expression's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
void exclude_sig_assign_and_recalc( vsignal* sig, funit_inst* inst, bool excluded ) {

  int hit01;  /* Number of bits transitioning from 0 -> 1 */
  int hit10;  /* Number of bits transitioning from 1 -> 0 */

  /* First, set the exclude bit in the signal supplemental field */
  sig->suppl.part.excluded = excluded ? 1 : 0;

  /* Get the total hit01 and hit10 information */
  vector_toggle_count( sig->value, &hit01, &hit10 );

  /* Recalculate the total and hit values for toggle coverage */
  if( excluded ) {
    inst->stat->tog_total -= sig->value->width;
    inst->stat->tog01_hit -= hit01;
    inst->stat->tog10_hit -= hit10;
  } else {
    inst->stat->tog_total += sig->value->width;
    inst->stat->tog01_hit += hit01;
    inst->stat->tog10_hit += hit10;
  }

}

/*
 $Log$
 Revision 1.1  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

*/

