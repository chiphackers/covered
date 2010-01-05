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
 \file     assertion.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/4/2006
*/


#include <stdio.h>
#include <assert.h>

#include "assertion.h"
#include "db.h"
#include "defines.h"
#include "func_unit.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "profiler.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern bool         report_covered;
extern bool         report_instance;
extern isuppl       info_suppl;
extern bool         report_exclusions;


/*!
 \param arg  Command-line argument specified in -A option to score command to parse

 Parses the specified command-line argument as an assertion to consider for coverage.
*/
void assertion_parse( /*@unused@*/const char* arg ) { PROFILE(ASSERTION_PARSE);

  PROFILE_END;

}

/*!
 Parses the specified assertion attribute for assertion coverage details.
*/
void assertion_parse_attr(
  /*@unused@*/ attr_param*      ap,      /*!< Pointer to attribute to parse */
  /*@unused@*/ const func_unit* funit,   /*!< Pointer to current functional unit containing this attribute */
  /*@unused@*/ bool             exclude  /*!< If TRUE, excludes this assertion from coverage consideration */
) { PROFILE(ASSERTION_PARSE_ATTR);

  PROFILE_END;

}

/*!
 Gets total and hit assertion coverage statistics for the given functional unit.
*/
void assertion_get_stats(
            const func_unit* funit,     /*!< Pointer to current functional unit */
  /*@out@*/ unsigned int*    hit,       /*!< Pointer to the total number of hit assertions in this functional unit */
  /*@out@*/ unsigned int*    excluded,  /*!< Pointer to the number of excluded assertions */
  /*@out@*/ unsigned int*    total      /*!< Pointer to the total number of assertions found in this functional unit */
) { PROFILE(ASSERTION_GET_STATS);

  assert( funit != NULL );

  /* Initialize total and hit values */
  *hit      = 0;
  *excluded = 0;
  *total    = 0;

  /* If OVL assertion coverage is needed, check this functional unit */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_get_funit_stats( funit, hit, excluded, total );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if at least one assertion was found to be not hit; otherwise, returns FALSE.

 Displays the assertion summary information for a given instance to the specified output stream.
*/
static bool assertion_display_instance_summary(
  FILE*       ofile,  /*!< Pointer to file handle to output instance summary results to */
  const char* name,   /*!< Name of instance being reported */
  int         hits,   /*!< Total number of assertions hit in the given instance */
  int         total   /*!< Total number of assertions in the given instance */
) { PROFILE(ASSERTION_DISPLAY_INSTANCE_SUMMARY);

  float percent;  /* Percentage of assertions hit */
  int   miss;     /* Number of assertions missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-43.43s    %5d/%5d/%5d      %3.0f%%\n",
           name, hits, miss, total, percent );

  PROFILE_END;

  return( miss > 0 );

}

/*!
 \return Returns TRUE if at least one assertion was found to not be covered in this
         function unit instance; otherwise, returns FALSE.

 Outputs the instance summary assertion coverage information for the given functional
 unit instance to the given output file.
*/
static bool assertion_instance_summary(
            FILE*             ofile,        /*!< Pointer to the file to output the instance summary information to */
            const funit_inst* root,         /*!< Pointer to the current functional unit instance to look at */
            const char*       parent_inst,  /*!< Scope of parent instance of this instance */
  /*@out@*/ int*              hits,         /*!< Pointer to number of assertions hit in given instance tree */
  /*@out@*/ int*              total         /*!< Pointer to total number of assertions found in given instance tree */
) { PROFILE(ASSERTION_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary holder of instance name */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if assertion was missed */

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
    /*@-retvalint@*/snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && root->stat->show && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    miss_found |= assertion_display_instance_summary( ofile, tmpname, root->stat->assert_hit, root->stat->assert_total ); 

    /* Update accumulated information */
    *hits  += root->stat->assert_hit;
    *total += root->stat->assert_total;

  }

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= assertion_instance_summary( ofile, curr, tmpname, hits, total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one assertion was found to be not hit; otherwise, returns FALSE.

 Displays the assertion summary information for a given instance to the specified output stream.
*/
static bool assertion_display_funit_summary(
  FILE*       ofile,  /*!< Pointer to file handle to output instance summary results to */
  const char* name,   /*!< Name of functional unit being reported */
  const char* fname,  /*!< Name of file containing the given functional unit */
  int         hits,   /*!< Total number of assertions hit in the given functional unit */
  int         total   /*!< Total number of assertions in the given functional unit */
) { PROFILE(ASSERTION_DISPLAY_FUNIT_SUMMARY);

  float percent;  /* Percentage of assertions hit */
  int   miss;     /* Number of assertions missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5d/%5d      %3.0f%%\n",
           name, fname, hits, miss, total, percent );

  PROFILE_END;

  return( miss > 0 );

}

/*!
 \return Returns TRUE if at least one assertion was found to not be covered in this
         functional unit; otherwise, returns FALSE.

 Outputs the functional unit summary assertion coverage information for the given
 functional unit to the given output file.
*/
static bool assertion_funit_summary(
            FILE*             ofile,  /*!< Pointer to output file to display summary information to */
            const funit_link* head,   /*!< Pointer to the current functional unit link to evaluate */
  /*@out@*/ int*              hits,   /*!< Pointer to the number of assertions hit in all functional units */
  /*@out@*/ int*              total   /*!< Pointer to the total number of assertions found in all functional units */
) { PROFILE(ASSERTION_FUNIT_SUMMARY);

  bool miss_found = FALSE;  /* Set to TRUE if assertion was found to be missed */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      /*@only@*/ char* pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= assertion_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->orig_fname ) ),
                                                     head->funit->stat->assert_hit, head->funit->stat->assert_total );

      /* Update accumulated information */
      *hits  += head->funit->stat->assert_hit;
      *total += head->funit->stat->assert_total;

      free_safe( pname, (strlen( pname ) + 1) );

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 Displays the verbose hit/miss assertion information for the given functional unit.
*/
static void assertion_display_verbose(
  FILE*            ofile,  /*!< Pointer to the file to output the verbose information to */
  const func_unit* funit,  /*!< Pointer to the functional unit to display */
  rpt_type         rtype   /*!< Type of report output to generate */
) { PROFILE(ASSERTION_DISPLAY_VERBOSE);

  switch( rtype ) {
    case RPT_TYPE_HIT  :  fprintf( ofile, "    Hit Assertions\n\n" );       break;
    case RPT_TYPE_MISS :  fprintf( ofile, "    Missed Assertions\n\n" );    break;
    case RPT_TYPE_EXCL :  fprintf( ofile, "    Excluded Assertions\n\n" );  break;
  }

  /* If OVL assertion coverage is needed, output it in the OVL style */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_display_verbose( ofile, funit, rtype );
  }

  fprintf( ofile, "\n" );

  PROFILE_END;

}

/*!
 Outputs the instance verbose assertion coverage information for the given functional
 unit instance to the given output file.
*/
static void assertion_instance_verbose(
  FILE*       ofile,       /*!< Pointer to the file to output the instance verbose information to */
  funit_inst* root,        /*!< Pointer to the current functional unit instance to look at */
  char*       parent_inst  /*!< Scope of parent of this functional unit instance */
) { PROFILE(ASSERTION_INSTANCE_VERBOSE);

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
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( !funit_is_unnamed( root->funit ) &&
      (((root->stat->assert_hit < root->stat->assert_total) && !report_covered) ||
       ((root->stat->assert_hit > 0) && report_covered) ||
       ((root->stat->assert_excluded > 0) && report_exclusions)) ) {

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

    if( ((root->stat->assert_hit < root->stat->assert_total) && !report_covered) ||
        ((root->stat->assert_hit > 0) && report_covered && (!report_exclusions || (root->stat->assert_hit > root->stat->assert_excluded))) ) {
      assertion_display_verbose( ofile, root->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    }
    if( report_exclusions && (root->stat->assert_excluded > 0) ) {
      assertion_display_verbose( ofile, root->funit, RPT_TYPE_EXCL );
    }

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    assertion_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

  PROFILE_END;

}

/*!
 Outputs the functional unit verbose assertion coverage information for the given
 functional unit to the given output file.
*/
static void assertion_funit_verbose(
  FILE*             ofile,  /*!< Pointer to the file to output the functional unit verbose information to */
  const funit_link* head    /*!< Pointer to the current functional unit link to look at */
) { PROFILE(ASSERTION_FUNIT_VERBOSE);

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        (((head->funit->stat->assert_hit < head->funit->stat->assert_total) && !report_covered) ||
         ((head->funit->stat->assert_hit > 0) && report_covered) ||
         ((head->funit->stat->assert_excluded > 0) && report_exclusions)) ) {

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

      if( ((head->funit->stat->assert_hit < head->funit->stat->assert_total) && !report_covered) ||
          ((head->funit->stat->assert_hit > 0) && report_covered && (!report_exclusions || (head->funit->stat->assert_hit > head->funit->stat->assert_excluded))) ) {
        assertion_display_verbose( ofile, head->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
      }
      if( report_exclusions && (head->funit->stat->assert_excluded > 0) ) {
        assertion_display_verbose( ofile, head->funit, RPT_TYPE_EXCL );
      }

    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Outputs assertion coverage report information to the given file handle. 
*/
void assertion_report(
  FILE* ofile,   /*!< File to output report contents to */
  bool  verbose  /*!< Specifies if verbose report output needs to be generated */
) { PROFILE(ASSERTION_REPORT);

  bool       missed_found = FALSE;  /* If set to TRUE, lines were found to be missed */
  inst_link* instl;                 /* Pointer to current instance link */
  int        acc_hits     = 0;      /* Total number of assertions hit */
  int        acc_total    = 0;      /* Total number of assertions in design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   ASSERTION COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= assertion_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_hits, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)assertion_display_instance_summary( ofile, "Accumulated", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        assertion_instance_verbose( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*") );
        instl = instl->next;
      }

    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = assertion_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)assertion_display_funit_summary( ofile, "Accumulated", "", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      assertion_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

/*!
 Counts the total number and number of hit assertions for the specified functional unit.
*/
void assertion_get_funit_summary(
            func_unit*    funit,     /*!< Pointer to functional unit */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to the number of hit assertions in the specified functional unit */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to the number of excluded assertions */
  /*@out@*/ unsigned int* total      /*!< Pointer to the total number of assertions in the specified functional unit */
) { PROFILE(ASSERTION_GET_FUNIT_SUMMARY);
	
  /* Initialize total and hit counts */
  *hit      = 0;
  *excluded = 0;
  *total    = 0;
  
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_get_funit_stats( funit, hit, excluded, total );
  }
    
  PROFILE_END;
  
}

/*!
 Searches the specified functional unit, collecting all uncovered or covered assertion module instance names.
*/
void assertion_collect(
            func_unit*    funit,       /*!< Pointer to functional unit */
            int           cov,         /*!< Specifies if uncovered (0) or covered (1) assertions should be looked for */
  /*@out@*/ char***       inst_names,  /*!< Pointer to array of uncovered instance names found within the specified functional unit */
  /*@out@*/ int**         excludes,    /*!< Pointer to array of integers that contain the exclude information for the given instance names */
  /*@out@*/ unsigned int* inst_size    /*!< Number of valid elements in the uncov_inst_names array */
) { PROFILE(ASSERTION_COLLECT);
  
  /* Initialize outputs */
  *inst_names = NULL;
  *excludes   = NULL;
  *inst_size  = 0;
    
  /* If OVL assertion coverage is needed, get this information */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_collect( funit, cov, inst_names, excludes, inst_size );
  }
    
  PROFILE_END;

}

/*!
 Finds all of the coverage points for the given assertion instance and stores their
 string descriptions and execution counts in the cp list.
*/
void assertion_get_coverage(
            const func_unit* funit,       /*!< Pointer to functional unit */
            const char*      inst_name,   /*!< Name of assertion module instance to retrieve */
  /*@out@*/ char**           assert_mod,  /*!< Pointer to assertion module name and filename being retrieved */
  /*@out@*/ str_link**       cp_head,     /*!< Pointer to head of list of strings/integers containing coverage point information */
  /*@out@*/ str_link**       cp_tail      /*!< Pointer to tail of list of strings/integers containing coverage point information */
) { PROFILE(ASSERTION_GET_COVERAGE);

  /* Find functional unit */
  *cp_head = *cp_tail = NULL;

  /* If OVL assertion coverage is needed, get this information */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_get_coverage( funit, inst_name, assert_mod, cp_head, cp_tail );
  }

  PROFILE_END;
 
}

