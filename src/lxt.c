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
 \param    lxt.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;
extern char**     curr_inst_scope;
extern int        curr_inst_scope_size;


/*! Specifies the last timestamp simulated */
static lxtint64_t vcd_prevtime = 0;

/*! Specifies the vcd_prevtime value has been assigned by the simulator */
static bool vcd_prevtime_valid = FALSE;

/*! Specifies when we are handling dumping */
static bool vcd_blackout;


/*!
 \return Returns a unique string ID for the given value
*/
static char* vcdid(
  int value  /*!< Unique ID for a specific signal */
) { PROFILE(VCDID);

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

  PROFILE_END;

  return( buf );

}

/*!
 \throws anonymous db_do_timestep
*/
static void vcd_callback(
  struct lxt2_rd_trace** lt,
  lxtint64_t*            pnt_time,
  lxtint32_t*            pnt_facidx,
  char**                 pnt_value
) { PROFILE(VCD_CALLBACK);

  struct lxt2_rd_geometry *g = lxt2_rd_get_fac_geometry( *lt, *pnt_facidx );

  /* If this is a new timestamp, perform a simulation */
  if( (vcd_prevtime != *pnt_time) || !vcd_prevtime_valid ) {
    if( vcd_prevtime_valid ) {
      (void)db_do_timestep( vcd_prevtime, FALSE );
    }
    vcd_prevtime       = *pnt_time;
    vcd_prevtime_valid = TRUE;
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

    db_set_symbol_string( vcdid( *pnt_facidx ), *pnt_value );

  } else if( g->flags & LXT2_RD_SYM_F_STRING ) {

    /* We ignore string values for now */

  } else {

    if( g->len==1 ) {
      db_set_symbol_char( vcdid( *pnt_facidx ), (*pnt_value)[0] );
    } else {                        
      db_set_symbol_string( vcdid( *pnt_facidx ), *pnt_value );
    }

  }                               

  PROFILE_END;

}

/*!
 \throws anonymous db_do_timestep Throw Throw Throw lxt2_rd_iter_blocks

 Main LXT parsing function.  Reads in an LXT-style dumpfile, tells Covered about signal information
 and simulation results.
*/
void lxt_parse(
  const char* lxt_file  /*!< Name of LXT file to read and score */
) { PROFILE(LXT_PARSE);

  struct lxt2_rd_trace*    lt;             /* LXT read structure */
  int                      i;              /* Loop iterator */
  int                      numfacs;        /* Number of symbols in design */
  struct lxt2_rd_geometry* g;
  lxtint32_t               newindx;
  char                     netname[4096];  /* Name of current signal */
  char                     tmpname[4096];

  /* Open LXT file for opening and extract members */
  if( (lt = lxt2_rd_init( lxt_file )) != NULL ) {

    numfacs = lxt2_rd_get_num_facs( lt );

    (void)lxt2_rd_set_fac_process_mask_all( lt );
    (void)lxt2_rd_set_max_block_mem_usage( lt, 0 ); /* no need to cache blocks */

    /* Create initial symbol table */
    vcd_symtab = symtable_create();

    /* Allocate memory for instance scope */
    curr_inst_scope      = (char**)malloc_safe( sizeof( char* ) );
    curr_inst_scope[0]   = (char*)malloc_safe( 4096 );
    curr_inst_scope_size = 1;

    Try {

      /* Get symbol information */
      for( i=0; i<numfacs; i++ ) {

        g       = lxt2_rd_get_fac_geometry( lt, i );
        newindx = lxt2_rd_get_alias_root( lt, i );

        /* Extract scope and net name from facility name */
        strcpy( tmpname, lxt2_rd_get_facname( lt, i ) );
        if( strchr( tmpname, '\\' ) != NULL ) {
          strcat( tmpname, " " );
        }
        scope_extract_back( tmpname, netname, curr_inst_scope[0] );
        db_sync_curr_instance();

        if( g->flags & LXT2_RD_SYM_F_DOUBLE ) {

          db_assign_symbol( netname, vcdid( newindx ), 63, 0 );

        } else if( g->flags & LXT2_RD_SYM_F_STRING ) {

          /* We ignore string values at the moment */

        } else {

          if( g->len == 1 ) {
            if( g->msb != 0 ) {
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
      db_check_dumpfile_scopes();

      /* Create timestep symbol table array */
      if( vcd_symtab_size > 0 ) {
        timestep_tab = malloc_safe_nolimit( sizeof( symtable*) * vcd_symtab_size );
      }

      /* Perform simulation */
      (void)lxt2_rd_iter_blocks( lt, vcd_callback, NULL );

      /* Perform last simulation if necessary */
      if( vcd_prevtime_valid ) {
        (void)db_do_timestep( vcd_prevtime, FALSE );
      }

    } Catch_anonymous {
      assert( curr_inst_scope_size == 1 );
      free_safe( curr_inst_scope[0], 4096 );
      free_safe( curr_inst_scope, sizeof( char* ) );
      curr_inst_scope      = NULL;
      curr_inst_scope_size = 0;
      symtable_dealloc( vcd_symtab );
      free_safe( timestep_tab, (sizeof( symtable* ) * vcd_symtab_size) );
      lxt2_rd_close( lt );
      Throw 0;
    }

    /* Deallocate memory */
    assert( curr_inst_scope_size == 1 );
    free_safe( curr_inst_scope[0], 4096 );
    free_safe( curr_inst_scope, sizeof( char* ) );
    curr_inst_scope      = NULL;
    curr_inst_scope_size = 0;
    symtable_dealloc( vcd_symtab );
    free_safe( timestep_tab, (sizeof( symtable* ) * vcd_symtab_size) );

    /* Close LXT file */
    lxt2_rd_close( lt );

  } else {

    print_output( "Unable to read data from LXT dumpfile.  Exiting without scoring.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

