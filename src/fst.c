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
 \param    fst.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/25/2006
 \note     This file is based on the fst2vcd utility that comes with GtkWave-3.3.6
*/

#ifdef HAVE_STRING_H
#include "string.h"
#endif
#include "assert.h"

#include "defines.h"
#include "fstapi.c"
#include "symtable.h"
#include "db.h"
#include "util.h"


#define FST_ID_NAM_SIZ 			(512)


extern char       user_msg[USER_MSG_LENGTH];
extern symtable*  vcd_symtab;
extern int        vcd_symtab_size;
extern symtable** timestep_tab;


/*! Specifies the last timestamp simulated */
static uint64_t vcd_prevtime = 0;

/*! Specifies the vcd_prevtime value has been assigned by the simulator */
static bool vcd_prevtime_valid = FALSE;

/*! Specifies when we are handling dumping */
static bool vcd_blackout;


/*!
 Handles the definitions portion of the dumpfile.
*/
static void fst_reader_process_hier(
  void* ctx  /*!< Pointer to the current context */
) { PROFILE(FST_READER_PROCESS_HIER);

  struct fstReaderContext *xc = (struct fstReaderContext *)ctx;
  char str[FST_ID_NAM_SIZ+1];
  char str2[FST_ID_NAM_SIZ+1];
  char *pnt;
  int ch, scopetype;
  int vartype, vardir;
  uint32_t len, alias;
  uint32_t msb, lsb;
  uint32_t maxvalpos=0;
  int num_signal_dyn = 65536;

  if( !xc ) {
    return;
  }

  xc->longest_signal_value_len = 32; /* arbitrarily set at 32...this is much longer than an expanded double */

  if( !xc->fh ) {
    fstReaderRecreateHierFile( xc );
  }

  xc->maxhandle = 0;
  xc->num_alias = 0;

  if( xc->signal_lens ) free(xc->signal_lens);
  xc->signal_lens = malloc(num_signal_dyn*sizeof(uint32_t));

  if( xc->signal_typs ) free(xc->signal_typs);
  xc->signal_typs = malloc(num_signal_dyn*sizeof(unsigned char));

  fseeko( xc->fh, 0, SEEK_SET );

  while( !feof( xc->fh ) ) {

    int tag = fgetc(xc->fh);

    switch( tag ) {

      case FST_ST_VCD_SCOPE:
        scopetype = fgetc(xc->fh);
        pnt = str;
        while( (ch = fgetc( xc->fh )) ) {
          *(pnt++) = ch;
        } /* scopename */
        *pnt = 0;
        while( fgetc( xc->fh ) ) { }; /* scopecomp */
        db_set_vcd_scope( str );
        break;

      case FST_ST_VCD_UPSCOPE:
        db_vcd_upscope();
        break;

      case FST_VT_VCD_EVENT:
      case FST_VT_VCD_INTEGER:
      case FST_VT_VCD_PARAMETER:
      case FST_VT_VCD_REAL:
      case FST_VT_VCD_REAL_PARAMETER:
      case FST_VT_VCD_REG:
      case FST_VT_VCD_SUPPLY0:
      case FST_VT_VCD_SUPPLY1:
      case FST_VT_VCD_TIME:
      case FST_VT_VCD_TRI:
      case FST_VT_VCD_TRIAND:
      case FST_VT_VCD_TRIOR:
      case FST_VT_VCD_TRIREG:
      case FST_VT_VCD_TRI0:
      case FST_VT_VCD_TRI1:
      case FST_VT_VCD_WAND:
      case FST_VT_VCD_WIRE:
      case FST_VT_VCD_WOR:
      case FST_VT_VCD_PORT:
      case FST_VT_VCD_ARRAY:
      case FST_VT_VCD_REALTIME:
        vartype = tag;
        vardir  = fgetc( xc->fh ); /* unused in VCD reader */
        pnt     = str;
        while( (ch = fgetc( xc->fh )) ) {
          *(pnt++) = ch;
        } /* varname */
        *pnt  = 0;
        len   = fstReaderVarint32(xc->fh);
        alias = fstReaderVarint32(xc->fh);
        msb   = len - 1;
        lsb   = 0;

        if( !alias ) {
          if( xc->maxhandle == num_signal_dyn ) {
            num_signal_dyn *= 2;
            xc->signal_lens = realloc( xc->signal_lens, num_signal_dyn*sizeof(uint32_t) );
            xc->signal_typs = realloc( xc->signal_typs, num_signal_dyn*sizeof(unsigned char) );
          }
          xc->signal_lens[xc->maxhandle] = len;
          xc->signal_typs[xc->maxhandle] = vartype;

          maxvalpos+=len;
          if( len > xc->longest_signal_value_len ) {
            xc->longest_signal_value_len = len;
          }

          if((vartype == FST_VT_VCD_REAL) || (vartype == FST_VT_VCD_REAL_PARAMETER) || (vartype == FST_VT_VCD_REALTIME)) {
            len = 64;
            msb = 63;
            lsb = 0;
            xc->signal_typs[xc->maxhandle] = FST_VT_VCD_REAL;
          } else {
            if( sscanf( str, "%s \[%d:%d]", str2, &msb, &lsb ) != 3 ) {
              if( sscanf( str, "%s \[%d]", str2, &lsb ) == 2 ) {
                msb = lsb;
                strcpy( str, str2 );
              }
            } else {
              strcpy( str, str2 );
            }
          }
          {
            uint32_t modlen = (vartype != FST_VT_VCD_PORT) ? len : ((len - 2) / 3);
            db_assign_symbol( str, fstVcdID( xc->maxhandle + 1 ), msb, lsb );
          }
          xc->maxhandle++;
        } else {
          if((vartype == FST_VT_VCD_REAL) || (vartype == FST_VT_VCD_REAL_PARAMETER) || (vartype == FST_VT_VCD_REALTIME)) {
            len = 64;
            msb = 63;
            lsb = 0;
            xc->signal_typs[xc->maxhandle] = FST_VT_VCD_REAL;
          } else {
            if( sscanf( str, "%s \[%d:%d]", str2, &msb, &lsb ) != 3 ) {
              if( sscanf( str, "%s \[%d]", str2, &lsb ) == 2 ) {
                msb = lsb;
                strcpy( str, str2 );
              }
            } else {
              strcpy( str, str2 );
            }
          }
          {
            uint32_t modlen = (vartype != FST_VT_VCD_PORT) ? len : ((len - 2) / 3);
            db_assign_symbol( str, fstVcdID( alias ), msb, lsb );
          }
          xc->num_alias++;
        }
        break;

      default:
        break;
    }

  }

  xc->signal_lens = realloc( xc->signal_lens, xc->maxhandle*sizeof(uint32_t) );
  xc->signal_typs = realloc( xc->signal_typs, xc->maxhandle*sizeof(unsigned char) );

  if( xc->process_mask ) { free( xc->process_mask ); }
  xc->process_mask = calloc( 1, (xc->maxhandle+7)/8 );

  if( xc->temp_signal_value_buf ) free( xc->temp_signal_value_buf );
  xc->temp_signal_value_buf = malloc( xc->longest_signal_value_len + 1 );

  xc->var_count = xc->maxhandle + xc->num_alias;

  PROFILE_END;

}

/*!
 \throws anonymous db_do_timestep
*/
static void fst_callback(
  void*                user_callback_data_pointer,
  uint64_t             time,
  fstHandle            facidx,
  const unsigned char* value
) { PROFILE(FST_CALLBACK);

  /* If this is a new timestamp, perform a simulation */
  if( (vcd_prevtime != time) || !vcd_prevtime_valid ) {
    if( vcd_prevtime_valid ) {
      (void)db_do_timestep( vcd_prevtime, FALSE );
    }
    vcd_prevtime       = time;
    vcd_prevtime_valid = TRUE;
  }

  /* Handle dumpon/off information */
  if( !value[0] ) {

    if( !vcd_blackout ) {
      vcd_blackout = TRUE;
    }

  } else {

    if( vcd_blackout ) {
      vcd_blackout = FALSE;
    }

    if( !value[1] ) {
      db_set_symbol_char( fstVcdID( facidx ), value[0] );
    } else {
      db_set_symbol_string( fstVcdID( facidx ), value );
    }

  }

  PROFILE_END;

}

/*!
 Main FST parsing function.  Reads in an FST-style dumpfile, tells Covered about signal information
 and simulation results.
*/
void fst_parse(
  const char* fst_file  /*!< Name of FST file to read and score */
) { PROFILE(FST_PARSE);

  struct fstReaderContext *xc;

  /* Open LXT file for opening and extract members */
  if( (xc = fstReaderOpen( fst_file )) != NULL ) {

    /* Create initial symbol table */
    vcd_symtab = symtable_create();

    Try {

      /* Handle the dumpfile definitions */
      fst_reader_process_hier( xc );

      /* Check to see that at least one instance was found */
      db_check_dumpfile_scopes(); 
          
      /* Create timestep symbol table array */
      if( vcd_symtab_size > 0 ) {
        timestep_tab = malloc_safe_nolimit( sizeof( symtable*) * vcd_symtab_size );
      }
        
      /* Not sure what this does but it seems to be a requirement of the FST reader */
      fstReaderSetFacProcessMaskAll( xc );

      /* Perform simulation */
      fstReaderIterBlocks( xc, fst_callback, NULL, NULL );

      /* Perform last simulation if necessary */
      if( vcd_prevtime_valid ) {
        (void)db_do_timestep( vcd_prevtime, FALSE );
      }

    } Catch_anonymous {
      symtable_dealloc( vcd_symtab );
      free_safe( timestep_tab, (sizeof( symtable* ) * vcd_symtab_size) );
      fstReaderClose( xc );
      Throw 0;
    }
        
    /* Deallocate memory */
    symtable_dealloc( vcd_symtab );
    free_safe( timestep_tab, (sizeof( symtable* ) * vcd_symtab_size) );

    /* Close FST file */
    fstReaderClose( xc );

  } else {

    print_output( "Unable to read data from FST dumpfile.  Exiting without scoring.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

