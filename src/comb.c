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


extern mod_inst*    instance_root;
extern mod_link*    mod_head;

extern bool         report_covered;
extern unsigned int report_comb_depth;
extern bool         report_instance;
extern char         leading_hierarchy[4096];
extern char         second_hierarchy[4096];
extern int          line_width;
extern char         user_msg[USER_MSG_LENGTH];


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

      if( (exp->left != NULL) && (SUPPL_OP( exp->suppl ) == SUPPL_OP( exp->left->suppl )) ) {
        return( curr_depth );
      } else {
        return( curr_depth + 1 );
      }

    } else {

      if( (exp->right != NULL) && (SUPPL_OP( exp->suppl ) == SUPPL_OP( exp->right->suppl )) ) {
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
    and_op = (SUPPL_OP( exp->suppl ) == EXP_OP_AND) || (SUPPL_OP( exp->suppl ) == EXP_OP_LAND);

    /* Decide if our expression requires that this sequence gets underlined */
    if( !ul ) {
      if( and_op ) {
        ul = (((exp->suppl >> SUPPL_LSB_EVAL_11) & 0x1) == 0) || (SUPPL_WAS_FALSE( exp->left->suppl ) == 0) || (SUPPL_WAS_FALSE( exp->right->suppl ) == 0);
      } else {
        ul = (((exp->suppl >> SUPPL_LSB_EVAL_00) & 0x1) == 0) || (SUPPL_WAS_TRUE( exp->left->suppl )  == 0) || (SUPPL_WAS_TRUE( exp->right->suppl )  == 0);
      }
    }

    if( (exp->left != NULL) && (SUPPL_OP( exp->suppl ) != SUPPL_OP( exp->left->suppl )) ) {
      if( and_op ) {
        *hit += SUPPL_WAS_FALSE( exp->left->suppl );
      } else {
        *hit += SUPPL_WAS_TRUE( exp->left->suppl );
      }
      if( (exp->left->ulid == -1) && ul ) { 
        exp->left->ulid = *ulid;
        (*ulid)++;
      }
      (*total)++;
    } else {
      ul = combination_multi_expr_calc( exp->left, ulid, ul, hit, total );
    }

    if( and_op ) {
      *hit += SUPPL_WAS_FALSE( exp->right->suppl );
    } else {
      *hit += SUPPL_WAS_TRUE( exp->right->suppl );
    }
    if( (exp->right->ulid == -1) && ul ) {
      exp->right->ulid = *ulid;
      (*ulid)++;
    }
    (*total)++;

    if( (SUPPL_IS_ROOT( exp->suppl ) == 1) || (SUPPL_OP( exp->suppl ) != SUPPL_OP( exp->parent->expr->suppl )) ) {
      if( and_op ) {
        *hit += ((exp->suppl >> SUPPL_LSB_EVAL_11) & 0x1);
      } else {
        *hit += ((exp->suppl >> SUPPL_LSB_EVAL_00) & 0x1);
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
          (SUPPL_IS_ROOT( exp->suppl ) == 0) && 
          (exp->parent->expr->left  != NULL) &&
          (exp->parent->expr->right != NULL) &&
          (exp->parent->expr->right->id == exp->id) &&
          (exp->parent->expr->left->ulid == -1) &&
          ( (SUPPL_OP( exp->parent->expr->suppl ) == EXP_OP_AND)  ||
            (SUPPL_OP( exp->parent->expr->suppl ) == EXP_OP_LAND) ||
            (SUPPL_OP( exp->parent->expr->suppl ) == EXP_OP_OR)   ||
            (SUPPL_OP( exp->parent->expr->suppl ) == EXP_OP_LOR) ) &&
          ( ( (SUPPL_IS_ROOT( exp->parent->expr->suppl ) == 0) &&
              (SUPPL_OP( exp->parent->expr->suppl ) == SUPPL_OP( exp->parent->expr->parent->expr->suppl )) ) ||
            (SUPPL_OP( exp->parent->expr->left->suppl ) == SUPPL_OP( exp->parent->expr->suppl )) ) );

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

  if( exp != NULL ) {

    /* Calculate children */
    combination_get_tree_stats( exp->left,  ulid, combination_calc_depth( exp, curr_depth, TRUE ),  total, hit );
    combination_get_tree_stats( exp->right, ulid, combination_calc_depth( exp, curr_depth, FALSE ), total, hit );

    if( ((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
         (report_comb_depth == REPORT_VERBOSE) ||
         (report_comb_depth == REPORT_SUMMARY) ) {

      if( (EXPR_IS_MEASURABLE( exp ) == 1) && (SUPPL_WAS_COMB_COUNTED( exp->suppl ) == 0) ) {

        if( (SUPPL_IS_ROOT( exp->suppl ) == 1) || (SUPPL_OP( exp->suppl ) != SUPPL_OP( exp->parent->expr->suppl )) ||
            ((SUPPL_OP( exp->suppl ) != EXP_OP_AND) &&
             (SUPPL_OP( exp->suppl ) != EXP_OP_LAND) &&
             (SUPPL_OP( exp->suppl ) != EXP_OP_OR)   &&
             (SUPPL_OP( exp->suppl ) != EXP_OP_LOR)) ) {

          /* Calculate current expression combination coverage */
          if( (exp->left != NULL) && (SUPPL_OP( exp->suppl ) == SUPPL_OP( exp->left->suppl )) &&
              ((SUPPL_OP( exp->suppl ) == EXP_OP_AND)  ||
               (SUPPL_OP( exp->suppl ) == EXP_OP_OR)   ||
               (SUPPL_OP( exp->suppl ) == EXP_OP_LAND) ||
               (SUPPL_OP( exp->suppl ) == EXP_OP_LOR)) ) {
            combination_multi_expr_calc( exp, ulid, FALSE, hit, total );
          } else {
            if( expression_is_static_only( exp ) ) {
              *total = *total + 2;
              *hit   = *hit + 2;
            } else if( EXPR_IS_COMB( exp ) == 1 ) {
              *total  = *total + 4;
              num_hit = ((exp->suppl >> SUPPL_LSB_EVAL_00) & 0x1) +
                        ((exp->suppl >> SUPPL_LSB_EVAL_01) & 0x1) +
                        ((exp->suppl >> SUPPL_LSB_EVAL_10) & 0x1) +
                        ((exp->suppl >> SUPPL_LSB_EVAL_11) & 0x1);
              *hit    = *hit + num_hit;
              if( (num_hit != 4) && (exp->ulid == -1) && !combination_is_expr_multi_node( exp ) ) {
                exp->ulid = *ulid;
                (*ulid)++;
              }
            } else {
              *total  = *total + 2;
              num_hit = SUPPL_WAS_TRUE( exp->suppl ) + SUPPL_WAS_FALSE( exp->suppl );
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

    /* Consider this expression to be counted */
    exp->suppl = exp->suppl | (0x1 << SUPPL_LSB_COMB_CNTD);

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
  int       ulid;      /* Current underline ID for this expression           */
  
  curr_exp = expl;

  while( curr_exp != NULL ) {
    if( SUPPL_IS_ROOT( curr_exp->exp->suppl ) == 1 ) {
      ulid = 1;
      combination_get_tree_stats( curr_exp->exp, &ulid, 0, total, hit );
    }
    curr_exp = curr_exp->next;
  }

}

/*!
 \param ofile   Pointer to file to output results to.
 \param root    Pointer to node in instance tree to evaluate.
 \param parent  Name of parent instance name.

 \return Returns TRUE if combinations were found to be missed; otherwise,
         returns FALSE.

 Outputs summarized results of the combinational logic coverage per module
 instance to the specified output stream.  Summarized results are printed 
 as percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
bool combination_instance_summary( FILE* ofile, mod_inst* root, char* parent ) {

  mod_inst* curr;           /* Pointer to current child module instance of this node */
  float     percent;        /* Percentage of lines hit                               */
  float     miss;           /* Number of lines missed                                */
  char      tmpname[4096];  /* Temporary name holder of instance                     */

  assert( root != NULL );
  assert( root->stat != NULL );

  if( root->stat->comb_total == 0 ) {
    percent = 100;
  } else {
    percent = ((root->stat->comb_hit / root->stat->comb_total) * 100);
  }
  miss    = (root->stat->comb_total - root->stat->comb_hit);

  if( strcmp( parent, "*" ) == 0 ) {
    strcpy( tmpname, root->name );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent, root->name );
  }

  fprintf( ofile, "  %-43.43s    %4d/%4.0f/%4.0f      %3.0f%%\n",
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

  return( miss > 0 );

}

/*!
 \param ofile   Pointer to file to output results to.
 \param head    Pointer to link in current module list to evaluate.

 \return Returns TRUE if combinations were found to be missed; otherwise,
         returns FALSE.

 Outputs summarized results of the combinational logic coverage per module
 to the specified output stream.  Summarized results are printed as 
 percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
bool combination_module_summary( FILE* ofile, mod_link* head ) {

  float percent;             /* Percentage of lines hit                        */
  float miss;                /* Number of lines missed                         */
  float miss_found = FALSE;  /* Set to TRUE if missing combinations were found */

  while( head != NULL ) {

    if( head->mod->stat->comb_total == 0 ) {
      percent = 100;
    } else {
      percent = ((head->mod->stat->comb_hit / head->mod->stat->comb_total) * 100);
    }
    miss = (head->mod->stat->comb_total - head->mod->stat->comb_hit);
    if( miss > 0 ) {
      miss_found = TRUE;
    }

    fprintf( ofile, "  %-20.20s    %-20.20s   %4d/%4.0f/%4.0f      %3.0f%%\n", 
             head->mod->name,
             get_basename( head->mod->filename ),
             head->mod->stat->comb_hit,
             miss,
             head->mod->stat->comb_total,
             percent );

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

  char str_exp_id[12];   /* String containing value of exp_id        */
  int  exp_id_size;      /* Number of characters exp_id is in length */
  int  i;                /* Loop iterator                            */

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

  char str_exp_id[12];   /* String containing value of exp_id        */
  int  exp_id_size;      /* Number of characters exp_id is in length */
  int  i;                /* Loop iterator                            */

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
 \param exp              Pointer to expression to create underline for.
 \param curr_depth       Specifies current depth in expression tree.
 \param lines            Stack of lines for left child.
 \param depth            Pointer to top of left child stack.
 \param size             Pointer to character width of this node.
 \param parent_op        Expression operation of parent used for calculating parenthesis.
 \param center           Specifies if expression IDs should be centered in underlines or at beginning.

 Recursively parses specified expression tree, underlining and labeling each
 measurable expression.
*/
void combination_underline_tree( expression* exp, unsigned int curr_depth, char*** lines, int* depth, int* size, int parent_op, bool center ) {

  char** l_lines;       /* Pointer to left underline stack              */
  char** r_lines;       /* Pointer to right underline stack             */
  int    l_depth = 0;   /* Index to top of left stack                   */
  int    r_depth = 0;   /* Index to top of right stack                  */
  int    l_size;        /* Number of characters for left expression     */
  int    r_size;        /* Number of characters for right expression    */
  int    i;             /* Loop iterator                                */
  char*  exp_sp;        /* Space to take place of missing expression(s) */
  char   code_fmt[300]; /* Contains format string for rest of stack     */
  char*  tmpstr;        /* Temporary string value                       */
  int    comb_missed;   /* If set to 1, current combination was missed  */
  char*  tmpname;       /* Temporary pointer to current signal name     */
  
  *depth  = 0;
  *size   = 0;
  l_lines = NULL;
  r_lines = NULL;

  if( exp != NULL ) {
    
    if( SUPPL_OP( exp->suppl ) == EXP_OP_LAST ) {

      *size = 0;

    } else if( SUPPL_OP( exp->suppl ) == EXP_OP_STATIC ) {

      if( exp->value->suppl == DECIMAL ) {

        snprintf( code_fmt, 300, "%d", vector_to_int( exp->value ) );
        *size = strlen( code_fmt );
      
      } else {

        tmpstr = vector_to_string( exp->value, exp->value->suppl );
        *size  = strlen( tmpstr );
        free_safe( tmpstr );

      }

    } else {

      if( (SUPPL_OP( exp->suppl ) == EXP_OP_SIG) ||
          (SUPPL_OP( exp->suppl ) == EXP_OP_PARAM) ) {

        if( exp->sig->name[0] == '#' ) {
          tmpname = exp->sig->name + 1;
        } else {
          tmpname = exp->sig->name;
        }

        *size = strlen( tmpname );
        switch( *size ) {
          case 0 :  assert( *size > 0 );                     break;
          case 1 :  *size = 3;  strcpy( code_fmt, " %s " );  break;
          case 2 :  *size = 3;  strcpy( code_fmt, " %s" );   break;
          default:  strcpy( code_fmt, "%s" );                break;
        }
        
      } else {

        combination_underline_tree( exp->left,  combination_calc_depth( exp, curr_depth, TRUE ),  &l_lines, &l_depth, &l_size, SUPPL_OP( exp->suppl ), center );
        combination_underline_tree( exp->right, combination_calc_depth( exp, curr_depth, FALSE ), &r_lines, &r_depth, &r_size, SUPPL_OP( exp->suppl ), center );

        if( parent_op == SUPPL_OP( exp->suppl ) ) {

          switch( SUPPL_OP( exp->suppl ) ) {
            case EXP_OP_XOR        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_MULTIPLY   :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_DIVIDE     :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_MOD        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_ADD        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_SUBTRACT   :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_AND        :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_OR         :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_NAND       :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_NOR        :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_NXOR       :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_LT         :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_GT         :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"        );  break;
            case EXP_OP_LSHIFT     :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
            case EXP_OP_RSHIFT     :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s"       );  break;
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

          switch( SUPPL_OP( exp->suppl ) ) {
            case EXP_OP_XOR        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_MULTIPLY   :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_DIVIDE     :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_MOD        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_ADD        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_SUBTRACT   :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_AND        :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_OR         :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_NAND       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_NOR        :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_NXOR       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_LT         :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_GT         :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "        );  break;
            case EXP_OP_LSHIFT     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
            case EXP_OP_RSHIFT     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "       );  break;
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

          switch( SUPPL_OP( exp->suppl ) ) {
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
              if( exp->sig->name[0] == '#' ) {
                tmpname = exp->sig->name + 1;
              } else {
                tmpname = exp->sig->name;
              }
              *size = l_size + r_size + strlen( tmpname ) + 2;
              for( i=0; i<strlen( tmpname ); i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              strcat( code_fmt, " %s " );
              break;
            case EXP_OP_PARAM_MBIT :
            case EXP_OP_MBIT_SEL   :  
              if( exp->sig->name[0] == '#' ) {
                tmpname = exp->sig->name + 1;
              } else {
                tmpname = exp->sig->name;
              }
              *size = l_size + r_size + strlen( tmpname ) + 3;  
              for( i=0; i<strlen( tmpname ); i++ ) {
                code_fmt[i] = ' ';
              }
              code_fmt[i] = '\0';
              strcat( code_fmt, " %s %s " );
              break;
            case EXP_OP_EXPAND   :  *size = l_size + r_size + 4;  strcpy( code_fmt, " %s %s  "         );  break;
            case EXP_OP_CONCAT   :  *size = l_size + r_size + 2;  strcpy( code_fmt, " %s "             );  break;
            case EXP_OP_LIST     :  *size = l_size + r_size + 2;  strcpy( code_fmt, "%s  %s"           );  break;
            case EXP_OP_PEDGE    :
              if( SUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 11;  strcpy( code_fmt, "          %s " );
              } else {
                *size = l_size + r_size + 8;   strcpy( code_fmt, "        %s" );
              }
              break;
            case EXP_OP_NEDGE    :
              if( SUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 11;  strcpy( code_fmt, "          %s " );
              } else {
                *size = l_size + r_size + 8;   strcpy( code_fmt, "        %s" );
              }
              break;
            case EXP_OP_AEDGE    :
              if( SUPPL_IS_ROOT( exp->suppl ) == 1 ) {
                *size = l_size + r_size + 3;  strcpy( code_fmt, "  %s " );
              } else {
                *size = l_size + r_size + 0;  strcpy( code_fmt, "%s" );
              }
              break;
            case EXP_OP_EOR      :
              if( SUPPL_IS_ROOT( exp->suppl ) == 1 ) {
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
            case EXP_OP_BASSIGN  :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s" );           break;
            case EXP_OP_NASSIGN  :  *size = l_size + r_size + 4;  strcpy( code_fmt, "%s    %s" );          break;
            case EXP_OP_IF       :  *size = r_size + 6;           strcpy( code_fmt, "    %s  " );          break;
            default              :
              snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Unknown expression type in combination_underline_tree (%d)",
                        SUPPL_OP( exp->suppl ) );
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

        /* Allocate memory for this underline */
        (*lines)[(*depth)-1] = (char*)malloc_safe( (*size + 1), __FILE__, __LINE__ );

        /* Create underline or space */
        if( comb_missed == 1 ) {
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

*/
char* combination_prep_line( char* line, int start, int len ) {

  char* str;                /* Prepared line to return                           */
  int   exp_id;             /* Expression ID of current line                     */
  int   chars_read;         /* Number of characters read from sscanf function    */
  int   i;                  /* Loop iterator                                     */
  int   curr_index;         /* Index current character in str to set             */
  bool  line_ip   = FALSE;  /* Specifies if a line is currently in progress      */
  bool  line_seen = FALSE;  /* Specifies that a line has been seen for this line */
  int   start_ul;           /* Index of starting underline                       */

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

 Traverses through the expression tree that is on the same line as the parent,
 creating underline strings.  An underline is created for each expression that
 does not have complete combination logic coverage.  Each underline (children to
 parent creates an inverted tree) and contains a number for the specified expression.
*/
void combination_underline( FILE* ofile, char** code, int code_depth, expression* exp ) {

  char** lines;    /* Pointer to a stack of lines     */
  int    depth;    /* Depth of underline stack        */
  int    size;     /* Width of stack in characters    */
  int    i;        /* Loop iterator                   */
  int    j;        /* Loop iterator                   */
  char*  tmpstr;   /* Temporary string variable       */
  int    start;    /* Starting index                  */
  int    k;

  start = 0;

  combination_underline_tree( exp, 0, &lines, &depth, &size, SUPPL_OP( exp->suppl ), (code_depth == 1) );

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
 \param ofile  Pointer to file to output results to.
 \param exp    Pointer to expression to evaluate.
 \param op     String showing current expression operation type.

 Displays the missed unary combination(s) that keep the combination coverage for
 the specified expression from achieving 100% coverage.
*/
void combination_unary( FILE* ofile, expression* exp, char* op ) {

  int hit = 0;

  assert( exp != NULL );

  /* Get hit information */
  hit = SUPPL_WAS_FALSE( exp->suppl ) + SUPPL_WAS_TRUE( exp->suppl );

  assert( exp->ulid != -1 );

  fprintf( ofile, "        Expression %d   (%d/2)\n", exp->ulid, hit );
  fprintf( ofile, "        ^^^^^^^^^^^^^ - %s\n", op );
  fprintf( ofile, "         E | E\n" );
  fprintf( ofile, "        =0=|=1=\n" );
  fprintf( ofile, "         %c   %c\n\n",
		  ((SUPPL_WAS_FALSE( exp->suppl ) == 1) ? ' ' : '*'),
		  ((SUPPL_WAS_TRUE( exp->suppl )  == 1) ? ' ' : '*') );

}

/*!
 \param ofile  Pointer to file to output results to.
 \param exp    Pointer to expression to evaluate.
 \param op     String showing current expression operation type.

 Displays the missed combinational sequences for the specified expression to the
 specified output stream in tabular form.
*/
void combination_two_vars( FILE* ofile, expression* exp, char* op ) {

  int hit = 0;

  assert( exp != NULL );

  /* Verify that left child expression is valid for this operation */
  assert( exp->left != NULL );

  /* Verify that right child expression is valid for this operation */
  assert( exp->right != NULL );

  /* Get hit information */
  hit = ((exp->suppl >> SUPPL_LSB_EVAL_00) & 0x1) +
        ((exp->suppl >> SUPPL_LSB_EVAL_01) & 0x1) +
        ((exp->suppl >> SUPPL_LSB_EVAL_10) & 0x1) +
        ((exp->suppl >> SUPPL_LSB_EVAL_11) & 0x1);

  assert( exp->ulid != -1 );

  fprintf( ofile, "        Expression %d   (%d/4)\n", exp->ulid, hit );
  fprintf( ofile, "        ^^^^^^^^^^^^^ - %s\n", op );
  fprintf( ofile, "         LR | LR | LR | LR \n" );
  fprintf( ofile, "        =00=|=01=|=10=|=11=\n" );
  fprintf( ofile, "         %c    %c    %c    %c\n\n",
                  ((((exp->suppl >> SUPPL_LSB_EVAL_00) & 0x1) == 1) ? ' ' : '*'),
                  ((((exp->suppl >> SUPPL_LSB_EVAL_01) & 0x1) == 1) ? ' ' : '*'),
                  ((((exp->suppl >> SUPPL_LSB_EVAL_10) & 0x1) == 1) ? ' ' : '*'),
                  ((((exp->suppl >> SUPPL_LSB_EVAL_11) & 0x1) == 1) ? ' ' : '*') );

}

/*!
 \param line1  Pointer to first line of multi-variable expression output.
 \param line2  Pointer to second line of multi-variable expression output.
 \param line3  Pointer to third line of multi-variable expression output.
 \param exp    Pointer to current expression to output.

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

    and_op = (SUPPL_OP( exp->suppl ) == EXP_OP_AND) || (SUPPL_OP( exp->suppl ) == EXP_OP_LAND);

    /* If we have hit the left-most expression, start creating string here */
    if( (exp->left != NULL) && (SUPPL_OP( exp->suppl ) != SUPPL_OP( exp->left->suppl )) ) {

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
        snprintf( left_line3, (curr_id_str_len + 4), " %c%s  ", ((SUPPL_WAS_FALSE( exp->left->suppl ) == 1) ? ' ' : '*'), curr_id_str );
      } else {
        snprintf( left_line3, (curr_id_str_len + 4), " %c%s  ", ((SUPPL_WAS_TRUE( exp->left->suppl )  == 1) ? ' ' : '*'), curr_id_str );
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
                left_line3, ((SUPPL_WAS_FALSE( exp->right->suppl ) == 1) ? ' ' : '*'), curr_id_str );
    } else {
      snprintf( *line3, (strlen( left_line3 ) + curr_id_str_len + 4), "%s %c%s  ",
                left_line3, ((SUPPL_WAS_TRUE( exp->right->suppl )  == 1) ? ' ' : '*'),  curr_id_str );
    }
    free_safe( left_line1 );
    free_safe( left_line2 );
    free_safe( left_line3 );

    /* If we are the root, output all value */
    if( (SUPPL_IS_ROOT( exp->suppl ) == 1) || (SUPPL_OP( exp->suppl ) != SUPPL_OP( exp->parent->expr->suppl )) ) {
      left_line1 = *line1;
      left_line2 = *line2;
      left_line3 = *line3;
      *line1     = (char*)malloc_safe( (strlen( left_line1 ) + 7), __FILE__, __LINE__ );
      *line2     = (char*)malloc_safe( (strlen( left_line2 ) + 7), __FILE__, __LINE__ );
      *line3     = (char*)malloc_safe( (strlen( left_line3 ) + 7), __FILE__, __LINE__ );
      if( and_op ) {
        snprintf( *line1, (strlen( left_line1 ) + 7), "%s All",   left_line1 );
        snprintf( *line2, (strlen( left_line2 ) + 7), "%s==1==",  left_line2 );
        snprintf( *line3, (strlen( left_line3 ) + 7), "%s  %c  ", left_line3, ((((exp->suppl >> SUPPL_LSB_EVAL_11) & 0x1) == 1) ? ' ' : '*') );
      } else {
        snprintf( *line1, (strlen( left_line1 ) + 7), "%s All",   left_line1 );
        snprintf( *line2, (strlen( left_line2 ) + 7), "%s==0==",  left_line2 );
        snprintf( *line3, (strlen( left_line3 ) + 7), "%s  %c  ", left_line3, ((((exp->suppl >> SUPPL_LSB_EVAL_00) & 0x1) == 1) ? ' ' : '*') );
      }
      free_safe( left_line1 );
      free_safe( left_line2 );
      free_safe( left_line3 );
    }

  }

}

/*!
 \param ofile  Pointer to file to output report contents to.
 \param line1  First line of multi-variable expression output.
 \param line2  Second line of multi-variable expression output.
 \param line3  Third line of multi-variable expression output.
*/
void combination_multi_expr_output( FILE* ofile, char* line1, char* line2, char* line3 ) {

  int start = 0;
  int i;
  int len   = strlen( line1 );

  for( i=0; i<len; i++ ) {

    if( (i + 1) == len ) {

      fprintf( ofile, "        %s\n",   (line1 + start) );
      fprintf( ofile, "        %s\n",   (line2 + start) );
      fprintf( ofile, "        %s\n\n", (line3 + start) );

    } else if( (line1[i] == '|') && ((i - start) >= line_width) ) {

      line1[i] = '\0';
      line2[i] = '\0';
      line3[i] = '\0';
      fprintf( ofile, "        %s|\n",   (line1 + start) );
      fprintf( ofile, "        %s|\n",   (line2 + start) );
      fprintf( ofile, "        %s \n\n", (line3 + start) );
      start = i + 1;

    }

  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param exp    Pointer to top-level AND/OR expression to evaluate.

 Displays the missed combinational sequences for the specified expression to the
 specified output stream in tabular form.
*/
void combination_multi_vars( FILE* ofile, expression* exp ) {

  int         ulid    = 1;
  float       total   = 0;
  int         hit     = 0;
  char*       line1   = NULL;
  char*       line2   = NULL;
  char*       line3   = NULL;

  /* Only output this expression if we are missing coverage. */
  if( exp->ulid != -1 ) {

    /* Gather report output for this expression */
    combination_multi_var_exprs( &line1, &line2, &line3, exp );

    /* Calculate hit and total values for this sub-expression */
    combination_multi_expr_calc( exp, &ulid, FALSE, &hit, &total );

    fprintf( ofile, "        Expression %d   (%d/%.0f)\n", exp->ulid, hit, total );

    switch( SUPPL_OP( exp->suppl ) ) {
      case EXP_OP_AND  :  fprintf( ofile, "        ^^^^^^^^^^^^^ - &\n" );   break;
      case EXP_OP_OR   :  fprintf( ofile, "        ^^^^^^^^^^^^^ - |\n" );   break;
      case EXP_OP_LAND :  fprintf( ofile, "        ^^^^^^^^^^^^^ - &&\n" );  break;
      case EXP_OP_LOR  :  fprintf( ofile, "        ^^^^^^^^^^^^^ - ||\n" );  break;
      default          :  break;
    }

    /* Output the lines paying attention to the current line width */
    combination_multi_expr_output( ofile, line1, line2, line3 );

    free_safe( line1 );
    free_safe( line2 );
    free_safe( line3 );

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

  if( exp != NULL ) {
    
    combination_list_missed( ofile, exp->left,  combination_calc_depth( exp, curr_depth, TRUE ) );
    combination_list_missed( ofile, exp->right, combination_calc_depth( exp, curr_depth, FALSE ) );

    if( (EXPR_COMB_MISSED( exp ) == 1) && 
        (((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
          (report_comb_depth == REPORT_VERBOSE)) ) {

      if( (SUPPL_IS_ROOT( exp->suppl ) == 1) || (SUPPL_OP( exp->suppl ) != SUPPL_OP( exp->parent->expr->suppl )) ||
          ((SUPPL_OP( exp->suppl ) != EXP_OP_AND)  &&
           (SUPPL_OP( exp->suppl ) != EXP_OP_LAND) &&
           (SUPPL_OP( exp->suppl ) != EXP_OP_OR)   &&
           (SUPPL_OP( exp->suppl ) != EXP_OP_LOR)) ) {

        if( (exp->left != NULL) && (SUPPL_OP( exp->suppl ) == SUPPL_OP( exp->left->suppl )) &&
            ((SUPPL_OP( exp->suppl ) == EXP_OP_AND)  ||
             (SUPPL_OP( exp->suppl ) == EXP_OP_OR)   ||
             (SUPPL_OP( exp->suppl ) == EXP_OP_LAND) ||
             (SUPPL_OP( exp->suppl ) == EXP_OP_LOR)) ) {

          combination_multi_vars( ofile, exp );

        } else {

          /* Create combination table */
          switch( SUPPL_OP( exp->suppl ) ) {
            case EXP_OP_SIG        :  combination_unary( ofile, exp, "" );         break;
            case EXP_OP_XOR        :  combination_two_vars( ofile, exp, "^" );     break;
            case EXP_OP_ADD        :  combination_two_vars( ofile, exp, "+" );     break;
            case EXP_OP_SUBTRACT   :  combination_two_vars( ofile, exp, "-" );     break;
            case EXP_OP_MULTIPLY   :  combination_unary( ofile, exp, "*" );        break;
            case EXP_OP_DIVIDE     :  combination_unary( ofile, exp, "/" );        break;
            case EXP_OP_MOD        :  combination_unary( ofile, exp, "%%" );       break;
            case EXP_OP_AND        :  combination_two_vars( ofile, exp, "&" );     break;
            case EXP_OP_OR         :  combination_two_vars( ofile, exp, "|" );     break;
            case EXP_OP_NAND       :  combination_two_vars( ofile, exp, "~&" );    break;
            case EXP_OP_NOR        :  combination_two_vars( ofile, exp, "~|" );    break;
            case EXP_OP_NXOR       :  combination_two_vars( ofile, exp, "~^" );    break;
            case EXP_OP_LT         :  combination_unary( ofile, exp, "<" );        break;
            case EXP_OP_GT         :  combination_unary( ofile, exp, ">" );        break;
            case EXP_OP_LSHIFT     :  combination_unary( ofile, exp, "<<" );       break;
            case EXP_OP_RSHIFT     :  combination_unary( ofile, exp, ">>" );       break;
            case EXP_OP_EQ         :  combination_unary( ofile, exp, "==" );       break;
            case EXP_OP_CEQ        :  combination_unary( ofile, exp, "===" );      break;
            case EXP_OP_LE         :  combination_unary( ofile, exp, "<=" );       break;
            case EXP_OP_GE         :  combination_unary( ofile, exp, ">=" );       break;
            case EXP_OP_NE         :  combination_unary( ofile, exp, "!=" );       break;
            case EXP_OP_CNE        :  combination_unary( ofile, exp, "!==" );      break;
            case EXP_OP_COND       :  combination_unary( ofile, exp, "?:" );       break;
            case EXP_OP_LOR        :  combination_two_vars( ofile, exp, "||" );    break;
            case EXP_OP_LAND       :  combination_two_vars( ofile, exp, "&&" );    break;
            case EXP_OP_UINV       :  combination_unary( ofile, exp, "~" );        break;
            case EXP_OP_UAND       :  combination_unary( ofile, exp, "&" );        break;
            case EXP_OP_UNOT       :  combination_unary( ofile, exp, "!" );        break;
            case EXP_OP_UOR        :  combination_unary( ofile, exp, "|" );        break;
            case EXP_OP_UXOR       :  combination_unary( ofile, exp, "^" );        break;
            case EXP_OP_UNAND      :  combination_unary( ofile, exp, "~&" );       break;
            case EXP_OP_UNOR       :  combination_unary( ofile, exp, "~|" );       break;
            case EXP_OP_UNXOR      :  combination_unary( ofile, exp, "~^" );       break;
            case EXP_OP_PARAM_SBIT :
            case EXP_OP_SBIT_SEL   :  combination_unary( ofile, exp, "[]" );       break;
            case EXP_OP_PARAM_MBIT :
            case EXP_OP_MBIT_SEL   :  combination_unary( ofile, exp, "[:]" );      break;
            case EXP_OP_EXPAND     :  combination_unary( ofile, exp, "{{}}" );     break;
            case EXP_OP_CONCAT     :  combination_unary( ofile, exp, "{}" );       break;
            case EXP_OP_CASE       :  combination_unary( ofile, exp, "" );         break;
            case EXP_OP_CASEX      :  combination_unary( ofile, exp, "" );         break;
            case EXP_OP_CASEZ      :  combination_unary( ofile, exp, "" );         break;        
            case EXP_OP_PEDGE      :  combination_unary( ofile, exp, "posedge" );  break;
            case EXP_OP_NEDGE      :  combination_unary( ofile, exp, "negedge" );  break;
            case EXP_OP_AEDGE      :  combination_unary( ofile, exp, "" );         break;
            default                :  break;
          }
      
        }

      }

    }

  }

}

/*!
 \param expr        Pointer to root of expression tree to search.
 \param curr_depth  Specifies current depth of expression tree.

 Recursively traverses specified expression tree, returning TRUE
 if an expression is found that has not received 100% coverage for
 combinational logic.
*/
bool combination_missed_expr( expression* expr, unsigned int curr_depth ) {

  bool missed_right;  /* Set to TRUE if missed expression found on right */
  bool missed_left;   /* Set to TRUE if missed expression found on left  */

  if( (expr != NULL) && (SUPPL_WAS_COMB_COUNTED( expr->suppl ) == 1) ) {

    expr->suppl  = expr->suppl & ~(0x1 << SUPPL_LSB_COMB_CNTD);

    missed_right = combination_missed_expr( expr->right, combination_calc_depth( expr, curr_depth, FALSE ) );
    missed_left  = combination_missed_expr( expr->left,  combination_calc_depth( expr, curr_depth, TRUE ) );

    if( ((report_comb_depth == REPORT_DETAILED) && (curr_depth <= report_comb_depth)) ||
         (report_comb_depth == REPORT_VERBOSE) ) {

      return( (expr->ulid != -1) || missed_right || missed_left );

    } else {

      return( missed_right || missed_left );

    }

  } else {

    return( FALSE );

  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param stmtl  Pointer to statement list head.

 Displays the expressions (and groups of expressions) that were considered 
 to be measurable (evaluates to a value of TRUE (1) or FALSE (0) but were 
 not hit during simulation.  The entire Verilog expression is displayed
 to the specified output stream with each of its measured expressions being
 underlined and numbered.  The missed combinations are then output below
 the Verilog code, showing those logical combinations that were not hit
 during simulation.
*/
void combination_display_verbose( FILE* ofile, stmt_link* stmtl ) {

  stmt_iter   stmti;         /* Statement list iterator                  */
  expression* unexec_exp;    /* Pointer to current unexecuted expression */
  char**      code;          /* Code string from code generator          */
  int         code_depth;    /* Depth of code array                      */

  if( report_covered ) {
    fprintf( ofile, "    Hit Combinations\n\n" );
  } else { 
    fprintf( ofile, "    Missed Combinations  (* = missed value)\n\n" );
  }

  /* Display current instance missed lines */
  stmt_iter_reset( &stmti, stmtl );
  stmt_iter_find_head( &stmti, FALSE );

  while( stmti.curr != NULL ) {

    if( combination_missed_expr( stmti.curr->stmt->exp, 0 ) != report_covered ) {

      stmti.curr->stmt->exp->suppl = stmti.curr->stmt->exp->suppl & ~(0x1 << SUPPL_LSB_COMB_CNTD);
      unexec_exp = stmti.curr->stmt->exp;

      fprintf( ofile, "      =========================================================================================================\n" );
      fprintf( ofile, "       Line #     Expression\n" );
      fprintf( ofile, "      =========================================================================================================\n" );

      /* Generate line of code that missed combinational coverage */
      codegen_gen_expr( unexec_exp, SUPPL_OP( unexec_exp->suppl ), &code, &code_depth );

      /* Output underlining feature for missed expressions */
      combination_underline( ofile, code, code_depth, unexec_exp );
      fprintf( ofile, "\n" );

      /* Output logical combinations that missed complete coverage */
      combination_list_missed( ofile, unexec_exp, 0 );

    }
    
    stmt_iter_get_next_in_order( &stmti );

  }

}

/*!
 \param ofile   Pointer to file to output results to.
 \param root    Pointer to current module instance to evaluate.
 \param parent  Name of parent instance.

 Outputs the verbose coverage report for the specified module instance
 to the specified output stream.
*/
void combination_instance_verbose( FILE* ofile, mod_inst* root, char* parent ) {

  mod_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char      tmpname[4096];  /* Temporary name holder of instance           */

  assert( root != NULL );

  if( strcmp( parent, "*" ) == 0 ) {
    strcpy( tmpname, root->name );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent, root->name );
  }

  if( ((root->stat->comb_hit < root->stat->comb_total) && !report_covered) ||
      ((root->stat->comb_hit > 0) && report_covered) ) {

    fprintf( ofile, "\n" );
    fprintf( ofile, "    Module: %s, File: %s, Instance: %s\n", 
             root->mod->name, 
             root->mod->filename,
             tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

    combination_display_verbose( ofile, root->mod->stmt_tail );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    combination_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to current module to evaluate.

 Outputs the verbose coverage report for the specified module
 to the specified output stream.
*/
void combination_module_verbose( FILE* ofile, mod_link* head ) {

  while( head != NULL ) {

    if( ((head->mod->stat->comb_hit < head->mod->stat->comb_total) && !report_covered) ||
        ((head->mod->stat->comb_hit > 0) && report_covered) ) {

      fprintf( ofile, "\n" );
      fprintf( ofile, "    Module: %s, File: %s\n", 
               head->mod->name, 
               head->mod->filename );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      combination_display_verbose( ofile, head->mod->stmt_tail );

    }

    head = head->next;

  }

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the combinational logic coverage for each module encountered.  The parent 
 module will specify its own combinational logic coverage along with a total combinational
 logic coverage including its children.
*/
void combination_report( FILE* ofile, bool verbose ) {

  bool missed_found;  /* If set to TRUE, indicates combinations were missed */
  char tmp[4096];     /* Temporary string value                             */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   COMBINATIONAL LOGIC COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( strcmp( leading_hierarchy, second_hierarchy ) != 0 ) {
      strcpy( tmp, "<NA>" );
    } else {
      strcpy( tmp, leading_hierarchy );
    }

    fprintf( ofile, "Instance                                               Logic Combinations\n" );
    fprintf( ofile, "                                                  Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = combination_instance_summary( ofile, instance_root, tmp );
    
    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      combination_instance_verbose( ofile, instance_root, tmp );
    }

  } else {

    fprintf( ofile, "Module                    Filename                     Logical Combinations\n" );
    fprintf( ofile, "                                                  Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = combination_module_summary( ofile, mod_head );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      combination_module_verbose( ofile, mod_head );
    }

  }

  fprintf( ofile, "\n\n" );

}


/*
 $Log$
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

