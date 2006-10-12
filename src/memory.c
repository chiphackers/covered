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
 \file     memory.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/24/2006
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "defines.h"
#include "link.h"
#include "memory.h"
#include "obfuscate.h"
#include "ovl.h"
#include "vector.h"
#include "vsignal.h"
#include "util.h"


extern inst_link*  inst_head;
extern funit_link* funit_head;

extern bool   report_covered;
extern bool   report_instance;
extern char** leading_hierarchies;
extern int    leading_hier_num;
extern bool   leading_hiers_differ;
extern isuppl info_suppl;


/*!
 \param sig          Pointer to signal list to traverse for memories
 \param ae_total     Pointer to total number of addressable elements
 \param wr_hit       Pointer to total number of addressable elements written
 \param rd_hit       Pointer to total number of addressable elements read
 \param tog_total    Pointer to total number of bits in memories that can be toggled
 \param tog01_hit    Pointer to total number of bits toggling from 0->1
 \param tog10_hit    Pointer to total number of bits toggling from 1->0
 \param ignore_excl  If set to TRUE, ignores the current value of the excluded bit

 Calculates the total and hit memory coverage information for the given memory signal.
*/
void memory_get_stat( vsignal* sig, float* ae_total, int* wr_hit, int* rd_hit, float* tog_total, int* tog01_hit, int* tog10_hit,
                      bool ignore_excl ) {

  int    i;       /* Loop iterator */
  int    wr;      /* Number of bits written within an addressable element */
  int    rd;      /* Number of bits read within an addressable element */
  vector vec;     /* Temporary vector used for storing one addressable element */
  int    pwidth;  /* Width of packed portion of memory */

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
    } else {
      vector_init( &vec, NULL, pwidth, VTYPE_MEM );
      vec.value = &(sig->value->value[i]);
      wr = 0;
      rd = 0;
      vector_mem_rw_count( &vec, &wr, &rd );
      if( wr > 0 ) {
        (*wr_hit)++;
      }
      if( rd > 0 ) {
        (*rd_hit)++;
      }
    }
    (*ae_total)++;
  }

  /* Calculate toggle coverage information for the memory */
  *tog_total += sig->value->width;
  if( (sig->suppl.part.excluded == 1) && !ignore_excl ) {
    *tog01_hit += sig->value->width;
    *tog10_hit += sig->value->width;
  } else {
    vector_toggle_count( sig->value, tog01_hit, tog10_hit );
  }

}

/*!
 \param sig        Pointer to signal list to traverse for memories
 \param ae_total   Pointer to total number of addressable elements
 \param wr_hit     Pointer to total number of addressable elements written
 \param rd_hit     Pointer to total number of addressable elements read
 \param tog_total  Pointer to total number of bits in memories that can be toggled
 \param tog01_hit  Pointer to total number of bits toggling from 0->1
 \param tog10_hit  Pointer to total number of bits toggling from 1->0
*/
void memory_get_stats( sig_link* sigl, float* ae_total, int* wr_hit, int* rd_hit, float* tog_total, int* tog01_hit, int* tog10_hit ) {

  while( sigl != NULL ) {

    /* Calculate only for memory elements (must contain one or more unpacked dimensions) */
    if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_MEM) && (sigl->sig->udim_num > 0) ) {

      memory_get_stat( sigl->sig, ae_total, wr_hit, rd_hit, tog_total, tog01_hit, tog10_hit, FALSE );

    }

    sigl = sigl->next;

  }

}

/*!
 \param funit_name  Name of functional unit to find memories in
 \param funit_type  Type of functional unit to find memories in
 \param total       Pointer to total number of memories in the given functional unit
 \param hit         Pointer to number of memories that received 100% coverage of all memory metrics

 \return Returns TRUE if we were able to determine the summary information properly; otherwise,
         returns FALSE to indicate that an error occurred.

 Retrieves memory summary information for a given functional unit made by a GUI request.
*/
bool memory_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit ) {

  bool        retval    = FALSE;  /* Return value for this function */
  func_unit   funit;              /* Functional unit container used for searching */
  funit_link* funitl;             /* Pointer to functional unit link containing the requested functional unit */
  sig_link*   sigl;               /* Pointer to current signal link in list */
  float       ae_total  = 0;      /* Total number of addressable elements */
  int         wr_hit    = 0;      /* Number of addressable elements written */
  int         rd_hit    = 0;      /* Number of addressable elements read */
  float       tog_total = 0;      /* Total number of toggle bits */
  int         tog01_hit = 0;      /* Number of bits toggling from 0->1 */
  int         tog10_hit = 0;      /* Number of bits toggling from 1->0 */

  /* Find the given functional unit in the design */
  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {
  
    /* Initialize total and hit information */
    *total = 0;
    *hit   = 0;

    retval = TRUE;
    sigl   = funitl->funit->sig_head;

    while( sigl != NULL ) {

      /* Only evaluate a signal if it is a memory type */
      if( sigl->sig->suppl.part.type == SSUPPL_TYPE_MEM ) {

        /* Increment the total number */
        (*total)++;

        /* Figure out if this signal is 100% covered in memory coverage */
        memory_get_stat( sigl->sig, &ae_total, &wr_hit, &rd_hit, &tog_total, &tog01_hit, &tog10_hit, FALSE );

        if( (wr_hit > 0) && (rd_hit > 0) && (tog01_hit == tog_total) && (tog10_hit == tog_total) ) {
          (*hit)++;
        } 

      }

      sigl = sigl->next;

    }
    
  }

  return( retval );

}

/*!
 \param str     Pointer to string array to populate with packed dimension information
 \param sig     Pointer to signal that we are solving for
 \param prefix  Prefix string to append to the beginning of the newly created string
 \param dim     Current dimension to solve for

 Creates a string array for each bit in the given signal corresponding to its position
 in the packed array portion.
*/
void memory_create_pdim_bit_array( char** str, vsignal* sig, char* prefix, int dim ) {

  char name[4096];  /* Temporary string */
  int  i;           /* Loop iterator */
  bool last_dim;    /* Specifies if this is the final dimension */

  /* Calculate final dimension */
  last_dim = (dim + 1) == (sig->pdim_num + sig->udim_num);

  if( sig->dim[dim].msb > sig->dim[dim].lsb ) {

    for( i=sig->dim[dim].lsb; i<=sig->dim[dim].msb; i++ ) {
      if( last_dim ) {
        snprintf( name, 4096, "%d", i );
        *str = (char*)realloc( *str, (strlen( *str ) + strlen( prefix ) + strlen( name ) + 4) );
        strcat( *str, prefix );
        strcat( *str, "[" );
        strcat( *str, name );
        strcat( *str, "] " );
      } else {
        snprintf( name, 4096, "%s[%d]", prefix, i );
        memory_create_pdim_bit_array( str, sig, name, (dim + 1) );
      }
    }

  } else {

    for( i=sig->dim[dim].lsb; i>=sig->dim[dim].msb; i-- ) {
      if( last_dim ) {
        snprintf( name, 4096, "%d", i );
        *str = (char*)realloc( *str, (strlen( *str ) + strlen( prefix ) + strlen( name ) + 4) );
        strcat( *str, prefix );
        strcat( *str, "[" );
        strcat( *str, name );
        strcat( *str, "] " );
      } else {
        snprintf( name, 4096, "%s[%d]", prefix, i );
        memory_create_pdim_bit_array( str, sig, name, (dim + 1) );
      }
    }

  }

}

/*!
 \param mem_str           String containing memory information
 \param sig               Pointer to signal to get memory coverage for
 \param value             Pointer to vector value containing the current vector to interrogate (initially this will
                          be the signal value)
 \param prefix            String containing memory prefix to output (initially this will be just the signal name)
 \param dim               Current dimension index (initially this will be 0)
 \param parent_dim_width  Bit width of parent dimension (initially this will be the width of the signal)

 Creates memory array structure for Tcl and stores it into the mem_str parameter.
*/
void memory_get_mem_coverage( char** mem_str, vsignal* sig, vec_data* value, char* prefix, int dim, int parent_dim_width ) {

  char name[4096];  /* Contains signal name */
  int  msb;         /* MSB of current dimension */
  int  lsb;         /* LSB of current dimension */
  int  be;          /* Big endianness of current dimension */
  int  i;           /* Loop iterator */
  int  dim_width;   /* Bit width of current dimension */

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

    vector vec;
    int    tog01;
    int    tog10;
    int    wr;
    int    rd;
    char*  tog01_str;
    char*  tog10_str;
    char   hit_str[2];
    char   int_str[20];
    char*  dim_str;
    char*  entry_str;

    /* Iterate through each addressable element in the current dimension */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      /* Initialize the vector */
      vector_init( &vec, NULL, dim_width, VTYPE_MEM );
      if( be ) {
        vec.value = value + (dim_width * ((msb - lsb) - i));
      } else {
        vec.value = value + (dim_width * i);
      }

      /* Create dimension string */
      snprintf( int_str, 20, "%d", i );
      dim_str = (char*)malloc_safe( (strlen( prefix ) + strlen( int_str ) + 5), __FILE__, __LINE__ );
      snprintf( dim_str, (strlen( prefix ) + strlen( int_str ) + 5), "%s\\[%d\\]", prefix, i );

      /* Get toggle information */
      tog01 = 0;
      tog10 = 0;
      vector_toggle_count( &vec, &tog01, &tog10 );

      /* Get toggle strings */
      tog01_str = vector_get_toggle01( vec.value, vec.width );
      tog10_str = vector_get_toggle10( vec.value, vec.width );

      /* Get write/read information */
      wr = 0;
      rd = 0;
      vector_mem_rw_count( &vec, &wr, &rd );

      /* Output the addressable memory element if it is found to be lacking in coverage */
      if( (tog01 < dim_width) || (tog10 < dim_width) || (wr == 0) || (rd == 0) ) {
        strcpy( hit_str, "0" );
      } else {
        strcpy( hit_str, "1" );
      }

      /* Create a string list for this entry */
      entry_str = (char*)malloc_safe( (strlen( dim_str ) + strlen( hit_str ) + strlen( tog01_str ) + strlen( tog10_str ) + 10), __FILE__, __LINE__ );
      snprintf( entry_str, (strlen( dim_str ) + strlen( hit_str ) + strlen( tog01_str ) + strlen( tog10_str ) + 10), "{%s %s %d %d %s %s}",
                dim_str, hit_str, ((wr == 0) ? 0 : 1), ((rd == 0) ? 0 : 1), tog01_str, tog10_str );

      *mem_str = (char*)realloc( *mem_str, (strlen( *mem_str ) + strlen( entry_str ) + 2) );
      strcat( *mem_str, " " );
      strcat( *mem_str, entry_str );

      /* Deallocate memory */
      free_safe( dim_str );
      free_safe( tog01_str );
      free_safe( tog10_str );
      free_safe( entry_str );

    } 

  /* Otherwise, go down one level */
  } else {

    /* Iterate through each entry in the current dimesion */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      /* Create new prefix */
      snprintf( name, 4096, "%s[%d]", prefix, i );

      if( be ) {
        memory_get_mem_coverage( mem_str, sig, (value + (dim_width * ((msb - lsb) - i))), name, (dim + 1), dim_width );
      } else {
        memory_get_mem_coverage( mem_str, sig, (value + (dim_width * i)),                 name, (dim + 1), dim_width );
      }

    }

  }

}

/*!
 \param funit_name   Name of functional unit containing the given signal
 \param funit_type   Type of functional unit containing the given signal
 \param signame      Name of signal to find memory coverage information for
 \param pdim_info    Pointer to string to store packed dimensional information
 \param udim_info    Pointer to string to store unpacked dimensional information
 \param memory_info  Pointer to string to store memory information into
 \param excluded     Pointer to excluded indicator to store

 \return Returns TRUE if the given signal was found and its memory coverage information properly
         stored in the given pointers; otherwise, returns FALSE to indicate that an error occurred.

 Retrieves memory coverage information for the given signal in the specified functional unit.
*/
bool memory_get_coverage( char* funit_name, int funit_type, char* signame,
                          char** pdim_str, char** pdim_array, char** udim_str, char** memory_info, int* excluded ) {

  bool        retval = FALSE;  /* Return value for this function */
  func_unit   funit;           /* Functional unit container used for searching */
  funit_link* funitl;          /* Pointer to found functional unit link */
  vsignal     sig;             /* Signal container used for searching */
  sig_link*   sigl;            /* Pointer to found signal link */
  int         i;               /* Loop iterator */
  char        tmp1[20];        /* Temporary string holder */
  char        tmp2[20];        /* Temporary string holder */

  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    sig.name = signame;

    if( (sigl = sig_link_find( &sig, funitl->funit->sig_head )) != NULL ) {

      /* Allocate and populate the pdim_array and pdim_width parameters */
      *pdim_array = (char*)malloc_safe( 1, __FILE__, __LINE__ );
      (*pdim_array)[0] = '\0';
      memory_create_pdim_bit_array( pdim_array, sigl->sig, "", sigl->sig->pdim_num );

      /* Allocate and populate the pdim_str string */
      *pdim_str = NULL;
      for( i=sigl->sig->udim_num; i<(sigl->sig->pdim_num + sigl->sig->udim_num); i++ ) {
        snprintf( tmp1, 20, "%d", sigl->sig->dim[i].msb );
        snprintf( tmp2, 20, "%d", sigl->sig->dim[i].lsb );
        *pdim_str = (char*)realloc( *pdim_str, (strlen( tmp1 ) + strlen( tmp2 ) + 4) );
        if( i == sigl->sig->udim_num ) {
          snprintf( *pdim_str, (strlen( tmp1 ) + strlen( tmp2 ) + 4), "[%s:%s]", tmp1, tmp2 );
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
      for( i=0; i<sigl->sig->udim_num; i++ ) {
        snprintf( tmp1, 20, "%d", sigl->sig->dim[i].msb );
        snprintf( tmp2, 20, "%d", sigl->sig->dim[i].lsb );
        *udim_str = (char*)realloc( *udim_str, (strlen( tmp1 ) + strlen( tmp2 ) + 4) );
        if( i == 0 ) {
          snprintf( *udim_str, (strlen( tmp1 ) + strlen( tmp2 ) + 4), "[%s:%s]", tmp1, tmp2 );
        } else {
          strcat( *udim_str, "[" );
          strcat( *udim_str, tmp1 );
          strcat( *udim_str, ":" );
          strcat( *udim_str, tmp2 );
          strcat( *udim_str, "]" );
        }
      }

      /* Populate the memory_info string */
      *memory_info = (char*)malloc_safe( 1, __FILE__, __LINE__ );
      (*memory_info)[0] = '\0';
      memory_get_mem_coverage( memory_info, sigl->sig, sigl->sig->value->value, "", 0, sigl->sig->value->width );

      /* Populate the excluded value */
      *excluded = sigl->sig->suppl.part.excluded;
      
      retval = TRUE;

    }

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit to collect memories for
 \param funit_type  Type of functional unit to collect memories for
 \param cov         Set to 0 to get uncovered memories or 1 to get covered memories
 \param head        Pointer to head of signal list containing retrieved signals
 \param tail        Pointer to tail of signal list containing retrieved signals

 \return Returns TRUE if we were able to find the given functional unit; otherwise, returns
         FALSE to indicate that an error occurred.

 Collects all signals that are memories and match the given coverage type and stores them
 in the given signal list.
*/
bool memory_collect( char* funit_name, int funit_type, int cov, sig_link** head, sig_link** tail ) {

  bool        retval = FALSE;  /* Return value for this function */
  func_unit   funit;           /* Functional unit used for searching */
  funit_link* funitl;          /* Pointer to found functional unit link */
  sig_link*   sigl;            /* Pointer to current signal link being evaluated */
  float       ae_total  = 0;   /* Total number of addressable elements */
  int         wr_hit    = 0;   /* Total number of addressable elements written */
  int         rd_hit    = 0;   /* Total number of addressable elements read */
  float       tog_total = 0;   /* Total number of toggle bits */
  int         hit01     = 0;   /* Number of bits that toggled from 0 to 1 */
  int         hit10     = 0;   /* Number of bits that toggled from 1 to 0 */

  /* First, find functional unit in functional unit array */
  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    sigl = funitl->funit->sig_head;
    retval = TRUE;

    while( sigl != NULL ) {

      if( sigl->sig->suppl.part.type == SSUPPL_TYPE_MEM ) {

        ae_total = 0;
   
        memory_get_stat( sigl->sig, &ae_total, &wr_hit, &rd_hit, &tog_total, &hit01, &hit10, TRUE );

        /* If this signal meets the coverage requirement, add it to the signal list */
        if( ((cov == 1) && (wr_hit > 0) && (rd_hit > 0) && (hit01 == tog_total) && (hit10 == tog_total)) ||
            ((cov == 0) && ((wr_hit == 0) || (rd_hit == 0) || (hit01 < tog_total) || (hit10 < tog_total))) ) {

          sig_link_add( sigl->sig, head, tail );

        }

      }

      sigl = sigl->next;

    }

  }


  return( retval );

}

/*!
 \param ofile        File to output coverage information to.
 \param root         Instance node in the functional unit instance tree being evaluated.
 \param parent_inst  Name of parent instance.

 \return Returns TRUE if any bits were found to be not toggled; otherwise, returns FALSE.

 Displays the memory instance summarization to the specified file.  Recursively
 iterates through functional unit instance tree, outputting the toggle information that
 is found at that instance.
*/
bool memory_toggle_instance_summary( FILE* ofile, funit_inst* root, char* parent_inst ) { 

  funit_inst* curr;           /* Pointer to current child functional unit instance of this node */
  float       percent01;      /* Percentage of bits toggling from 0 -> 1 */
  float       percent10;      /* Percentage of bits toggling from 1 -> 0 */
  float       miss01  = 0;    /* Number of bits that did not toggle from 0 -> 1 */
  float       miss10  = 0;    /* Number of bits that did not toggle from 1 -> 0 */
  char        tmpname[4096];  /* Temporary name holder for instance */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Get printable version of this instance */
  pname = scope_gen_printable( root->name );
  if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname );

  if( root->stat->show && ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    /* Calculate for toggle01 information */
    if( root->stat->mem_tog_total == 0 ) {
      percent01 = 100;
    } else {
      percent01 = ((root->stat->mem_tog01_hit / root->stat->mem_tog_total) * 100);
    }
    miss01 = (root->stat->mem_tog_total - root->stat->mem_tog01_hit);

    /* Calculate for toggle10 information */
    if( root->stat->mem_tog_total == 0 ) {
      percent10 = 100;
    } else {
      percent10 = ((root->stat->mem_tog10_hit / root->stat->mem_tog_total) * 100);
    }
    miss10 = (root->stat->mem_tog_total - root->stat->mem_tog10_hit);

    /* Output toggle information */
    fprintf( ofile, "  %-43.43s    %5d/%5.0f/%5.0f      %3.0f%%         %5d/%5.0f/%5.0f      %3.0f%%\n",
             tmpname,
             root->stat->mem_tog01_hit,
             miss01,
             root->stat->mem_tog_total,
             percent01,
             root->stat->mem_tog10_hit,
             miss10,
             root->stat->mem_tog_total,
             percent10 ); 

  } 

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss01 = miss01 + memory_toggle_instance_summary( ofile, curr, tmpname );
      curr = curr->next;
    }

  }

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \param ofile        File to output coverage information to.
 \param root         Instance node in the functional unit instance tree being evaluated.
 \param parent_inst  Name of parent instance.

 \return Returns TRUE if any bits were found to be not toggled; otherwise, returns FALSE.

 Displays the memory instance summarization to the specified file.  Recursively
 iterates through functional unit instance tree, outputting the toggle information that
 is found at that instance.
*/
bool memory_ae_instance_summary( FILE* ofile, funit_inst* root, char* parent_inst ) { 

  funit_inst* curr;           /* Pointer to current child functional unit instance of this node */
  float       percent_wr;     /* Percentage of addressable elements written */
  float       percent_rd;     /* Percentage of addressable elements read */
  float       miss_wr = 0;    /* Number of addressable elements that were not written */
  float       miss_rd = 0;    /* Number of addressable elements that were not read */
  char        tmpname[4096];  /* Temporary name holder for instance */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Get printable version of this instance */   pname = scope_gen_printable( root->name );
   if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname );

  if( root->stat->show && ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    /* Calculate for addressable element write information */
    if( root->stat->mem_ae_total == 0 ) {
      percent_wr = 100;
    } else {
      percent_wr = ((root->stat->mem_wr_hit / root->stat->mem_ae_total) * 100);
    }
    miss_wr = (root->stat->mem_ae_total - root->stat->mem_wr_hit);

    /* Calculate for addressable element read information */
    if( root->stat->mem_ae_total == 0 ) {
      percent_rd = 100;
    } else {
      percent_rd = ((root->stat->mem_rd_hit / root->stat->mem_ae_total) * 100);
    }
    miss_rd = (root->stat->mem_ae_total - root->stat->mem_rd_hit);

    /* Output toggle information */
    fprintf( ofile, "  %-43.43s    %5d/%5.0f/%5.0f      %3.0f%%         %5d/%5.0f/%5.0f      %3.0f%%\n",
             tmpname,
             root->stat->mem_wr_hit,
             miss_wr,
             root->stat->mem_ae_total,
             percent_wr,
             root->stat->mem_rd_hit,
             miss_rd,
             root->stat->mem_ae_total,
             percent_rd ); 

  } 

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_wr = miss_wr + memory_ae_instance_summary( ofile, curr, tmpname );
      curr = curr->next;
    }

  }

  return( (miss_wr > 0) || (miss_rd > 0) );

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of functional unit list to parse.

 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Iterates through the functional unit list displaying the memory toggle coverage summary for
 each functional unit.
*/
bool memory_toggle_funit_summary( FILE* ofile, funit_link* head ) { 

  float percent01;           /* Percentage of bits that toggled from 0 to 1 */
  float percent10;           /* Percentage of bits that toggled from 1 to 0 */
  float miss01;              /* Number of bits that did not toggle from 0 to 1 */
  float miss10;              /* Number of bits that did not toggle from 1 to 0 */
  float miss_found = FALSE;  /* Set to TRUE if missing toggles were found */
  char* pname;               /* Printable version of the functional unit name */

  while( head != NULL ) {

    /* Calculate for toggle01 */
    if( head->funit->stat->mem_tog_total == 0 ) {
      percent01 = 100;
    } else {
      percent01 = ((head->funit->stat->mem_tog01_hit / head->funit->stat->mem_tog_total) * 100);
    }
    miss01 = (head->funit->stat->mem_tog_total - head->funit->stat->mem_tog01_hit);

    /* Calculate for toggle10 */
    if( head->funit->stat->mem_tog_total == 0 ) {
      percent10 = 100;
    } else {
      percent10 = ((head->funit->stat->mem_tog10_hit / head->funit->stat->mem_tog_total) * 100);
    }
    miss10 = (head->funit->stat->mem_tog_total - head->funit->stat->mem_tog10_hit); 
    miss_found = ((miss01 > 0) || (miss10 > 0)) ? TRUE : miss_found;

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( head->funit->name );

      fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5.0f/%5.0f      %3.0f%%         %5d/%5.0f/%5.0f      %3.0f%%\n",
               pname,
               get_basename( obf_file( head->funit->filename ) ),
               head->funit->stat->mem_tog01_hit,
               miss01,
               head->funit->stat->mem_tog_total,
               percent01,
               head->funit->stat->mem_tog10_hit,
               miss10,
               head->funit->stat->mem_tog_total,
               percent10 );

      free_safe( pname );

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of functional unit list to parse.

 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Iterates through the functional unit list displaying the memory toggle coverage summary for
 each functional unit.
*/
bool memory_ae_funit_summary( FILE* ofile, funit_link* head ) { 

  float percent_wr;          /* Percentage of addressable elements that were written */
  float percent_rd;          /* Percentage of addressable elements that were read */
  float miss_wr;             /* Number of addressable elements that were not written */
  float miss_rd;             /* Number of addressable elements that were not read */
  float miss_found = FALSE;  /* Set to TRUE if missing toggles were found */
  char* pname;               /* Printable version of the functional unit name */

  while( head != NULL ) {

    /* Calculate for writes */
    if( head->funit->stat->mem_ae_total == 0 ) {
      percent_wr = 100;
    } else {
      percent_wr = ((head->funit->stat->mem_wr_hit / head->funit->stat->mem_ae_total) * 100);
    }
    miss_wr = (head->funit->stat->mem_ae_total - head->funit->stat->mem_wr_hit);

    /* Calculate for reads */
    if( head->funit->stat->mem_ae_total == 0 ) {
      percent_rd = 100;
    } else {
      percent_rd = ((head->funit->stat->mem_rd_hit / head->funit->stat->mem_ae_total) * 100);
    }
    miss_rd = (head->funit->stat->mem_ae_total - head->funit->stat->mem_rd_hit); 
    miss_found = ((miss_wr > 0) || (miss_rd > 0)) ? TRUE : miss_found;

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( head->funit->name );

      fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5.0f/%5.0f      %3.0f%%         %5d/%5.0f/%5.0f      %3.0f%%\n",
               pname,
               get_basename( obf_file( head->funit->filename ) ),
               head->funit->stat->mem_wr_hit,
               miss_wr,
               head->funit->stat->mem_ae_total,
               percent_wr,
               head->funit->stat->mem_rd_hit,
               miss_rd,
               head->funit->stat->mem_ae_total,
               percent_rd );

      free_safe( pname );

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile             Pointer to output file
 \param sig               Pointer to the current memory element to output
 \param value             Pointer to vector value containing the current vector to interrogate (initially this will
                          be the signal value)
 \param prefix            String containing memory prefix to output (initially this will be just the signal name)
 \param dim               Current dimension index (initially this will be 0)
 \param parent_dim_width  Bit width of parent dimension (initially this will be the width of the signal)

 Outputs the contents of the given memory in verbose output format.
*/
void memory_display_memory( FILE* ofile, vsignal* sig, vec_data* value, char* prefix, int dim, int parent_dim_width ) {

  char name[4096];  /* Contains signal name */
  int  msb;         /* MSB of current dimension */
  int  lsb;         /* LSB of current dimension */
  int  be;          /* Big endianness of current dimension */
  int  i;           /* Loop iterator */
  int  dim_width;   /* Bit width of current dimension */

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

    vector vec;
    int    tog01;
    int    tog10;
    int    wr;
    int    rd;

    /* Iterate through each addressable element in the current dimension */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      /* Initialize the vector */
      vector_init( &vec, NULL, dim_width, VTYPE_MEM );
      if( be ) {
        vec.value = value + (dim_width * ((msb - lsb) - i));
      } else {
        vec.value = value + (dim_width * i);
      }

      /* Get toggle information */
      tog01 = 0;
      tog10 = 0;
      vector_toggle_count( &vec, &tog01, &tog10 );

      /* Get write/read information */
      wr = 0;
      rd = 0;
      vector_mem_rw_count( &vec, &wr, &rd );

      /* Output the addressable memory element if it is found to be lacking in coverage */
      if( (tog01 < dim_width) || (tog10 < dim_width) || (wr == 0) || (rd == 0) ) {

        int j;

        snprintf( name, 4096, "%s[%d]", prefix, i );
        fprintf( ofile, "        %s  Written: %d  0->1: ", name, ((wr == 0) ? 0 : 1) );
        vector_display_toggle01( vec.value, vec.width, ofile );
        fprintf( ofile, "\n" );
        fprintf( ofile, "        " );
        for( j=0; j<strlen( name ); j++ ) {
          fprintf( ofile, "." );
        }
        fprintf( ofile, "  Read   : %d  1->0: ", ((rd == 0) ? 0 : 1) );
        vector_display_toggle10( vec.value, vec.width, ofile );
        fprintf( ofile, " ...\n" );
      }

    } 

  /* Otherwise, go down one level */
  } else {

    /* Iterate through each entry in the current dimesion */
    for( i=0; i<((msb - lsb) + 1); i++ ) {

      /* Create new prefix */
      snprintf( name, 4096, "%s[%d]", prefix, i );

      if( be ) {
        memory_display_memory( ofile, sig, (value + (dim_width * ((msb - lsb) - i))), name, (dim + 1), dim_width );
      } else {
        memory_display_memory( ofile, sig, (value + (dim_width * i)),                 name, (dim + 1), dim_width );
      }

    }

  }

}

/*!
 \param ofile  Pointer to file to output results to.
 \param sigl   Pointer to signal list head.

 Displays the memories that did not achieve 100% toggle coverage and/or 100%
 write/read coverage to standard output from the specified signal list.
*/
void memory_display_verbose( FILE* ofile, sig_link* sigl ) {

  sig_link* curr_sig;   /* Pointer to current signal link being evaluated */
  int       hit01;      /* Number of bits that toggled from 0 to 1 */
  int       hit10;      /* Number of bits that toggled from 1 to 0 */
  char*     pname;      /* Printable version of signal name */
  int       i;          /* Loop iterator */

  if( report_covered ) {
    fprintf( ofile, "    Memories getting 100%% coverage\n\n" );
  } else {
    fprintf( ofile, "    Memories not getting 100%% coverage\n\n" );
  }

  curr_sig = sigl;

  while( curr_sig != NULL ) {

    hit01 = 0;
    hit10 = 0;

    /* Get printable version of the signal name */
    pname = scope_gen_printable( curr_sig->sig->name );

    if( curr_sig->sig->suppl.part.type == SSUPPL_TYPE_MEM ) {

      fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );
      fprintf( ofile, "      Memory name:  %s", pname );
      for( i=0; i<curr_sig->sig->udim_num; i++ ) {
        fprintf( ofile, "[%d:%d]", curr_sig->sig->dim[i].msb, curr_sig->sig->dim[i].lsb );
      }
      fprintf( ofile, "\n" );
      fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

      vector_toggle_count( curr_sig->sig->value, &hit01, &hit10 );

      if( report_covered ) {

        if( (hit01 == curr_sig->sig->value->width) && (hit10 == curr_sig->sig->value->width) ) {

          fprintf( ofile, "      %-24s\n", pname );

        }

      } else {

        memory_display_memory( ofile, curr_sig->sig, curr_sig->sig->value->value, curr_sig->sig->name, 0, curr_sig->sig->value->width );

      }

    }

    free_safe( pname );

    curr_sig = curr_sig->next;

  }

}

/*!
 \param ofile        Pointer to file to display coverage results to.
 \param root         Pointer to root of instance functional unit tree to parse.
 \param parent_inst  Name of parent instance.

 Displays the verbose memory coverage results to the specified output stream on
 an instance basis.  The verbose memory coverage includes the signal names,
 the bits that did not receive 100% toggle and addressable elements that did not
 receive 100% write/read coverage during simulation.
*/
void memory_instance_verbose( FILE* ofile, funit_inst* root, char* parent_inst ) {

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder of instance */
  char*       pname;          /* Printable version of the name */

  assert( root != NULL );

  /* Get printable version of the signal */
  pname = scope_gen_printable( root->name );

  if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname );

  if( (root->stat->mem_tog01_hit < root->stat->mem_tog_total) ||
      (root->stat->mem_tog10_hit < root->stat->mem_tog_total) ||
      (root->stat->mem_wr_hit    < root->stat->mem_ae_total)  ||
      (root->stat->mem_rd_hit    < root->stat->mem_ae_total) ) {

    fprintf( ofile, "\n" );
    switch( root->funit->type ) {
      case FUNIT_MODULE      :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_NAMED_BLOCK :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_FUNCTION    :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_TASK        :  fprintf( ofile, "    Task: " );         break;
      default                :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    pname = scope_gen_printable( root->funit->name );
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, obf_file( root->funit->filename ), tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );
    free_safe( pname );

    memory_display_verbose( ofile, root->funit->sig_head );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    memory_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of functional unit list to parse.

 Displays the verbose memory coverage results to the specified output stream on
 a functional unit basis (combining functional units that are instantiated multiple times).
 The verbose memory coverage includes the signal names, the bits that
 did not receive 100% toggle, and the addressable elements that did not receive 100%
 write/read coverage during simulation.
*/
void memory_funit_verbose( FILE* ofile, funit_link* head ) {

  while( head != NULL ) {

    if( (head->funit->stat->mem_tog01_hit < head->funit->stat->mem_tog_total) ||
        (head->funit->stat->mem_tog10_hit < head->funit->stat->mem_tog_total) ||
        (head->funit->stat->mem_wr_hit    < head->funit->stat->mem_ae_total)  ||
        (head->funit->stat->mem_rd_hit    < head->funit->stat->mem_ae_total) ) {

      fprintf( ofile, "\n" );
      switch( head->funit->type ) {
        case FUNIT_MODULE      :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_NAMED_BLOCK :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_FUNCTION    :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_TASK        :  fprintf( ofile, "    Task: " );         break;
        default                :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", obf_funit( head->funit->name ), obf_file( head->funit->filename ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      memory_display_verbose( ofile, head->funit->sig_head );

    }

    head = head->next;

  }

}

/*!
 \param ofile    Pointer to output file to write
 \param verbose  Specifies if verbose coverage information should be output
*/
void memory_report( FILE* ofile, bool verbose ) {

  bool       missed_found = FALSE;  /* If set to TRUE, indicates that untoggled bits were found */
  char       tmp[4096];             /* Temporary string value */
  inst_link* instl;                 /* Pointer to current instance link */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   MEMORY COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( leading_hiers_differ ) {
      strcpy( tmp, "<NA>" );
    } else {
      assert( leading_hier_num > 0 );
      strcpy( tmp, leading_hierarchies[0] );
    }

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = inst_head;
    while( instl != NULL ) {
      missed_found |= memory_toggle_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
      instl = instl->next;
    }

    fprintf( ofile, "\n" );
    fprintf( ofile, "                                                    Addressable elements written         Addressable elements read\n" );
    fprintf( ofile, "                                                   Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = inst_head;
    while( instl != NULL ) {
      missed_found |= memory_ae_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
      instl = instl->next;
    }

    if( verbose && missed_found ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = inst_head;
      while( instl != NULL ) {
        memory_instance_verbose( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found |= memory_toggle_funit_summary( ofile, funit_head );

    fprintf( ofile, "\n" );
    fprintf( ofile, "                                                    Addressable elements written         Addressable elements read\n" );
    fprintf( ofile, "                                                   Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found |= memory_ae_funit_summary( ofile, funit_head );

    if( verbose && missed_found ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      memory_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}


/*
 $Log$
 Revision 1.7  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.6  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.5  2006/10/02 22:41:00  phase1geo
 Lots of bug fixes to memory coverage functionality for GUI.  Memory coverage
 should now be working correctly.  We just need to update the GUI documentation
 as well as other images for the new feature add.

 Revision 1.4  2006/09/27 21:38:35  phase1geo
 Adding code to interract with data in memory coverage verbose window.  Majority
 of code is in place; however, this has not been thoroughly debugged at this point.
 Adding mem3 diagnostic for GUI debugging purposes and checkpointing.

 Revision 1.3  2006/09/26 22:36:38  phase1geo
 Adding code for memory coverage to GUI and related files.  Lots of work to go
 here so we are checkpointing for the moment.

 Revision 1.2  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.1  2006/09/25 04:15:03  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

*/

