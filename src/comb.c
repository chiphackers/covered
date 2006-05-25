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
 \file     comb.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 
 \par
 The functions in this file are used by the report command to calculate and display all 
 report output for combination logic coverage.  Combinational logic coverage is calculated
 solely from the list of expression trees in the scored design.  For each module or instance,
 the expression list is passed to the calculation routine which iterates through each
 expression tree, tallying the total number of expression values and the total number of
 expression values reached.
 
 \par
 Every expression contains two possible expression values during simulation:  0 and 1 (or 1+).
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
 combinations of its two children have occurred during simulation.  These four bits are EVAL_00, EVAL_01,
 EVAL_10, and EVAL_11.  If the left and right expressions simultaneously evaluated to 0 during simulation,
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

#include "defines.h"
#include "comb.h"
#include "codegen.h"
#include "util.h"
#include "vector.h"
#include "expr.h"
#include "iter.h"
#include "link.h"
#include "func_unit.h"


extern funit_inst*    instance_root;
extern funit_link*    funit_head;
extern bool           report_covered;
extern unsigned int   report_comb_depth;
extern bool           report_instance;
extern char**         leading_hierarchies;
extern int            leading_hier_num;
extern bool           leading_hiers_differ;
extern int            line_width;
extern char           user_msg[USER_MSG_LENGTH];
extern const exp_info exp_op_info[EXP_OP_NUM];
extern isuppl         info_suppl;


/*!
 \param exp         Pointer to current expression.
 \param curr_depth  Current depth in expression tree.
 \param left        TRUE if evaluating for left child.

 \return Returns new depth value for specified child expression.

 Based on the current point in the expression tree, calculates the left or
 right child's curr_depth value.
*/
int combination_calc_depth( expression* exp, unsigned int curr_depth, bool left ) {

  if( ((report_comb_depth == REPORT_DETAILED) && ((curr_depth + 1) <= report_comb_depth)) ||
       (report_comb_depth == REPORT_VERBOSE) ) {

    if( left ) {

      if( (exp->left != NULL) && (exp->op == exp->left->op) ) {
        return( curr_depth );
      } else {
        return( curr_depth + 1 );
      }

    } else {

      if( (exp->right != NULL) && (exp->op == exp->right->op) ) {
        return( curr_depth );
      } else {
        return( curr_depth + 1 );
      }

    }

  } else {

    return( curr_depth + 1 );

  }

}

/*!
 \param exp    Pointer to expression to calculate hit and total of multi-expression subtrees.
 \param ulid   Pointer to current underline ID.
 \param ul     If TRUE, parent expressions were found to be missing so force the underline.
 \param hit    Pointer to value containing number of hit expression values in this expression.
 \param total  Pointer to value containing total number of expression values in this expression.

 \return Returns TRUE if child expressions were found to be missed; otherwise, returns FALSE.

 Parses the specified expression tree, calculating the hit and total values of all
 sub-expressions that are the same operation types as their left children.
*/
bool combination_multi_expr_calc( expression* exp, int* ulid, bool ul, int* hit, float* total ) {

  bool and_op;  /* Specifies if current expression is an AND or LAND operation */

  if( exp != NULL ) {

    /* Figure out if this is an AND/LAND operation */
    and_op = (exp->op == EXP_OP_AND) || (exp->op == EXP_OP_LAND);

    /* Decide if our expression requires that this sequence gets underlined */
    if( !ul ) {
      if( and_op ) {
        ul = (exp->suppl.part.eval_11 == 0) || (ESUPPL_WAS_FALSE( exp->left->suppl ) == 0) || (ESUPPL_WAS_FALSE( exp->right->suppl ) == 0);
      } else {
        ul = (exp->suppl.part.eval_00 == 0) || (ESUPPL_WAS_TRUE( exp->left->suppl )  == 0) || (ESUPPL_WAS_TRUE( exp->right->suppl )  == 0);
      }
    }

    if( (exp->left != NULL) && (exp->op != exp->left->op) ) {
      if( and_op ) {
        *hit += ESUPPL_WAS_FALSE( exp->left->suppl );
      } else {
        *hit += ESUPPL_WAS_TRUE( exp->left->suppl );
      }
      if( (exp->left->ulid == -1) && ul ) { 
        exp->left->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    } else {
      ul = combination_multi_expr_calc( exp->left, ulid, ul, hit, total );
    }

    if( (exp->right != NULL) && (exp->op != exp->right->op) ) {
      if( and_op ) {
        *hit += ESUPPL_WAS_FALSE( exp->right->suppl );
      } else {
        *hit += ESUPPL_WAS_TRUE( exp->right->suppl );
      }
      if( (exp->right->ulid == -1) && ul ) {
        exp->right->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    } else {
      ul = combination_multi_expr_calc( exp->right, ulid, ul, hit, total );
    }

    if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ) {
      if( and_op ) {
        *hit += exp->suppl.part.eval_11;
      } else {
        *hit += exp->suppl.part.eval_00;
      }
      if( (exp->ulid == -1) && ul ) {
        exp->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    }

  }

  return( ul );

}

/*!
 \param exp  Pointer to expression to evaluate.

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
bool combination_is_expr_multi_node( expression* exp ) {

  return( (exp != NULL) &&
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
            (exp->parent->expr->left->op == exp->parent->expr->op) ) );

}

/*!
 \param exp         Pointer to expression tree to traverse.
 \param ulid        Pointer to current underline ID.
 \param curr_depth  Current search depth in given expression tree.
 \param total       Pointer to total number of logical combinations.
 \param hit         Pointer to number of logical combinations hit during simulation.

 Recursively traverses the specified expression tree, recording the total number
 of logical combinations in the expression list and the number of combinations
 hit during the course of simulation.  An expression can be considered for
 combinational coverage if the "measured" bit is set in the expression.
*/
void combination_get_tree_stats( expression* exp, int* ulid, unsigned int curr_depth, float* total, int* hit ) {

  int num_hit = 0;  /* Number of expression value hits for the current expression */
  int tot_num;      /* Total number of combinations for the current expression */

  if( exp != NULL ) {

    /* Calculate children */
    combination_get_tree_stats( exp->left,  ulid, combination_calc_depth( exp, curr_depth, TRUE ),  total, hit );
    combination_get_tree_stats( exp->right, ulid, combination_calc_depth( exp, curr_depth, FALSE ), total, hit );

    if( ((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
         (report_comb_depth == REPORT_VERBOSE) ||
         (report_comb_depth == REPORT_SUMMARY) ) {

      if( (EXPR_IS_MEASURABLE( exp ) == 1) && (ESUPPL_WAS_COMB_COUNTED( exp->suppl ) == 0) ) {

        if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ||
            ((exp->op != EXP_OP_AND) &&
             (exp->op != EXP_OP_LAND) &&
             (exp->op != EXP_OP_OR)   &&
             (exp->op != EXP_OP_LOR)) ) {

          /* Calculate current expression combination coverage */
          if( (((exp->left != NULL) &&
                (exp->op == exp->left->op)) ||
               ((exp->right != NULL) &&
                (exp->op == exp->right->op))) &&
              ((exp->op == EXP_OP_AND)  ||
               (exp->op == EXP_OP_OR)   ||
               (exp->op == EXP_OP_LAND) ||
               (exp->op == EXP_OP_LOR)) ) {
            combination_multi_expr_calc( exp, ulid, FALSE, hit, total );
          } else {
            if( !expression_is_static_only( exp ) ) {
              if( EXPR_IS_COMB( exp ) == 1 ) {
                if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {
                  tot_num = 3;
                  num_hit = (((exp->suppl.part.eval_00 + exp->suppl.part.eval_01) > 0) ? 1 : 0) +
                            (((exp->suppl.part.eval_00 + exp->suppl.part.eval_10) > 0) ? 1 : 0) +
                            exp->suppl.part.eval_11;
                } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {
                  tot_num = 3;
                  num_hit = (((exp->suppl.part.eval_10 + exp->suppl.part.eval_11) > 0) ? 1 : 0) +
                            (((exp->suppl.part.eval_01 + exp->suppl.part.eval_11) > 0) ? 1 : 0) +
                            exp->suppl.part.eval_00;
                } else {
                  tot_num = 4;
                  num_hit = exp->suppl.part.eval_00 +
                            exp->suppl.part.eval_01 +
                            exp->suppl.part.eval_10 +
                            exp->suppl.part.eval_11;
                }
                *total = *total + tot_num;
                *hit   = *hit   + num_hit;
                if( (num_hit != tot_num) && (exp->ulid == -1) && !combination_is_expr_multi_node( exp ) ) {
                  exp->ulid = *ulid;
                  (*ulid)++;
                }
              } else if( EXPR_IS_EVENT( exp ) == 1 ) {
                *total  = *total + 1;
                num_hit = ESUPPL_WAS_TRUE( exp->suppl );
                *hit    = *hit + num_hit;
                if( (num_hit != 1) && (exp->ulid == -1) && !combination_is_expr_multi_node( exp ) ) {
                  exp->ulid = *ulid;
                  (*ulid)++;
                }
              } else {
                *total  = *total + 2;
                num_hit = ESUPPL_WAS_TRUE( exp->suppl ) + ESUPPL_WAS_FALSE( exp->suppl );
                *hit    = *hit + num_hit;
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

    /* Consider this expression to be counted */
    exp->suppl.part.comb_cntd = 1;

  }

}

/*!
 \param expl  Pointer to expression list of current module to reset.

 Iterates through specified expression list, setting the combination counted bit
 in the supplemental field of each expression.  This function needs to get called
 whenever a new module is picked by the GUI.
*/
void combination_reset_counted_exprs( exp_link* expl ) {

  while( expl != NULL ) {
    expl->exp->suppl.part.comb_cntd = 1;
    expl             = expl->next;
  }

}

/*!
 \param expl   Pointer to current expression link to evaluate.
 \param total  Pointer to total number of logical combinations.
 \param hit    Pointer to number of logical combinations hit during simulation.

 Iterates through specified expression list and finds all root expressions.  For
 each root expression, the combination_get_tree_stats function is called to generate
 the coverage numbers for the specified expression tree.  Called by report function.
*/
void combination_get_stats( exp_link* expl, float* total, int* hit ) {

  exp_link* curr_exp;  /* Pointer to the current expression link in the list */
  int       ulid;      /* Current underline ID for this expression */
  
  curr_exp = expl;

  while( curr_exp != NULL ) {
    if( ESUPPL_IS_ROOT( curr_exp->exp->suppl ) == 1 ) {
      ulid = 1;
      combination_get_tree_stats( curr_exp->exp, &ulid, 0, total, hit );
    }
    curr_exp = curr_exp->next;
  }

}

/*!
 \param funit_name  Name of functional unit to retrieve summary information for
 \param funit_type  Type of functional unit to retrieve summary information for
 \param total       Pointer to location to store the total number of combinations for the specified functional unit
 \param hit         Pointer to location to store the number of hit combinations for the specified functional unit

 \return Returns TRUE if the specified module was found; otherwise, returns FALSE.

 Retrieves the combinational logic summary information for the specified functional unit
*/
bool combination_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit ) {

  bool        retval = TRUE;  /* Return value of this function */
  func_unit   funit;          /* Functional unit used for searching */
  funit_link* funitl;         /* Pointer to found functional unit link */
  char        tmp[21];        /* Temporary string for total */

  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    snprintf( tmp, 21, "%20.0f", funitl->funit->stat->comb_total );
    assert( sscanf( tmp, "%d", total ) == 1 );
    *hit = funitl->funit->stat->comb_hit;

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param ofile   Pointer to file to output results to.
 \param root    Pointer to node in instance tree to evaluate.
 \param parent  Name of parent instance name.

 \return Returns TRUE if combinations were found to be missed; otherwise,
         returns FALSE.

 Outputs summarized results of the combinational logic coverage per functional unit
 instance to the specified output stream.  Summarized results are printed 
 as percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
bool combination_instance_summary( FILE* ofile, funit_inst* root, char* parent ) {

  funit_inst* curr;           /* Pointer to current child functional unit instance of this node */
  float       percent;        /* Percentage of lines hit */
  float       miss;           /* Number of lines missed */
  char        tmpname[4096];  /* Temporary name holder of instance */

  assert( root != NULL );
  assert( root->stat != NULL );

  if( root->stat->comb_total == 0 ) {
    percent = 100;
  } else {
    percent = ((root->stat->comb_hit / root->stat->comb_total) * 100);
  }
  miss    = (root->stat->comb_total - root->stat->comb_hit);

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    if( strcmp( parent, "*" ) == 0 ) {
      strcpy( tmpname, root->name );
    } else {
      snprintf( tmpname, 4096, "%s.%s", parent, root->name );
    }

    fprintf( ofile, "  %-63.63s    %4d/%4.0f/%4.0f      %3.0f%%\n",
             tmpname,
             root->stat->comb_hit,
             miss,
             root->stat->comb_total,
             percent );

    curr = root->child_head;
    while( curr != NULL ) {
      miss = miss + combination_instance_summary( ofile, curr, tmpname );
      curr = curr->next;
    }

  }

  return( miss > 0 );

}

/*!
 \param ofile   Pointer to file to output results to.
 \param head    Pointer to link in current functional unit list to evaluate.

 \return Returns TRUE if combinations were found to be missed; otherwise,
         returns FALSE.

 Outputs summarized results of the combinational logic coverage per functional unit
 to the specified output stream.  Summarized results are printed as 
 percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
bool combination_funit_summary( FILE* ofile, funit_link* head ) {

  float percent;             /* Percentage of lines hit */
  float miss;                /* Number of lines missed */
  float miss_found = FALSE;  /* Set to TRUE if missing combinations were found */

  while( head != NULL ) {

    if( head->funit->stat->comb_total == 0 ) {
      percent = 100;
    } else {
      percent = ((head->funit->stat->comb_hit / head->funit->stat->comb_total) * 100);
    }
    miss = (head->funit->stat->comb_total - head->funit->stat->comb_hit);
    if( miss > 0 ) {
      miss_found = TRUE;
    }

    /* If this is an assertion module, don't output any further */
    if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit ) ) {

      fprintf( ofile, "  %-30.30s    %-30.30s   %4d/%4.0f/%4.0f      %3.0f%%\n", 
               head->funit->name,
               get_basename( head->funit->filename ),
               head->funit->stat->comb_hit,
               miss,
               head->funit->stat->comb_total,
               percent );

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param line    Pointer to line to create line onto.
 \param size    Number of characters long line is.
 \param exp_id  ID to place in underline.

 Draws an underline containing the specified expression ID to the specified
 line.  The expression ID will be placed immediately following the beginning
 vertical bar.
*/
void combination_draw_line( char* line, int size, int exp_id ) {

  char str_exp_id[12];   /* String containing value of exp_id */
  int  exp_id_size;      /* Number of characters exp_id is in length */
  int  i;                /* Loop iterator */

  /* Calculate size of expression ID */
  snprintf( str_exp_id, 12, "%d", exp_id );
  exp_id_size = strlen( str_exp_id );

  line[0] = '|';
  line[1] = '\0';
  strcat( line, str_exp_id );

  for( i=(exp_id_size + 1); i<(size - 1); i++ ) {
    line[i] = '-';
  }

  line[i]   = '|';
  line[i+1] = '\0';

}

/*!
 \param line       Pointer to line to create line onto.
 \param size       Number of characters long line is.
 \param exp_id     ID to place in underline.
 \param left_bar   If set to TRUE, draws a vertical bar at the beginning of the underline.
 \param right_bar  If set to TRUE, draws a vertical bar at the end of the underline.

 Draws an underline containing the specified expression ID to the specified
 line.  The expression ID will be placed in the center of the generated underline.
*/
void combination_draw_centered_line( char* line, int size, int exp_id, bool left_bar, bool right_bar ) {

  char str_exp_id[12];   /* String containing value of exp_id */
  int  exp_id_size;      /* Number of characters exp_id is in length */
  int  i;                /* Loop iterator */

  /* Calculate size of expression ID */
  snprintf( str_exp_id, 12, "%d", exp_id );
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

}

/*!
 \param exp         Pointer to expression to create underline for.
 \param curr_depth  Specifies current depth in expression tree.
 \param lines       Stack of lines for left child.
 \param depth       Pointer to top of left child stack.
 \param size        Pointer to character width of this node.
 \param parent_op   Expression operation of parent used for calculating parenthesis.
 \param center      Specifies if expression IDs should be centered in underlines or at beginning.
 \param funit       Pointer to current functional unit containing this expression

 Recursively parses specified expression tree, underlining and labeling each
 measurable expression.
*/
void combination_underline_tree( expression* exp, unsigned int curr_depth, char*** lines, int* depth, int* size,
                                 int parent_op, bool center, func_unit* funit ) {

  char**     l_lines;        /* Pointer to left underline stack */
  char**     r_lines;        /* Pointer to right underline stack */
  int        l_depth = 0;    /* Index to top of left stack */
  int        r_depth = 0;    /* Index to top of right stack */
  int        l_size;         /* Number of characters for left expression */
  int        r_size;         /* Number of characters for right expression */
  int        i;              /* Loop iterator */
  char*      exp_sp;         /* Space to take place of missing expression(s) */
  char       code_fmt[300];  /* Contains format string for rest of stack */
  char*      tmpstr;         /* Temporary string value */
  int        comb_missed;    /* If set to 1, current combination was missed */
  char*      tmpname;        /* Temporary pointer to current signal name */
  char*      pname;          /* Printable version of signal/function/task name */
  func_unit* tfunit;         /* Temporary pointer to found functional unit */
  
  *depth  = 0;
  *size   = 0;
  l_lines = NULL;
  r_lines = NULL;

  if( exp != NULL ) {

    if( (exp->op == EXP_OP_LAST) || (exp->op == EXP_OP_NB_CALL) ) {

      *size = 0;

    } else if( exp->op == EXP_OP_STATIC ) {

      if( exp->value->suppl.part.base == DECIMAL ) {

        snprintf( code_fmt, 300, "%d", vector_to_int( exp->value ) );
        *size = strlen( code_fmt );

        /*
         If the size of this decimal value is only 1 and its parent is a NEGATE op,
         make it two so that we don't have problems with negates and the like later.
        */
        if( (*size == 1) && (exp->parent->expr->op == EXP_OP_NEGATE) ) {
          *size = 2;
        }
      
      } else {

        tmpstr = vector_to_string( exp->value );
        *size  = strlen( tmpstr );
        free_safe( tmpstr );

        /* Adjust for quotation marks */
        if( exp->value->suppl.part.base == QSTRING ) {
          *size += 2;
        }

      }

    } else if( exp->op == EXP_OP_SLIST ) {

      *size = 2;
      strcpy( code_fmt, "@*" );

    } else {

      if( (exp->op == EXP_OP_SIG) || (exp->op == EXP_OP_PARAM) ) {

#ifdef OBSOLETE
        if( exp->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
          tmpname = scope_gen_printable( exp->sig->name );
        } else {
#endif
          tmpname = scope_gen_printable( exp->name );
#ifdef OBSOLETE
        }
#endif

        *size = strlen( tmpname );
        switch( *size ) {
          case 0 :  assert( *size > 0 );                     break;
          case 1 :  *size = 3;  strcpy( code_fmt, " %s " );  break;
          case 2 :  *size = 3;  strcpy( code_fmt, " %s" );   break;
          default:  strcpy( code_fmt, "%s" );                break;
        }

        free_safe( tmpname );
        
      } else {

        combination_underline_tree( exp->left,  combination_calc_depth( exp, curr_depth, TRUE ),  &l_lines, &l_depth, &l_size, exp->op, center, funit );
        combination_underline_tree( exp->right, combination_calc_depth( exp, curr_depth, FALSE ), &r_lines, &r_depth, &r_size, exp->op, center, funit );

        if( parent_op == exp->op ) {

          switch( exp->op ) {
            case EXP_OP_XOR        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_MULTIPLY   :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_DIVIDE     :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_MOD        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_ADD        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_SUBTRACT   :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_EXPONENT   :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_AND        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_OR         :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_NAND       :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_NOR        :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_NXOR       :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_LT         :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_GT         :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_LSHIFT     :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_ALSHIFT    :  *size = l_size + r_size + 5;  strcpy( code_fmt, "%s     %s"      );  break;
            case EXP_OP_RSHIFT     :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_ARSHIFT    :  *size = l_size + r_size + 5;  strcpy( code_fmt, "%s     %s"      );  break;
            case EXP_OP_EQ         :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_CEQ        :  *size = l_size + r_size + 5;  strcpy( code_fmt, "%s     %s"      );  break;
            case EXP_OP_LE         :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_GE         :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_NE         :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_CNE        :  *size = l_size + r_size + 5;  strcpy( code_fmt, "%s     %s"      );  break;
            case EXP_OP_LOR        :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_LAND       :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            default                :  break;
          }

        } else {

          switch( exp->op ) {
            case EXP_OP_XOR        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_MULTIPLY   :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_DIVIDE     :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_MOD        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_ADD        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_SUBTRACT   :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_EXPONENT   :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_AND        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_OR         :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_NAND       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_NOR        :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_NXOR       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_LT         :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_GT         :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_LSHIFT     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_ALSHIFT    :  *size = l_size + r_size + 7;  strcpy( code_fmt, " %s     %s "      );  break;
            case EXP_OP_RSHIFT     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_ARSHIFT    :  *size = l_size + r_size + 7;  strcpy( code_fmt, " %s     %s "      );  break;
            case EXP_OP_EQ         :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_CEQ        :  *size = l_size + r_size + 7;  strcpy( code_fmt, " %s     %s "      );  break;
            case EXP_OP_LE         :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_GE         :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_NE         :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_CNE        :  *size = l_size + r_size + 7;  strcpy( code_fmt, " %s     %s "      );  break;
            case EXP_OP_LOR        :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_LAND       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            default                :  break;
          }

        }

        if( *size == 0 ) {

          switch( exp->op ) {
            case EXP_OP_COND       :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"          );  break;
            case EXP_OP_COND_SEL   :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"          );  break;
            case EXP_OP_UINV       :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"              );  break;
            case EXP_OP_UAND       :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"              );  break;
            case EXP_OP_UNOT       :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"              );  break;
            case EXP_OP_UOR        :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"              );  break;
            case EXP_OP_UXOR       :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"              );  break;
            case EXP_OP_UNAND      :  *size = l_size + r_size + 2;  strcpy( code_fmt, "  %s"             );  break;
            case EXP_OP_UNOR       :  *size = l_size + r_size + 2;  strcpy( code_fmt, "  %s"             );  break;
            case EXP_OP_UNXOR      :  *size = l_size + r_size + 2;  strcpy( code_fmt, "  %s"             );  break;
            case EXP_OP_PARAM_SBIT :
            case EXP_OP_SBIT_SEL   :  
#ifdef OBSOLETE
              if( exp->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
                tmpname = scope_gen_printable( exp->sig->name );
              } else {
#endif
                tmpname = scope_gen_printable( exp->name );
#ifdef OBSOLETE
              }
#endif
              *size = l_size + r_size + strlen( tmpname ) + 2;
              for( i=0; i<strlen( tmpname ); i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              strcat( code_fmt, " %s " );
              free_safe( tmpname );
              break;
            case EXP_OP_PARAM_MBIT :
            case EXP_OP_MBIT_SEL   :  
#ifdef OBSOLETE
              if( exp->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
                tmpname = scope_gen_printable( exp->sig->name );
              } else {
#endif
                tmpname = scope_gen_printable( exp->name );
#ifdef OBSOLETE
              }
#endif
              *size = l_size + r_size + strlen( tmpname ) + 3;  
              for( i=0; i<strlen( tmpname ); i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              strcat( code_fmt, " %s %s " );
              free_safe( tmpname );
              break;
            case EXP_OP_PARAM_MBIT_POS :
            case EXP_OP_PARAM_MBIT_NEG :
            case EXP_OP_MBIT_POS       :
            case EXP_OP_MBIT_NEG       :
#ifdef OBSOLETE
              if( exp->sig->suppl.part.type == SSUPPL_TYPE_PARAM ) {
                tmpname = scope_gen_printable( exp->sig->name );
              } else {
#endif
                tmpname = scope_gen_printable( exp->name );
#ifdef OBSOLETE
              }
#endif
              *size = l_size + r_size + strlen( tmpname ) + 4;
              for( i=0; i<strlen( tmpname ); i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              strcat( code_fmt, " %s  %s " );
              free_safe( tmpname );
              break;
            case EXP_OP_TRIGGER  :
              tmpname = scope_gen_printable( exp->name );
              *size = l_size + r_size + strlen( tmpname ) + 2;
              for( i=0; i<strlen( tmpname ) + 2; i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              free_safe( tmpname );
              break;
            case EXP_OP_EXPAND   :  *size = l_size + r_size + 4;  strcpy( code_fmt, " %s %s  "         );  break;
            case EXP_OP_CONCAT   :  *size = l_size + r_size + 2;  strcpy( code_fmt, " %s "             );  break;
            case EXP_OP_LIST     :  *size = l_size + r_size + 2;  strcpy( code_fmt, "%s  %s"           );  break;
            case EXP_OP_PEDGE    :
              if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 11;  strcpy( code_fmt, "          %s " );
              } else {
                *size = l_size + r_size + 8;   strcpy( code_fmt, "        %s" );
              }
              break;
            case EXP_OP_NEDGE    :
              if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 11;  strcpy( code_fmt, "          %s " );
              } else {
                *size = l_size + r_size + 8;   strcpy( code_fmt, "        %s" );
              }
              break;
            case EXP_OP_AEDGE    :
              if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 3;  strcpy( code_fmt, "  %s " );
              } else {
                *size = l_size + r_size + 0;  strcpy( code_fmt, "%s" );
              }
              break;
            case EXP_OP_EOR      :
              if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 7;  strcpy( code_fmt, "  %s    %s " );
              } else {
                *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s" );
              }
              break;
            case EXP_OP_CASE     :  *size = l_size + r_size + 11; strcpy( code_fmt, "      %s   %s  "  );  break;
            case EXP_OP_CASEX    :  *size = l_size + r_size + 12; strcpy( code_fmt, "       %s   %s  " );  break;
            case EXP_OP_CASEZ    :  *size = l_size + r_size + 12; strcpy( code_fmt, "       %s   %s  " );  break;
            case EXP_OP_DELAY    :  *size = l_size + r_size + 3;  strcpy( code_fmt, "  %s " );             break;
            case EXP_OP_ASSIGN   :  *size = l_size + r_size + 10; strcpy( code_fmt, "       %s   %s" );    break;
            case EXP_OP_DASSIGN  :
            case EXP_OP_BASSIGN  :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s" );           break;
            case EXP_OP_NASSIGN  :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s" );          break;
            case EXP_OP_PASSIGN  :  *size = r_size;               strcpy( code_fmt, "%s" );                break;
            case EXP_OP_IF       :  *size = r_size + 6;           strcpy( code_fmt, "    %s  " );          break;
            case EXP_OP_REPEAT   :  *size = r_size + 10;          strcpy( code_fmt, "        %s  " );      break;
            case EXP_OP_WHILE    :  *size = r_size + 9;           strcpy( code_fmt, "       %s  " );       break;
            case EXP_OP_TASK_CALL :
            case EXP_OP_FUNC_CALL :
              if( (tfunit = funit_find_by_id( exp->stmt->exp->id )) != NULL ) {
                tmpname = strdup_safe( tfunit->name, __FILE__, __LINE__ );
                scope_extract_back( tfunit->name, tmpname, user_msg );
                pname = scope_gen_printable( tmpname );
              } else {
                snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Could not find statement %d in module %s",
                          exp->stmt->exp->id, funit->name );
                print_output( user_msg, FATAL, __FILE__, __LINE__ );
                exit( 1 );
              }
              *size = l_size + r_size + strlen( pname ) + 4;
              for( i=0; i<strlen( pname ); i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              strcat( code_fmt, "  %s  " );
              free_safe( tmpname );
              free_safe( pname );
              break;
            case EXP_OP_NEGATE   :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"              );  break;
            default              :
              snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Unknown expression type in combination_underline_tree (%d)",
                        exp->op );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              exit( 1 );
              break;
          }

        }

      }

      comb_missed = (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
                      (report_comb_depth == REPORT_VERBOSE)) ? ((exp->ulid != -1) ? 1 : 0) : 0;

      if( l_depth > r_depth ) {
        *depth = l_depth + comb_missed;
      } else {
        *depth = r_depth + comb_missed;
      }
      
      if( *depth > 0 ) {
                
        /* Allocate all memory for the stack */
        *lines = (char**)malloc_safe( (sizeof( char* ) * (*depth)), __FILE__, __LINE__ );

        /* Create underline or space */
        if( comb_missed == 1 ) {

          /* Allocate memory for this underline */
          (*lines)[(*depth)-1] = (char*)malloc_safe( (*size + 1), __FILE__, __LINE__ );

          if( center ) {
            combination_draw_centered_line( (*lines)[(*depth)-1], *size, exp->ulid, TRUE, TRUE );
          } else {
            combination_draw_line( (*lines)[(*depth)-1], *size, exp->ulid );
          }
          /* printf( "Drawing line (%s), size: %d, depth: %d\n", (*lines)[(*depth)-1], *size, (*depth) ); */
        }

        /* Combine the left and right line stacks */
        for( i=0; i<(*depth - comb_missed); i++ ) {

          (*lines)[i] = (char*)malloc_safe( (*size + 1), __FILE__, __LINE__ );

          if( (i < l_depth) && (i < r_depth) ) {
            
            /* Merge left and right lines */
            snprintf( (*lines)[i], (*size + 1), code_fmt, l_lines[i], r_lines[i] );
            
            free_safe( l_lines[i] );
            free_safe( r_lines[i] );

          } else if( i < l_depth ) {
            
            /* Create spaces for right side */
            exp_sp = (char*)malloc_safe( (r_size + 1), __FILE__, __LINE__ );
            gen_space( exp_sp, r_size );

            /* Merge left side only */
            snprintf( (*lines)[i], (*size + 1), code_fmt, l_lines[i], exp_sp );
            
            free_safe( l_lines[i] );
            free_safe( exp_sp );

          } else if( i < r_depth ) {

            if( l_size == 0 ) { 

              snprintf( (*lines)[i], (*size + 1), code_fmt, r_lines[i] );

            } else {

              /* Create spaces for left side */
              exp_sp = (char*)malloc_safe( (l_size + 1), __FILE__, __LINE__ );
              gen_space( exp_sp, l_size );

              /* Merge right side only */
              snprintf( (*lines)[i], (*size + 1), code_fmt, exp_sp, r_lines[i] );
              
              free_safe( exp_sp );
          
            }
  
            free_safe( r_lines[i] );
   
          } else {

            print_output( "Internal error:  Reached entry without a left or right underline", FATAL, __FILE__, __LINE__ );
            exit( 1 );

          }

        }

        /* Free left child stack */
        if( l_depth > 0 ) {
          free_safe( l_lines );
        }

        /* Free right child stack */
        if( r_depth > 0 ) {
          free_safe( r_lines );
        }

      }

    }

  }
    
}

/*!
 \param line   Line containing underlines that needs to be reformatted for line wrap.
 \param start  Starting index in line to take underline information from.
 \param len    Number of characters to use for the current line.

 \return Returns a newly allocated string that contains the underline information for
         the current line.  If there is no underline information to return, a value of
         NULL is returned.

 Formats the specified underline line to line wrap according to the generated code.  This
 function must be called after the underline lines have been calculated prior to being
 output to the ASCII report or the GUI.
*/
char* combination_prep_line( char* line, int start, int len ) {

  char* str;                /* Prepared line to return */
  int   exp_id;             /* Expression ID of current line */
  int   chars_read;         /* Number of characters read from sscanf function */
  int   i;                  /* Loop iterator */
  int   curr_index;         /* Index current character in str to set */
  bool  line_ip   = FALSE;  /* Specifies if a line is currently in progress */
  bool  line_seen = FALSE;  /* Specifies that a line has been seen for this line */
  int   start_ul  = 0;      /* Index of starting underline */

  /* Allocate memory for string to return */
  str = (char*)malloc_safe( (len + 2), __FILE__, __LINE__ );

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
    free_safe( str );
    str = NULL;
  } else {
    str[curr_index] = '\0';
  }

  return( str );

}

/*!
 \param ofile       Pointer output stream to display underlines to.
 \param code        Array of strings containing code to output.
 \param code_depth  Number of entries in code array.
 \param exp         Pointer to parent expression to create underline for.
 \param funit       Pointer to the functional unit containing the expression to underline.

 Traverses through the expression tree that is on the same line as the parent,
 creating underline strings.  An underline is created for each expression that
 does not have complete combination logic coverage.  Each underline (children to
 parent creates an inverted tree) and contains a number for the specified expression.
*/
void combination_underline( FILE* ofile, char** code, int code_depth, expression* exp, func_unit* funit ) {

  char** lines;    /* Pointer to a stack of lines */
  int    depth;    /* Depth of underline stack */
  int    size;     /* Width of stack in characters */
  int    i;        /* Loop iterator */
  int    j;        /* Loop iterator */
  char*  tmpstr;   /* Temporary string variable */
  int    start;    /* Starting index */

  start = 0;

  combination_underline_tree( exp, 0, &lines, &depth, &size, exp->op, (code_depth == 1), funit );

  for( j=0; j<code_depth; j++ ) {

    assert( code[j] != NULL );

    if( j == 0 ) {
      fprintf( ofile, "        %7d:    %s\n", exp->line, code[j] );
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
          free_safe( tmpstr );
        }
      }
    }

    start += strlen( code[j] );

    free_safe( code[j] );

  }

  for( i=0; i<depth; i++ ) {
    free_safe( lines[i] );
  }

  if( depth > 0 ) {
    free_safe( lines );
  }

  if( code_depth > 0 ) {
    free_safe( code );
  }

}

/*!
 \param info       Pointer to array of strings that will contain the coverage information for this expression
 \param info_size  Pointer to integer containing number of elements in info array 
 \param exp        Pointer to expression to evaluate.

 Displays the missed unary combination(s) that keep the combination coverage for
 the specified expression from achieving 100% coverage.
*/
void combination_unary( char*** info, int* info_size, expression* exp ) {

  int   hit = 0;                           /* Number of combinations hit for this expression */
  char  tmp[20]; 
  int   length; 
  char* op = exp_op_info[exp->op].op_str;  /* Operations string */

  assert( exp != NULL );

  /* Get hit information */
  hit = ESUPPL_WAS_FALSE( exp->suppl ) + ESUPPL_WAS_TRUE( exp->suppl );

  if( hit != 2 ) {

    assert( exp->ulid != -1 );

    /* Allocate memory for info array */
    *info_size = 5;
    *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size), __FILE__, __LINE__ );

    /* Allocate lines and assign values */ 
    length = 27;  snprintf( tmp, 20, "%d", exp->ulid );  length += strlen( tmp );  snprintf( tmp, 20, "%d", hit );  length += strlen( tmp );
    (*info)[0] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[0], length, "        Expression %d   (%d/2)", exp->ulid, hit );

    length  = 25 + strlen( op );  (*info)[1] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[1], length, "        ^^^^^^^^^^^^^ - %s", op );

    (*info)[2] = strdup( "         E | E" );
    (*info)[3] = strdup( "        =0=|=1=" );

    length = 15;  (*info)[4] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[4], length, "         %c   %c",
              ((ESUPPL_WAS_FALSE( exp->suppl ) == 1) ? ' ' : '*'),
              ((ESUPPL_WAS_TRUE( exp->suppl )  == 1) ? ' ' : '*') );

  }

}

/*!
 \param info       Pointer to array of strings that will contain the coverage information for this expression
 \param info_size  Pointer to integer containing number of elements in info array
 \param exp        Pointer to expression to evaluate.

 Displays the missed unary combination(s) that keep the combination coverage for
 the specified expression from achieving 100% coverage.
*/
void combination_event( char*** info, int* info_size, expression* exp ) {

  char  tmp[20];
  int   length;
  char* op = exp_op_info[exp->op].op_str;  /* Operation string */

  assert( exp != NULL );

  if( !ESUPPL_WAS_TRUE( exp->suppl ) ) {

    assert( exp->ulid != -1 );

    /* Allocate memory for info array */
    *info_size = 3;
    *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size), __FILE__, __LINE__ );

    /* Allocate lines and assign values */
    length = 28;  snprintf( tmp, 20, "%d", exp->ulid );  length += strlen( tmp );
    (*info)[0] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[0], length, "        Expression %d   (0/1)", exp->ulid );

    length  = 25 + strlen( op );  (*info)[1] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[1], length, "        ^^^^^^^^^^^^^ - %s", op );

    (*info)[2] = strdup( "         * Event did not occur" );

  }

}

/*!
 \param info       Pointer to array of strings that will contain the coverage information for this expression
 \param info_size  Pointer to integer containing number of elements in info array 
 \param exp        Pointer to expression to evaluate.

 Displays the missed combinational sequences for the specified expression to the
 specified output stream in tabular form.
*/
void combination_two_vars( char*** info, int* info_size, expression* exp ) {

  int   hit;      /* Number of combinations hit for this expression */
  int   total;    /* Total number of combinations for this expression */
  char  tmp[20];
  int   length; 
  char* op = exp_op_info[exp->op].op_str;  /* Operation string */

  assert( exp != NULL );

  /* Verify that left child expression is valid for this operation */
  assert( exp->left != NULL );

  /* Verify that right child expression is valid for this operation */
  assert( exp->right != NULL );

  /* Get hit information */
  if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {
    hit   = (((exp->suppl.part.eval_00 + exp->suppl.part.eval_01) > 0) ? 1 : 0) +
            (((exp->suppl.part.eval_00 + exp->suppl.part.eval_10) > 0) ? 1 : 0) +
            exp->suppl.part.eval_11;
    total = 3;
  } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {
    hit   = (((exp->suppl.part.eval_10 + exp->suppl.part.eval_11) > 0) ? 1 : 0) +
            (((exp->suppl.part.eval_01 + exp->suppl.part.eval_11) > 0) ? 1 : 0) +
            exp->suppl.part.eval_00;
    total = 3;
  } else {
    hit = exp->suppl.part.eval_00 +
          exp->suppl.part.eval_01 +
          exp->suppl.part.eval_10 +
          exp->suppl.part.eval_11;
    total = 4;
  }

  if( hit != total ) {

    assert( exp->ulid != -1 );

    /* Allocate memory for info array */
    *info_size = 5;
    *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size), __FILE__, __LINE__ );

    /* Allocate lines and assign values */ 
    length = 27;  snprintf( tmp, 20, "%d", exp->ulid );  length += strlen( tmp );  snprintf( tmp, 20, "%d", hit );  length += strlen( tmp );
    (*info)[0] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[0], length, "        Expression %d   (%d/%d)", exp->ulid, hit, total );

    length = 25 + strlen( op );
    (*info)[1] = (char*)malloc_safe( length, __FILE__, __LINE__ );
    snprintf( (*info)[1], length, "        ^^^^^^^^^^^^^ - %s", op );

    if( exp_op_info[exp->op].suppl.is_comb == AND_COMB ) {

      (*info)[2] = strdup( "         LR | LR | LR " );
      (*info)[3] = strdup( "        =0-=|=-0=|=11=" );

      length = 21;
      (*info)[4] = (char*)malloc_safe( length, __FILE__, __LINE__ );
      snprintf( (*info)[4], length, "         %c    %c    %c",
                (((exp->suppl.part.eval_00 + exp->suppl.part.eval_01) > 0) ? ' ' : '*'),
                (((exp->suppl.part.eval_00 + exp->suppl.part.eval_10) > 0) ? ' ' : '*'),
                ((exp->suppl.part.eval_11 > 0) ? ' ' : '*') );

    } else if( exp_op_info[exp->op].suppl.is_comb == OR_COMB ) {

      (*info)[2] = strdup( "         LR | LR | LR " );
      (*info)[3] = strdup( "        =1-=|=-1=|=00=" );

      length = 21;
      (*info)[4] = (char*)malloc_safe( length, __FILE__, __LINE__ );
      snprintf( (*info)[4], length, "         %c    %c    %c",
                (((exp->suppl.part.eval_10 + exp->suppl.part.eval_11) > 0) ? ' ' : '*'),
                (((exp->suppl.part.eval_01 + exp->suppl.part.eval_11) > 0) ? ' ' : '*'),
                ((exp->suppl.part.eval_00 > 0) ? ' ' : '*') );

    } else {

      (*info)[2] = strdup( "         LR | LR | LR | LR " );
      (*info)[3] = strdup( "        =00=|=01=|=10=|=11=" );

      length = 26;
      (*info)[4] = (char*)malloc_safe( length, __FILE__, __LINE__ );
      snprintf( (*info)[4], length, "         %c    %c    %c    %c",
                ((exp->suppl.part.eval_00 == 1) ? ' ' : '*'),
                ((exp->suppl.part.eval_01 == 1) ? ' ' : '*'),
                ((exp->suppl.part.eval_10 == 1) ? ' ' : '*'),
                ((exp->suppl.part.eval_11 == 1) ? ' ' : '*') );

    }

  }

}

/*!
 \param line1  Pointer to first line of multi-variable expression output.
 \param line2  Pointer to second line of multi-variable expression output.
 \param line3  Pointer to third line of multi-variable expression output.
 \param exp    Pointer to current expression to output.

 Creates the verbose report information for a multi-variable expression, storing its output in
 the line1, line2, and line3 strings.
*/
void combination_multi_var_exprs( char** line1, char** line2, char** line3, expression* exp ) {

  char* left_line1;
  char* left_line2;
  char* left_line3;
  char  curr_id_str[20];
  int   curr_id_str_len;
  int   i;
  bool  and_op;

  if( exp != NULL ) {

    and_op = (exp->op == EXP_OP_AND) || (exp->op == EXP_OP_LAND);

    /* If we have hit the left-most expression, start creating string here */
    if( (exp->left != NULL) && (exp->op != exp->left->op) ) {

      assert( exp->left->ulid != -1 );
      snprintf( curr_id_str, 20, "%d", exp->left->ulid );
      curr_id_str_len = strlen( curr_id_str );
      left_line1 = (char*)malloc_safe( (curr_id_str_len + 4), __FILE__, __LINE__ );
      left_line2 = (char*)malloc_safe( (curr_id_str_len + 4), __FILE__, __LINE__ );
      left_line3 = (char*)malloc_safe( (curr_id_str_len + 4), __FILE__, __LINE__ );
      snprintf( left_line1, (curr_id_str_len + 4), " %s |", curr_id_str );
      for( i=0; i<(curr_id_str_len-1); i++ ) {
        curr_id_str[i] = '=';
      }
      curr_id_str[i] = '\0'; 
      if( and_op ) { 
        snprintf( left_line2, (curr_id_str_len + 4), "=0%s=|", curr_id_str );
      } else { 
        snprintf( left_line2, (curr_id_str_len + 4), "=1%s=|", curr_id_str );
      }
      for( i=0; i<(curr_id_str_len - 1); i++ ) {
        curr_id_str[i] = ' ';
      }
      curr_id_str[i] = '\0';
      if( and_op ) {
        snprintf( left_line3, (curr_id_str_len + 4), " %c%s  ", ((ESUPPL_WAS_FALSE( exp->left->suppl ) == 1) ? ' ' : '*'), curr_id_str );
      } else {
        snprintf( left_line3, (curr_id_str_len + 4), " %c%s  ", ((ESUPPL_WAS_TRUE( exp->left->suppl )  == 1) ? ' ' : '*'), curr_id_str );
      }

    } else {

      combination_multi_var_exprs( &left_line1, &left_line2, &left_line3, exp->left );

    }

    /* Take left side and merge it with right side */
    assert( left_line1 != NULL );
    assert( exp->right->ulid != -1 );
    snprintf( curr_id_str, 20, "%d", exp->right->ulid );
    curr_id_str_len = strlen( curr_id_str );
    *line1 = (char*)malloc_safe( (strlen( left_line1 ) + curr_id_str_len + 4), __FILE__, __LINE__ );
    *line2 = (char*)malloc_safe( (strlen( left_line2 ) + curr_id_str_len + 4), __FILE__, __LINE__ );
    *line3 = (char*)malloc_safe( (strlen( left_line3 ) + curr_id_str_len + 4), __FILE__, __LINE__ );
    snprintf( *line1, (strlen( left_line1 ) + curr_id_str_len + 4), "%s %s |", left_line1, curr_id_str );
    for( i=0; i<(curr_id_str_len-1); i++ ) {
      curr_id_str[i] = '=';
    }
    curr_id_str[i] = '\0'; 
    if( and_op ) {
      snprintf( *line2, (strlen( left_line2 ) + curr_id_str_len + 4), "%s=0%s=|", left_line2, curr_id_str );
    } else { 
      snprintf( *line2, (strlen( left_line2 ) + curr_id_str_len + 4), "%s=1%s=|", left_line2, curr_id_str );
    }
    for( i=0; i<(curr_id_str_len - 1); i++ ) {
      curr_id_str[i] = ' ';
    }
    curr_id_str[i] = '\0';
    if( and_op ) {
      snprintf( *line3, (strlen( left_line3 ) + curr_id_str_len + 4), "%s %c%s  ",
                left_line3, ((ESUPPL_WAS_FALSE( exp->right->suppl ) == 1) ? ' ' : '*'), curr_id_str );
    } else {
      snprintf( *line3, (strlen( left_line3 ) + curr_id_str_len + 4), "%s %c%s  ",
                left_line3, ((ESUPPL_WAS_TRUE( exp->right->suppl )  == 1) ? ' ' : '*'),  curr_id_str );
    }
    free_safe( left_line1 );
    free_safe( left_line2 );
    free_safe( left_line3 );

    /* If we are the root, output all value */
    if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ) {
      left_line1 = *line1;
      left_line2 = *line2;
      left_line3 = *line3;
      *line1     = (char*)malloc_safe( (strlen( left_line1 ) + 7), __FILE__, __LINE__ );
      *line2     = (char*)malloc_safe( (strlen( left_line2 ) + 7), __FILE__, __LINE__ );
      *line3     = (char*)malloc_safe( (strlen( left_line3 ) + 7), __FILE__, __LINE__ );
      if( and_op ) {
        snprintf( *line1, (strlen( left_line1 ) + 7), "%s All",   left_line1 );
        snprintf( *line2, (strlen( left_line2 ) + 7), "%s==1==",  left_line2 );
        snprintf( *line3, (strlen( left_line3 ) + 7), "%s  %c  ", left_line3, ((exp->suppl.part.eval_11 == 1) ? ' ' : '*') );
      } else {
        snprintf( *line1, (strlen( left_line1 ) + 7), "%s All",   left_line1 );
        snprintf( *line2, (strlen( left_line2 ) + 7), "%s==0==",  left_line2 );
        snprintf( *line3, (strlen( left_line3 ) + 7), "%s  %c  ", left_line3, ((exp->suppl.part.eval_00 == 1) ? ' ' : '*') );
      }
      free_safe( left_line1 );
      free_safe( left_line2 );
      free_safe( left_line3 );
    }

  }

}

/*!
 \param line1  First line of multi-variable expression output
 \param line2  Second line of multi-variable expression output
 \param line3  Third line of multi-variable expression output

 \return Returns the number of lines required to store the multi-variable expression output
         contained in line1, line2, and line3.
*/
int combination_multi_expr_output_length( char* line1, char* line2, char* line3 ) {

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

  return( length );

}

/*!
 \param info   Pointer string to output report contents to.
 \param line1  First line of multi-variable expression output.
 \param line2  Second line of multi-variable expression output.
 \param line3  Third line of multi-variable expression output.

 Stores the information from line1, line2 and line3 in the string array info.
*/
void combination_multi_expr_output( char** info, char* line1, char* line2, char* line3 ) {

  int start      = 0;
  int i;
  int len        = strlen( line1 );
  int info_index = 2;

  for( i=0; i<len; i++ ) {

    if( (i + 1) == len ) {

      info[info_index+0] = (char*)malloc_safe( strlen( line1 + start ) + 9, __FILE__, __LINE__ );
      info[info_index+1] = (char*)malloc_safe( strlen( line2 + start ) + 9, __FILE__, __LINE__ );
      info[info_index+2] = (char*)malloc_safe( strlen( line3 + start ) + 9, __FILE__, __LINE__ );

      snprintf( info[info_index+0], (strlen( line1 + start ) + 9), "        %s", (line1 + start) );
      snprintf( info[info_index+1], (strlen( line2 + start ) + 9), "        %s", (line2 + start) );
      snprintf( info[info_index+2], (strlen( line3 + start ) + 9), "        %s", (line3 + start) );

    } else if( (line1[i] == '|') && ((i - start) >= line_width) ) {

      line1[i] = '\0';
      line2[i] = '\0';
      line3[i] = '\0';

      info[info_index+0] = (char*)malloc_safe( strlen( line1 + start ) + 10, __FILE__, __LINE__ );
      info[info_index+1] = (char*)malloc_safe( strlen( line2 + start ) + 10, __FILE__, __LINE__ );
      info[info_index+2] = (char*)malloc_safe( strlen( line3 + start ) + 11, __FILE__, __LINE__ );

      snprintf( info[info_index+0], (strlen( line1 + start ) + 10), "        %s|",   (line1 + start) );
      snprintf( info[info_index+1], (strlen( line2 + start ) + 10), "        %s|",   (line2 + start) );
      snprintf( info[info_index+2], (strlen( line3 + start ) + 11), "        %s \n", (line3 + start) );

      start       = i + 1;
      info_index += 3;

    }

  }

}

/*!
 \param info       Pointer to character array containing coverage information to output
 \param info_size  Pointer to integer containing number of valid array entries in info
 \param exp        Pointer to top-level AND/OR expression to evaluate.

 Displays the missed combinational sequences for the specified expression to the
 specified output stream in tabular form.
*/
void combination_multi_vars( char*** info, int* info_size, expression* exp ) {

  int   ulid      = 1;
  float total     = 0;
  int   hit       = 0;
  char* line1     = NULL;
  char* line2     = NULL;
  char* line3     = NULL;
  char  tmp[20];
  int   line_size = 1;

  /* Only output this expression if we are missing coverage. */
  if( exp->ulid != -1 ) {

    /* Calculate hit and total values for this sub-expression */
    combination_multi_expr_calc( exp, &ulid, FALSE, &hit, &total );

    if( hit != (int)total ) {

      /* Gather report output for this expression */
      combination_multi_var_exprs( &line1, &line2, &line3, exp );

      /* Calculate the array needed to store the output information and allocate this memory */
      *info_size = combination_multi_expr_output_length( line1, line2, line3 ) + 2;
      *info      = (char**)malloc_safe( sizeof( char* ) * (*info_size), __FILE__, __LINE__ );

      /* Calculate needed line length */
      snprintf( tmp, 20, "%d", exp->ulid );
      line_size += strlen( tmp );
      snprintf( tmp, 20, "%d", hit );
      line_size += strlen( tmp );
      snprintf( tmp, 20, "%.0f", total );
      line_size += strlen( tmp );
      line_size += 27;                   /* Number of additional characters in line below */
      (*info)[0] = (char*)malloc_safe( line_size, __FILE__, __LINE__ );
    
      snprintf( (*info)[0], line_size, "        Expression %d   (%d/%.0f)", exp->ulid, hit, total );
    
      switch( exp->op ) {
        case EXP_OP_AND  :  (*info)[1] = strdup( "        ^^^^^^^^^^^^^ - &" );   break;
        case EXP_OP_OR   :  (*info)[1] = strdup( "        ^^^^^^^^^^^^^ - |" );   break;
        case EXP_OP_LAND :  (*info)[1] = strdup( "        ^^^^^^^^^^^^^ - &&" );  break;
        case EXP_OP_LOR  :  (*info)[1] = strdup( "        ^^^^^^^^^^^^^ - ||" );  break;
        default          :  break;
      }

      /* Output the lines paying attention to the current line width */
      combination_multi_expr_output( *info, line1, line2, line3 );

      free_safe( line1 );
      free_safe( line2 );
      free_safe( line3 );

    }

  }

}

/*!
 \param info        Pointer to an array of strings containing expression coverage detail
 \param info_size   Pointer to a value that will be set to indicate the number of valid elements in the info array
 \param exp         Pointer to the expression to get the coverage detail for
 \param curr_depth  Current expression depth (used to figure out when to stop getting coverage information -- if
                    the user has specified a maximum depth)

 Calculates the missed coverage detail output for the given expression, placing the output to the info array.  This
 array can then be sent to an ASCII report file or the GUI.
*/
void combination_get_missed_expr( char*** info, int* info_size, expression* exp, unsigned int curr_depth ) {

  assert( exp != NULL );

  *info_size = 0;

  if( EXPR_COMB_MISSED( exp ) &&
      (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
        (report_comb_depth == REPORT_VERBOSE)) ) {

    if( (ESUPPL_IS_ROOT( exp->suppl ) == 1) || (exp->op != exp->parent->expr->op) ||
        ((exp->op != EXP_OP_AND)  &&
         (exp->op != EXP_OP_LAND) &&
         (exp->op != EXP_OP_OR)   &&
         (exp->op != EXP_OP_LOR)) ) {

      if( (exp->left != NULL) && (exp->op == exp->left->op) &&
          ((exp->op == EXP_OP_AND)  ||
           (exp->op == EXP_OP_OR)   ||
           (exp->op == EXP_OP_LAND) ||
           (exp->op == EXP_OP_LOR)) ) {

        combination_multi_vars( info, info_size, exp );

      } else {

        /* Create combination table */
        if( EXPR_IS_COMB( exp ) ) {
          combination_two_vars( info, info_size, exp );
        } else if( EXPR_IS_EVENT( exp ) ) {
          combination_event( info, info_size, exp );
        } else {
          combination_unary( info, info_size, exp );
        }

      }

    }

  }

}

/*!
 \param ofile       Pointer to file to output results to.
 \param exp         Pointer to expression tree to evaluate.
 \param curr_depth  Specifies current depth of expression tree.

 Describe which combinations were not hit for all subexpressions in the
 specified expression tree.  We display the value of missed combinations by
 displaying the combinations of the children expressions that were not run
 during simulation.
*/
void combination_list_missed( FILE* ofile, expression* exp, unsigned int curr_depth ) {

  char** info;       /* String array containing combination coverage information for this expression */
  int    info_size;  /* Specifies the number of valid entries in the info array */
  int    i;          /* Loop iterator */
  
  if( exp != NULL ) {
    
    combination_list_missed( ofile, exp->left,  combination_calc_depth( exp, curr_depth, TRUE ) );
    combination_list_missed( ofile, exp->right, combination_calc_depth( exp, curr_depth, FALSE ) );

    /* Get coverage information for this expression */
    combination_get_missed_expr( &info, &info_size, exp, curr_depth );

    /* If there was any coverage information for this expression, output it to the specified output stream */
    if( info_size > 0 ) {

      for( i=0; i<info_size; i++ ) {

        fprintf( ofile, "%s\n", info[i] );
        free_safe( info[i] );

      }

      fprintf( ofile, "\n" );

      free_safe( info );

    }

  }

}

/*!
 \param expr            Pointer to root of expression tree to search.
 \param curr_depth      Specifies current depth of expression tree.
 \param any_missed      Pointer to indicate if any subexpressions were missed in the specified expression.
 \param any_measurable  Pointer to indicate if any subexpressions were measurable in the specified expression.

 Recursively traverses specified expression tree, returning TRUE
 if an expression is found that has not received 100% coverage for
 combinational logic.
*/
void combination_output_expr( expression* expr, unsigned int curr_depth, int* any_missed, int* any_measurable ) {

  if( (expr != NULL) && (ESUPPL_WAS_COMB_COUNTED( expr->suppl ) == 1) ) {

    expr->suppl.part.comb_cntd = 0;

    combination_output_expr( expr->right, combination_calc_depth( expr, curr_depth, FALSE ), any_missed, any_measurable );
    combination_output_expr( expr->left,  combination_calc_depth( expr, curr_depth, TRUE ),  any_missed, any_measurable );

    if( ((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
         (report_comb_depth == REPORT_VERBOSE) ) {
 
      if( expr->ulid != -1 ) {
        *any_missed = 1;
      }
      if( EXPR_IS_MEASURABLE( expr ) == 1 ) {
        *any_measurable = 1;
      }

    }

  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param funit  Pointer to functional unit to display verbose combinational logic output for.

 Displays the expressions (and groups of expressions) that were considered 
 to be measurable (evaluates to a value of TRUE (1) or FALSE (0) but were 
 not hit during simulation.  The entire Verilog expression is displayed
 to the specified output stream with each of its measured expressions being
 underlined and numbered.  The missed combinations are then output below
 the Verilog code, showing those logical combinations that were not hit
 during simulation.
*/
void combination_display_verbose( FILE* ofile, func_unit* funit ) {

  stmt_iter   stmti;           /* Statement list iterator */
  expression* unexec_exp;      /* Pointer to current unexecuted expression */
  char**      code;            /* Code string from code generator */
  int         code_depth;      /* Depth of code array */
  int         any_missed;      /* Specifies if any of the subexpressions were missed in this expression */
  int         any_measurable;  /* Specifies if any of the subexpressions were measurable in this expression */

  if( report_covered ) {
    fprintf( ofile, "    Hit Combinations\n\n" );
  } else { 
    fprintf( ofile, "    Missed Combinations  (* = missed value)\n\n" );
  }

  /* Display current instance missed lines */
  stmt_iter_reset( &stmti, funit->stmt_tail );
  stmt_iter_find_head( &stmti, FALSE );

  while( stmti.curr != NULL ) {

    any_missed     = 0;
    any_measurable = 0;

    combination_output_expr( stmti.curr->stmt->exp, 0, &any_missed, &any_measurable );

    if( ((report_covered == 0) && (any_missed == 1)) ||
        ((report_covered == 1) && (any_missed == 0) && (any_measurable == 1)) ) {
 
      stmti.curr->stmt->exp->suppl.part.comb_cntd = 0;
      unexec_exp = stmti.curr->stmt->exp;

      fprintf( ofile, "      =========================================================================================================\n" );
      fprintf( ofile, "       Line #     Expression\n" );
      fprintf( ofile, "      =========================================================================================================\n" );

      /* Generate line of code that missed combinational coverage */
      codegen_gen_expr( unexec_exp, unexec_exp->op, &code, &code_depth, funit );

      /* Output underlining feature for missed expressions */
      combination_underline( ofile, code, code_depth, unexec_exp, funit );
      fprintf( ofile, "\n" );

      /* Output logical combinations that missed complete coverage */
      combination_list_missed( ofile, unexec_exp, 0 );

    }
    
    stmt_iter_get_next_in_order( &stmti );

  }

}

/*!
 \param ofile   Pointer to file to output results to.
 \param root    Pointer to current functional unit instance to evaluate.
 \param parent  Name of parent instance.

 Outputs the verbose coverage report for the specified functional unit instance
 to the specified output stream.
*/
void combination_instance_verbose( FILE* ofile, funit_inst* root, char* parent ) {

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder of instance */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );

  /* Get printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( strcmp( parent, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent, pname );
  }

  free_safe( pname );

  if( ((root->stat->comb_hit < root->stat->comb_total) && !report_covered) ||
      ((root->stat->comb_hit > 0) && report_covered) ) {

    /* Get printable version of functional unit name */
    pname = scope_gen_printable( root->funit->name );

    fprintf( ofile, "\n" );
    switch( root->funit->type ) {
      case FUNIT_MODULE      :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_NAMED_BLOCK :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_FUNCTION    :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_TASK        :  fprintf( ofile, "    Task: " );         break;
      default                :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, root->funit->filename, tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

    free_safe( pname );

    combination_display_verbose( ofile, root->funit );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    combination_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to current functional unit to evaluate.

 Outputs the verbose coverage report for the specified functional unit
 to the specified output stream.
*/
void combination_funit_verbose( FILE* ofile, funit_link* head ) {

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( ((head->funit->stat->comb_hit < head->funit->stat->comb_total) && !report_covered) ||
        ((head->funit->stat->comb_hit > 0) && report_covered) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( head->funit->name );

      fprintf( ofile, "\n" );
      switch( head->funit->type ) {
        case FUNIT_MODULE      :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_NAMED_BLOCK :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_FUNCTION    :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_TASK        :  fprintf( ofile, "    Task: " );         break;
        default                :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", pname, head->funit->filename );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      free_safe( pname );

      combination_display_verbose( ofile, head->funit );

    }

    head = head->next;

  }

}

/*!
 \param funit_name  Name of functional unit to collect combinational logic coverage information for
 \param funit_type  Type of functional unit to collect combinational logic coverage information for
 \param covs        Pointer to an array of expression pointers that contain all fully covered expressions
 \param cov_cnt     Pointer to a value that will be set to indicate the number of expressions in covs array
 \param uncovs      Pointer to an array of expression pointers that contain uncovered expressions
 \param uncov_cnt   Pointer to a value that will be set to indicate the number of expressions in uncovs array

 \return Returns TRUE if combinational coverage information was found for the given functional unit; otherwise,
         returns FALSE to indicate that an error occurred.

 Gathers the covered and uncovered combinational logic information, storing their expressions in the covs and uncovs
 expression arrays.  Used by the GUI for verbose combinational logic output.
*/
bool combination_collect( char* funit_name, int funit_type, expression*** covs, int* cov_cnt, expression*** uncovs, int* uncov_cnt ) {

  bool        retval = TRUE;   /* Return value of this function */
  func_unit   funit;           /* Functional unit used for searching */
  funit_link* funitl;          /* Pointer to found functional unit link */
  stmt_iter   stmti;           /* Statement list iterator */
  int         any_missed;      /* Specifies if any of the subexpressions were missed in this expression */
  int         any_measurable;  /* Specifies if any of the subexpressions were measurable in this expression */
  int         cov_size;        /* Current maximum allocated space in covs array */
  int         uncov_size;      /* Current maximum allocated space in uncovs array */
 
  /* First, find functional unit in functional unit array */
  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    /* Reset combination counted bits */
    combination_reset_counted_exprs( funitl->funit->exp_head );

    /* Create an array that will hold the number of uncovered combinations */
    cov_size   = 20;
    uncov_size = 20;
    *cov_cnt   = 0;
    *uncov_cnt = 0;
    *covs      = (expression**)malloc_safe( (sizeof( expression* ) * cov_size),   __FILE__, __LINE__ );
    *uncovs    = (expression**)malloc_safe( (sizeof( expression* ) * uncov_size), __FILE__, __LINE__ );

    /* Display current instance missed lines */
    stmt_iter_reset( &stmti, funitl->funit->stmt_tail );
    stmt_iter_find_head( &stmti, FALSE );

    while( stmti.curr != NULL ) {

      any_missed     = 0;
      any_measurable = 0;

      combination_output_expr( stmti.curr->stmt->exp, 0, &any_missed, &any_measurable );

      /* Check for uncovered statements */
      if( any_missed == 1 ) {
        if( stmti.curr->stmt->exp->line != 0 ) {
          if( *uncov_cnt == uncov_size ) {
            uncov_size += 20;
            *uncovs     = (expression**)realloc( *uncovs, (sizeof( expression* ) * uncov_size) );
          }
          (*uncovs)[(*uncov_cnt)] = stmti.curr->stmt->exp;
          (*uncov_cnt)++;
        }
        stmti.curr->stmt->exp->suppl.part.comb_cntd = 0;
      }

      /* Check for covered statements */
      if( (any_missed == 0) && (any_measurable == 1) ) {
        if( stmti.curr->stmt->exp->line != 0 ) {
          if( *cov_cnt == cov_size ) {
            cov_size += 20;
            *covs     = (expression**)realloc( *covs, (sizeof( expression* ) * cov_size) );
          }
          (*covs)[(*cov_cnt)] = stmti.curr->stmt->exp;
          (*cov_cnt)++;
        }
        stmti.curr->stmt->exp->suppl.part.comb_cntd = 0;
      }

      stmt_iter_get_next_in_order( &stmti );

    }

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit to get combinational expression for.
 \param funit_type  Type of functional unit to get combinational expression for.
 \param expr_id     Expression ID to retrieve information for
 \param code        Pointer to an array of strings containing generated code for this expression
 \param uline_groups  Pointer to an array of integers used for underlined missed subexpressions in this expression
 \param code_size     Pointer to value that will be set to indicate the number of elements in the code array
 \param ulines        Pointer to an array of strings that contain underlines of missed subexpressions
 \param uline_size    Pointer to value that will be set to indicate the number of elements in the ulines array
 
 \return Returns TRUE if the given expression was found; otherwise, returns FALSE to indicate an error occurred.

 Gets the combinational logic coverage information for the specified expression ID, storing the output in the
 code and ulines arrays.  Used by the GUI for displaying an expression's coverage information.
*/
bool combination_get_expression( char* funit_name, int funit_type, int expr_id, char*** code, int** uline_groups, int* code_size,
                                 char*** ulines, int* uline_size ) {

  bool        retval    = TRUE;  /* Return value for this function */
  func_unit   funit;             /* Functional unit used for searching */
  funit_link* funitl;            /* Pointer to found functional unit link */
  expression  exp;               /* Expression used for searching */
  exp_link*   expl;              /* Pointer to found signal link */
  int         tmp;               /* Temporary integer (unused) */
  int         i, j;
  char**      tmp_ulines;
  int         tmp_uline_size;
  int         start     = 0;
  int         uline_max = 20;

  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    exp.id = expr_id;

    if( (expl = exp_link_find( &exp, funitl->funit->exp_head )) != NULL ) {

      /* Generate line of code that missed combinational coverage */
      codegen_gen_expr( expl->exp, expl->exp->op, code, code_size, funitl->funit );
      *uline_groups = (int*)malloc_safe( sizeof( int ) * (*code_size), __FILE__, __LINE__ );

      /* Output underlining feature for missed expressions */
      combination_underline_tree( expl->exp, 0, &tmp_ulines, &tmp_uline_size, &tmp, expl->exp->op, (*code_size == 1), funitl->funit );

      *ulines     = (char**)malloc_safe( sizeof( char* ) * uline_max, __FILE__, __LINE__ );
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
                *ulines    = (char**)realloc( *ulines, (sizeof( char* ) * uline_max) );
              }
            }
          }
        }

        start += strlen( (*code)[i] );

      }

      for( i=0; i<tmp_uline_size; i++ ) {
        free_safe( tmp_ulines[i] );
      }

      if( tmp_uline_size > 0 ) {
        free_safe( tmp_ulines );
      }

    } else {

      retval = FALSE;

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit containing subexpression to get coverage detail for
 \param funit_type  Type of functional unit containing subexpression to get coverage detail for
 \param exp_id      Expression ID of statement containing subexpression to get coverage detail for
 \param uline_id    Underline ID of subexpression to get coverage detail for
 \param info        Pointer to string array that will be populated with the coverage detail
 \param info_size   Number of entries in info array

 \return Returns TRUE if we were successful in obtaining coverage detail for the specified information;
         otherwise, returns FALSE to indicate an error.

 Calculates the coverage detail for the specified subexpression and stores this in string format in the
 info and info_size arguments.  The coverage detail created matches the coverage detail output format that
 is used in the ASCII reports.
*/
bool combination_get_coverage( char* funit_name, int funit_type, int exp_id, int uline_id, char*** info, int* info_size ) {

  bool        retval = FALSE;  /* Return value for this function */
  func_unit   funit;           /* Module used to find specified functional unit */
  funit_link* funitl;          /* Pointer to found functional unit link */
  exp_link*   expl;            /* Pointer to current expression link */
  expression* exp;             /* Pointer to found expression */
  expression  tmpexp;          /* Temporary expression used for searching */

  funit.name = funit_name;
  funit.type = funit_type;

  /* Find the functional unit containing this subexpression */
  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    /* Find statement containing this expression */
    tmpexp.id = exp_id;
    if( (expl = exp_link_find( &tmpexp, funitl->funit->exp_head )) != NULL ) {

      /* Now find the subexpression that matches the given underline ID */
      if( (exp = expression_find_uline_id( expl->exp, uline_id )) != NULL ) {
        combination_get_missed_expr( info, info_size, exp, 0 );
        retval = TRUE;
      }

    }

  }

  return( retval );

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information

 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the combinational logic coverage for each functional unit encountered.  The parent 
 functional unit will specify its own combinational logic coverage along with a total combinational
 logic coverage including its children.
*/
void combination_report( FILE* ofile, bool verbose ) {

  bool missed_found;  /* If set to TRUE, indicates combinations were missed */
  char tmp[4096];     /* Temporary string value                             */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   COMBINATIONAL LOGIC COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( leading_hiers_differ ) {
      strcpy( tmp, "<NA>" );
    } else {
      assert( leading_hier_num > 0 );
      strcpy( tmp, leading_hierarchies[0] );
    }

    fprintf( ofile, "Instance                                                                    Logic Combinations\n" );
    fprintf( ofile, "                                                                      Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = combination_instance_summary( ofile, instance_root, tmp );
    
    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      combination_instance_verbose( ofile, instance_root, tmp );
    }

  } else {

    fprintf( ofile, "Module/Task/Function                Filename                                Logic Combinations\n" );
    fprintf( ofile, "                                                                      Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = combination_funit_summary( ofile, funit_head );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      combination_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}


/*
 $Log$
 Revision 1.139  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.138  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.137.8.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.137  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.136  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.135  2006/03/24 19:01:44  phase1geo
 Changed two variable report output to be as concise as possible.  Updating
 regressions per these changes.

 Revision 1.134  2006/03/23 22:42:54  phase1geo
 Changed two variable combinational expressions that have a constant value in either the
 left or right expression tree to unary expressions.  Removed expressions containing only
 static values from coverage totals.  Fixed bug in stmt_iter_get_next_in_order for looping
 cases (the verbose output was not being emitted for these cases).  Updated regressions for
 these changes -- full regression passes.

 Revision 1.133  2006/03/22 22:00:43  phase1geo
 Fixing bug in missed combinational logic determination where a static expression
 on the left/right of a combination expression should cause the entire expression to
 be considered fully covered.  Regressions have not been run which may contain some
 miscompares due to this change.

 Revision 1.132  2006/03/20 16:43:38  phase1geo
 Fixing code generator to properly display expressions based on lines.  Regression
 still needs to be updated for these changes.

 Revision 1.131  2006/03/15 22:48:29  phase1geo
 Updating run program.  Fixing bugs in statement_connect algorithm.  Updating
 regression files.

 Revision 1.130  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.129  2006/02/06 22:48:33  phase1geo
 Several enhancements to GUI look and feel.  Fixed error in combinational logic
 window.

 Revision 1.128  2006/02/06 05:07:26  phase1geo
 Fixed expression_set_static_only function to consider static expressions
 properly.  Updated regression as a result of this change.  Added files
 for signed3 diagnostic.  Documentation updates for GUI (these are not quite
 complete at this time yet).

 Revision 1.127  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.126  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.125  2006/01/25 16:51:26  phase1geo
 Fixing performance/output issue with hierarchical references.  Added support
 for hierarchical references to parser.  Full regression passes.

 Revision 1.124  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.123  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.122  2006/01/13 04:01:04  phase1geo
 Adding support for exponential operation.  Added exponent1 diagnostic to verify
 but Icarus does not support this currently.

 Revision 1.121  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.120  2006/01/10 05:12:48  phase1geo
 Added arithmetic left and right shift operators.  Added ashift1 diagnostic
 to verify their correct operation.

 Revision 1.119  2006/01/06 18:54:03  phase1geo
 Breaking up expression_operate function into individual functions for each
 expression operation.  Also storing additional information in a globally accessible,
 constant structure array to increase performance.  Updating full regression for these
 changes.  Full regression passes.

 Revision 1.118  2005/12/13 23:15:14  phase1geo
 More fixes for memory leaks.  Regression fully passes at this point.

 Revision 1.117  2005/12/08 22:50:58  phase1geo
 Adding support for while loops.  Added while1 and while1.1 to regression suite.
 Ran VCS on regression suite and updated.  Full regression passes.

 Revision 1.116  2005/12/07 21:50:50  phase1geo
 Added support for repeat blocks.  Added repeat1 to regression and fixed errors.
 Full regression passes.

 Revision 1.115  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.114  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.113  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.112  2005/11/17 23:35:16  phase1geo
 Blocking assignment is now working properly along with support for event expressions
 (currently only the original PEDGE, NEDGE, AEDGE and DELAY are supported but more
 can now follow).  Added new race4 diagnostic to verify that a register cannot be
 assigned from more than one location -- this works.  Regression fails at this point.

 Revision 1.111  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.110  2005/11/10 19:28:22  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.109  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.108  2005/05/02 15:33:34  phase1geo
 Updates.

 Revision 1.107  2005/02/05 04:13:27  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.106  2005/01/07 23:30:08  phase1geo
 Adding ability to handle strings in expressions.  Added string1.v diagnostic
 to verify this functionality.  Updated regressions for this change.

 Revision 1.105  2005/01/07 23:00:09  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.104  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.103  2004/09/07 03:17:13  phase1geo
 Fixing bug that did not allow combinational logic to be revisited in GUI properly.
 Also removing comments from bgerror function in Tcl code.

 Revision 1.102  2004/08/17 15:23:37  phase1geo
 Added combinational logic coverage output to GUI.  Modified comb.c code to get this
 to work that impacts ASCII coverage output; however, regression is fully passing with
 these changes.  Combinational coverage for GUI is mostly complete regarding information
 and usability.  Possibly some cleanup in output and in code is needed.

 Revision 1.101  2004/08/17 04:43:57  phase1geo
 Updating unary and binary combinational expression output functions to create
 string arrays instead of immediately sending the information to standard output
 to support GUI interface as well as ASCII interface.

 Revision 1.100  2004/08/13 20:45:05  phase1geo
 More added for combinational logic verbose reporting.  At this point, the
 code is being output with underlines that can move up and down the expression
 tree.  No expression reporting is being done at this time, however.

 Revision 1.99  2004/08/11 22:11:38  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.98  2004/03/20 15:26:50  phase1geo
 Fixing assertion error in report command for multi-value expression display.
 Added multi_exp3 report information to regression suite and fixed regression
 Makefile.

 Revision 1.97  2004/03/19 22:34:23  phase1geo
 Removing assertion error from DELAY expression underline output.

 Revision 1.96  2004/03/18 23:01:45  phase1geo
 Fixing combination_output_expr function to correctly determine when an
 expression should be output to the report.  Full regression now passes.
 Also modified TCL scripts to not be dependent on the original widgets
 package (we use straight TCL for now).

 Revision 1.95  2004/03/18 14:06:51  phase1geo
 Fixing combination_output_expr to work correctly for covered and uncovered
 modes.

 Revision 1.94  2004/03/18 04:43:23  phase1geo
 Cleaning up verbose output.  The last modification still isn't working
 exactly as hoped; more work to do here.

 Revision 1.93  2004/03/17 23:04:08  phase1geo
 Attempting to fix output problem when -c option is specified for expressions
 that contain non-measurable subexpressions.

 Revision 1.92  2004/03/17 13:25:00  phase1geo
 Fixing some more report-related bugs.  Added new diagnostics to regression
 suite to test for these.

 Revision 1.91  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.90  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.89  2004/01/31 18:58:35  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.88  2004/01/30 23:23:22  phase1geo
 More report output improvements.  Still not ready with regressions.

 Revision 1.87  2004/01/30 06:04:42  phase1geo
 More report output format tweaks.  Adjusted lines and spaces to make things
 look more organized.  Still some more to go.  Regression will fail at this
 point.

 Revision 1.86  2004/01/27 23:16:08  phase1geo
 Tweaks and bug fix to combination_is_multi_expr function.  Removed test.v
 and test1.v and renamed them to multi_exp2.v and multi_exp2.1.v, respectively.
 Added these new diagnostics to the regression suite.

 Revision 1.85  2004/01/27 13:34:30  phase1geo
 Working version of combinational logic report output but I still want to clean
 this code up.

 Revision 1.84  2004/01/26 19:09:46  phase1geo
 Fixes to comb.c for last checkin.  We are not quite there yet with the output
 quality for comb.c but we are close.  Next round of changes for comb.c should
 do the trick.

 Revision 1.83  2004/01/26 05:39:36  phase1geo
 Initial swag at new underline ID algorithm.  This not quite working correctly
 at this time.  Added two generic diagnostics to regression suite that will
 test this new capability.

 Revision 1.82  2004/01/25 03:41:48  phase1geo
 Fixes bugs in summary information not matching verbose information.  Also fixes
 bugs where instances were output when no logic was missing, where instance
 children were missing but not output.  Changed code to output summary
 information on a per instance basis (where children instances are not merged
 into parent instance summary information).  Updated regressions as a result.
 Updates to user documentation (though this is not complete at this time).

 Revision 1.81  2004/01/23 14:36:26  phase1geo
 Fixing output of instance line, toggle, comb and fsm to only output module
 name if logic is detected missing in that instance.  Full regression fails
 with this fix.

 Revision 1.80  2004/01/10 05:19:56  phase1geo
 Changing missed cases to use asterisk instead of capital M for signifier.
 Updated regression for last batch of changes.  Full regression now passes.

 Revision 1.79  2004/01/09 23:50:08  phase1geo
 Updated look of unary, two vars and multi vars combinational logic report
 output to be more succinct.

 Revision 1.78  2004/01/03 06:03:51  phase1geo
 Fixing file changes from last checkin.

 Revision 1.77  2004/01/02 22:11:03  phase1geo
 Updating regression for latest batch of changes.  Full regression now passes.
 Fixed bug with event or operation in report command.

 Revision 1.76  2003/12/30 23:02:28  phase1geo
 Contains rest of fixes for multi-expression combinational logic report output.
 Full regression fails currently.

 Revision 1.75  2003/12/22 23:37:02  phase1geo
 More fixes to report output code.

 Revision 1.74  2003/12/22 23:18:09  phase1geo
 More work on combinational logic report output.  Still not quite there in the
 look and feel and full regression should still fail.

 Revision 1.73  2003/12/19 23:08:48  phase1geo
 Adding initial version of expression report concatenation for more easily
 viewable/understandable reporting for combinational logic coverage.  This
 code is not yet completely set and debugged.

 Revision 1.72  2003/12/18 18:40:23  phase1geo
 Increasing detailed depth from 1 to 2 and making detail depth somewhat
 programmable.

 Revision 1.71  2003/12/13 05:52:02  phase1geo
 Removed verbose output and updated development documentation for new code.

 Revision 1.70  2003/12/13 03:26:39  phase1geo
 Adding code to optimize cases where output code is only on one line (no
 need to reformat the line in this case).

 Revision 1.69  2003/12/12 22:39:13  phase1geo
 Adding rest of line wrap code.  Full regression should now pass.

 Revision 1.68  2003/12/12 17:16:25  phase1geo
 Changing code generator to output logic based on user supplied format.  Full
 regression fails at this point due to mismatching report files.

 Revision 1.67  2003/11/30 05:46:45  phase1geo
 Adding IF report outputting capability.  Updated always9 diagnostic for these
 changes and updated rest of regression CDD files accordingly.

 Revision 1.66  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.65  2003/10/11 05:15:07  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.64  2003/10/03 03:08:44  phase1geo
 Modifying filename in summary output to only specify basename of file instead
 of entire path.  The verbose report contains the full pathname still, however.

 Revision 1.63  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.62  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.61  2002/12/07 17:46:52  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.60  2002/12/05 14:45:17  phase1geo
 Removing assertion error from instance6.1 failure; however, this case does not
 work correctly according to instance6.2.v diagnostic.  Added @(...) output in
 report command for edge-triggered events.  Also fixed bug where a module could be
 parsed more than once.  Full regression does not pass at this point due to
 new instance6.2.v diagnostic.

 Revision 1.59  2002/11/30 05:06:21  phase1geo
 Fixing bug in report output for covered results.  Allowing any nettype to
 be parsable and usable by Covered (even though some of these are unsupported
 by Icarus Verilog at the current moment).  Added diagnostics to test all
 net types and their proper handling.  Full regression passes at this point.

 Revision 1.58  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.57  2002/11/24 14:38:12  phase1geo
 Updating more regression CDD files for bug fixes.  Fixing bugs where combinational
 expressions were counted more than once.  Adding new diagnostics to regression
 suite.

 Revision 1.56  2002/11/23 21:27:25  phase1geo
 Fixing bug with combinational logic being output when unmeasurable.

 Revision 1.55  2002/11/23 16:10:46  phase1geo
 Updating changelog and development documentation to include FSM description
 (this is a brainstorm on how to handle FSMs when we get to this point).  Fixed
 bug with code underlining function in handling parameter in reports.  Fixing bugs
 with MBIT/SBIT handling (this is not verified to be completely correct yet).

 Revision 1.54  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.53  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.52  2002/10/31 23:13:21  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.51  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.50  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.49  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.48  2002/10/25 03:44:39  phase1geo
 Fixing bug in comb.c that caused statically allocated string to be exceeded
 which caused memory corruption problems.  Full regression now passes.

 Revision 1.47  2002/10/24 23:19:38  phase1geo
 Making some fixes to report output.  Fixing bugs.  Added long_exp1.v diagnostic
 to regression suite which finds a current bug in the report underlining
 functionality.  Need to look into this.

 Revision 1.46  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.45  2002/10/01 13:21:24  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.44  2002/09/25 22:41:29  phase1geo
 Adding diagnostics to check missing concatenation cases that uncovered bugs
 in testing other codes.  Also fixed case in report command for summary information.
 The combinational logic information was not being reported correctly for summary
 reports.

 Revision 1.43  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.42  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.41  2002/09/12 05:16:25  phase1geo
 Updating all CDD files in regression suite due to change in vector handling.
 Modified vectors to assign a default value of 0xaa to unassigned registers
 to eliminate bugs where values never assigned and VCD file doesn't contain
 information for these.  Added initial working version of depth feature in
 report generation.  Updates to man page and parameter documentation.

 Revision 1.40  2002/09/10 05:40:09  phase1geo
 Adding support for MULTIPLY, DIVIDE and MOD in expression verbose display.
 Fixing cases where -c option was not generating covered information in
 line and combination report output.  Updates to assign1.v diagnostic for
 logic that is now supported by both Covered and IVerilog.  Updated assign1.cdd
 to account for correct coverage file for the updated assign1.v diagnostic.

 Revision 1.39  2002/08/20 05:55:25  phase1geo
 Starting to add combination depth option to report command.  Currently, the
 option is not implemented.

 Revision 1.38  2002/08/20 04:48:18  phase1geo
 Adding option to report command that allows the user to display logic that is
 being covered (-c option).  This overrides the default behavior of displaying
 uncovered logic.  This is useful for debugging purposes and understanding what
 logic the tool is capable of handling.

 Revision 1.37  2002/08/19 04:59:49  phase1geo
 Adjusting summary format to allow for larger line, toggle and combination
 counts.

 Revision 1.36  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.35  2002/07/20 13:58:01  phase1geo
 Fixing bug in EXP_OP_LAST for changes in binding.  Adding correct line numbering
 to lexer (tested).  Added '+' to report outputting for signals larger than 1 bit.
 Added mbit_sel1.v diagnostic to verify some multi-bit functionality.  Full
 regression passes.

 Revision 1.34  2002/07/16 00:05:31  phase1geo
 Adding support for replication operator (EXPAND).  All expressional support
 should now be available.  Added diagnostics to test replication operator.
 Rewrote binding code to be more efficient with memory use.

 Revision 1.33  2002/07/14 05:27:34  phase1geo
 Fixing report outputting to allow multiple modules/instances to be
 output.

 Revision 1.32  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.31  2002/07/10 16:27:17  phase1geo
 Fixing output for single/multi-bit select signals in reports.

 Revision 1.30  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.29  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.28  2002/07/09 23:13:10  phase1geo
 Fixing report output bug for conditionals.  Also adjusting combinational logic
 report outputting.

 Revision 1.27  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.26  2002/07/09 03:24:48  phase1geo
 Various fixes for module instantiantion handling.  This now works.  Also
 modified report output for toggle, line and combinational information.
 Regression passes.

 Revision 1.25  2002/07/05 05:01:51  phase1geo
 Removing unecessary debugging output.

 Revision 1.24  2002/07/05 05:00:13  phase1geo
 Removing CASE, CASEX, and CASEZ from line and combinational logic results.

 Revision 1.23  2002/07/05 00:10:18  phase1geo
 Adding report support for case statements.  Everything outputs fine; however,
 I want to remove CASE, CASEX and CASEZ expressions from being reported since
 it causes redundant and misleading information to be displayed in the verbose
 reports.  New diagnostics to check CASE expressions have been added and pass.

 Revision 1.22  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.21  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.20  2002/07/03 00:59:14  phase1geo
 Fixing bug with conditional statements and other "deep" expression trees.

 Revision 1.19  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.18  2002/06/27 21:18:48  phase1geo
 Fixing report Verilog output.  simple.v verilog diagnostic now passes.

 Revision 1.17  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.16  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.15  2002/06/21 05:55:05  phase1geo
 Getting some codes ready for writing simulation engine.  We should be set
 now.

 Revision 1.14  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

