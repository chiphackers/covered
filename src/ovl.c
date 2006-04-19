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
 \author   Trevor Williams  (trevorw@charter.net)
 \date     04/13/2006
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>

#include "defines.h"
#include "ovl.h"
#include "util.h"
#include "link.h"
#include "instance.h"
#include "iter.h"
#include "search.h"

/*!
 Specifies the number of assertion modules that need to exist in the ovl_assertions string array.
*/
#define OVL_ASSERT_NUM 27

extern bool        flag_check_ovl_assertions;
extern funit_inst* instance_root;

/*!
 Specifies the module names of all OVL assertions that contain functional coverage task calls.
*/
char* ovl_assertions[OVL_ASSERT_NUM] = { "assert_change",      "assert_cycle_sequence", "assert_decrement",     "assert_delta",
                                         "assert_even_parity", "assert_fifo_index",     "assert_frame",         "assert_handshake",
                                         "assert_implication", "assert_increment",      "assert_never_unknown", "assert_next",
                                         "assert_no_overflow", "assert_no_transition",  "assert_no_underflow",  "assert_odd_parity",
                                         "assert_one_cold",    "assert_one_hot",        "assert_range",         "assert_time",
                                         "assert_transition",  "assert_unchange",       "assert_width",         "assert_win_change",
                                         "assert_window",      "assert_win_unchange",   "assert_zero_one_hot" };

/*!
 \param name  Name of assertion module to check

 \return Returns TRUE if the specified name refers to a supported OVL coverage module; otherwise,
         returns FALSE.
*/
bool ovl_is_assertion_name( char* name ) {

  int i = OVL_ASSERT_NUM;  /* Loop iterator */

  /* Rule out the possibility if the first 7 characters does not start with "assert" */
  if( strncmp( name, "assert_", 7 ) == 0 ) {

    i = 0;
    while( (i < OVL_ASSERT_NUM) && (strncmp( (name + 7), (ovl_assertions[i] + 7), strlen( ovl_assertions[i] + 7 ) ) != 0) ) {
      i++;
    }

  }

  return( i < OVL_ASSERT_NUM );

}

/*!
 \param funit  Pointer to the functional unit to check

 \return Returns TRUE if the specified module name is a supported OVL assertion; otherwise,
         returns FALSE.
*/
bool ovl_is_assertion_module( func_unit* funit ) {

  bool        retval = FALSE;  /* Return value for this function */
  funit_link* funitl;          /* Pointer to current functional unit link */

  assert( funit != NULL );

  if( ovl_is_assertion_name( funit->name ) ) {

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

  return( retval );

}

/*!
 \param rm_tasks  Removes extraneous tasks only that are not used for coverage

 Adds all OVL assertions to the no_score list.
*/
void ovl_add_assertions_to_no_score_list( bool rm_tasks ) {

  int  i;          /* Loop iterator */
  char tmp[4096];  /* Temporary string holder */

  for( i=0; i<OVL_ASSERT_NUM; i++ ) {
    if( rm_tasks ) {
      snprintf( tmp, 4096, "%s.ovl_error_t", ovl_assertions[i] );
      search_add_no_score_funit( tmp );
      snprintf( tmp, 4096, "%s.ovl_finish_t", ovl_assertions[i] );
      search_add_no_score_funit( tmp );
      snprintf( tmp, 4096, "%s.ovl_init_msg_t", ovl_assertions[i] );
      search_add_no_score_funit( tmp );
    } else {
      search_add_no_score_funit( ovl_assertions[i] );
    }
  }

}

/*!
 \param funit  Pointer to functional unit to gather OVL assertion coverage information for
 \param total  Pointer to the total number of assertions in the given module
 \param hit    Pointer to the number of assertions hit in the given module during simulation

 Gathers the total and hit assertion coverage information for the specified functional unit and
 stores this information in the total and hit pointers.
*/
void ovl_get_funit_stats( func_unit* funit, float* total, int* hit ) {

  funit_inst* funiti;      /* Pointer to found functional unit instance containing this functional unit */
  funit_inst* curr_child;  /* Current child of this functional unit's instance */
  int         ignore = 0;  /* Number of functional units to ignore */
  stmt_iter   si;          /* Statement iterator */

  /* If the current functional unit is not an OVL module, get the statistics */
  if( !ovl_is_assertion_module( funit ) ) {

    /* Get one instance of this module from the design */
    funiti = instance_find_by_funit( instance_root, funit, &ignore );
    assert( funiti != NULL );

    /* Find all child instances of this module that are assertion modules */
    curr_child = funiti->child_head;
    while( curr_child != NULL ) {

      /* If this child instance module type is an assertion module, get its assertion information */
      if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

        stmt_iter_reset( &si, curr_child->funit->stmt_head );
        while( si.curr != NULL ) {

          /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
          if( (si.curr->stmt->exp->op == EXP_OP_TASK_CALL) && (strcmp( si.curr->stmt->exp->name, "ovl_cover_t" ) == 0) ) {
            *total = *total + 1;
            if( si.curr->stmt->exp->exec_num > 0 ) {
              (*hit)++;
            }
          }

          stmt_iter_next( &si );

        }

      }

      /* Advance child pointer to next child instance */
      curr_child = curr_child->next;

    }

  }

}

/*
 $Log$
 Revision 1.1  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

*/

