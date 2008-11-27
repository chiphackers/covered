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
 \file     ovl.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     04/13/2006
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "exclude.h"
#include "func_iter.h"
#include "func_unit.h"
#include "instance.h"
#include "iter.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "report.h"
#include "search.h"
#include "util.h"
#include "vector.h"


/*!
 Specifies the number of assertion modules that need to exist in the ovl_assertions string array.
*/
#define OVL_ASSERT_NUM 27


extern db**         db_list;
extern unsigned int curr_db;
extern bool         flag_check_ovl_assertions;
extern bool         report_covered;
extern bool         report_instance;
extern bool         flag_output_exclusion_ids;


/*!
 Specifies the module names of all OVL assertions that contain functional coverage task calls.
*/
static char* ovl_assertions[OVL_ASSERT_NUM] = { "assert_change",      "assert_cycle_sequence", "assert_decrement",     "assert_delta",
                                                "assert_even_parity", "assert_fifo_index",     "assert_frame",         "assert_handshake",
                                                "assert_implication", "assert_increment",      "assert_never_unknown", "assert_next",
                                                "assert_no_overflow", "assert_no_transition",  "assert_no_underflow",  "assert_odd_parity",
                                                "assert_one_cold",    "assert_one_hot",        "assert_range",         "assert_time",
                                                "assert_transition",  "assert_unchange",       "assert_width",         "assert_win_change",
                                                "assert_window",      "assert_win_unchange",   "assert_zero_one_hot" };

/*!
 \return Returns TRUE if the specified name refers to a supported OVL coverage module; otherwise,
         returns FALSE.
*/
static bool ovl_is_assertion_name(
  const char* name  /*!< Name of assertion module to check */
) { PROFILE(OVL_IS_ASSERTION_NAME);

  int i = OVL_ASSERT_NUM;  /* Loop iterator */

  /* Rule out the possibility if the first 7 characters does not start with "assert" */
  if( strncmp( name, "assert_", 7 ) == 0 ) {

    i = 0;
    while( (i < OVL_ASSERT_NUM) && (strncmp( (name + 7), (ovl_assertions[i] + 7), strlen( ovl_assertions[i] + 7 ) ) != 0) ) {
      i++;
    }

  }

  PROFILE_END;

  return( i < OVL_ASSERT_NUM );

}

/*!
 \return Returns TRUE if the specified module name is a supported OVL assertion; otherwise,
         returns FALSE.
*/
bool ovl_is_assertion_module(
  const func_unit* funit  /*!< Pointer to the functional unit to check */
) { PROFILE(OVL_IS_ASSERTION_MODULE);

  bool        retval = FALSE;  /* Return value for this function */
  funit_link* funitl;          /* Pointer to current functional unit link */

  if( (funit != NULL) && ovl_is_assertion_name( funit->name ) ) {

    /*
     If the module matches in name, check to see if the ovl_cover_t task exists (this
     will tell us if coverage was enabled for this module.
    */
    funitl = funit->tf_head;
    while( (funitl != NULL) && ((strcmp( funitl->funit->name, "ovl_cover_t" ) != 0) || (funitl->funit->type != FUNIT_TASK)) ) {
      funitl = funitl->next;
    }
    retval = (funitl == NULL);

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the specifies expression corresponds to a coverage point; otherwise, returns FALSE.
*/
bool ovl_is_coverage_point(
  const expression* exp  /*!< Pointer to expression to check */
) { PROFILE(OVL_IS_COVERAGE_POINT);

  bool retval = (exp->op == EXP_OP_TASK_CALL) && (strcmp( exp->name, "ovl_cover_t" ) == 0);

  PROFILE_END;

  return( retval );

}

/*!
 \throws anonymous search_add_no_score_funit search_add_no_score_funit search_add_no_score_funit search_add_no_score_funit

 Adds all OVL assertions to the no_score list.
*/
void ovl_add_assertions_to_no_score_list(
  bool rm_tasks  /*!< Removes extraneous tasks only that are not used for coverage */
) { PROFILE(OVL_ADD_ASSERTIONS_TO_NO_SCORE_LIST);

  int  i;          /* Loop iterator */
  char tmp[4096];  /* Temporary string holder */

  for( i=0; i<OVL_ASSERT_NUM; i++ ) {
    if( rm_tasks ) {
      unsigned int rv;
      rv = snprintf( tmp, 4096, "%s.ovl_error_t", ovl_assertions[i] );
      assert( rv < 4096 );
      (void)search_add_no_score_funit( tmp );
      rv = snprintf( tmp, 4096, "%s.ovl_finish_t", ovl_assertions[i] );
      assert( rv < 4096 );
      (void)search_add_no_score_funit( tmp );
      rv = snprintf( tmp, 4096, "%s.ovl_init_msg_t", ovl_assertions[i] );
      assert( rv < 4096 );
      (void)search_add_no_score_funit( tmp );
    } else {
      (void)search_add_no_score_funit( ovl_assertions[i] );
    }
  }

  PROFILE_END;

}

/*!
 Gathers the total and hit assertion coverage information for the specified functional unit and
 stores this information in the total and hit pointers.
*/
void ovl_get_funit_stats(
            const func_unit* funit,     /*!< Pointer to functional unit to gather OVL assertion coverage information for */
  /*@out@*/ unsigned int*    hit,       /*!< Pointer to the number of assertions hit in the given module during simulation */
  /*@out@*/ unsigned int*    excluded,  /*!< Pointer to the number of excluded assertions */
  /*@out@*/ unsigned int*    total      /*!< Pointer to the total number of assertions in the given module */
) { PROFILE(OVL_GET_FUNIT_STATS);

  funit_inst* funiti;      /* Pointer to found functional unit instance containing this functional unit */
  funit_inst* curr_child;  /* Current child of this functional unit's instance */
  int         ignore = 0;  /* Number of functional units to ignore */
  func_iter   fi;          /* Functional unit iterator */
  statement*  stmt;        /* Pointer to current statement */

  /* If the current functional unit is not an OVL module, get the statistics */
  if( !ovl_is_assertion_module( funit ) ) {

    /* Get one instance of this module from the design */
    funiti = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore );
    assert( funiti != NULL );

    /* Find all child instances of this module that are assertion modules */
    curr_child = funiti->child_head;
    while( curr_child != NULL ) {

      /* If this child instance module type is an assertion module, get its assertion information */
      if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

        /* Initialize the functional unit iterator */
        func_iter_init( &fi, curr_child->funit, TRUE, FALSE, TRUE );

        while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

          /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
          if( ovl_is_coverage_point( stmt->exp ) ) {
            *total = *total + 1;
            if( (stmt->exp->exec_num > 0) || (ESUPPL_EXCLUDED( stmt->exp->suppl ) == 1) ) {
              (*hit)++;
              if( ESUPPL_EXCLUDED( stmt->exp->suppl ) == 1 ) {
                (*excluded)++;
              }
            }
          }

        }

        /* Deallocate functional unit iterator */
        func_iter_dealloc( &fi );

      }

      /* Advance child pointer to next child instance */
      curr_child = curr_child->next;

    }

  }

  PROFILE_END;

}

/*!
 \return Returns the string passed to the task call

 Finds the task call parameter that specifies the name of the coverage point and returns this
 string value to the calling function.
*/
static char* ovl_get_coverage_point(
  statement* stmt  /*!< Pointer to statement of task call to coverage task */
) { PROFILE(OVL_GET_COVERAGE_POINT);

  char* cpoint;  /* Return value for this function */

  /*
   We are going to make a lot of assumptions about the structure of the statement, so just
   do a lot of assertions to make sure that our assumptions are correct.
  */
  assert( stmt != NULL );
  assert( stmt->exp != NULL );
  assert( stmt->exp->left != NULL );
  assert( stmt->exp->left->op == EXP_OP_PASSIGN );
  assert( stmt->exp->left->right != NULL );
  assert( stmt->exp->left->right->op == EXP_OP_STATIC );
  assert( ESUPPL_STATIC_BASE( stmt->exp->left->right->suppl ) == QSTRING );

  cpoint = vector_to_string( stmt->exp->left->right->value, ESUPPL_STATIC_BASE( stmt->exp->left->right->suppl ), FALSE );

  PROFILE_END;

  return( cpoint );

}

/*!
 Displays the verbose hit/miss information to the given output file for the given functional
 unit.
*/
void ovl_display_verbose(
  FILE*            ofile,  /*!< Pointer to output file to display verbose information to */
  const func_unit* funit,  /*!< Pointer to module containing assertions that were not covered */
  rpt_type         rtype   /*!< Specifies the type of report to generate */
) { PROFILE(OVL_DISPLAY_VERBOSE);

  funit_inst*  funiti;        /* Pointer to functional unit instance found for this functional unit */
  int          ignore   = 0;  /* Specifies that we do not want to ignore any modules */
  funit_inst*  curr_child;    /* Pointer to current child instance of this module */
  char*        cov_point;     /* Pointer to name of current coverage point */
  func_iter    fi;            /* Functional unit iterator */
  statement*   stmt;          /* Pointer to current statement */
  unsigned int eid_size = 1;  /* Number of characters needed to store an exclusion ID */
  char         tmp[30];       /* Temporary string */

  fprintf( ofile, "      " );
  if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) ) {
    eid_size = db_get_exclusion_id_size();
    gen_char_string( tmp, ' ', (eid_size - 2) );
    fprintf( ofile, "EID%s  ", tmp );
  }
  fprintf( ofile, "Instance Name               Assertion Name          Coverage Point" );
  if( rtype == RPT_TYPE_HIT ) {
    fprintf( ofile, "                            # of hits" );
  }
  fprintf( ofile, "\n" );
  gen_char_string( tmp, '-', (eid_size - 1) );
  fprintf( ofile, "      %s---------------------------------------------------------------------------------------------------------\n", tmp );

  /* Get one instance of this module from the design */
  funiti = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore );
  assert( funiti != NULL );

  /* Find all child instances of this module that are assertion modules */
  curr_child = funiti->child_head;
  while( curr_child != NULL ) {

    /* If this child instance module type is an assertion module, check its assertion information */
    if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

      /* Initialize the functional unit iterator */
      func_iter_init( &fi, curr_child->funit, TRUE, FALSE, TRUE );

      while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

        if( ((((stmt->exp->exec_num > 0) ? 1 : 0) == report_covered) && (stmt->exp->suppl.part.excluded == 0) && (rtype != RPT_TYPE_EXCL)) ||
            ((stmt->exp->suppl.part.excluded == 1) && (rtype == RPT_TYPE_EXCL)) ) {

          /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
          if( ovl_is_coverage_point( stmt->exp ) ) {

            /* Get coverage point name */
            cov_point = ovl_get_coverage_point( stmt );

            /* Output the coverage verbose results to the specified output file */
            if( (stmt->exp->exec_num == 0) && (rtype != RPT_TYPE_HIT) ) {
              if( flag_output_exclusion_ids ) {
                exclude_reason* er;
                fprintf( ofile, "      (%s)  %-26s  %-22s  \"%-38s\"\n", db_gen_exclusion_id( 'A', stmt->exp->id ),
                         obf_inst( curr_child->name ), obf_funit( funit_flatten_name( curr_child->funit ) ), cov_point );
                if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'A', stmt->exp->id, curr_child->funit )) != NULL) ) {
                  report_output_exclusion_reason( ofile, (12 + (db_get_exclusion_id_size() - 1)), er->reason, TRUE );
                }
              } else {
                exclude_reason* er;
                fprintf( ofile, "      %-26s  %-22s  \"%-38s\"\n",
                         obf_inst( curr_child->name ), obf_funit( funit_flatten_name( curr_child->funit ) ), cov_point );
                if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'A', stmt->exp->id, curr_child->funit )) != NULL) ) {
                  report_output_exclusion_reason( ofile, (8 + (db_get_exclusion_id_size() - 1)), er->reason, TRUE );
                }
              }
            } else if( (stmt->exp->exec_num > 0) && (rtype == RPT_TYPE_HIT) ) {
              fprintf( ofile, "      %-26s  %-22s  \"%-38s\"  %9u\n",
                       obf_inst( curr_child->name ), obf_funit( funit_flatten_name( curr_child->funit ) ), cov_point, stmt->exp->exec_num );
            }

            /* Deallocate the coverage point */
            free_safe( cov_point, (strlen( cov_point ) + 1) );
          
          }

        }

      }

      /* Deallocate functional unit iterator */
      func_iter_dealloc( &fi );

    }

    /* Advance child pointer to next child instance */
    curr_child = curr_child->next;

  }

  PROFILE_END;

}

/*!
 Populates the uncovered or covered string arrays with the instance names of child modules that match the
 respective coverage level.
*/
void ovl_collect(
            func_unit*    funit,       /*!< Pointer to functional unit to gather uncovered/covered assertion instances from */
            int           cov,         /*!< Specifies if uncovered (0) or covered (1) assertions should be gathered */
  /*@out@*/ char***       inst_names,  /*!< Pointer to array of uncovered instance names in the specified functional unit */
  /*@out@*/ int**         excludes,    /*!< Pointer to array of integers indicating exclusion of associated uncovered instance name */
  /*@out@*/ unsigned int* inst_size    /*!< Number of valid elements in the uncov_inst_names array */
) { PROFILE(OVL_COLLECT);
  
  funit_inst*  funiti;             /* Pointer to found functional unit instance containing this functional unit */
  funit_inst*  curr_child;         /* Current child of this functional unit's instance */
  int          ignore        = 0;  /* Number of functional units to ignore */
  stmt_iter    si;                 /* Statement iterator */
  unsigned int total         = 0;  /* Total number of coverage points for a given assertion module */
  unsigned int hit           = 0;  /* Number of hit coverage points for a given assertion module */
  int          exclude_found = 0;  /* Set to a value of 1 if at least one excluded coverage point was found */

  /* Get one instance of this module from the design */
  funiti = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore );
  assert( funiti != NULL );

  /* Find all child instances of this module that are assertion modules */
  curr_child = funiti->child_head;
  while( curr_child != NULL ) {

    /* If this child instance module type is an assertion module, get its assertion information */
    if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

      func_iter  fi;
      statement* stmt;

      /* Initialize the total and hit values */
      total = 0;
      hit   = 0;

      /* Initialize the functional unit iterator */
      func_iter_init( &fi, curr_child->funit, TRUE, FALSE, TRUE );

      while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

        /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
        if( ovl_is_coverage_point( stmt->exp ) ) {
          total = total + 1;
          if( (stmt->exp->exec_num > 0) || (ESUPPL_EXCLUDED( stmt->exp->suppl ) == 1) ) {
            hit++;
            exclude_found |= ESUPPL_EXCLUDED( stmt->exp->suppl );
          }
        }

      }

      /* Deallocate functional unit iterator */
      func_iter_dealloc( &fi );

      /* If there are uncovered coverage points, add this instance to the uncov array */
      if( (cov == 0) && (hit < total) ) {
        *inst_names = (char**)realloc_safe( *inst_names, (sizeof( char** ) * (*inst_size)), (sizeof( char** ) * (*inst_size + 1)) );
        *excludes   = (int*)  realloc_safe( *excludes,   (sizeof( int )    * (*inst_size)), (sizeof( int )    * (*inst_size + 1)) );
        (*inst_names)[*inst_size] = strdup_safe( curr_child->name );
        (*excludes)[*inst_size]   = 0;
        (*inst_size)++;
      
      /* Otherwise, populate the cov array */
      } else {
        if( (cov == 0) && (exclude_found == 1) ) {
          *inst_names = (char**)realloc_safe( *inst_names, (sizeof( char** ) * (*inst_size)), (sizeof( char** ) * (*inst_size + 1)) );
          *excludes   = (int*)  realloc_safe( *excludes,   (sizeof( int )    * (*inst_size)), (sizeof( int )    * (*inst_size + 1)) );
          (*inst_names)[*inst_size] = strdup_safe( curr_child->name );
          (*excludes)[*inst_size]   = 1;
          (*inst_size)++;
        } else if( cov == 1 ) {
          *inst_names = (char**)realloc_safe( *inst_names, (sizeof( char** ) * (*inst_size)), (sizeof( char** ) * (*inst_size + 1)) );
          (*inst_names)[*inst_size] = strdup_safe( curr_child->name );
          (*inst_size)++;
        }
      }

    }

    /* Advance child pointer to next child instance */
    curr_child = curr_child->next;

  }

  PROFILE_END;

}

/*!
 Retrieves the coverage point strings and execution counts from the specified assertion module.
*/
void ovl_get_coverage(
            const func_unit* funit,       /*!< Pointer to functional unit to get assertion information from */
            const char*      inst_name,   /*!< Name of assertion instance to get coverage points from */
  /*@out@*/ char**           assert_mod,  /*!< Pointer to the assertion module name and filename of instance being retrieved */
  /*@out@*/ str_link**       cp_head,     /*!< Pointer to head of coverage point list */
  /*@out@*/ str_link**       cp_tail      /*!< Pointer to tail of coverage point list */
) { PROFILE(OVL_GET_COVERAGE);

  funit_inst*  funiti;      /* Pointer to found functional unit instance */
  funit_inst*  curr_child;  /* Pointer to current child functional instance */
  int          ignore = 0;  /* Number of functional units to ignore */
  func_iter    fi;          /* Functional unit iterator to iterate through statements */
  statement*   stmt;        /* Pointer to current statement */
  int          str_size;    /* Size of assert_mod string needed for memory allocation */
  unsigned int rv;          /* Return value */

  /* Get one instance of this module from the design */
  funiti = inst_link_find_by_funit( funit, db_list[curr_db]->inst_head, &ignore );
  assert( funiti != NULL );

  /* Find child instance */
  curr_child = funiti->child_head;
  while( (curr_child != NULL) && (strcmp( curr_child->name, inst_name ) != 0) ) {
    curr_child = curr_child->next;
  }
  assert( curr_child != NULL );
  
  /* Get the module name and store it in assert_mod */
  str_size    = strlen( curr_child->funit->name ) + 1 + strlen( curr_child->funit->filename ) + 1;
  *assert_mod = (char*)malloc_safe( str_size ); 
  rv = snprintf( *assert_mod, str_size, "%s %s", curr_child->funit->name, curr_child->funit->filename );
  assert( rv < str_size );

  /* Initialize the functional unit iterator */
  func_iter_init( &fi, curr_child->funit, TRUE, FALSE, TRUE );

  /* Gather all missed coverage points */
  while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

    /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
    if( ovl_is_coverage_point( stmt->exp ) ) {

      exclude_reason* er;

      /* Store the coverage point string and execution count */
      (void)str_link_add( ovl_get_coverage_point( stmt ), cp_head, cp_tail );
      (*cp_tail)->suppl  = stmt->exp->exec_num;
      (*cp_tail)->suppl2 = stmt->exp->id;
      (*cp_tail)->suppl3 = ESUPPL_EXCLUDED( stmt->exp->suppl );

      /* If the toggle is currently excluded, check to see if there's a reason associated with it */
      if( (ESUPPL_EXCLUDED( stmt->exp->suppl ) == 1) && ((er = exclude_find_exclude_reason( 'A', stmt->exp->id, curr_child->funit )) != NULL) ) {
        (*cp_tail)->str2 = strdup_safe( er->reason );
      } else {
        (*cp_tail)->str2 = NULL;
      }

    }

  }

  /* Deallocate functional unit iterator */
  func_iter_dealloc( &fi );

  PROFILE_END;

  PROFILE_END;

}


/*
 $Log$
 Revision 1.38  2008/09/15 03:43:49  phase1geo
 Cleaning up splint warnings.

 Revision 1.37  2008/09/06 05:59:45  phase1geo
 Adding assertion exclusion reason support and have most code implemented for
 FSM exclusion reason support (still working on debugging this code).  I believe
 that assertions, FSMs and lines might suffer from the same problem...
 Checkpointing.

 Revision 1.36  2008/09/04 04:15:10  phase1geo
 Adding -p option to exclude command.  Updating other files per this change.
 Checkpointing.

 Revision 1.35  2008/09/03 03:46:37  phase1geo
 Updates for memory and assertion exclusion output.  Checkpointing.

 Revision 1.34  2008/09/02 05:20:41  phase1geo
 More updates for exclude command.  Updates to CVER regression.

 Revision 1.33  2008/08/29 13:01:17  phase1geo
 Removing exclusion ID from covered coverage points.  Checkpointing.

 Revision 1.32  2008/08/28 21:24:15  phase1geo
 Adding support for exclusion output for assertions.  Updated regressions accordingly.
 Checkpointing.

 Revision 1.31  2008/08/22 20:56:35  phase1geo
 Starting to make updates for proper unnamed scope report handling (fix for bug 2054686).
 Not complete yet.  Also making updates to documentation.  Checkpointing.

 Revision 1.30  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.27.2.3  2008/08/07 06:39:11  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.27.2.2  2008/08/06 20:11:34  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.27.2.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.28  2008/06/27 14:02:03  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.27  2008/05/30 05:38:31  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.26.2.1  2008/05/07 21:09:10  phase1geo
 Added functionality to allow to_string to output full vector bits (even
 non-significant bits) for purposes of reporting for FSMs (matches original
 behavior).

 Revision 1.26  2008/04/15 20:37:11  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.25  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.24  2008/03/18 21:36:24  phase1geo
 Updates from regression runs.  Regressions still do not completely pass at
 this point.  Checkpointing.

 Revision 1.23  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.22  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.21  2008/01/16 23:10:31  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.20  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.19  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.18  2008/01/09 23:54:15  phase1geo
 More splint updates.

 Revision 1.17  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.16  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.15  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.14  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.13  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.12  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.11  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.10  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.9  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.8  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

 Revision 1.7  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.6  2006/05/01 13:19:07  phase1geo
 Enhancing the verbose assertion window.  Still need to fix a few bugs and add
 a few more enhancements.

 Revision 1.5  2006/04/29 05:12:14  phase1geo
 Adding initial version of assertion verbose window.  This is currently working; however,
 I think that I want to enhance this window a bit more before calling it good.

 Revision 1.4  2006/04/28 17:10:19  phase1geo
 Adding GUI support for assertion coverage.  Halfway there.

 Revision 1.3  2006/04/21 22:03:58  phase1geo
 Adding ovl1 and ovl1.1 diagnostics to testsuite.  ovl1 passes while ovl1.1
 currently fails due to a problem with outputting parameters to the CDD file
 (need to look into this further).  Added OVL assertion verbose output support
 which seems to be working for the moment.

 Revision 1.2  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.1  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

*/

