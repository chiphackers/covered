/*!
 \file     comb.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "comb.h"
#include "codegen.h"
#include "util.h"


extern mod_inst* instance_root;
extern mod_link* mod_head;


/*!
 \param expl   Pointer to current expression link to evaluate.
 \param total  Pointer to total number of logical combinations.
 \param hit    Pointer to number of logical combinations hit during simulation.

 Recursively traverses the specified expression list, recording the total number
 of logical combinations in the expression list and the number of combinations
 hit during the course of simulation.  An expression can be considered for
 combinational coverage if the "measured" bit is set in the expression.
*/
void combination_get_stats( exp_link* expl, float* total, int* hit ) {

  exp_link* curr_exp;    /* Pointer to the current expression link in the list */

  curr_exp = expl;

  while( curr_exp != NULL ) {
    if( SUPPL_IS_MEASURABLE( curr_exp->exp->suppl ) == 1 ) {
      *total = *total + 2;
      *hit   = *hit + SUPPL_WAS_TRUE( curr_exp->exp->suppl ) + SUPPL_WAS_FALSE( curr_exp->exp->suppl );
    }
    curr_exp = curr_exp->next;
  }

}

/*!
 \param ofile   Pointer to file to output results to.
 \param root    Pointer to node in instance tree to evaluate.
 \param parent  Name of parent instance name.

 Outputs summarized results of the combinational logic coverage per module
 instance to the specified output stream.  Summarized results are printed 
 as percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
void combination_instance_summary( FILE* ofile, mod_inst* root, char* parent ) {

  mod_inst* curr;       /* Pointer to current child module instance of this node */
  float     percent;    /* Percentage of lines hit                               */
  float     miss;       /* Number of lines missed                                */

  assert( root != NULL );
  assert( root->stat != NULL );

  percent = ((root->stat->comb_hit / root->stat->comb_total) * 100);
  miss    = (root->stat->comb_total - root->stat->comb_hit);

  fprintf( ofile, "  %-20.20s    %-20.20s    %3d/%3.0f/%3.0f      %3.0f\%\n",
           root->name,
           parent,
           root->stat->comb_hit,
           miss,
           root->stat->comb_total,
           percent );

  curr = root->child_head;
  while( curr != NULL ) {
    combination_instance_summary( ofile, curr, root->name );
    curr = curr->next;
  }

}

/*!
 \param ofile   Pointer to file to output results to.
 \param head    Pointer to link in current module list to evaluate.

 Outputs summarized results of the combinational logic coverage per module
 to the specified output stream.  Summarized results are printed as 
 percentages based on the number of combinations hit during simulation 
 divided by the total number of expression combinations possible in the 
 design.  An expression is said to be measurable for combinational coverage 
 if it evaluates to a value of 0 or 1.
*/
void combination_module_summary( FILE* ofile, mod_link* head ) {

  float     total_lines = 0;  /* Total number of lines parsed                         */
  int       hit_lines   = 0;  /* Number of lines executed during simulation           */
  mod_inst* curr;             /* Pointer to current child module instance of this node */
  float     percent;          /* Percentage of lines hit                               */
  float     miss;             /* Number of lines missed                                */

  combination_get_stats( head->mod->exp_head, &total_lines, &hit_lines );

  percent = ((hit_lines / total_lines) * 100);
  miss    = (total_lines - hit_lines);

  fprintf( ofile, "  %-20.20s    %-20.20s    %3d/%3.0f/%3.0f      %3.0f\%\n", 
           head->mod->name,
           head->mod->filename,
           hit_lines,
           miss,
           total_lines,
           percent );

  if( head->next != NULL ) {
    combination_module_summary( ofile, head->next );
  }

}

/*!
 \param line  Pointer to line to create line onto.
 \param size  Number of characters long line is.
 \param exp_id  ID to place in underline.

 Draws an underline containing the specified expression ID to the specified
 line.
*/
void combination_draw_line( char* line, int size, int exp_id ) {

  char str_exp_id[12];   /* String containing value of exp_id        */
  int  exp_id_size;      /* Number of characters exp_id is in length */
  int  i;                /* Loop iterator                            */

  /* Calculate size of expression ID */
  snprintf( str_exp_id, 12, "%d", exp_id );
  exp_id_size = strlen( str_exp_id );

  line[0] = '|';

  for( i=1; i<((size - exp_id_size) / 2); i++ ) {
    line[i] = '-';
  }

  line[i] = '\0';
  strcat( line, str_exp_id );

  for( i=(i + exp_id_size); i<(size - 1); i++ ) {
    line[i] = '-';
  }

  line[i]   = '|';
  line[i+1] = '\0';

}

/*!
 \param exp     Pointer to expression to create underline for.
 \param lines   Stack of lines for left child.
 \param depth   Pointer to top of left child stack.
 \param size    Pointer to character width of this node.
 \param exp_id  Pointer to current expression ID to use in labeling.

 Recursively parses specified expression tree, underlining and labeling each
 measurable expression.
*/
void combination_underline_tree( expression* exp, char*** lines, int* depth, int* size, int* exp_id ) {

  char** l_lines;       /* Pointer to left underline stack                          */
  char** r_lines;       /* Pointer to right underline stack                         */
  int    l_depth;       /* Index to top of left stack                               */
  int    r_depth;       /* Index to top of right stack                              */
  int    l_size;        /* Number of characters for left expression                 */
  int    r_size;        /* Number of characters for right expression                */
  int    i;             /* Loop iterator                                            */
  char   exp_sp[256];   /* Space to take place of missing expression(s)             */
  char   code_fmt[12];  /* Contains format string for rest of stack                 */
  int    uline_this;    /* Specifies if the current expression should be underlined */
  
  *depth  = 0;
  *size   = 0;
  l_lines = NULL;
  r_lines = NULL;

  if( exp != NULL ) {

    if( exp->op == EXP_OP_NONE ) {
      
      *size = exp->value->width + 12;

    } else if( exp->op == EXP_OP_SIG ) {

      *size = strlen( exp->sig->name );

    } else {

      combination_underline_tree( exp->left,  &l_lines, &l_depth, &l_size, exp_id );
      combination_underline_tree( exp->right, &r_lines, &r_depth, &r_size, exp_id );

      switch( exp->op ) {
        case EXP_OP_XOR      :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_MULTIPLY :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_DIVIDE   :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_MOD      :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_ADD      :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_SUBTRACT :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_AND      :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_OR       :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_NAND     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_NOR      :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_NXOR     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_LT       :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_GT       :  *size = l_size + r_size + 5;  strcpy( code_fmt, " %s   %s "   );  break;
        case EXP_OP_LSHIFT   :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_RSHIFT   :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_EQ       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_CEQ      :  *size = l_size + r_size + 7;  strcpy( code_fmt, " %s     %s " );  break;
        case EXP_OP_LE       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_GE       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_NE       :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_CNE      :  *size = l_size + r_size + 7;  strcpy( code_fmt, " %s     %s " );  break;
        case EXP_OP_LOR      :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_LAND     :  *size = l_size + r_size + 6;  strcpy( code_fmt, " %s    %s "  );  break;
        case EXP_OP_COND_T   :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s   %s"     );  break;
        case EXP_OP_COND_F   :  *size = l_size + r_size + 0;  strcpy( code_fmt, "%s"          );  break;
        case EXP_OP_UINV     :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"         );  break;
        case EXP_OP_UAND     :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"         );  break;
        case EXP_OP_UNOT     :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"         );  break;
        case EXP_OP_UOR      :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"         );  break;
        case EXP_OP_UXOR     :  *size = l_size + r_size + 1;  strcpy( code_fmt, " %s"         );  break;
        case EXP_OP_UNAND    :  *size = l_size + r_size + 2;  strcpy( code_fmt, "  %s"        );  break;
        case EXP_OP_UNOR     :  *size = l_size + r_size + 2;  strcpy( code_fmt, "  %s"        );  break;
        case EXP_OP_UNXOR    :  *size = l_size + r_size + 2;  strcpy( code_fmt, "  %s"        );  break;
        case EXP_OP_SBIT_SEL :  *size = l_size + r_size + 2;  strcpy( code_fmt, "%s"          );  break;
        case EXP_OP_MBIT_SEL :  *size = l_size + r_size + 3;  strcpy( code_fmt, "%s"          );  break;
        case EXP_OP_EXPAND   :  *size = l_size + r_size + 0;  strcpy( code_fmt, "%s"          );  break;  // ???
        case EXP_OP_CONCAT   :  *size = l_size + r_size + 0;  strcpy( code_fmt, "%s"          );  break;  // ???
        case EXP_OP_PEDGE    :  *size = l_size + r_size + 0;  strcpy( code_fmt, "%s"          );  break;  // ???
        case EXP_OP_NEDGE    :  *size = l_size + r_size + 0;  strcpy( code_fmt, "%s"          );  break;  // ???
        case EXP_OP_AEDGE    :  *size = l_size + r_size + 0;  strcpy( code_fmt, "%s"          );  break;  // ???
        default              :  
          print_output( "Internal error:  Unknown expression type in combination_underline_tree", FATAL );
          exit( 1 );
          break;
      }

      uline_this = (  SUPPL_IS_MEASURABLE( exp->suppl ) 
                    & (  ~SUPPL_WAS_TRUE( exp->suppl ) 
                       | ~SUPPL_WAS_FALSE( exp->suppl )));

      if( l_depth > r_depth ) {
        *depth = l_depth + uline_this;
      } else {
        *depth = r_depth + uline_this;
      }

      if( *depth > 0 ) {

        /* Allocate all memory for the stack */
        *lines = (char**)malloc_safe( sizeof( char* ) * (*depth) );

        /* Allocate memory for this underline */
        (*lines)[(*depth)-1] = (char*)malloc_safe( *size + 1 );

        /* Create underline or space */
        if( uline_this == 1 ) {
          combination_draw_line( (*lines)[(*depth)-1], *size, *exp_id );
          *exp_id = *exp_id + 1;
        }

        /* Combine the left and right line stacks */
        for( i=0; i<(*depth - uline_this); i++ ) {

          (*lines)[i] = (char*)malloc_safe( *size + 1 );

          if( (i < l_depth) && (i < r_depth) ) {
         
            /* Merge left and right lines */
            snprintf( (*lines)[i], *size, code_fmt, l_lines[i], r_lines[i] );

            free_safe( l_lines[i] );
            free_safe( r_lines[i] );

          } else if( i < l_depth ) {

            /* Create spaces for right side */
            gen_space( exp_sp, r_size );

            /* Merge left side only */
            snprintf( (*lines)[i], *size, code_fmt, l_lines[i], exp_sp );

            free_safe( l_lines[i] );

          } else if( i < r_depth ) {
 
            /* Create spaces for left side */
            gen_space( exp_sp, l_size );

            /* Merge left side only */
            snprintf( (*lines)[i], *size, code_fmt, exp_sp, r_lines[i] );

            free_safe( r_lines[i] );
   
          } else {

            print_output( "Internal error:  Reached entry without a left or right underline", FATAL );
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
 \param ofile     Pointer output stream to display underlines to.
 \param exp       Pointer to parent expression to create underline for.
 \param begin_sp  Spacing that is placed at the beginning of the generated line.

 Traverses through the expression tree that is on the same line as the parent,
 creating underline strings.  An underline is created for each expression that
 does not have complete combination logic coverage.  Each underline (children to
 parent creates an inverted tree) and contains a number for the specified expression.
*/
void combination_underline( FILE* ofile, expression* exp, char* begin_sp ) {

  char** lines;    /* Pointer to a stack of lines     */
  int    depth;    /* Depth of underline stack        */
  int    size;     /* Width of stack in characters    */
  int    exp_id;   /* Expression ID to place in label */
  int    i;        /* Loop iterator                   */

  exp_id = 1;

  combination_underline_tree( exp, &lines, &depth, &size, &exp_id );

  for( i=0; i<depth; i++ ) {
    fprintf( ofile, "%s%s\n", begin_sp, lines[i] );
    free_safe( lines[i] );
  }

  if( depth > 0 ) {
    free_safe( lines );
  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param expl   Pointer to expression list head.

 Displays the expressions (and groups of expressions) that were considered 
 to be measurable (evaluates to a value of TRUE (1) or FALSE (0) but were 
 not hit during simulation.  The entire Verilog expression is displayed
 to the specified output stream with each of its measured expressions being
 underlined and numbered.  The missed combinations are then output below
 the Verilog code, showing those logical combinations that were not hit
 during simulation.
*/
void combination_display_verbose( FILE* ofile, exp_link* expl ) {

  expression* unexec_exp;      /* Pointer to current unexecuted expression    */
  char*       code;            /* Code string from code generator             */
  char*       underline;       /* Underline string for specified code         */
  int         last_line = -1;  /* Line number of last line found to be missed */

  fprintf( ofile, "Missed Combinations\n" );

  /* Display current instance missed lines */
  while( expl != NULL ) {

    if( ((SUPPL_WAS_TRUE( expl->exp->suppl )  == 0) ||
         (SUPPL_WAS_FALSE( expl->exp->suppl ) == 0)) &&
        (SUPPL_IS_MEASURABLE( expl->exp->suppl ) == 1) &&
        (expl->exp->line != last_line) ) {

      last_line  = expl->exp->line;
      unexec_exp = expl->exp;

      while( (unexec_exp->parent != NULL) && (unexec_exp->parent->line == unexec_exp->line) ) {
        unexec_exp = unexec_exp->parent;
      }

      code = codegen_gen_expr( unexec_exp, unexec_exp->line );

      fprintf( ofile, "%7d:    %s\n", unexec_exp->line, code );

      combination_underline( ofile, unexec_exp, "            " );

      fprintf( ofile, "\n" );

      free_safe( code );
      free_safe( underline );

    }

    expl = expl->next;

  }

  fprintf( ofile, "\n" );

}

/*!
 \param ofile  Pointer to file to output results to.
 \param root   Pointer to current module instance to evaluate.

 Outputs the verbose coverage report for the specified module instance
 to the specified output stream.
*/
void combination_instance_verbose( FILE* ofile, mod_inst* root ) {

  mod_inst*   curr_inst;   /* Pointer to current instance being evaluated */

  assert( root != NULL );

  fprintf( ofile, "\n" );
  fprintf( ofile, "Module: %s, File: %s, Instance: %s\n", 
           root->mod->name, 
           root->mod->filename,
           root->name );
  fprintf( ofile, "--------------------------------------------------------\n" );

  combination_display_verbose( ofile, root->mod->exp_head );

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    combination_instance_verbose( ofile, curr_inst );
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

  assert( head != NULL );

  fprintf( ofile, "\n" );
  fprintf( ofile, "Module: %s, File: %s\n", 
           head->mod->name, 
           head->mod->filename );
  fprintf( ofile, "--------------------------------------------------------\n" );

  combination_display_verbose( ofile, head->mod->exp_head );
  
  if( head->next != NULL ) {
    combination_module_verbose( ofile, head->next );
  }

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 \param instance  Specifies to report by instance or module.

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the combinational logic coverage for each module encountered.  The parent 
 module will specify its own combinational logic coverage along with a total combinational
 logic coverage including its children.
*/
void combination_report( FILE* ofile, bool verbose, bool instance ) {

  if( instance ) {

    fprintf( ofile, "COMBINATIONAL LOGIC COVERAGE RESULTS BY INSTANCE\n" );
    fprintf( ofile, "------------------------------------------------\n" );
    fprintf( ofile, "Instance                  Parent                       Logic Combinations\n" );
    fprintf( ofile, "                                                 Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "------------------------------------------------------------------------------\n" );

    combination_instance_summary( ofile, instance_root, "<root>" );
    
    if( verbose ) {
      combination_instance_verbose( ofile, instance_root );
    }

  } else {

    fprintf( ofile, "COMBINATIONAL LOGIC COVERAGE RESULTS BY MODULE\n" );
    fprintf( ofile, "----------------------------------------------\n" );
    fprintf( ofile, "Module                    Filename                     Logical Combinations\n" );
    fprintf( ofile, "                                                 Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "------------------------------------------------------------------------------\n" );

    combination_module_summary( ofile, mod_head );

    if( verbose ) {
      combination_module_verbose( ofile, mod_head );
    }

  }

}
