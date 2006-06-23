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
#include "arc.h"
#include "fsm.h"
#include "assertion.h"
#include "ovl.h"
#include "link.h"
#include "instance.h"
#include "vector.h"


extern funit_inst* instance_root;
extern funit_link* funit_head;
extern isuppl      info_suppl;


/*!
 \param expr      Pointer to expression that is being excluded/included.
 \param inst      Pointer to functional unit instance that contains this expression.
 \param excluded  Specifies if expression is being excluded or included.

 Sets the specified expression's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
void exclude_expr_assign_and_recalc( expression* expr, funit_inst* inst, bool excluded ) {

  float comb_total = 0;  /* Total number of combinational logic coverage points within this tree */
  int   comb_hit   = 0;  /* Total number of hit combinations within this tree */
  int   ulid       = 0;  /* Temporary value */

  /* Now recalculate the coverage information for all metrics if this module is not an OVL module */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( inst->funit ) ) {

    /* If this expression is a root expression, recalculate line coverage */
    if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
      if( excluded ) {
        inst->stat->line_total--;
        inst->stat->line_hit--;
      } else {
        inst->stat->line_total++;
        inst->stat->line_hit++;
      }
    }

    /* Always recalculate combinational coverage */
    if( excluded ) {
      combination_get_tree_stats( expr, &ulid, 0, &comb_total, &comb_hit );
      inst->stat->comb_total -= comb_total;
      inst->stat->comb_hit   -= comb_hit;
    } else {
      expr->suppl.part.excluded = 0;
      combination_get_tree_stats( expr, &ulid, 0, &comb_total, &comb_hit );
      inst->stat->comb_total += comb_total;
      inst->stat->comb_hit   += comb_hit;
    }

  } else {

    /* If the expression is a coverage point, recalculate the assertion coverage */
    if( ovl_is_assertion_module( inst->funit ) && ovl_is_coverage_point( expr ) ) {
      if( excluded ) {
        inst->stat->assert_total--;
        inst->stat->assert_hit--;
      } else {
        inst->stat->assert_total++;
        inst->stat->assert_hit++;
      }
    }

  }

  /* Set the exclude bit in the expression supplemental field */
  expr->suppl.part.excluded = excluded ? 1 : 0;

}

/*!
 \param sig       Pointer to signal that is being excluded/included.
 \param inst      Pointer to functional unit instance that contains this expression.
 \param excluded  Specifies if expression is being excluded or included.

 Sets the specified signal's exclude bit to the given value and recalculates all
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

/*!
 \param arcs       Pointer to arc array.
 \param arc_index  Specifies the index of the entry containing the transition
 \param forward    Specifies if the direction of the transition is forward or backward for the given arc entry
 \param inst       Pointer to functional unit instance containing the FSM
 \param excluded   Specifies if we are excluding or including coverage.

 Sets the specified arc entry's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
void exclude_arc_assign_and_recalc( char* arcs, int arc_index, bool forward, funit_inst* inst, bool excluded ) {

  /* Set the excluded bit in the specified entry */
  arc_set_entry_suppl( arcs, arc_index, (forward ? ARC_EXCLUDED_F : ARC_EXCLUDED_R), (excluded ? 1 : 0) );

  /* Adjust arc coverage numbers */
  if( excluded ) {
    inst->stat->arc_total--;
    inst->stat->arc_hit--;
  } else {
    inst->stat->arc_total++;
    inst->stat->arc_hit++;
  }

}

/*!
 \param funit_name  Name of functional unit to search for
 \param funit_type  Type of functional unit to search for

 \return Returns a pointer to the functional unit instance that points to the functional unit
         described in the parameter list if one is found; otherwise, returns NULL.

 Using the specified functional unit information, returns the functional unit instance that
 corresponds to this description.  If one could not be found, a value of NULL is returned.
*/
funit_inst* exclude_find_instance_from_funit_info( char* funit_name, int funit_type ) {

  func_unit   funit;          /* Temporary functional unit structure used for searching */
  funit_link* funitl;         /* Found functional unit link */
  int         ignore = 0;     /* Number of functional unit instances to ignore in search */
  funit_inst* inst   = NULL;  /* Found functional unit instance */

  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {
    inst = instance_find_by_funit( instance_root, funitl->funit, &ignore );
  }

  return( inst );

}

/*!
 \param funit_name  Name of functional unit containing expression to set line exclusion for
 \param funit_type  Type of functional unit containing expression to set line exclusion for
 \param line        Line number of expression that needs to be set
 \param value       Specifies if we should exclude (1) or include (0) the specified line

 \return Returns TRUE if we successfully set the appropriate expression(s); otherwise, returns FALSE.

 Finds the expression(s) and functional unit instance for the given name, type and line number and calls
 the exclude_expr_assign_and_recalc function for each matching expression, setting the excluded bit
 of the expression and recalculating the summary coverage information.
*/
bool exclude_set_line_exclude( char* funit_name, int funit_type, int line, int value ) {

  bool        retval = FALSE;  /* Return value for this function */
  funit_inst* inst;            /* Found functional unit instance */
  exp_link*   expl;            /* Pointer to current expression link */

  /* Find the functional unit instance that matches the functional unit description */ 
  if( (inst = exclude_find_instance_from_funit_info( funit_name, funit_type )) != NULL ) {

    /* Find the expression(s) that match the given line number */
    expl = inst->funit->exp_head;
    do {
      while( (expl != NULL) && ((expl->exp->line != line) || (ESUPPL_IS_ROOT( expl->exp->suppl ) == 0)) ) {
        expl = expl->next;
      }
      if( expl != NULL ) {
        exclude_expr_assign_and_recalc( expl->exp, inst, (value == 1) );      
        retval = TRUE;
      }
    } while( expl != NULL );

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit containing expression to set line exclusion for
 \param funit_type  Type of functional unit containing expression to set line exclusion for
 \param sig_name    Name of signal to set the toggle exclusion for
 \param value       Specifies if we should exclude (1) or include (0) the specified line

 \return Returns TRUE if we successfully set the appropriate expression(s); otherwise, returns FALSE.

 Finds the signal and functional unit instance for the given name, type and sig_name and calls
 the exclude_sig_assign_and_recalc function for the matching signal, setting the excluded bit
 of the signal and recalculating the summary coverage information.
*/
bool exclude_set_toggle_exclude( char* funit_name, int funit_type, char* sig_name, int value ) {

  bool        retval = FALSE;  /* Return value for this function */
  funit_inst* inst;            /* Found functional unit instance */
  vsignal     sig;             /* Temporary signal used for searching */
  sig_link*   sigl;            /* Pointer to current signal link */

  /* Find the functional unit instance that matches the functional unit description */
  if( (inst = exclude_find_instance_from_funit_info( funit_name, funit_type )) != NULL ) {

    /* Find the signal that matches the given signal name and sets its excluded bit */
    sig.name = sig_name;
    if( (sigl = sig_link_find( &sig, inst->funit->sig_head )) != NULL ) {
      exclude_sig_assign_and_recalc( sigl->sig, inst, (value == 1) );
      retval = TRUE;
    }
      
  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit containing expression to set combination/assertion exclusion for
 \param funit_type  Type of functional unit containing expression to set combination/assertion exclusion for
 \param expr_id     Expression ID of expression to set exclude value for
 \param value       Specifies if we should exclude (1) or include (0) the specified line

 \return Returns TRUE if we successfully set the appropriate expression(s); otherwise, returns FALSE.

 Finds the expression and functional unit instance for the given name, type and sig_name and calls
 the exclude_expr_assign_and_recalc function for the matching expression, setting the excluded bit
 of the expression and recalculating the summary coverage information.
*/
bool exclude_set_comb_assert_exclude( char* funit_name, int funit_type, int expr_id, int value ) {

  bool        retval = FALSE;  /* Return value for this function */
  funit_inst* inst;            /* Found functional unit instance */
  expression  exp;             /* Temporary expression used for searching */
  exp_link*   expl;            /* Pointer to current expression link */

  /* Find the functional unit instance that matches the functional unit description */
  if( (inst = exclude_find_instance_from_funit_info( funit_name, funit_type )) != NULL ) {

    /* Find the signal that matches the given signal name and sets its excluded bit */
    exp.id = expr_id;
    if( (expl = exp_link_find( &exp, inst->funit->exp_head )) != NULL ) {
      exclude_expr_assign_and_recalc( expl->exp, inst, (value == 1) );
      retval = TRUE;
    }

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit containing expression to set combination/assertion exclusion for
 \param funit_type  Type of functional unit containing expression to set combination/assertion exclusion for
 \param expr_id     Expression ID of output state variable
 \param from_state  String containing input state value
 \param to_state    String containing output state value
 \param value       Specifies if we should exclude (1) or include (0) the specified line

 \return Returns TRUE if we successfully set the appropriate expression(s); otherwise, returns FALSE.

*/
bool exclude_set_fsm_exclude( char* funit_name, int funit_type, int expr_id, char* from_state, char* to_state, int value ) {

  bool        retval = FALSE;  /* Return value for this function */
  funit_inst* inst;            /* Found functional unit instance */
  int         find_val;        /* Value from return of arc_find function */
  int         found_index;     /* Index of found arc entry */
  vector*     from_vec;        /* Vector form of from_state */
  vector*     to_vec;          /* Vector form of to_state */
  fsm_link*   curr_fsm;        /* Pointer to the current FSM to search on */

  /* Find the functional unit instance that matches the functional unit description */
  if( (inst = exclude_find_instance_from_funit_info( funit_name, funit_type )) != NULL ) {

    /* Find the corresponding table */
    curr_fsm = inst->funit->fsm_head;
    while( (curr_fsm != NULL) && (curr_fsm->table->to_state->id != expr_id) ) {
      curr_fsm = curr_fsm->next;
    }

    if( curr_fsm != NULL ) {

      /* Convert from/to state strings into vector values */
      from_vec = vector_from_string( &from_state, FALSE );
      to_vec   = vector_from_string( &to_state, FALSE );

      /* Find the arc entry and perform the exclusion assignment and coverage recalculation */
      if( (find_val = arc_find( curr_fsm->table->table, from_vec, to_vec, &found_index )) != 2 ) {
        exclude_arc_assign_and_recalc( curr_fsm->table->table, found_index, (find_val == 0), inst, (value == 1) );
        retval = TRUE;
      }

    }

  }

  return( retval );

}


/*
 $Log$
 Revision 1.2  2006/06/23 04:03:30  phase1geo
 Updating build files and removing syntax errors in exclude.h and exclude.c
 (though this code doesn't do anything meaningful at this point).

 Revision 1.1  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

*/

