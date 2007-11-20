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
#include "util.h"


extern inst_link*  inst_head;
extern funit_link* funit_head;
extern bool        report_covered;
extern bool        report_instance;
extern char**      leading_hierarchies;
extern int         leading_hier_num;
extern bool        leading_hiers_differ;
extern isuppl      info_suppl;


/*!
 \param arg  Command-line argument specified in -A option to score command to parse

 Parses the specified command-line argument as an assertion to consider for coverage.
*/
void assertion_parse( const char* arg ) {

}

/*!
 \param ap     Pointer to attribute to parse
 \param funit  Pointer to current functional unit containing this attribute

 Parses the specified assertion attribute for assertion coverage details.
*/
void assertion_parse_attr( attr_param* ap, const func_unit* funit ) {

}

/*!
 \param funit  Pointer to current functional unit
 \param total  Pointer to the total number of assertions found in this functional unit
 \param hit    Pointer to the total number of hit assertions in this functional unit

 Gets total and hit assertion coverage statistics for the given functional unit.
*/
void assertion_get_stats( const func_unit* funit, float* total, int* hit ) {

  assert( funit != NULL );

  /* Initialize total and hit values */
  *total = 0;
  *hit   = 0;

  /* If OVL assertion coverage is needed, check this functional unit */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_get_funit_stats( funit, total, hit );
  }

}

/*!
 \param ofile  Pointer to file handle to output instance summary results to
 \param name   Name of instance being reported
 \param hits   Total number of assertions hit in the given instance
 \param total  Total number of assertions in the given instance

 \return Returns TRUE if at least one assertion was found to be not hit; otherwise, returns FALSE.

 Displays the assertion summary information for a given instance to the specified output stream.
*/
bool assertion_display_instance_summary( FILE* ofile, const char* name, int hits, float total ) {

  float percent;  /* Percentage of assertions hit */
  float miss;     /* Number of assertions missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-43.43s    %5d/%5.0f/%5.0f      %3.0f%%\n", name, hits, miss, total, percent );

  return( miss > 0 );

}

/*!
 \param ofile        Pointer to the file to output the instance summary information to
 \param root         Pointer to the current functional unit instance to look at
 \param parent_inst  Scope of parent instance of this instance

 \return Returns TRUE if at least one assertion was found to not be covered in this
         function unit instance; otherwise, returns FALSE.

 Outputs the instance summary assertion coverage information for the given functional
 unit instance to the given output file.
*/
bool assertion_instance_summary( FILE* ofile, const funit_inst* root, const char* parent_inst, int* hits, float* total ) {

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary holder of instance name */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if assertion was missed */

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
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname );

  if( root->stat->show && !funit_is_unnamed( root->funit ) &&
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

  return( miss_found );

}

/*!
 \param ofile  Pointer to file handle to output instance summary results to
 \param name   Name of functional unit being reported
 \param fname  Name of file containing the given functional unit
 \param hits   Total number of assertions hit in the given functional unit
 \param total  Total number of assertions in the given functional unit

 \return Returns TRUE if at least one assertion was found to be not hit; otherwise, returns FALSE.

 Displays the assertion summary information for a given instance to the specified output stream.
*/
bool assertion_display_funit_summary( FILE* ofile, const char* name, const char* fname, int hits, float total ) {

  float percent;  /* Percentage of assertions hit */
  float miss;     /* Number of assertions missed */

  calc_miss_percent( hits, total, &miss, &percent );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5.0f/%5.0f      %3.0f%%\n",
           name, fname, hits, miss, total, percent );

  return( miss > 0 );

}

/*!
 \param ofile  Pointer to output file to display summary information to
 \param head   Pointer to the current functional unit link to evaluate

 \return Returns TRUE if at least one assertion was found to not be covered in this
         functional unit; otherwise, returns FALSE.

 Outputs the functional unit summary assertion coverage information for the given
 functional unit to the given output file.
*/
bool assertion_funit_summary( FILE* ofile, const funit_link* head, int* hits, float* total ) {

  bool  miss_found = FALSE;  /* Set to TRUE if assertion was found to be missed */
  char* pname;               /* Printable version of functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= assertion_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->filename ) ),
                                                     head->funit->stat->assert_hit, head->funit->stat->assert_total );

      /* Update accumulated information */
      *hits  += head->funit->stat->assert_hit;
      *total += head->funit->stat->assert_total;

      free_safe( pname );

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile  Pointer to the file to output the verbose information to
 \param funit  Pointer to the functional unit to display

 Displays the verbose hit/miss assertion information for the given functional unit.
*/
void assertion_display_verbose( FILE* ofile, const func_unit* funit ) {

  if( report_covered ) {
    fprintf( ofile, "    Hit Assertions\n\n" );
  } else {
    fprintf( ofile, "    Missed Assertions\n\n" );
  }

  /* If OVL assertion coverage is needed, output it in the OVL style */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_display_verbose( ofile, funit );
  }

  fprintf( ofile, "\n" );

}

/*!
 \param ofile        Pointer to the file to output the instance verbose information to
 \param root         Pointer to the current functional unit instance to look at
 \param parent_inst  Scope of parent of this functional unit instance

 Outputs the instance verbose assertion coverage information for the given functional
 unit instance to the given output file.
*/
void assertion_instance_verbose( FILE* ofile, funit_inst* root, char* parent_inst ) {

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
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname );

  if( !funit_is_unnamed( root->funit ) &&
      (((root->stat->assert_hit < root->stat->assert_total) && !report_covered) ||
       ((root->stat->assert_hit > 0) && report_covered)) ) {

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

    free_safe( pname );

    assertion_display_verbose( ofile, root->funit );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    assertion_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

}

/*!
 \param ofile  Pointer to the file to output the functional unit verbose information to
 \param head   Pointer to the current functional unit link to look at

 Outputs the functional unit verbose assertion coverage information for the given
 functional unit to the given output file.
*/
void assertion_funit_verbose( FILE* ofile, const funit_link* head ) {

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        (((head->funit->stat->assert_hit < head->funit->stat->assert_total) && !report_covered) ||
         ((head->funit->stat->assert_hit > 0) && report_covered)) ) {

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

      free_safe( pname );

      assertion_display_verbose( ofile, head->funit );

    }

    head = head->next;

  }

}

/*!
 \param ofile    File to output report contents to
 \param verbose  Specifies if verbose report output needs to be generated

 Outputs assertion coverage report information to the given file handle. 
*/
void assertion_report( FILE* ofile, bool verbose ) {

  bool       missed_found = FALSE;  /* If set to TRUE, lines were found to be missed */
  char       tmp[4096];             /* Temporary string value */
  inst_link* instl;                 /* Pointer to current instance link */
  int        acc_hits     = 0;      /* Total number of assertions hit */
  float      acc_total    = 0;      /* Total number of assertions in design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   ASSERTION COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
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

    instl = inst_head;
    while( instl != NULL ) {
      missed_found |= assertion_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*"), &acc_hits, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    assertion_display_instance_summary( ofile, "Accumulated", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = inst_head;
      while( instl != NULL ) {
        assertion_instance_verbose( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
        instl = instl->next;
      }

    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = assertion_funit_summary( ofile, funit_head, &acc_hits, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    assertion_display_funit_summary( ofile, "Accumulated", "", acc_hits, acc_total );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      assertion_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}

/*!
 \param funit_name  Name of functional unit to search for
 \param funit_type  Type of functional unit to search for
 \param total       Pointer to the total number of assertions in the specified functional unit
 \param hit         Pointer to the number of hit assertions in the specified functional unit
 
 \return Returns TRUE if the specified functional unit was found in the design; otherwise, returns FALSE.
 
 Counts the total number and number of hit assertions for the specified functional unit.
*/
bool assertion_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit ) {
	
  bool        retval = TRUE;  /* Return value for this function */
  func_unit   funit;          /* Functional unit used for searching */
  funit_link* funitl;         /* Pointer to found functional unit link */
  float       ftotal;         /* Float version of total */
  
  funit.name = funit_name;
  funit.type = funit_type;
  
  /* Initialize total and hit counts */
  ftotal = 0;
  *hit   = 0;
  
  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {
    
    if( info_suppl.part.assert_ovl == 1 ) {
      ovl_get_funit_stats( funitl->funit, &ftotal, hit );
    }
    
  } else {
    
    retval = FALSE;
    
  }
  
  /* Convert the floating point value to an integer value */
  *total = (int)ftotal;
  
  return( retval );
  
}

/*!
 \param funit_name        Name of functional unit to collect assertion information for
 \param funit_type        Type of functional unit to collect assertion information for
 \param uncov_inst_names  Pointer to array of uncovered instance names found within the specified functional unit
 \param excludes          Pointer to array of integers that contain the exclude information for the given instance names
 \param uncov_inst_size   Number of valid elements in the uncov_inst_names array
 \param cov_inst_names    Pointer to array of covered instance names found within the specified functional unit
 \param cov_inst_size     Number of valid elements in the cov_inst_names array
 
 \return Returns TRUE if the specified functional unit exists in the design; otherwise, returns FALSE.
 
 Searches the specified functional unit, collecting all uncovered and covered assertion module instance names.
*/
bool assertion_collect( const char* funit_name, int funit_type, char*** uncov_inst_names, int** excludes, int* uncov_inst_size,
                        char*** cov_inst_names, int* cov_inst_size ) {
  
  bool        retval = TRUE;  /* Return value for this function */
  func_unit   funit;          /* Temporary functional unit used for searching */
  funit_link* funitl;         /* Pointer to found functional unit */
  
  /* Initialize functional unit to search for */
  funit.name = strdup_safe( funit_name, __FILE__, __LINE__ );
  funit.type = funit_type;
  
  /* Find functional unit */
  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {
    
    /* Initialize outputs */
    *uncov_inst_names = NULL;
    *excludes         = NULL;
    *uncov_inst_size  = 0;
    *cov_inst_names   = NULL;
    *cov_inst_size    = 0;
    
    /* If OVL assertion coverage is needed, get this information */
    if( info_suppl.part.assert_ovl == 1 ) {
      ovl_collect( funitl->funit, uncov_inst_names, excludes, uncov_inst_size, cov_inst_names, cov_inst_size );
    }
    
  } else {
    
    retval = FALSE;
    
  }

  free_safe( funit.name );
  
  return( retval );
  
}

/*!
 \param funit_name  Name of functional unit to retrieve missed coverage points for
 \param funit_type  Type of functional unit to retrieve missed coverage points for
 \param inst_name   Name of assertion module instance to retrieve
 \param assert_mod  Pointer to name of assertion module being retrieved
 \param cp_head     Pointer to head of list of strings/integers containing coverage point information
 \param cp_head     Pointer to tail of list of strings/integers containing coverage point information

 \return Returns TRUE if the specified functional unit was found; otherwise, returns FALSE.

 Finds all of the coverage points for the given assertion instance and stores their
 string descriptions and execution counts in the cp list.
*/
bool assertion_get_coverage( const char* funit_name, int funit_type, const char* inst_name, char** assert_mod, str_link** cp_head, str_link** cp_tail ) {

  bool        retval = TRUE;  /* Return value for this function */
  func_unit   funit;          /* Temporary functional unit used for searching */
  funit_link* funitl;         /* Pointer to found functional unit link */

  funit.name = strdup_safe( funit_name, __FILE__, __LINE__ );
  funit.type = funit_type;

  /* Find functional unit */
  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    *cp_head = *cp_tail = NULL;

    /* If OVL assertion coverage is needed, get this information */
    if( info_suppl.part.assert_ovl == 1 ) {
      ovl_get_coverage( funitl->funit, inst_name, assert_mod, cp_head, cp_tail );
    }

  } else {

    retval = FALSE;

  }
 
  free_safe( funit.name );

  return( retval );

}

/*
 $Log$
 Revision 1.21  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.20  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.19  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.18  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.17  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.16  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.15  2006/10/12 22:48:45  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.14  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.13  2006/09/01 04:06:36  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.12  2006/08/18 22:07:44  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.11  2006/06/29 20:06:32  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.10  2006/06/23 21:43:53  phase1geo
 More updates to include toggle exclusion (this does not work correctly at
 this time).

 Revision 1.9  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.8  2006/05/01 13:19:05  phase1geo
 Enhancing the verbose assertion window.  Still need to fix a few bugs and add
 a few more enhancements.

 Revision 1.7  2006/04/29 05:12:14  phase1geo
 Adding initial version of assertion verbose window.  This is currently working; however,
 I think that I want to enhance this window a bit more before calling it good.

 Revision 1.6  2006/04/28 17:10:19  phase1geo
 Adding GUI support for assertion coverage.  Halfway there.

 Revision 1.5  2006/04/21 22:03:58  phase1geo
 Adding ovl1 and ovl1.1 diagnostics to testsuite.  ovl1 passes while ovl1.1
 currently fails due to a problem with outputting parameters to the CDD file
 (need to look into this further).  Added OVL assertion verbose output support
 which seems to be working for the moment.

 Revision 1.4  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.3  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.2  2006/04/06 22:30:03  phase1geo
 Adding VPI capability back and integrating into autoconf/automake scheme.  We
 are getting close but still have a ways to go.

 Revision 1.1  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

*/

