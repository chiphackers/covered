/*!
 \file     line.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#include <stdio.h>
#include <assert.h>

#include "line.h"
#include "defines.h"
#include "link.h"
#include "instance.h"
#include "codegen.h"

extern mod_inst* instance_root;
extern mod_link* mod_head;

/*!
 \param expl   Pointer to current expression list to explore.
 \param total  Holds total number of lines parsed.
 \param hit    Holds total number of lines hit.

 Iterates through given expression list, gathering information about which
 lines exist in the list, which lines were hit during simulation and which
 lines were missed during simulation.  This information is used to report
 summary information about line coverage.
*/
void line_get_stats( exp_link* expl, float* total, int* hit ) {

  exp_link* curr      = expl;   /* Pointer to current expression link in list      */
  int       last_line = -1;     /* Last line number found                          */
  bool      line_hit  = TRUE;   /* Specifies that the current line was hit already */

  while( curr != NULL ) {

    if( curr->exp->line != last_line ) {
      line_hit = FALSE;
      *total = *total + 1;
      if(    (SUPPL_WAS_EXECUTED( curr->exp->suppl ) == 1) 
          || (   (curr->exp->op == EXP_OP_NONE) 
              && (   (curr->next == NULL) 
                  || (curr->next->exp->line != curr->exp->line))) ) {
        (*hit)++;
        line_hit = TRUE;
      }
    } else {
      if( (SUPPL_WAS_EXECUTED( curr->exp->suppl ) == 1) && !line_hit ) {
        (*hit)++;
        line_hit = TRUE;
      }
    }
        
    last_line = curr->exp->line;
    curr      = curr->next;

  }

}

/*!
 \param ofile        Pointer to file to output results to.
 \param root         Current node in instance tree.
 \param parent_inst  Name of parent instance.
 
 Recursively iterates through the instance tree gathering the
 total number of lines parsed vs. the total number of lines
 executed during the course of simulation.  The parent node will
 display its information before calling its children.
*/
void line_instance_summary( FILE* ofile, mod_inst* root, char* parent_inst ) {

  mod_inst* curr;       /* Pointer to current child module instance of this node */
  float     percent;    /* Percentage of lines hit                               */
  float     miss;       /* Number of lines missed                                */

  assert( root != NULL );
  assert( root->stat != NULL );

  percent = ((root->stat->line_hit / root->stat->line_total) * 100);
  miss    = (root->stat->line_total - root->stat->line_hit);

  fprintf( ofile, "  %-20.20s    %-20.20s    %3d/%3.0f/%3.0f      %3.0f%%\n",
           root->name,
           parent_inst,
           root->stat->line_hit,
           miss,
           root->stat->line_total,
           percent );

  curr = root->child_head;
  while( curr != NULL ) {
    line_instance_summary( ofile, curr, root->name );
    curr = curr->next;
  }
           
}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to head of module list to explore.

 Iterates through the module list, displaying the line coverage results (summary
 format) for each module.
*/
void line_module_summary( FILE* ofile, mod_link* head ) {

  float     total_lines = 0;  /* Total number of lines parsed                         */
  int       hit_lines   = 0;  /* Number of lines executed during simulation           */
  mod_inst* curr;             /* Pointer to current child module instance of this node */
  float     percent;          /* Percentage of lines hit                               */
  float     miss;             /* Number of lines missed                                */

  line_get_stats( head->mod->exp_head, &total_lines, &hit_lines );

  percent = ((hit_lines / total_lines) * 100);
  miss    = (total_lines - hit_lines);

  fprintf( ofile, "  %-20.20s    %-20.20s    %3d/%3.0f/%3.0f      %3.0f%%\n", 
           head->mod->name,
           head->mod->filename,
           hit_lines,
           miss,
           total_lines,
           percent );

  if( head->next != NULL ) {
    line_module_summary( ofile, head->next );
  }

}

/*!
 \param ofile      Pointer to file to output results to.
 \param expl       Pointer to expression list head.

 Displays the lines missed during simulation to standard output from the
 specified expression list.
*/
void line_display_verbose( FILE* ofile, exp_link* expl ) {

  expression* unexec_exp;      /* Pointer to current unexecuted expression    */
  char*       code;            /* Pointer to code string from code generator  */
  int         last_line = -1;  /* Line number of last line found to be missed */

  fprintf( ofile, "Missed Lines\n\n" );

  /* Display current instance missed lines */
  while( expl != NULL ) {

    if(   (SUPPL_WAS_EXECUTED( expl->exp->suppl ) == 0)
       && (expl->exp->op != EXP_OP_NONE)
       && (expl->exp->line != last_line) ) {

      last_line  = expl->exp->line;
      unexec_exp = expl->exp;
      while( (unexec_exp->parent != NULL) && (unexec_exp->parent->line == unexec_exp->line) ) {
        unexec_exp = unexec_exp->parent;
      }

      code = codegen_gen_expr( unexec_exp, unexec_exp->line );
      fprintf( ofile, "%7d:    %s\n", unexec_exp->line, code );
      free_safe( code );

    }

    expl = expl->next;

  }

  fprintf( ofile, "\n" );

}

/*!
 \param ofile  Pointer to file to output results to.
 \param root   Pointer to root node of instance tree to search through.

 Displays the verbose line coverage results to the specified output stream on
 an instance basis.  The verbose line coverage includes the line numbers 
 (and associated verilog code) and file/module name of the lines that were 
 not hit during simulation.
*/
void line_instance_verbose( FILE* ofile, mod_inst* root ) {

  mod_inst*   curr_inst;   /* Pointer to current instance being evaluated */

  assert( root != NULL );

  fprintf( ofile, "\n" );
  fprintf( ofile, "Module: %s, File: %s, Instance: %s\n", 
           root->mod->name, 
           root->mod->filename,
           root->name );
  fprintf( ofile, "--------------------------------------------------------\n" );

  line_display_verbose( ofile, root->mod->exp_head );

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    line_instance_verbose( ofile, curr_inst );
    curr_inst = curr_inst->next;
  }
 
}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to head of module list to search through.

 Displays the verbose line coverage results to the specified output stream on
 a module basis (combining modules that are instantiated multiple times).
 The verbose line coverage includes the line numbers (and associated verilog
 code) and file/module name of the lines that were not hit during simulation.
*/
void line_module_verbose( FILE* ofile, mod_link* head ) {

  assert( head != NULL );

  fprintf( ofile, "\n" );
  fprintf( ofile, "Module: %s, File: %s\n", 
           head->mod->name, 
           head->mod->filename );
  fprintf( ofile, "--------------------------------------------------------\n" );

  line_display_verbose( ofile, head->mod->exp_head );
  
  if( head->next != NULL ) {
    line_module_verbose( ofile, head->next );
  }

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 \param instance  Specifies to report by instance or module.

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the line coverage for each module encountered.  The parent module will
 specify its own line coverage along with a total line coverage including its 
 children.
*/
void line_report( FILE* ofile, bool verbose, bool instance ) {

  if( instance ) {

    fprintf( ofile, "LINE COVERAGE RESULTS BY INSTANCE\n" );
    fprintf( ofile, "---------------------------------\n" );
    fprintf( ofile, "Instance                  Parent                 Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "------------------------------------------------------------------------------\n" );

    line_instance_summary( ofile, instance_root, "<root>" );
    
    if( verbose ) {
      line_instance_verbose( ofile, instance_root );
    }

  } else {

    fprintf( ofile, "LINE COVERAGE RESULTS BY MODULE\n" );
    fprintf( ofile, "-------------------------------\n" );
    fprintf( ofile, "Module                    Filename               Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "------------------------------------------------------------------------------\n" );

    line_module_summary( ofile, mod_head );

    if( verbose ) {
      line_module_verbose( ofile, mod_head );
    }

  }

  fprintf( ofile, "==============================================================================\n" );
  fprintf( ofile, "\n" );

}
