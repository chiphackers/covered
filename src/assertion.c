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
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/4/2006
*/


#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "assertion.h"
#include "ovl.h"
#include "util.h"


extern funit_inst* instance_root;
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
void assertion_parse( char* arg ) {

}

/*!
 \param ap     Pointer to attribute to parse
 \param funit  Pointer to current functional unit containing this attribute

 Parses the specified assertion attribute for assertion coverage details.
*/
void assertion_parse_attr( attr_param* ap, func_unit* funit ) {

}

/*!
 \param funit  Pointer to current functional unit
 \param total  Pointer to the total number of assertions found in this functional unit
 \param hit    Pointer to the total number of hit assertions in this functional unit

 Gets total and hit assertion coverage statistics for the given functional unit.
*/
void assertion_get_stats( func_unit* funit, float* total, int* hit ) {

  assert( funit != NULL );

  /* Initialize total and hit values */
  total = 0;
  hit   = 0;

  /* If OVL assertion coverage is needed, check this functional unit */
  if( info_suppl.part.assert_ovl == 1 ) {
    ovl_get_funit_stats( funit, total, hit );
  }

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
bool assertion_instance_summary( FILE* ofile, funit_inst* root, char* parent_inst ) {

  funit_inst* curr;           /* Pointer to current child functional unit instance of this node */
  float       percent;        /* Percentage of assertions hit */
  float       miss;           /* Number of assertions missed */
  char        tmpname[4096];  /* Temporary holder of instance name */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );
  assert( root->stat != NULL );

  if( root->stat->assert_total == 0 ) {
    percent = 100.0;
  } else {
    percent = ((root->stat->assert_hit / root->stat->assert_total) * 100);
  }

  miss = (root->stat->assert_total - root->stat->assert_hit);

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    /* Get printable version of the instance name */
    pname = scope_gen_printable( root->name );

    /* Calculate instance name */
    if( strcmp( parent_inst, "*" ) == 0 ) {
      strcpy( tmpname, pname );
    } else {
      snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    }

    free_safe( pname );

    fprintf( ofile, "  %-43.43s    %5d/%5.0f/%5.0f      %3.0f%%\n",
             tmpname,
             root->stat->assert_hit,
             miss,
             root->stat->assert_total,
             percent );

    curr = root->child_head;
    while( curr != NULL ) {
      miss = miss + assertion_instance_summary( ofile, curr, tmpname );
      curr = curr->next;
    }

  }

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
bool assertion_funit_summary( FILE* ofile, funit_link* head ) {

  float percent;             /* Percentage of assertions hit */
  float miss;                /* Number of assertions missed */
  bool  miss_found = FALSE;  /* Set to TRUE if assertion was found to be missed */
  char* pname;               /* Printable version of functional unit name */

  while( head != NULL ) {

    if( head->funit->stat->assert_total == 0 ) {
      percent = 100.0;
    } else {
      percent = ((head->funit->stat->assert_hit / head->funit->stat->assert_total) * 100);
    }

    miss       = (head->funit->stat->assert_total - head->funit->stat->assert_hit);
    miss_found = (miss > 0) ? TRUE : miss_found;

    /* If this is an assertion module, don't output any further */
    if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit ) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( head->funit->name );

      fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5.0f/%5.0f      %3.0f%%\n",
               pname,
               get_basename( head->funit->filename ),
               head->funit->stat->assert_hit,
               miss,
               head->funit->stat->assert_total,
               percent );

      free_safe( pname );

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile  Pointer to the file to output the instance verbose information to
 \param root   Pointer to the current functional unit instance to look at
 \param scope  Current scope of this functional unit instance

 Outputs the instance verbose assertion coverage information for the given functional
 unit instance to the given output file.
*/
void assertion_instance_verbose( FILE* ofile, funit_inst* root, char* scope ) {

  /* TBD */

}

/*!
 \param ofile  Pointer to the file to output the functional unit verbose information to
 \param head   Pointer to the current functional unit link to look at

 Outputs the functional unit verbose assertion coverage information for the given
 functional unit to the given output file.
*/
void assertion_funit_verbose( FILE* ofile, funit_link* head ) {

  /* TBD */

}

/*!
 \param ofile    File to output report contents to
 \param verbose  Specifies if verbose report output needs to be generated

 Outputs assertion coverage report information to the given file handle. 
*/
void assertion_report( FILE* ofile, bool verbose ) {

  bool missed_found;  /* If set to TRUE, lines were found to be missed */
  char tmp[4096];     /* Temporary string value */

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

    missed_found = assertion_instance_summary( ofile, instance_root, tmp );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      assertion_instance_verbose( ofile, instance_root, tmp );
    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = assertion_funit_summary( ofile, funit_head );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      assertion_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );


}

/*
 $Log$
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

