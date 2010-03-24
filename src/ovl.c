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

#include "db.h"
#include "defines.h"
#include "exclude.h"
#include "func_iter.h"
#include "func_unit.h"
#include "instance.h"
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
    while( (funitl != NULL) && ((strcmp( funitl->funit->name, "ovl_cover_t" ) != 0) || (funitl->funit->suppl.part.type != FUNIT_TASK)) ) {
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
      if( (curr_child->funit->suppl.part.type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

        /* Initialize the functional unit iterator */
        func_iter_init( &fi, curr_child->funit, TRUE, FALSE, FALSE );

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

  cpoint = vector_to_string( stmt->exp->left->right->value, ESUPPL_STATIC_BASE( stmt->exp->left->right->suppl ), FALSE, 0 );

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
    if( (curr_child->funit->suppl.part.type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

      /* Initialize the functional unit iterator */
      func_iter_init( &fi, curr_child->funit, TRUE, FALSE, FALSE );

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
    if( (curr_child->funit->suppl.part.type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

      func_iter  fi;
      statement* stmt;

      /* Initialize the total and hit values */
      total = 0;
      hit   = 0;

      /* Initialize the functional unit iterator */
      func_iter_init( &fi, curr_child->funit, TRUE, FALSE, FALSE );

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
  str_size    = strlen( curr_child->funit->name ) + 1 + strlen( curr_child->funit->orig_fname ) + 1;
  *assert_mod = (char*)malloc_safe( str_size ); 
  rv = snprintf( *assert_mod, str_size, "%s %s", curr_child->funit->name, curr_child->funit->orig_fname );
  assert( rv < str_size );

  /* Initialize the functional unit iterator */
  func_iter_init( &fi, curr_child->funit, TRUE, FALSE, FALSE );

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

