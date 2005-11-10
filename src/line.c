/*!
 \file     line.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
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

extern funit_inst* instance_root;
extern funit_link* funit_head;

extern bool         report_covered;
extern unsigned int report_comb_depth;
extern bool         report_instance;
extern char         leading_hierarchy[4096];
extern char         second_hierarchy[4096];

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

  stmt_iter curr;          /* Statement list iterator               */
  int       last_hit = 0;  /* Tracks the last line number evaluated */

  stmt_iter_reset( &curr, stmtl );
  
  while( curr.curr != NULL ) {

    if( (curr.curr->stmt->exp->op != EXP_OP_DELAY)   &&
        (curr.curr->stmt->exp->op != EXP_OP_CASE)    &&
        (curr.curr->stmt->exp->op != EXP_OP_CASEX)   &&
        (curr.curr->stmt->exp->op != EXP_OP_CASEZ)   &&
        (curr.curr->stmt->exp->op != EXP_OP_DEFAULT) &&
        (curr.curr->stmt->exp->line != 0) ) {
      *total = *total + 1;
      if( ESUPPL_WAS_EXECUTED( curr.curr->stmt->exp->suppl ) == 1 ) {
        (*hit)++;
        last_hit = curr.curr->stmt->exp->line;
      }
    }

    stmt_iter_next( &curr );

  }

}

/*!
 \param funit_name  Name of functional unit to get missed line number array from.
 \param funit_type  Type of functional unit to get missed line number array from.
 \param cov         If set to 1, gets covered lines, if 0 retrieves uncovered lines; otherwise, gets all lines
 \param lines       Pointer to array of integers that will contain the missed lines.
 \param line_cnt    Pointer to size of lines array.

 \return Returns TRUE if the functional unit specified was found; otherwise, returns FALSE.

 Searches design for specified functional unit name.  If the functional unit name is found, the lines
 array and line_cnt values are initialized and filled with the line numbers that were
 not hit during simulation and a value of TRUE is returned.  If the functional unit name was
 not found, a value of FALSE is returned.
*/
bool line_collect( char* funit_name, int funit_type, int cov, int** lines, int* line_cnt ) {

  bool        retval = TRUE;  /* Return value for this function */
  stmt_iter   stmti;          /* Statement list iterator */
  func_unit   funit;          /* Functional unit used for searching */
  funit_link* funitl;         /* Pointer to found functional unit link */
  int         i;              /* Loop iterator */
  int         last_line;      /* Specifies the last line of the current expression  */
  int         line_size;      /* Indicates the number of entries in the lines array */

  /* First, find functional unit in functional unit array */
  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    /* Create an array that will hold the number of uncovered lines */
    line_size = 20;
    *line_cnt = 0;
    *lines    = (int*)malloc_safe( (sizeof( int ) * line_size), __FILE__, __LINE__ );

    stmt_iter_reset( &stmti, funitl->funit->stmt_tail );
    stmt_iter_find_head( &stmti, FALSE );

    while( stmti.curr != NULL ) {

      if( (stmti.curr->stmt->exp->op != EXP_OP_DELAY)   &&
          (stmti.curr->stmt->exp->op != EXP_OP_CASE)    &&
          (stmti.curr->stmt->exp->op != EXP_OP_CASEX)   &&
          (stmti.curr->stmt->exp->op != EXP_OP_CASEZ)   &&
          (stmti.curr->stmt->exp->op != EXP_OP_DEFAULT) &&
          (stmti.curr->stmt->exp->line != 0) ) {

        if( ESUPPL_WAS_EXECUTED( stmti.curr->stmt->exp->suppl ) == cov ) {

          last_line = expression_get_last_line_expr( stmti.curr->stmt->exp )->line;
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

/*!
 \param funit_name  Name of functional unit to retrieve summary information from.
 \param funit_type  Type of functional unit to retrieve summary information from.
 \param total       Pointer to total number of lines in this functional unit.
 \param hit         Pointer to number of lines hit in this functional unit.

 \return Returns TRUE if specified functional unit was found in design; otherwise,
         returns FALSE.

 Looks up summary information for specified functional unit.  If the functional unit was found,
 the hit and total values for this functional unit are returned to the calling function.
 If the functional unit was not found, a value of FALSE is returned to the calling
 function, indicating that the functional unit was not found in the design and the values
 of total and hit should not be used.
*/
bool line_get_funit_summary( char* funit_name, char* funit_type, int* total, int* hit ) {

  bool        retval = TRUE;  /* Return value for this function        */
  func_unit   funit;          /* Functional unit used for searching    */
  funit_link* funitl;         /* Pointer to found functional unit link */
  char        tmp[21];        /* Temporary string for total            */

  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    snprintf( tmp, 21, "%20.0f", funitl->funit->stat->line_total );
    assert( sscanf( tmp, "%d", total ) == 1 );
    *hit = funitl->funit->stat->line_hit;

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
bool line_instance_summary( FILE* ofile, funit_inst* root, char* parent_inst ) {

  funit_inst* curr;           /* Pointer to current child functional unit instance of this node */
  float       percent;        /* Percentage of lines hit                                        */
  float       miss;           /* Number of lines missed                                         */
  char        tmpname[4096];  /* Temporary holder of instance name                              */

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
 \param head   Pointer to head of functional unit list to explore.

 \return Returns TRUE if there where lines that were found to be missed; otherwise,
         returns FALSE.

 Iterates through the functional unit list, displaying the line coverage results (summary
 format) for each functional unit.
*/
bool line_funit_summary( FILE* ofile, funit_link* head ) {

  float percent;             /* Percentage of lines hit                    */
  float miss;                /* Number of lines missed                     */
  bool  miss_found = FALSE;  /* Set to TRUE if line was found to be missed */

  while( head != NULL ) {

    if( head->funit->stat->line_total == 0 ) {
      percent = 100.0;
    } else {
      percent = ((head->funit->stat->line_hit / head->funit->stat->line_total) * 100);
    }

    miss       = (head->funit->stat->line_total - head->funit->stat->line_hit);
    miss_found = (miss > 0) ? TRUE : miss_found;

    fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5.0f/%5.0f      %3.0f%%\n", 
             head->funit->name,
             get_basename( head->funit->filename ),
             head->funit->stat->line_hit,
             miss,
             head->funit->stat->line_total,
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
void line_display_verbose( FILE* ofile, func_unit* funit ) {

  stmt_iter   stmti;       /* Statement list iterator                    */
  expression* unexec_exp;  /* Pointer to current unexecuted expression   */
  char**      code;        /* Pointer to code string from code generator */
  int         code_depth;  /* Depth of code array                        */
  int         i;           /* Loop iterator                              */

  if( report_covered ) {
    fprintf( ofile, "    Hit Lines\n\n" );
  } else {
    fprintf( ofile, "    Missed Lines\n\n" );
  }

  /* Display current instance missed lines */
  stmt_iter_reset( &stmti, funit->stmt_tail );
  stmt_iter_find_head( &stmti, FALSE );
  
  while( stmti.curr != NULL ) {

    if( (stmti.curr->stmt->exp->op != EXP_OP_DELAY)   &&
        (stmti.curr->stmt->exp->op != EXP_OP_CASE)    &&
        (stmti.curr->stmt->exp->op != EXP_OP_CASEX)   &&
        (stmti.curr->stmt->exp->op != EXP_OP_CASEZ)   &&
        (stmti.curr->stmt->exp->op != EXP_OP_DEFAULT) &&
        (stmti.curr->stmt->exp->line != 0) ) {

      if( ESUPPL_WAS_EXECUTED( stmti.curr->stmt->exp->suppl ) == report_covered ) {

        unexec_exp = stmti.curr->stmt->exp;

        codegen_gen_expr( unexec_exp, unexec_exp->op, &code, &code_depth, funit );
        if( code_depth == 1 ) {
          fprintf( ofile, "      %7d:    %s\n", unexec_exp->line, code[0] );
        } else {
          fprintf( ofile, "      %7d:    %s...\n", unexec_exp->line, code[0] );
        }
        for( i=0; i<code_depth; i++ ) {
          free_safe( code[i] );
        }
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
 (and associated verilog code) and file/functional unit name of the lines that were 
 not hit during simulation.
*/
void line_instance_verbose( FILE* ofile, funit_inst* root, char* parent_inst ) {

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder for instance          */

  assert( root != NULL );

  if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, root->name );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, root->name );
  }

  if( ((root->stat->line_hit < root->stat->line_total) && !report_covered) ||
        ((root->stat->line_hit > 0) && report_covered) ) {

    fprintf( ofile, "\n" );
    fprintf( ofile, "    Module: %s, File: %s, Instance: %s\n", 
             root->funit->name, 
             root->funit->filename,
             tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

    line_display_verbose( ofile, root->funit );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    line_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }
 
}

/*!
 \param ofile  Pointer to file to output results to.
 \param head   Pointer to head of functional unit list to search through.

 Displays the verbose line coverage results to the specified output stream on
 a functional unit basis (combining functional units that are instantiated multiple times).
 The verbose line coverage includes the line numbers (and associated verilog
 code) and file/functional unit name of the lines that were not hit during simulation.
*/
void line_funit_verbose( FILE* ofile, funit_link* head ) {

  while( head != NULL ) {

    if( ((head->funit->stat->line_hit < head->funit->stat->line_total) && !report_covered) ||
        ((head->funit->stat->line_hit > 0) && report_covered) ) {

      fprintf( ofile, "\n" );
      fprintf( ofile, "    Module: %s, File: %s\n", 
               head->funit->name, 
               head->funit->filename );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      line_display_verbose( ofile, head->funit );
  
    }

    head = head->next;
 
  }

}

/*!
 \param ofile    Pointer to file to output results to.
 \param verbose  Specifies whether to generate summary or verbose output.

 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the line coverage for each functional unit encountered.  The parent functional unit will
 specify its own line coverage along with a total line coverage including its 
 children.
*/
void line_report( FILE* ofile, bool verbose ) {

  bool missed_found;  /* If set to TRUE, lines were found to be missed */
  char tmp[4096];     /* Temporary string value                        */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   LINE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( strcmp( leading_hierarchy, second_hierarchy ) != 0 ) {
      strcpy( tmp, "<NA>" );
    } else {
      strcpy( tmp, leading_hierarchy );
    }

    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = line_instance_summary( ofile, instance_root, tmp );
    
    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      line_instance_verbose( ofile, instance_root, tmp );
    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = line_funit_summary( ofile, funit_head );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      line_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}

/*
 $Log$
 Revision 1.49  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.48  2005/02/05 05:29:25  phase1geo
 Added race condition reporting to GUI.

 Revision 1.47  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.46  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.45  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.44  2004/04/01 22:54:38  phase1geo
 Making text field read-only.  Adding message when reading in new CDD files
 (as status information -- this is not working correctly yet).  Fixing bug
 in line.c when getting total lines summary information.

 Revision 1.43  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.42  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.41  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.40  2004/01/30 23:23:27  phase1geo
 More report output improvements.  Still not ready with regressions.

 Revision 1.39  2004/01/30 06:04:45  phase1geo
 More report output format tweaks.  Adjusted lines and spaces to make things
 look more organized.  Still some more to go.  Regression will fail at this
 point.

 Revision 1.38  2004/01/23 14:37:09  phase1geo
 Fixing output of instance line, toggle, comb and fsm to only output module
 name if logic is detected missing in that instance.  Full regression fails
 with this fix.

 Revision 1.37  2003/12/12 17:16:25  phase1geo
 Changing code generator to output logic based on user supplied format.  Full
 regression fails at this point due to mismatching report files.

 Revision 1.36  2003/12/02 22:38:06  phase1geo
 Removing unnecessary verbosity.

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

