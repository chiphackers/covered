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
 \file     exclude.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/22/2006
*/

#include "arc.h"
#include "assertion.h"
#include "comb.h"
#include "db.h"
#include "defines.h"
#include "exclude.h"
#include "expr.h"
#include "fsm.h"
#include "func_iter.h"
#include "func_unit.h"
#include "instance.h"
#include "line.h"
#include "link.h"
#include "memory.h"
#include "ovl.h"
#include "profiler.h"
#include "toggle.h"
#include "vector.h"


extern db**         db_list;
extern unsigned int curr_db;
extern isuppl       info_suppl;
extern char         user_msg[USER_MSG_LENGTH];
extern int          merge_er_value;

/*!
 Name of CDD file that will be read, modified with exclusion modifications and written back.
*/
static char* exclude_cdd = NULL;

/*!
 Pointer to the head of the list of exclusion IDs to toggle exclusion/inclusion mode of.
*/
static str_link* excl_ids_head = NULL;

/*!
 Pointer to the tail of the list of exclusion IDs to toggle exclusion/inclusion mode of.
*/
static str_link* excl_ids_tail = NULL;

/*!
 If set to TRUE, causes a message prompt to be displayed for each coverage point that will
 be excluded from coverage.
*/
static bool exclude_prompt_for_msgs = FALSE;

/*!
 If set to TRUE, prints the status and exclusion reason for the given exclusion ID.  Set to
 TRUE via the -p option.
*/
static bool exclude_print = FALSE;


static char* exclude_get_message( const char* eid );


/*!
 \return Returns TRUE if a parent expression of this expression was found to be excluded from
         coverage; otherwise, returns FALSE.
*/
static bool exclude_is_parent_excluded(
    expression* expr  /*!< Pointer to current expression to evaluate */
) {

  return( (expr != NULL) &&
          ((ESUPPL_EXCLUDED( expr->suppl ) == 1) ||
           ((ESUPPL_IS_ROOT( expr->suppl ) == 0) && exclude_is_parent_excluded( expr->parent->expr ))) );

}

/*!
 Sets the specified expression's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
static void exclude_expr_assign_and_recalc(
            expression* expr,       /*!< Pointer to expression that is being excluded/included */
            func_unit*  funit,      /*!< Pointer to functional unit containing this expression */
            bool        excluded,   /*!< Specifies if expression is being excluded or included */
            bool        set_line,   /*!< Set to TRUE when this function is being called for line exclusion */
            bool        rpt_comb,   /*!< Combinational coverage is being reported */
            bool        rpt_event,  /*!< Event coverage is being reported */
  /*@out@*/ statistic*  stat        /*!< Pointer to statistics structure to update */
) { PROFILE(EXCLUDE_EXPR_ASSIGN_AND_RECALC);

  unsigned int comb_hit      = 0;  /* Total number of hit combinations within this tree */
  unsigned int comb_excluded = 0;  /* Total number of excluded combinations */
  unsigned int comb_total    = 0;  /* Total number of combinational logic coverage points within this tree */
  int          ulid          = 0;  /* Temporary value */

  /* Now recalculate the coverage information for all metrics if this module is not an OVL module */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( funit ) ) {

    /* If this expression is a root expression, recalculate line coverage */
    if( ESUPPL_IS_ROOT( expr->suppl ) == 1 ) {
      if( (expr->op != EXP_OP_DELAY)   &&
          (expr->op != EXP_OP_CASE)    &&
          (expr->op != EXP_OP_CASEX)   &&
          (expr->op != EXP_OP_CASEZ)   &&
          (expr->op != EXP_OP_DEFAULT) &&
          (expr->op != EXP_OP_NB_CALL) &&
          (expr->op != EXP_OP_FORK)    &&
          (expr->op != EXP_OP_JOIN)    &&
          (expr->op != EXP_OP_NOOP)    &&
          (expr->line != 0) &&
          (expr->exec_num == 0) ) {
        if( excluded ) {
          stat->line_hit++;
          stat->line_excluded++;
        } else {
          stat->line_hit--;
          stat->line_excluded--;
        }
      }
    }

    /* Always recalculate combinational coverage */
    combination_reset_counted_expr_tree( expr );
    if( excluded ) {
      combination_get_tree_stats( expr, rpt_comb, rpt_event, &ulid, 0, exclude_is_parent_excluded( expr ), &comb_hit, &comb_excluded, &comb_total );
      stat->comb_hit      += (comb_total - comb_hit);
      stat->comb_excluded += (comb_total - comb_excluded);
    } else {
      expr->suppl.part.excluded = 0;
      combination_get_tree_stats( expr, rpt_comb, rpt_event, &ulid, 0, exclude_is_parent_excluded( expr ), &comb_hit, &comb_excluded, &comb_total );
      stat->comb_hit      -= (comb_total - comb_hit);
      stat->comb_excluded -= (comb_total - comb_excluded);
    }

  } else {

    /* If the expression is a coverage point, recalculate the assertion coverage */
    if( ovl_is_assertion_module( funit ) && ovl_is_coverage_point( expr ) ) {
      if( expr->exec_num == 0 ) {
        if( excluded ) {
          stat->assert_hit++;
          stat->assert_excluded++;
        } else {
          stat->assert_hit--;
          stat->assert_excluded--;
        }
      }
    }

  }

  /* Set the exclude bits in the expression supplemental field */
  expr->suppl.part.excluded = excluded ? 1 : 0;
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 1) && (expr->parent->stmt != NULL) ) {
    expr->parent->stmt->suppl.part.excluded = (excluded && set_line) ? 1 : 0;
  }

  PROFILE_END;

}

/*!
 Sets the specified signal's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
static void exclude_sig_assign_and_recalc(
            vsignal*   sig,       /*!< Pointer to signal that is being excluded/included */
            bool       excluded,  /*!< Specifies if signal is being excluded or included */
  /*@out@*/ statistic* stat       /*!< Pointer to statistic structure to update */
) { PROFILE(EXCLUDE_SIG_ASSIGN_AND_RECALC);

  /* First, set the exclude bit in the signal supplemental field */
  sig->suppl.part.excluded = excluded ? 1 : 0;

  /* If the signal is a memory, we need to update the memory coverage numbers */
  if( sig->suppl.part.type == SSUPPL_TYPE_MEM ) {

    unsigned int wr_hit       = 0;  /* Number of addressable elements written */
    unsigned int rd_hit       = 0;  /* Number of addressable elements read */
    unsigned int ae_total     = 0;  /* Number of addressable elements in this memory */
    unsigned int tog01_hit    = 0;  /* Number of bits toggling from 0->1 */
    unsigned int tog10_hit    = 0;  /* Number of bits toggling from 1->0 */
    unsigned int tog_total    = 0;  /* Total number of toggle bits */
    unsigned int mem_excluded = 0;  /* Number of excluded memory coverage points */
    bool         cov_found;

    /* Get the stats for the current memory */
    memory_get_stat( sig, &wr_hit, &rd_hit, &ae_total, &tog01_hit, &tog10_hit, &tog_total, &mem_excluded, &cov_found, TRUE );

    /* Recalculate the total and hit values for memory coverage */
    if( excluded ) {
      stat->mem_wr_hit    += (ae_total  - wr_hit);
      stat->mem_rd_hit    += (ae_total  - rd_hit);
      stat->mem_tog01_hit += (tog_total - tog01_hit);
      stat->mem_tog10_hit += (tog_total - tog10_hit);
      stat->mem_excluded  += ((ae_total * 2) + (tog_total * 2));
    } else {
      stat->mem_wr_hit    -= (ae_total  - wr_hit);
      stat->mem_rd_hit    -= (ae_total  - rd_hit);
      stat->mem_tog01_hit -= (tog_total - tog01_hit);
      stat->mem_tog10_hit -= (tog_total - tog10_hit);
      stat->mem_excluded  -= ((ae_total * 2) + (tog_total * 2));
    }

  /* Otherwise, the toggle coverage numbers should be adjusted */
  } else {

    unsigned int hit01 = 0;  /* Number of bits transitioning from 0 -> 1 */
    unsigned int hit10 = 0;  /* Number of bits transitioning from 1 -> 0 */

    /* Get the total hit01 and hit10 information */
    vector_toggle_count( sig->value, &hit01, &hit10 );

    /* Recalculate the total and hit values for toggle coverage */
    if( excluded ) {
      stat->tog01_hit    += (sig->value->width - hit01);
      stat->tog10_hit    += (sig->value->width - hit10);
      stat->tog_excluded += (sig->value->width * 2);
    } else {
      stat->tog01_hit    -= (sig->value->width - hit01);
      stat->tog10_hit    -= (sig->value->width - hit10);
      stat->tog_excluded -= (sig->value->width * 2);
    }

  }

  PROFILE_END;

}

/*!
 Sets the specified arc entry's exclude bit to the given value and recalculates all
 affected coverage information for this instance.
*/
static void exclude_arc_assign_and_recalc(
            fsm_table* table,      /*!< Pointer FSM table */
            int        arc_index,  /*!< Specifies the index of the entry containing the transition */
            bool       exclude,    /*!< Specifies if we are excluding or including coverage */
  /*@out@*/ statistic* stat        /*!< Pointer to statistic structure to update */
) { PROFILE(EXCLUDE_ARC_ASSIGN_AND_RECALC);

  /* Set the excluded bit in the specified entry and adjust coverage numbers, if necessary */
  table->arcs[arc_index]->suppl.part.excluded = (exclude ? 1 : 0);
  if( table->arcs[arc_index]->suppl.part.hit == 0 ) {
    stat->arc_hit      += exclude ? 1 : -1;
    stat->arc_excluded += exclude ? 1 : -1;
  }

  PROFILE_END;

}

/*!
 Handle the creation of the exclude reason structure.
*/
static void exclude_add_exclude_reason(
  char       type,    /*!< Exclusion ID type */
  int        id,      /*!< Exclusion ID number */
  char*      reason,  /*!< Exclusion reason */
  func_unit* funit    /*!< Functional unit containing sig */
) { PROFILE(EXCLUDE_ADD_EXCLUDE_REASON);

  exclude_reason* er;
  int             rv;
  struct timeval  tv;

  /*
   If the coverage point was not previously excluded, allow the user to specify a reason and
   store this information in the functional unit.
  */
  er            = (exclude_reason*)malloc_safe( sizeof( exclude_reason ) );
  er->type      = type;
  er->id        = id;
  er->reason    = reason;
  er->next      = NULL;

  /* Add timestamp information */
  rv = gettimeofday( &tv, NULL );
  assert( rv == 0 );
  er->timestamp = tv.tv_sec;

  if( funit->er_head == NULL ) { 
    funit->er_head = funit->er_tail = er; 
  } else {
    funit->er_tail->next = er; 
    funit->er_tail       = er; 
  }   

}

/*!
 Handles the deallocation of the exclude reason structure.
*/
static void exclude_remove_exclude_reason(
  char       type,  /*!< Exclusion ID type */
  int        id,    /*!< Exclusion ID number */
  func_unit* funit  /*!< Functional unit containing sig */
) { PROFILE(EXCLUDE_REMOVE_EXCLUDE_REASON);

  exclude_reason* last_er = NULL;
  exclude_reason* er      = funit->er_head;

  /*
   If the coverage point was previously excluded, attempt to find the matching exclusion reason, and, if
   it is found, remove it from the list.
  */
  while( (er != NULL) && ((er->type != type) || (er->id != id)) ) {
    last_er = er; 
    er      = er->next;
  }

  if( er != NULL ) { 
    if( last_er == NULL ) { 
      funit->er_head = er->next;
      if( er->next == NULL ) {
        funit->er_tail = NULL;
      }
    } else {
      last_er->next = er->next;
    }
    free_safe( er->reason, (strlen( er->reason ) + 1) );
    free_safe( er, sizeof( exclude_reason ) );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the specified line is excluded in the given functional unit; otherwise, returns FALSE.
*/
bool exclude_is_line_excluded(
  func_unit* funit,  /*!< Pointer to functional unit to check */
  int        line    /*!< Line number of line to check */
) { PROFILE(EXCLUDE_IS_LINE_EXCLUDED);

  func_iter  fi;    /* Functional unit iterator */
  statement* stmt;  /* Pointer to current statement */

  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );
  while( ((stmt = func_iter_get_next_statement( &fi )) != NULL) && (stmt->exp->line != line) );
  func_iter_dealloc( &fi );

  PROFILE_END;

  return( (stmt == NULL) || (stmt->suppl.part.excluded == 1) );

}

/*!
 Finds the expression(s) and functional unit instance for the given name, type and line number and calls
 the exclude_expr_assign_and_recalc function for each matching expression, setting the excluded bit
 of the expression and recalculating the summary coverage information.
*/
void exclude_set_line_exclude(
            func_unit* funit,      /*!< Pointer to functional unit */
            int        line,       /*!< Line number of expression that needs to be set */
            int        value,      /*!< Specifies if we should exclude (1) or include (0) the specified line */
            char*      reason,     /*!< Reason for the exclusion if value is 1 */
            bool       rpt_comb,   /*!< Combinational coverage is being reported */
            bool       rpt_event,  /*!< Event coverage is being reported */
  /*@out@*/ statistic* stat        /*!< Pointer to statistics structure to update */
) { PROFILE(EXCLUDE_SET_LINE_EXCLUDE);

  func_iter  fi;    /* Functional unit iterator */
  statement* stmt;  /* Pointer to current statement */

  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

  do {
    while( ((stmt = func_iter_get_next_statement( &fi )) != NULL) && (stmt->exp->line != line) );
    if( stmt != NULL ) {

      exclude_expr_assign_and_recalc( stmt->exp, funit, (value == 1), TRUE, rpt_comb, rpt_event, stat );

      /* Handle the exclusion reason */
      if( value == 1 ) {
        if( reason != NULL ) {
          exclude_add_exclude_reason( 'L', stmt->exp->id, reason, funit );
        } 
      } else {
        exclude_remove_exclude_reason( 'L', stmt->exp->id, funit );
      }

    }
  } while( stmt != NULL );

  func_iter_dealloc( &fi );

  PROFILE_END;

}

/*!
 \return Returns TRUE if the specified signal is excluded from coverage consideration; otherwise, returns FALSE.
*/
bool exclude_is_toggle_excluded(
  func_unit* funit,    /*!< Pointer to functional unit to check */
  char*      sig_name  /*!< Name of signal to search for */
) { PROFILE(EXCLUDE_IS_TOGGLE_EXCLUDED);

  func_iter fi;   /* Functional unit iterator */
  vsignal*  sig;  /* Pointer to current signal */

  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );
  while( ((sig = func_iter_get_next_signal( &fi )) != NULL) && (strcmp( sig->name, sig_name ) != 0) );
  func_iter_dealloc( &fi );

  PROFILE_END;

  return( (sig == NULL) || (sig->suppl.part.excluded == 1) );

}

/*!
 Finds the signal and functional unit instance for the given name, type and sig_name and calls
 the exclude_sig_assign_and_recalc function for the matching signal, setting the excluded bit
 of the signal and recalculating the summary coverage information.
*/
void exclude_set_toggle_exclude(
            func_unit*  funit,     /*!< Pointer to functional unit */
            const char* sig_name,  /*!< Name of signal to set the toggle exclusion for */
            int         value,     /*!< Specifies if we should exclude (1) or include (0) the specified line */
            char        type,      /*!< Exclusion ID type (T=toggle, M=memory) */
            char*       reason,    /*!< Reason for exclusion (if value is 1) */
  /*@out@*/ statistic*  stat       /*!< Pointer to statistics structure to update */
) { PROFILE(EXCLUDE_SET_TOGGLE_EXCLUDE);

  func_iter fi;   /* Functional unit iterator */
  vsignal*  sig;  /* Pointer to current signal */

  /* Find the signal that matches the given signal name, if it exists */
  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );
  while( ((sig = func_iter_get_next_signal( &fi )) != NULL) && (strcmp( sig->name, sig_name ) != 0) );
  func_iter_dealloc( &fi );

  /* Set its exclude bit if it exists */
  if( sig != NULL ) {

    /* Exclude/include the signal and recalculate the summary information */
    exclude_sig_assign_and_recalc( sig, (value == 1), stat );

    /* Handle the exclusion reason */
    if( value == 1 ) {
      if( reason != NULL ) {
        exclude_add_exclude_reason( type, sig->id, reason, funit ); 
      }
    } else {
      exclude_remove_exclude_reason( type, sig->id, funit );
    }
      
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if specified underlined expression is excluded from coverage; otherwise, returns FALSE.
*/
bool exclude_is_comb_excluded(
  func_unit* funit,    /*!< Pointer to functional unit containing the expression to exclude/include */
  int        expr_id,  /*!< Expression ID of the root expression to exclude/include */
  int        uline_id  /*!< Underline ID of expression to exclude/include */
) { PROFILE(EXCLUDE_IS_COMB_EXCLUDED);

  func_iter   fi;      /* Functional unit iterator */
  statement*  stmt;    /* Pointer to found statement */
  expression* subexp;  /* Pointer to found expression */

  /* Find the matching root expression */
  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );
  while( ((stmt = func_iter_get_next_statement( &fi )) != NULL) && (stmt->exp->id != expr_id) );
  func_iter_dealloc( &fi );

  if( stmt != NULL ) {
    subexp = expression_find_uline_id( stmt->exp, uline_id );
  }

  PROFILE_END;

  return( (stmt == NULL) || (subexp == NULL) || (subexp->suppl.part.excluded == 1) );

}

/*!
 Finds the expression and functional unit instance for the given name, type and sig_name and calls
 the exclude_expr_assign_and_recalc function for the matching expression, setting the excluded bit
 of the expression and recalculating the summary coverage information.
*/
void exclude_set_comb_exclude(
            func_unit* funit,      /*!< Pointer to functional unit */
            int        expr_id,    /*!< Expression ID of root expression to set exclude value for */
            int        uline_id,   /*!< Underline ID of expression to set exclude value for */
            int        value,      /*!< Specifies if we should exclude (1) or include (0) the specified line */
            char*      reason,     /*!< Reason for the exclusion (if value is 1) */
            bool       rpt_comb,   /*!< Combinational coverage is being reported */
            bool       rpt_event,  /*!< Event coverage is being reported */
  /*@out@*/ statistic* stat        /*!< Pointer to statistic structure to update */
) { PROFILE(EXCLUDE_SET_COMB_EXCLUDE);

  func_iter  fi;    /* Functional unit iterator */
  statement* stmt;  /* Pointer to current statement */

  /* Find the root expression */
  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );
  while( ((stmt = func_iter_get_next_statement( &fi )) != NULL) && (stmt->exp->id != expr_id) );
  func_iter_dealloc( &fi );

  if( stmt != NULL ) {

    expression* subexp;

    if( (subexp = expression_find_uline_id( stmt->exp, uline_id )) != NULL ) {

      /* Exclude/include the expression and recalculate the summary information */
      exclude_expr_assign_and_recalc( subexp, funit, (value == 1), FALSE, rpt_comb, rpt_event, stat );

      /* Handle the exclusion reason */
      if( value == 1 ) {
        if( reason != NULL ) {
          exclude_add_exclude_reason( 'E', subexp->id, reason, funit );
        }
      } else {
        exclude_remove_exclude_reason( 'E', subexp->id, funit );
      }

    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the given FSM state transition was excluded from coverage; otherwise, returns FALSE.
*/
bool exclude_is_fsm_excluded(
  func_unit* funit,       /*!< Pointer to functional unit containing FSM to exclude/include */
  int        expr_id,     /*!< Expression ID of FSM */
  char*      from_state,  /*!< String form of the from_state value */
  char*      to_state     /*!< String form of the to_state value */
) { PROFILE(EXCLUDE_IS_FSM_EXCLUDED);

  int          found_index;  /* Index of found state transition */
  unsigned int i = 0;

  /* Find the corresponding table */
  while( (i < funit->fsm_size) && (funit->fsms[i]->to_state->id != expr_id) ) i++;

  if( i < funit->fsm_size ) {

    vector* from_vec;
    vector* to_vec;
    int     from_base, to_base;

    /* Convert from/to state strings into vector values */
    vector_from_string( &from_state, FALSE, &from_vec, &from_base );
    vector_from_string( &to_state, FALSE, &to_vec, &to_base );

    /* Find the arc entry and perform the exclusion assignment and coverage recalculation */
    found_index = arc_find_arc( funit->fsms[i]->table, arc_find_from_state( funit->fsms[i]->table, from_vec ), arc_find_to_state( funit->fsms[i]->table, to_vec ) );

    /* Deallocate vectors */
    vector_dealloc( from_vec );
    vector_dealloc( to_vec );

  }

  PROFILE_END;

  return( (i == funit->fsm_size) || (found_index == -1) || (funit->fsms[i]->table->arcs[found_index]->suppl.part.excluded == 1) );

}

/*!
 Finds the FSM table associated with the specified expr_id and sets the include/exclude status of the given
 from_state/to_state transition appropriately.
*/
void exclude_set_fsm_exclude(
            func_unit* funit,       /*!< Pointer to functional unit */
            int        expr_id,     /*!< Expression ID of output state variable */
            char*      from_state,  /*!< String containing input state value */
            char*      to_state,    /*!< String containing output state value */
            int        value,       /*!< Specifies if we should exclude (1) or include (0) the specified line */
            char*      reason,      /*!< Reason for the exclusion (if value is 1) */
  /*@out@*/ statistic* stat         /*!< Pointer to statistics structure to update */
) { PROFILE(EXCLUDE_SET_FSM_EXCLUDE);

  unsigned int i = 0;
  /* Find the corresponding table */
  while( (i < funit->fsm_size) && (funit->fsms[i]->to_state->id != expr_id) ) i++;

  if( i < funit->fsm_size ) {

    vector* from_vec;
    vector* to_vec;
    int     found_index;
    int     from_base, to_base;

    /* Convert from/to state strings into vector values */
    vector_from_string( &from_state, FALSE, &from_vec, &from_base );
    vector_from_string( &to_state, FALSE, &to_vec, &to_base );

    /* Find the arc entry and perform the exclusion assignment and coverage recalculation */
    if( (found_index = arc_find_arc( funit->fsms[i]->table, arc_find_from_state( funit->fsms[i]->table, from_vec ), arc_find_to_state( funit->fsms[i]->table, to_vec ) )) != -1 ) {

      /* Handle the exclusion and recalculate the summary values */
      exclude_arc_assign_and_recalc( funit->fsms[i]->table, found_index, (value == 1), stat );
 
      /* Handle the exclusion reason */
      if( value == 1 ) {
        if( reason != NULL ) {
          exclude_add_exclude_reason( 'F', (funit->fsms[i]->table->id + found_index), reason, funit );
        }
      } else {
        exclude_remove_exclude_reason( 'F', (funit->fsms[i]->table->id + found_index), funit );
      }

    }

    /* Deallocate vectors */
    vector_dealloc( from_vec );
    vector_dealloc( to_vec );

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the given assertion is excluded from coverage consideration; otherwise, returns FALSE.
*/
bool exclude_is_assert_excluded(
  func_unit* funit,      /*!< Pointer to functional unit containing the assertion to exclude/include */
  char*      inst_name,  /*!< Name of assertion instance to exclude/include */
  int        expr_id     /*!< Expression ID to exclude/include */
) { PROFILE(EXCLUDE_IS_ASSERT_EXCLUDED);

  funit_inst* inst;        /* Pointer to found functional unit instance */
  funit_inst* curr_child;  /* Pointer to current child functional instance */
  int         ignore = 0;  /* Number of instances to ignore */
  statement*  stmt;        /* Pointer to current statement */

  /* Find the functional unit instance that matches the description */
  if( (inst = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore )) != NULL ) {

    func_iter fi;

    /* Find child instance */
    curr_child = inst->child_head;
    while( (curr_child != NULL) && (strcmp( curr_child->name, inst_name ) != 0) ) {
      curr_child = curr_child->next;
    }
    assert( curr_child != NULL );

    /* Initialize the functional unit iterator */
    func_iter_init( &fi, curr_child->funit, TRUE, FALSE, FALSE );

    while( ((stmt = func_iter_get_next_statement( &fi )) != NULL) && (stmt->exp->id != expr_id) );

    /* Deallocate functional unit statement iterator */
    func_iter_dealloc( &fi );

  }

  PROFILE_END;

  return( (inst == NULL) || (stmt == NULL) || (stmt->exp->id != expr_id) || (stmt->exp->suppl.part.excluded == 1) );

}

/*!
 Finds the expression and functional unit instance for the given name, type and sig_name and calls
 the exclude_expr_assign_and_recalc function for the matching expression, setting the excluded bit
 of the expression and recalculating the summary coverage information.
*/
void exclude_set_assert_exclude(
            func_unit* funit,      /*!< Pointer to functional unit */
            char*      inst_name,  /*!< Name of child instance to find in given functional unit */
            int        expr_id,    /*!< Expression ID of expression to set exclude value for */
            int        value,      /*!< Specifies if we should exclude (1) or include (0) the specified line */
            char*      reason,     /*!< Reason for the exclusion (if value is 1) */
            bool       rpt_comb,   /*!< Combinational coverage is being reported */
            bool       rpt_event,  /*!< Event coverage is being reported */
  /*@out@*/ statistic* stat        /*!< Pointer to statistic structure to update */
) { PROFILE(EXCLUDE_SET_ASSERT_EXCLUDE);

  funit_inst* inst;        /* Pointer to found functional unit instance */
  funit_inst* curr_child;  /* Pointer to current child functional instance */
  int         ignore = 0;  /* Number of instances to ignore */

  /* Find the functional unit instance that matches the description */
  if( (inst = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore )) != NULL ) {
   
    func_iter  fi;
    statement* stmt;

    /* Find child instance */
    curr_child = inst->child_head;
    while( (curr_child != NULL) && (strcmp( curr_child->name, inst_name ) != 0) ) {
      curr_child = curr_child->next;
    }
    assert( curr_child != NULL );

    /* Initialize the functional unit iterator */
    func_iter_init( &fi, curr_child->funit, TRUE, FALSE, FALSE );

    while( ((stmt = func_iter_get_next_statement( &fi )) != NULL) && (stmt->exp->id != expr_id) );

    /* Find the signal that matches the given signal name and sets its excluded bit */
    if( stmt->exp->id == expr_id ) {

      /* Exclude/include the assertion and recalculate the summary information */
      exclude_expr_assign_and_recalc( stmt->exp, curr_child->funit, (value == 1), FALSE, rpt_comb, rpt_event, stat );

      /* Handle the exclusion reason */
      if( value == 1 ) {
        if( reason != NULL ) {
          exclude_add_exclude_reason( 'A', stmt->exp->id, reason, curr_child->funit );
        }
      } else {
        exclude_remove_exclude_reason( 'A', stmt->exp->id, curr_child->funit );
      }

    }

    /* Deallocate functional unit statement iterator */
    func_iter_dealloc( &fi );

  }

  PROFILE_END;

}

/********************************************************************************************/

/*!
 Outputs usage information to standard output for exclude command.
*/
static void exclude_usage() {

  printf( "\n" );
  printf( "Usage:  covered exclude (-h | ([<options>] <exclusion_ids>+ <database_file>)\n" );
  printf( "\n" );
  printf( "   -h                           Displays this help information.\n" );
  printf( "\n" );
  printf( "   Options:\n" );
  printf( "      -f <filename>             Name of file containing additional arguments to parse.\n" );
  printf( "      -m                        Allows a message to be associated with an exclusion.\n" );
  printf( "                                  The message should describe the reason why a coverage point\n" );
  printf( "                                  is being excluded.  If a coverage point is being included for\n" );
  printf( "                                  coverage (i.e., it was previously excluded from coverage), no\n" );
  printf( "                                  message prompt will be specified.\n" );
  printf( "      -p                        Outputs the status of the exclusion ID and an exclusion message\n" );
  printf( "                                  if one exists.  No excluding will occur if this option is set.\n" );
  printf( "\n" );

}

/*!
 \return Returns TRUE if the help option was parsed.

 \throws anonymous Throw Throw Throw

 Parses the exclude argument list, placing all parsed values into
 global variables.  If an argument is found that is not valid
 for the score operation, an error message is displayed to the
 user.
*/
static bool exclude_parse_args(
  int          argc,      /*!< Number of arguments in argument list argv */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< Argument list passed to this program */
) {

  int  i;
  bool help_found = FALSE;

  i = last_arg + 1;

  while( (i < argc) && !help_found ) {

    if( strncmp( "-h", argv[i], 2 ) == 0 ) {

      exclude_usage();
      help_found = TRUE;

    } else if( strncmp( "-f", argv[i], 2 ) == 0 ) {

      if( check_option_value( argc, argv, i ) ) {
        char**       arg_list = NULL;
        int          arg_num  = 0;
        unsigned int j;
        i++;
        Try {
          read_command_file( argv[i], &arg_list, &arg_num );
          help_found = exclude_parse_args( arg_num, -1, (const char**)arg_list );
        } Catch_anonymous {
          for( j=0; j<arg_num; j++ ) {
            free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
          }
          free_safe( arg_list, (sizeof( char* ) * arg_num) );
          Throw 0;
        }
        for( j=0; j<arg_num; j++ ) {
          free_safe( arg_list[j], (strlen( arg_list[j] ) + 1) );
        }
        free_safe( arg_list, (sizeof( char* ) * arg_num) );
      } else {
        Throw 0;
      }

    } else if( strncmp( "-m", argv[i], 2 ) == 0 ) {

      exclude_prompt_for_msgs = TRUE;

    } else if( strncmp( "-p", argv[i], 2 ) == 0 ) {

      exclude_print = TRUE;

    } else if( strncmp( "-", argv[i], 1 ) == 0 ) {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown exclude option (%s) specified.", argv[i] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else if( (i + 1) == argc ) {

      /* Check to make sure that the user has specified at least one exclusion ID */
      if( excl_ids_head == NULL ) {
        print_output( "At least one exclusion ID must be specified", FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

      if( file_exists( argv[i] ) ) {
        exclude_cdd = strdup_safe( argv[i] );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Specified CDD file (%s) does not exist", argv[i] );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

    } else {

      (void)str_link_add( strdup_safe( argv[i] ), &excl_ids_head, &excl_ids_tail );

    }

    i++;

  }

  return( help_found );

}

/*!
 \return Returns a pointer to the found exclusion reason that matches the given type and ID; otherwise,
         returns a value of NULL.
*/
exclude_reason* exclude_find_exclude_reason(
  char       type,
  int        id,
  func_unit* funit
) { PROFILE(EXCLUDE_FIND_EXCLUDE_REASON);

  exclude_reason* er;  /* Pointer to current exclude reason structure */

  er = funit->er_head;
  while( (er != NULL) && ((er->type != type) || (er->id != id)) ) {
    er = er->next;
  }

  PROFILE_END;

  return( er );

}

/*!
 Writes the given exclude reason to the specified output stream.
*/
void exclude_db_write(
  exclude_reason* er,    /*!< Pointer to exclude reason structure to output */
  FILE*           ofile  /*!< Pointer to output file stream */
) { PROFILE(EXCLUDE_DB_WRITE);

  /*@+longintegral@*/
  fprintf( ofile, "%d %c %d %ld %s\n", DB_TYPE_EXCLUDE, er->type, er->id, er->timestamp, er->reason );
  /*@=longintegral@*/

  PROFILE_END;

}

/*!
 Reads the given exclude reason structure information from the line read from the CDD file.
*/
void exclude_db_read(
  char**     line,       /*!< Pointer to the line read from the CDD file */
  func_unit* curr_funit  /*!< Pointer to the current functional unit */
) { PROFILE(EXCLUDE_DB_READ);

  char   type;        /* Specifies the type of exclusion this structure represents */
  int    id;          /* ID of signal/expression/FSM */
  int    chars_read;  /* Number of characters read from line */
  time_t timestamp;   /* Reason timestamp */

  /*@+longintegral@*/
  if( sscanf( *line, " %c %d %ld%n", &type, &id, &timestamp, &chars_read ) == 3 ) {
  /*@=longintegral@*/

    exclude_reason* er;

    *line = *line + chars_read;

    /* Allocate and initialize the exclude reason structure */
    er            = (exclude_reason*)malloc_safe( sizeof( exclude_reason ) );
    er->type      = type;
    er->id        = id;
    er->reason    = NULL;
    er->timestamp = timestamp;
    er->next      = NULL;

    /* Remove leading whitespace */
    while( (*line)[0] == ' ' ) {
      (*line)++;
    }
    er->reason = strdup_safe( *line );

    /* Add the given exclude reason to the current functional unit list */
    if( curr_funit->er_head == NULL ) {
      curr_funit->er_head = curr_funit->er_tail = er;
    } else {
      curr_funit->er_tail->next = er;
      curr_funit->er_tail       = er;
    }

  } else {

    print_output( "CDD being read is not compatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Resolves an exclusion reason conflict according to the merge option specified by the user.
*/
void exclude_resolve_reason(
  exclude_reason* orig_er,       /*!< Pointer to the original exclusion reason structure */
  func_unit*      orig_funit,    /*!< Pointer to the original functional unit */
  int             resolution,    /*!< Type of resolution to perform */
  const char*     new_reason,    /*!< Reason from new CDD file */
  long            new_timestamp  /*!< Timestamp from new CDD file */
) { PROFILE(EXCLUDE_RESOLVE_REASON);

  unsigned int slen;
  char*        eid;
  char         answer;
  char         c;
 
  switch( resolution ) {

    case MERGE_ER_NONE :
      eid = db_gen_exclusion_id( orig_er->type, orig_er->id );
      printf( "Exclusion reason conflict for %s\n", eid );
      printf( "  Exclusion reason #1:\n" );
      report_output_exclusion_reason( stdout, 4, orig_er->reason, FALSE );
      printf( "  Exclusion reason #2:\n" );
      report_output_exclusion_reason( stdout, 4, new_reason, FALSE );
      do {
        printf( "Choose an exclusion reason to use (none, 1, 2, both, rewrite, abort): " );
        answer = (char)getchar();
        while( ((c = (char)getchar()) != EOF) && (c != '\n') );
      } while( (answer != 'n') && (answer != '1') && (answer != '2') && (answer != 'b') && (answer != 'r') && (answer != 'a') );
      switch( answer ) {
        case 'n' :
          exclude_remove_exclude_reason( orig_er->type, orig_er->id, orig_funit );
          break;
        case '1' :
          /* No need to do anything */
          break;
        case '2' :
          exclude_resolve_reason( orig_er, orig_funit, MERGE_ER_LAST, new_reason, new_timestamp );
          break;
        case 'b' :
          exclude_resolve_reason( orig_er, orig_funit, MERGE_ER_ALL, new_reason, new_timestamp );
          break;
        case 'r' :
          free_safe( orig_er->reason, (strlen( orig_er->reason ) + 1) );
          orig_er->reason = exclude_get_message( eid );
          break;
        case 'a' :
          Throw 0;
          /*@-unreachable@*/
          break;
          /*@=unreachable@*/
      }
      free_safe( eid, (strlen( eid ) + 1) );
      break;

    case MERGE_ER_FIRST :
      /* Do nothing.  The first message is already in the base. */
      break;

    case MERGE_ER_LAST :
      free_safe( orig_er->reason, (strlen( orig_er->reason ) + 1) );
      orig_er->reason = strdup_safe( new_reason );
      break;

    case MERGE_ER_ALL :
      slen = strlen( orig_er->reason ) + 1 + strlen( new_reason ) + 1;
      if( orig_er->reason[strlen( orig_er->reason ) - 1] != '.' ) {
        slen++;
      }
      orig_er->reason = (char*)realloc_safe( orig_er->reason, (strlen( orig_er->reason ) + 1), slen );
      if( orig_er->reason[strlen( orig_er->reason ) - 1] != '.' ) {
        strcat( orig_er->reason, "." );
      }
      strcat( orig_er->reason, " " );
      strcat( orig_er->reason, new_reason );
      break;

    case MERGE_ER_NEW :
      if( orig_er->timestamp < new_timestamp ) {
        free_safe( orig_er->reason, (strlen( orig_er->reason ) + 1) );
        orig_er->reason = strdup_safe( new_reason );
      }
      break;

    case MERGE_ER_OLD :
      if( orig_er->timestamp > new_timestamp ) {
        free_safe( orig_er->reason, (strlen( orig_er->reason ) + 1) );
        orig_er->reason = strdup_safe( new_reason );
      }
      break;

    default :
      assert( 0 );
      break;
  }

  PROFILE_END;

}

/*!
 Reads the given exclude reason structure information from the line read from the CDD file.
*/
void exclude_db_merge(
  func_unit* base,  /*!< Pointer to base functional unit to merge into */
  char**     line   /*!< Pointer to the line read from the CDD file */
) { PROFILE(EXCLUDE_DB_MERGE);

  char   type;        /* Specifies the type of exclusion this structure represents */
  int    id;          /* ID of signal/expression/FSM */
  int    chars_read;  /* Number of characters read from line */
  time_t timestamp;   /* Reason timestamp */
  char*  reason;      /* Pointer to the exclusion reason from the CDD file */

  /*@+longintegral@*/
  if( sscanf( *line, " %c %d %ld%n", &type, &id, &timestamp, &chars_read ) == 3 ) {
  /*@=longintegral@*/

    exclude_reason* er;

    *line = *line + chars_read;

    /* Get the reason string and remove leading whitespace */
    while( (*line)[0] == ' ' ) {
      (*line)++;
    }
    reason = *line;
    assert( reason != NULL );
    assert( reason[0] != '\0' );

    /* If the exclusion reason does not exist in the base CDD, go ahead and add it */
    if( (er = exclude_find_exclude_reason( type, id, base )) == NULL ) {

      /* Allocate and initialize the exclude reason structure */
      er            = (exclude_reason*)malloc_safe( sizeof( exclude_reason ) );
      er->type      = type;
      er->id        = id;
      er->timestamp = timestamp;
      er->reason    = strdup_safe( reason );
      er->next      = NULL;

      /* Add the given exclude reason to the current functional unit list */
      if( base->er_head == NULL ) {
        base->er_head = base->er_tail = er;
      } else {
        base->er_tail->next = er;
        base->er_tail       = er;
      }
 
    /* Otherwise, if the exclusion reason does exist, check for a conflict and handle it */
    } else {

      /*
       If the exclusion reason string does not match the current string, resolve the conflict appropriately
       (otherwise, just use the reason in the base functional unit).
      */
      if( strcmp( er->reason, reason ) != 0 ) {
        exclude_resolve_reason( er, base, merge_er_value, reason, timestamp );
      }

    }

  } else {

    print_output( "CDD being read is not compatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Reads the given exclude reason structure information from the line read from the CDD file.
*/
void exclude_merge(
  func_unit*      base,  /*!< Pointer to base functional unit to merge into */
  exclude_reason* er     /*!< Pointer to exclusion reason to merge */
) { PROFILE(EXCLUDE_MERGE);

  exclude_reason* found_er;

  /* If the exclusion reason does not exist in the base CDD, go ahead and add it */
  if( (found_er = exclude_find_exclude_reason( er->type, er->id, base )) == NULL ) {

    exclude_reason* new_er;
 
    /* Allocate and initialize the exclude reason structure */
    new_er            = (exclude_reason*)malloc_safe( sizeof( exclude_reason ) );
    new_er->type      = er->type;
    new_er->id        = er->id;
    new_er->timestamp = er->timestamp;
    new_er->reason    = strdup_safe( er->reason );
    new_er->next      = NULL;

    /* Add the given exclude reason to the current functional unit list */
    if( base->er_head == NULL ) {
      base->er_head = base->er_tail = new_er;
    } else {
      base->er_tail->next = new_er;
      base->er_tail       = new_er;
    }

  /* Otherwise, if the exclusion reason does exist, check for a conflict and handle it */
  } else {

    /*
     If the exclusion reason string does not match the current string, resolve the conflict appropriately
     (otherwise, just use the reason in the base functional unit).
    */
    if( strcmp( found_er->reason, er->reason ) != 0 ) {
      exclude_resolve_reason( found_er, base, merge_er_value, er->reason, er->timestamp );
    }

  }

  PROFILE_END;

}

/*!
 \return Returns pointer to found signal if it was found; otherwise, returns NULL.
*/
vsignal* exclude_find_signal(
            int         id,          /*!< Exclusion ID to search for */
  /*@out@*/ func_unit** found_funit  /*!< Pointer to functional unit containing found signal */
) { PROFILE(EXCLUDE_FIND_SIGNAL);

  inst_link* instl;       /* Pointer to current instance link */
  vsignal*   sig = NULL;  /* Pointer to found signal */

  instl = db_list[curr_db]->inst_head;
  while( (instl != NULL) && ((sig = instance_find_signal_by_exclusion_id( instl->inst, id, found_funit )) == NULL) ) {
    instl = instl->next;
  }

  if( sig != NULL ) {
    *found_funit = funit_get_curr_module( *found_funit );
  }

  PROFILE_END;

  return( sig );

}

/*!
 \return Returns pointer to found expression if it was found; otherwise, returns NULL.
*/
expression* exclude_find_expression(
            int         id,          /*!< Exclusion ID to search for */
  /*@out@*/ func_unit** found_funit  /*!< Pointer to functional unit containing found expression */
) { PROFILE(EXCLUDE_FIND_EXPRESSION);

  inst_link*  instl;       /* Pointer to current instance link */
  expression* exp = NULL;  /* Pointer to found expression */

  instl = db_list[curr_db]->inst_head;
  while( (instl != NULL) && ((exp = instance_find_expression_by_exclusion_id( instl->inst, id, found_funit )) == NULL) ) {
    instl = instl->next;
  }

  if( exp != NULL ) {
    *found_funit = funit_get_curr_module( *found_funit );
  }

  PROFILE_END;

  return( exp );

}

/*!
 \return Returns the index of the state transition that was found with the given exclusion ID.
*/
int exclude_find_fsm_arc(
            int         id,          /*!< Exclusion ID to search for */
  /*@out@*/ fsm_table** found_fsm,   /*!< Pointer to FSM table that was found */
  /*@out@*/ func_unit** found_funit  /*!< Pointer to found functional unit */
) { PROFILE(EXCLUDE_FIND_FSM_ARC);

  inst_link* instl;  /* Pointer to current instance link */
  int        arc_index = -1;

  instl = db_list[curr_db]->inst_head;
  while( (instl != NULL) && ((arc_index = instance_find_fsm_arc_index_by_exclusion_id( instl->inst, id, found_fsm, found_funit )) == -1) ) {
    instl = instl->next;
  }

  if( arc_index != -1 ) {
    *found_funit = funit_get_curr_module( *found_funit );
  }

  PROFILE_END;

  return( arc_index );

}

/*!
 \return Returns the reformatted string that removes all newlines and extra spaces.
*/
char* exclude_format_reason(
  const char* old_str  /*!< Pointer to string that needs to be formatted */
) { PROFILE(EXCLUDE_FORMAT_REASON);

  char*        msg;                  /* Pointer to the reformatted message */
  unsigned int msg_size;             /* Current size of message array */
  char         c;                    /* Current character */
  bool         sp_just_seen = TRUE;  /* Set to TRUE if a space character was just seen */
  char         str[100];             /* Temporary string */
  unsigned int i;                    /* Loop iterator */
  unsigned int index        = 0;     /* Index into str array to store next character */

  msg      = strdup_safe( "" );
  msg_size = 1;
  str[0]   = '\0';

  if( old_str != NULL ) {

    for( i=0; i<strlen( old_str ); i++ ) {

      c = old_str[i];

      /* Convert any formatting characters to spaces */
      c = ((c == '\n') || (c == '\t') || (c == '\r')) ? ' ' : c;

      /* If the user has specified multiple formatting characters together, ignore all but the first. */
      if( (c != ' ') || !sp_just_seen ) {
        sp_just_seen = (c == ' ') ? TRUE : FALSE;
        str[(index % 99) + 0] = c;
        str[(index % 99) + 1] = '\0';
        if( ((index + 1) % 99) == 0 ) {
          msg = (char*)realloc_safe( msg, msg_size, (msg_size + strlen( str )) );
          msg_size += strlen( str );
          strcat( msg, str );
          str[0] = '\0';
        }
        index++;
      }

    }

  }

  /* Take what's left in the str array and put it into the msg array */
  if( strlen( str ) > 0 ) {
    msg = (char*)realloc_safe( msg, msg_size, (msg_size + strlen( str )) );
    strcat( msg, str );
    msg[strlen(msg)] = '\0';
  }

  PROFILE_END;

  return( msg );

} 

/*!
 \return Returns the message specified by the user.
*/
static char* exclude_get_message(
  const char* eid  /*!< Exclusion ID to get message for */
) { PROFILE(EXCLUDED_GET_MESSAGE);

  char*        msg;                  /* Pointer to the message from the user */
  unsigned int msg_size;             /* The current size of the specified message */
  char         c;                    /* Current character */
  bool         nl_just_seen = TRUE;  /* Set to TRUE if a newline was just seen */
  bool         sp_just_seen = TRUE;  /* Set to TRUE if a space character was just seen */
  int          index        = 0;     /* Current string index */
  char         str[100];
  char*        formatted_msg;        /* Formatted message */
  unsigned int rv;

  rv = snprintf( user_msg, USER_MSG_LENGTH, "Please specify a reason for exclusion for exclusion ID %s (Enter a '.' (period) on a newline to end):\n", eid );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, NORMAL, __FILE__, __LINE__ );

  msg      = strdup_safe( "" );
  msg_size = 1;
  str[0]   = '\0';

  while( ((c = (char)getchar()) != EOF) && ((c != '.') || !nl_just_seen) ) {

    /* Mark if we have just seen a newline (for the purposes of determining if the user has completed input) */
    nl_just_seen = (c == '\n') ? TRUE : FALSE;

    /* Place the read character into the temporary string */
    str[(index % 99) + 0] = c;
    str[(index % 99) + 1] = '\0';

    /*
     If the temporary string has been filled, realloc the msg and append the contents of the the temporary array to this newly
     allocated array.
    */
    if( ((index + 1) % 99) == 0 ) {
      msg = (char*)realloc_safe( msg, msg_size, (msg_size + strlen( str )) );
      msg_size += strlen( str );
      strcat( msg, str );
    }

    index++;

  }

  if( strlen( str ) > 0 ) {
    msg = (char*)realloc_safe( msg, msg_size, (msg_size + strlen( str )) );
    strcat( msg, str );
    msg[strlen(msg)-1] = '\0';
  }

  print_output( "", NORMAL, __FILE__, __LINE__ );

  /* Now reformat the message */
  formatted_msg = exclude_format_reason( msg );

  free_safe( msg, (strlen( msg ) + ((strlen( str ) > 0) ? 2 : 1)) );

  PROFILE_END;

  return( formatted_msg );

}

/*!
 Handle the creation/deallocation of the exclude reason structure.
*/
static void exclude_handle_exclude_reason(
  int         prev_excluded,  /*!< Specifies if the coverage point was previously excluded or not */
  const char* id,             /*!< Exclusion ID */
  func_unit*  funit           /*!< Functional unit containing sig */
) { PROFILE(EXCLUDE_HANDLE_EXCLUDE_REASON);

  /*
   If the coverage point was not previously excluded, allow the user to specify a reason and
   store this information in the functional unit.
  */
  if( prev_excluded == 0 ) { 

    char* str = exclude_get_message( id );

    if( (str != NULL) && (strlen( str ) > 0) ) {
      exclude_add_exclude_reason( id[0], atoi( id + 1 ), str, funit );
    } else {
      free_safe( str, (strlen( str ) + 1) );
    }

  /*
   If the coverage point was previously excluded, attempt to find the matching exclusion reason, and, if
   it is found, remove it from the list.
  */
  } else {

    exclude_remove_exclude_reason( id[0], atoi( id + 1 ), funit );

  }

  PROFILE_END;

}

/*!
 Prints the exclusion information to standard output.
*/
static void exclude_print_exclusion(
  const char* id,        /*!< Exclusion ID to output */
  int         excluded,  /*!< Specifies the current value of the exclusion is exclude (1) or include (0) */
  func_unit*  funit      /*!< Pointer to functional unit containing the excluded structure */
) { PROFILE(EXCLUDE_PRINT_EXCLUSION);

  printf( "Exclusion ID: %s, Status: %s\n", id, ((excluded == 1) ? "EXCLUDED" : "INCLUDED") );

  /* Only output an exclusion message if we are currently excluded */
  if( excluded == 1 ) {

    exclude_reason* er = exclude_find_exclude_reason( id[0], atoi( id + 1 ), funit );

    if( er != NULL ) {
      report_output_exclusion_reason( stdout, 2, er->reason, FALSE );
    } else {
      report_output_exclusion_reason( stdout, 2, "No exclusion information has been specified.", FALSE );
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the exclusion ID was found and the exclusion applied; otherwise, returns FALSE.

 Finds the line that matches the given exclusion ID and toggles its exclusion value, providing a reason
 for exclusion if it is excluding the coverage point and the -m option was specified on the command-line.
*/
static bool exclude_line_from_id(
  const char* id  /*!< String version of exclusion ID */
) { PROFILE(EXCLUDE_LINE_FROM_ID);

  expression* exp;          /* Pointer to found expression */
  func_unit*  found_funit;  /* Pointer to functional unit containing found expression */

  if( (exp = exclude_find_expression( atoi( id + 1 ), &found_funit )) != NULL ) {

    int          prev_excluded;
    unsigned int rv;

    assert( ESUPPL_IS_ROOT( exp->suppl ) == 1 );

    /* Get the previously excluded value */
    prev_excluded = exp->parent->stmt->suppl.part.excluded;

    /* If the user wants to print the information do so */
    if( exclude_print ) {

      exclude_print_exclusion( id, prev_excluded, found_funit );

    /* Otherwise, perform the exclusion */
    } else {

      /* Output result */
      if( prev_excluded == 0 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Excluding %s", id );
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Including %s", id );
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
  
      /* Set the exclude bits in the expression supplemental field */
      exp->suppl.part.excluded               = (prev_excluded ^ 1);
      exp->parent->stmt->suppl.part.excluded = (prev_excluded ^ 1);

      /* If we are excluding and the -m option was specified, get an exclusion reason from the user */
      if( exclude_prompt_for_msgs || (prev_excluded == 1) ) {
        exclude_handle_exclude_reason( prev_excluded, id, found_funit );
      }

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find line associated with exclusion ID %s", id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  PROFILE_END;

  return( (exp != NULL) && !exclude_print );

}

/*!
 \return Returns TRUE if the exclusion ID was found and the exclusion applied; otherwise, returns FALSE.

 Finds the signal that matches the given exclusion ID and toggles its exclusion value, providing a reason
 for exclusion if it is excluding the coverage point and the -m option was specified on the command-line.
*/
static bool exclude_toggle_from_id(
  const char* id  /*!< String version of exclusion ID */
) { PROFILE(EXCLUDE_TOGGLE_FROM_ID);

  vsignal*   sig;          /* Pointer to found signal */
  func_unit* found_funit;  /* Pointer to functional unit containing sig */
  
  if( (sig = exclude_find_signal( atoi( id + 1 ), &found_funit )) != NULL ) {
  
    int          prev_excluded = sig->suppl.part.excluded;
    unsigned int rv;

    /* If the user wants to print the information do so */
    if( exclude_print ) {

      exclude_print_exclusion( id, prev_excluded, found_funit );

    /* Otherwise, perform the exclusion */
    } else {

      /* Output result */
      if( prev_excluded == 0 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Excluding %s", id );
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Including %s", id );
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );
    
      /* Set the exclude bits in the expression supplemental field */
      sig->suppl.part.excluded = (prev_excluded ^ 1);
    
      /* If we are excluding and the -m option was specified, get an exclusion reason from the user */
      if( exclude_prompt_for_msgs || (prev_excluded == 1) ) { 
        exclude_handle_exclude_reason( prev_excluded, id, found_funit );
      }

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find toggle signal associated with exclusion ID %s", id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  PROFILE_END;

  return( (sig != NULL) && !exclude_print );

}

/*!
 \return Returns TRUE if the exclusion ID was found and the exclusion applied; otherwise, returns FALSE.

 Finds the memory that matches the given exclusion ID and toggles its exclusion value, providing a reason
 for exclusion if it is excluding the coverage point and the -m option was specified on the command-line.
*/
static bool exclude_memory_from_id(
  const char* id  /*!< String version of exclusion ID */
) { PROFILE(EXCLUDE_MEMORY_FROM_ID);

  vsignal*   sig;          /* Pointer to found signal */
  func_unit* found_funit;  /* Pointer to functional unit containing sig */
  
  if( (sig = exclude_find_signal( atoi( id + 1 ), &found_funit )) != NULL ) {
  
    int          prev_excluded = sig->suppl.part.excluded;
    unsigned int rv;
    
    /* If the user has specified to print the exclusion, do so */
    if( exclude_print ) {

      exclude_print_exclusion( id, prev_excluded, found_funit );

    /* Otherwise, perform the exclusion */
    } else {

      /* Output result */
      if( prev_excluded == 0 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Excluding %s", id );
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Including %s", id );
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );

      /* Set the exclude bits in the expression supplemental field */
      sig->suppl.part.excluded = (prev_excluded ^ 1);
   
      /* If we are excluding and the -m option was specified, get an exclusion reason from the user */
      if( exclude_prompt_for_msgs || (prev_excluded == 1) ) {
        exclude_handle_exclude_reason( prev_excluded, id, found_funit );
      }

    }
  
  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find memory associated with exclusion ID %s", id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  PROFILE_END;

  return( (sig != NULL) && !exclude_print );

}

/*!
 \return Returns TRUE if the exclusion ID was found and the exclusion applied; otherwise, returns FALSE.

 Finds the expression that matches the given exclusion ID and toggles its exclusion value, providing a reason
 for exclusion if it is excluding the coverage point and the -m option was specified on the command-line.
*/
static bool exclude_expr_from_id(
  const char* id  /*!< String version of exclusion ID */
) { PROFILE(EXCLUDE_EXPR_FROM_ID);

  expression* exp;          /* Pointer to found expression */
  func_unit*  found_funit;  /* Pointer to functional unit containing exp */
  
  if( (exp = exclude_find_expression( atoi( id + 1 ), &found_funit )) != NULL ) {
  
    int          prev_excluded;
    unsigned int rv;
    
    /* Get the previously excluded value */
    prev_excluded = exp->suppl.part.excluded;

    /* If the user has specified to print the exclusion, do so */
    if( exclude_print ) {

      exclude_print_exclusion( id, prev_excluded, found_funit );

    /* Otherwise, perform the exclusion */
    } else {
    
      /* Output result */
      if( prev_excluded == 0 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Excluding %s", id );
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Including %s", id );
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );

      /* Set the exclude bits in the expression supplemental field */
      exp->suppl.part.excluded = (prev_excluded ^ 1);
    
      /* If we are excluding and the -m option was specified, get an exclusion reason from the user */
      if( exclude_prompt_for_msgs || (prev_excluded == 1) ) { 
        exclude_handle_exclude_reason( prev_excluded, id, found_funit );
      }

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find expression associated with exclusion ID %s", id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  PROFILE_END;

  return( (exp != NULL) && !exclude_print );

}

/*!
 \return Returns TRUE if the exclusion ID was found and the exclusion applied; otherwise, returns FALSE.

 Finds the FSM that matches the given exclusion ID and toggles its exclusion value, providing a reason
 for exclusion if it is excluding the coverage point and the -m option was specified on the command-line.
*/
static bool exclude_fsm_from_id(
  const char* id  /*!< String version of exclusion ID */
) { PROFILE(EXCLUDE_FSM_FROM_ID);

  int        arc_index;    /* Index of found state transition in arcs array */
  fsm_table* found_fsm;    /* Pointer to found FSM structure */
  func_unit* found_funit;  /* Pointer to functional unit containing arc */

  if( (arc_index = exclude_find_fsm_arc( atoi( id + 1 ), &found_fsm, &found_funit )) != -1 ) {

    int         prev_excluded;
    unsigned int rv;

    /* Get the previously excluded value */
    prev_excluded = found_fsm->arcs[arc_index]->suppl.part.excluded;

    /* If the user has specified to print the exclusion, do so */
    if( exclude_print ) {

      exclude_print_exclusion( id, prev_excluded, found_funit );

    /* Otherwise, perform the exclusion */
    } else {

      /* Output result */
      if( prev_excluded == 0 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Excluding %s", id );
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Including %s", id );
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );

      /* Toggle the exclude bit */
      found_fsm->arcs[arc_index]->suppl.part.excluded = (prev_excluded ^ 1);

      /* If we are excluding and the -m option was specified, get an exclusion reason from the user */
      if( exclude_prompt_for_msgs || (prev_excluded == 1) ) {
        exclude_handle_exclude_reason( prev_excluded, id, found_funit );
      }

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find FSM arc associated with exclusion ID %s", id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  PROFILE_END;

  return( (arc_index != -1) && !exclude_print );

}

/*!
 \return Returns TRUE if the exclusion ID was found and the exclusion applied; otherwise, returns FALSE.

 Finds the assertion that matches the given exclusion ID and toggles its exclusion value, providing a reason
 for exclusion if it is excluding the coverage point and the -m option was specified on the command-line.
*/
static bool exclude_assert_from_id(
  const char* id  /*!< String version of exclusion ID */
) { PROFILE(EXCLUDE_ASSERT_FROM_ID);

  expression* exp;          /* Pointer to found expression */
  func_unit*  found_funit;  /* Pointer to functional unit containing exp */

  if( (exp = exclude_find_expression( atoi( id + 1 ), &found_funit )) != NULL ) {

    int          prev_excluded;
    unsigned int rv;

    /* Get the previously excluded value */
    prev_excluded = exp->suppl.part.excluded;

    /* If the user has specified to print the exclusion, do so */
    if( exclude_print ) {

      exclude_print_exclusion( id, prev_excluded, found_funit );

    /* Otherwise, perform the exclusion */
    } else {

      /* Output result */
      if( prev_excluded == 0 ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Excluding %s", id );
      } else {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "  Including %s", id );
      }
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );

      /* Set the exclude bits in the expression supplemental field */
      exp->suppl.part.excluded = (prev_excluded ^ 1);
  
      /* If we are excluding and the -m option was specified, get an exclusion reason from the user */
      if( exclude_prompt_for_msgs || (prev_excluded == 1) ) {
        exclude_handle_exclude_reason( prev_excluded, id, found_funit );
      }

    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find assertion associated with exclusion ID %s", id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  PROFILE_END;

  return( (exp != NULL) && !exclude_print );

}

/*!
 \return Returns TRUE if one or more exclusion IDs were applied; otherwise, returns FALSE.

 Applies the user-specified exclusion IDs to the currently opened database.
*/
static bool exclude_apply_exclusions() { PROFILE(EXCLUDE_APPLY_EXCLUSIONS);

  bool         retval = FALSE;  /* Return value for this function */
  str_link*    strl;            /* Pointer to current string link */
  unsigned int rv;

  strl = excl_ids_head;
  while( strl != NULL ) {
    switch( strl->str[0] ) {
      case 'L' :  retval |= exclude_line_from_id( strl->str );    break;
      case 'T' :  retval |= exclude_toggle_from_id( strl->str );  break;
      case 'M' :  retval |= exclude_memory_from_id( strl->str );  break;
      case 'E' :  retval |= exclude_expr_from_id( strl->str );    break;
      case 'F' :  retval |= exclude_fsm_from_id( strl->str );     break;
      case 'A' :  retval |= exclude_assert_from_id( strl->str );  break;
      default  :
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal exclusion identifier specified (%s)", strl->str );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
        /*@-unreachable@*/
        break;
        /*@=unreachable@*/
    }
    strl = strl->next;
  }

  PROFILE_END;

  return( retval );

}

/*!
 Performs the exclude command.
*/
void command_exclude(
  int          argc,      /*!< Number of arguments in command-line to parse */
  int          last_arg,  /*!< Index of last parsed argument from list */
  const char** argv       /*!< List of arguments from command-line to parse */
) { PROFILE(COMMAND_EXCLUDE);

  unsigned int   rv;
  comp_cdd_cov** comp_cdds    = NULL;
  unsigned int   comp_cdd_num = 0;
  bool           error        = FALSE;

  /* Output header information */
  rv = snprintf( user_msg, USER_MSG_LENGTH, COVERED_HEADER );
  assert( rv < USER_MSG_LENGTH );
  print_output( user_msg, HEADER, __FILE__, __LINE__ );

  Try {

    unsigned int rv;

    /* Parse score command-line */
    if( !exclude_parse_args( argc, last_arg, argv ) ) {

      /* Read in database */
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Reading CDD file \"%s\"", exclude_cdd );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, NORMAL, __FILE__, __LINE__ );

      (void)db_read( exclude_cdd, READ_MODE_REPORT_NO_MERGE );
      bind_perform( TRUE, 0 );

      /* Apply the specified exclusion IDs */
      if( exclude_apply_exclusions() ) {
        rv = snprintf( user_msg, USER_MSG_LENGTH, "Writing CDD file \"%s\"", exclude_cdd );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, NORMAL, __FILE__, __LINE__ );
        db_write( exclude_cdd, FALSE, FALSE );
      }

    }

  } Catch_anonymous {
    error = TRUE;
  }

  /* Close down the database */
  db_close();
    
  /* Deallocate other allocated variables */
  str_link_delete_list( excl_ids_head );
  free_safe( exclude_cdd, (strlen( exclude_cdd ) + 1) );

  if( error ) {
    Throw 0;
  }
      
  PROFILE_END;
 
}   

