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
  char*       code;            /* Pointer to code string from code generator  */
  int         last_line = -1;  /* Line number of last line found to be missed */

  fprintf( ofile, "Missed Combinations\n" );

  /* Display current instance missed lines */
  while( expl != NULL ) {

    if(   (SUPPL_WAS_TRUE( expl->exp->suppl ) == 0)
       || (SUPPL_WAS_FALSE( expl->exp->suppl ) == 0) ) {

      unexec_exp = expl->exp;
      while( unexec_exp->parent != NULL ) {
        unexec_exp = unexec_exp->parent;
      }

      code = codegen_gen_expr( unexec_exp, -1 );

/* NEED TO ADD COMBINATIONAL LOGIC OUTPUT HERE */

      free_safe( code );

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

}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to current module to evaluate.

 Outputs the verbose coverage report for the specified module
 to the specified output stream.
*/
void combination_module_verbose( FILE* ofile, mod_link* head ) {

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
