/*!
 \file     line.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "line.h"
#include "defines.h"
#include "link.h"
#include "instance.h"
#include "codegen.h"
#include "iter.h"
#include "util.h"
#include "expr.h"

extern mod_inst* instance_root;
extern mod_link* mod_head;

extern bool         report_covered;
extern unsigned int report_comb_depth;
extern bool         report_instance;
extern char         leading_hierarchy[4096];

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

  stmt_iter curr;  /* Statement list iterator */

  stmt_iter_reset( &curr, stmtl );
  
  while( curr.curr != NULL ) {

    if( (SUPPL_OP( curr.curr->stmt->exp->suppl ) != EXP_OP_DELAY)   &&
        (SUPPL_OP( curr.curr->stmt->exp->suppl ) != EXP_OP_CASE)    &&
        (SUPPL_OP( curr.curr->stmt->exp->suppl ) != EXP_OP_CASEX)   &&
        (SUPPL_OP( curr.curr->stmt->exp->suppl ) != EXP_OP_CASEZ)   &&
        (SUPPL_OP( curr.curr->stmt->exp->suppl ) != EXP_OP_DEFAULT) &&
        (curr.curr->stmt->exp->line != 0) ) {
      *total = *total + 1;
      if( SUPPL_WAS_EXECUTED( curr.curr->stmt->exp->suppl ) == 1 ) {
        (*hit)++;
      }
    }
        
    stmt_iter_next( &curr );

  }

}

/*!
 \param mod_name  Name of module to get missed line number array from.
 \param lines     Pointer to array of integers that will contain the missed lines.
 \param line_cnt  Pointer to size of lines array.

 \return Returns TRUE if module specified was found; otherwise, returns FALSE.

 Searches design for specified module name.  If the module name is found, the lines
 array and line_cnt values are initialized and filled with the line numbers that were
 not hit during simulation and a value of TRUE is returned.  If the module name was
 not found, a value of FALSE is returned.
*/
bool line_collect_uncovered( char* mod_name, int** lines, int* line_cnt ) {

  bool      retval = TRUE;  /* Return value for this function                     */
  stmt_iter stmti;          /* Statement list iterator                            */
  module    mod;            /* Module used for searching                          */
  mod_link* modl;           /* Pointer to found module link                       */
  int       i;              /* Loop iterator                                      */
  int       last_line;      /* Specifies the last line of the current expression  */
  int       line_size;      /* Indicates the number of entries in the lines array */

  /* First, find module in module array */
  mod.name = mod_name;
  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {

    /* Create an array that will hold the number of uncovered lines */
    line_size = 20;
    *line_cnt = 0;
    *lines    = (int*)malloc_safe( sizeof( int ) * line_size );

    stmt_iter_reset( &stmti, modl->mod->stmt_tail );
    stmt_iter_find_head( &stmti, FALSE );

    while( stmti.curr != NULL ) {

      if( (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_DELAY)   &&
          (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_CASE)    &&
          (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_CASEX)   &&
          (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_CASEZ)   &&
          (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_DEFAULT) &&
          (stmti.curr->stmt->exp->line != 0) ) {

        if( !SUPPL_WAS_EXECUTED( stmti.curr->stmt->exp->suppl ) ) {

          last_line = expression_get_last_line( stmti.curr->stmt->exp );
          for( i=stmti.curr->stmt->exp->line; i<=last_line; i++ ) {
            if( *line_cnt == line_size ) {
              line_size += 20;
              *lines = (int*)realloc( *lines, (sizeof( int ) * line_size) );
            }
            (*lines)[(*line_cnt)] = i;
            (*line_cnt)++;
          }

        }

      }

      stmt_iter_get_next_in_order( &stmti );

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

bool line_get_module_summary( char* mod_name, int* total, int* hit ) {

  bool      retval = TRUE;  /* Return value for this function */
  module    mod;            /* Module used for searching      */
  mod_link* modl;           /* Pointer to found module link   */
  char      tmp[4];         /* Temporary string for total     */

  mod.name = mod_name;

  if( (modl = mod_link_find( &mod, mod_head )) != NULL ) {

    snprintf( tmp, 4, "%3.0f", modl->mod->stat->line_total );
    assert( sscanf( tmp, "%d", total ) == 1 );
    *hit = modl->mod->stat->line_hit;

  } else {

    retval = FALSE;

  }

  return( retval );

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

  mod_inst* curr;           /* Pointer to current child module instance of this node */
  float     percent;        /* Percentage of lines hit                               */
  float     miss;           /* Number of lines missed                                */
  char      tmpname[4096];  /* Temporary holder of instance name                     */

  assert( root != NULL );
  assert( root->stat != NULL );

  if( root->stat->line_total == 0 ) {
    percent = 100.0;
  } else {
    percent = ((root->stat->line_hit / root->stat->line_total) * 100);
  }
  miss    = (root->stat->line_total - root->stat->line_hit);

  /* Calculate instance name */
  if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, root->name );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, root->name );
  }

  fprintf( ofile, "  %-43.43s    %5d/%5.0f/%5.0f      %3.0f%%\n",
           tmpname,
           root->stat->line_hit,
           miss,
           root->stat->line_total,
           percent );

  curr = root->child_head;
  while( curr != NULL ) {
    miss = miss + line_instance_summary( ofile, curr, tmpname );
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

  float percent;             /* Percentage of lines hit                    */
  float miss;                /* Number of lines missed                     */
  bool  miss_found = FALSE;  /* Set to TRUE if line was found to be missed */

  while( head != NULL ) {

    if( head->mod->stat->line_total == 0 ) {
      percent = 100.0;
    } else {
      percent = ((head->mod->stat->line_hit / head->mod->stat->line_total) * 100);
    }

    miss       = (head->mod->stat->line_total - head->mod->stat->line_hit);
    miss_found = (miss > 0) ? TRUE : miss_found;

    fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5.0f/%5.0f      %3.0f%%\n", 
             head->mod->name,
             get_basename( head->mod->filename ),
             head->mod->stat->line_hit,
             miss,
             head->mod->stat->line_total,
             percent );

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile  Pointer to file to output results to.
 \param stmtl  Pointer to expression list head.

 Displays the lines missed during simulation to standard output from the
 specified expression list.
*/
void line_display_verbose( FILE* ofile, stmt_link* stmtl ) {

  stmt_iter   stmti;       /* Statement list iterator                    */
  expression* unexec_exp;  /* Pointer to current unexecuted expression   */
  char*       code;        /* Pointer to code string from code generator */

  if( report_covered ) {
    fprintf( ofile, "Hit Lines\n\n" );
  } else {
    fprintf( ofile, "Missed Lines\n\n" );
  }

  /* Display current instance missed lines */
  stmt_iter_reset( &stmti, stmtl );
  stmt_iter_find_head( &stmti, FALSE );
  
  while( stmti.curr != NULL ) {

    if( (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_DELAY)   &&
        (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_CASE)    &&
        (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_CASEX)   &&
        (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_CASEZ)   &&
        (SUPPL_OP( stmti.curr->stmt->exp->suppl ) != EXP_OP_DEFAULT) &&
        (stmti.curr->stmt->exp->line != 0) ) {

      if( SUPPL_WAS_EXECUTED( stmti.curr->stmt->exp->suppl ) == report_covered ) {

        unexec_exp = stmti.curr->stmt->exp;

        code = codegen_gen_expr( unexec_exp, unexec_exp->line, SUPPL_OP( unexec_exp->suppl ) );
        fprintf( ofile, "%7d:    %s\n", unexec_exp->line, code );
        free_safe( code );

      }

    }

    stmt_iter_get_next_in_order( &stmti );

  }

  fprintf( ofile, "\n" );

}

/*!
 \param ofile        Pointer to file to output results to.
 \param root         Pointer to root node of instance tree to search through.
 \param parent_inst  Hierarchical path of parent instance.

 Displays the verbose line coverage results to the specified output stream on
 an instance basis.  The verbose line coverage includes the line numbers 
 (and associated verilog code) and file/module name of the lines that were 
 not hit during simulation.
*/
void line_instance_verbose( FILE* ofile, mod_inst* root, char* parent_inst ) {

  mod_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char      tmpname[4096];  /* Temporary name holder for instance          */

  assert( root != NULL );

  if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, root->name );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, root->name );
  }

  fprintf( ofile, "\n" );
  fprintf( ofile, "Module: %s, File: %s, Instance: %s\n", 
           root->mod->name, 
           root->mod->filename,
           tmpname );
  fprintf( ofile, "--------------------------------------------------------\n" );

  line_display_verbose( ofile, root->mod->stmt_tail );

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    line_instance_verbose( ofile, curr_inst, tmpname );
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

  while( head != NULL ) {

    if( ((head->mod->stat->line_hit < head->mod->stat->line_total) && !report_covered) ||
        ((head->mod->stat->line_hit > 0) && report_covered) ) {

      fprintf( ofile, "\n" );
      fprintf( ofile, "Module: %s, File: %s\n", 
               head->mod->name, 
               head->mod->filename );
      fprintf( ofile, "--------------------------------------------------------\n" );

      line_display_verbose( ofile, head->mod->stmt_tail );
  
    }

    head = head->next;
 
  }

}

/*!
 \param ofile    Pointer to file to output results to.
 \param verbose  Specifies whether to generate summary or verbose output.

 After the design is read into the module hierarchy, parses the hierarchy by module,
 reporting the line coverage for each module encountered.  The parent module will
 specify its own line coverage along with a total line coverage including its 
 children.
*/
void line_report( FILE* ofile, bool verbose ) {

  bool missed_found;      /* If set to TRUE, lines were found to be missed */

  if( report_instance ) {

    fprintf( ofile, "LINE COVERAGE RESULTS BY INSTANCE\n" );
    fprintf( ofile, "---------------------------------\n" );
    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------\n" );

    missed_found = line_instance_summary( ofile, instance_root, leading_hierarchy );
    
    if( verbose && (missed_found || report_covered) ) {
      line_instance_verbose( ofile, instance_root, leading_hierarchy );
    }

  } else {

    fprintf( ofile, "LINE COVERAGE RESULTS BY MODULE\n" );
    fprintf( ofile, "-------------------------------\n" );
    fprintf( ofile, "Module                    Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------\n" );

    missed_found = line_module_summary( ofile, mod_head );

    if( verbose && (missed_found || report_covered) ) {
      line_module_verbose( ofile, mod_head );
    }

  }

  fprintf( ofile, "=================================================================================\n" );
  fprintf( ofile, "\n" );

}

/*
 $Log$
 Revision 1.35  2003/12/01 23:27:16  phase1geo
 Adding code for retrieving line summary module coverage information for
 GUI.

 Revision 1.34  2003/11/30 21:50:45  phase1geo
 Modifying line_collect_uncovered function to create array containing all physical
 lines (rather than just uncovered statement starting line values) for more
 accurate line coverage results for the GUI.  Added new long_exp2 diagnostic that
 is used to test this functionality.

 Revision 1.33  2003/11/22 20:44:58  phase1geo
 Adding function to get array of missed line numbers for GUI purposes.  Updates
 to report command for getting information ready when running the GUI.

 Revision 1.32  2003/10/13 03:56:29  phase1geo
 Fixing some problems with new FSM code.  Not quite there yet.

 Revision 1.31  2003/10/03 12:31:04  phase1geo
 More report tweaking.

 Revision 1.30  2003/10/03 03:08:44  phase1geo
 Modifying filename in summary output to only specify basename of file instead
 of entire path.  The verbose report contains the full pathname still, however.

 Revision 1.29  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.28  2003/02/23 23:32:36  phase1geo
 Updates to provide better cross-platform compiler support.

 Revision 1.27  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.26  2002/12/07 17:46:53  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.25  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.24  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.23  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.22  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.21  2002/10/24 23:19:39  phase1geo
 Making some fixes to report output.  Fixing bugs.  Added long_exp1.v diagnostic
 to regression suite which finds a current bug in the report underlining
 functionality.  Need to look into this.

 Revision 1.20  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.19  2002/09/10 05:40:09  phase1geo
 Adding support for MULTIPLY, DIVIDE and MOD in expression verbose display.
 Fixing cases where -c option was not generating covered information in
 line and combination report output.  Updates to assign1.v diagnostic for
 logic that is now supported by both Covered and IVerilog.  Updated assign1.cdd
 to account for correct coverage file for the updated assign1.v diagnostic.

 Revision 1.18  2002/08/20 04:48:18  phase1geo
 Adding option to report command that allows the user to display logic that is
 being covered (-c option).  This overrides the default behavior of displaying
 uncovered logic.  This is useful for debugging purposes and understanding what
 logic the tool is capable of handling.

 Revision 1.17  2002/08/19 04:59:49  phase1geo
 Adjusting summary format to allow for larger line, toggle and combination
 counts.

 Revision 1.16  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.15  2002/07/14 05:27:34  phase1geo
 Fixing report outputting to allow multiple modules/instances to be
 output.

 Revision 1.14  2002/07/10 13:15:57  phase1geo
 Adding case1.1.v Verilog diagnostic to check default case statement.  There
 were reporting problems related to this.  Report problems have been fixed and
 full regression passes.

 Revision 1.13  2002/07/09 03:24:48  phase1geo
 Various fixes for module instantiantion handling.  This now works.  Also
 modified report output for toggle, line and combinational information.
 Regression passes.

 Revision 1.12  2002/07/05 05:00:14  phase1geo
 Removing CASE, CASEX, and CASEZ from line and combinational logic results.

 Revision 1.11  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.10  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.9  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.8  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.7  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).
*/

