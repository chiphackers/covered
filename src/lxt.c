/*!
 \param    lxt.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/25/2006
 \note     This file is based on the lxt2vcd utility that comes with GtkWave-1.3.82
*/

#ifdef HAVE_STRING_H
#include "string.h"
#endif

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


struct namehier {
  struct namehier* next;
  char*            name;
  bool             not_final;
};

/*! Specifies the last timestamp simulated */
static lxtint64_t vcd_prevtime = -1;

/*! Specifies when we are handling dumping */
bool vcd_blackout;

static struct namehier* nhold = NULL;

void lxt_free_hier() {

  struct namehier* nhtemp;

  while( nhold ) {

    nhtemp = nhold->next;	
    free_safe( nhold->name );
    free_safe( nhold );
    nhold = nhtemp;

  }

}

/*!
 \param nh1  First hierarchy
 \param nh2  Second hierarchy

 Navigates up and down the scope hierarchy.
*/
static void lxt_diff_hier( struct namehier *nh1, struct namehier *nh2 ) {

  struct namehier* nhtemp;

  if( !nh2 ) {
    while( nh1 && nh1->not_final ) {
      db_set_vcd_scope( nh1->name );
      nh1 = nh1->next;
    }
    return;
  }

  for( ; ; ) {
    if( !nh1->not_final && !nh2->not_final ) { /* both are equal */
      break;
    }
    if( !nh2->not_final ) {	/* old hier is shorter */
      nhtemp = nh1;
      while( nh1 && nh1->not_final ) {
        db_set_vcd_scope( nh1->name );
	nh1 = nh1->next;
      }
      break;
    }
    if( !nh1->not_final ) {	/* new hier is shorter */
      nhtemp = nh2;
      while( nh2 && nh2->not_final ) {
        db_vcd_upscope();
	nh2 = nh2->next;
      }
      break;
    }
    if( strcmp( nh1->name, nh2->name ) ) {
      nhtemp = nh2;				/* prune old hier */
      while( nh2 && nh2->not_final ) {
        db_vcd_upscope();
	nh2 = nh2->next;
      }
      nhtemp = nh1;				/* add new hier */
      while( nh1 && nh1->not_final ) {
        db_set_vcd_scope( nh1->name );
	nh1 = nh1->next;
      }
      break;
    }
    nh1 = nh1->next;
    nh2 = nh2->next;
  }

}

/*
 * output scopedata for a given name if needed, return pointer to name string
 */
char* lxt_find_hier( char* name ) {

  char*            pnt;
  char*            pnt2;
  char*            s;
  int              len;
  struct namehier* nh_head=NULL;
  struct namehier* nh_curr=NULL;
  struct namehier* nhtemp;

  pnt = pnt2 = name;

  for( ; ; ) {

    while( (*pnt2 != '.') && *pnt2 ) pnt2++;

    s = (char*)calloc( 1, (len = pnt2 - pnt) + 1 );
    memcpy(s, pnt, len);
    nhtemp = (struct namehier*)calloc( 1, sizeof( struct namehier ) );
    nhtemp->name = s;

    if( !nh_curr ) {
      nh_head = nh_curr = nhtemp;
    } else {
      nh_curr->next      = nhtemp;
      nh_curr->not_final = 1;
      nh_curr            = nhtemp;
    }

    if( !*pnt2 ) {
      break;
    }

    pnt = (++pnt2);

  }

  lxt_diff_hier( nh_head, nhold );
  lxt_free_hier();
  nhold = nh_head;

  return( nh_curr->name );

}

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

static char* vcd_truncate_bitvec( char* s ) {

  char l, r;

  r = *s;
  if( r == '1' ) {
    return s;
  } else {
    s++;
  }

  for( ; ; s++ ) {
    l = r;
    r = *s;
    if( !r ) {
      return( s - 1 );
    }
    if( l != r ) {
      return( ((l == '0') && (r == '1')) ? s : (s - 1) );
    }
  }

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
      db_set_symbol_string( vcdid( *pnt_facidx ), vcd_truncate_bitvec( *pnt_value ) );
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
  char*                    netname;  /* Name of net to assign symbol to */

  /* Open LXT file for opening and extract members */
  if( (lt = lxt2_rd_init( lxt_file )) != NULL ) {

    numfacs = lxt2_rd_get_num_facs( lt );

    lxt2_rd_set_fac_process_mask_all( lt );
    lxt2_rd_set_max_block_mem_usage( lt, 0 ); /* no need to cache blocks */

    /* Create initial symbol table */
    vcd_symtab = symtable_create();

    /* Get symbol information */
    for( i=0; i<numfacs; i++ ) {

      g       = lxt2_rd_get_fac_geometry( lt, i );
      newindx = lxt2_rd_get_alias_root( lt, i );
      netname = lxt_find_hier( lxt2_rd_get_facname( lt, i ) );

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

    /* Flush any remaining hierarchy if not back to top-level */
    lxt_find_hier( "" );
    lxt_free_hier();

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
 Revision 1.1  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

*/
