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
#include "exclude.h"
#include "expr.h"
#include "func_iter.h"
#include "func_unit.h"
#include "instance.h"
#include "line.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "report.h"
#include "util.h"

extern db**         db_list;
extern unsigned int curr_db;

extern bool         report_covered;
extern unsigned int report_comb_depth;
extern bool         report_instance;
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
    func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

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
          (stmt->exp->op != EXP_OP_FOREVER) &&
          (stmt->exp->op != EXP_OP_RASSIGN) &&
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
            func_unit* funit,     /*!< Pointer to functional unit */
            int        cov,       /*!< If set to 1, gets covered lines, if 0 retrieves uncovered lines; otherwise, gets all lines */
  /*@out@*/ int**      lines,     /*!< Pointer to array of integers that will contain the line numbers */
  /*@out@*/ int**      excludes,  /*!< Pointer to array of integers that will contain the exclude values */
  /*@out@*/ char***    reasons,   /*!< Pointer to array of strings that may contain exclusion reasons */
  /*@out@*/ int*       line_cnt,  /*!< Pointer to size of lines and excludes arrays */
  /*@out@*/ int*       line_size  /*!< Pointer to the total number of lines/excludes integers allocated */
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
  *reasons   = (char**)malloc_safe( sizeof( char* ) * (*line_size) );

  /* Initialize the functional unit iterator */
  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

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
        (stmt->exp->op != EXP_OP_FOREVER) &&
        (stmt->exp->op != EXP_OP_RASSIGN) &&
        (stmt->exp->line != 0) ) {

      if( ((stmt->exp->exec_num > 0) ? 1 : 0) == cov ) {

        last_line = expression_get_last_line_expr( stmt->exp )->line;
        for( i=stmt->exp->line; i<=last_line; i++ ) {
          exclude_reason* er;
          if( *line_cnt == *line_size ) {
            *line_size += 20;
            *lines      = (int*)realloc_safe( *lines,    (sizeof( int ) * (*line_size - 20)), (sizeof( int ) * (*line_size)) );
            *excludes   = (int*)realloc_safe( *excludes, (sizeof( int ) * (*line_size - 20)), (sizeof( int ) * (*line_size)) );
            *reasons    = (char**)realloc_safe( *reasons, (sizeof( char* ) * (*line_size - 20)), (sizeof( char* ) * (*line_size)) );
          }
          (*lines)[(*line_cnt)]    = i;
          (*excludes)[(*line_cnt)] = ESUPPL_EXCLUDED( stmt->exp->suppl );

          /* If the toggle is currently excluded, check to see if there's a reason associated with it */
          if( (ESUPPL_EXCLUDED( stmt->exp->suppl ) == 1) && ((er = exclude_find_exclude_reason( 'L', stmt->exp->id, funit )) != NULL) ) {
            (*reasons)[(*line_cnt)] = strdup_safe( er->reason );
          } else {
            (*reasons)[(*line_cnt)] = NULL;
          }
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
  if( db_is_unnamed_scope( pname ) || root->suppl.name_diff ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && root->stat->show && !funit_is_unnamed( root->funit ) &&
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

      miss_found |= line_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->orig_fname ) ), head->funit->stat->line_hit, head->funit->stat->line_total );

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
 Displays the lines missed during simulation to standard output from the
 specified expression list.
*/
static void line_display_verbose(
  FILE*      ofile,  /*!< Pointer to file to output results to */
  func_unit* funit,  /*!< Pointer to functional unit containing lines to display in verbose format */
  rpt_type   rtype   /*!< Specifies the type of lines to output */
) { PROFILE(LINE_DISPLAY_VERBOSE);

  statement*   stmt;        /* Pointer to current statement */
  expression*  unexec_exp;  /* Pointer to current unexecuted expression */
  char**       code;        /* Pointer to code string from code generator */
  unsigned int code_depth;  /* Depth of code array */
  unsigned int i;           /* Loop iterator */
  func_iter    fi;          /* Functional unit iterator */

  switch( rtype ) {
    case RPT_TYPE_HIT  :  fprintf( ofile, "    Hit Lines\n\n" );       break;
    case RPT_TYPE_MISS :  fprintf( ofile, "    Missed Lines\n\n" );    break;
    case RPT_TYPE_EXCL :  fprintf( ofile, "    Excluded Lines\n\n" );  break;
  }

  /* Initialize functional unit iterator */
  func_iter_init( &fi, funit, TRUE, FALSE, FALSE );

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
        (stmt->exp->op != EXP_OP_FOREVER) &&
        (stmt->exp->op != EXP_OP_RASSIGN) &&
        (stmt->exp->line != 0) ) {

      if( ((((stmt->exp->exec_num > 0) ? 1 : 0) == report_covered) && (stmt->suppl.part.excluded == 0) && (rtype != RPT_TYPE_EXCL)) ||
          ((stmt->suppl.part.excluded == 1) && (rtype == RPT_TYPE_EXCL)) ) {

        unexec_exp = stmt->exp;

        codegen_gen_expr( unexec_exp, funit, &code, &code_depth );
        if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) ) {
          exclude_reason* er;
          fprintf( ofile, "      (%s)  %7u:    %s%s\n",
                   db_gen_exclusion_id( 'L', unexec_exp->id ), unexec_exp->line, code[0], ((code_depth == 1) ? "" : "...") );
          if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'L', unexec_exp->id, funit )) != NULL) ) {
            report_output_exclusion_reason( ofile, (22 + (db_get_exclusion_id_size() - 1)), er->reason, TRUE );
          }
        } else {
          exclude_reason* er;
          fprintf( ofile, "      %7u:    %s%s\n", unexec_exp->line, code[0], ((code_depth == 1) ? "" : "...") );
          if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'L', unexec_exp->id, funit )) != NULL) ) {
            report_output_exclusion_reason( ofile, 18, er->reason, TRUE );
          }
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

  if( db_is_unnamed_scope( pname ) || root->suppl.name_diff ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && !funit_is_unnamed( root->funit ) &&
      (((root->stat->line_hit < root->stat->line_total) && !report_covered) ||
       ((root->stat->line_hit > 0) && report_covered) ||
       ((root->stat->line_excluded > 0) && report_exclusions)) ) {

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

    if( ((root->stat->line_hit < root->stat->line_total) && !report_covered) ||
        ((root->stat->line_hit > 0) && report_covered && (!report_exclusions || (root->stat->line_hit > root->stat->line_excluded))) ) {
      line_display_verbose( ofile, root->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    }
    if( (root->stat->line_excluded > 0) && report_exclusions ) {
      line_display_verbose( ofile, root->funit, RPT_TYPE_EXCL );
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
         ((head->funit->stat->line_hit > 0) && report_covered) ||
         ((head->funit->stat->line_excluded > 0) && report_exclusions)) ) {

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

      if( ((head->funit->stat->line_hit < head->funit->stat->line_total) && !report_covered) ||
          ((head->funit->stat->line_hit > 0) && report_covered && (!report_exclusions || (head->funit->stat->line_hit > head->funit->stat->line_excluded))) ) {
        line_display_verbose( ofile, head->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
      }
      if( (head->funit->stat->line_excluded > 0) && report_exclusions ) {
        line_display_verbose( ofile, head->funit, RPT_TYPE_EXCL );
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
  inst_link* instl;                 /* Pointer to current instance link */
  int        acc_hits     = 0;      /* Accumulated line hits for entire design */
  int        acc_total    = 0;      /* Accumulated line total for entire design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   LINE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= line_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_hits, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)line_display_instance_summary( ofile, "Accumulated", acc_hits, acc_total );
    
    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        line_instance_verbose( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = line_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)line_display_funit_summary( ofile, "Accumulated", "", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      line_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

