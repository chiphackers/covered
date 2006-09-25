/*!
 \file     memory.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/24/2006
*/

#include <stdio.h>

#include "defines.h"
#include "memory.h"
#include "obfuscate.h"
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
 \param sig        Pointer to signal list to traverse for memories
 \param ae_total   Pointer to total number of addressable elements
 \param wr_hit     Pointer to total number of addressable elements written
 \param rd_hit     Pointer to total number of addressable elements read
 \param tog_total  Pointer to total number of bits in memories that can be toggled
 \param tog01_hit  Pointer to total number of bits toggling from 0->1
 \param tog10_hit  Pointer to total number of bits toggling from 1->0
*/
void memory_get_stats( sig_link* sigl, float* ae_total, int* wr_hit, int* rd_hit, float* tog_total, int* tog01_hit, int* tog10_hit ) {

  int    i;       /* Loop iterator */
  int    wr;      /* Number of bits written within an addressable element */
  int    rd;      /* Number of bits read within an addressable element */
  vector vec;     /* Temporary vector used for storing one addressable element */
  int    pwidth;  /* Width of packed portion of memory */

  while( sigl != NULL ) {

    /* Calculate only for memory elements (must contain one or more unpacked dimensions) */
    if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_MEM) && (sigl->sig->udim_num > 0) ) {

      /* Calculate width of smallest addressable element */
      pwidth = 1;
      for( i=(sigl->sig->udim_num); i<(sigl->sig->udim_num + sigl->sig->pdim_num); i++ ) {
        if( sigl->sig->dim[i].msb > sigl->sig->dim[i].lsb ) {
          pwidth *= (sigl->sig->dim[i].msb - sigl->sig->dim[i].lsb) + 1;
        } else {
          pwidth *= (sigl->sig->dim[i].lsb - sigl->sig->dim[i].msb) + 1;
        }
      }

      /* Calculate total number of addressable elements and their write/read information */
      for( i=0; i<sigl->sig->value->width; i+=pwidth ) {
        vector_init( &vec, NULL, pwidth, VTYPE_MEM );
        vec.value = &(sigl->sig->value->value[i]);
        wr = 0;
        rd = 0;
        vector_mem_rw_count( &vec, &wr, &rd );
        if( wr > 0 ) {
          (*wr_hit)++;
        }
        if( rd > 0 ) {
          (*rd_hit)++;
        }
        (*ae_total)++;
      }

      /* Calculate toggle coverage information for the memory */
      *tog_total += sigl->sig->value->width;
      if( sigl->sig->suppl.part.excluded == 1 ) {
        *tog01_hit += sigl->sig->value->width;
        *tog10_hit += sigl->sig->value->width;
      } else {
        vector_toggle_count( sigl->sig->value, tog01_hit, tog10_hit );
      }

    }

    sigl = sigl->next;

  }

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
      vec.value = value;

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
 Revision 1.1  2006/09/25 04:15:03  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

*/

