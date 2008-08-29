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
 \file     line.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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

#include "codegen.h"
#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_iter.h"
#include "func_unit.h"
#include "instance.h"
#include "iter.h"
#include "line.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "util.h"

extern db**         db_list;
extern unsigned int curr_db;

extern bool         report_covered;
extern unsigned int report_comb_depth;
extern bool         report_instance;
extern char**       leading_hierarchies;
extern int          leading_hier_num;
extern bool         leading_hiers_differ;
extern isuppl       info_suppl;
extern bool         flag_suppress_empty_funits;
extern bool         flag_output_exclusion_ids;
extern bool         report_exclusions;


/*!
 Iterates through given statement list, gathering information about which
 lines exist in the list, which lines were hit during simulation and which
 lines were missed during simulation.  This information is used to report
 summary information about line coverage.
*/
void line_get_stats(
            func_unit*    funit,     /*!< Pointer to current functional unit to explore */
  /*@out@*/ unsigned int* hit,       /*!< Holds total number of lines hit */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to the number of excluded lines */
  /*@out@*/ unsigned int* total      /*!< Holds total number of lines parsed */
) { PROFILE(LINE_GET_STATS);

  statement* stmt;  /* Pointer to current statement */
  func_iter  fi;    /* Functional unit iterator */

  if( !funit_is_unnamed( funit ) ) {

    /* Initialize the functional unit iterator */
    func_iter_init( &fi, funit, TRUE, FALSE );

    stmt = func_iter_get_next_statement( &fi );
    while( stmt != NULL ) {

      if( (stmt->exp->op != EXP_OP_DELAY)   &&
          (stmt->exp->op != EXP_OP_CASE)    &&
          (stmt->exp->op != EXP_OP_CASEX)   &&
          (stmt->exp->op != EXP_OP_CASEZ)   &&
          (stmt->exp->op != EXP_OP_DEFAULT) &&
          (stmt->exp->op != EXP_OP_NB_CALL) &&
          (stmt->exp->op != EXP_OP_FORK)    &&
          (stmt->exp->op != EXP_OP_JOIN)    &&
          (stmt->exp->op != EXP_OP_NOOP)    &&
          (stmt->exp->line != 0) ) {
        *total = *total + 1;
        if( (stmt->exp->exec_num > 0) || (stmt->suppl.part.excluded == 1) ) {
          (*hit)++;
          if( stmt->suppl.part.excluded == 1 ) {
            (*excluded)++;
          }
        }
      }

      stmt = func_iter_get_next_statement( &fi );

    }

    func_iter_dealloc( &fi );

  }

  PROFILE_END;

}

/*!
 Allocates and populates the lines and exludes array with the line numbers and exclusion values (respectively) that were
 not hit during simulation.
*/
void line_collect(
            func_unit* funit,       /*!< Pointer to functional unit */
            int        cov,         /*!< If set to 1, gets covered lines, if 0 retrieves uncovered lines; otherwise, gets all lines */
  /*@out@*/ int**      lines,       /*!< Pointer to array of integers that will contain the line numbers */
  /*@out@*/ int**      excludes,    /*!< Pointer to array of integers that will contain the exclude values */
  /*@out@*/ int*       line_cnt,    /*!< Pointer to size of lines and excludes arrays */
  /*@out@*/ int*       line_size    /*!< Pointer to the total number of lines/excludes integers allocated */
) { PROFILE(LINE_COLLECT);

  int         i;          /* Loop iterator */
  int         last_line;  /* Specifies the last line of the current expression  */
  statement*  stmt;       /* Pointer to current statement */
  func_iter   fi;         /* Functional unit iterator */

  /* Create an array that will hold the number of uncovered lines */
  *line_size = 20;
  *line_cnt  = 0;
  *lines     = (int*)malloc_safe( sizeof( int ) * (*line_size) );
  *excludes  = (int*)malloc_safe( sizeof( int ) * (*line_size) );

  /* Initialize the functional unit iterator */
  func_iter_init( &fi, funit, TRUE, FALSE );

  stmt = func_iter_get_next_statement( &fi );
  while( stmt != NULL ) {

    if( (stmt->exp->op != EXP_OP_DELAY)   &&
        (stmt->exp->op != EXP_OP_CASE)    &&
        (stmt->exp->op != EXP_OP_CASEX)   &&
        (stmt->exp->op != EXP_OP_CASEZ)   &&
        (stmt->exp->op != EXP_OP_DEFAULT) &&
        (stmt->exp->op != EXP_OP_NB_CALL) &&
        (stmt->exp->op != EXP_OP_FORK)    &&
        (stmt->exp->op != EXP_OP_JOIN)    &&
        (stmt->exp->op != EXP_OP_NOOP)    &&
        (stmt->exp->line != 0) ) {

      if( ((stmt->exp->exec_num > 0) ? 1 : 0) == cov ) {

        last_line = expression_get_last_line_expr( stmt->exp )->line;
        for( i=stmt->exp->line; i<=last_line; i++ ) {
          if( *line_cnt == *line_size ) {
            *line_size += 20;
            *lines      = (int*)realloc_safe( *lines,    (sizeof( int ) * (*line_size - 20)), (sizeof( int ) * (*line_size)) );
            *excludes   = (int*)realloc_safe( *excludes, (sizeof( int ) * (*line_size - 20)), (sizeof( int ) * (*line_size)) );
          }
          (*lines)[(*line_cnt)]    = i;
          (*excludes)[(*line_cnt)] = ESUPPL_EXCLUDED( stmt->exp->suppl );
          (*line_cnt)++;
        }

      }

    }

    stmt = func_iter_get_next_statement( &fi );

  }

  func_iter_dealloc( &fi );

  PROFILE_END;

}

/*!
 Looks up summary information for specified functional unit.
*/
void line_get_funit_summary(
            func_unit*    funit,     /*!< Pointer to functional unit */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to number of lines hit in this functional unit */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of lines excluded in this functional unit */
  /*@out@*/ unsigned int* total      /*!< Pointer to total number of lines in this functional unit */
) { PROFILE(LINE_GET_FUNIT_SUMMARY);

  *hit      = funit->stat->line_hit;
  *excluded = funit->stat->line_excluded; 
  *total    = funit->stat->line_total;

  PROFILE_END;

}

/*!
 Looks up summary information for specified functional unit instance.
*/
void line_get_inst_summary(
            funit_inst*   inst,      /*!< Pointer to functional unit instance */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to number of lines hit in this functional unit */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of lines excluded in this functional unit */
  /*@out@*/ unsigned int* total      /*!< Pointer to total number of lines in this functional unit */
) { PROFILE(LINE_GET_INST_SUMMARY);

  *hit      = inst->stat->line_hit;
  *excluded = inst->stat->line_excluded;
  *total    = inst->stat->line_total;

  PROFILE_END;

}

/*!
 \return Returns TRUE if any missed lines were found.

 Outputs the instance summary information to the given output file.
*/
static bool line_display_instance_summary(
  FILE*       ofile,  /*!< Pointer to output file to display information to */
  const char* name,   /*!< Name of instance to display */
  int         hits,   /*!< Number of lines hit in the given instance */
  int         total   /*!< Total number of lines in the given instance */
) { PROFILE(LINE_DISPLAY_INSTANCE_SUMMARY);

  float percent;  /* Percentage of lines hits */
  int   miss;     /* Number of lines missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-43.43s    %5d/%5d/%5d      %3.0f%%\n",
           name, hits, miss, total, percent );

  PROFILE_END;

  return( miss > 0 );

}

/*!
 \return Returns TRUE if lines were found to be missed; otherwise, returns FALSE.
 
 Recursively iterates through the instance tree gathering the
 total number of lines parsed vs. the total number of lines
 executed during the course of simulation.  The parent node will
 display its information before calling its children.
*/
static bool line_instance_summary(
            FILE*       ofile,        /*!< Pointer to file to output results to */
            funit_inst* root,         /*!< Current node in instance tree */
            char*       parent_inst,  /*!< Name of parent instance */
  /*@out@*/ int*        hits,         /*!< Pointer to accumulated hit information */
  /*@out@*/ int*        total         /*!< Pointer to accumulated total information */
) { PROFILE(LINE_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary holder of instance name */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if a line was found to be missed */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Get printable version of the instance name */
  pname = scope_gen_printable( root->name );
  
  /* Calculate instance name */
  if( db_is_unnamed_scope( pname ) ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( root->stat->show && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    miss_found = line_display_instance_summary( ofile, tmpname, root->stat->line_hit, root->stat->line_total );

    /* Update accumulated information */
    *hits  += root->stat->line_hit;
    *total += root->stat->line_total;

  }

  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= line_instance_summary( ofile, curr, tmpname, hits, total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );
           
}

/*!
 \return Returns TRUE if at least one line was found to be missed; otherwise, returns FALSE.

 Calculates the percentage and miss information for the given hit and total coverage info and
 outputs this information in human-readable format to the given output file.
*/
static bool line_display_funit_summary(
  FILE*       ofile,  /*!< Pointer to file to output functional unit line summary information to */
  const char* name,   /*!< Name of functional unit being displayed */
  const char* fname,  /*!< Filename containing function unit being displayed */
  int         hits,   /*!< Number of hits in this functional unit */
  int         total   /*!< Number of total lines in this functional unit */
) { PROFILE(LINE_DISPLAY_FUNIT_SUMMARY);

  float percent;  /* Percentage of lines hits */
  int   miss;     /* Number of lines missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5d/%5d      %3.0f%%\n", 
           name, fname, hits, miss, total, percent );

  PROFILE_END;

  return (miss > 0);

}

/*!
 \return Returns TRUE if there where lines that were found to be missed; otherwise,
         returns FALSE.

 Iterates through the functional unit list, displaying the line coverage results (summary
 format) for each functional unit.
*/
static bool line_funit_summary(
            FILE*       ofile,  /*!< Pointer to file to output results to */
            funit_link* head,   /*!< Pointer to head of functional unit list to explore */
  /*@out@*/ int*        hits,   /*!< Pointer to accumulated hit information */
  /*@out@*/ int*        total   /*!< Pointer to accumulated total information */
) { PROFILE(LINE_FUNIT_SUMMARY);

  bool  miss_found = FALSE;  /* Set to TRUE if line was found to be missed */
  char* pname;               /* Printable version of functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= line_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->filename ) ), head->funit->stat->line_hit, head->funit->stat->line_total );

      /* Update accumulated information */
      *hits  += head->funit->stat->line_hit;
      *total += head->funit->stat->line_total;

      free_safe( pname, (strlen( pname ) + 1) );

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one excluded line was found.

 Displays the lines missed during simulation to standard output from the
 specified expression list.
*/
static bool line_display_verbose(
  FILE*      ofile,  /*!< Pointer to file to output results to */
  func_unit* funit,  /*!< Pointer to functional unit containing lines to display in verbose format */
  rpt_type   rtype   /*!< Specifies the type of lines to output */
) { PROFILE(LINE_DISPLAY_VERBOSE);

  bool         retval = FALSE;  /* Return value for this function */
  statement*   stmt;            /* Pointer to current statement */
  expression*  unexec_exp;      /* Pointer to current unexecuted expression */
  char**       code;            /* Pointer to code string from code generator */
  unsigned int code_depth;      /* Depth of code array */
  unsigned int i;               /* Loop iterator */
  func_iter    fi;              /* Functional unit iterator */

  switch( rtype ) {
    case RPT_TYPE_HIT  :  fprintf( ofile, "    Hit Lines\n\n" );       break;
    case RPT_TYPE_MISS :  fprintf( ofile, "    Missed Lines\n\n" );    break;
    case RPT_TYPE_EXCL :  fprintf( ofile, "    Excluded Lines\n\n" );  break;
  }

  /* Initialize functional unit iterator */
  func_iter_init( &fi, funit, TRUE, FALSE );

  /* Display current instance missed lines */
  while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

    if( (stmt->exp->op != EXP_OP_DELAY)   &&
        (stmt->exp->op != EXP_OP_CASE)    &&
        (stmt->exp->op != EXP_OP_CASEX)   &&
        (stmt->exp->op != EXP_OP_CASEZ)   &&
        (stmt->exp->op != EXP_OP_DEFAULT) &&
        (stmt->exp->op != EXP_OP_NB_CALL) &&
        (stmt->exp->op != EXP_OP_FORK)    &&
        (stmt->exp->op != EXP_OP_JOIN)    &&
        (stmt->exp->op != EXP_OP_NOOP)    &&
        (stmt->exp->line != 0) ) {

      retval |= stmt->suppl.part.excluded;

      if( ((((stmt->exp->exec_num > 0) ? 1 : 0) == report_covered) && (stmt->suppl.part.excluded == 0) && (rtype != RPT_TYPE_EXCL)) ||
          ((stmt->suppl.part.excluded == 1) && (rtype == RPT_TYPE_EXCL)) ) {

        unexec_exp = stmt->exp;

        codegen_gen_expr( unexec_exp, unexec_exp->op, &code, &code_depth, funit );
        if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) ) {
          fprintf( ofile, "      (%s)  %7d:    %s%s\n",
                   db_gen_exclusion_id( 'L', unexec_exp->id ), unexec_exp->line, code[0], ((code_depth == 1) ? "" : "...") );
        } else {
          fprintf( ofile, "      %7d:    %s%s\n", unexec_exp->line, code[0], ((code_depth == 1) ? "" : "...") );
        }
        for( i=0; i<code_depth; i++ ) {
          free_safe( code[i], (strlen( code[i] ) + 1) );
        }
        free_safe( code, (sizeof( char* ) * code_depth) );

      }

    }

  }

  func_iter_dealloc( &fi );

  fprintf( ofile, "\n" );

  PROFILE_END;

  return( retval );

}

/*!
 Displays the verbose line coverage results to the specified output stream on
 an instance basis.  The verbose line coverage includes the line numbers 
 (and associated verilog code) and file/functional unit name of the lines that were 
 not hit during simulation.
*/
static void line_instance_verbose(
  FILE*       ofile,       /*!< Pointer to file to output results to */
  funit_inst* root,        /*!< Pointer to root node of instance tree to search through */
  char*       parent_inst  /*!< Hierarchical path of parent instance */
) { PROFILE(LINE_INSTANCE_VERBOSE);

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder for instance */
  char*       pname;          /* Printable version of functional unit name */

  assert( root != NULL );

  /* Get printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( !funit_is_unnamed( root->funit ) &&
      (((root->stat->line_hit < root->stat->line_total) && !report_covered) ||
       ((root->stat->line_hit > 0) && report_covered)) ) {

    bool exclusion_found;

    /* Get printable version of functional unit name */
    pname = scope_gen_printable( funit_flatten_name( root->funit ) );

    fprintf( ofile, "\n" );
    switch( root->funit->type ) {
      case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_AFUNCTION    :
      case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_ATASK        :
      case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
      default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, obf_file( root->funit->filename ), tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

    free_safe( pname, (strlen( pname ) + 1) );

    exclusion_found = line_display_verbose( ofile, root->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    if( report_exclusions && exclusion_found ) {
      (void)line_display_verbose( ofile, root->funit, RPT_TYPE_EXCL );
    }

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    line_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

  PROFILE_END;
 
}

/*!
 Displays the verbose line coverage results to the specified output stream on
 a functional unit basis (combining functional units that are instantiated multiple times).
 The verbose line coverage includes the line numbers (and associated verilog
 code) and file/functional unit name of the lines that were not hit during simulation.
*/
static void line_funit_verbose(
  FILE*       ofile,  /*!< Pointer to file to output results to */
  funit_link* head    /*!< Pointer to head of functional unit list to search through */
) { PROFILE(LINE_FUNIT_VERBOSE);

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        (((head->funit->stat->line_hit < head->funit->stat->line_total) && !report_covered) ||
         ((head->funit->stat->line_hit > 0) && report_covered)) ) {

      bool exclusion_found;

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      fprintf( ofile, "\n" );
      switch( head->funit->type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", pname, obf_file( head->funit->filename ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      free_safe( pname, (strlen( pname ) + 1) );

      exclusion_found = line_display_verbose( ofile, head->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
      if( report_exclusions && exclusion_found ) {
        (void)line_display_verbose( ofile, head->funit, RPT_TYPE_EXCL );
      }
  
    }

    head = head->next;
 
  }

  PROFILE_END;

}

/*!
 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the line coverage for each functional unit encountered.  The parent functional unit will
 specify its own line coverage along with a total line coverage including its 
 children.
*/
void line_report(
  FILE* ofile,   /*!< Pointer to file to output results to */
  bool  verbose  /*!< Specifies whether to generate summary or verbose output */
) { PROFILE(LINE_REPORT);

  bool       missed_found = FALSE;  /* If set to TRUE, lines were found to be missed */
  char       tmp[4096];             /* Temporary string value */
  inst_link* instl;                 /* Pointer to current instance link */
  int        acc_hits     = 0;      /* Accumulated line hits for entire design */
  int        acc_total    = 0;      /* Accumulated line total for entire design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   LINE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( leading_hiers_differ ) {
      strcpy( tmp, "<NA>" );
    } else {
      assert( leading_hier_num > 0 );
      strcpy( tmp, leading_hierarchies[0] );
    }

    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= line_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*"), &acc_hits, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)line_display_instance_summary( ofile, "Accumulated", acc_hits, acc_total );
    
    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        line_instance_verbose( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = line_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)line_display_funit_summary( ofile, "Accumulated", "", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      line_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

/*
 $Log$
 Revision 1.97  2008/08/28 13:59:19  phase1geo
 More updates to be more efficient in outputting exclusion IDs.  Also added
 capability (or the start of) to output exclusions when the -e option is
 specified.  Updated regressions per these changes.  Checkpointing.

 Revision 1.96  2008/08/28 04:37:18  phase1geo
 Starting to add support for exclusion output and exclusion IDs to generated
 reports.  These changes should break regressions.  Checkpointing.

 Revision 1.95  2008/08/22 20:56:35  phase1geo
 Starting to make updates for proper unnamed scope report handling (fix for bug 2054686).
 Not complete yet.  Also making updates to documentation.  Checkpointing.

 Revision 1.94  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.89.4.3  2008/08/07 06:39:11  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.89.4.2  2008/08/06 20:11:34  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.89.4.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.92  2008/06/27 14:02:02  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.91  2008/06/22 05:08:40  phase1geo
 Fixing memory testing error in tcl_funcs.c.  Removed unnecessary output in main_view.tcl.
 Added a few more report diagnostics and started to remove width report files (these will
 no longer be needed and will improve regression runtime and diagnostic memory footprint.

 Revision 1.90  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.89  2008/04/15 20:37:11  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.88  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.87  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.86  2008/03/14 05:20:49  phase1geo
 Fixing line.c and updating regressions as necessary.

 Revision 1.85  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.84  2008/02/22 16:40:00  phase1geo
 Fixing bug 1899735.

 Revision 1.83  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.82  2008/01/16 05:01:22  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.81  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.80  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.79  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.78  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.77  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.76  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.75  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.74  2007/07/16 12:39:33  phase1geo
 Started to add support for displaying accumulated coverage results for
 each metric.  Finished line and toggle and am half-way done with memory
 coverage (still have combinational logic, FSM and assertion coverage
 to complete before this feature is fully functional).

 Revision 1.73  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.72  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.71  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.70  2007/04/02 04:50:04  phase1geo
 Adding func_iter files to iterate through a functional unit for reporting
 purposes.  Updated affected files.

 Revision 1.69  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.68  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.67  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.66  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.65  2006/08/22 21:46:03  phase1geo
 Updating from stable branch.

 Revision 1.64  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.63  2006/06/29 20:57:24  phase1geo
 Added stmt_excluded bit to expression to allow us to individually control line
 and combinational logic exclusion.  This also allows us to exclude combinational
 logic within excluded lines.  Also fixing problem with highlighting the listbox
 (due to recent changes).

 Revision 1.62  2006/06/26 04:12:55  phase1geo
 More updates for supporting coverage exclusion.  Still a bit more to go
 before this is working properly.

 Revision 1.61  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

 Revision 1.60  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.59  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.58  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.57  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.56  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.55  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.54  2005/12/31 05:00:57  phase1geo
 Updating regression due to recent changes in adding exec_num field in expression
 and removing the executed bit in the expression supplemental field.  This will eventually
 allow us to get information on where the simulator is spending the most time.

 Revision 1.53  2005/12/06 15:42:11  phase1geo
 Added disable2.1 diagnostic to regression suite.  Fixed bad output from line
 coverage reports.  Full regression passes.

 Revision 1.52  2005/11/30 18:25:56  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.51  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.

 Revision 1.50  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

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

