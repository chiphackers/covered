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
 \file     comb.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 
 \par
 The functions in this file are used by the report command to calculate and display all 
 report output for combination logic coverage.  Combinational logic coverage is calculated
 solely from the list of expression trees in the scored design.  For each module or instance,
 the expression list is passed to the calculation routine which iterates through each
 expression tree, tallying the total number of expression values and the total number of
 expression values reached.
 
 \par
 Every expression contains two possible expression values during simulation:  0 and 1.
 If an expression evaluated to some unknown value, this is not recorded by Covered.
 If an expression has evaluated to 0, the WAS_FALSE bit of the expression's supplemental
 field will be set.  If the expression has evaluated to 1 or a value greater than 1, the
 WAS_TRUE bit of the expression's supplemental field will be set.  If both the WAS_FALSE and
 the WAS_TRUE bits are set after scoring, the expression is considered to be fully covered.

 \par
 If the expression is an event type, only the WAS_TRUE bit is examined.  It it was set during
 simulation, the event is completely covered; otherwise, the event was not covered during simulation.

 \par
 For combinational logic, four other expression supplemental bits are used to determine which logical
 combinations of its two children have occurred during simulation.  These four bits are EVAL_A, EVAL_B,
 EVAL_C, and EVAL_D.  If the left and right expressions simultaneously evaluated to 0 during simulation,
 the EVAL_00 bit is set.  If the left and right expressions simultaneously evaluated to 0 and 1, respectively,
 the EVAL_01 bit is set.  If the left and right expressions simultaneously evaluated to 1 and 0, respectively,
 the EVAL_10 bit is set.  If the left and right expression simultaneously evaluated to 1 during simulation,
 the EVAL_11 bit is set.  For an AND-type operation, full coverage is achieved if (EVAL_00 || EVAL_01) &&
 (EVAL_00 || EVAL10) && EVAL_11.  For an OR-type operation, full coverage is achieved if (EVAL_10 || EVAL_11) &&
 (EVAL_01 || EVAL11) && EVAL_00.  For any other combinational expression (where both the left and right
 expression trees are non-NULL), full coverage is achieved if all four EVAL_xx bits are set.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "codegen.h"
#include "comb.h"
#include "db.h"
#include "defines.h"
#include "exclude.h"
#include "expr.h"
#include "func_iter.h"
#include "func_unit.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "report.h"
#include "util.h"
#include "vector.h"


extern db**           db_list;
extern unsigned int   curr_db;
extern bool           report_covered;
extern unsigned int   report_comb_depth;
extern bool           report_instance;
extern bool           report_bitwise;
extern int            line_width;
extern char           user_msg[USER_MSG_LENGTH];
extern const exp_info exp_op_info[EXP_OP_NUM];
extern isuppl         info_suppl;
extern bool           report_exclusions;
extern bool           flag_output_exclusion_ids;
extern unsigned int   inline_comb_depth;


/*!
 Controls whether multi-expressions are used or not.
*/
bool allow_multi_expr = TRUE;


/*!
 \return Returns new depth value for specified child expression.

 Based on the current point in the expression tree, calculates the left or
 right child's curr_depth value.
*/
static int combination_calc_depth(
  expression*  exp,         /*!< Pointer to current expression */
  unsigned int curr_depth,  /*!< Current depth in expression tree */
  bool         left         /*!< TRUE if evaluating for left child */
) { PROFILE(COMBINATION_CALC_DEPTH);

  int our_depth;  /* Return value for this function */

  if( (!info_suppl.part.inlined || ((curr_depth + 1) <= inline_comb_depth)) &&
      (((report_comb_depth == REPORT_DETAILED) && ((curr_depth + 1) <= report_comb_depth)) ||
       (report_comb_depth == REPORT_VERBOSE)) ) {

    if( left ) {

      if( (exp->left != NULL) && (exp->op == exp->left->op) ) {
        our_depth = curr_depth;
      } else {
        our_depth = curr_depth + 1;
      }

    } else {

      if( (exp->right != NULL) && (exp->op == exp->right->op) ) {
        our_depth = curr_depth;
      } else {
        our_depth = curr_depth + 1;
      }

    }

  } else {

    our_depth = curr_depth + 1;

  }

  PROFILE_END;

  return( our_depth );

}

/*!
 \return Returns TRUE if the given expression multi-expression tree needs to be underlined
*/
static bool combination_does_multi_exp_need_ul(
  expression* exp  /*!< Pointer to expression to check for multi-expression underlines */
) { PROFILE(COMBINATION_DOES_MULTI_EXP_NEED_UL);

  bool ul = FALSE;  /* Return value for this function */
  bool and_op;      /* Specifies if current expression is an AND or LAND operation */

  if( exp != NULL ) {

    /* Figure out if this is an AND/LAND operation */
    and_op = (exp->op == EXP_OP_AND) || (exp->op == EXP_OP_LAND);

    /* Decide if our expression requires that this sequence gets underlined */
    if( and_op ) {
      ul = (exp->suppl.part.eval_11 == 0) || (exp->suppl.part.eval_01 == 0) || (exp->suppl.part.eval_10 == 0);
    } else {
      ul = (exp->suppl.part.eval_00 == 0) || (exp->suppl.part.eval_10 == 0) || (exp->suppl.part.eval_01 == 0);
    }

    /* If we did not require underlining, check our left child */
    if( !ul && ((exp->left == NULL) || (exp->op == exp->left->op)) ) {
      ul = combination_does_multi_exp_need_ul( exp->left );
    }

    /* If we still have not found a reason to underline, check our right child */
    if( !ul && ((exp->right == NULL) || (exp->op == exp->right->op)) ) {
      ul = combination_does_multi_exp_need_ul( exp->right );
    }

  }

  PROFILE_END;

  return( ul );

}

/*!
 Parses the specified expression tree, calculating the hit and total values of all
 sub-expressions that are the same operation types as their left children.
*/
static void combination_multi_expr_calc(
            expression*   exp,       /*!< Pointer to expression to calculate hit and total of multi-expression subtrees */
            int*          ulid,      /*!< Pointer to current underline ID */
            bool          ul,        /*!< If TRUE, parent expressions were found to be missing so force the underline */
            bool          excluded,  /*!< If TRUE, parent expressions were found to be excluded */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to value containing number of hit expression values in this expression */
  /*@out@*/ unsigned int* excludes,  /*!< Pointer to value containing number of excluded combinational expressions */
  /*@out@*/ unsigned int* total      /*!< Pointer to value containing total number of expression values in this expression */
) { PROFILE(COMBINATION_MULTI_EXPR_CALC);

  bool and_op;  /* Specifies if current expression is an AND or LAND operation */

  if( exp != NULL ) {

    /* Calculate this expression's exclusion */
    excluded |= ESUPPL_EXCLUDED( exp->suppl );

    /* Figure out if this is an AND/LAND operation */
    and_op = (exp->op == EXP_OP_AND) || (exp->op == EXP_OP_LAND);

    /* Decide if our expression requires that this sequence gets underlined */
    if( !ul ) {
      ul = combination_does_multi_exp_need_ul( exp );
    }

    if( (exp->left != NULL) && (exp->op != exp->left->op) ) {
      if( excluded ) {
        (*hit)++;
        (*excludes)++;
      } else {
        if( and_op ) {
          *hit += exp->suppl.part.eval_01;
        } else {
          *hit += exp->suppl.part.eval_10;
        }
      }
      if( (exp->left->ulid == -1) && ul ) { 
        exp->left->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    } else {
      combination_multi_expr_calc( exp->left, ulid, ul, excluded, hit, excludes, total );
    }

    if( (exp->right != NULL) && (exp->op != exp->right->op) ) {
      if( excluded ) {
        (*hit)++;
        (*excludes)++;
      } else {
        if( and_op ) {
          *hit += exp->suppl.part.eval_10;
        } else {
          *hit += exp->suppl.part.eval_01;
        }
      }
      if( (exp->right->ulid == -1) && ul ) {
        exp->right->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    } else {
      combination_multi_expr_calc( exp->right, ulid, ul, excluded, hit, excludes, total );
    }

    if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ) {
      if( excluded ) {
        (*hit)++;
        (*excludes)++;
      } else {
        if( and_op ) {
          *hit += exp->suppl.part.eval_11;
        } else {
          *hit += exp->suppl.part.eval_00;
        }
      }
      if( (exp->ulid == -1) && ul ) {
        exp->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the specified expression is part of a multi-value expression tree;
         otherwise, returns FALSE.

 Checks the specified expression to see if it is a part of a multi-value expression
 tree.  If the expression is part of a tree, returns a value of TRUE; otherwise, returns
 a value of FALSE.  This function is used when determining if a non-multi-value expression
 should be assigned an underline ID (it should if the expression is not part of a multi-value
 expression tree) or be assigned one later (if the expression is part of a multi-value
 expression tree -- the ID will be assigned to it when the multi-value expression tree is
 assigned underline IDs.
*/
static bool combination_is_expr_multi_node(
  expression* exp  /*!< Pointer to expression to evaluate */
) { PROFILE(COMBINATION_IS_EXPR_MULTI_NODE);

  bool retval = (exp != NULL) &&
                (ESUPPL_IS_ROOT( exp->suppl ) == 0) && 
                (exp->parent->expr->left  != NULL) &&
                (exp->parent->expr->right != NULL) &&
                ( ( (exp->parent->expr->right->id == exp->id) &&
                    (exp->parent->expr->left->ulid == -1) ) ||
                  (exp->parent->expr->left->id == exp->id) ) &&
                ( (exp->parent->expr->op == EXP_OP_AND)  ||
                  (exp->parent->expr->op == EXP_OP_LAND) ||
                  (exp->parent->expr->op == EXP_OP_OR)   ||
                  (exp->parent->expr->op == EXP_OP_LOR) ) &&
                ( ( (ESUPPL_IS_ROOT( exp->parent->expr->suppl ) == 0) &&
                    (exp->parent->expr->op == exp->parent->expr->parent->expr->op) ) ||
                  (exp->parent->expr->left->op == exp->parent->expr->op) );

  PROFILE_END;

  return( retval );

}

/*!
 Recursively traverses the specified expression tree, recording the total number
 of logical combinations in the expression list and the number of combinations
 hit during the course of simulation.  An expression can be considered for
 combinational coverage if the "measured" bit is set in the expression.
*/
void combination_get_tree_stats(
            expression*   exp,         /*!< Pointer to expression tree to traverse */
            bool          rpt_comb,    /*!< Report combinational coverage */
            bool          rpt_event,   /*!< Report event coverage */
  /*@out@*/ int*          ulid,        /*!< Pointer to current underline ID */
            unsigned int  curr_depth,  /*!< Current search depth in given expression tree */
            bool          excluded,    /*!< Specifies that this expression should be excluded for hit information because one
                                            or more of its parent expressions have been excluded */
  /*@out@*/ unsigned int* hit,         /*!< Pointer to number of logical combinations hit during simulation */
  /*@out@*/ unsigned int* excludes,    /*!< Pointer to number of excluded logical combinations */
  /*@out@*/ unsigned int* total        /*!< Pointer to total number of logical combinations */
) { PROFILE(COMBINATION_GET_TREE_STATS);

  int num_hit = 0;  /* Number of expression value hits for the current expression */
  int tot_num;      /* Total number of combinations for the current expression */

  if( exp != NULL ) {

    /* Calculate excluded value for this expression */
    excluded |= ESUPPL_EXCLUDED( exp->suppl );

    /* Calculate children */
    combination_get_tree_stats( exp->left,  rpt_comb, rpt_event, ulid, combination_calc_depth( exp, curr_depth, TRUE ),  excluded, hit, excludes, total );
    combination_get_tree_stats( exp->right, rpt_comb, rpt_event, ulid, combination_calc_depth( exp, curr_depth, FALSE ), excluded, hit, excludes, total );

    if( (!info_suppl.part.inlined || (curr_depth <= inline_comb_depth)) &&
        (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
         (report_comb_depth == REPORT_VERBOSE) ||
         (report_comb_depth == REPORT_SUMMARY)) ) {

      if( (EXPR_IS_MEASURABLE( exp ) == 1) && (ESUPPL_WAS_COMB_COUNTED( exp->suppl ) == 0) ) {

        if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ||
            ((exp->op != EXP_OP_AND) &&
             (exp->op != EXP_OP_LAND) &&
             (exp->op != EXP_OP_OR)   &&
             (exp->op != EXP_OP_LOR)) ||
             !allow_multi_expr ) {

          /* Calculate current expression combination coverage */
          if( (((exp->left != NULL) &&
                (exp->op == exp->left->op)) ||
               ((exp->right != NULL) &&
                (exp->op == exp->right->op))) &&
              ((exp->op == EXP_OP_AND)  ||
               (exp->op == EXP_OP_OR)   ||
               (exp->op == EXP_OP_LAND) ||
               (exp->op == EXP_OP_LOR)) && allow_multi_expr ) {
            if( (info_suppl.part.scored_comb == 1) && rpt_comb ) {
              combination_multi_expr_calc( exp, ulid, FALSE, excluded, hit, excludes, total );
            }
          } else {
            if( !expression_is_static_only( exp ) ) {
              if( EXPR_IS_COMB( exp ) == 1 ) {
                if( (info_suppl.part.scored_comb == 1) && rpt_comb ) {
                  if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {
                    if( report_bitwise ) {
                      tot_num = 3 * exp->value->width;
                      num_hit = vector_get_eval_abc_count( exp->value );
                    } else {
                      tot_num = 3;
                      num_hit = exp->suppl.part.eval_01 +
                                exp->suppl.part.eval_10 +
                                exp->suppl.part.eval_11;
                    }
                  } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {
                    if( report_bitwise ) {
                      tot_num = 3 * exp->value->width;
                      num_hit = vector_get_eval_abc_count( exp->value );
                    } else {
                      tot_num = 3;
                      num_hit = exp->suppl.part.eval_10 +
                                exp->suppl.part.eval_01 +
                                exp->suppl.part.eval_00;
                    }
                  } else {
                    if( report_bitwise ) {
                      tot_num = 4 * exp->value->width;
                      num_hit = vector_get_eval_abcd_count( exp->value );
                    } else {
                      tot_num = 4;
                      num_hit = exp->suppl.part.eval_00 +
                                exp->suppl.part.eval_01 +
                                exp->suppl.part.eval_10 +
                                exp->suppl.part.eval_11;
                    }
                  }
                  *total += tot_num;
                  if( excluded ) {
                    *hit      += tot_num;
                    *excludes += tot_num;
                  } else {
                    *hit += num_hit;
                  }
                  if( (num_hit != tot_num) && (exp->ulid == -1) && !combination_is_expr_multi_node( exp ) ) {
                    exp->ulid = *ulid;
                    (*ulid)++;
                  }
                }
              } else if( EXPR_IS_EVENT( exp ) == 1 ) {
                if( (info_suppl.part.scored_events == 1) && rpt_event ) {
                  (*total)++;
                  num_hit = ESUPPL_WAS_TRUE( exp->suppl );
                  if( excluded ) {
                    (*hit)++;
                    (*excludes)++;
                  } else {
                    *hit += num_hit;
                  }
                  if( (num_hit != 1) && (exp->ulid == -1) && !combination_is_expr_multi_node( exp ) ) {
                    exp->ulid = *ulid;
                    (*ulid)++;
                  }
                }
              } else {
                if( (info_suppl.part.scored_comb == 1) && rpt_comb &&
                    ((exp->op != EXP_OP_EXPAND) || (exp->value->width > 0)) ) {
                  if( report_bitwise ) {
                    *total  = *total + (2 * exp->value->width);
                    num_hit = vector_get_eval_ab_count( exp->value );
                  } else {
                    *total  = *total + 2;
                    num_hit = ESUPPL_WAS_TRUE( exp->suppl ) + ESUPPL_WAS_FALSE( exp->suppl );
                  }
                  if( excluded ) {
                    *hit      += 2;
                    *excludes += 2;
                  } else {
                    *hit += num_hit;
                  }
                  if( (num_hit != 2) && (exp->ulid == -1) && !combination_is_expr_multi_node( exp ) ) {
                    exp->ulid = *ulid;
                    (*ulid)++;
                  }
                }
              }
            }
          }

        }

      }

    }

    /* Consider this expression to be counted */
    exp->suppl.part.comb_cntd = 1;

  }

  PROFILE_END;

}

/*!
 Iterates through specified expression list, setting the combination counted bit
 in the supplemental field of each expression.  This function needs to get called
 whenever a new module is picked by the GUI.
*/
static void combination_reset_counted_exprs(
  func_unit* funit  /*!< Pointer to functional unit to reset */
) { PROFILE(COMBINATION_RESET_COUNTED_EXPRS);

  unsigned int i;
  funit_link*  child;  /* Pointer to current child functional unit */

  assert( funit != NULL );

  /* Reset the comb_cntd bit in all expressions for the current functional unit */
  for( i=0; i<funit->exp_size; i++ ) {
    funit->exps[i]->suppl.part.comb_cntd = 1;
  }

  /* Do the same for all children functional units that are unnamed */
  child = funit->tf_head;
  while( child != NULL ) {
    if( funit_is_unnamed( child->funit ) ) {
      combination_reset_counted_exprs( child->funit );
    }
    child = child->next;
  }

  PROFILE_END;

}

/*!
 Recursively iterates through specified expression tree, clearing the combination
 counted bit in the supplemental field of each child expression.  This functions
 needs to get called whenever the excluded bit of an expression is changed.
*/
void combination_reset_counted_expr_tree(
  expression* exp  /*!< Pointer to expression tree to reset */
) { PROFILE(COMBINATION_RESET_COUNTED_EXPR_TREE);

  if( exp != NULL ) {

    exp->suppl.part.comb_cntd = 0;

    combination_reset_counted_expr_tree( exp->left );
    combination_reset_counted_expr_tree( exp->right );

  }

  PROFILE_END;

}

/*!
 Iterates through specified expression list and finds all root expressions.  For
 each root expression, the combination_get_tree_stats function is called to generate
 the coverage numbers for the specified expression tree.  Called by report function.
*/
void combination_get_stats(
            func_unit*    funit,      /*!< Pointer to functional unit to search */
            bool          rpt_comb,   /*!< Report combinational coverage */
            bool          rpt_event,  /*!< Report event coverage */
  /*@out@*/ unsigned int* hit,        /*!< Pointer to number of logical combinations hit during simulation */
  /*@out@*/ unsigned int* excluded,   /*!< Pointer to number of excluded logical combinations */
  /*@out@*/ unsigned int* total       /*!< Pointer to total number of logical combinations */
) { PROFILE(COMBINATION_GET_STATS);

  func_iter  fi;    /* Functional unit iterator */
  statement* stmt;  /* Pointer to current statement being examined */
  int        ulid;  /* Current underline ID for this expression */
  
  /* If the given functional unit is not an unnamed scope, traverse it now */
  if( !funit_is_unnamed( funit ) ) {

    /* Initialize functional unit iterator */
    func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

    /* Traverse statements in the given functional unit */
    while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {
      ulid = 1;
      combination_get_tree_stats( stmt->exp, rpt_comb, rpt_event, &ulid, 0, stmt->suppl.part.excluded, hit, excluded, total );
    }

    /* Deallocate functional unit iterator */
    func_iter_dealloc( &fi );

  }

  PROFILE_END;

}

/*!
 Retrieves the combinational logic summary information for the specified functional unit
*/
void combination_get_funit_summary(
            func_unit*    funit,     /*!< Pointer to functional unit */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to location to store the number of hit combinations for the specified functional unit */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded logical combinations */
  /*@out@*/ unsigned int* total      /*!< Pointer to location to store the total number of combinations for the specified functional unit */
) { PROFILE(COMBINATION_GET_FUNIT_SUMMARY);

  *hit      = funit->stat->comb_hit;
  *excluded = funit->stat->comb_excluded;
  *total    = funit->stat->comb_total;

  PROFILE_END;

}

/*!
 Retrieves the combinational logic summary information for the specified functional unit instance
*/
void combination_get_inst_summary(
            funit_inst*   inst,      /*!< Pointer to functional unit instance */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to location to store the number of hit combinations for the specified functional unit instance */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded logical combinations */
  /*@out@*/ unsigned int* total      /*!< Pointer to location to store the total number of combinations for the specified functional unit instance */
) { PROFILE(COMBINATION_GET_INST_SUMMARY);
  
  *hit      = inst->stat->comb_hit;
  *excluded = inst->stat->comb_excluded;
  *total    = inst->stat->comb_total;
            
  PROFILE_END;
  
}

/*!
 Outputs the instance combinational logic summary information to the given output file.
*/
static bool combination_display_instance_summary(
  FILE* ofile,  /*!< Pointer to file to output instance summary to */
  char* name,   /*!< Name of instance to display */
  int   hits,   /*!< Number of combinations hit in instance */
  int   total   /*!< Total number of logic combinations in instance */
) { PROFILE(COMBINATION_DISPLAY_INSTANCE_SUMMARY);

  float percent;  /* Percentage of lines hit */
  int   miss;     /* Number of lines missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-63.63s    %4d/%4d/%4d      %3.0f%%\n",
           name, hits, miss, total, percent );

  PROFILE_END;

  return( miss > 0 );

}

/*!
 \return Returns TRUE if combinations were found to be missed; otherwise,
         returns FALSE.

 Outputs summarized results of the combinational logic coverage per functional unit
 instance to the specified output stream.  Summarized results are printed 
 as percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
static bool combination_instance_summary(
            FILE*       ofile,   /*!< Pointer to file to output results to */
            funit_inst* root,    /*!< Pointer to node in instance tree to evaluate */
            char*       parent,  /*!< Name of parent instance name */
  /*@out@*/ int*        hits,    /*!< Pointer to accumulated number of combinations hit */
  /*@out@*/ int*        total    /*!< Pointer to accumulated number of total combinations in design */
) { PROFILE(COMBINATION_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder of instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if a logical combination was missed */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Generate printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) || root->suppl.name_diff ) {
    strcpy( tmpname, parent );
  } else if( strcmp( parent, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent, obf_inst( pname ) );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && root->stat->show && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    miss_found |= combination_display_instance_summary( ofile, tmpname, root->stat->comb_hit, root->stat->comb_total );

    /* Update accumulated information */
    *hits  += root->stat->comb_hit;
    *total += root->stat->comb_total;

  }

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= combination_instance_summary( ofile, curr, tmpname, hits, total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one logic combination was found to not be hit; otherwise, returns FALSE.

 Outputs the summary combinational logic information for the specified functional unit to the given output stream.
*/
static bool combination_display_funit_summary(
  FILE*       ofile,  /*!< Pointer to file to output functional unit summary information to */
  const char* name,   /*!< Name of functional unit being reported */
  const char* fname,  /*!< Filename that contains the functional unit being reported */
  int         hits,   /*!< Number of logic combinations that were hit during simulation */
  int         total   /*!< Number of total logic combinations that exist in the given functional unit */
) { PROFILE(COMBINATION_DISPLAY_FUNIT_SUMMARY);

  float percent;  /* Percentage of lines hit */
  int   miss;     /* Number of lines missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-30.30s    %-30.30s   %4d/%4d/%4d      %3.0f%%\n",
           name, fname, hits, miss, total, percent );

  PROFILE_END;

  return( miss > 0 );

}

/*!
 \return Returns TRUE if combinations were found to be missed; otherwise,
         returns FALSE.

 Outputs summarized results of the combinational logic coverage per functional unit
 to the specified output stream.  Summarized results are printed as 
 percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
static bool combination_funit_summary(
            FILE*       ofile,  /*!< Pointer to file to output results to */
            funit_link* head,   /*!< Pointer to link in current functional unit list to evaluate */
  /*@out@*/ int*        hits,   /*!< Pointer to number of combinations hit in all functional units */
  /*@out@*/ int*        total   /*!< Pointer to total number of combinations found in all functional units */
) { PROFILE(COMBINATION_FUNIT_SUMMARY);

  bool miss_found = FALSE;  /* Set to TRUE if missing combinations were found */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      miss_found |= combination_display_funit_summary( ofile, obf_funit( funit_flatten_name( head->funit ) ), get_basename( obf_file( head->funit->orig_fname ) ),
                                                       head->funit->stat->comb_hit, head->funit->stat->comb_total );

      /* Update accumulated information */
      *hits  += head->funit->stat->comb_hit;
      *total += head->funit->stat->comb_total;

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 Draws an underline containing the specified expression ID to the specified
 line.  The expression ID will be placed immediately following the beginning
 vertical bar.
*/
static void combination_draw_line(
  char* line,   /*!< Pointer to line to create line onto */
  int   size,   /*!< Number of characters long line is */
  int   exp_id  /*!< ID to place in underline */
) { PROFILE(COMBINATION_DRAW_LINE);

  char         str_exp_id[12];  /* String containing value of exp_id */
  int          exp_id_size;     /* Number of characters exp_id is in length */
  int          i;               /* Loop iterator */
  unsigned int rv;              /* Return value from calls to snprintf */

  /* Calculate size of expression ID */
  rv = snprintf( str_exp_id, 12, "%d", exp_id );
  assert( rv < 12 );
  exp_id_size = strlen( str_exp_id );

  line[0] = '|';
  line[1] = '\0';
  strcat( line, str_exp_id );

  for( i=(exp_id_size + 1); i<(size - 1); i++ ) {
    line[i] = '-';
  }

  line[i]   = '|';
  line[i+1] = '\0';

  PROFILE_END;

}

/*!
 Draws an underline containing the specified expression ID to the specified
 line.  The expression ID will be placed in the center of the generated underline.
*/
static void combination_draw_centered_line(
  char* line,      /*!< Pointer to line to create line onto */
  int   size,      /*!< Number of characters long line is */
  int   exp_id,    /*!< ID to place in underline */
  bool  left_bar,  /*!< If set to TRUE, draws a vertical bar at the beginning of the underline */
  bool  right_bar  /*!< If set to TRUE, draws a vertical bar at the end of the underline */
) { PROFILE(COMBINATION_DRAW_CENTERED_LINE);

  char         str_exp_id[12];   /* String containing value of exp_id */
  int          exp_id_size;      /* Number of characters exp_id is in length */
  int          i;                /* Loop iterator */
  unsigned int rv;               /* Return value from snprintf call */

  /* Calculate size of expression ID */
  rv = snprintf( str_exp_id, 12, "%d", exp_id );
  assert( rv < 12 );
  exp_id_size = strlen( str_exp_id );

  if( left_bar ) {
    line[0] = '|';
  } else {
    line[0] = '-';
  }

  for( i=1; i<((size - exp_id_size) / 2); i++ ) {
    line[i] = '-';
  }

  line[i] = '\0';
  strcat( line, str_exp_id );

  for( i=(i + exp_id_size); i<(size - 1); i++ ) {
    line[i] = '-';
  }

  if( right_bar ) {
    line[i] = '|';
  } else {
    line[i] = '-';
  }
  line[i+1] = '\0';

  PROFILE_END;

}

/*!
 Parenthesizes the code format and code format size variables.
*/
static void combination_parenthesize(
            bool          parenthesis,
            const char*   pre_code_format,
  /*@out@*/ char**        new_code_format,
  /*@out@*/ unsigned int* code_format_size
) { PROFILE(COMBINATION_PARENTHESIZE);

  if( parenthesis ) {

    /* Create the new code format string with spacing for the parenthesis */
    unsigned int rv = snprintf( *new_code_format, 300, " %s ", pre_code_format );
    assert( rv < 300 );

    /* Update the code format size */
    *code_format_size += 2;

  } else {

    strcpy( *new_code_format, pre_code_format );

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw combination_underline_tree combination_underline_tree

 Recursively parses specified expression tree, underlining and labeling each
 measurable expression.
*/
static void combination_underline_tree(
               expression*   exp,         /*!< Pointer to expression to create underline for */
               unsigned int  curr_depth,  /*!< Specifies current depth in expression tree */
  /*@out@*/    char***       lines,       /*!< Stack of lines for left child */
  /*@out@*/    unsigned int* depth,       /*!< Pointer to top of left child stack */
  /*@out@*/    unsigned int* size,        /*!< Pointer to character width of this node */
  /*@unused@*/ exp_op_type   parent_op,   /*!< Expression operation of parent used for calculating parenthesis */
               bool          center,      /*!< Specifies if expression IDs should be centered in underlines or at beginning */
               func_unit*    funit        /*!< Pointer to current functional unit containing this expression */
) { PROFILE(COMBINATION_UNDERLINE_TREE);

  char**       l_lines;         /* Pointer to left underline stack */
  char**       r_lines;         /* Pointer to right underline stack */
  unsigned int l_depth = 0;     /* Index to top of left stack */
  unsigned int r_depth = 0;     /* Index to top of right stack */
  unsigned int l_size;          /* Number of characters for left expression */
  unsigned int r_size;          /* Number of characters for right expression */
  char*        exp_sp;          /* Space to take place of missing expression(s) */
  char*        code_fmt;        /* Contains format string for rest of stack */
  char*        tmpstr;          /* Temporary string value */
  unsigned int comb_missed;     /* If set to 1, current combination was missed */
  char*        tmpname = NULL;  /* Temporary pointer to current signal name */
  char*        pname;           /* Printable version of signal/function/task name */
  func_unit*   tfunit;          /* Temporary pointer to found functional unit */
  int          ulid;            /* Underline ID to use */
  unsigned int rv;              /* Return value from snprintf calls */
  
  *depth      = 0;
  *size       = 0;
  l_lines     = NULL;
  r_lines     = NULL;
  comb_missed = 0;
  code_fmt    = (char*)malloc_safe( 300 );

  if( exp != NULL ) {

    if( (exp->op == EXP_OP_LAST) || (exp->op == EXP_OP_NB_CALL) ) {

      *size = 0;

    } else if( exp->op == EXP_OP_STATIC ) {

      unsigned int data_type = exp->value->suppl.part.data_type;

      if( data_type == VDATA_R64 ) {

        *size = strlen( exp->value->value.r64->str );

      } else if( data_type == VDATA_R32 ) {

        *size = strlen( exp->value->value.r32->str );

      } else if( ESUPPL_STATIC_BASE( exp->suppl ) == DECIMAL ) {

        rv = snprintf( code_fmt, 300, "%d", vector_to_int( exp->value ) );
        assert( rv < 300 );
        *size = strlen( code_fmt );

        /*
         If the size of this decimal value is only 1 and its parent is a NEGATE op,
         make it two so that we don't have problems with negates and the like later.
        */
        if( (*size == 1) && (exp->parent->expr->op == EXP_OP_NEGATE) ) {
          *size = 2;
        }
      
      } else {

        tmpstr = vector_to_string( exp->value, ESUPPL_STATIC_BASE( exp->suppl ), FALSE, 0 );
        *size  = strlen( tmpstr );
        free_safe( tmpstr, (strlen( tmpstr ) + 1) );

        /* Adjust for quotation marks */
        if( ESUPPL_STATIC_BASE( exp->suppl ) == QSTRING ) {
          *size += 2;
        }

      }

    } else if( exp->op == EXP_OP_SLIST ) {

      *size = 2;
      strcpy( code_fmt, "@*" );

    } else if( exp->op == EXP_OP_ALWAYS_COMB ) {

      *size = 11;
      strcpy( code_fmt, "always_comb" );

    } else if( exp->op == EXP_OP_ALWAYS_LATCH ) {
   
      *size = 12;
      strcpy( code_fmt, "always_latch" );

    } else if( exp->op == EXP_OP_STIME ) {

      *size = 5;
      strcpy( code_fmt, "$time" );

    } else if( (exp->op == EXP_OP_SRANDOM) && (exp->left == NULL) ) {

      *size = 7;
      strcpy( code_fmt, "$random" );

    } else if( (exp->op == EXP_OP_SURANDOM) && (exp->left == NULL) ) {

      *size = 8;
      strcpy( code_fmt, "$urandom" );

    } else {

      Try {

        if( (exp->op == EXP_OP_SIG) || (exp->op == EXP_OP_PARAM) ) {

          tmpname = scope_gen_printable( exp->name );
          *size   = strlen( tmpname );
          switch( *size ) {
            case 0 :  assert( *size > 0 );                     break;
            case 1 :  *size = 3;  strcpy( code_fmt, " %s " );  break;
            case 2 :  *size = 3;  strcpy( code_fmt, " %s" );   break;
            default:  strcpy( code_fmt, "%s" );                break;
          }

          free_safe( tmpname, (strlen( tmpname ) + 1) );
        
        } else {

          bool paren = (exp->suppl.part.parenthesis == 1);

          combination_underline_tree( exp->left,  combination_calc_depth( exp, curr_depth, TRUE ),  &l_lines, &l_depth, &l_size, exp->op, center, funit );
          combination_underline_tree( exp->right, combination_calc_depth( exp, curr_depth, FALSE ), &r_lines, &r_depth, &r_size, exp->op, center, funit );

          switch( exp->op ) {
            case EXP_OP_XOR        :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_XOR_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_MULTIPLY   :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_MLT_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_DIVIDE     :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_DIV_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_MOD        :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_MOD_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_ADD        :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_ADD_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_SUBTRACT   :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_SUB_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_EXPONENT   :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_AND        :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_AND_A      :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_OR         :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_OR_A       :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_NAND       :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_NOR        :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_NXOR       :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_LT         :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_GT         :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_LSHIFT     :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_LS_A       :  *size = l_size + r_size + 5;  combination_parenthesize( paren, "%s     %s",  &code_fmt, size );  break;
            case EXP_OP_ALSHIFT    :  *size = l_size + r_size + 5;  combination_parenthesize( paren, "%s     %s",  &code_fmt, size );  break;
            case EXP_OP_ALS_A      :  *size = l_size + r_size + 6;  combination_parenthesize( paren, "%s      %s", &code_fmt, size );  break;
            case EXP_OP_RSHIFT     :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_RS_A       :  *size = l_size + r_size + 5;  combination_parenthesize( paren, "%s     %s",  &code_fmt, size );  break;
            case EXP_OP_ARSHIFT    :  *size = l_size + r_size + 5;  combination_parenthesize( paren, "%s     %s",  &code_fmt, size );  break;
            case EXP_OP_ARS_A      :  *size = l_size + r_size + 6;  combination_parenthesize( paren, "%s      %s", &code_fmt, size );  break;
            case EXP_OP_EQ         :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_CEQ        :  *size = l_size + r_size + 5;  combination_parenthesize( paren, "%s     %s",  &code_fmt, size );  break;
            case EXP_OP_LE         :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_GE         :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_NE         :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_CNE        :  *size = l_size + r_size + 5;  combination_parenthesize( paren, "%s     %s",  &code_fmt, size );  break;
            case EXP_OP_LOR        :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_LAND       :  *size = l_size + r_size + 4;  combination_parenthesize( paren, "%s    %s",   &code_fmt, size );  break;
            case EXP_OP_COND       :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_COND_SEL   :  *size = l_size + r_size + 3;  combination_parenthesize( paren, "%s   %s",    &code_fmt, size );  break;
            case EXP_OP_UINV       :  *size = l_size + r_size + 1;  combination_parenthesize( paren, " %s",        &code_fmt, size );  break;
            case EXP_OP_UAND       :  *size = l_size + r_size + 1;  combination_parenthesize( paren, " %s",        &code_fmt, size );  break;
            case EXP_OP_UNOT       :  *size = l_size + r_size + 1;  combination_parenthesize( paren, " %s",        &code_fmt, size );  break;
            case EXP_OP_UOR        :  *size = l_size + r_size + 1;  combination_parenthesize( paren, " %s",        &code_fmt, size );  break;
            case EXP_OP_UXOR       :  *size = l_size + r_size + 1;  combination_parenthesize( paren, " %s",        &code_fmt, size );  break;
            case EXP_OP_UNAND      :  *size = l_size + r_size + 2;  combination_parenthesize( paren, "  %s",       &code_fmt, size );  break;
            case EXP_OP_UNOR       :  *size = l_size + r_size + 2;  combination_parenthesize( paren, "  %s",       &code_fmt, size );  break;
            case EXP_OP_UNXOR      :  *size = l_size + r_size + 2;  combination_parenthesize( paren, "  %s",       &code_fmt, size );  break;
            case EXP_OP_PARAM_SBIT :
            case EXP_OP_SBIT_SEL   :  
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 0) &&
                  (exp->parent->expr->op == EXP_OP_DIM) &&
                  (exp->parent->expr->right == exp) ) {
                *size = l_size + r_size + 2;
                code_fmt[0] = '\0';
              } else {
                unsigned int i;
                tmpname = scope_gen_printable( exp->name );
                *size = l_size + r_size + strlen( tmpname ) + 2;
                for( i=0; i<strlen( tmpname ); i++ ) {
                  code_fmt[i] = ' ';
                }
                code_fmt[i] = '\0';
              }
              strcat( code_fmt, " %s " );
              free_safe( tmpname, (strlen( tmpname ) + 1) );
              break;
            case EXP_OP_PARAM_MBIT :
            case EXP_OP_MBIT_SEL   :  
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 0) &&
                  (exp->parent->expr->op == EXP_OP_DIM) &&
                  (exp->parent->expr->right == exp) ) {
                *size = l_size + r_size + 3;
                code_fmt[0] = '\0';
              } else {
                unsigned int i;
                tmpname = scope_gen_printable( exp->name );
                *size = l_size + r_size + strlen( tmpname ) + 3;  
                for( i=0; i<strlen( tmpname ); i++ ) {
                  code_fmt[i] = ' ';
                }
                code_fmt[i] = '\0';
              }
              strcat( code_fmt, " %s %s " );
              free_safe( tmpname, (strlen( tmpname ) + 1) );
              break;
            case EXP_OP_PARAM_MBIT_POS :
            case EXP_OP_PARAM_MBIT_NEG :
            case EXP_OP_MBIT_POS       :
            case EXP_OP_MBIT_NEG       :
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 0) &&
                  (exp->parent->expr->op == EXP_OP_DIM) &&
                  (exp->parent->expr->right == exp) ) {
                *size = l_size + r_size + 4;
                code_fmt[0] = '\0';
              } else {
                unsigned int i;
                tmpname = scope_gen_printable( exp->name );
                *size = l_size + r_size + strlen( tmpname ) + 4;
                for( i=0; i<strlen( tmpname ); i++ ) {
                  code_fmt[i] = ' ';
                }
                code_fmt[i] = '\0';
              }
              strcat( code_fmt, " %s  %s " );
              free_safe( tmpname, (strlen( tmpname ) + 1) );
              break;
            case EXP_OP_TRIGGER  :
              {
                unsigned int i;
                tmpname = scope_gen_printable( exp->name );
                *size = l_size + r_size + strlen( tmpname ) + 2;
                for( i=0; i<strlen( tmpname ) + 2; i++ ) {
                  code_fmt[i] = ' ';
                }
                code_fmt[i] = '\0';
                free_safe( tmpname, (strlen( tmpname ) + 1) );
              }
              break;
            case EXP_OP_EXPAND   :  *size = l_size + r_size + 4;  strcpy( code_fmt, " %s %s  "         );  break;
            case EXP_OP_CONCAT   :  *size = l_size + r_size + 2;  strcpy( code_fmt, " %s "             );  break;
            case EXP_OP_PLIST    :
            case EXP_OP_LIST     :  *size = l_size + r_size + 2;  strcpy( code_fmt, "%s  %s"           );  break;
            case EXP_OP_PEDGE    :
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 1)       ||
                  (exp->parent->expr->op == EXP_OP_RPT_DLY) ||
                  (exp->parent->expr->op == EXP_OP_DLY_OP) ) {
                *size = l_size + r_size + 11;  strcpy( code_fmt, "          %s " );
              } else {
                *size = l_size + r_size + 8;   strcpy( code_fmt, "        %s" );
              }
              break;
            case EXP_OP_NEDGE    :
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 1)       ||
                  (exp->parent->expr->op == EXP_OP_RPT_DLY) ||
                  (exp->parent->expr->op == EXP_OP_DLY_OP) ) {
                *size = l_size + r_size + 11;  strcpy( code_fmt, "          %s " );
              } else {
                *size = l_size + r_size + 8;   strcpy( code_fmt, "        %s" );
              }
              break;
            case EXP_OP_AEDGE    :
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 1)       ||
                  (exp->parent->expr->op == EXP_OP_RPT_DLY) ||
                  (exp->parent->expr->op == EXP_OP_DLY_OP) ) {
                *size = l_size + r_size + 3;  strcpy( code_fmt, "  %s " );
              } else {
                *size = l_size + r_size + 0;  strcpy( code_fmt, "%s" );
              }
              break;
            case EXP_OP_EOR      :
              if( (ESUPPL_IS_ROOT( exp->suppl ) == 1)       ||
                  (exp->parent->expr->op == EXP_OP_RPT_DLY) ||
                  (exp->parent->expr->op == EXP_OP_DLY_OP) ) {
                *size = l_size + r_size + 7;  strcpy( code_fmt, "  %s    %s " );
              } else {
                *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s" );
              }
              break;
            case EXP_OP_CASE     :  *size = l_size + r_size + 11; strcpy( code_fmt, "      %s   %s  "  );  break;
            case EXP_OP_CASEX    :  *size = l_size + r_size + 12; strcpy( code_fmt, "       %s   %s  " );  break;
            case EXP_OP_CASEZ    :  *size = l_size + r_size + 12; strcpy( code_fmt, "       %s   %s  " );  break;
            case EXP_OP_DELAY    :  *size = r_size + 3;  strcpy( code_fmt, "  %s " );             break;
            case EXP_OP_ASSIGN   :  *size = l_size + r_size + 10; strcpy( code_fmt, "       %s   %s" );    break;
            case EXP_OP_DASSIGN  :
            case EXP_OP_DLY_ASSIGN :
            case EXP_OP_BASSIGN  :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s" );           break;
            case EXP_OP_NASSIGN  :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s" );          break;
            case EXP_OP_SASSIGN  :
            case EXP_OP_PASSIGN  :  *size = r_size;               strcpy( code_fmt, "%s" );                break;
            case EXP_OP_IF       :  *size = r_size + 6;           strcpy( code_fmt, "    %s  " );          break;
            case EXP_OP_REPEAT   :  *size = r_size + 10;          strcpy( code_fmt, "        %s  " );      break;
            case EXP_OP_WHILE    :  *size = r_size + 9;           strcpy( code_fmt, "       %s  " );       break;
            case EXP_OP_WAIT     :  *size = r_size + 8;           strcpy( code_fmt, "      %s  " );        break;
            case EXP_OP_DLY_OP   :
            case EXP_OP_RPT_DLY  :  *size = l_size + r_size + 1;  strcpy( code_fmt, "%s %s" );             break;
            case EXP_OP_TASK_CALL :
            case EXP_OP_FUNC_CALL :
              {
                unsigned int i;
                tfunit = exp->elem.funit;
                tmpname = strdup_safe( tfunit->name );
                scope_extract_back( tfunit->name, tmpname, user_msg );
                pname = scope_gen_printable( tmpname );
                *size = l_size + r_size + strlen( pname ) + 4;
                for( i=0; i<strlen( pname ); i++ ) {
                  code_fmt[i] = ' ';
                }
                code_fmt[i] = '\0';
                strcat( code_fmt, "  %s  " );
                free_safe( tmpname, (strlen( tfunit->name ) + 1) );
                free_safe( pname, (strlen( pname ) + 1) );
              }
              break;
            case EXP_OP_NEGATE       :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"                    );  break;
            case EXP_OP_DIM          :  *size = l_size + r_size;      strcpy( code_fmt, "%s%s"                   );  break;
            case EXP_OP_IINC         :
            case EXP_OP_IDEC         :  *size = l_size + 2;           strcpy( code_fmt, "  %s"                   );  break;
            case EXP_OP_PINC         :
            case EXP_OP_PDEC         :  *size = l_size + 2;           strcpy( code_fmt, "%s  "                   );  break;
            case EXP_OP_SSIGNED      :  *size = l_size + 11;          strcpy( code_fmt, "         %s  "          );  break;
            case EXP_OP_SUNSIGNED    :  *size = l_size + 13;          strcpy( code_fmt, "           %s  "        );  break;
            case EXP_OP_SCLOG2       :  *size = l_size + 10;          strcpy( code_fmt, "        %s  "           );  break;
            case EXP_OP_SRANDOM      :  *size = l_size + 11;          strcpy( code_fmt, "         %s  "          );  break;
            case EXP_OP_SURANDOM     :  *size = l_size + 12;          strcpy( code_fmt, "          %s  "         );  break;
            case EXP_OP_SURAND_RANGE :  *size = l_size + 18;          strcpy( code_fmt, "                %s  "   );  break;
            case EXP_OP_SSRANDOM     :  *size = l_size + 12;          strcpy( code_fmt, "          %s  "         );  break;
            case EXP_OP_SR2B         :
            case EXP_OP_SB2R         :  *size = l_size + 15;          strcpy( code_fmt, "             %s  "      );  break;
            case EXP_OP_SI2R         :
            case EXP_OP_SR2I         :  *size = l_size + 9;           strcpy( code_fmt, "       %s  "            );  break;
            case EXP_OP_SSR2B        :
            case EXP_OP_SB2SR        :  *size = l_size + 20;          strcpy( code_fmt, "                  %s  " );  break;
            case EXP_OP_STESTARGS    :  *size = l_size + 18;          strcpy( code_fmt, "                %s  "   );  break;
            case EXP_OP_SVALARGS     :  *size = l_size + 19;          strcpy( code_fmt, "                 %s  "  );  break;
            default              :
              rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Unknown expression type in combination_underline_tree (%d)",
                             exp->op );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              Throw 0;
              /*@-unreachable@*/
              break;
              /*@=unreachable@*/
          }
  
        }

        /* Calculate ulid */
        ulid = exp->ulid;

        comb_missed = ((!info_suppl.part.inlined || (curr_depth <= inline_comb_depth)) &&
                       (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
                        (report_comb_depth == REPORT_VERBOSE))) ? ((ulid != -1) ? 1 : 0) : 0;

        if( l_depth > r_depth ) {
          *depth = l_depth + comb_missed;
        } else {
          *depth = r_depth + comb_missed;
        }
      
        if( *depth > 0 ) {

          unsigned int i;
                
          /* Allocate all memory for the stack */
          *lines = (char**)malloc_safe( sizeof( char* ) * (*depth) );

          /* Create underline or space */
          if( comb_missed == 1 ) {

            /* Allocate memory for this underline */
            (*lines)[(*depth)-1] = (char*)malloc_safe( *size + 1 );

            if( center ) {
              combination_draw_centered_line( (*lines)[(*depth)-1], *size, ulid, TRUE, TRUE );
            } else {
              combination_draw_line( (*lines)[(*depth)-1], *size, ulid );
            }
            /* printf( "Drawing line (%s), size: %d, depth: %d\n", (*lines)[(*depth)-1], *size, (*depth) ); */
          }

          /* Combine the left and right line stacks */
          for( i=0; i<(*depth - comb_missed); i++ ) {

            (*lines)[i] = (char*)malloc_safe( *size + 1 );

            if( (i < l_depth) && (i < r_depth) ) {
            
              /* Merge left and right lines */
              rv = snprintf( (*lines)[i], (*size + 1), code_fmt, l_lines[i], r_lines[i] );
              assert( rv < (*size + 1) );
            
              free_safe( l_lines[i], (strlen( l_lines[i] ) + 1) );
              free_safe( r_lines[i], (strlen( r_lines[i] ) + 1) );

            } else if( i < l_depth ) {
            
              /* Create spaces for right side */
              exp_sp = (char*)malloc_safe( r_size + 1 );
              gen_char_string( exp_sp, ' ', r_size );

              /* Merge left side only */
              rv = snprintf( (*lines)[i], (*size + 1), code_fmt, l_lines[i], exp_sp );
              assert( rv < (*size + 1) );
            
              free_safe( l_lines[i], (strlen( l_lines[i] ) + 1) );
              free_safe( exp_sp, (r_size + 1) );

            } else if( i < r_depth ) {

              if( l_size == 0 ) { 

                rv = snprintf( (*lines)[i], (*size + 1), code_fmt, r_lines[i] );
                assert( rv < (*size + 1) );

              } else {

                /* Create spaces for left side */
                exp_sp = (char*)malloc_safe( l_size + 1 );
                gen_char_string( exp_sp, ' ', l_size );

                /* Merge right side only */
                rv = snprintf( (*lines)[i], (*size + 1), code_fmt, exp_sp, r_lines[i] );
                assert( rv < (*size + 1) );
              
                free_safe( exp_sp, (l_size + 1) );
          
              }
  
              free_safe( r_lines[i], (strlen( r_lines[i] ) + 1) );
   
            } else {

              print_output( "Internal error:  Reached entry without a left or right underline", FATAL, __FILE__, __LINE__ );
              Throw 0;

            }

          }

          /* Free left child stack */
          if( l_depth > 0 ) {
            free_safe( l_lines, (sizeof( char* ) * l_depth) );
          }

          /* Free right child stack */
          if( r_depth > 0 ) {
            free_safe( r_lines, (sizeof( char* ) * r_depth) );
          }

        }

      } Catch_anonymous {
        unsigned int i;
        for( i=0; i<(*depth - comb_missed); i++ ) {
          free_safe( (*lines)[i], (strlen( (*lines)[i] ) + 1) );
        }
        free_safe( *lines, (sizeof( char* ) * (*depth - comb_missed)) );
        *lines = NULL;
        *depth = 0;
        for( i=0; i<l_depth; i++ ) {
          free_safe( l_lines[i], (strlen( l_lines[i] ) + 1) );
        }
        free_safe( l_lines, (sizeof( char* ) * l_depth) );
        for( i=0; i<r_depth; i++ ) {
          free_safe( r_lines[i], (strlen( r_lines[i] ) + 1) );
        }
        free_safe( r_lines, (sizeof( char* ) * r_depth) );
        Throw 0;
      }

    }

  }

  /* Deallocate memory */
  free_safe( code_fmt, 300 );

  PROFILE_END;
    
}

/*!
 \return Returns a newly allocated string that contains the underline information for
         the current line.  If there is no underline information to return, a value of
         NULL is returned.

 Formats the specified underline line to line wrap according to the generated code.  This
 function must be called after the underline lines have been calculated prior to being
 output to the ASCII report or the GUI.
*/
static char* combination_prep_line(
  char* line,   /*!< Line containing underlines that needs to be reformatted for line wrap */
  int   start,  /*!< Starting index in line to take underline information from */
  int   len     /*!< Number of characters to use for the current line */
) { PROFILE(COMBINATION_PREP_LINE);

  char* str;                /* Prepared line to return */
  char* newstr;             /* Prepared line to return */
  int   str_size;           /* Allocated size of str */
  int   exp_id;             /* Expression ID of current line */
  int   chars_read;         /* Number of characters read from sscanf function */
  int   i;                  /* Loop iterator */
  int   curr_index;         /* Index current character in str to set */
  bool  line_ip   = FALSE;  /* Specifies if a line is currently in progress */
  bool  line_seen = FALSE;  /* Specifies that a line has been seen for this line */
  int   start_ul  = 0;      /* Index of starting underline */

  /* Allocate memory for string to return */
  str_size = len + 2;
  str      = (char*)malloc_safe( str_size );

  i          = 0;
  curr_index = 0;

  while( i < (start + len) ) {
   
    if( *(line + i) == '|' ) {
      if( i >= start ) {
        line_seen = TRUE;
      }
      if( !line_ip ) {
        line_ip  = TRUE;
        start_ul = i;
        if( sscanf( (line + i + 1), "%d%n", &exp_id, &chars_read ) != 1 ) {
          assert( 0 == 1 );
        } else {
          i += chars_read;
        }
      } else {
        line_ip = FALSE;
        if( i >= start ) {
          if( start_ul >= start ) {
            combination_draw_centered_line( (str + curr_index), ((i - start_ul) + 1), exp_id, TRUE,  TRUE );
            curr_index += (i - start_ul) + 1;
          } else {
            combination_draw_centered_line( (str + curr_index), ((i - start) + 1), exp_id, FALSE, TRUE );
            curr_index += (i - start) + 1;
          }
        }
      }
    } else {
      if( i >= start ) {
        if( *(line + i) == '-' ) {
          line_seen = TRUE;
        } else {
          str[curr_index] = *(line + i);
          curr_index++;
        }
      }
    }

    i++;

  }

  if( line_ip ) {
    /* If our pointer exceeded the allotted size, resize the str to fit */
    if( i > (start + len) ) {
      str      = (char*)realloc_safe( str, str_size, (len + 2 + (i - (start + len))) );
      str_size = (len + 2 + (i - (start + len)));
    }
    if( start_ul >= start ) {
      combination_draw_centered_line( (str + curr_index), ((i - start_ul) + 1), exp_id, TRUE,  FALSE );
      curr_index += (i - start_ul) + 1;
    } else {
      combination_draw_centered_line( (str + curr_index), ((i - start) + 1), exp_id, FALSE, FALSE );
      curr_index += (i - start) + 1;
    }
  }

  /* If we didn't see any underlines here, return NULL */
  if( !line_seen ) {
    newstr = NULL;
  } else {
    str[curr_index] = '\0';
    newstr = strdup_safe( str );
  }

  /* The str may be a bit oversized, so we copied it and now free it here (where we know its allocated size) */
  free_safe( str, str_size );

  PROFILE_END;

  return( newstr );

}

/*!
 \throws anonymous combination_underline_tree

 Traverses through the expression tree that is on the same line as the parent,
 creating underline strings.  An underline is created for each expression that
 does not have complete combination logic coverage.  Each underline (children to
 parent creates an inverted tree) and contains a number for the specified expression.
*/
static void combination_underline(
  FILE*        ofile,       /*!< Pointer output stream to display underlines to */
  char**       code,        /*!< Array of strings containing code to output */
  unsigned int code_depth,  /*!< Number of entries in code array */
  expression*  exp,         /*!< Pointer to parent expression to create underline for */
  func_unit*   funit        /*!< Pointer to the functional unit containing the expression to underline */
) { PROFILE(COMBINATION_UNDERLINE);

  char**       lines;      /* Pointer to a stack of lines */
  unsigned int depth;      /* Depth of underline stack */
  unsigned int size;       /* Width of stack in characters */
  unsigned int i;          /* Loop iterator */
  unsigned int j;          /* Loop iterator */
  char*        tmpstr;     /* Temporary string variable */
  unsigned int start = 0;  /* Starting index */

  combination_underline_tree( exp, 0, &lines, &depth, &size, exp->op, (code_depth == 1), funit );

  for( j=0; j<code_depth; j++ ) {

    assert( code[j] != NULL );

    if( j == 0 ) {
      fprintf( ofile, "        %7u:    %s\n", exp->line, code[j] );
    } else {
      fprintf( ofile, "                    %s\n", code[j] );
    }

    if( code_depth == 1 ) {
      for( i=0; i<depth; i++ ) {
        fprintf( ofile, "                    %s\n", lines[i] );
      }
    } else {
      for( i=0; i<depth; i++ ) {
        if( (tmpstr = combination_prep_line( lines[i], start, strlen( code[j] ) )) != NULL ) {
          fprintf( ofile, "                    %s\n", tmpstr );
          free_safe( tmpstr, (strlen( tmpstr ) + 1) );
        }
      }
    }

    start += strlen( code[j] );

    free_safe( code[j], (strlen( code[j] ) + 1) );

  }

  for( i=0; i<depth; i++ ) {
    free_safe( lines[i], (strlen( lines[i] ) + 1) );
  }

  if( depth > 0 ) {
    free_safe( lines, (sizeof( char* ) * depth) );
  }

  if( code_depth > 0 ) {
    free_safe( code, (sizeof( char* ) * code_depth) );
  }

  PROFILE_END;

}

/*!
 Displays the missed unary combination(s) that keep the combination coverage for
 the specified expression from achieving 100% coverage.
*/
static void combination_unary(
  /*@out@*/ char***     info,          /*!< Pointer to array of strings that will contain the coverage information for this expression */
  /*@out@*/ int*        info_size,     /*!< Pointer to integer containing number of elements in info array */
            expression* exp,           /*!< Pointer to expression to evaluate */
            bool        show_excluded  /*!< Set to TRUE to output expression if it is excluded */
) { PROFILE(COMBINATION_UNARY);

  int          hit = 0;                           /* Number of combinations hit for this expression */
  int          tot;                               /* Total number of coverage points possible */
  char         tmp[20];                           /* Temporary string used for sizing lines for numbers */
  unsigned int length;                            /* Length of the current line to allocate */
  char*        op = exp_op_info[exp->op].op_str;  /* Operations string */
  int          lines;                             /* Specifies the number of lines to allocate memory for */
  unsigned int rv;                                /* Return value from snprintf calls */

  assert( exp != NULL );

  /* Get hit information */
  if( report_bitwise && (exp->value->width > 1) ) {
    hit   = vector_get_eval_ab_count( exp->value );
    lines = exp->value->width + 2;
    tot   = (2 * exp->value->width);
  } else {
    lines = 1;
    tot   = 2;
    hit   = ESUPPL_WAS_FALSE( exp->suppl ) + ESUPPL_WAS_TRUE( exp->suppl );
  }

  if( ((hit != tot) && !ESUPPL_EXCLUDED( exp->suppl )) ||
      (ESUPPL_EXCLUDED( exp->suppl ) && show_excluded) ) {

    char         spaces[30];
    unsigned int eid_size;

    if( flag_output_exclusion_ids ) {
      eid_size = db_get_exclusion_id_size();
    }

    spaces[0] = '\0';

    assert( exp->ulid != -1 );

    /* Allocate memory for info array */
    *info_size = 4 + lines;
    *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size) );

    /* Allocate lines and assign values */ 
    length = 26;
    rv = snprintf( tmp, 20, "%d", exp->ulid );  assert( rv < 20 );  length += strlen( tmp );
    rv = snprintf( tmp, 20, "%d", hit );        assert( rv < 20 );  length += strlen( tmp );
    rv = snprintf( tmp, 20, "%d", tot );        assert( rv < 20 );  length += strlen( tmp );
    if( flag_output_exclusion_ids ) {
      length += (eid_size - 1) + 4;
      gen_char_string( spaces, ' ', ((eid_size - 1) + 4) );
    }
    (*info)[0] = (char*)malloc_safe( length );
    if( flag_output_exclusion_ids ) {
      rv = snprintf( (*info)[0], length, "        (%s)  Expression %d   (%d/%d)", db_gen_exclusion_id( 'E', exp->id ), exp->ulid, hit, tot );
    } else {
      rv = snprintf( (*info)[0], length, "        Expression %d   (%d/%d)", exp->ulid, hit, tot );
    }
    assert( rv < length );

    length = 25 + strlen( op ) + strlen( spaces );  (*info)[1] = (char*)malloc_safe( length );
    rv = snprintf( (*info)[1], length, "%s        ^^^^^^^^^^^^^ - %s", spaces, op );
    assert( rv < length );

    if( report_bitwise && (exp->value->width > 1) ) {

      char*        tmp;
      unsigned int i;
   
      length = 23 + strlen( spaces );
      (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s          Bit | E | E ", spaces );  assert( rv < length );
      (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        ======|=0=|=1=", spaces );  assert( rv < length );
      (*info)[5] = (char*)malloc_safe( length );  rv = snprintf( (*info)[5], length, "%s        ------|---|---", spaces );  assert( rv < length );

      length = 22 + strlen( spaces );
      (*info)[4] = (char*)malloc_safe( length );
      rv = snprintf( (*info)[4], length, "%s          All | %c   %c", spaces,
                     ((ESUPPL_WAS_FALSE( exp->suppl ) == 1) ? ' ' : '*'),
                     ((ESUPPL_WAS_TRUE( exp->suppl )  == 1) ? ' ' : '*') );
      assert( rv < length );
      for( i=0; i<exp->value->width; i++ ) {
        (*info)[i+6] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[i+6], length, "%s         %4u | %c   %c", spaces, i,
                       ((vector_get_eval_a( exp->value, i ) == 1) ? ' ' : '*'),
                       ((vector_get_eval_b( exp->value, i ) == 1) ? ' ' : '*') );
        assert( rv < length );
      }

    } else {

      length = 16 + strlen( spaces );
      (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s         E | E ", spaces );
      (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        =0=|=1=", spaces );

      length = 15 + strlen( spaces );
      (*info)[4] = (char*)malloc_safe( length );
      rv = snprintf( (*info)[4], length, "%s         %c   %c", spaces,
                     ((ESUPPL_WAS_FALSE( exp->suppl ) == 1) ? ' ' : '*'),
                     ((ESUPPL_WAS_TRUE( exp->suppl )  == 1) ? ' ' : '*') );
      assert( rv < length );

    }

  }

  PROFILE_END;

}

/*!
 Displays the missed unary combination(s) that keep the combination coverage for
 the specified expression from achieving 100% coverage.
*/
static void combination_event(
  /*@out@*/ char***     info,       /*!< Pointer to array of strings that will contain the coverage information for this expression */
  /*@out@*/ int*        info_size,  /*!< Pointer to integer containing number of elements in info array */
            expression* exp,        /*!< Pointer to expression to evaluate */
            bool        show_excluded  /*!< Set to TRUE to output the expression if it is excluded */
) { PROFILE(COMBINATION_EVENT);

  char         tmp[20];
  unsigned int length;
  char*        op = exp_op_info[exp->op].op_str;  /* Operation string */

  assert( exp != NULL );

  if( (!ESUPPL_WAS_TRUE( exp->suppl ) && !ESUPPL_EXCLUDED( exp->suppl )) ||
      (ESUPPL_EXCLUDED( exp->suppl ) && show_excluded) ) {

    char         spaces[30];
    unsigned int rv;
    unsigned int eid_size;

    if( flag_output_exclusion_ids ) {
      eid_size = db_get_exclusion_id_size();
    }

    spaces[0] = '\0';

    assert( exp->ulid != -1 );

    /* Allocate memory for info array */
    *info_size = 3;
    *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size) );

    /* Allocate lines and assign values */
    length = 28;  rv = snprintf( tmp, 20, "%d", exp->ulid );  assert( rv < 20 );  length += strlen( tmp );
    if( flag_output_exclusion_ids ) {
      length += (eid_size - 1) + 4;
      gen_char_string( spaces, ' ', ((eid_size - 1) + 4) );
    }
    (*info)[0] = (char*)malloc_safe( length );
    if( flag_output_exclusion_ids ) {
      rv = snprintf( (*info)[0], length, "        (%s)  Expression %d   (0/1)", db_gen_exclusion_id( 'E', exp->id ), exp->ulid );
    } else {
      rv = snprintf( (*info)[0], length, "        Expression %d   (0/1)", exp->ulid );
    }
    assert( rv < length );

    length = 25 + strlen( op ) + strlen( spaces );
    (*info)[1] = (char*)malloc_safe( length );  rv = snprintf( (*info)[1], length, "%s        ^^^^^^^^^^^^^ - %s", spaces, op );  assert( rv < length );
    length = 31 + strlen( spaces );
    (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s         * Event did not occur", spaces );  assert( rv < length );

  }

  PROFILE_END;

}

/*!
 Displays the missed combinational sequences for the specified expression to the
 specified output stream in tabular form.
*/
static void combination_two_vars(
  /*@out@*/ char***     info,          /*!< Pointer to array of strings that will contain the coverage information for this expression */
  /*@out@*/ int*        info_size,     /*!< Pointer to integer containing number of elements in info array */
            expression* exp,           /*!< Pointer to expression to evaluate */
            bool        show_excluded  /*!< If set to TRUE, displays the expression if it has been excluded */
) { PROFILE(COMBINATION_TWO_VARS);

  int          hit;                               /* Number of combinations hit for this expression */
  int          total;                             /* Total number of combinations for this expression */
  char         tmp[20];                           /* Temporary string used for calculating line width */
  unsigned int length;                            /* Specifies the length of the current line */
  char*        op = exp_op_info[exp->op].op_str;  /* Operation string */
  int          lines;                             /* Specifies the number of lines needed to output this vector */
  unsigned int rv;                                /* Return value from snprintf calls */

  assert( exp != NULL );

  /* Verify that left child expression is valid for this operation */
  assert( exp->left != NULL );

  /* Verify that right child expression is valid for this operation */
  assert( exp->right != NULL );

  /* Get hit information */
  if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {
    if( report_bitwise && (exp->value->width > 1) ) {
      lines = exp->value->width + 2;
      total = (3 * exp->value->width);
      hit   = vector_get_eval_abc_count( exp->value );
    } else {
      lines = 1;
      total = 3;
      hit   = exp->suppl.part.eval_01 +
              exp->suppl.part.eval_10 +
              exp->suppl.part.eval_11;
    }
  } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {
    if( report_bitwise && (exp->value->width > 1) ) {
      lines = exp->value->width + 2;
      total = (3 * exp->value->width);
      hit   = vector_get_eval_abc_count( exp->value );
    } else {
      lines = 1;
      total = 3;
      hit   = exp->suppl.part.eval_10 +
              exp->suppl.part.eval_01 +
              exp->suppl.part.eval_00;
    }
  } else {
    if( report_bitwise && (exp->value->width > 1) ) {
      lines = exp->value->width + 2;
      total = (4 * exp->value->width);
      hit   = vector_get_eval_abcd_count( exp->value );
    } else {
      lines = 1;
      total = 4;
      hit   = exp->suppl.part.eval_00 +
              exp->suppl.part.eval_01 +
              exp->suppl.part.eval_10 +
              exp->suppl.part.eval_11;
    }
  }

  if( ((hit != total) && !ESUPPL_EXCLUDED( exp->suppl )) ||
      (ESUPPL_EXCLUDED( exp->suppl ) && show_excluded) ) {

    char         spaces[30];
    unsigned int eid_size;

    if( flag_output_exclusion_ids ) {
      eid_size = db_get_exclusion_id_size();
    }

    spaces[0] = '\0';

    assert( exp->ulid != -1 );

    /* Allocate memory for info array */
    *info_size = 4 + lines;
    *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size) );

    /* Allocate lines and assign values */ 
    length = 26;
    rv = snprintf( tmp, 20, "%d", exp->ulid );  assert( rv < 20 );  length += strlen( tmp );
    rv = snprintf( tmp, 20, "%d", hit );        assert( rv < 20 );  length += strlen( tmp );
    rv = snprintf( tmp, 20, "%d", total );      assert( rv < 20 );  length += strlen( tmp );
    if( flag_output_exclusion_ids ) {
      length += (eid_size - 1) + 4;
      gen_char_string( spaces, ' ', ((eid_size - 1) + 4) );
    }
    (*info)[0] = (char*)malloc_safe( length );
    if( flag_output_exclusion_ids ) {
      rv = snprintf( (*info)[0], length, "        (%s)  Expression %d   (%d/%d)", db_gen_exclusion_id( 'E', exp->id ), exp->ulid, hit, total );
    } else {
      rv = snprintf( (*info)[0], length, "        Expression %d   (%d/%d)", exp->ulid, hit, total );
    }
    assert( rv < length );

    length = 25 + strlen( op ) + strlen( spaces );
    (*info)[1] = (char*)malloc_safe( length );
    rv = snprintf( (*info)[1], length, "%s        ^^^^^^^^^^^^^ - %s", spaces, op );
    assert( rv < length );

    if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {

      if( report_bitwise && (exp->value->width > 1) ) {

        unsigned int i;
 
        length = 30 + strlen( spaces );
        (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s          Bit | LR | LR | LR ", spaces );  assert( rv < length );
        (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        ======|=0-=|=-0=|=11=", spaces );  assert( rv < length );
        (*info)[5] = (char*)malloc_safe( length );  rv = snprintf( (*info)[5], length, "%s        ------|----|----|----", spaces );  assert( rv < length );

        length = 28 + strlen( spaces );
        (*info)[4] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[4], length, "%s          All | %c    %c    %c", spaces,
                       (exp->suppl.part.eval_01 ? ' ' : '*'),
                       (exp->suppl.part.eval_10 ? ' ' : '*'),
                       (exp->suppl.part.eval_11 ? ' ' : '*') );
        assert( rv < length );
        for( i=0; i<exp->value->width; i++ ) {
          (*info)[i+6] = (char*)malloc_safe( length );
          rv = snprintf( (*info)[i+6], length, "%s         %4u | %c    %c    %c", spaces, i,
                         ((vector_get_eval_a( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_b( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_c( exp->value, i ) == 1) ? ' ' : '*') );
          assert( rv < length );
        }

      } else {

        length = 23 + strlen( spaces );
        (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s         LR | LR | LR ", spaces );  assert( rv < length );
        (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        =0-=|=-0=|=11=", spaces );  assert( rv < length );

        length = 21 + strlen( spaces );
        (*info)[4] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[4], length, "%s         %c    %c    %c", spaces,
                       (exp->suppl.part.eval_01 ? ' ' : '*'),
                       (exp->suppl.part.eval_10 ? ' ' : '*'),
                       (exp->suppl.part.eval_11 ? ' ' : '*') );
        assert( rv < length );

      }

    } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {

      if( report_bitwise && (exp->value->width > 1) ) {

        unsigned int i;

        length = 30 + strlen( spaces );
        (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s          Bit | LR | LR | LR ", spaces );  assert( rv < length );
        (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        ======|=1-=|=-1=|=00=", spaces );  assert( rv < length );
        (*info)[5] = (char*)malloc_safe( length );  rv = snprintf( (*info)[5], length, "%s        ------|----|----|----", spaces );  assert( rv < length );

        length = 28 + strlen( spaces );
        (*info)[4] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[4], length, "%s          All | %c    %c    %c", spaces,
                       (exp->suppl.part.eval_10 ? ' ' : '*'),
                       (exp->suppl.part.eval_01 ? ' ' : '*'),
                       (exp->suppl.part.eval_00 ? ' ' : '*') );
        assert( rv < length );
        for( i=0; i<exp->value->width; i++ ) {
          (*info)[i+6] = (char*)malloc_safe( length );
          rv = snprintf( (*info)[i+6], length, "%s         %4u | %c    %c    %c", spaces, i,
                         ((vector_get_eval_a( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_b( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_c( exp->value, i ) == 1) ? ' ' : '*') );
          assert( rv < length );
        }

      } else {

        length = 23 + strlen( spaces );
        (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s         LR | LR | LR ", spaces );  assert( rv < length );
        (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        =1-=|=-1=|=00=", spaces );  assert( rv < length );

        length = 21 + strlen( spaces );
        (*info)[4] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[4], length, "%s         %c    %c    %c", spaces,
                       (exp->suppl.part.eval_10 ? ' ' : '*'),
                       (exp->suppl.part.eval_01 ? ' ' : '*'),
                       (exp->suppl.part.eval_00 ? ' ' : '*') );
        assert( rv < length );

      }

    } else {

      if( report_bitwise && (exp->value->width > 1) ) {

        unsigned int i;

        length = 35 + strlen( spaces );
        (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s          Bit | LR | LR | LR | LR ", spaces );  assert( rv < length );
        (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        ======|=00=|=01=|=10=|=11=", spaces );  assert( rv < length );
        (*info)[5] = (char*)malloc_safe( length );  rv = snprintf( (*info)[5], length, "%s        ------|----|----|----|----", spaces );  assert( rv < length );

        length = 33 + strlen( spaces );
        (*info)[4] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[4], length, "%s          All | %c    %c    %c    %c", spaces,
                       (exp->suppl.part.eval_00 ? ' ' : '*'),
                       (exp->suppl.part.eval_01 ? ' ' : '*'),
                       (exp->suppl.part.eval_10 ? ' ' : '*'),
                       (exp->suppl.part.eval_11 ? ' ' : '*') );
        assert( rv < length );
        for( i=0; i<exp->value->width; i++ ) {
          (*info)[i+6] = (char*)malloc_safe( length );
          rv = snprintf( (*info)[i+6], length, "%s         %4u | %c    %c    %c    %c", spaces, i,
                         ((vector_get_eval_a( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_b( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_c( exp->value, i ) == 1) ? ' ' : '*'),
                         ((vector_get_eval_d( exp->value, i ) == 1) ? ' ' : '*') );
          assert( rv < length );
        }

      } else {

        length = 28 + strlen( spaces );
        (*info)[2] = (char*)malloc_safe( length );  rv = snprintf( (*info)[2], length, "%s         LR | LR | LR | LR ", spaces );  assert( rv < length );
        (*info)[3] = (char*)malloc_safe( length );  rv = snprintf( (*info)[3], length, "%s        =00=|=01=|=10=|=11=", spaces );  assert( rv < length );
  
        length = 26 + strlen( spaces );
        (*info)[4] = (char*)malloc_safe( length );
        rv = snprintf( (*info)[4], length, "%s         %c    %c    %c    %c", spaces,
                       (exp->suppl.part.eval_00 ? ' ' : '*'),
                       (exp->suppl.part.eval_01 ? ' ' : '*'),
                       (exp->suppl.part.eval_10 ? ' ' : '*'),
                       (exp->suppl.part.eval_11 ? ' ' : '*') );
        assert( rv < length );

      }

    }

  }

  PROFILE_END;

}

/*!
 Creates the verbose report information for a multi-variable expression, storing its output in
 the line1, line2, and line3 strings.
*/
static void combination_multi_var_exprs(
  /*@out@*/ char**      line1,  /*!< Pointer to first line of multi-variable expression output */
  /*@out@*/ char**      line2,  /*!< Pointer to second line of multi-variable expression output */
  /*@out@*/ char**      line3,  /*!< Pointer to third line of multi-variable expression output */
            expression* exp     /*!< Pointer to current expression to output */
) { PROFILE(COMBINATION_MULTI_VAR_EXPRS);

  char*        left_line1  = NULL;
  char*        left_line2  = NULL;
  char*        left_line3  = NULL;
  char*        right_line1 = NULL;
  char*        right_line2 = NULL;
  char*        right_line3 = NULL;
  char         curr_id_str[20];
  unsigned int curr_id_str_len;
  unsigned int i;
  bool         and_op;
  unsigned int rv;                  /* Return value from snprintf calls */

  if( exp != NULL ) {

    and_op = (exp->op == EXP_OP_AND) || (exp->op == EXP_OP_LAND);

    /* If we have hit the left-most expression, start creating string here */
    if( (exp->left != NULL) && (exp->op != exp->left->op) ) {

      assert( exp->left->ulid != -1 );
      rv = snprintf( curr_id_str, 20, "%d", exp->left->ulid );
      assert( rv < 20 );
      curr_id_str_len = strlen( curr_id_str );
      left_line1 = (char*)malloc_safe( curr_id_str_len + 4 );
      left_line2 = (char*)malloc_safe( curr_id_str_len + 4 );
      left_line3 = (char*)malloc_safe( curr_id_str_len + 4 );
      rv = snprintf( left_line1, (curr_id_str_len + 4), " %s |", curr_id_str );
      assert( rv < (curr_id_str_len + 4) );
      for( i=0; i<(curr_id_str_len-1); i++ ) {
        curr_id_str[i] = '=';
      }
      curr_id_str[i] = '\0'; 
      if( and_op ) { 
        rv = snprintf( left_line2, (curr_id_str_len + 4), "=0%s=|", curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      } else { 
        rv = snprintf( left_line2, (curr_id_str_len + 4), "=1%s=|", curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      }
      for( i=0; i<(curr_id_str_len - 1); i++ ) {
        curr_id_str[i] = ' ';
      }
      curr_id_str[i] = '\0';
      if( and_op ) {
        rv = snprintf( left_line3, (curr_id_str_len + 4), " %c%s  ", (exp->suppl.part.eval_01 ? ' ' : '*'), curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      } else {
        rv = snprintf( left_line3, (curr_id_str_len + 4), " %c%s  ", (exp->suppl.part.eval_10 ? ' ' : '*'), curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      }

    } else {

      combination_multi_var_exprs( &left_line1, &left_line2, &left_line3, exp->left );

    }

    /* Get right-side information */
    if( (exp->right != NULL) && (exp->op != exp->right->op) ) {

      assert( exp->right->ulid != -1 );
      rv = snprintf( curr_id_str, 20, "%d", exp->right->ulid );
      assert( rv < 20 );
      curr_id_str_len = strlen( curr_id_str );
      right_line1 = (char*)malloc_safe( curr_id_str_len + 4 );
      right_line2 = (char*)malloc_safe( curr_id_str_len + 4 );
      right_line3 = (char*)malloc_safe( curr_id_str_len + 4 );
      rv = snprintf( right_line1, (curr_id_str_len + 4), " %s |", curr_id_str );
      assert( rv < (curr_id_str_len + 4) );
      for( i=0; i<(curr_id_str_len-1); i++ ) {
        curr_id_str[i] = '=';
      }
      curr_id_str[i] = '\0';
      if( and_op ) {
        rv = snprintf( right_line2, (curr_id_str_len + 4), "=0%s=|", curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      } else {
        rv = snprintf( right_line2, (curr_id_str_len + 4), "=1%s=|", curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      }
      for( i=0; i<(curr_id_str_len - 1); i++ ) {
        curr_id_str[i] = ' ';
      }
      curr_id_str[i] = '\0';
      if( and_op ) {
        rv = snprintf( right_line3, (curr_id_str_len + 4), " %c%s  ", (exp->suppl.part.eval_10 ? ' ' : '*'), curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      } else {
        rv = snprintf( right_line3, (curr_id_str_len + 4), " %c%s  ", (exp->suppl.part.eval_01 ? ' ' : '*'), curr_id_str );
        assert( rv < (curr_id_str_len + 4) );
      }

    } else {

      combination_multi_var_exprs( &right_line1, &right_line2, &right_line3, exp->right );

    }

    if( left_line1 != NULL ) {
      if( right_line1 != NULL ) {
        unsigned int slen1 = strlen( left_line1 ) + strlen( right_line1 ) + 1;
        unsigned int slen2 = strlen( left_line2 ) + strlen( right_line2 ) + 1;
        unsigned int slen3 = strlen( left_line3 ) + strlen( right_line3 ) + 1;
        *line1 = (char*)malloc_safe( slen1 );
        *line2 = (char*)malloc_safe( slen2 );
        *line3 = (char*)malloc_safe( slen3 );
        rv = snprintf( *line1, slen1, "%s%s", left_line1, right_line1 );
        assert( rv < slen1 );
        rv = snprintf( *line2, slen2, "%s%s", left_line2, right_line2 );
        assert( rv < slen2 );
        rv = snprintf( *line3, slen3, "%s%s", left_line3, right_line3 );
        assert( rv < slen3 );
        free_safe( left_line1, (strlen( left_line1 ) + 1) );
        free_safe( left_line2, (strlen( left_line2 ) + 1) );
        free_safe( left_line3, (strlen( left_line3 ) + 1) );
        free_safe( right_line1, (strlen( right_line1 ) + 1) );
        free_safe( right_line2, (strlen( right_line2 ) + 1) );
        free_safe( right_line3, (strlen( right_line3 ) + 1) );
      } else {
        *line1 = left_line1;
        *line2 = left_line2;
        *line3 = left_line3;
      }
    } else {
      assert( right_line1 != NULL );
      *line1 = right_line1;
      *line2 = right_line2;
      *line3 = right_line3;
    }

    /* If we are the root, output all value */
    if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ) {
      unsigned int slen1 = strlen( *line1 ) + 5;
      unsigned int slen2 = strlen( *line2 ) + 6;
      unsigned int slen3 = strlen( *line3 ) + 6;
      left_line1 = *line1;
      left_line2 = *line2;
      left_line3 = *line3;
      *line1     = (char*)malloc_safe( slen1 );
      *line2     = (char*)malloc_safe( slen2 );
      *line3     = (char*)malloc_safe( slen3 );
      if( and_op ) {
        rv = snprintf( *line1, slen1, "%s All",   left_line1 );
        assert( rv < slen1 );
        rv = snprintf( *line2, slen2, "%s==1==",  left_line2 );
        assert( rv < slen2 );
        rv = snprintf( *line3, slen3, "%s  %c  ", left_line3, (exp->suppl.part.eval_11 ? ' ' : '*') );
        assert( rv < slen3 );
      } else {
        rv = snprintf( *line1, slen1, "%s All",   left_line1 );
        assert( rv < slen1 );
        rv = snprintf( *line2, slen2, "%s==0==",  left_line2 );
        assert( rv < slen2 );
        rv = snprintf( *line3, slen3, "%s  %c  ", left_line3, (exp->suppl.part.eval_00 ? ' ' : '*') );
        assert( rv < slen3 );
      }
      free_safe( left_line1, (strlen( left_line1 ) + 1) );
      free_safe( left_line2, (strlen( left_line2 ) + 1) );
      free_safe( left_line3, (strlen( left_line3 ) + 1) );
    }

  }

  PROFILE_END;

}

/*!
 \return Returns the number of lines required to store the multi-variable expression output
         contained in line1, line2, and line3.
*/
static int combination_multi_expr_output_length(
  char* line1  /*!< First line of multi-variable expression output */
) { PROFILE(COMBINATION_MULTI_EXPR_OUTPUT_LENGTH);

  int start  = 0;
  int i;
  int len    = strlen( line1 );
  int length = 0;

  for( i=0; i<len; i++ ) {
    if( (i + 1) == len ) {
      length += 3;
    } else if( (line1[i] == '|') && ((i - start) >= line_width) ) {
      length += 3;
      start   = i + 1;
    }
  }

  PROFILE_END;

  return( length );

}

/*!
 Stores the information from line1, line2 and line3 in the string array info.
*/
static void combination_multi_expr_output(
  char** info,   /*!< Pointer string to output report contents to */
  char*  line1,  /*!< First line of multi-variable expression output */
  char*  line2,  /*!< Second line of multi-variable expression output */
  char*  line3   /*!< Third line of multi-variable expression output */
) { PROFILE(COMBINATION_MULTI_EXPR_OUTPUT);

  int          start      = 0;
  int          i;
  int          len        = strlen( line1 );
  int          info_index = 2;
  unsigned int eid_size;

  if( flag_output_exclusion_ids ) {
    eid_size = db_get_exclusion_id_size();
  }

  for( i=0; i<len; i++ ) {

    if( (i + 1) == len ) {

      unsigned int rv;
      unsigned int slen1 = strlen( line1 + start ) + 9;
      unsigned int slen2 = strlen( line2 + start ) + 9;
      unsigned int slen3 = strlen( line3 + start ) + 9;

      if( flag_output_exclusion_ids ) {
        slen1 += (eid_size - 1) + 4;
        slen2 += (eid_size - 1) + 4;
        slen3 += (eid_size - 1) + 4;
      }

      info[info_index+0] = (char*)malloc_safe( slen1 );
      info[info_index+1] = (char*)malloc_safe( slen2 );
      info[info_index+2] = (char*)malloc_safe( slen3 );

      if( flag_output_exclusion_ids ) {
        char tmp[30];
        gen_char_string( tmp, ' ', ((eid_size - 1) + 4) );
        rv = snprintf( info[info_index+0], slen1, "%s        %s", tmp, (line1 + start) );
        assert( rv < slen1 );
        rv = snprintf( info[info_index+1], slen2, "%s        %s", tmp, (line2 + start) );
        assert( rv < slen2 );
        rv = snprintf( info[info_index+2], slen3, "%s        %s", tmp, (line3 + start) );
        assert( rv < slen3 );
      } else {
        rv = snprintf( info[info_index+0], slen1, "        %s", (line1 + start) );
        assert( rv < slen1 );
        rv = snprintf( info[info_index+1], slen2, "        %s", (line2 + start) );
        assert( rv < slen2 );
        rv = snprintf( info[info_index+2], slen3, "        %s", (line3 + start) );
        assert( rv < slen3 );
      }

    } else if( (line1[i] == '|') && ((i - start) >= line_width) ) {

      unsigned int rv;
      unsigned int slen1;
      unsigned int slen2;
      unsigned int slen3;

      line1[i] = '\0';
      line2[i] = '\0';
      line3[i] = '\0';

      slen1 = strlen( line1 + start ) + 10;
      slen2 = strlen( line2 + start ) + 10;
      slen3 = strlen( line3 + start ) + 11;

      if( flag_output_exclusion_ids ) {
        slen1 += (eid_size - 1) + 4;
        slen2 += (eid_size - 1) + 4;
        slen3 += (eid_size - 1) + 4;
      }

      info[info_index+0] = (char*)malloc_safe( slen1 );
      info[info_index+1] = (char*)malloc_safe( slen2 );
      info[info_index+2] = (char*)malloc_safe( slen3 );

      if( flag_output_exclusion_ids ) {
        char tmp[30];
        gen_char_string( tmp, ' ', ((eid_size - 1) + 4) );
        rv = snprintf( info[info_index+0], slen1, "%s        %s|",   tmp, (line1 + start) );
        assert( rv < slen1 );
        rv = snprintf( info[info_index+1], slen2, "%s        %s|",   tmp, (line2 + start) );
        assert( rv < slen2 );
        rv = snprintf( info[info_index+2], slen3, "%s        %s \n", tmp, (line3 + start) );
        assert( rv < slen3 );
      } else {
        rv = snprintf( info[info_index+0], slen1, "        %s|",   (line1 + start) );
        assert( rv < slen1 );
        rv = snprintf( info[info_index+1], slen2, "        %s|",   (line2 + start) );
        assert( rv < slen2 );
        rv = snprintf( info[info_index+2], slen3, "        %s \n", (line3 + start) );
        assert( rv < slen3 );
      }

      start       = i + 1;
      info_index += 3;

    }

  }

  PROFILE_END;

}

/*!
 Displays the missed combinational sequences for the specified expression to the
 specified output stream in tabular form.
*/
static void combination_multi_vars(
  char***     info,          /*!< Pointer to character array containing coverage information to output */
  int*        info_size,     /*!< Pointer to integer containing number of valid array entries in info */
  expression* exp,           /*!< Pointer to top-level AND/OR expression to evaluate */
  bool        show_excluded  /*!< Set to TRUE to output expression if it is excluded */
) { PROFILE(COMBINATION_MULTI_VARS);

  int          ulid      = 1;
  unsigned int hit       = 0;
  unsigned int excluded  = 0;
  unsigned int total     = 0;
  char*        line1     = NULL;
  char*        line2     = NULL;
  char*        line3     = NULL;
  char         tmp[20];
  unsigned int line_size = 1;

  /* Only output this expression if we are missing coverage. */
  if( exp->ulid != -1 ) {

    /* Calculate hit and total values for this sub-expression */
    combination_multi_expr_calc( exp, &ulid, FALSE, FALSE, &hit, &excluded, &total );

    if( ((hit != total) && !ESUPPL_EXCLUDED( exp->suppl )) ||
        (ESUPPL_EXCLUDED( exp->suppl ) && show_excluded) ) {

      unsigned int rv;
      unsigned int slen1;
      unsigned int slen2;
      unsigned int slen3;
      unsigned int eid_size;

      if( flag_output_exclusion_ids ) {
        eid_size = db_get_exclusion_id_size();
      }

      /* Gather report output for this expression */
      combination_multi_var_exprs( &line1, &line2, &line3, exp );

      /* Get the lengths of the original string lengths -- these strings will be possibly altered by combination_multi_expr_output */
      slen1 = strlen( line1 ) + 1;
      slen2 = strlen( line2 ) + 1;
      slen3 = strlen( line3 ) + 1;

      /* Calculate the array needed to store the output information and allocate this memory */
      *info_size = combination_multi_expr_output_length( line1 ) + 2;
      *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size) );

      /* Calculate needed line length */
      rv = snprintf( tmp, 20, "%u", hit );
      assert( rv < 20 );
      line_size += strlen( tmp );
      rv = snprintf( tmp, 20, "%u", total );
      assert( rv < 20 );
      line_size += strlen( tmp );
      rv = snprintf( tmp, 20, "%d", exp->ulid );
      assert( rv < 20 );
      line_size += strlen( tmp );
      line_size += 25;                   /* Number of additional characters in line below */
      if( flag_output_exclusion_ids ) {
        line_size += (eid_size - 1) + 4;
      }
      (*info)[0] = (char*)malloc_safe( line_size );
    
      if( flag_output_exclusion_ids ) {
        char* tmp;
        char  spaces[30];
        int   size;
        rv = snprintf( (*info)[0], line_size, "        (%s)  Expression %d   (%u/%u)", db_gen_exclusion_id( 'E', exp->id ), exp->ulid, hit, total );
        assert( rv < line_size );
        switch( exp->op ) {
          case EXP_OP_AND  :  tmp = strdup_safe( "        ^^^^^^^^^^^^^ - &" );   break;
          case EXP_OP_OR   :  tmp = strdup_safe( "        ^^^^^^^^^^^^^ - |" );   break;
          case EXP_OP_LAND :  tmp = strdup_safe( "        ^^^^^^^^^^^^^ - &&" );  break;
          case EXP_OP_LOR  :  tmp = strdup_safe( "        ^^^^^^^^^^^^^ - ||" );  break;
          default          :  assert( 0 );  break;
        }
        size = strlen( tmp ) + (eid_size - 1) + 5;
        gen_char_string( spaces, ' ', (eid_size - 1) );
        (*info)[1] = (char*)malloc_safe( size );
        rv = snprintf( (*info)[1], size, "%s    %s", spaces, tmp );
        assert( rv < size );
        free_safe( tmp, (strlen( tmp ) + 1) );
      } else {
        rv = snprintf( (*info)[0], line_size, "        Expression %d   (%u/%u)", exp->ulid, hit, total );
        assert( rv < line_size );
        switch( exp->op ) {
          case EXP_OP_AND  :  (*info)[1] = strdup_safe( "        ^^^^^^^^^^^^^ - &" );   break;
          case EXP_OP_OR   :  (*info)[1] = strdup_safe( "        ^^^^^^^^^^^^^ - |" );   break;
          case EXP_OP_LAND :  (*info)[1] = strdup_safe( "        ^^^^^^^^^^^^^ - &&" );  break;
          case EXP_OP_LOR  :  (*info)[1] = strdup_safe( "        ^^^^^^^^^^^^^ - ||" );  break;
          default          :  assert( 0 );  break;
        }
      }

      /* Output the lines paying attention to the current line width */
      combination_multi_expr_output( *info, line1, line2, line3 );

      free_safe( line1, slen1 );
      free_safe( line2, slen2 );
      free_safe( line3, slen3 );

    }

  }

  PROFILE_END;

}

/*!
 Calculates the missed coverage detail output for the given expression, placing the output to the info array.  This
 array can then be sent to an ASCII report file or the GUI.
*/
static void combination_get_missed_expr(
  char***      info,          /*!< Pointer to an array of strings containing expression coverage detail */
  int*         info_size,     /*!< Pointer to a value that will be set to indicate the number of valid elements in the info array */
  expression*  exp,           /*!< Pointer to the expression to get the coverage detail for */
  unsigned int curr_depth,    /*!< Current expression depth (used to figure out when to stop getting coverage information -- if
                                   the user has specified a maximum depth) */
  bool         show_excluded  /*!< Set to TRUE if excluded expressions should be output */
) { PROFILE(COMBINATION_GET_MISSED_EXPR);

  assert( exp != NULL );

  *info_size = 0;

  if( EXPR_COMB_MISSED( exp ) &&
      (!info_suppl.part.inlined || (curr_depth <= inline_comb_depth)) &&
      (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
        (report_comb_depth == REPORT_VERBOSE)) ) {
 
    if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ||
        ((exp->op != EXP_OP_AND)  &&
         (exp->op != EXP_OP_LAND) &&
         (exp->op != EXP_OP_OR)   &&
         (exp->op != EXP_OP_LOR)) ) {

      if( (((exp->left != NULL) &&
            (exp->op == exp->left->op)) ||
           ((exp->right != NULL) &&
            (exp->op == exp->right->op))) &&
          ((exp->op == EXP_OP_AND)  ||
           (exp->op == EXP_OP_OR)   ||
           (exp->op == EXP_OP_LAND) ||
           (exp->op == EXP_OP_LOR)) ) {

        combination_multi_vars( info, info_size, exp, show_excluded );

      } else {

        /* Create combination table */
        if( EXPR_IS_COMB( exp ) ) {
          combination_two_vars( info, info_size, exp, show_excluded );
        } else if( EXPR_IS_EVENT( exp ) ) {
          combination_event( info, info_size, exp, show_excluded );
        } else {
          combination_unary( info, info_size, exp, show_excluded );
        }

      }

    }

  }

  PROFILE_END;

}

/*!
 Describe which combinations were not hit for all subexpressions in the
 specified expression tree.  We display the value of missed combinations by
 displaying the combinations of the children expressions that were not run
 during simulation.
*/
static void combination_list_missed(
  FILE*        ofile,           /*!< Pointer to file to output results to */
  expression*  exp,             /*!< Pointer to expression tree to evaluate */
  unsigned int curr_depth,      /*!< Specifies current depth of expression tree */
  func_unit*   funit,           /*!< Pointer to current functional unit */
  bool         show_exclusions  /*!< Set to TRUE to display exclusions */
) { PROFILE(COMBINATION_LIST_MISSED);

  char** info;       /* String array containing combination coverage information for this expression */
  int    info_size;  /* Specifies the number of valid entries in the info array */
  int    i;          /* Loop iterator */
  
  if( exp != NULL ) {

    exclude_reason* er;
    
    combination_list_missed( ofile, exp->left,  combination_calc_depth( exp, curr_depth, TRUE ),  funit, show_exclusions );
    combination_list_missed( ofile, exp->right, combination_calc_depth( exp, curr_depth, FALSE ), funit, show_exclusions );

    /* Get coverage information for this expression */
    combination_get_missed_expr( &info, &info_size, exp, curr_depth, show_exclusions );

    /* If there was any coverage information for this expression, output it to the specified output stream */
    if( info_size > 0 ) {

      for( i=0; i<info_size; i++ ) {

        fprintf( ofile, "%s\n", info[i] );
        free_safe( info[i], (strlen( info[i] ) + 1) );

      }

      /* Output the exclusion reason information, if it exists */
      if( report_exclusions && (er = exclude_find_exclude_reason( 'E', exp->id, funit )) != NULL ) {
        if( flag_output_exclusion_ids ) {
          report_output_exclusion_reason( ofile, (14 + (db_get_exclusion_id_size() - 1)), er->reason, TRUE );
        } else {
          report_output_exclusion_reason( ofile, 10, er->reason, TRUE );
        }
      }

      fprintf( ofile, "\n" );

      free_safe( info, (sizeof( char* ) * info_size) );

    }

  }

  PROFILE_END;

}

/*!
 Recursively traverses specified expression tree, returning TRUE
 if an expression is found that has not received 100% coverage for
 combinational logic.
*/
static void combination_output_expr(
            expression*  expr,            /*!< Pointer to root of expression tree to search */
            unsigned int curr_depth,      /*!< Specifies current depth of expression tree */
  /*@out@*/ int*         any_missed,      /*!< Pointer to indicate if any subexpressions were missed in the specified expression */
  /*@out@*/ int*         any_measurable,  /*!< Pointer to indicate if any subexpressions were measurable in the specified expression */
  /*@out@*/ int*         any_excluded,    /*!< Pointer to indicate if any subexpressions were excluded */
  /*@out@*/ int*         all_excluded     /*!< Pointer to indicate if all subexpressions that can be excluded are */
) { PROFILE(COMBINATION_OUTPUT_EXPR);

  if( (expr != NULL) && (ESUPPL_WAS_COMB_COUNTED( expr->suppl ) == 1) ) {

    expr->suppl.part.comb_cntd = 0;

    combination_output_expr( expr->right, combination_calc_depth( expr, curr_depth, FALSE ), any_missed, any_measurable, any_excluded, all_excluded );
    combination_output_expr( expr->left,  combination_calc_depth( expr, curr_depth, TRUE ),  any_missed, any_measurable, any_excluded, all_excluded );

    if( (!info_suppl.part.inlined || (curr_depth <= inline_comb_depth)) &&
        (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
         (report_comb_depth == REPORT_VERBOSE)) ) {
 
      if( expr->ulid != -1 ) {
        *any_missed = 1;
      }
      if( (EXPR_IS_MEASURABLE( expr ) == 1) && !expression_is_static_only( expr ) ) {
        if( ESUPPL_EXCLUDED( expr->suppl ) == 0 ) {
          *any_measurable = 1;
          if( expr->ulid != -1 ) {
            *all_excluded = 0;
          }
        } else {
          *any_excluded = 1;
          *all_excluded = 1;
        }
      }

    }

  }

  PROFILE_END;

}

/*!
 \throws anonymous combination_underline

 Displays the expressions (and groups of expressions) that were considered 
 to be measurable (evaluates to a value of TRUE (1) or FALSE (0) but were 
 not hit during simulation.  The entire Verilog expression is displayed
 to the specified output stream with each of its measured expressions being
 underlined and numbered.  The missed combinations are then output below
 the Verilog code, showing those logical combinations that were not hit
 during simulation.
*/
static void combination_display_verbose(
  FILE*      ofile,  /*!< Pointer to file to output results to */
  func_unit* funit,  /*!< Pointer to functional unit to display verbose combinational logic output for */
  rpt_type   rtype   /*!< Specifies the type of report to display */
) { PROFILE(COMBINATION_DISPLAY_VERBOSE);

  func_iter    fi;          /* Functional unit iterator */
  statement*   stmt;        /* Pointer to current statement */
  char**       code;        /* Code string from code generator */
  unsigned int code_depth;  /* Depth of code array */

  switch( rtype ) {
    case RPT_TYPE_HIT  :  fprintf( ofile, "    Hit Combinations\n\n" );                         break;
    case RPT_TYPE_MISS :  fprintf( ofile, "    Missed Combinations  (* = missed value)\n\n" );  break;
    case RPT_TYPE_EXCL :  fprintf( ofile, "    Excluded Combinations\n\n" );                    break;
  }

  /* Initialize functional unit iterator */
  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

  /* Display missed combinations */
  stmt = func_iter_get_next_statement( &fi );
  while( stmt != NULL ) {

    int any_missed     = 0;
    int any_measurable = 0;
    int any_excluded   = 0;
    int all_excluded   = 0;

    combination_output_expr( stmt->exp, 0, &any_missed, &any_measurable, &any_excluded, &all_excluded );

    if( ((rtype == RPT_TYPE_MISS) && (any_missed == 1) && (all_excluded == 0) && (any_measurable == 1)) ||
        ((rtype == RPT_TYPE_HIT)  && (any_missed == 0) && (any_measurable == 1)) ||
        ((rtype == RPT_TYPE_EXCL) && (any_excluded == 1)) ) {

      stmt->exp->suppl.part.comb_cntd = 0;

      fprintf( ofile, "      =========================================================================================================\n" );
      fprintf( ofile, "       Line #     Expression\n" );
      fprintf( ofile, "      =========================================================================================================\n" );

      /* Generate line of code that missed combinational coverage */
      codegen_gen_expr( stmt->exp, funit, &code, &code_depth );

      /* Output underlining feature for missed expressions */
      combination_underline( ofile, code, code_depth, stmt->exp, funit );
      fprintf( ofile, "\n" );

      /* Output logical combinations that missed complete coverage */
      combination_list_missed( ofile, stmt->exp, 0, funit, (rtype == RPT_TYPE_EXCL) );

    }
    
    stmt = func_iter_get_next_statement( &fi );

  }

  /* Deallocate functional unit iterator */
  func_iter_dealloc( &fi );

  PROFILE_END;

}

/*!
 \throws anonymous combination_display_verbose combination_instance_verbose

 Outputs the verbose coverage report for the specified functional unit instance
 to the specified output stream.
*/
static void combination_instance_verbose(
  FILE*       ofile,  /*!< Pointer to file to output results to */
  funit_inst* root,   /*!< Pointer to current functional unit instance to evaluate */
  char*       parent  /*!< Name of parent instance */
) { PROFILE(COMBINATION_INSTANCE_VERBOSE);

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder of instance */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );

  /* Get printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) || root->suppl.name_diff ) {
    strcpy( tmpname, parent );
  } else if( strcmp( parent, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && !funit_is_unnamed( root->funit ) &&
      (((root->stat->comb_hit < root->stat->comb_total) && !report_covered) ||
       ((root->stat->comb_hit > 0) && report_covered) ||
       ((root->stat->comb_excluded > 0) && report_exclusions)) ) {

    /* Get printable version of functional unit name */
    pname = scope_gen_printable( funit_flatten_name( root->funit ) );

    fprintf( ofile, "\n" );
    switch( root->funit->suppl.part.type ) {
      case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_AFUNCTION    :
      case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_ATASK        :
      case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
      default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, obf_file( root->funit->orig_fname ), tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

    free_safe( pname, (strlen( pname ) + 1) );

    if( ((root->stat->comb_hit < root->stat->comb_total) && !report_covered) ||
        ((root->stat->comb_hit > 0) && report_covered && (!report_exclusions || (root->stat->comb_hit > root->stat->comb_excluded))) ) {
      combination_display_verbose( ofile, root->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    }
    if( report_exclusions && (root->stat->comb_excluded > 0) ) {
      combination_reset_counted_exprs( root->funit );
      combination_display_verbose( ofile, root->funit, RPT_TYPE_EXCL );
    }

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    combination_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

  PROFILE_END;

}

/*!
 \throws anonymous combination_display_verbose

 Outputs the verbose coverage report for the specified functional unit
 to the specified output stream.
*/
static void combination_funit_verbose(
  FILE*       ofile,  /*!< Pointer to file to output results to */
  funit_link* head    /*!< Pointer to current functional unit to evaluate */
) { PROFILE(COMBINATION_FUNIT_VERBOSE);

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        (((head->funit->stat->comb_hit < head->funit->stat->comb_total) && !report_covered) ||
         ((head->funit->stat->comb_hit > 0) && report_covered) ||
         ((head->funit->stat->comb_excluded > 0) && report_exclusions)) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      fprintf( ofile, "\n" );
      switch( head->funit->suppl.part.type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", pname, obf_file( head->funit->orig_fname ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      free_safe( pname, (strlen( pname ) + 1) );

      if( ((head->funit->stat->comb_hit < head->funit->stat->comb_total) && !report_covered) ||
          ((head->funit->stat->comb_hit > 0) && report_covered && (!report_exclusions || (head->funit->stat->comb_hit > head->funit->stat->comb_excluded))) ) {
        combination_display_verbose( ofile, head->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
      }
      if( (head->funit->stat->comb_excluded > 0) && report_exclusions ) {
        combination_reset_counted_exprs( head->funit );
        combination_display_verbose( ofile, head->funit, RPT_TYPE_EXCL );
      }

    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Gathers the covered or uncovered combinational logic information, storing their expressions in the exprs
 expression arrays.  Used by the GUI for verbose combinational logic output.
*/
void combination_collect(
            func_unit*    funit,    /*!< Pointer to functional unit */
            int           cov,      /*!< Specifies the coverage type to find */
  /*@out@*/ expression*** exprs,    /*!< Pointer to an array of expression pointers that contain all fully covered/uncovered expressions */
  /*@out@*/ unsigned int* exp_cnt,  /*!< Pointer to a value that will be set to indicate the number of expressions in covs array */
  /*@out@*/ int**         excludes  /*!< Pointer to an array of integers indicating exclusion property of each uncovered expression */
) { PROFILE(COMBINATION_COLLECT);

  func_iter  fi;              /* Functional unit iterator */
  statement* stmt;            /* Pointer to current statement */
 
  /* Reset combination counted bits */
  combination_reset_counted_exprs( funit );

  /* Create an array that will hold the number of uncovered combinations */
  *exp_cnt  = 0;
  *exprs    = NULL;
  *excludes = NULL;

  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

  stmt = func_iter_get_next_statement( &fi );
  while( stmt != NULL ) {

    int any_missed     = 0;
    int any_measurable = 0;
    int any_excluded   = 0;
    int all_excluded   = 0;

    combination_output_expr( stmt->exp, 0, &any_missed, &any_measurable, &any_excluded, &all_excluded );

    /* Check for uncovered statements */
    if( ((cov == 0) && (any_missed == 1)) ||
        ((cov == 1) && (any_missed == 0) && (any_measurable == 1)) ) {
      if( stmt->exp->line != 0 ) {
        *exprs    = (expression**)realloc_safe( *exprs,    (sizeof( expression* ) * (*exp_cnt)), (sizeof( expression* ) * (*exp_cnt + 1)) );
        *excludes =         (int*)realloc_safe( *excludes, (sizeof( int* )        * (*exp_cnt)), (sizeof( int* )        * (*exp_cnt + 1)) );

        (*exprs)[(*exp_cnt)]    = stmt->exp;
        (*excludes)[(*exp_cnt)] = all_excluded ? 1 : 0;
        (*exp_cnt)++;
      }
      stmt->exp->suppl.part.comb_cntd = 0;
    }

    stmt = func_iter_get_next_statement( &fi );

  }

  func_iter_dealloc( &fi );

  PROFILE_END;

}

/*!
 Recursively iterates through the specified expression tree, storing the exclude values
 for each underlined expression within the tree.  The values are stored in the excludes
 parameter and its size is stored in the exclude_size parameter.
*/
static void combination_get_exclude_list(
            expression*   exp,          /*!< Pointer to current expression */
            func_unit*    funit,        /*!< Pointer to functional unit containing the exclude reasons for this expression tree */
  /*@out@*/ int**         excludes,     /*!< Array of exclude values for each underlined expression in this tree */
  /*@out@*/ char***       reasons,      /*!< Array of exclude reasons for each underlined expression in this tree */
  /*@out@*/ unsigned int* exclude_size  /*!< Number of elements in the excludes array */
) { PROFILE(COMBINATION_GET_EXCLUDE_LIST);

  if( exp != NULL ) {

    /* Store the exclude value for this expression */
    if( exp->ulid != -1 ) {

      exclude_reason* er;
     
      if( (exp->ulid > 0) && ((unsigned int)exp->ulid > *exclude_size) ) {
        int i;
        *excludes = (int*)realloc_safe( *excludes, (sizeof( int ) * exp->ulid), (sizeof( int ) * (exp->ulid + 1)) );
        *reasons  = (char**)realloc_safe( *reasons, (sizeof( int ) * exp->ulid), (sizeof( int ) * (exp->ulid + 1)) );
        for( i=*exclude_size; i<exp->ulid; i++ ) {
          (*excludes)[i] = 0;
          (*reasons)[i]  = NULL;
        }
        *exclude_size = exp->ulid + 1;
      }

      (*excludes)[exp->ulid] = ESUPPL_EXCLUDED( exp->suppl );

      /* If the expression is currently excluded, check to see if there's a reason associated with it */
      if( (ESUPPL_EXCLUDED( exp->suppl ) == 1) && ((er = exclude_find_exclude_reason( 'E', exp->id, funit )) != NULL) ) {
        (*reasons)[exp->ulid] = strdup_safe( er->reason );
      } else {
        (*reasons)[exp->ulid] = NULL;
      }

    }

    /* Get exclude values for children */
    combination_get_exclude_list( exp->left,  funit, excludes, reasons, exclude_size );
    combination_get_exclude_list( exp->right, funit, excludes, reasons, exclude_size );

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw combination_underline_tree

 Gets the combinational logic coverage information for the specified expression ID, storing the output in the
 code and ulines arrays.  Used by the GUI for displaying an expression's coverage information.
*/
void combination_get_expression(
            int           expr_id,       /*!< Expression ID to retrieve information for */
  /*@out@*/ char***       code,          /*!< Pointer to an array of strings containing generated code for this expression */
  /*@out@*/ int**         uline_groups,  /*!< Pointer to an array of integers used for underlined missed subexpressions in this expression */
  /*@out@*/ unsigned int* code_size,     /*!< Pointer to value that will be set to indicate the number of elements in the code array */
  /*@out@*/ char***       ulines,        /*!< Pointer to an array of strings that contain underlines of missed subexpressions */
  /*@out@*/ unsigned int* uline_size,    /*!< Pointer to value that will be set to indicate the number of elements in the ulines array */
  /*@out@*/ int**         excludes,      /*!< Pointer to an array of values that determine if the associated subexpression is currently
                                              excluded or not from coverage */
  /*@out@*/ char***       reasons,       /*!< Pointer to an array of strings that specify the reason for exclusion */
  /*@out@*/ unsigned int* exclude_size   /*!< Pointer to value that will be set to indicate the number of elements in excludes */
) { PROFILE(COMBINATION_GET_EXPRESSION);

  unsigned int tmp;               /* Temporary integer (unused) */
  unsigned int i, j;              /* Loop iterators */
  char**       tmp_ulines;
  unsigned int tmp_uline_size;
  int          start     = 0;
  unsigned int uline_max = 20;
  func_unit*   funit;
  expression*  exp;

  /* Find functional unit that contains this expression */
  funit = funit_find_by_id( expr_id );
  assert( funit != NULL );

  /* Find the expression itself */
  exp = exp_link_find( expr_id, funit->exps, funit->exp_size );
  assert( exp != NULL );

  /* Generate line of code that missed combinational coverage */
  codegen_gen_expr( exp, funit, code, code_size );
  *uline_groups = (int*)malloc_safe( sizeof( int ) * (*code_size) );

  /* Generate exclude information */
  *excludes     = NULL;
  *reasons      = NULL;
  *exclude_size = 0;
  combination_get_exclude_list( exp, funit_get_curr_module( funit ), excludes, reasons, exclude_size );

  Try {

    /* Output underlining feature for missed expressions */
    combination_underline_tree( exp, 0, &tmp_ulines, &tmp_uline_size, &tmp, exp->op, (*code_size == 1), funit );
  
  } Catch_anonymous {
    unsigned int i;
    free_safe( *uline_groups, (sizeof( int ) * (*code_size)) );
    *uline_groups = NULL;
    for( i=0; i<*code_size; i++ ) {
      free_safe( (*code)[i], (strlen( (*code)[i] ) + 1) );
    }
    free_safe( *code, (sizeof( char* ) * *code_size) );
    *code      = NULL;
    *code_size = 0;
    free_safe( *excludes, (sizeof( int* ) * *exclude_size) );
    *excludes     = NULL;
    *exclude_size = 0;
    Throw 0;
  }

  *ulines     = (char**)malloc_safe( sizeof( char* ) * uline_max );
  *uline_size = 0;

  for( i=0; i<*code_size; i++ ) {

    assert( (*code)[i] != NULL );

    (*uline_groups)[i] = 0;

    if( *code_size == 1 ) {
      *ulines            = tmp_ulines;
      *uline_size        = tmp_uline_size;
      (*uline_groups)[0] = tmp_uline_size;
      tmp_uline_size     = 0;
    } else {
      for( j=0; j<tmp_uline_size; j++ ) {
        if( ((*ulines)[*uline_size] = combination_prep_line( tmp_ulines[j], start, strlen( (*code)[i] ) )) != NULL ) {
          ((*uline_groups)[i])++;
          (*uline_size)++;
          if( *uline_size == uline_max ) {
            uline_max += 20;
            *ulines    = (char**)realloc_safe( *ulines, (sizeof( char* ) * (uline_max - 20)), (sizeof( char* ) * uline_max) );
          }
        }
      }
    }

    start += strlen( (*code)[i] );

  }

  for( i=0; i<tmp_uline_size; i++ ) {
    free_safe( tmp_ulines[i], (strlen( tmp_ulines[i] ) + 1) );
  }

  if( tmp_uline_size > 0 ) {
    free_safe( tmp_ulines, (sizeof( char* ) * tmp_uline_size) );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if we were successful in obtaining coverage detail for the specified information;
         otherwise, returns FALSE to indicate an error.

 Calculates the coverage detail for the specified subexpression and stores this in string format in the
 info and info_size arguments.  The coverage detail created matches the coverage detail output format that
 is used in the ASCII reports.
*/
void combination_get_coverage(
            int        exp_id,    /*!< Expression ID of statement containing subexpression to get coverage detail for */
            int        uline_id,  /*!< Underline ID of subexpression to get coverage detail for */
  /*@out@*/ char***    info,      /*!< Pointer to string array that will be populated with the coverage detail */
  /*@out@*/ int*       info_size  /*!< Number of entries in info array */
) { PROFILE(COMBINATION_GET_COVERAGE);

  func_unit*  funit;  /* Pointer to found functional unit */
  expression* exp;    /* Pointer to found expression */

  /* Find the functional unit that contains this expression */
  funit = funit_find_by_id( exp_id );
  assert( funit != NULL );

  /* Find statement containing this expression */
  exp = exp_link_find( exp_id, funit->exps, funit->exp_size );
  assert( exp != NULL );

  /* Now find the subexpression that matches the given underline ID */
  exp = expression_find_uline_id( exp, uline_id );
  assert( exp != NULL );

  combination_get_missed_expr( info, info_size, exp, 0, TRUE );

  PROFILE_END;

}

/*!
 \throws anonymous combination_funit_verbose combination_instance_verbose

 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the combinational logic coverage for each functional unit encountered.  The parent 
 functional unit will specify its own combinational logic coverage along with a total combinational
 logic coverage including its children.
*/
void combination_report(
  FILE* ofile,   /*!< Pointer to file to output results to */
  bool  verbose  /*!< Specifies whether or not to provide verbose information */
) { PROFILE(COMBINATION_REPORT);

  bool       missed_found = FALSE;  /* If set to TRUE, indicates combinations were missed */
  inst_link* instl;                 /* Pointer to current instance link */
  int        acc_hits     = 0;      /* Accumulated number of combinations hit */
  int        acc_total    = 0;      /* Accumulated number of combinations in design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   COMBINATIONAL LOGIC COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    fprintf( ofile, "                                                                            Logic Combinations\n" );
    fprintf( ofile, "Instance                                                              Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= combination_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_hits, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)combination_display_instance_summary( ofile, "Accumulated", acc_hits, acc_total );
    
    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        combination_instance_verbose( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                                            Logic Combinations\n" );
    fprintf( ofile, "Module/Task/Function                Filename                          Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = combination_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)combination_display_funit_summary( ofile, "Accumulated", "", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      combination_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

