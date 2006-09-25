/*!
 \file     memory.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/24/2006
*/

#include <stdio.h>

#include "defines.h"
#include "memory.h"


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
 \param write_hit  Pointer to total number of addressable elements written
 \param read_hit   Pointer to total number of addressable elements read
 \param tog_total  Pointer to total number of bits in memories that can be toggled
 \param tog01_hit  Pointer to total number of bits toggling from 0->1
 \param tog10_hit  Pointer to total number of bits toggling from 1->0
*/
void memory_get_stats( sig_link* sigl, float* ae_total, int* write_hit, int* read_hit, float* tog_total, int* tog01_hit, int* tog10_hit ) {

  int aes;  /* Total number of address elements for a single memory */
  int i;    /* Loop iterator */

  while( sigl != NULL ) {

    /* Calculate only for memory elements (must contain one or more unpacked dimensions) */
    if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_MEM) && (sigl->sig->udim_num > 0) ) {

      /* Calculate number of addressable elements and the write/read hit information */
      aes = 1;
      for( i=0; i<sigl->sig->udim_num; i++ ) {
        if( sigl->sig->dim[i].msb > sigl->sig->dim[i].lsb ) {
          aes = aes * ((sigl->sig->dim[i].msb - sigl->sig->dim[i].lsb) + 1);
        } else {
          aes = aes * ((sigl->sig->dim[i].lsb - sigl->sig->dim[i].msb) + 1);
        }
      }
      *ae_total += aes;

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

    fprintf( ofile, "Instance                                                   Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "                                                   Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = inst_head;
    while( instl != NULL ) {
//      missed_found |= memory_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
      instl = instl->next;
    }

    if( verbose && missed_found ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = inst_head;
      while( instl != NULL ) {
//        memory_instance_verbose( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "Module/Task/Function      Filename                         Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "                                                   Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

//    missed_found = memory_funit_summary( ofile, funit_head );

    if( verbose && missed_found ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
//      memory_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}


/*
 $Log$
*/

