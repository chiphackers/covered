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
extern char*      top_instance;
extern bool       instance_specified;
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;
extern bool       one_instance_found;
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
        scope_extract_back( lxt2_rd_get_facname( lt, i ), netname, curr_inst_scope[0] );
        db_sync_curr_instance();

        if( g->flags & LXT2_RD_SYM_F_DOUBLE ) {

          /* We ignore real values at the moment */

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
            db_assign_symbol( netname, vcdid( newindx ), ((g->lsb > g->msb) ? g->lsb : g->msb), ((g->lsb > g->msb) ? g->msb : g->lsb) );
          }

        }

      }

      /* Check to see that at least one instance was found */
      if( !one_instance_found ) {

        print_output( "No instances were found in specified VCD file that matched design", FATAL, __FILE__, __LINE__ );

        /* If the -i option was not specified, let the user know */
        if( !instance_specified ) {
          print_output( "  Please use -i option to specify correct hierarchy to top-level module to score",
                        FATAL, __FILE__, __LINE__ );
        } else {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "  Incorrect hierarchical path specified in -i option: %s", top_instance );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
        }

        Throw 0;

      }

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

/*
 $Log$
 Revision 1.28  2008/08/18 23:07:28  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.25.4.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.26  2008/06/27 14:02:03  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.25  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.24  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.23  2008/03/13 10:28:55  phase1geo
 The last of the exception handling modifications.

 Revision 1.22  2008/03/12 05:09:43  phase1geo
 More exception handling updates.  Added TODO item of creating a graduated test list
 from merged CDD files.

 Revision 1.21  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.20  2008/02/29 23:58:19  phase1geo
 Continuing to work on adding exception handling code.

 Revision 1.19  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.18  2008/01/23 04:05:19  phase1geo
 Fixing LXT regressions.  Added code to regression Makefile to remove certain
 diagnostics from being run in LXT regression due to the apparent inability for
 LXT files to contain time-0 only information.  Also contains a fix for bug
 1877853.  Full regressions now run cleanly.

 Revision 1.17  2008/01/22 19:53:22  phase1geo
 First attempt to fix bug 1876681.

 Revision 1.16  2008/01/16 23:10:30  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.15  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.14  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.13  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.12  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.11  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.10  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.9  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.8  2006/05/25 12:11:01  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.7  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.6.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.6  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.5  2006/02/18 06:26:15  phase1geo
 Final file updates prior to covered-20060218 development release.

 Revision 1.4  2006/01/27 15:43:57  phase1geo
 Added ifdefs for HAVE_ZLIB define to allow Covered to compile correctly when
 zlib.h and associated library is unavailable.  Also handle dumpfile reading
 appropriately for this condition.  Moved report file opening after the CDD file
 has been read in to avoid empty report files when a problem is detected in the
 CDD file.

 Revision 1.3  2006/01/26 22:40:13  phase1geo
 Fixing last LXT bug.

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
