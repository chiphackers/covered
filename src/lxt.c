/*!
 \param    lxt.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/25/2006
 \note     This file is based on the lxt2vcd utility that comes with GtkWave-1.3.82
*/

#ifdef HAVE_STRING_H
#include "string.h"
#endif
#include "assert.h"

#include "defines.h"
#include "lxt2_read.h"
#include "symtable.h"
#include "db.h"
#include "util.h"


extern char       user_msg[USER_MSG_LENGTH];
extern char*      top_instance;
extern bool       instance_specified;
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;
extern bool       one_instance_found;
extern char*      curr_inst_scope;


/*! Specifies the last timestamp simulated */
static lxtint64_t vcd_prevtime = -1;

/*! Specifies when we are handling dumping */
bool vcd_blackout;


/*!
 \param value  Unique ID for a specific signal

 \return Returns a unique string ID for the given value
*/
static char* vcdid( int value ) {

  static char buf[16];
  int         i;

  for( i=0; i<15; i++ ) {
    buf[i] = (char)((value % 94) + 33); /* for range 33..126 */
    value  = value / 94;
    if( !value ) {
      buf[i+1] = 0;
      break;
    }
  }

  return( buf );

}

void vcd_callback(struct lxt2_rd_trace **lt, lxtint64_t *pnt_time, lxtint32_t *pnt_facidx, char **pnt_value) {

  struct lxt2_rd_geometry *g = lxt2_rd_get_fac_geometry( *lt, *pnt_facidx );

  /* If this is a new timestamp, perform a simulation */
  if( vcd_prevtime != *pnt_time ) {
    if( vcd_prevtime >= 0 ) {
      db_do_timestep( vcd_prevtime );
    }
    vcd_prevtime = *pnt_time;
  }

  /* Handle dumpon/off information */
  if( !(*pnt_value)[0] ) {
    if( !vcd_blackout ) {
      vcd_blackout = TRUE;
    }
    return;
  } else {
    if( vcd_blackout ) {
      vcd_blackout = FALSE;
    }	
  }

  if( g->flags & LXT2_RD_SYM_F_DOUBLE ) {

    /* We ignore real values for now */

  } else if( g->flags & LXT2_RD_SYM_F_STRING ) {

    /* We ignore string values for now */

  } else {

    if( g->len==1 ) {
      db_set_symbol_char( vcdid( *pnt_facidx ), (*pnt_value)[0] );
    } else {                        
      db_set_symbol_string( vcdid( *pnt_facidx ), *pnt_value );
    }

  }                               

}

/*!
 \param lxt_file  Name of LXT file to read and score

 Main LXT parsing function.  Reads in an LXT-style dumpfile, tells Covered about signal information
 and simulation results.
*/
void lxt_parse( char* lxt_file ) {

  struct lxt2_rd_trace*    lt;       /* LXT read structure */
  int                      i;        /* Loop iterator */
  int                      numfacs;  /* Number of symbols in design */
  struct lxt2_rd_geometry* g;
  lxtint32_t               newindx;
  char                     netname[4096];  /* Name of current signal */

  /* Open LXT file for opening and extract members */
  if( (lt = lxt2_rd_init( lxt_file )) != NULL ) {

    numfacs = lxt2_rd_get_num_facs( lt );

    lxt2_rd_set_fac_process_mask_all( lt );
    lxt2_rd_set_max_block_mem_usage( lt, 0 ); /* no need to cache blocks */

    /* Create initial symbol table */
    vcd_symtab = symtable_create();

    /* Allocate memory for instance scope */
    curr_inst_scope = (char*)malloc_safe( 4096, __FILE__, __LINE__ );

    /* Get symbol information */
    for( i=0; i<numfacs; i++ ) {

      g       = lxt2_rd_get_fac_geometry( lt, i );
      newindx = lxt2_rd_get_alias_root( lt, i );

      /* Extract scope and net name from facility name */
      scope_extract_back( lxt2_rd_get_facname( lt, i ), netname, curr_inst_scope );
      db_sync_curr_instance();

      if( g->flags & LXT2_RD_SYM_F_DOUBLE ) {

        /* We ignore real values at the moment */

      } else if( g->flags & LXT2_RD_SYM_F_STRING ) {

        /* We ignore string values at the moment */

      } else {

        if( g->len == 1 ) {
          if( (g->msb != -1) && (g->msb != 0) ) {
            db_assign_symbol( netname, vcdid( newindx ), g->msb, g->msb );
          } else {
            db_assign_symbol( netname, vcdid( newindx ), 0, 0 );
          }
        } else {
          db_assign_symbol( netname, vcdid( newindx ), g->msb, g->lsb );
        }

      }

    }

    /* Check to see that at least one instance was found */
    if( !one_instance_found ) {

      print_output( "No instances were found in specified VCD file that matched design", FATAL, __FILE__, __LINE__ );

      /* If the -i option was not specified, let the user know */
      if( instance_specified ) {
        print_output( "  Please use -i option to specify correct hierarchy to top-level module to score",
                      FATAL, __FILE__, __LINE__ );
      } else {
        snprintf( user_msg, USER_MSG_LENGTH, "  Incorrect hierarchical path specified in -i option: %s", top_instance );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
      }

      exit( 1 );

    }

    /* Create timestep symbol table array */
    if( vcd_symtab_size > 0 ) {
      timestep_tab = malloc_safe_nolimit( (sizeof( symtable*) * vcd_symtab_size), __FILE__, __LINE__ );
    }

    /* Perform simulation */
    lxt2_rd_iter_blocks( lt, vcd_callback, NULL );

    /* Perform last simulation if necessary */
    if( vcd_prevtime >= 0 ) {
      db_do_timestep( vcd_prevtime );
    }

    /* Deallocate memory */
    symtable_dealloc( vcd_symtab );
    if( timestep_tab != NULL ) {
      free_safe( timestep_tab );
    }

    /* Close LXT file */
    lxt2_rd_close( lt );

  } else {

    print_output( "Unable to open specified LXT file", FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }

}

/*
 $Log$
 Revision 1.2  2006/01/26 17:34:02  phase1geo
 Finished work on lxt2_read.c and fixed bug in LXT-based simulator.  Also adding
 missing lxt.h header file.

 Revision 1.1  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

*/