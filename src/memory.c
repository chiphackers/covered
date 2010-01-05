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
 \file     memory.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     9/24/2006
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "db.h"
#include "defines.h"
#include "exclude.h"
#include "func_iter.h"
#include "func_unit.h"
#include "link.h"
#include "memory.h"
#include "obfuscate.h"
#include "ovl.h"
#include "report.h"
#include "vector.h"
#include "vsignal.h"
#include "util.h"


extern db**         db_list;
extern unsigned int curr_db;
extern bool         report_covered;
extern bool         report_instance;
extern isuppl       info_suppl;
extern bool         report_exclusions;
extern unsigned int exclusion_id_size;
extern bool         flag_output_exclusion_ids; 


/*!
 Calculates the total and hit memory coverage information for the given memory signal.
*/
void memory_get_stat(
  vsignal*      sig,         /*!< Pointer to signal list to traverse for memories */
  unsigned int* wr_hit,      /*!< Pointer to total number of addressable elements written */
  unsigned int* rd_hit,      /*!< Pointer to total number of addressable elements read */
  unsigned int* ae_total,    /*!< Pointer to total number of addressable elements */
  unsigned int* tog01_hit,   /*!< Pointer to total number of bits toggling from 0->1 */
  unsigned int* tog10_hit,   /*!< Pointer to total number of bits toggling from 1->0 */
  unsigned int* tog_total,   /*!< Pointer to total number of bits in memories that can be toggled */
  unsigned int* excluded,    /*!< Pointer to total number of excluded memory coverage points */
  bool*         cov_found,   /*!< Pointer to value that is set to TRUE if at least one memory was found to be fully covered */
  bool          ignore_excl  /*!< If set to TRUE, ignores the current value of the excluded bit */
) { PROFILE(MEMORY_GET_STAT);

  unsigned int i;                      /* Loop iterator */
  unsigned int pwidth;                 /* Width of packed portion of memory */
  bool         ae_cov_found  = TRUE;   /* Set to TRUE if wr/rd hits equals the total */
  bool         tog_cov_found = FALSE;  /* Set to TRUE if tog01/10 hits equals the total */

  /* Calculate width of smallest addressable element */
  pwidth = 1;
  for( i=(sig->udim_num); i<(sig->udim_num + sig->pdim_num); i++ ) {
    if( sig->dim[i].msb > sig->dim[i].lsb ) {
      pwidth *= (sig->dim[i].msb - sig->dim[i].lsb) + 1;
    } else {
      pwidth *= (sig->dim[i].lsb - sig->dim[i].msb) + 1;
    }
  }

  /* Calculate total number of addressable elements and their write/read information */
  for( i=0; i<sig->value->width; i+=pwidth ) {
    if( (sig->suppl.part.excluded == 1) && !ignore_excl ) {
      (*wr_hit)++;
      (*rd_hit)++;
      *excluded += 2;
    } else {
      unsigned int wr = 0;
      unsigned int rd = 0;
      vector_mem_rw_count( sig->value, (int)i, (int)((i + pwidth) - 1), &wr, &rd );
      if( wr > 0 ) {
        (*wr_hit)++;
      }
      if( rd > 0 ) {
        (*rd_hit)++;
      }
      ae_cov_found &= (wr > 0) && (rd > 0);
    }
    (*ae_total)++;
  }

  /* Calculate toggle coverage information for the memory */
  *tog_total += sig->value->width;
  if( (sig->suppl.part.excluded == 1) && !ignore_excl ) {
    *tog01_hit += sig->value->width;
    *tog10_hit += sig->value->width;
    *excluded  += (sig->value->width * 2);
  } else {
    unsigned int hit01 = 0;
    unsigned int hit10 = 0;
    vector_toggle_count( sig->value, &hit01, &hit10 );
    *tog01_hit += hit01;
    *tog10_hit += hit10;
    tog_cov_found = (hit01 == sig->value->width) && (hit10 == sig->value->width);
  }

  *cov_found |= ae_cov_found && tog_cov_found;

  PROFILE_END;

}

/*!
 Gathers memory statistics for the memories in the given signal list.
*/
void memory_get_stats(
            func_unit*    funit,      /*!< Pointer to current functional unit */
  /*@out@*/ unsigned int* wr_hit,     /*!< Pointer to total number of addressable elements written */
  /*@out@*/ unsigned int* rd_hit,     /*!< Pointer to total number of addressable elements read */
  /*@out@*/ unsigned int* ae_total,   /*!< Pointer to total number of addressable elements */
  /*@out@*/ unsigned int* tog01_hit,  /*!< Pointer to total number of bits toggling from 0->1 */
  /*@out@*/ unsigned int* tog10_hit,  /*!< Pointer to total number of bits toggling from 1->0 */
  /*@out@*/ unsigned int* tog_total,  /*!< Pointer to total number of bits in memories that can be toggled */
  /*@out@*/ unsigned int* excluded,   /*!< Pointer to total number of excluded memory coverage points */
  /*@out@*/ bool*         cov_found   /*!< Pointer to value indicating that at least one memory was found to be fully covered */
) { PROFILE(MEMORY_GET_STATS);

  if( !funit_is_unnamed( funit ) ) {

    func_iter fi;
    vsignal*  sig;

    func_iter_init( &fi, funit, FALSE, TRUE, FALSE );
   
    while( (sig = func_iter_get_next_signal( &fi )) != NULL ) {

      /* Calculate only for memory elements (must contain one or more unpacked dimensions) */
      if( (sig->suppl.part.type == SSUPPL_TYPE_MEM) && (sig->udim_num > 0) ) {

        memory_get_stat( sig, wr_hit, rd_hit, ae_total, tog01_hit, tog10_hit, tog_total, excluded, cov_found, FALSE );

      }

    }

    func_iter_dealloc( &fi );

  }

  PROFILE_END;

}

/*!
 Retrieves memory summary information for a given functional unit made by a GUI request.
*/
void memory_get_funit_summary(
            func_unit*    funit,     /*!< Pointer to found functional unit */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to number of memories that received 100% coverage of all memory metrics */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded memory coverage points */
  /*@out@*/ unsigned int* total      /*!< Pointer to total number of memories in the given functional unit */
) { PROFILE(MEMORY_GET_FUNIT_SUMMARY);

  *hit      = funit->stat->mem_wr_hit + funit->stat->mem_rd_hit + funit->stat->mem_tog01_hit + funit->stat->mem_tog10_hit;
  *excluded = funit->stat->mem_excluded;
  *total    = (funit->stat->mem_ae_total * 2) + (funit->stat->mem_tog_total * 2);

  PROFILE_END;

}

/*!
 Retrieves memory summary information for a given functional unit instance made by a GUI request.
*/
void memory_get_inst_summary(
            funit_inst*   inst,      /*!< Pointer to found functional unit instance */
  /*@out@*/ unsigned int* hit,       /*!< Pointer to number of memories that received 100% coverage of all memory metrics */
  /*@out@*/ unsigned int* excluded,  /*!< Pointer to number of excluded memory coverage points */
  /*@out@*/ unsigned int* total      /*!< Pointer to total number of memories in the given functional unit instance */
) { PROFILE(MEMORY_GET_INST_SUMMARY);

  *hit      = inst->stat->mem_wr_hit + inst->stat->mem_rd_hit + inst->stat->mem_tog01_hit + inst->stat->mem_tog10_hit;
  *excluded = inst->stat->mem_excluded;
  *total    = (inst->stat->mem_ae_total * 2) + (inst->stat->mem_tog_total * 2);
  
  PROFILE_END; 
  
}

/*!
 Creates a string array for each bit in the given signal corresponding to its position
 in the packed array portion.
*/
static void memory_create_pdim_bit_array(
  /*@out@*/ char**       str,     /*!< Pointer to string array to populate with packed dimension information */
            vsignal*     sig,     /*!< Pointer to signal that we are solving for */
            char*        prefix,  /*!< Prefix string to append to the beginning of the newly created string */
            unsigned int dim      /*!< Current dimension to solve for */
) { PROFILE(MEMORY_CREATE_PDIM_BIT_ARRAY);

  char name[4096];  /* Temporary string */
  int  i;           /* Loop iterator */
  bool last_dim;    /* Specifies if this is the final dimension */

  /* Calculate final dimension */
  last_dim = (dim + 1) == (sig->pdim_num + sig->udim_num);

  if( sig->dim[dim].msb > sig->dim[dim].lsb ) {

    for( i=sig->dim[dim].lsb; i<=sig->dim[dim].msb; i++ ) {
      if( last_dim ) {
        unsigned int rv = snprintf( name, 4096, "%d", i );
        assert( rv < 4096 );
        *str = (char*)realloc_safe( *str, (strlen( *str ) + 1), (strlen( *str ) + strlen( prefix ) + strlen( name ) + 4) );
        strcat( *str, prefix );
        strcat( *str, "[" );
        strcat( *str, name );
        strcat( *str, "] " );
      } else {
        unsigned int rv = snprintf( name, 4096, "%s[%d]", prefix, i );
        assert( rv < 4096 );
        memory_create_pdim_bit_array( str, sig, name, (dim + 1) );
      }
    }

  } else {

    for( i=sig->dim[dim].lsb; i>=sig->dim[dim].msb; i-- ) {
      if( last_dim ) {
        unsigned int rv = snprintf( name, 4096, "%d", i );
        assert( rv < 4096 );
        *str = (char*)realloc_safe( *str, (strlen( *str ) + 1), (strlen( *str ) + strlen( prefix ) + strlen( name ) + 4) );
        strcat( *str, prefix );
        strcat( *str, "[" );
        strcat( *str, name );
        strcat( *str, "] " );
      } else {
        unsigned int rv = snprintf( name, 4096, "%s[%d]", prefix, i );
        assert( rv < 4096 );
        memory_create_pdim_bit_array( str, sig, name, (dim + 1) );
      }
    }

  }

  PROFILE_END;

}

/*!
 Creates memory array structure for Tcl and stores it into the mem_str parameter.
*/
static void memory_get_mem_coverage(
  char**       mem_str,          /*!< String containing memory information */
  vsignal*     sig,              /*!< Pointer to signal to get memory coverage for */
  int          offset,           /*!< Bit offset of signal vector to start retrieving information from */
  char*        prefix,           /*!< String containing memory prefix to output (initially this will be just the signal name) */
  unsigned int dim,              /*!< Current dimension index (initially this will be 0) */
  unsigned int parent_dim_width  /*!< Bit width of parent dimension (initially this will be the width of the signal) */
) { PROFILE(MEMORY_GET_MEM_COVERAGE);

  char         name[4096];  /* Contains signal name */
  int          msb;         /* MSB of current dimension */
  int          lsb;         /* LSB of current dimension */
  int          be;          /* Big endianness of current dimension */
  int          i;           /* Loop iterator */
  unsigned int dim_width;   /* Bit width of current dimension */

  assert( sig != NULL );
  assert( prefix != NULL );
  assert( dim < sig->udim_num );

  /* Calculate MSB, LSB and big endianness of current dimension */
  if( sig->dim[dim].msb > sig->dim[dim].lsb ) {
    msb = sig->dim[dim].msb;
    lsb = sig->dim[dim].lsb;
    be  = FALSE;
  } else {
    msb = sig->dim[dim].lsb;
    lsb = sig->dim[dim].msb;
    be  = TRUE;
  }

  /* Calculate current dimensional width */
  dim_width = parent_dim_width / ((msb - lsb) + 1);

  /* Only output memory contents if we have reached the lowest dimension */
  if( (dim + 1) == sig->udim_num ) {

    vector*      vec = vector_create( dim_width, VTYPE_MEM, VDATA_UL, TRUE );
    unsigned int tog01;
    unsigned int tog10;
    unsigned int wr;
    unsigned int rd;
    char*        tog01_str;
    char*        tog10_str;
    char         hit_str[2];
    char         int_str[20];
    char*        dim_str;
    char*        entry_str;

    /* Iterate through each addressable element in the current dimension */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      unsigned int rv;
      unsigned int slen;

      /* Re-initialize the vector */
      if( be ) {
        vector_copy_range( vec, sig->value, ((dim_width * ((msb - lsb) - i)) + offset) );
      } else {
        vector_copy_range( vec, sig->value, ((dim_width * i) + offset) );
      }

      /* Create dimension string */
      rv = snprintf( int_str, 20, "%d", i );
      assert( rv < 20 );
      slen    = strlen( prefix ) + strlen( int_str ) + 5;
      dim_str = (char*)malloc_safe( slen );
      rv = snprintf( dim_str, slen, "%s\\[%d\\]", prefix, i );
      assert( rv < slen );

      /* Get toggle information */
      tog01 = 0;
      tog10 = 0;
      vector_toggle_count( vec, &tog01, &tog10 );

      /* Get toggle strings */
      tog01_str = vector_get_toggle01_ulong( vec->value.ul, vec->width );
      tog10_str = vector_get_toggle10_ulong( vec->value.ul, vec->width );

      /* Get write/read information */
      wr = 0;
      rd = 0;
      vector_mem_rw_count( vec, 0, (int)(vec->width - 1), &wr, &rd );

      /* Output the addressable memory element if it is found to be lacking in coverage */
      if( (tog01 < dim_width) || (tog10 < dim_width) || (wr == 0) || (rd == 0) ) {
        strcpy( hit_str, "0" );
      } else {
        strcpy( hit_str, "1" );
      }

      /* Create a string list for this entry */
      slen      = strlen( dim_str ) + strlen( hit_str ) + strlen( tog01_str ) + strlen( tog10_str ) + 10;
      entry_str = (char*)malloc_safe( slen );
      rv = snprintf( entry_str, slen, "{%s %s %d %d %s %s}",
                dim_str, hit_str, ((wr == 0) ? 0 : 1), ((rd == 0) ? 0 : 1), tog01_str, tog10_str );
      assert( rv < slen );

      *mem_str = (char*)realloc_safe( *mem_str, (strlen( *mem_str ) + 1), (strlen( *mem_str ) + strlen( entry_str ) + 2) );
      strcat( *mem_str, " " );
      strcat( *mem_str, entry_str );

      /* Deallocate memory */
      free_safe( dim_str, (strlen( dim_str ) + 1) );
      free_safe( tog01_str, (strlen( tog01_str ) + 1) );
      free_safe( tog10_str, (strlen( tog10_str ) + 1) );
      free_safe( entry_str, (strlen( entry_str ) + 1) );

    }

    /* Deallocate vector */
    vector_dealloc( vec );

  /* Otherwise, go down one level */
  } else {

    /* Iterate through each entry in the current dimesion */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      /* Create new prefix */
      unsigned int rv = snprintf( name, 4096, "%s[%d]", prefix, i );
      assert( rv < 4096 );

      if( be ) {
        memory_get_mem_coverage( mem_str, sig, (offset + (dim_width * ((msb - lsb) - i))), name, (dim + 1), dim_width );
      } else {
        memory_get_mem_coverage( mem_str, sig, (offset + (dim_width * i)),                 name, (dim + 1), dim_width );
      }

    }

  }
 
  PROFILE_END;

}

/*!
 Retrieves memory coverage information for the given signal in the specified functional unit.
*/
void memory_get_coverage(
            func_unit*  funit,        /*!< Pointer to functional unit */
            const char* signame,      /*!< Name of signal to find memory coverage information for */
  /*@out@*/ char**      pdim_str,     /*!< Pointer to string to store packed dimensional information */
  /*@out@*/ char**      pdim_array,   /*!< Pointer to string to store packed dimensional array */
  /*@out@*/ char**      udim_str,     /*!< Pointer to string to store unpacked dimensional information */
  /*@out@*/ char**      memory_info,  /*!< Pointer to string to store memory information into */
  /*@out@*/ int*        excluded,     /*!< Pointer to excluded indicator to store */
  /*@out@*/ char**      reason        /*!< Pointer to reason for exclusion */
) { PROFILE(MEMORY_GET_COVERAGE);

  func_iter       fi;        /* Functional unit iterator */
  vsignal*        sig;       /* Pointer to current signal */
  unsigned int    i;         /* Loop iterator */
  char            tmp1[20];  /* Temporary string holder */
  char            tmp2[20];  /* Temporary string holder */
  unsigned int    rv;        /* Return value */
  exclude_reason* er;

  /* Find the signal in the functional unit */
  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );
  while( ((sig = func_iter_get_next_signal( &fi )) != NULL) && (strcmp( sig->name, signame ) != 0) );
  func_iter_dealloc( &fi );
  assert( sig != NULL );

  /* Allocate and populate the pdim_array and pdim_width parameters */
  *pdim_array = (char*)malloc_safe( 1 );
  (*pdim_array)[0] = '\0';
  memory_create_pdim_bit_array( pdim_array, sig, "", sig->pdim_num );

  /* Allocate and populate the pdim_str string */
  *pdim_str = NULL;
  for( i=sig->udim_num; i<(sig->pdim_num + sig->udim_num); i++ ) {
    unsigned int slen;
    rv = snprintf( tmp1, 20, "%d", sig->dim[i].msb );
    assert( rv < 20 );
    rv = snprintf( tmp2, 20, "%d", sig->dim[i].lsb );
    assert( rv < 20 );
    slen = strlen( tmp1 ) + strlen( tmp2 ) + 4;
    *pdim_str = (char*)realloc_safe( *pdim_str, (strlen( *pdim_str ) + 1), slen );
    if( i == sig->udim_num ) {
      rv = snprintf( *pdim_str, slen, "[%s:%s]", tmp1, tmp2 );
      assert( rv < slen );
    } else {
      strcat( *pdim_str, "[" );
      strcat( *pdim_str, tmp1 );
      strcat( *pdim_str, ":" );
      strcat( *pdim_str, tmp2 );
      strcat( *pdim_str, "]" );
    }
  }

  /* Allocate and populate the udim_info string */
  *udim_str = NULL;
  for( i=0; i<sig->udim_num; i++ ) {
    unsigned int slen;
    rv = snprintf( tmp1, 20, "%d", sig->dim[i].msb );
    assert( rv < 20 );
    rv = snprintf( tmp2, 20, "%d", sig->dim[i].lsb );
    assert( rv < 20 );
    slen = strlen( tmp1 ) + strlen( tmp2 ) + 4;
    *udim_str = (char*)realloc_safe( *udim_str, (strlen( *udim_str ) + 1), slen );
    if( i == 0 ) {
      rv = snprintf( *udim_str, slen, "[%s:%s]", tmp1, tmp2 );
      assert( rv < slen );
    } else {
      strcat( *udim_str, "[" );
      strcat( *udim_str, tmp1 );
      strcat( *udim_str, ":" );
      strcat( *udim_str, tmp2 );
      strcat( *udim_str, "]" );
    }
  }

  /* Populate the memory_info string */
  *memory_info = (char*)malloc_safe( 1 );
  (*memory_info)[0] = '\0';
  memory_get_mem_coverage( memory_info, sig, 0, "", 0, sig->value->width );

  /* Populate the excluded value */
  *excluded = sig->suppl.part.excluded;

  /* Populate the exclusion reason */
  if( (*excluded == 1) && ((er = exclude_find_exclude_reason( 'M', sig->id, funit )) != NULL) ) {
    *reason = strdup_safe( er->reason );
  } else {
    *reason = NULL;
  }
  
  PROFILE_END;

}

/*!
 Collects all signals that are memories and match the given coverage type and stores them
 in the given signal list.
*/
void memory_collect(
            func_unit*    funit,           /*!< Pointer to functional unit */
            int           cov,             /*!< Set to 0 to get uncovered memories or 1 to get covered memories */
  /*@out@*/ vsignal***    sigs,            /*!< Pointer to signal array containing retrieved signals */
  /*@out@*/ unsigned int* sig_size,        /*!< Pointer to tail of signal list containing retrieved signals */
  /*@out@*/ unsigned int* sig_no_rm_index  /*!< Pointer to starting index to being removing signal */
) { PROFILE(MEMORY_COLLECT);

  func_iter    fi;             /* Functional unit iterator */
  vsignal*     sig;            /* Pointer to current signal */
  unsigned int wr_hit    = 0;  /* Total number of addressable elements written */
  unsigned int rd_hit    = 0;  /* Total number of addressable elements read */
  unsigned int ae_total  = 0;  /* Total number of addressable elements */
  unsigned int hit01     = 0;  /* Number of bits that toggled from 0 to 1 */
  unsigned int hit10     = 0;  /* Number of bits that toggled from 1 to 0 */
  unsigned int tog_total = 0;  /* Total number of toggle bits */
  unsigned int excluded  = 0;  /* Number of excluded memory coverage points */
  bool         cov_found;

  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );

  while( (sig = func_iter_get_next_signal( &fi )) != NULL ) {

    if( sig->suppl.part.type == SSUPPL_TYPE_MEM ) {

      ae_total = 0;
   
      memory_get_stat( sig, &ae_total, &wr_hit, &rd_hit, &tog_total, &hit01, &hit10, &excluded, &cov_found, TRUE );

      /* If this signal meets the coverage requirement, add it to the signal list */
      if( ((cov == 1) && (wr_hit > 0) && (rd_hit > 0) && (hit01 == tog_total) && (hit10 == tog_total)) ||
          ((cov == 0) && ((wr_hit == 0) || (rd_hit == 0) || (hit01 < tog_total) || (hit10 < tog_total))) ) {

        sig_link_add( sig, TRUE, sigs, sig_size, sig_no_rm_index );

      }

    }

  }

  func_iter_dealloc( &fi );

  PROFILE_END;

}

/*!
 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Calculates and outputs the memory toggle summary coverage results for a given instance.
*/
static bool memory_display_toggle_instance_summary(
  FILE*       ofile,   /*!< Pointer to file to display coverage results to */
  const char* name,    /*!< Name of instance being reported */
  int         hits01,  /*!< Pointer to number of bits in memory toggled from 0 -> 1 */
  int         hits10,  /*!< Pointer to number of bits in memory toggled from 1 -> 0 */
  int         total    /*!< Pointer to total number of bits in memories */
) { PROFILE(MEMORY_DISPLAY_TOGGLE_INSTANCE_SUMMARY);

  float percent01;    /* Percentage of bits toggling from 0 -> 1 */
  float percent10;    /* Percentage of bits toggling from 1 -> 0 */
  int   miss01  = 0;  /* Number of bits that did not toggle from 0 -> 1 */
  int   miss10  = 0;  /* Number of bits that did not toggle from 1 -> 0 */

  calc_miss_percent( hits01, total, &miss01, &percent01 );
  calc_miss_percent( hits10, total, &miss10, &percent10 );

  /* Output toggle information */
  fprintf( ofile, "  %-43.43s    %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, hits01, miss01, total, percent01, hits10, miss10, total, percent10 );

  PROFILE_END;

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \return Returns TRUE if any bits were found to be not toggled; otherwise, returns FALSE.

 Displays the memory instance summarization to the specified file.  Recursively
 iterates through functional unit instance tree, outputting the toggle information that
 is found at that instance.
*/
static bool memory_toggle_instance_summary(
            FILE*       ofile,        /*!< File to output coverage information to */
            funit_inst* root,         /*!< Instance node in the functional unit instance tree being evaluated */
            char*       parent_inst,  /*!< Name of parent instance */
  /*@out@*/ int*        hits01,       /*!< Pointer to accumulated toggle 0 -> 1 hit count */
  /*@out@*/ int*        hits10,       /*!< Pointer to accumulated toggle 1 -> 0 hit count */
  /*@out@*/ int*        total         /*!< Pointer to total number of memory bits */
) { PROFILE(MEMORY_TOGGLE_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder for instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if at least one bit is found to not be toggled */

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

    miss_found |= memory_display_toggle_instance_summary( ofile, tmpname, root->stat->mem_tog01_hit, root->stat->mem_tog10_hit, root->stat->mem_tog_total );

    /* Update accumulated coverage information */
    *hits01 += root->stat->mem_tog01_hit;
    *hits10 += root->stat->mem_tog10_hit;
    *total  += root->stat->mem_tog_total;

  } 

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= memory_toggle_instance_summary( ofile, curr, tmpname, hits01, hits10, total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one miss was found; otherwise, returns FALSE.

 Calculates the miss and hit percentage statistics for the given instance and outputs this information
 to the given output file.
*/
static bool memory_display_ae_instance_summary(
  FILE* ofile,   /*!< Pointer to file to output coverage summary information to */
  char* name,    /*!< Name of instance being displayed */
  int   wr_hit,  /*!< Number of addressable elements that were written */
  int   rd_hit,  /*!< Number of addressable elements that were read */
  int   total    /*!< Number of all addressable elements */
) { PROFILE(MEMORY_DISPLAY_AE_INSTANCE_SUMMARY);

  float wr_percent;  /* Percentage of addressable elements written */
  float rd_percent;  /* Percentage of addressable elements read */
  int   wr_miss;     /* Number of addressable elements that were not written */
  int   rd_miss;     /* Number of addressable elements that were not read */

  calc_miss_percent( wr_hit, total, &wr_miss, &wr_percent );
  calc_miss_percent( rd_hit, total, &rd_miss, &rd_percent );

  /* Output toggle information */
  fprintf( ofile, "  %-43.43s    %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, wr_hit, wr_miss, total, wr_percent, rd_hit, rd_miss, total, rd_percent );

  PROFILE_END;

  return( (wr_miss > 0) || (rd_miss > 0) );

}

/*!
 \return Returns TRUE if any bits were found to be not toggled; otherwise, returns FALSE.

 Displays the memory instance summarization to the specified file.  Recursively
 iterates through functional unit instance tree, outputting the toggle information that
 is found at that instance.
*/
static bool memory_ae_instance_summary(
            FILE*       ofile,        /*!< File to output coverage information to */
            funit_inst* root,         /*!< Instance node in the functional unit instance tree being evaluated */
            char*       parent_inst,  /*!< Name of parent instance */
  /*@out@*/ int*        wr_hits,      /*!< Pointer to accumulated number of addressable elements written */
  /*@out@*/ int*        rd_hits,      /*!< Pointer to accumulated number of addressable elements read */
  /*@out@*/ int*        total         /*!< Pointer to the total number of addressable elements */
) { PROFILE(MEMORY_AE_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder for instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to true if a coverage type was missed */

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

    miss_found |= memory_display_ae_instance_summary( ofile, tmpname, root->stat->mem_wr_hit, root->stat->mem_rd_hit, root->stat->mem_ae_total );

    /* Update accumulated stats */
    *wr_hits += root->stat->mem_wr_hit;
    *rd_hits += root->stat->mem_rd_hit;
    *total   += root->stat->mem_ae_total;

  } 

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= memory_ae_instance_summary( ofile, curr, tmpname, wr_hits, rd_hits, total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one bit was found to not have transitioned during simulation.

 Calculates and outputs the toggle coverage for all memories in the given functional unit.
*/
static bool memory_display_toggle_funit_summary(
  FILE*       ofile,  /*!< Pointer to file to output functional unit summary results */
  const char* name,   /*!< Name of functional unit being reported */
  const char* fname,  /*!< Filename containing the functional unit being reported */
  int         hit01,  /*!< Number of memory bits that transitioned from a value of 0 to a value of 1 during simulation for this functional unit */
  int         hit10,  /*!< Number of memory bits that transitioned from a value of 1 to a value of 0 during simulation for this functional unit */
  int         total   /*!< Number of total memory bits in the given functional unit */
) { PROFILE(MEMORY_DISPLAY_TOGGLE_FUNIT_SUMMARY);

  float percent01;  /* Percentage of bits that toggled from 0 to 1 */
  float percent10;  /* Percentage of bits that toggled from 1 to 0 */
  int   miss01;     /* Number of bits that did not toggle from 0 to 1 */
  int   miss10;     /* Number of bits that did not toggle from 1 to 0 */

  calc_miss_percent( hit01, total, &miss01, &percent01 );
  calc_miss_percent( hit10, total, &miss10, &percent10 );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, fname, hit01, miss01, total, percent01, hit10, miss10, total, percent10 );

  PROFILE_END;

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Iterates through the functional unit list displaying the memory toggle coverage summary for
 each functional unit.
*/
static bool memory_toggle_funit_summary(
            FILE*       ofile,   /*!< Pointer to file to display coverage results to */
            funit_link* head,    /*!< Pointer to head of functional unit list to parse */
  /*@out@*/ int*        hits01,  /*!< Pointer to number of bits in memory toggled from 0 -> 1 */
  /*@out@*/ int*        hits10,  /*!< Pointer to number of bits in memory toggled from 1 -> 0 */
  /*@out@*/ int*        total    /*!< Pointer to total number of bits in memories */
) { PROFILE(MEMORY_TOGGLE_FUNIT_SUMMARY);

  bool  miss_found = FALSE;  /* Set to TRUE if missing toggles were found */
  char* pname;               /* Printable version of the functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= memory_display_toggle_funit_summary( ofile, pname, get_basename( obf_file( head->funit->orig_fname ) ),
                                                         head->funit->stat->mem_tog01_hit, head->funit->stat->mem_tog10_hit, head->funit->stat->mem_tog_total );

      /* Update accumulated information */
      *hits01 += head->funit->stat->mem_tog01_hit;
      *hits10 += head->funit->stat->mem_tog10_hit;
      *total  += head->funit->stat->mem_tog_total;

      free_safe( pname, (strlen( pname ) + 1) );

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one addressable memory element was not written or read during simulation; otherwise,
         returns FALSE.

 Calculates and outputs the summary addressable memory element coverage information for a given functional unit.
*/
static bool memory_display_ae_funit_summary(
  FILE*       ofile,    /*!< Pointer to output file that will be written */
  const char* name,     /*!< Name of functional unit being reported */
  const char* fname,    /*!< Filename containing the given functional unit being reported */
  int         wr_hits,  /*!< Number of addressable memory elements that were written in this functional unit during simulation */
  int         rd_hits,  /*!< Number of addressable memory elements that were read in this functional unit during simulation */
  int         total     /*!< Number of addressable memory elements in the given functional unit */
) { PROFILE(MEMORY_DISPLAY_AE_FUNIT_SUMMARY);

  float wr_percent;  /* Percentage of addressable elements that were written */
  float rd_percent;  /* Percentage of addressable elements that were read */
  int   wr_miss;     /* Number of addressable elements that were not written */
  int   rd_miss;     /* Number of addressable elements that were not read */

  calc_miss_percent( wr_hits, total, &wr_miss, &wr_percent );
  calc_miss_percent( rd_hits, total, &rd_miss, &rd_percent );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, fname, wr_hits, wr_miss, total, wr_percent, rd_hits, rd_miss, total, rd_percent );

  PROFILE_END;

  return( (wr_miss > 0) || (rd_miss > 0) );

}

/*!
 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Iterates through the functional unit list displaying the memory toggle coverage summary for
 each functional unit.
*/
static bool memory_ae_funit_summary(
            FILE*       ofile,    /*!< Pointer to file to display coverage results to */
            funit_link* head,     /*!< Pointer to head of functional unit list to parse */
  /*@out@*/ int*        wr_hits,  /*!< Pointer to number of bits in memory toggled from 0 -> 1 */
  /*@out@*/ int*        rd_hits,  /*!< Pointer to number of bits in memory toggled from 1 -> 0 */
  /*@out@*/ int*        total     /*!< Pointer to total number of bits in memories */
) { PROFILE(MEMORY_AE_FUNIT_SUMMARY);

  bool  miss_found = FALSE;  /* Set to TRUE if missing toggles were found */
  char* pname;               /* Printable version of the functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= memory_display_ae_funit_summary( ofile, pname, get_basename( obf_file( head->funit->orig_fname ) ),
                                                     head->funit->stat->mem_wr_hit, head->funit->stat->mem_rd_hit, head->funit->stat->mem_ae_total );

      /* Update accumulated information */
      *wr_hits += head->funit->stat->mem_wr_hit;
      *rd_hits += head->funit->stat->mem_rd_hit;
      *total   += head->funit->stat->mem_ae_total; 

      free_safe( pname, (strlen( pname ) + 1) );

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 Outputs the contents of the given memory in verbose output format.
*/
static void memory_display_memory(
  FILE*        ofile,            /*!< Pointer to output file */
  vsignal*     sig,              /*!< Pointer to the current memory element to output */
  int          offset,           /*!< Bit offset of signal vector value to start interrogating */
  char*        prefix,           /*!< String containing memory prefix to output (initially this will be just the signal name) */
  unsigned int dim,              /*!< Current dimension index (initially this will be 0) */
  unsigned int parent_dim_width  /*!< Bit width of parent dimension (initially this will be the width of the signal) */
) { PROFILE(MEMORY_DISPLAY_MEMORY);

  char         name[4096];  /* Contains signal name */
  int          msb;         /* MSB of current dimension */
  int          lsb;         /* LSB of current dimension */
  int          be;          /* Big endianness of current dimension */
  int          i;           /* Loop iterator */
  unsigned int dim_width;   /* Bit width of current dimension */

  assert( sig != NULL );
  assert( prefix != NULL );
  assert( dim < sig->udim_num );

  /* Calculate MSB, LSB and big endianness of current dimension */
  if( sig->dim[dim].msb > sig->dim[dim].lsb ) {
    msb = sig->dim[dim].msb;
    lsb = sig->dim[dim].lsb;
    be  = FALSE;
  } else {
    msb = sig->dim[dim].lsb;
    lsb = sig->dim[dim].msb;
    be  = TRUE;
  }

  /* Calculate current dimensional width */
  dim_width = parent_dim_width / ((msb - lsb) + 1);

  /* Only output memory contents if we have reached the lowest dimension */
  if( (dim + 1) == sig->udim_num ) {

    vector*      vec = vector_create( dim_width, VTYPE_MEM, VDATA_UL, TRUE );
    unsigned int tog01;
    unsigned int tog10;
    unsigned int wr;
    unsigned int rd;

    /* Iterate through each addressable element in the current dimension */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      if( be ) {
        vector_copy_range( vec, sig->value, ((dim_width * ((msb - lsb) - i)) + offset) );
      } else {
        vector_copy_range( vec, sig->value, ((dim_width * i) + offset) );
      }

      /* Get toggle information */
      tog01 = 0;
      tog10 = 0;
      vector_toggle_count( vec, &tog01, &tog10 );

      /* Get write/read information */
      wr = 0;
      rd = 0;
      vector_mem_rw_count( vec, 0, (int)(vec->width - 1), &wr, &rd );

      /* Output the addressable memory element if it is found to be lacking in coverage */
      if( (tog01 < dim_width) || (tog10 < dim_width) || (wr == 0) || (rd == 0) ) {

        unsigned int j;
        unsigned int rv = snprintf( name, 4096, "%s[%d]", prefix, i );
        assert( rv < 4096 );

        fprintf( ofile, "        %s  Written: %d  0->1: ", name, ((wr == 0) ? 0 : 1) );
        vector_display_toggle01_ulong( vec->value.ul, vec->width, ofile );
        fprintf( ofile, "\n" );
        fprintf( ofile, "        " );
        for( j=0; j<strlen( name ); j++ ) {
          fprintf( ofile, "." );
        }
        fprintf( ofile, "  Read   : %d  1->0: ", ((rd == 0) ? 0 : 1) );
        vector_display_toggle10_ulong( vec->value.ul, vec->width, ofile );
        fprintf( ofile, " ...\n" );
      }

    } 

    /* Deallocate the vector */
    vector_dealloc( vec );

  /* Otherwise, go down one level */
  } else {

    /* Iterate through each entry in the current dimesion */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      /* Create new prefix */
      unsigned int rv = snprintf( name, 4096, "%s[%d]", prefix, i );
      assert( rv < 4096 );

      if( be ) {
        memory_display_memory( ofile, sig, (offset + (dim_width * ((msb - lsb) - i))), name, (dim + 1), dim_width );
      } else {
        memory_display_memory( ofile, sig, (offset + (dim_width * i)),                 name, (dim + 1), dim_width );
      }

    }

  }

  PROFILE_END;

}

/*!
 Displays the memories that did not achieve 100% toggle coverage and/or 100%
 write/read coverage to standard output from the specified signal list.
*/
static void memory_display_verbose(
  FILE*      ofile,  /*!< Pointer to file to output results to */
  func_unit* funit,  /*!< Pointer to the current functional unit */
  rpt_type   rtype   /*!< Report type to generate */
) { PROFILE(MEMORY_DISPLAY_VERBOSE);

  func_iter    fi;     /* Functional unit iterator */
  vsignal*     sig;    /* Pointer to current signal */
  unsigned int hit01;  /* Number of bits that toggled from 0 to 1 */
  unsigned int hit10;  /* Number of bits that toggled from 1 to 0 */
  char*        pname;  /* Printable version of signal name */
  unsigned int i;      /* Loop iterator */

  switch( rtype ) {
    case RPT_TYPE_HIT  :  fprintf( ofile, "    Memories getting 100%% coverage\n\n" );      break;
    case RPT_TYPE_MISS :  fprintf( ofile, "    Memories not getting 100%% coverage\n\n" );  break;
    case RPT_TYPE_EXCL :  fprintf( ofile, "    Memories excluded from coverage\n\n" );      break;
  }

  func_iter_init( &fi, funit, FALSE, TRUE, FALSE );

  while( (sig = func_iter_get_next_signal( &fi )) != NULL ) {

    hit01 = 0;
    hit10 = 0;

    /* Get printable version of the signal name */
    pname = scope_gen_printable( sig->name );

    if( sig->suppl.part.type == SSUPPL_TYPE_MEM ) {

      if( ((sig->suppl.part.excluded == 0) && (rtype != RPT_TYPE_EXCL)) ||
          ((sig->suppl.part.excluded == 1) && (rtype == RPT_TYPE_EXCL)) ) {

        fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );
        fprintf( ofile, "      " );
        if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) ) {
          fprintf( ofile, "(%s)  ", db_gen_exclusion_id( 'M', sig->id ) );
        }
        fprintf( ofile, "Memory name:  %s", pname );
        for( i=0; i<sig->udim_num; i++ ) {
          fprintf( ofile, "[%d:%d]", sig->dim[i].msb, sig->dim[i].lsb );
        }
        fprintf( ofile, "\n" );
        fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

        vector_toggle_count( sig->value, &hit01, &hit10 );

        if( rtype == RPT_TYPE_HIT ) {
          if( (hit01 == sig->value->width) && (hit10 == sig->value->width) ) {
            fprintf( ofile, "      %-24s\n", pname );
          }
        } else {
          exclude_reason* er;
          memory_display_memory( ofile, sig, 0, sig->name, 0, sig->value->width );
          if( (rtype == RPT_TYPE_EXCL) && ((er = exclude_find_exclude_reason( 'M', sig->id, funit )) != NULL) ) {
            report_output_exclusion_reason( ofile, 10, er->reason, TRUE );
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
 Displays the verbose memory coverage results to the specified output stream on
 an instance basis.  The verbose memory coverage includes the signal names,
 the bits that did not receive 100% toggle and addressable elements that did not
 receive 100% write/read coverage during simulation.
*/
static void memory_instance_verbose(
  FILE*       ofile,       /*!< Pointer to file to display coverage results to */
  funit_inst* root,        /*!< Pointer to root of instance functional unit tree to parse */
  char*       parent_inst  /*!< Name of parent instance */
) { PROFILE(MEMORY_INSTANCE_VERBOSE);

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
      ((((root->stat->mem_tog01_hit < root->stat->mem_tog_total) ||
         (root->stat->mem_tog10_hit < root->stat->mem_tog_total) ||
         (root->stat->mem_wr_hit    < root->stat->mem_ae_total)  ||
         (root->stat->mem_rd_hit    < root->stat->mem_ae_total)) && !report_covered) ||
       (root->stat->mem_cov_found && report_covered) ||
       ((root->stat->mem_excluded > 0) && report_exclusions)) ) {

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

    if( (((root->stat->mem_tog01_hit < root->stat->mem_tog_total) ||
          (root->stat->mem_tog10_hit < root->stat->mem_tog_total) ||
          (root->stat->mem_wr_hit    < root->stat->mem_ae_total)  ||
          (root->stat->mem_rd_hit    < root->stat->mem_ae_total)) && !report_covered) || 
        (root->stat->mem_cov_found && report_covered) ) {
      memory_display_verbose( ofile, root->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    }
    if( report_exclusions && (root->stat->mem_excluded > 0) ) {
      memory_display_verbose( ofile, root->funit, RPT_TYPE_EXCL );
    }

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    memory_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

  PROFILE_END;

}

/*!
 Displays the verbose memory coverage results to the specified output stream on
 a functional unit basis (combining functional units that are instantiated multiple times).
 The verbose memory coverage includes the signal names, the bits that
 did not receive 100% toggle, and the addressable elements that did not receive 100%
 write/read coverage during simulation.
*/
static void memory_funit_verbose(
  FILE*       ofile,  /*!< Pointer to file to display coverage results to */
  funit_link* head    /*!< Pointer to head of functional unit list to parse */
) { PROFILE(MEMORY_FUNIT_VERBOSE);

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        ((((head->funit->stat->mem_tog01_hit < head->funit->stat->mem_tog_total) ||
           (head->funit->stat->mem_tog10_hit < head->funit->stat->mem_tog_total) ||
           (head->funit->stat->mem_wr_hit    < head->funit->stat->mem_ae_total)  ||
           (head->funit->stat->mem_rd_hit    < head->funit->stat->mem_ae_total)) && !report_covered) ||
         (head->funit->stat->mem_cov_found && report_covered) ||
         ((head->funit->stat->mem_excluded > 0) && report_exclusions)) ) {

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

      if( (((head->funit->stat->mem_tog01_hit < head->funit->stat->mem_tog_total) ||
            (head->funit->stat->mem_tog10_hit < head->funit->stat->mem_tog_total) ||
            (head->funit->stat->mem_wr_hit    < head->funit->stat->mem_ae_total)  ||
            (head->funit->stat->mem_rd_hit    < head->funit->stat->mem_ae_total)) && !report_covered) || 
         (head->funit->stat->mem_cov_found && report_covered) ) {
        memory_display_verbose( ofile, head->funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
      }
      if( report_exclusions && (head->funit->stat->mem_excluded > 0) ) {
        memory_display_verbose( ofile, head->funit, RPT_TYPE_EXCL );
      }

    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Outputs the memory portion of the report to the given output stream.
*/
void memory_report(
  FILE* ofile,   /*!< Pointer to output file to write */
  bool  verbose  /*!< Specifies if verbose coverage information should be output */
) { PROFILE(MEMORY_REPORT);

  bool       missed_found  = FALSE;  /* If set to TRUE, indicates that untoggled bits were found */
  inst_link* instl;                  /* Pointer to current instance link */
  int        acc_hits01    = 0;      /* Accumulated hits 0 -> 1 count */
  int        acc_hits10    = 0;      /* Accumulated hits 1 -> 0 count */
  int        acc_tog_total = 0;      /* Accumulated bit toggle count */
  int        acc_wr_hits   = 0;      /* Accumulated number of addressable elements written */
  int        acc_rd_hits   = 0;      /* Accumulated number of addressable elements read */
  int        acc_ae_total  = 0;      /* Accumulated number of addressable elements */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   MEMORY COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= memory_toggle_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_hits01, &acc_hits10, &acc_tog_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)memory_display_toggle_instance_summary( ofile, "Accumulated", acc_hits01, acc_hits10, acc_tog_total );

    fprintf( ofile, "\n" );
    fprintf( ofile, "                                                    Addressable elements written         Addressable elements read\n" );
    fprintf( ofile, "                                                   Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= memory_ae_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_wr_hits, &acc_rd_hits, &acc_ae_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)memory_display_ae_instance_summary( ofile, "Accumulated", acc_wr_hits, acc_rd_hits, acc_ae_total );

    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        memory_instance_verbose( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found |= memory_toggle_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits01, &acc_hits10, &acc_tog_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)memory_display_toggle_funit_summary( ofile, "Accumulated", "", acc_hits01, acc_hits10, acc_tog_total );

    fprintf( ofile, "\n" );
    fprintf( ofile, "                                                    Addressable elements written         Addressable elements read\n" );
    fprintf( ofile, "                                                   Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found |= memory_ae_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_wr_hits, &acc_rd_hits, &acc_ae_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)memory_display_ae_funit_summary( ofile, "Accumulated", "", acc_wr_hits, acc_rd_hits, acc_ae_total );

    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      memory_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}

