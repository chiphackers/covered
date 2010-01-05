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
 \file     toggle.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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

#include "db.h"
#include "defines.h"
#include "exclude.h"
#include "func_iter.h"
#include "func_unit.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "report.h"
#include "toggle.h"
#include "util.h"
#include "vector.h"


extern db**         db_list;
extern unsigned int curr_db;
extern bool         report_covered;
extern bool         report_instance;
extern isuppl       info_suppl;
extern bool         report_exclusions;
extern bool         flag_output_exclusion_ids;


/*!
 Searches specified expression list and signal list, gathering information 
 about toggled bits.  For each bit that is found in the design, the total
 value is incremented by one.  For each bit that toggled from a 0 to a 1,
 the value of hit01 is incremented by one.  For each bit that toggled from
 a 1 to a 0, the value of hit10 is incremented by one.
*/
void toggle_get_stats(
            func_unit*    funit,     /*!< Pointer to named functional unit to search */
  /*@out@*/ unsigned int* hit01,     /*!< Number of bits toggling from 0 to 1 during simulation */
  /*@out@*/ unsigned int* hit10,     /*!< Number of bits toggling from 1 to 0 during simulation */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded bits */
  /*@out@*/ unsigned int* total,     /*!< Total number of bits in the design/functional unit */
  /*@out@*/ bool*         cov_found  /*!< Set to TRUE if at least one signal was completely covered */
) { PROFILE(TOGGLE_GET_STATS);

  if( !funit_is_unnamed( funit ) ) {

    func_iter fi;   /* Functional unit iterator */
    vsignal*  sig;  /* Pointer to current signal */

    func_iter_init( &fi, funit, FALSE, TRUE, FALSE );
  
    /* Search signal list */
    while( (sig = func_iter_get_next_signal( &fi )) != NULL ) {
      if( (sig->suppl.part.type != SSUPPL_TYPE_PARAM)          &&
          (sig->suppl.part.type != SSUPPL_TYPE_PARAM_REAL)     &&
          (sig->suppl.part.type != SSUPPL_TYPE_ENUM)           &&
          (sig->suppl.part.type != SSUPPL_TYPE_MEM)            &&
          (sig->suppl.part.type != SSUPPL_TYPE_DECL_REAL)      &&
          (sig->suppl.part.type != SSUPPL_TYPE_DECL_SREAL)     &&
          (sig->suppl.part.type != SSUPPL_TYPE_IMPLICIT_REAL)  &&
          (sig->suppl.part.type != SSUPPL_TYPE_IMPLICIT_SREAL) &&
          (sig->suppl.part.mba == 0) ) {
        *total += sig->value->width;
        if( sig->suppl.part.excluded == 1 ) {
          *hit01    += sig->value->width;
          *hit10    += sig->value->width;
          *excluded += (sig->value->width * 2);
        } else {
          unsigned int tmp_hit01 = 0;
          unsigned int tmp_hit10 = 0;
          vector_toggle_count( sig->value, &tmp_hit01, &tmp_hit10 );
          *hit01     += tmp_hit01;
          *hit10     += tmp_hit10;
          *cov_found |= ((sig->value->width == tmp_hit01) && (sig->value->width == tmp_hit10));
        }
      }
    }

    func_iter_dealloc( &fi );

  }

  PROFILE_END;

}

/*!
 Searches the list of signals for the specified functional unit for signals that are either covered
 or uncovered.  When a signal is found that meets the requirements, signal is added to the signal list.
*/
void toggle_collect(
            func_unit*    funit,           /*!< Pointer to functional unit */
            int           cov,             /*!< Specifies to get uncovered (0) or covered (1) signals */
  /*@out@*/ vsignal***    sigs,            /*!< Pointer to head of list of covered/uncovered signals */
  /*@out@*/ unsigned int* sig_size,        /*!< Pointer to tail of list of covered/uncovered signals */
  /*@out@*/ unsigned int* sig_no_rm_index  /*!< Pointer to starting index to not removing signals */
) { PROFILE(TOGGLE_COLLECT);

  func_iter    fi;     /* Functional unit iterator */
  vsignal*     sig;    /* Pointer to current signal */
  unsigned int hit01;  /* Number of bits that toggled from 0 to 1 */
  unsigned int hit10;  /* Number of bits that toggled from 1 to 0 */
     
  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );

  while( (sig = func_iter_get_next_signal( &fi )) != NULL ) {

    hit01 = 0;
    hit10 = 0;

    if( (sig->suppl.part.type != SSUPPL_TYPE_PARAM)          &&
        (sig->suppl.part.type != SSUPPL_TYPE_PARAM_REAL)     &&
        (sig->suppl.part.type != SSUPPL_TYPE_ENUM)           &&
        (sig->suppl.part.type != SSUPPL_TYPE_MEM)            &&
        (sig->suppl.part.type != SSUPPL_TYPE_DECL_REAL)      &&
        (sig->suppl.part.type != SSUPPL_TYPE_DECL_SREAL)     &&
        (sig->suppl.part.type != SSUPPL_TYPE_IMPLICIT_REAL)  &&
        (sig->suppl.part.type != SSUPPL_TYPE_IMPLICIT_SREAL) &&
        (sig->suppl.part.mba == 0) ) {

      vector_toggle_count( sig->value, &hit01, &hit10 );

      /* If this signal meets the coverage requirement, add it to the signal list */
      if( ((cov == 1) && (hit01 == sig->value->width) && (hit10 == sig->value->width)) ||
          ((cov == 0) && ((hit01 < sig->value->width) || (hit10 < sig->value->width))) ) {

        sig_link_add( sig, TRUE, sigs, sig_size, sig_no_rm_index );
          
      }

    }

  }

  func_iter_dealloc( &fi );

  PROFILE_END;

}

/*!
 Returns toggle coverage information for a specified signal in a specified functional unit.  This
 is needed by the GUI for verbose toggle coverage display.
*/
void toggle_get_coverage(
            func_unit* funit,     /*!< Pointer to functional unit */
            char*      sig_name,  /*!< Name of signal within the specified functional unit to get toggle coverage information for */
  /*@out@*/ int*       msb,       /*!< Most-significant bit position of the requested signal */
  /*@out@*/ int*       lsb,       /*!< Least-significant bit position of the requested signal */
  /*@out@*/ char**     tog01,     /*!< Toggle vector of bits transitioning from a 0 to a 1 */
  /*@out@*/ char**     tog10,     /*!< Toggle vector of bits transitioning from a 1 to a 0 */
  /*@out@*/ int*       excluded,  /*!< Pointer to integer specifying if this signal should be excluded or not */
  /*@out@*/ char**     reason     /*!< Reason for exclusion if one exists */
) { PROFILE(TOGGLE_GET_COVERAGE);

  func_iter       fi;   /* Functional unit iterator */
  vsignal*        sig;  /* Pointer to current signal */
  exclude_reason* er;   /* Pointer to found exclude reason structure */

  /* Find the matching signal */
  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );
  while( ((sig = func_iter_get_next_signal( &fi )) != NULL) && (strcmp( sig->name, sig_name ) != 0) );
  func_iter_dealloc( &fi );

  assert( sig != NULL );
  assert( sig->dim != NULL );

  *msb      = sig->dim[0].msb;
  *lsb      = sig->dim[0].lsb; 
  *tog01    = vector_get_toggle01_ulong( sig->value->value.ul, sig->value->width );
  *tog10    = vector_get_toggle10_ulong( sig->value->value.ul, sig->value->width );
  *excluded = sig->suppl.part.excluded;

  /* If the toggle is currently excluded, check to see if there's a reason associated with it */
  if( (*excluded == 1) && ((er = exclude_find_exclude_reason( 'T', sig->id, funit )) != NULL) ) {
    *reason = strdup_safe( er->reason );
  } else {
    *reason = NULL;
  }

  PROFILE_END;

}

/*!
 Looks up summary information for specified functional unit.
*/
void toggle_get_funit_summary(
            func_unit*    funit,     /*!< Pointer to found functional unit */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to total number of toggles hit in this functional unit */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded bits */
  /*@out@*/ unsigned int* total      /*!< Pointer to total number of toggles in this functional unit */
) { PROFILE(TOGGLE_GET_FUNIT_SUMMARY);

  *hit      = (funit->stat->tog01_hit + funit->stat->tog10_hit);
  *excluded = funit->stat->tog_excluded;
  *total    = (funit->stat->tog_total * 2);
        
  PROFILE_END;

}

/*!
 Looks up summary information for specified functional unit instance.
*/
void toggle_get_inst_summary(
            funit_inst*   inst,      /*!< Pointer to found functional unit instance */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to total number of toggles hit in this functional unit instance */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded bits */
  /*@out@*/ unsigned int* total      /*!< Pointer to total number of toggles in this functional unit instance */
) { PROFILE(TOGGLE_GET_INST_SUMMARY);

  *hit      = (inst->stat->tog01_hit + inst->stat->tog10_hit);
  *excluded = inst->stat->tog_excluded;
  *total    = (inst->stat->tog_total * 2);
        
  PROFILE_END;

}

/*!
 \return Returns TRUE if at least one bit was found to not be toggled; otherwise, returns FALSE.

 Displays the toggle instance summary information to the given output file, calculating the miss and
 percentage information.
*/
static bool toggle_display_instance_summary(
  FILE* ofile,   /*!< Pointer to file to output summary information to */
  char* name,    /*!< Name of instance to output */
  int   hits01,  /*!< Number of bits that toggled from 0 -> 1 */
  int   hits10,  /*!< Number of bits that toggled from 1 -> 0 */
  int   total    /*!< Total number of bits in given instance */
) { PROFILE(TOGGLE_DISPLAY_INSTANCE_SUMMARY);

  float percent01;  /* Percentage of bits toggled from 0 -> 1 */
  float percent10;  /* Percentage of bits toggled from 1 -> 0 */
  int   miss01;     /* Number of bits not toggled from 0 -> 1 */
  int   miss10;     /* Number of bits not toggled from 1 -> 0 */

  calc_miss_percent( hits01, total, &miss01, &percent01 );
  calc_miss_percent( hits10, total, &miss10, &percent10 );

  fprintf( ofile, "  %-43.43s    %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, hits01, miss01, total, percent01, hits10, miss10, total, percent10 );

  PROFILE_END;

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \return Returns TRUE if any bits were found to be not toggled; otherwise, returns FALSE.

 Displays the toggle instance summarization to the specified file.  Recursively
 iterates through functional unit instance tree, outputting the toggle information that
 is found at that instance.
*/
static bool toggle_instance_summary(
            FILE*         ofile,        /*!< File to output coverage information to */
            funit_inst*   root,         /*!< Instance node in the functional unit instance tree being evaluated */
            char*         parent_inst,  /*!< Name of parent instance */
  /*@out@*/ unsigned int* hits01,       /*!< Pointer to number of 0 -> 1 toggles that were hit in the given instance tree */
  /*@out@*/ unsigned int* hits10,       /*!< Pointer to number of 1 -> 0 toggles that were hit in the given instance tree */
  /*@out@*/ unsigned int* total         /*!< Pointer to total number of bits that could be toggled in the given instance tree */
) { PROFILE(TOGGLE_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder for instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Specifies if at least one bit was not fully toggled */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Get printable version of this instance */
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

  if( (root->funit != NULL) && root->stat->show && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    miss_found |= toggle_display_instance_summary( ofile, tmpname, root->stat->tog01_hit, root->stat->tog10_hit, root->stat->tog_total );

    /* Update accumulated information */
    *hits01 += root->stat->tog01_hit;
    *hits10 += root->stat->tog10_hit;
    *total  += root->stat->tog_total;

  }

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= toggle_instance_summary( ofile, curr, tmpname, hits01, hits10, total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one bit was found to not be toggled; otherwise, returns FALSE.
  
 Displays the toggle functional unit summary information to the given output file, calculating the miss and
 percentage information.
*/
static bool toggle_display_funit_summary(
  FILE*        ofile,   /*!< Pointer to file to output summary information to */
  const char*  name,    /*!< Name of instance to output */
  const char*  fname,   /*!< Name of file containing instance to output */
  unsigned int hits01,  /*!< Number of bits that toggled from 0 -> 1 */
  unsigned int hits10,  /*!< Number of bits that toggled from 1 -> 0 */
  unsigned int total    /*!< Total number of bits in given instance */
) { PROFILE(TOGGLE_DISPLAY_FUNIT_SUMMARY);

  float percent01;  /* Percentage of bits that toggled from 0 to 1 */
  float percent10;  /* Percentage of bits that toggled from 1 to 0 */
  int   miss01;     /* Number of bits that did not toggle from 0 to 1 */
  int   miss10;     /* Number of bits that did not toggle from 1 to 0 */

  calc_miss_percent( hits01, total, &miss01, &percent01 );
  calc_miss_percent( hits10, total, &miss10, &percent10 );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5u/%5d/%5u      %3.0f%%         %5u/%5d/%5u      %3.0f%%\n",
           name, fname, hits01, miss01, total, percent01, hits10, miss10, total, percent10 );

  PROFILE_END;

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Iterates through the functional unit list displaying the toggle coverage for
 each functional unit.
*/
static bool toggle_funit_summary(
            FILE*         ofile,   /*!< Pointer to file to display coverage results to */
            funit_link*   head,    /*!< Pointer to head of functional unit list to parse */
  /*@out@*/ unsigned int* hits01,  /*!< Pointer to accumulated hit count for toggles 0 -> 1 */
  /*@out@*/ unsigned int* hits10,  /*!< Pointer to accumulated hit count for toggles 1 -> 0 */
  /*@out@*/ unsigned int* total    /*!< Pointer to accumulated total of bits */
) { PROFILE(TOGGLE_FUNIT_SUMMARY);

  bool  miss_found = FALSE;  /* Set to TRUE if missing toggles were found */
  char* pname;               /* Printable version of the functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= toggle_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->orig_fname ) ),
                                                  head->funit->stat->tog01_hit, head->funit->stat->tog10_hit, head->funit->stat->tog_total );

      free_safe( pname, (strlen( pname ) + 1) );

      /* Update accumulated information */
      *hits01 += head->funit->stat->tog01_hit;
      *hits10 += head->funit->stat->tog10_hit;
      *total  += head->funit->stat->tog_total;

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 Displays the signals that did not achieve 100% toggle coverage to standard 
 output from the specified signal list.
*/
static void toggle_display_verbose(
  FILE*      ofile,  /*!< Pointer to file to output results to */
  func_unit* funit,  /*!< Pointer to current named functional unit */
  rpt_type   rtype   /*!< Specifies the type of output to generate */
) { PROFILE(TOGGLE_DISPLAY_VERBOSE);

  func_iter    fi;        /* Functional unit iterator */
  vsignal*     sig;       /* Pointer to current signal being evaluated */
  unsigned int hit01;     /* Number of bits that toggled from 0 to 1 */
  unsigned int hit10;     /* Number of bits that toggled from 1 to 0 */
  char*        pname;     /* Printable version of signal name */
  unsigned int eid_size;  /* String length of exclusion ID */

  switch( rtype ) {
    case RPT_TYPE_HIT  :  fprintf( ofile, "    Signals getting 100%% toggle coverage\n\n" );      break;
    case RPT_TYPE_MISS :  fprintf( ofile, "    Signals not getting 100%% toggle coverage\n\n" );  break;
    case RPT_TYPE_EXCL :  fprintf( ofile, "    Signals excluded from toggle coverage\n\n" );      break;
  }

  if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) ) { 
    eid_size = db_get_exclusion_id_size();
  }

  if( rtype != RPT_TYPE_HIT ) {
    if( flag_output_exclusion_ids ) {
      char tmp[30];
      gen_char_string( tmp, ' ', (eid_size - 3) );
      fprintf( ofile, "      EID%s   Signal                    Toggle\n", tmp );
    } else {
      fprintf( ofile, "      Signal                    Toggle\n" );
    }
  }

  fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );

  while( (sig = func_iter_get_next_signal( &fi )) != NULL ) {

    hit01 = 0;
    hit10 = 0;

    /* Get printable version of the signal name */
    pname = scope_gen_printable( sig->name );

    if( (sig->suppl.part.type != SSUPPL_TYPE_PARAM)          &&
        (sig->suppl.part.type != SSUPPL_TYPE_PARAM_REAL)     &&
        (sig->suppl.part.type != SSUPPL_TYPE_ENUM)           &&
        (sig->suppl.part.type != SSUPPL_TYPE_MEM)            &&
        (sig->suppl.part.type != SSUPPL_TYPE_DECL_REAL)      &&
        (sig->suppl.part.type != SSUPPL_TYPE_DECL_SREAL)     &&
        (sig->suppl.part.type != SSUPPL_TYPE_IMPLICIT_REAL)  &&
        (sig->suppl.part.type != SSUPPL_TYPE_IMPLICIT_SREAL) &&
        (sig->suppl.part.mba == 0) ) {

      if( ((sig->suppl.part.excluded == 0) && (rtype != RPT_TYPE_EXCL)) ||
          ((sig->suppl.part.excluded == 1) && (rtype == RPT_TYPE_EXCL)) ) {

        vector_toggle_count( sig->value, &hit01, &hit10 );

        if( rtype == RPT_TYPE_HIT ) {

          if( (hit01 == sig->value->width) && (hit10 == sig->value->width) ) {
        
            fprintf( ofile, "      %-24s\n", pname );

          }

        } else {

          if( (((hit01 < sig->value->width) || (hit10 < sig->value->width)) && (rtype == RPT_TYPE_MISS)) ||
              ((sig->suppl.part.excluded == 1) && (rtype == RPT_TYPE_EXCL)) ) {

            exclude_reason* er;

            if( flag_output_exclusion_ids ) {
              char tmp[30];
              fprintf( ofile, "      (%s)  %-24s  0->1: ", db_gen_exclusion_id( 'T', sig->id ), pname );
              vector_display_toggle01_ulong( sig->value->value.ul, sig->value->width, ofile );      
              gen_char_string( tmp, ' ', (eid_size - 1) );
              fprintf( ofile, "\n       %s   ......................... 1->0: ", tmp );
              vector_display_toggle10_ulong( sig->value->value.ul, sig->value->width, ofile );      
              fprintf( ofile, " ...\n" );
              if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'T', sig->id, funit )) != NULL) ) {
                report_output_exclusion_reason( ofile, (12 + (eid_size - 1)), er->reason, TRUE );
              }
            } else {
              fprintf( ofile, "      %-24s  0->1: ", pname );
              vector_display_toggle01_ulong( sig->value->value.ul, sig->value->width, ofile );      
              fprintf( ofile, "\n      ......................... 1->0: " );
              vector_display_toggle10_ulong( sig->value->value.ul, sig->value->width, ofile );      
              fprintf( ofile, " ...\n" );
              if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'T', sig->id, funit )) != NULL) ) {
                report_output_exclusion_reason( ofile, 8, er->reason, TRUE );
              }
            }

          }

        }

      }

    }

    free_safe( pname, (strlen( pname ) + 1) );

  }

  func_iter_dealloc( &fi );

  fprintf( ofile, "\n" );

  PROFILE_END;

}

/*!
 Displays the verbose toggle coverage results to the specified output stream on
 an instance basis.  The verbose toggle coverage includes the signal names
 and their bits that did not receive 100% toggle coverage during simulation. 
*/
static void toggle_instance_verbose(
  FILE*       ofile,       /*!< Pointer to file to display coverage results to */
  funit_inst* root,        /*!< Pointer to root of instance functional unit tree to parse */
  char*       parent_inst  /*!< Name of parent instance */
) { PROFILE(TOGGLE_INSTANCE_VERBOSE);

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder of instance */
  char*       pname;          /* Printable version of the name */

  assert( root != NULL );

  /* Get printable version of the signal */
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
      ((!report_covered && ((root->stat->tog01_hit < root->stat->tog_total) || (root->stat->tog10_hit < root->stat->tog_total))) ||
       ( report_covered && root->stat->tog_cov_found) ||
       (report_exclusions && (root->stat->tog_excluded > 0))) ) {

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

    if( (!report_covered && ((root->stat->tog01_hit < root->stat->tog_total) || (root->stat->tog10_hit < root->stat->tog_total))) ||
        ( report_covered && root->stat->tog_cov_found) ) {
      toggle_display_verbose( ofile, root->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    }
    if( report_exclusions && (root->stat->tog_excluded > 0) ) {
      toggle_display_verbose( ofile, root->funit, RPT_TYPE_EXCL );
    }

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    toggle_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

  PROFILE_END;

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of functional unit list to parse.

 Displays the verbose toggle coverage results to the specified output stream on
 a functional unit basis (combining functional units that are instantiated multiple times).
 The verbose toggle coverage includes the signal names and their bits that
 did not receive 100% toggle coverage during simulation.
*/
static void toggle_funit_verbose(
  FILE*       ofile,  /*!< Pointer to file to display coverage results to */
  funit_link* head    /*!< Pointer to head of functional unit list to parse */
) { PROFILE(TOGGLE_FUNIT_VERBOSE);

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        ((!report_covered && ((head->funit->stat->tog01_hit < head->funit->stat->tog_total) || (head->funit->stat->tog10_hit < head->funit->stat->tog_total))) ||
         ( report_covered && head->funit->stat->tog_cov_found) ||
         (report_exclusions && (head->funit->stat->tog_excluded > 0))) ) {

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
      fprintf( ofile, "%s, File: %s\n", obf_funit( funit_flatten_name( head->funit ) ), obf_file( head->funit->orig_fname ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      if( (!report_covered && ((head->funit->stat->tog01_hit < head->funit->stat->tog_total) || (head->funit->stat->tog10_hit < head->funit->stat->tog_total))) ||
          ( report_covered && head->funit->stat->tog_cov_found) ) {
        toggle_display_verbose( ofile, head->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
      }
      if( report_exclusions && (head->funit->stat->tog_excluded > 0) ) {
        toggle_display_verbose( ofile, head->funit, RPT_TYPE_EXCL );
      }

    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the toggle coverage for each functional unit encountered.  The parent functional unit will
 specify its own toggle coverage along with a total toggle coverage including its 
 children.
*/
void toggle_report(
  FILE* ofile,   /*!< Pointer to file to output results to */
  bool  verbose  /*!< Specifies whether or not to provide verbose information */
) { PROFILE(TOGGLE_REPORT);

  bool         missed_found = FALSE;  /* If set to TRUE, indicates that untoggled bits were found */
  inst_link*   instl;                 /* Pointer to current instance link */
  unsigned int acc_hits01   = 0;      /* Accumulated 0 -> 1 toggle hit count */
  unsigned int acc_hits10   = 0;      /* Accumulated 1 -> 0 toggle hit count */
  unsigned int acc_total    = 0;      /* Accumulated bit count */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   TOGGLE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= toggle_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_hits01, &acc_hits10, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)toggle_display_instance_summary( ofile, "Accumulated", acc_hits01, acc_hits10, acc_total );
    
    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        toggle_instance_verbose( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = toggle_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits01, &acc_hits10, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)toggle_display_funit_summary( ofile, "Accumulated", "", acc_hits01, acc_hits10, acc_total );

    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      toggle_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

