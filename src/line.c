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
 \param stmtl  Pointer to current statement list to explore.
 \param total  Holds total number of lines parsed.
 \param hit    Holds total number of lines hit.

 Iterates through given statement list, gathering information about which
 lines exist in the list, which lines were hit during simulation and which
 lines were missed during simulation.  This information is used to report
 summary information about line coverage.
*/
void line_get_stats( stmt_link* stmtl, float* total, int* hit ) {

  stmt_link* curr      = stmtl;  /* Pointer to current statement link in list       */
  int        last_line = -1;     /* Last line number found                          */

  while( curr != NULL ) {

    if( (SUPPL_OP( curr->stmt->exp->suppl ) != EXP_OP_DELAY) &&
        (SUPPL_OP( curr->stmt->exp->suppl ) != EXP_OP_CASE)  &&
        (SUPPL_OP( curr->stmt->exp->suppl ) != EXP_OP_CASEX) &&
        (SUPPL_OP( curr->stmt->exp->suppl ) != EXP_OP_CASEZ) &&
        (SUPPL_OP( curr->stmt->exp->suppl ) != EXP_OP_DEFAULT) ) {
      *total = *total + 1;
      if( SUPPL_WAS_EXECUTED( curr->stmt->exp->suppl ) == 1 ) {
        (*hit)++;
      }
    }
        
    curr      = curr->next;

  }

}

/*!
 \param ofile        Pointer to file to output results to.
 \param root         Current node in instance tree.
 \param parent_inst  Name of parent instance.

 \return Returns TRUE if lines were found to be missed; otherwise, returns FALSE.
 
 Recursively iterates through the instance tree gathering the
 total number of lines parsed vs. the total number of lines
 executed during the course of simulation.  The parent node will
 display its information before calling its children.
*/
bool line_instance_summary( FILE* ofile, mod_inst* root, char* parent_inst ) {

  mod_inst* curr;       /* Pointer to current child module instance of this node */
  float     percent;    /* Percentage of lines hit                               */
  float     miss;       /* Number of lines missed                                */

  assert( root != NULL );
  assert( root->stat != NULL );

  if( root->stat->line_total == 0 ) {
    percent = 100.0;
  } else {
    percent = ((root->stat->line_hit / root->stat->line_total) * 100);
  }
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

  return( miss > 0 );
           
}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to head of module list to explore.

 \return Returns TRUE if there where lines that were found to be missed; otherwise,
         returns FALSE.

 Iterates through the module list, displaying the line coverage results (summary
 format) for each module.
*/
bool line_module_summary( FILE* ofile, mod_link* head ) {

  float     total_lines = 0;  /* Total number of lines parsed                         */
  int       hit_lines   = 0;  /* Number of lines executed during simulation           */
  mod_inst* curr;             /* Pointer to current child module instance of this node */
  float     percent;          /* Percentage of lines hit                               */
  float     miss;             /* Number of lines missed                                */

  line_get_stats( head->mod->stmt_head, &total_lines, &hit_lines );

  if( total_lines == 0 ) {
    percent = 100.0;
  } else {
    percent = ((hit_lines / total_lines) * 100);
  }
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

  return( miss > 0 );

}

/*!
 \param ofile  Pointer to file to output results to.
 \param stmtl  Pointer to expression list head.

 Displays the lines missed during simulation to standard output from the
 specified expression list.
*/
void line_display_verbose( FILE* ofile, stmt_link* stmtl ) {

  expression* unexec_exp;      /* Pointer to current unexecuted expression    */
  char*       code;            /* Pointer to code string from code generator  */

  fprintf( ofile, "Missed Lines\n\n" );

  /* Display current instance missed lines */
  while( stmtl != NULL ) {

    if( (SUPPL_WAS_EXECUTED( stmtl->stmt->exp->suppl ) == 0)  &&
        (SUPPL_OP( stmtl->stmt->exp->suppl ) != EXP_OP_DELAY) &&
        (SUPPL_OP( stmtl->stmt->exp->suppl ) != EXP_OP_CASE)  &&
        (SUPPL_OP( stmtl->stmt->exp->suppl ) != EXP_OP_CASEX) &&
        (SUPPL_OP( stmtl->stmt->exp->suppl ) != EXP_OP_CASEZ) &&
        (SUPPL_OP( stmtl->stmt->exp->suppl ) != EXP_OP_DEFAULT) ) {

      unexec_exp = stmtl->stmt->exp;
/*
      while( (unexec_exp->parent->expr != NULL) && (unexec_exp->parent->expr->line == unexec_exp->line) ) {
        unexec_exp = unexec_exp->parent->expr;
      }
*/

      code = codegen_gen_expr( unexec_exp, unexec_exp->line );
      fprintf( ofile, "%7d:    %s\n", unexec_exp->line, code );
      free_safe( code );

    }

    stmtl = stmtl->next;

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

  line_display_verbose( ofile, root->mod->stmt_head );

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

  line_display_verbose( ofile, head->mod->stmt_head );
  
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

  bool missed_found;      /* If set to TRUE, lines were found to be missed */

  if( instance ) {

    fprintf( ofile, "LINE COVERAGE RESULTS BY INSTANCE\n" );
    fprintf( ofile, "---------------------------------\n" );
    fprintf( ofile, "Instance                  Parent                 Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "------------------------------------------------------------------------------\n" );

    missed_found = line_instance_summary( ofile, instance_root, "<root>" );
    
    if( verbose && missed_found ) {
      line_instance_verbose( ofile, instance_root );
    }

  } else {

    fprintf( ofile, "LINE COVERAGE RESULTS BY MODULE\n" );
    fprintf( ofile, "-------------------------------\n" );
    fprintf( ofile, "Module                    Filename               Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "------------------------------------------------------------------------------\n" );

    missed_found = line_module_summary( ofile, mod_head );

    if( verbose && missed_found ) {
      line_module_verbose( ofile, mod_head );
    }

  }

  fprintf( ofile, "==============================================================================\n" );
  fprintf( ofile, "\n" );

}

/* $Log$
/* Revision 1.13  2002/07/09 03:24:48  phase1geo
/* Various fixes for module instantiantion handling.  This now works.  Also
/* modified report output for toggle, line and combinational information.
/* Regression passes.
/*
/* Revision 1.12  2002/07/05 05:00:14  phase1geo
/* Removing CASE, CASEX, and CASEZ from line and combinational logic results.
/*
/* Revision 1.11  2002/07/02 19:52:50  phase1geo
/* Removing unecessary diagnostics.  Cleaning up extraneous output and
/* generating new documentation from source.  Regression passes at the
/* current time.
/*
/* Revision 1.10  2002/06/27 20:39:43  phase1geo
/* Fixing scoring bugs as well as report bugs.  Things are starting to work
/* fairly well now.  Added rest of support for delays.
/*
/* Revision 1.9  2002/06/25 21:46:10  phase1geo
/* Fixes to simulator and reporting.  Still some bugs here.
/*
/* Revision 1.8  2002/06/22 05:27:30  phase1geo
/* Additional supporting code for simulation engine and statement support in
/* parser.
/*
/* Revision 1.7  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/* */

