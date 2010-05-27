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
 \file     db.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/7/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <assert.h>

#include "attr.h"
#include "binding.h"
#include "db.h"
#include "defines.h"
#include "enumerate.h"
#include "expr.h"
#include "fsm.h"
#include "func_unit.h"
#include "gen_item.h"
#include "info.h"
#include "instance.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "param.h"
#include "race.h"
#include "scope.h"
#include "sim.h"
#include "stat.h"
#include "statement.h"
#include "static.h"
#include "symtable.h"
#include "tree.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern char*       top_module;
extern str_link*   no_score_head;
extern char        user_msg[USER_MSG_LENGTH];
extern isuppl      info_suppl;
extern uint64      timestep_update;
extern bool        debug_mode;
extern int*        fork_block_depth;
extern int         fork_depth;
extern int         block_depth;
/*@null@*/extern tnode*      def_table;
extern int         generate_mode;
extern int         generate_top_mode;
extern int         generate_expr_mode;
extern int         for_mode;
extern int         curr_sig_id;
extern int         curr_arc_id;
extern int         vcd_symtab_size;
extern bool        instance_specified;
extern char*       top_instance;


/*!
 Array of database pointers storing all currently loaded databases.
*/
/*@null@*/ db** db_list = NULL;

/*!
 Size of the db_list array.
*/
unsigned int db_size = 0;

/*!
 Index of current database in db_list array that is being handled. 
*/
unsigned int curr_db = 0;

/*!
 Specifies the string Verilog scope that is currently specified in the VCD file.
*/
/*@null@*/ char** curr_inst_scope = NULL;

/*!
 Current size of curr_inst_scope array
*/
int curr_inst_scope_size = 0;

/*!
 Pointer to the current instance selected by the VCD parser.  If this value is
 NULL, the current instance does not reside in the design specified for coverage.
*/
/*@null@*/ funit_inst* curr_instance = NULL;

/*!
 Pointer to head of list of module names that need to be parsed yet.  These names
 are added in the db_add_instance function and removed in the db_end_module function.
*/
/*@null@*/ str_link* modlist_head = NULL;

/*!
 Pointer to tail of list of module names that need to be parsed yet.  These names
 are added in the db_add_instance function and removed in the db_end_module function.
*/
/*@null@*/ str_link* modlist_tail = NULL;

/*!
 Pointer to the functional unit structure for the functional unit that is currently being parsed.
*/
/*@null@*/ func_unit* curr_funit = NULL;

/*!
 Pointer to the global function unit that is available in SystemVerilog.
*/
/*@null@*/ func_unit* global_funit = NULL;

/*!
 This static value contains the current expression ID number to use for the next expression found, it
 is incremented by one when an expression is found.  This allows us to have a unique expression ID
 for each expression (since expressions have no intrinsic names).
*/
int curr_expr_id = 1;

/*!
 Specifies current connection ID to use for connecting statements.  This value should be passed
 to the statement_connect function and incremented immediately after.
*/
int stmt_conn_id = 1;

/*!
 Specifies current connection ID to use for connecting generate items.
*/
int gi_conn_id = 1;

/*!
 Pointer to current implicitly connected generate item block.
*/
/*@null@*/ static gen_item* curr_gi_block = NULL;

/*!
 Pointer to the most recently created generate item.
*/
/*@null@*/ gen_item* last_gi = NULL;

/*!
 Specifies the current timescale unit shift value.
*/
static int current_timescale_unit = 2;

/*!
 Specifies the global timescale precision shift value.
*/
static int global_timescale_precision = 2;

/*!
 Specifies the state of pragma-controlled exclusion.  If the mode is a value of 0, we
 should not be excluding anything.  If it is a value greater than 0, we will exclude.
*/
unsigned int exclude_mode = 0;

/*!
 Specifies the state of pragma-controlled race condition check handling.  If the mode
 is a value of 0, we should be performing race condition checking.  If it is a value
 greater than 0, we will not perform race condition checking.
*/
unsigned int ignore_racecheck_mode = 0;

/*!
 Specifies the number of timesteps that have transpired during this simulation.
*/
uint64 num_timesteps = 0;

/*!
 Contains the current exclusion identifier created by the db_gen_exclusion_id function.
*/
/*@null@*/ static char* exclusion_id = NULL;

/*!
 Unnamed scope ID.
*/
int unnamed_scope_id = 0;


/*!
 \return Returns pointer to newly allocated and initialized database structure

 Allocates, initializes and stores new database structure into global database array. 
*/
db* db_create() { PROFILE(DB_CREATE);

  db* new_db;  /* Pointer to new database structure */

  /* Allocate new database */
  new_db                       = (db*)malloc_safe( sizeof( db ) );
  new_db->inst_head            = NULL;
  new_db->inst_tail            = NULL;
  new_db->insts                = NULL;
  new_db->inst_num             = 0;
  new_db->funit_head           = NULL;
  new_db->funit_tail           = NULL;
  new_db->fver_head            = NULL;
  new_db->fver_tail            = NULL;
  new_db->leading_hierarchies  = NULL;
  new_db->leading_hier_num     = 0;
  new_db->leading_hiers_differ = FALSE;

  /* Add this new database to the database array */
  db_list = (db**)realloc_safe( db_list, (sizeof( db ) * db_size), (sizeof( db ) * (db_size + 1)) );
  db_list[db_size] = new_db;
  db_size++;

  PROFILE_END;

  return( new_db );

}

/*!
 Deallocates all memory associated with the databases.
*/
void db_close() { PROFILE(DB_CLOSE);
  
  unsigned int i, j;

  for( i=0; i<db_size; i++ ) {

    if( db_list[i]->inst_head != NULL ) {

      /* Remove memory allocated for inst_head */
      inst_link_delete_list( db_list[i]->inst_head );
      db_list[i]->inst_head = NULL;
      db_list[i]->inst_tail = NULL;

      /* Remove memory allocated for all functional units */
      funit_link_delete_list( &(db_list[i]->funit_head), &(db_list[i]->funit_tail), TRUE );

    }

    /* Deallocate the insts array */
    if( db_list[i]->insts != NULL ) {
      free_safe( db_list[i]->insts, (sizeof( funit_inst* ) * db_list[i]->inst_num) );
      db_list[i]->insts = NULL;
    }

    /* Deallocate all information regarding hierarchies */
    for( j=0; j<db_list[i]->leading_hier_num; j++ ) {
      free_safe( db_list[i]->leading_hierarchies[j], (strlen( db_list[i]->leading_hierarchies[j] ) + 1) );
    }
    free_safe( db_list[i]->leading_hierarchies, (sizeof( char* ) * db_list[i]->leading_hier_num) );

    /* Deallocate the file version information */
    str_link_delete_list( db_list[i]->fver_head );
    db_list[i]->fver_head = NULL;
    db_list[i]->fver_tail = NULL;

    /* Deallocate database structure */
    free_safe( db_list[i], sizeof( db ) );

  }

  /* Clear the global functional unit */
  global_funit = NULL;

  /* Deallocate preprocessor define tree */
  tree_dealloc( def_table );
  def_table = NULL;

  /* Deallocate the binding list */
  bind_dealloc();

  /* Deallocate database information */
  info_dealloc();

  /* Free memory associated with current instance scope */
  assert( curr_inst_scope_size == 0 );

  /* Deallocate the exclusion identifier container, if it exists */
  free_safe( exclusion_id, db_get_exclusion_id_size() );

  /* Finally, deallocate the database list */
  free_safe( db_list, (sizeof( db ) * db_size) );
  db_list = NULL;
  db_size = 0;
  curr_db = 0;

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \return Returns TRUE if the top module specified in the -t option is the top-level module
         of the simulation; otherwise, returns FALSE.

 Iterates through the signal list of the top-level module.  Returns TRUE if no signals with
 type INPUT, OUTPUT or INOUT were found; otherwise, returns FALSE.  Called by the parse_design()
 function.
*/
bool db_check_for_top_module() { PROFILE(DB_CHECK_FOR_TOP_MODULE);

  bool        retval;
  funit_inst* top_inst;

  /* Get the top-most instance */
  instance_get_leading_hierarchy( db_list[curr_db]->inst_tail->inst, NULL, &top_inst );

  /* Check to see if the signal list is void of ports */
  retval = funit_is_top_module( top_inst->funit );

  PROFILE_END;

  return( retval );

}
#endif /* RUNLIB */

/*!
 \throws anonymous Throw Throw instance_db_write

 Opens specified database for writing.  If database open successful,
 iterates through functional unit, expression and signal lists, displaying each
 to the database file.  If database write successful, returns TRUE; otherwise,
 returns FALSE to the calling function.
*/
void db_write(
  const char* file,        /*!< Name of database file to output contents to */
  bool        parse_mode,  /*!< Specifies if we are outputting parse data or score data */
  bool        issue_ids    /*!< Specifies if we need to issue/reissue expression and signal IDs */
) { PROFILE(DB_WRITE);

  FILE*      db_handle;  /* Pointer to database file being written */
  inst_link* instl;      /* Pointer to current instance link */

  if( (db_handle = fopen( file, "w" )) != NULL ) {

    unsigned int rv;

    Try {

      /* Reset expression IDs */
      curr_expr_id = 1;

      /* Iterate through instance tree */
      assert( db_list[curr_db]->inst_head != NULL );
      info_db_write( db_handle );

      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {

        /* Only output the given instance tree if it is not ignored */
        if( !instl->ignore ) {

          str_link* strl;

          /*
           If the file version information has not been set for this instance's functional unit and a file version
           has been specified for this functional unit's file, set it now.
          */
          if( (instl->inst->funit != NULL) &&
              (instl->inst->funit->version == NULL) &&
              ((strl = str_link_find( instl->inst->funit->orig_fname, db_list[curr_db]->fver_head )) != NULL) ) {
            instl->inst->funit->version = strdup_safe( strl->str2 );
          }

          /* Now write the instance */
          instance_db_write( instl->inst, db_handle, instl->inst->name, parse_mode, issue_ids );

        }

        instl = instl->next;

      }

    } Catch_anonymous {
      rv = fclose( db_handle );
      assert( rv == 0 );
      Throw 0;
    }

    rv = fclose( db_handle );
    assert( rv == 0 );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for writing", obf_file( file ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \throws anonymous info_db_read args_db_read Throw Throw Throw expression_db_read fsm_db_read race_db_read funit_db_read vsignal_db_read funit_db_merge funit_db_merge statement_db_read

 \return Returns TRUE if the read in CDD created a database; otherwise, returns FALSE.

 Opens specified database file for reading.  Reads in each line from the
 file examining its contents and creating the appropriate type to store
 the specified information and stores it into the appropriate internal
 list.
*/
bool db_read(
  const char* file,      /*!< Name of database file to read contents from */
  int         read_mode  /*!< Specifies what to do with read data (see \ref read_modes for legal values) */
) { PROFILE(DB_READ);

  FILE*        db_handle;              /* Pointer to database file being read */
  int          type;                   /* Specifies object type */
  func_unit    tmpfunit;               /* Temporary functional unit pointer */
  char*        curr_line     = NULL;   /* Pointer to current line being read from db */
  unsigned int curr_line_size;         /* Allocated number of bytes for curr_line */
  char*        rest_line;              /* Pointer to rest of the current line */
  int          chars_read;             /* Number of characters currently read on line */
  char         parent_scope[4096];     /* Scope of parent functional unit to the current instance */
  char         back[4096];             /* Current functional unit instance name */
  char         funit_scope[4096];      /* Current scope of functional unit instance */
  char         funit_name[256];        /* Current name of functional unit instance */
  char         funit_ofile[4096];      /* Current filename of functional unit instance */
  char         funit_ifile[4096];      /* Current filename of functional unit instance */
  funit_link*  foundfunit;             /* Found functional unit link */
  funit_inst*  foundinst;              /* Found functional unit instance */
  bool         merge_mode    = FALSE;  /* If TRUE, we should currently be merging data */
  func_unit*   parent_mod;             /* Pointer to parent module of this functional unit */
  bool         inst_name_diff;         /* Specifies the read value of the name diff for the current instance */
  bool         stop_reading  = FALSE;
  bool         one_line_read = FALSE;
  unsigned int inst_index    = 0;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_read, file: %s, mode: %d", obf_file( file ), read_mode );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Setup temporary module for storage */
  tmpfunit.name       = funit_name;
  tmpfunit.orig_fname = funit_ofile;
  tmpfunit.incl_fname = funit_ifile;

  curr_funit  = NULL;

  if( (db_handle = fopen( file, "r" )) != NULL ) {

    unsigned int rv;

    Try {

      while( !stop_reading && util_readline( db_handle, &curr_line, &curr_line_size ) ) {

        one_line_read = TRUE;

        Try {

          if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {

            rest_line = curr_line + chars_read;

            if( type == DB_TYPE_INFO ) {
          
              /* Parse rest of line for general info */
              stop_reading = !info_db_read( &rest_line, read_mode );
  
              if( !stop_reading ) {

                /* If we are in report mode or merge mode and this CDD file has not been scored, bow out now */
                if( (info_suppl.part.scored == 0) &&
                    ((read_mode == READ_MODE_REPORT_NO_MERGE) ||
                     (read_mode == READ_MODE_REPORT_MOD_MERGE)) ) {
                  print_output( "Attempting to generate report on non-scored design.  Not supported.", FATAL, __FILE__, __LINE__ );
                  Throw 0;
                }

              }
          
            } else if( type == DB_TYPE_SCORE_ARGS ) {
          
              assert( !merge_mode );
         
              /* Parse rest of line for argument info (if we are not instance merging) */
              if( read_mode != READ_MODE_MERGE_INST_MERGE ) {
                args_db_read( &rest_line );
              }
            
            } else if( type == DB_TYPE_MESSAGE ) {
 
              assert( !merge_mode );
 
              /* Parse rest of line for user-supplied message */
              if( (read_mode != READ_MODE_MERGE_NO_MERGE) && (read_mode != READ_MODE_MERGE_INST_MERGE) ) {
                message_db_read( &rest_line );
              }

            } else if( type == DB_TYPE_MERGED_CDD ) {

              assert( !merge_mode );

              /* Parse rest of line for merged CDD information */
              merged_cdd_db_read( &rest_line );
    
            } else if( type == DB_TYPE_SIGNAL ) {
  
              assert( !merge_mode );

              /* Parse rest of line for signal info */
              vsignal_db_read( &rest_line, curr_funit );
 
            } else if( type == DB_TYPE_EXPRESSION ) {

              assert( !merge_mode );

              /* Parse rest of line for expression info */
              expression_db_read( &rest_line, curr_funit, (read_mode == READ_MODE_NO_MERGE) );
  
            } else if( type == DB_TYPE_STATEMENT ) {

              assert( !merge_mode );

              /* Parse rest of line for statement info */
              statement_db_read( &rest_line, curr_funit, read_mode );

            } else if( type == DB_TYPE_FSM ) {

              assert( !merge_mode );

              /* Parse rest of line for FSM info */
              fsm_db_read( &rest_line, curr_funit );

            } else if( type == DB_TYPE_EXCLUDE ) {

#ifndef RUNLIB
              /* Parse rest of line for exclude info */
              if( merge_mode ) {
                exclude_db_merge( curr_funit, &rest_line );
              } else {
                exclude_db_read( &rest_line, curr_funit );
              }
#else
              assert( 0 );  /* I don't believe that we should ever get here with RUNLIB */
#endif /* RUNLIB */

            } else if( type == DB_TYPE_RACE ) {

              assert( !merge_mode );

              /* Parse rest of line for race condition block info */
              race_db_read( &rest_line, curr_funit );

            } else if( type == DB_TYPE_FUNIT_VERSION ) {

              assert( !merge_mode );
  
              /* Parse rest of line for functional unit version information */
              funit_version_db_read( curr_funit, &rest_line );

            } else if( (type == DB_TYPE_FUNIT ) || (type == DB_TYPE_INST_ONLY) ) {

              /* Finish handling last functional unit read from CDD file */
              if( curr_funit != NULL ) {
              
                if( (read_mode != READ_MODE_MERGE_INST_MERGE) || !merge_mode ) {

                  funit_inst* inst;

                  /* Get the scope of the parent module */
                  scope_extract_back( funit_scope, back, parent_scope );

                  /* Attempt to add it to the last instance tree */
                  if( (db_list[curr_db]->inst_tail == NULL) ||
                      ((inst = instance_read_add( &(db_list[curr_db]->inst_tail->inst), parent_scope, curr_funit, back )) == NULL) ) {
                    inst = instance_create( curr_funit, funit_scope, 0, 0, inst_name_diff, FALSE, FALSE, NULL );
                    (void)inst_link_add( inst, &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
                  }

                  /* Add the instance to the instance array */
                  if( (info_suppl.part.scored == 0) && (info_suppl.part.inlined == 1) ) {
                    assert( inst_index < db_list[curr_db]->inst_num );
                    db_list[curr_db]->insts[inst_index++] = inst;
                  }

                }

                /* If the current functional unit is a merged unit, don't add it to the funit list again */
                if( !merge_mode ) {
                  funit_link_add( curr_funit, &(db_list[curr_db]->funit_head), &(db_list[curr_db]->funit_tail) );
                }

              }

              if( type == DB_TYPE_INST_ONLY ) {

                /* Parse rest of the line for an instance-only structure */
                if( !merge_mode ) {
                  funit_inst* inst = instance_only_db_read( &rest_line );
                  if( (info_suppl.part.scored == 0) && (info_suppl.part.inlined == 1) ) {
                    db_list[curr_db]->insts[inst_index++] = inst;
                  }
#ifndef RUNLIB
                } else {
                  instance_only_db_merge( &rest_line );
#endif /* RUNLIB */
                }

                /* Specify that the current functional unit does not exist */
                curr_funit = NULL;

              } else {

                /* Reset merge mode */
                merge_mode = FALSE;

                /* Now finish reading functional unit line */
                funit_db_read( &tmpfunit, funit_scope, &inst_name_diff, &rest_line );
#ifndef RUNLIB
                if( (read_mode == READ_MODE_MERGE_INST_MERGE) &&
                    ((foundinst = inst_link_find_by_scope( funit_scope, db_list[curr_db]->inst_head, FALSE )) != NULL) ) {
                  merge_mode = TRUE;
                  curr_funit = foundinst->funit;
                  funit_db_merge( foundinst->funit, db_handle, TRUE );
                } else if( (read_mode == READ_MODE_REPORT_MOD_MERGE) &&
                           ((foundfunit = funit_link_find( tmpfunit.name, tmpfunit.suppl.part.type, db_list[curr_db]->funit_head )) != NULL) ) {
                  merge_mode = TRUE;
                  curr_funit = foundfunit->funit;
                  funit_db_merge( foundfunit->funit, db_handle, FALSE );
                } else {
#endif /* RUNLIB */
                  curr_funit             = funit_create();
                  curr_funit->name       = strdup_safe( funit_name );
                  curr_funit->suppl.all  = tmpfunit.suppl.all;
                  curr_funit->orig_fname = strdup_safe( funit_ofile );
                  curr_funit->incl_fname = strdup_safe( funit_ifile );
                  curr_funit->start_line = tmpfunit.start_line;
                  curr_funit->end_line   = tmpfunit.end_line;
                  curr_funit->timescale  = tmpfunit.timescale;
                  if( tmpfunit.suppl.part.type != FUNIT_MODULE ) {
                    curr_funit->parent = scope_get_parent_funit( db_list[curr_db]->inst_tail->inst, funit_scope );
                    parent_mod         = scope_get_parent_module( db_list[curr_db]->inst_tail->inst, funit_scope );
                    funit_link_add( curr_funit, &(parent_mod->tf_head), &(parent_mod->tf_tail) );
                  }
#ifndef RUNLIB
                }
#endif /* RUNLIB */
  
                /* Set global functional unit, if it has been found */
                if( (curr_funit != NULL) && (strncmp( curr_funit->name, "$root", 5 ) == 0) ) {
                  global_funit = curr_funit;
                }

              }

            } else {

              unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unexpected type %d when parsing database file %s", type, obf_file( file ) );
              assert( rv < USER_MSG_LENGTH );
              print_output( user_msg, FATAL, __FILE__, __LINE__ );
              Throw 0;

            }

          } else {

            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unexpected line in database file %s", obf_file( file ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            Throw 0;

          }

        } Catch_anonymous {

          free_safe( curr_line, curr_line_size );
          if( (read_mode != READ_MODE_MERGE_INST_MERGE) && (read_mode != READ_MODE_REPORT_MOD_MERGE) ) {
            funit_dealloc( curr_funit );
          }
          Throw 0;

        }

      }

      /* If we ran into an issue with reading the CDD file, we will need to deallocate the line string */
      if( stop_reading ) {
        free_safe( curr_line, curr_line_size );
      }

    } Catch_anonymous {

      unsigned int rv = fclose( db_handle );
      assert( rv == 0 );
      Throw 0;

    }
 
    rv = fclose( db_handle );
    assert( rv == 0 );

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for reading", obf_file( file ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  /* If the last functional unit was being read, add it now */
  if( curr_funit != NULL ) {

    if( (read_mode != READ_MODE_MERGE_INST_MERGE) || !merge_mode ) {

      funit_inst* inst;

      /* Get the scope of the parent module */
      scope_extract_back( funit_scope, back, parent_scope );

      /* Attempt to add it to the last instance tree */
      if( (db_list[curr_db]->inst_tail == NULL) ||
          ((inst = instance_read_add( &(db_list[curr_db]->inst_tail->inst), parent_scope, curr_funit, back )) == NULL) ) {
        inst = instance_create( curr_funit, funit_scope, 0, 0, inst_name_diff, FALSE, FALSE, NULL );
        (void)inst_link_add( inst, &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
      }

      /* Add the instance to the instance array */
      if( (info_suppl.part.scored == 0) && (info_suppl.part.inlined == 1) ) {
        assert( inst_index < db_list[curr_db]->inst_num );
        db_list[curr_db]->insts[inst_index++] = inst;
      }

    }

    /* If the current functional unit was being merged, don't add it to the functional unit list again */
    if( !merge_mode ) {
      funit_link_add( curr_funit, &(db_list[curr_db]->funit_head), &(db_list[curr_db]->funit_tail) );
    }

    curr_funit = NULL;

  }

#ifdef DEBUG_MODE
  /* Display the instance trees, if we are debugging */
  if( debug_mode && (db_list != NULL) ) {
    inst_link_display( db_list[curr_db]->inst_head );
    printf( "-----------------------------------\n" );
  }
#endif

  /* Just make sure that that the number of instances read matches what we expect */
  if( (info_suppl.part.scored == 0) && (info_suppl.part.inlined == 1) ) {
    // printf( "db_list->inst_num: %u, inst_index: %u\n", db_list[curr_db]->inst_num, inst_index );
    assert( db_list[curr_db]->inst_num == inst_index );
  }

  /* Check to make sure that the CDD file contained valid information */
  if( !stop_reading && !one_line_read ) {
    print_output( "CDD file was found to be empty", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  PROFILE_END;

  return( !stop_reading );

}

#ifndef RUNLIB
/*! \brief Assigns instance IDs to all instances. */
void db_assign_ids() { PROFILE(DB_ASSIGN_IDS);

  inst_link*  instl  = db_list[curr_db]->inst_head;
  funit_link* funitl = db_list[curr_db]->funit_head;
  int         curr_id;

  /* Number the instances */
  curr_id = 0;
  while( instl != NULL ) {
    instance_assign_ids( instl->inst, &curr_id );
    instl = instl->next;
  }
  db_list[curr_db]->inst_num = curr_id;

  /* Number the functional units */
  curr_id = 0;
  while( funitl != NULL ) {
    funitl->funit->id = curr_id++;
    funitl = funitl->next;
  }

  PROFILE_END;

}

/*!
 Iterates through the instance tree list, merging all instances into the first instance tree that
 is not the $root instance tree.
*/
void db_merge_instance_trees() { PROFILE(DB_MERGE_INSTANCE_TREES);

  funit_inst* base  = NULL;
  inst_link*  instl = db_list[curr_db]->inst_head;
  bool        done  = FALSE;

  if( db_list != NULL ) {

    /* Merge all root trees */
    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      if( strcmp( instl->inst->name, "$root" ) == 0 ) {
        if( base == NULL ) {
          base        = instl->inst;
          instl->base = TRUE;
        } else {
          instl->ignore = instance_merge_two_trees( base, instl->inst );
        }
      }
      instl = instl->next;
    }

    /* Merge all other trees */
    while( !done ) {
      base  = NULL;
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        if( strcmp( instl->inst->name, "$root" ) != 0 ) {
          if( !instl->ignore && !instl->base ) {
            if( base == NULL ) {
              base        = instl->inst;
              instl->base = TRUE;
            } else {
              instl->ignore = instance_merge_two_trees( base, instl->inst );
            }
          }
        }
        instl = instl->next;
      }
      done = (base == NULL);
    }

  } else {

    print_output( "Attempting to merge unscored CDDs", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Iterates through the functional unit list, merging all matching functional units.
*/
void db_merge_funits() { PROFILE(DB_MERGE_FUNITS);

  funit_link* funitl;  /* Pointer to current functional unit link */

  funitl = db_list[curr_db]->funit_head;
  while( funitl != NULL ) {

    funit_link* tfunitl = db_list[curr_db]->funit_head;

    while( (tfunitl != NULL) && (funitl != tfunitl) ) {
      func_unit* tfunit = tfunitl->funit;
      tfunitl = tfunitl->next;
      if( (strcmp( funitl->funit->name, tfunit->name ) == 0) && (funitl->funit->suppl.part.type == tfunit->suppl.part.type) ) {
        funit_inst* inst;
        int         ignore = 0;
        funit_merge( funitl->funit, tfunit );
        inst = inst_link_find_by_funit( tfunit, db_list[curr_db]->inst_head, &ignore );
        assert( inst != NULL );
        inst->funit = funitl->funit;
        funit_link_remove( tfunit, &(db_list[curr_db]->funit_head), &(db_list[curr_db]->funit_tail), TRUE );
      }
    }

    funitl = funitl->next;

  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \return Returns scaled version of input value.

 Takes in specified delay value and scales it to the correct timescale for the given
 module.
*/
uint64 db_scale_to_precision(
  uint64     value,  /*!< Delay value to scale */
  func_unit* funit   /*!< Pointer to current functional unit of expression to scale */
) { PROFILE(DB_SCALE_TO_PRECISION);

  int units = funit->ts_unit;

  assert( units >= global_timescale_precision );

  while( units > global_timescale_precision ) {
    units--;
    value *= (uint64)10;
  }

  PROFILE_END;

  return( value );

}

#ifndef RUNLIB
/*!
 \return Returns a scope name for an unnamed scope.  Only called for parsing purposes.
*/
char* db_create_unnamed_scope() { PROFILE(DB_CREATE_UNNAMED_SCOPE);

  char         tmpname[30];
  char*        name;
  unsigned int rv = snprintf( tmpname, 30, "u$%d", unnamed_scope_id );

  assert( rv < 30 );
  
  name = strdup_safe( tmpname );
  unnamed_scope_id++;

  PROFILE_END;

  return( name );

}
#endif /* RUNLIB */

/*!
 \return Returns TRUE if the given scope is an unnamed scope name; otherwise, returns FALSE.
*/
bool db_is_unnamed_scope(
  char* scope  /*!< Name to check */
) { PROFILE(DB_IS_UNNAMED_SCOPE);

  bool is_unnamed = (scope != NULL) && (scope[0] == 'u') && (scope[1] == '$');

  PROFILE_END;

  return( is_unnamed );

}

#ifndef RUNLIB
#ifndef VPI_ONLY
/*!
 Sets the global timescale unit and precision variables.
*/
void db_set_timescale(
  int unit,      /*!< Timescale unit offset value */
  int precision  /*!< Timescale precision offset value */
) { PROFILE(DB_SET_TIMESCALE);

  current_timescale_unit = unit;

  /* Set the global precision value to the lowest precision value specified */
  if( precision < global_timescale_precision ) {
    global_timescale_precision = precision;
  }

  PROFILE_END;

}

/*!
 Searches for the module with the given name and sets the current functional unit pointer
 to it.
*/
void db_find_and_set_curr_funit(
  const char* name,  /*!< Scope name to find */
  int         type   /*!< Scope type to find */
) { PROFILE(DB_FIND_AND_SET_CURR_FUNIT);

  funit_link* funitl = funit_link_find( name, type, db_list[curr_db]->funit_head );

  assert( funitl != NULL );

  /* Set current functional unit pointer */
  curr_funit = funitl->funit;

  PROFILE_END;

}

/*!
 \return Returns a pointer to the current functional unit.

 This function returns a pointer to the current functional unit being parsed.
*/
func_unit* db_get_curr_funit() { PROFILE(DB_GET_CURR_FUNIT);

  PROFILE_END;

  return( curr_funit );

}

/*!
 \return Returns a pointer to the functional unit that begins at the specified position.
*/
func_unit* db_get_tfn_by_position(
  unsigned int first_line,   /*!< First line of the functional unit to find */
  unsigned int first_column  /*!< First column of the functional unit to find */
) { PROFILE(DB_GET_FUNIT_BY_POSITION);

  funit_link* funitl = NULL;
  func_unit*  funit;

  funitl = curr_funit->tf_head;
  while( (funitl != NULL) &&
         ((funitl->funit->start_line != first_line) || (funitl->funit->start_col != first_column)) ) {
    funitl = funitl->next;
  }

  /* If we didn't find the functional unit in the current functional unit, look at its generate TFNs */
  if( funitl == NULL ) {
    funit = generate_find_tfn_by_position( curr_funit, first_line, first_column );
  } else {
    funit = funitl->funit;
  }

  PROFILE_END;

  return( funit );

}

/*!
 \return Returns the size needed to allocate for an entire exclusion ID.
*/
unsigned int db_get_exclusion_id_size() { PROFILE(DB_GET_EXCLUSION_ID_SIZE);

  static unsigned int exclusion_id_size = 0;

  if( exclusion_id_size == 0 ) {

    char         tmp[30];
    unsigned int rv;

    /* Calculate the size needed to store the largest signal ID */
    rv = snprintf( tmp, 30, "%d", curr_sig_id );
    assert( rv < 30 );
    exclusion_id_size = strlen( tmp ) + 2;

    /* Now calculate the size needed to store the largest expression ID */
    rv = snprintf( tmp, 30, "%d", curr_expr_id );
    assert( rv < 30 );

    /* Figure out which value is greater and use that for the size of the exclusion ID */
    if( (strlen( tmp ) + 2) > exclusion_id_size ) {
      exclusion_id_size = strlen( tmp ) + 2;
    }

    /* Now calculate the size needed to store the largest arc ID */
    rv = snprintf( tmp, 30, "%d", curr_arc_id );
    assert( rv < 30 );

    /* Figure out which value is greater and use that for the size of the exclusion ID */
    if( (strlen( tmp ) + 2) > exclusion_id_size ) {
      exclusion_id_size = strlen( tmp ) + 2;
    }

    /* The minimum size of the exclusion ID should be 3 characters */
    if( exclusion_id_size < 4 ) {
      exclusion_id_size = 4;
    }

  }

  PROFILE_END;

  return( exclusion_id_size );

}

/*!
 \return Returns the generated exclusion ID given the parameters and the value of the report_exclusions global flag.

 Generates the exclusion ID string and stores the result in the excl_id array.

 \note
 This function should ONLY be called when the flag_output_exclusion_ids is set to TRUE.
*/
char* db_gen_exclusion_id(
  char type,  /*!< Single character specifying the metric type (L, T, M, C, A, F) */
  int  id     /*!< Numerical unique identifier */
) { PROFILE(DB_GEN_EXCLUSION_ID);

  char         tmp[30];
  int          size = db_get_exclusion_id_size();
  unsigned int rv;

  /* If the exclusion ID has not been created, create it now */
  if( exclusion_id == NULL ) {

    /* Allocate the memory needed */
    exclusion_id = (char*)malloc_safe( size );

  }

  /* Create format string */
  rv = snprintf( tmp, 30, "%%c%%0%dd", (size - 2) );
  assert( rv < 30 );

  /* Generate exclusion_id string */
  rv = snprintf( exclusion_id, size, tmp, type, id );
  assert( rv < size );
 
  PROFILE_END;

  return( exclusion_id );
 
}

/*!
 Adds the given filename and version information to the database.
*/
void db_add_file_version(
  const char* file,    /*!< Name of file to set version information to */
  const char* version  /*!< Name of file version */
) { PROFILE(DB_ADD_FILE_VERSION);

  str_link* strl;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_file_version, file: %s, version: %s", obf_file( file ), version );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Add the new file version information */
  strl       = str_link_add( strdup_safe( file ), &(db_list[curr_db]->fver_head), &(db_list[curr_db]->fver_tail) );
  strl->str2 = strdup_safe( version );

  PROFILE_END;

}

/*!
 Outputs all signals that need to be dumped to the given files.
*/
void db_output_dumpvars(
  FILE* vfile  /*!< Pointer to file to output dumpvars output to */
) { PROFILE(DB_OUTPUT_DUMPVARS);

  inst_link* instl = db_list[curr_db]->inst_head;

  while( instl != NULL ) {
    instance_output_dumpvars( vfile, instl->inst );
    instl = instl->next;
  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the created functional unit if the instance was added to the hierarchy;
         otherwise, returns NULL.

 \throws anonymous Throw
 
 Creates a new functional unit node with the instantiation name, search for matching functional unit.  If
 functional unit hasn't been created previously, create it now without a filename associated (NULL).
 Add functional unit node to tree if there are no problems in doing so.
*/
func_unit* db_add_instance(
  char*         scope,    /*!< Name of functional unit instance being added */
  char*         name,     /*!< Name of functional unit being instantiated */
  int           type,     /*!< Type of functional unit being instantiated */
  unsigned int  ppfline,  /*!< First line of instantiation from preprocessor */
  int           fcol,     /*!< First column of instantiation */
  vector_width* range     /*!< Optional range (used for arrays of instances) */
) { PROFILE(DB_ADD_INSTANCE);

  func_unit*  funit = NULL;      /* Pointer to functional unit */
  funit_link* found_funit_link;  /* Pointer to found funit_link in functional unit list */
  bool        score;             /* Specifies if this module should be scored */

  /* There should always be a parent so internal error if it does not exist. */
  assert( curr_funit != NULL );

  /* If this functional unit name is in our list of no_score functional units, skip adding the instance */
  score = str_link_find( name, no_score_head ) == NULL;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_instance, instance: %s, %s: %s (curr_funit: %s)",
                                obf_inst( scope ), get_funit_type( type ), obf_funit( name ), obf_funit( curr_funit->name ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Create new functional unit node */
  funit                  = funit_create();
  funit->name            = strdup_safe( name );
  funit->suppl.part.type = score ? type : FUNIT_NO_SCORE;

  /* If a range has been specified, calculate its width and lsb now */
  if( (range != NULL) && score ) {
    if( (range->left != NULL) && (range->left->exp != NULL) ) {
      (void)mod_parm_add( NULL, NULL, NULL, FALSE, range->left->exp, PARAM_TYPE_INST_MSB, curr_funit, scope );
    }
    if( (range->right != NULL) && (range->right->exp != NULL) ) {
      (void)mod_parm_add( NULL, NULL, NULL, FALSE, range->right->exp, PARAM_TYPE_INST_LSB, curr_funit, scope );
    }
  }

  if( ((found_funit_link = funit_link_find( funit->name, funit->suppl.part.type, db_list[curr_db]->funit_head )) != NULL) && (generate_top_mode == 0) ) {

    if( type != FUNIT_MODULE ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Multiple identical task/function/named-begin-end names (%s) found in module %s, file %s",
                                  scope, obf_funit( curr_funit->name ), obf_file( curr_funit->orig_fname ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      funit_dealloc( funit );
      Throw 0;
    }

    if( (last_gi == NULL) || (last_gi->suppl.part.type != GI_TYPE_INST) || !instance_parse_add( &last_gi->elem.inst, curr_funit, found_funit_link->funit, scope, ppfline, fcol, range, FALSE, TRUE, FALSE, FALSE ) ) {
      inst_link* instl = db_list[curr_db]->inst_head;
      while( (instl != NULL) && !instance_parse_add( &instl->inst, curr_funit, found_funit_link->funit, scope, ppfline, fcol, range, FALSE, FALSE, FALSE, FALSE ) ) {
        instl = instl->next;
      }
      if( instl == NULL ) {
        (void)inst_link_add( instance_create( found_funit_link->funit, scope, ppfline, fcol, FALSE, FALSE, FALSE, range ), &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
      }
    }

    funit_dealloc( funit );

  } else {

    /* Add new functional unit to functional unit list if we are not within a generate block. */
    if( (found_funit_link == NULL) || (funit->suppl.part.type != FUNIT_MODULE) ) {
      funit_link_add( funit, &(db_list[curr_db]->funit_head), &(db_list[curr_db]->funit_tail) );
    }

    /* If we are currently within a generate block, create a generate item for this instance to resolve it later */
    if( generate_top_mode > 0 ) {
      last_gi = gen_item_create_inst( instance_create( funit, scope, ppfline, fcol, FALSE, FALSE, FALSE, range ) );
      if( curr_gi_block != NULL ) {
        db_gen_item_connect( curr_gi_block, last_gi );
      } else {
        curr_gi_block = last_gi;
      }
    }

    /* Add the instance to the instance tree in the proper place */
    {
      inst_link* instl = db_list[curr_db]->inst_head;
      while( (instl != NULL) && !instance_parse_add( &instl->inst, curr_funit, funit, scope, ppfline, fcol, range, FALSE, FALSE, (generate_top_mode > 0), (generate_expr_mode > 0) ) ) {
        instl = instl->next;
      }
      if( instl == NULL ) {
        (void)inst_link_add( instance_create( funit, scope, ppfline, fcol, FALSE, (generate_top_mode > 0), (generate_expr_mode > 0), range ), &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
      }
    }

    if( (type == FUNIT_MODULE) && score && (str_link_find( name, modlist_head ) == NULL) ) {
      (void)str_link_add( strdup_safe( name ), &modlist_head, &modlist_tail );
    }
      
  }

  PROFILE_END;

  return( score ? funit : NULL );

}

/*!
 Creates a new module element with the contents specified by the parameters given
 and inserts this module into the module list.  This function can only be called when we
 are actually parsing a module which implies that we must have the name of the module
 at the head of the modlist linked-list structure.
*/
void db_add_module(
  char*        name,        /*!< Name of module being added to tree */
  char*        orig_fname,  /*!< Filename that module exists in */
  char*        incl_fname,  /*!< Name of file that this module has been included into */
  unsigned int start_line,  /*!< Starting line number of this module in the file */
  unsigned int start_col    /*!< Starting column number of this module in the file */
) { PROFILE(DB_ADD_MODULE);

  funit_link* modl;  /* Pointer to found tree node */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_module, module: %s, file: %s, start_line: %d, start_col: %d",
                                obf_funit( name ), obf_file( orig_fname ), start_line, start_col );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  modl = funit_link_find( name, FUNIT_MODULE, db_list[curr_db]->funit_head );

  assert( modl != NULL );

  curr_funit             = modl->funit;
  curr_funit->orig_fname = strdup_safe( orig_fname );
  curr_funit->incl_fname = strdup_safe( incl_fname );
  curr_funit->start_line = start_line;
  curr_funit->start_col  = start_col;
  curr_funit->ts_unit    = current_timescale_unit;
  if( strcmp( orig_fname, incl_fname ) != 0 ) {
    curr_funit->suppl.part.included = 1;
  }

  /* Reset the unnamed scope ID */
  unnamed_scope_id = 0;

  PROFILE_END;
  
}

/*!
 Updates the modlist for parsing purposes.
*/
void db_end_module(
  int end_line  /*!< Ending line number of specified module in file */
) { PROFILE(DB_END_MODULE);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_end_module, end_line: %d", end_line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  curr_funit->end_line = end_line;

  str_link_remove( curr_funit->name, &modlist_head, &modlist_tail );

  /* Return the current functional unit to the global functional unit, if it exists */
  curr_funit = global_funit;

  PROFILE_END;

}

/*!
 \return Returns TRUE if the new functional unit was added to the design; otherwise, returns FALSE
         to indicate that this block should be ignored.

 \throws anonymous Throw

 Creates a new function, task or named block scope and adds it to the instance tree.  Also sets the curr_funit global
 pointer to point to this new functional unit.
*/
bool db_add_function_task_namedblock(
  int   type,         /*!< Specifies type of functional unit being added (function, task or named_block) */
  char* name,         /*!< Name of functional unit */
  char* orig_fname,   /*!< File containing the specified functional unit */
  char* incl_fname,   /*!< Name of file that the function/task/namedblock is included into */
  int   start_line,   /*!< Starting line number of functional unit */
  int   start_column  /*!< Starting line column of functional unit */
) { PROFILE(DB_ADD_FUNCTION_TASK_NAMEDBLOCK);

  func_unit* tf = NULL;  /* Pointer to created functional unit */
  func_unit* parent;     /* Pointer to parent module for the newly created functional unit */
  char*      full_name;  /* Full name of function/task/namedblock which includes the parent module name */

  assert( (type == FUNIT_FUNCTION)  || (type == FUNIT_TASK) || (type == FUNIT_NAMED_BLOCK) ||
          (type == FUNIT_AFUNCTION) || (type == FUNIT_ATASK) );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_function_task_namedblock, %s: %s, file: %s (%s), start_line: %d",
                                get_funit_type( type ), obf_funit( name ), obf_file( orig_fname ), obf_file( incl_fname ), start_line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Generate full name to use for the function/task */
  full_name = funit_gen_task_function_namedblock_name( name, curr_funit );

  Try {

    /* Add this as an instance so we can get scope */
    if( (tf = db_add_instance( name, full_name, type, start_line, start_column, NULL )) != NULL ) {

      /* Get parent */
      parent = funit_get_curr_module( curr_funit );

      if( generate_expr_mode > 0 ) {
        /* Change the recently created instance generate item to a TFN item */
        last_gi->suppl.part.type = GI_TYPE_TFN;
      } else {
        /* Store this functional unit in the parent module list */
        funit_link_add( tf, &(parent->tf_head), &(parent->tf_tail) );
      }

      /* Set our parent pointer to the current functional unit */
      tf->parent = curr_funit;

      /* If we are in an automatic task or function, set our type to FUNIT_ANAMED_BLOCK */
      if( (curr_funit->suppl.part.type == FUNIT_AFUNCTION) ||
          (curr_funit->suppl.part.type == FUNIT_ATASK) ||
          (curr_funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {
        assert( tf->suppl.part.type == FUNIT_NAMED_BLOCK );
        tf->suppl.part.type = FUNIT_ANAMED_BLOCK;
      }

      /* Set current functional unit to this functional unit */
      curr_funit             = tf;
      curr_funit->orig_fname = strdup_safe( orig_fname );
      curr_funit->incl_fname = strdup_safe( incl_fname );
      curr_funit->start_line = start_line;
      curr_funit->start_col  = start_column;
      curr_funit->ts_unit    = current_timescale_unit;
      if( strcmp( orig_fname, incl_fname ) != 0 ) {
        curr_funit->suppl.part.included = 1;
      }
    
    }

  } Catch_anonymous {
    free_safe( full_name, (strlen( full_name ) + 1) );
    Throw 0;
  }

  free_safe( full_name, (strlen( full_name ) + 1) );

  PROFILE_END;

  return( tf != NULL );

}

/*!
 Causes the current function/task/named block to be ended and added to the database.
*/
void db_end_function_task_namedblock(
  int end_line  /*!< Line number of end of this task/function */
) { PROFILE(DB_END_FUNCTION_TASK_NAMEDBLOCK);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_end_function_task_namedblock, end_line: %d", end_line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Store last line information */
  curr_funit->end_line = end_line;

  /* Set the first statement pointer */
  if( curr_funit->stmt_head != NULL ) {
    assert( curr_funit->stmt_head->stmt != NULL );
    curr_funit->first_stmt = curr_funit->stmt_head->stmt->head;
  }

  /* Set the current functional unit to the parent module */
  curr_funit = curr_funit->parent;

  PROFILE_END;

}

/*!
 Searches current module to verify that specified parameter name has not been previously
 used in the module.  If the parameter name has not been found, it is created added to
 the current module's parameter list.
*/
void db_add_declared_param(
  bool         is_signed,  /*!< Specified if the declared parameter needs to be handled as a signed value */
  static_expr* msb,        /*!< Static expression containing MSB of this declared parameter */
  static_expr* lsb,        /*!< Static expression containing LSB of this declared parameter */
  char*        name,       /*!< Name of declared parameter to add */
  expression*  expr,       /*!< Expression containing value of this parameter */
  bool         local       /*!< If TRUE, specifies that this parameter is a local parameter */
) { PROFILE(DB_ADD_DECLARED_PARAM);

  mod_parm* mparm;  /* Pointer to added module parameter */

  assert( name != NULL );

  /* If a parameter value type is not supported, don't create this parameter */
  if( expr != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_declared_param, param: %s, expr: %d, local: %d", obf_sig( name ), expr->id, local );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    if( mod_parm_find( name, curr_funit->param_head ) == NULL ) {

      /* Add parameter to module parameter list */
      mparm = mod_parm_add( name, msb, lsb, is_signed, expr, (local ? PARAM_TYPE_DECLARED_LOCAL : PARAM_TYPE_DECLARED), curr_funit, NULL );

    }

  }

  PROFILE_END;

}

/*!
 Creates override parameter and stores this in the current module as well
 as all associated instances.
*/
void db_add_override_param(
  char*       inst_name,  /*!< Name of instance being overridden */
  expression* expr,       /*!< Expression containing value of override parameter */
  char*       param_name  /*!< Name of parameter being overridden (for parameter_value_byname syntax) */
) { PROFILE(DB_ADD_OVERRIDE_PARAM);

  mod_parm* mparm;  /* Pointer to module parameter added to current module */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv;
    char*        exp_str = strdup_safe( expression_string( expr ) );
    if( param_name != NULL ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s, param_name: %s, expr: %s",
                     obf_inst( inst_name ), obf_sig( param_name ), exp_str );
    } else {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s, expr: %s", obf_inst( inst_name ), exp_str );
    }
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    free_safe( exp_str, (strlen( exp_str ) + 1) );
  }
#endif

  /* Add override parameter to module parameter list */
  mparm = mod_parm_add( param_name, NULL, NULL, FALSE, expr, PARAM_TYPE_OVERRIDE, curr_funit, inst_name );

  PROFILE_END;

}

/*!
 Creates a vector parameter for the specified signal or expression with the specified
 parameter expression.  This function is called by the parser.
*/
static void db_add_vector_param(
  vsignal*    sig,       /*!< Pointer to signal to attach parameter to */
  expression* parm_exp,  /*!< Expression containing value of vector parameter */
  int         type,      /*!< Type of signal vector parameter to create (LSB or MSB) */
  int         dimension  /*!< Specifies the signal dimension to solve for */
) { PROFILE(DB_ADD_VECTOR_PARAM);

  mod_parm* mparm;  /* Holds newly created module parameter */

  assert( sig != NULL );
  assert( (type == PARAM_TYPE_SIG_LSB) || (type == PARAM_TYPE_SIG_MSB) );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_vector_param, signal: %s, type: %d", obf_sig( sig->name ), type );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Add signal vector parameter to module parameter list */
  mparm = mod_parm_add( NULL, NULL, NULL, FALSE, parm_exp, type, curr_funit, NULL );

  /* Add signal to module parameter list */
  mparm->sig = sig;

  /* Set the dimension value of the module parameter to our dimension */
  mparm->suppl.part.dimension = dimension;

  PROFILE_END;

}

/*!
 Adds specified parameter to the defparam list.
*/
void db_add_defparam(
  /*@unused@*/ char*       name,  /*!< Name of parameter value to override */
               expression* expr   /*!< Expression value of parameter override */
) { PROFILE(DB_ADD_DEFPARAM);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_defparam, defparam: %s", obf_sig( name ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "defparam construct is not supported, line: %u.  Use -P option to score instead", expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );
  }

  expression_dealloc( expr, FALSE );

  PROFILE_END;

}

/*!
 Creates a new signal with the specified parameter information and adds this
 to the signal list if it does not already exist.  If width == 0, the sig_msb
 and sig_lsb values are interrogated.  If sig_msb and/or is non-NULL, its value is
 add to the current module's parameter list and all associated instances are
 updated to contain new value.
*/
void db_add_signal(
  char*      name,       /*!< Name of signal being added */
  int        type,       /*!< Type of signal being added */
  sig_range* prange,     /*!< Specifies packed signal range information */
  sig_range* urange,     /*!< Specifies unpacked signal range information */
  bool       is_signed,  /*!< Specifies that this signal is signed (TRUE) or not (FALSE) */
  bool       mba,        /*!< Set to TRUE if specified signal must be assigned by simulated results */
  int        line,       /*!< Line number where signal was declared */
  int        col,        /*!< Starting column where signal was declared */
  bool       handled     /*!< Specifies if this signal is handled by Covered or not */
) { PROFILE(DB_ADD_SIGNAL);

  vsignal*     sig   = NULL;  /* Container for newly created signal */
  unsigned int i;
  int          j     = 0;
  bool         found = TRUE;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_signal, signal: %s, type: %d, line: %d, col: %d", obf_sig( name ), type, line, col );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Add signal to current module's signal list if it does not already exist */
  if( (sig = sig_link_find( name, curr_funit->sigs, curr_funit->sig_size )) == NULL ) {

    /* Create the signal */
    if( (type == SSUPPL_TYPE_GENVAR) || (type == SSUPPL_TYPE_DECL_SREAL) ) {
      /* For genvars and shortreals, set the size to 32, automatically */
      sig = vsignal_create( name, type, 32, line, col );
    } else if( type == SSUPPL_TYPE_DECL_REAL ) {
      /* For real types, they should be automatically sized to 64, automatically */
      sig = vsignal_create( name, type, 64, line, col );
    } else {
      /* For normal signals just make the width a value of 1 for now -- it will be resized during funit_resize_elements */
      sig = vsignal_create( name, type, 1, line, col );
    }

    /* Specify that this signal was not originally found */
    found = FALSE;

  /* If the signal has currently existed, check to see if the signal is unsized, and, if so, size it now */
  } else if( sig->suppl.part.implicit_size ) {

    sig->suppl.part.implicit_size = 0;

  } else {

    sig = NULL;

  }

  /* Check all of the dimensions within range and create vector parameters, if necessary */
  if( sig != NULL ) {
    if( sig->dim != NULL ) {
      free_safe( sig->dim, (sizeof( dim_range ) * (sig->pdim_num + sig->udim_num)) );
    }
    assert( prange != NULL );
    sig->udim_num = (urange != NULL) ? urange->dim_num : 0;
    sig->pdim_num = prange->dim_num;
    assert( (sig->pdim_num + sig->udim_num) > 0 );
    sig->dim = (dim_range*)malloc_safe( sizeof( dim_range ) * (sig->pdim_num + sig->udim_num) );
    for( i=0; i<sig->udim_num; i++ ) {
      assert( urange->dim[i].left != NULL );
      if( urange->dim[i].left->exp != NULL ) {
        db_add_vector_param( sig, urange->dim[i].left->exp, PARAM_TYPE_SIG_MSB, j );
      } else {
        sig->dim[j].msb = urange->dim[i].left->num;
      }
      assert( urange->dim[i].right != NULL );
      if( urange->dim[i].right->exp != NULL ) {
        db_add_vector_param( sig, urange->dim[i].right->exp, PARAM_TYPE_SIG_LSB, j );
      } else {
        sig->dim[j].lsb = urange->dim[i].right->num;
      }
      j++;
    }
    for( i=0; i<sig->pdim_num; i++ ) {
      assert( prange->dim[i].left != NULL );
      if( prange->dim[i].left->exp != NULL ) {
        db_add_vector_param( sig, prange->dim[i].left->exp, PARAM_TYPE_SIG_MSB, j );
      } else {
        sig->dim[j].msb = prange->dim[i].left->num;
      }
      assert( prange->dim[i].right != NULL );
      if( prange->dim[i].right->exp != NULL ) {
        db_add_vector_param( sig, prange->dim[i].right->exp, PARAM_TYPE_SIG_LSB, j );
      } else {
        sig->dim[j].lsb = prange->dim[i].right->num;
      }
      j++;
    }

    /* If exclude_mode is not zero, set the exclude bit in the signal */
    sig->suppl.part.excluded = (exclude_mode > 0) ? 1 : 0;

    /* Specify that we should not deallocate the expressions */
    if( prange != NULL ) {
      prange->exp_dealloc = FALSE;
    }
    if( urange != NULL ) {
      urange->exp_dealloc = FALSE;
    }
  }

  /* Only do the following if the signal was not previously found */
  if( !found ) {

    /* Add the signal to either the functional unit or a generate item */
    if( (generate_top_mode > 0) && (type != SSUPPL_TYPE_GENVAR) ) {
      last_gi = gen_item_create_sig( sig );
      if( curr_gi_block != NULL ) {
        db_gen_item_connect( curr_gi_block, last_gi );
      } else {
        curr_gi_block = last_gi;
      }
    } else {
      /* Add signal to current module's signal list */
      sig_link_add( sig, TRUE, &(curr_funit->sigs), &(curr_funit->sig_size), &(curr_funit->sig_no_rm_index) );
    }

    /* Indicate if signal must be assigned by simulated results or not */
    if( mba ) {
      sig->suppl.part.mba      = 1;
      sig->suppl.part.assigned = 1;
    }

    /* Indicate signed attribute */
    sig->value->suppl.part.is_signed = is_signed;

    /* Indicate handled attribute */
    sig->suppl.part.not_handled = handled ? 0 : 1;

    /* Set the implicit_size attribute */
    sig->suppl.part.implicit_size = (((type == SSUPPL_TYPE_INPUT_NET)  || (type == SSUPPL_TYPE_INPUT_REG) ||
                                      (type == SSUPPL_TYPE_OUTPUT_NET) || (type == SSUPPL_TYPE_OUTPUT_REG) ||
                                      (type == SSUPPL_TYPE_INOUT_NET)  || (type == SSUPPL_TYPE_INOUT_REG)) &&
                                     (prange != NULL) && prange->dim[0].implicit &&
                                     (prange->dim[0].left->exp == NULL) && (prange->dim[0].left->num == 0) &&
                                     (prange->dim[0].right->exp == NULL) && (prange->dim[0].right->num == 0)) ? 1 : 0;

  }

  PROFILE_END;
  
}

/*!
 Allocates and adds an enum_item to the current module's list to be elaborated later.
*/
void db_add_enum(
  vsignal*     enum_sig,  /*!< Pointer to signal created for the given enumerated value */
  static_expr* value      /*!< Value to later assign to the enum_sig (during elaboration) */
) { PROFILE(DB_ADD_ENUM);

  assert( enum_sig != NULL );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_enum, sig_name: %s", enum_sig->name );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  enumerate_add_item( enum_sig, value, curr_funit );

  PROFILE_END;

}

/*!
 Called after an entire enum list has been parsed and added to the database.
*/
void db_end_enum_list() { PROFILE(DB_END_ENUM_LIST);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "In db_end_enum_list", DEBUG, __FILE__, __LINE__ );
  }
#endif 

  enumerate_end_list( curr_funit );

  PROFILE_END;

}

/*!
 Adds the given names and information to the list of typedefs for the current module.
*/
void db_add_typedef(
  const char* name,         /*!< Typedef name for this type */
  bool        is_signed,    /*!< Specifies if this typedef is signed or not */
  bool        is_handled,   /*!< Specifies if this typedef is handled or not */
  bool        is_sizeable,  /*!< Specifies if a range can be later placed on this value */
  sig_range*  prange,       /*!< Dimensional packed range information for this value */
  sig_range*  urange        /*!< Dimensional unpacked range information for this value */
) { PROFILE(DB_ADD_TYPEDEF);

  typedef_item* tdi;   /* Typedef item to create */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_typedef, name: %s, is_signed: %d, is_handled: %d, is_sizeable: %d",
                                name, is_signed, is_handled, is_sizeable );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Allocate memory and initialize the structure */
  tdi              = (typedef_item*)malloc_safe( sizeof( typedef_item ) );
  tdi->name        = strdup_safe( name );
  tdi->is_signed   = is_signed;
  tdi->is_handled  = is_handled;
  tdi->is_sizeable = is_sizeable;
  tdi->prange      = prange;
  tdi->urange      = urange;
  tdi->next        = NULL;

  /* Add it the current module's typedef list */
  if( curr_funit->tdi_head == NULL ) {
    curr_funit->tdi_head = curr_funit->tdi_tail = tdi;
  } else {
    curr_funit->tdi_tail->next = tdi;
    curr_funit->tdi_tail       = tdi;
  }

  /* Specify that the prange and urange expressions should not be deallocated */
  if( prange != NULL ) {
    prange->exp_dealloc = FALSE;
  }
  if( urange != NULL ) {
    urange->exp_dealloc = FALSE;
  }

  PROFILE_END;

}

/*!
 \return Returns pointer to the found signal.

 \throws anonymous Throw

 Searches signal matching the specified name using normal scoping rules.  If the signal is
 found, returns a pointer to the calling function for that signal.  If the signal is not
 found, emits a user error and immediately halts execution.
*/
vsignal* db_find_signal(
  char* name,              /*!< String name of signal to find in current module */
  bool  okay_if_not_found  /*!< If set to TRUE, does not emit error message if signal is not found (returns NULL) */
) { PROFILE(DB_FIND_SIGNAL);

  vsignal*   found_sig;    /* Pointer to found signal (return value) */
  func_unit* found_funit;  /* Pointer to found functional unit (not used) */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_find_signal, searching for signal %s", obf_sig( name ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  if( !scope_find_signal( name, curr_funit, &found_sig, &found_funit, 0 ) && !okay_if_not_found ) {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find variable %s in module %s", obf_sig( name ), obf_funit( curr_funit->name ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

  return( found_sig );

}

/*!
 Adds the specified generate item block to the list of generate blocks for
 the current functional unit.
*/
void db_add_gen_item_block(
  gen_item* gi  /*!< Pointer to the head of a generate item block to add */
) { PROFILE(DB_ADD_GEN_ITEM_BLOCK);

  if( gi != NULL ) {

    /* Add the generate block to the list of generate blocks for this functional unit */
    gitem_link_add( gi, &(curr_funit->gitem_head), &(curr_funit->gitem_tail) );

  }

  PROFILE_END;

}

/*!
 \return Returns pointer to found matching generate item (NULL if not found)

 Searches the current functional unit for the generate item that matches the
 specified generate item.  If it is found, a pointer to the stored generate
 item is returned.  If it is not found, a value of NULL is returned.  Additionally,
 the specified generate item is automatically deallocated on behalf of the caller.
 This function should only be called during the parsing stage.
*/
gen_item* db_find_gen_item(
  gen_item* root,  /*!< Pointer to root generate item to start searching in */
  gen_item* gi     /*!< Pointer to created generate item to search for */
) { PROFILE(DB_FIND_GEN_ITEM);

  gen_item* found;  /* Return value for this function */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_find_gen_item, type %d", gi->suppl.part.type );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Search for the specified generate item */
  found = gen_item_find( root, gi );

  /* Deallocate the user-specified generate item */
  gen_item_dealloc( gi, FALSE );

  PROFILE_END;

  return( found );

}

/*!
 \return Returns pointer to found typedef item or NULL if none was found.

 Searches for the given typedef name in the current module.
*/
typedef_item* db_find_typedef(
  const char* name  /*!< Name of typedef to search for */
) { PROFILE(DB_FIND_TYPEDEF);

  func_unit*    parent;      /* Pointer to parent module */
  typedef_item* tdi = NULL;  /* Pointer to current typedef item */

  assert( name != NULL );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_find_typedef, searching for name: %s", name );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  if( curr_funit != NULL ) {

    parent = funit_get_curr_module( curr_funit );

    tdi = parent->tdi_head;
    while( (tdi != NULL) && (strcmp( tdi->name, name ) != 0) ) {
      tdi = tdi->next;
    }

    /* If we could not find the typedef in the current functional unit, look in the global funit, if it exists */
    if( (tdi == NULL) && (global_funit != NULL) ) {
      tdi = global_funit->tdi_head;
      while( (tdi != NULL) && (strcmp( tdi->name, name ) != 0) ) {
        tdi = tdi->next;
      }
    }

  }

  PROFILE_END;

  return( tdi );

}

/*!
 \return Returns a pointer to the last generate item added to the current functional unit.
*/
gen_item* db_get_curr_gen_block() { PROFILE(DB_GET_CURR_GEN_BLOCK);

  gen_item* block = curr_gi_block;  /* Temporary pointer to current generate item block */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_get_curr_gen_block (%p)", block );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Clear the curr_gi_block and last_gi pointers */
  curr_gi_block = NULL;
  last_gi       = NULL;

  PROFILE_END;

  return( block );

}

/*!
 \return Returns the number of signals in the current function unit.
*/
int db_curr_signal_count() { PROFILE(DB_CURR_SIGNAL_COUNT);

  PROFILE_END;

  return( curr_funit->sig_size );

}

/*!
 \return Returns pointer to newly created expression.

 \throws anonymous expression_create Throw

 Creates a new expression with the specified parameter information and returns a
 pointer to the newly created expression.
*/
expression* db_create_expression(
  expression*  right,     /*!< Pointer to expression on right side of expression */
  expression*  left,      /*!< Pointer to expression on left side of expression */
  exp_op_type  op,        /*!< Operation to perform on expression */
  bool         lhs,       /*!< Specifies this expression is a left-hand-side assignment expression */
  unsigned int line,      /*!< Line number of current expression */
  unsigned int ppfline,   /*!< First line number from preprocessed file */
  unsigned int pplline,   /*!< Last line number from preprocessed file */
  int          first,     /*!< Column index of first character in this expression */
  int          last,      /*!< Column index of last character in this expression */
  char*        sig_name,  /*!< Name of signal that expression is attached to (if valid) */
  bool         in_static  /*!< Set to TRUE if this expression exists in a static expression position */
) { PROFILE(DB_CREATE_EXPRESSION);

  expression* expr;        /* Temporary pointer to newly created expression */
  func_unit*  func_funit;  /* Pointer to function, if we are nested in one */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    int          right_id;    /* ID of right expression */
    int          left_id;     /* ID of left expression */
    unsigned int rv;          /* Return value from snprintf call */

    if( right == NULL ) {
      right_id = 0;
    } else {
      right_id = right->id;
    }

    if( left == NULL ) {
      left_id = 0;
    } else {
      left_id = left->id;
    }

    if( sig_name == NULL ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expression, right: %d, left: %d, id: %d, op: %s, lhs: %d, line: %d, first: %d, last: %d", 
                     right_id, left_id, curr_expr_id, expression_string_op( op ), lhs, line, first, last );
    } else {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expression, right: %d, left: %d, id: %d, op: %s, lhs: %d, line: %d, first: %d, last: %d, sig_name: %s",
                     right_id, left_id, curr_expr_id, expression_string_op( op ), lhs, line, first, last, sig_name );
    }
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Check to see if current expression is in a function */
  func_funit = funit_get_curr_function( curr_funit );

  /* Check to make sure that expression is allowed for the current functional unit type */
  if( (func_funit != NULL) &&
      ((op == EXP_OP_DELAY) ||
       (op == EXP_OP_TASK_CALL) ||
       (op == EXP_OP_NASSIGN)   ||
       (op == EXP_OP_PEDGE)     ||
       (op == EXP_OP_NEDGE)     ||
       (op == EXP_OP_AEDGE)     ||
       (op == EXP_OP_EOR)) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Attempting to use a delay, task call, non-blocking assign or event controls in function %s, file %s, line %u",
                                obf_funit( func_funit->name ), obf_file( curr_funit->orig_fname ), line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Create expression with next expression ID */
  expr = expression_create( right, left, op, lhs, curr_expr_id, line, ppfline, pplline, first, last, FALSE );
  curr_expr_id++;

  /* If current functional unit is nested in a function, set the IN_FUNC supplemental field bit */
  expr->suppl.part.in_func = (func_funit != NULL) ? 1 : 0;

  /* Set the clear_changed bit if any of our children have their clear_changed bit set or if we are a system function expression */
  if( ((left  != NULL) && 
       ((left->suppl.part.clear_changed == 1) ||
        (left->op == EXP_OP_STIME) || (left->op == EXP_OP_SRANDOM) || (left->op == EXP_OP_SURANDOM) || (left->op == EXP_OP_SURAND_RANGE) ||
        (left->op == EXP_OP_SB2R)  || (left->op == EXP_OP_SR2B)    || (left->op == EXP_OP_SI2R)     || (left->op == EXP_OP_SR2I) ||
        (left->op == EXP_OP_SB2SR) || (left->op == EXP_OP_SSR2B))) ||
      ((right != NULL) &&
       ((right->suppl.part.clear_changed == 1) ||
        (right->op == EXP_OP_STIME) || (right->op == EXP_OP_SRANDOM) || (right->op == EXP_OP_SURANDOM) || (right->op == EXP_OP_SURAND_RANGE) ||
        (right->op == EXP_OP_SB2R)  || (right->op == EXP_OP_SR2B)    || (right->op == EXP_OP_SI2R)     || (right->op == EXP_OP_SR2I) ||
        (right->op == EXP_OP_SB2SR) || (right->op == EXP_OP_SSR2B))) ) {
    expr->suppl.part.clear_changed = 1;
  }

  /* If we are in exclude mode, set the exclude and stmt_exclude bits */
  if( exclude_mode > 0 ) {
    expr->suppl.part.excluded = 1;
  }

  /*
   If this is some kind of assignment expression operator, set the our expression vector to that of
   the right expression.
  */
  if( (expr->op == EXP_OP_BASSIGN) ||
      (expr->op == EXP_OP_NASSIGN) ||
      (expr->op == EXP_OP_RASSIGN) ||
      (expr->op == EXP_OP_DASSIGN) ||
      (expr->op == EXP_OP_ASSIGN)  ||
      (expr->op == EXP_OP_IF)      ||
      (expr->op == EXP_OP_WHILE)   ||
      (expr->op == EXP_OP_DIM)     ||
      (expr->op == EXP_OP_DLY_ASSIGN) ) {
    vector_dealloc( expr->value );
    expr->suppl.part.owns_vec = 0;
    expr->value = right->value;
  }

  /* Add expression and signal to binding list */
  if( sig_name != NULL ) {

    /*
     If we are in a generate block and the signal name contains a generate variable/expression,
     create a generate item to handle the binding later.
    */
    if( (generate_mode > 0) && gen_item_varname_contains_genvar( sig_name ) ) {
      last_gi = gen_item_create_bind( sig_name, expr );
      if( curr_gi_block != NULL ) {
        db_gen_item_connect( curr_gi_block, last_gi );
      } else {
        curr_gi_block = last_gi;
      }
    } else {
      switch( op ) {
        case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    sig_name, expr, curr_funit, in_static );  break;
        case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        sig_name, expr, curr_funit, FALSE );      break;
        case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, sig_name, expr, curr_funit, FALSE );      break;
        case EXP_OP_DISABLE   :  bind_add( 1,                 sig_name, expr, curr_funit, FALSE );      break;
        default               :  bind_add( 0,                 sig_name, expr, curr_funit, FALSE );      break;
      }
    }

  }

  PROFILE_END;
 
  return( expr );

}

/*!
 Recursively iterates through the entire expression tree binding all selection expressions within that tree
 to the given signal.
*/
void db_bind_expr_tree(
  expression* root,     /*!< Pointer to root of expression tree to bind */
  char*       sig_name  /*!< Name of signal to bind to */
) { PROFILE(DB_BIND_EXPR_TREE);

  assert( sig_name != NULL );

  if( root != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_bind_expr_tree, root id: %d, sig_name: %s", root->id, sig_name );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Bind the children first */
    db_bind_expr_tree( root->left,  sig_name );
    db_bind_expr_tree( root->right, sig_name );

    /* Now bind ourselves if necessary */
    if( (root->op == EXP_OP_SBIT_SEL) ||
        (root->op == EXP_OP_MBIT_SEL) ||
        (root->op == EXP_OP_MBIT_POS) ||
        (root->op == EXP_OP_MBIT_NEG) ) {
      bind_add( 0, sig_name, root, curr_funit, FALSE );
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to an expression that represents the static expression specified

 \throws anonymous db_create_expression Throw

 Creates an expression structure from a static expression structure.
*/
expression* db_create_expr_from_static(
  static_expr* se,         /*!< Pointer to static expression structure */
  unsigned int line,       /*!< Line number that static expression was found on */
  unsigned int ppfline,    /*!< First line number from preprocessed file that static expression was found on */
  unsigned int pplline,    /*!< Last line number from preprocessed file that static expression was found on */
  int          first_col,  /*!< Column that the static expression starts on */
  int          last_col    /*!< Column that the static expression ends on */
) { PROFILE(DB_CREATE_EXPR_FROM_STATIC);

  expression* expr = NULL;  /* Return value for this function */
  vector*     vec;          /* Temporary vector */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expr_from_static, se: %p, line: %d, first_col: %d, last_col: %d",
                                se, line, first_col, last_col );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  Try {

    if( se->exp == NULL ) {

      /* This static expression is a static value so create a static expression from its value */
      expr = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, line, ppfline, pplline, first_col, last_col, NULL, TRUE );

      /* Create the new vector */
      vec = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
      (void)vector_from_int( vec, se->num );

      /* Assign the new vector to the expression's vector (after deallocating the expression's old vector) */
      assert( expr->value->value.ul == NULL );
      free_safe( expr->value, sizeof( vector ) );
      expr->value = vec;

    } else {

      /* The static expression is unresolved, so just get its expression */
      expr = se->exp;

    }

  } Catch_anonymous {
    static_expr_dealloc( se, FALSE );
    Throw 0;
  }

  /* Deallocate static expression */
  static_expr_dealloc( se, FALSE );

  PROFILE_END;

  return( expr );

}

/*!
 Adds the specified expression to the current module's expression list.
*/
void db_add_expression(
  expression* root  /*!< Pointer to root expression to add to module expression list */
) { PROFILE(DB_ADD_EXPRESSION);

  if( (root != NULL) && (root->suppl.part.exp_added == 0) ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_expression, id: %d, op: %s, line: %d, gen_top_mode: %d, gen_expr: %d", 
                                  root->id, expression_string_op( root->op ), root->line, generate_top_mode, root->suppl.part.gen_expr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    if( generate_top_mode > 0 ) {

      if( root->suppl.part.gen_expr == 1 ) {

        /* Add root expression to the generate item list for the current functional unit */
        last_gi = gen_item_create_expr( root );

        /* Attach it to the curr_gi_block, if one exists */
        if( curr_gi_block != NULL ) {
          db_gen_item_connect( curr_gi_block, last_gi );
        } else {
          curr_gi_block = last_gi;
        }

      }

    } else {

      /* Add expression's children first. */
      db_add_expression( root->right );
      db_add_expression( root->left );

      /* Now add this expression to the list. */
      exp_link_add( root, &(curr_funit->exps), &(curr_funit->exp_size) );

    }

    /* Specify that this expression has already been added */
    root->suppl.part.exp_added = 1;

  }

  PROFILE_END;

}

/*!
 \return Returns expression tree to execute a sensitivity list for the given statement block.

 \throws anonymous db_create_expression db_create_expression db_create_expression db_create_expression Throw
*/
expression* db_create_sensitivity_list(
  statement* stmt  /*!< Pointer to statement block to parse */
) { PROFILE(DB_CREATE_SENSITIVITY_LIST);

  str_link*   sig_head = NULL;  /* Pointer to head of signal name list containing RHS used signals */
  str_link*   sig_tail = NULL;  /* Pointer to tail of signal name list containing RHS used signals */
  str_link*   strl;             /* Pointer to current signal name link */
  expression* exps;             /* Pointer to created expression for type SIG */
  expression* expa;             /* Pointer to created expression for type AEDGE */
  expression* expe;             /* Pointer to created expression for type EOR */
  expression* expc     = NULL;  /* Pointer to left child expression */

  /* Get the list of all RHS signals in the given statement block */
  statement_find_rhs_sigs( stmt, &sig_head, &sig_tail );

  /* Create sensitivity expression tree for the list of RHS signals */
  if( sig_head != NULL ) {

    Try {

      strl = sig_head;
      while( strl != NULL ) {

        /* Create AEDGE and EOR for subsequent signals */
        exps = db_create_expression( NULL, NULL, EXP_OP_SIG,   FALSE, 0, 0, 0, 0, 0, strl->str, FALSE );
        expa = db_create_expression( exps, NULL, EXP_OP_AEDGE, FALSE, 0, 0, 0, 0, 0, NULL, FALSE );

        /* If we have a child expression already, create the EOR expression to connect them */
        if( expc != NULL ) {
          expe = db_create_expression( expa, expc, EXP_OP_EOR, FALSE, 0, 0, 0, 0, 0, NULL, FALSE );
          expc = expe;
        } else {
          expc = expa;
        }

        strl = strl->next;

      }

    } Catch_anonymous {
      str_link_delete_list( sig_head );
      Throw 0;
    }

    /* Deallocate string list */
    str_link_delete_list( sig_head );

  }

  PROFILE_END;

  return( expc );

}


/*!
 \return Returns pointer to parallelized statement block

 \throws anonymous db_create_statement Throw db_create_expression
*/
statement* db_parallelize_statement(
  statement* stmt  /*!< Pointer to statement to check for parallelization */
) { PROFILE(DB_PARALLELIZE_STATEMENT);

  expression* exp;         /* Expression containing FORK statement */
  char*       scope;       /* Name of current parallelized statement scope */

  /* If we are a parallel statement, create a FORK statement for this statement block */
  if( (stmt != NULL) && (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_parallelize_statement, id: %d, %s, line: %d, fork_depth: %d, block_depth: %d, fork_block_depth: %d",
                                  stmt->exp->id, expression_string_op( stmt->exp->op ), stmt->exp->line, fork_depth, block_depth, fork_block_depth[fork_depth] );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Create FORK expression */
    exp = db_create_expression( NULL, NULL, EXP_OP_FORK, FALSE, stmt->exp->line, stmt->exp->ppfline, stmt->exp->pplline, stmt->exp->col.part.first, stmt->exp->col.part.last, NULL, FALSE );

    /* Create unnamed scope */
    scope = db_create_unnamed_scope();
    if( db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, scope, curr_funit->orig_fname, curr_funit->incl_fname, stmt->exp->line, stmt->exp->col.part.first ) ) {

      /* Specify that the block was used for forking */
      curr_funit->suppl.part.fork = 1;

      /* Create a thread block for this statement block */
      stmt->suppl.part.head      = 1;
      stmt->suppl.part.is_called = 1;
      db_add_statement( stmt, stmt );

      /* Bind the FORK expression now */
      exp->elem.funit      = curr_funit;
      exp->suppl.part.type = ETYPE_FUNIT;
      exp->name            = strdup_safe( scope );

      /* Restore the original functional unit */
      db_end_function_task_namedblock( stmt->exp->line );

    }
    free_safe( scope, (strlen( scope ) + 1) );

    /* Reduce fork and block depth for the new statement */
    fork_depth--;
    block_depth--;

    Try {

      /* Create FORK statement and add the expression */
      stmt = db_create_statement( exp );

    } Catch_anonymous {
      expression_dealloc( exp, FALSE );
      Throw 0;
    }

    /* Restore fork and block depth values for parser */
    fork_depth++;
    block_depth++;

  }

  PROFILE_END;

  return( stmt );

}

/*!
 \return Returns pointer to created statement.

 \throws anonymous db_parallelize_statement Throw

 Creates an statement structure and adds created statement to current
 module's statement list.
*/
statement* db_create_statement(
  expression*  exp  /*!< Pointer to associated "root" expression */
) { PROFILE(DB_CREATE_STATEMENT);

  statement* stmt = NULL;  /* Pointer to newly created statement */

  /* If the statement expression is NULL, we can't create the statement */
  if( exp != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      char*        exp_str = strdup_safe( expression_string( exp ) );
      unsigned int rv      = snprintf( user_msg, USER_MSG_LENGTH, "In db_create_statement, %s", exp_str );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
      free_safe( exp_str, (strlen( exp_str ) + 1) );
    }
#endif

    /* Create the given statement */
    stmt = statement_create( exp, curr_funit );

    /* If we are in the exclude mode, exclude this statement */
    if( exclude_mode > 0 ) {
      stmt->suppl.part.excluded = 1;
    }

    /* If we need to exclude this statement from race condition checking, do so */
    if( ignore_racecheck_mode > 0 ) {
      stmt->suppl.part.ignore_rc = 1;
    }

    Try {

      /* If we are a parallel statement, create a FORK statement for this statement block */
      stmt = db_parallelize_statement( stmt );

    } Catch_anonymous {
      statement_dealloc( stmt );
      expression_dealloc( exp, FALSE );
      Throw 0;
    }

  }

  PROFILE_END;

  return( stmt );

}

/*!
 Adds the specified statement tree to the tail of the current module's statement list.
 The start statement is specified to avoid infinite looping.
*/
void db_add_statement(
  statement* stmt,  /*!< Pointer to statement add to current module's statement list */
  statement* start  /*!< Pointer to starting statement of statement tree */
) { PROFILE(DB_ADD_STATEMENT);
 
  if( (stmt != NULL) && (stmt->suppl.part.added == 0) ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_add_statement, id: %d, start id: %d", stmt->exp->id, start->exp->id );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Set the head statement pointer */
    stmt->head = start;

    /* Now add current statement */
    if( generate_top_mode > 0 ) {

      last_gi = gen_item_create_stmt( stmt );

      if( curr_gi_block != NULL ) {
        db_gen_item_connect( curr_gi_block, last_gi );
      } else {
        curr_gi_block = last_gi;
      }

    } else {

      /* Add the associated expression tree */
      db_add_expression( stmt->exp );

      /* Add TRUE and FALSE statement paths to list */
      if( (stmt->suppl.part.stop_false == 0) && (stmt->next_false != start) ) {
        db_add_statement( stmt->next_false, start );
      }

      if( (stmt->suppl.part.stop_true == 0) && (stmt->next_true != stmt->next_false) && (stmt->next_true != start) ) {
        db_add_statement( stmt->next_true, start );
      }

      /* Set ADDED bit of this statement */
      stmt->suppl.part.added = 1;

      /* Make sure that the current statement is pointing to the current functional unit */
      stmt->funit = curr_funit;

      /* Finally, add the statement to the functional unit statement list */
      stmt_link_add( stmt, TRUE, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

    }

  }

  PROFILE_END;

}
#endif

/*!
 Removes specified statement expression from the current functional unit.  Called by statement_dealloc_recursive in
 statement.c in its deallocation algorithm.
*/
void db_remove_statement_from_current_funit(
  statement* stmt  /*!< Pointer to statement to remove from memory */
) { PROFILE(DB_REMOVE_STATEMENT_FROM_CURRENT_FUNIT);

  inst_link* instl;  /* Pointer to current functional unit instance */

  if( (stmt != NULL) && (stmt->exp != NULL) ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement_from_current_funit %s, stmt id: %d, %s, line: %d",
                                  obf_funit( curr_funit->name ), stmt->exp->id, expression_string_op( stmt->exp->op ), stmt->exp->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /*
     Get a list of all parameters within the given statement expression tree and remove them from
     an instance and module parameters.
    */
    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      instance_remove_parms_with_expr( instl->inst, stmt );
      instl = instl->next;
    }

    /* Remove expression from current module expression list and delete expressions */
    exp_link_remove( stmt->exp, &(curr_funit->exps), &(curr_funit->exp_size), TRUE );

    /* Remove this statement link from the current module's stmt_link list */
    stmt_link_unlink( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

  }

  PROFILE_END;

}

#ifndef VPI_ONLY
/*!
 Removes specified statement expression and its tree from current module expression list and deallocates
 both the expression and statement from heap memory.  Called when a statement structure is
 found to contain a statement that is not supported by Covered.
*/
void db_remove_statement(
  statement* stmt  /*!< Pointer to statement to remove from memory */
) { PROFILE(DB_REMOVE_STATEMENT);

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement, stmt id: %d, line: %d", 
                                  stmt->exp->id, stmt->exp->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Call the recursive statement deallocation function */
    statement_dealloc_recursive( stmt, TRUE );

  }

  PROFILE_END;

}

/*!
 Connects the specified statement's true statement.
*/
void db_connect_statement_true(
  statement* stmt,      /*!< Pointer to statement to connect true path to */
  statement* next_true  /*!< Pointer to statement to run if statement evaluates to TRUE */
) { PROFILE(DB_CONNECT_STATEMENT_TRUE);

#ifdef DEBUG_MODE
  int next_id;  /* Statement ID of next TRUE statement */
#endif

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv;
      if( next_true == NULL ) {
        next_id = 0;
      } else {
        next_id = next_true->exp->id;
      }

      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_connect_statement_true, id: %d, next: %d", stmt->exp->id, next_id );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    stmt->next_true = next_true;

  }

  PROFILE_END;

}

/*!
 Connects the specified statement's false statement.
*/
void db_connect_statement_false(
  statement* stmt,       /*!< Pointer to statement to connect false path to */
  statement* next_false  /*!< Pointer to statement to run if statement evaluates to FALSE */
) { PROFILE(DB_CONNECT_STATEMENT_FALSE);

#ifdef DEBUG_MODE
  int next_id;  /* Statement ID of next FALSE statement */
#endif

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv;
      if( next_false == NULL ) {
        next_id = 0;
      } else {
        next_id = next_false->exp->id;
      }

      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_connect_statement_false, id: %d, next: %d", stmt->exp->id, next_id );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    stmt->next_false = next_false;

  }

  PROFILE_END;

}

/*!
 Connects gi2 to gi1's next_true pointer.
*/
void db_gen_item_connect_true(
  gen_item* gi1,  /*!< Pointer to generate item holding next_true */
  gen_item* gi2   /*!< Pointer to generate item to connect */
) { PROFILE(DB_GEN_ITEM_CONNECT_TRUE);

  assert( gi1 != NULL );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_gen_item_connect_true, gi1: %p, gi2: %p", gi1, gi2 );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  gi1->next_true = gi2;  

  PROFILE_END;

}

/*!
 Connects gi2 to gi1's next_false pointer.
*/
void db_gen_item_connect_false(
  gen_item* gi1,  /*!< Pointer to generate item holding next_false */
  gen_item* gi2   /*!< Pointer to generate item to connect */
) { PROFILE(DB_GEN_ITEM_CONNECT_FALSE);

  assert( gi1 != NULL );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_gen_item_connect_false, gi1: %p, gi2: %p", gi1, gi2 );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  gi1->next_false = gi2;

  PROFILE_END;

}

/*!
 Connects two generate items together.
*/
void db_gen_item_connect(
  gen_item* gi1,  /*!< Pointer to generate item block to connect to gi2 */
  gen_item* gi2   /*!< Pointer to generate item that will be connected to gi1 */
) { PROFILE(DB_GEN_ITEM_CONNECT);

  bool rv;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_gen_item_connect, gi1: %p, gi2: %p, conn_id: %d", gi1, gi2, gi_conn_id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Connect generate items */
  rv = gen_item_connect( gi1, gi2, gi_conn_id, FALSE );
  assert( rv );

  /* Increment gi_conn_id for next connection */
  gi_conn_id++;

  PROFILE_END;

}

/*!
 \return Returns TRUE if statement was properly connected to the given statement list; otherwise,
         returns FALSE.

 Calls the statement_connect function located in statement.c with the specified parameters.  If
 the statement connection was not achieved, displays warning to user and returns FALSE.  The calling
 function should throw this statement away.
*/
bool db_statement_connect(
  statement* curr_stmt,  /*!< Pointer to current statement to attach */
  statement* next_stmt   /*!< Pointer to next statement to attach to */
) { PROFILE(DB_STATEMENT_CONNECT);

  bool retval;  /* Return value for this function */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    int          curr_id;  /* Current statement ID */
    int          next_id;  /* Next statement ID */
    unsigned int rv;

    if( curr_stmt == NULL ) {
      curr_id = 0;
    } else {
      curr_id = curr_stmt->exp->id;
    }

    if( next_stmt == NULL ) {
      next_id = 0;
    } else {
      next_id = next_stmt->exp->id;
    }

    rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_statement_connect, curr_stmt: %d, next_stmt: %d", curr_id, next_id );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /*
   Connect statement, if it was not successful, add it to the functional unit's statement list immediately
   as it will not be later on.
  */
  if( !(retval = statement_connect( curr_stmt, next_stmt, stmt_conn_id )) ) {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unreachable statement found starting at line %u in file %s.  Ignoring...",
                                next_stmt->exp->line, obf_file( curr_funit->orig_fname ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  /* Increment stmt_conn_id for next statement connection */
  stmt_conn_id++;

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns a pointer to the newly created attribute parameter.

 Calls the attribute_create() function and returns the pointer returned by this function.
*/
attr_param* db_create_attr_param(
  char*       name,  /*!< Attribute parameter identifier */
  expression* expr   /*!< Pointer to constant expression that is assigned to the identifier */
) { PROFILE(DB_CREATE_ATTR_PARAM);

  attr_param* attr;  /* Pointer to newly allocated/initialized attribute parameter */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv;
    if( expr != NULL ) {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_create_attr_param, name: %s, expr: %d", name, expr->id );
    } else {
      rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_create_attr_param, name: %s", name );
    }
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  attr = attribute_create( name, expr );

  PROFILE_END;

  return( attr );

}

/*!
 \throws anonymous attribute_parse Throw

 Calls the attribute_parse() function and deallocates this list.
*/
void db_parse_attribute(
  attr_param* ap  /*!< Pointer to attribute parameter list to parse */
) { PROFILE(DB_PARSE_ATTRIBUTE);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "In db_parse_attribute", DEBUG, __FILE__, __LINE__ );
  }
#endif

  Try {

    /* First, parse the entire attribute */
    attribute_parse( ap, curr_funit, (exclude_mode > 0) );

  } Catch_anonymous {
    attribute_dealloc( ap );
    Throw 0;
  }

  /* Then deallocate the structure */
  attribute_dealloc( ap );

  PROFILE_END;

}
#endif /* VPI_ONLY */

/*!
 \return Returns a pointer to a list of all expressions found that call
         the specified statement.  Returns NULL if no expressions were
         found in the design that match this statement.

 Searches the list of all expressions in all functional units that call
 the specified statement and returns these in a list format to the calling
 function.  This function should only be called after the entire design has
 been parsed to be completely correct.
*/
void db_remove_stmt_blks_calling_statement(
  statement* stmt  /*!< Pointer to statement to compare with all expressions */
) { PROFILE(DB_REMOVE_STMT_BLKS_CALLING_STATEMENT);

  inst_link* instl;  /* Pointer to current instance */ 

  assert( stmt != NULL );

  instl = db_list[curr_db]->inst_head;
  while( instl != NULL ) {
    instance_remove_stmt_blks_calling_stmt( instl->inst, stmt );
    instl = instl->next;
  }

  PROFILE_END;

}

/*!
 \return Returns the string version of the current instance scope (memory allocated).
*/
static char* db_gen_curr_inst_scope() { PROFILE(DB_GEN_CURR_INST_SCOPE);

  char* scope      = NULL;  /* Pointer to current scope */
  int   scope_size = 0;     /* Calculated size of current instance scope */
  int   i;                  /* Loop iterator */

  if( curr_inst_scope_size > 0 ) {

    /* Calculate the total number of characters in the given scope (include . and newline chars) */
    for( i=0; i<curr_inst_scope_size; i++ ) {
      scope_size += strlen( curr_inst_scope[i] ) + 1;
    }

    /* Allocate memory for the generated current instance scope */
    scope = (char*)malloc_safe( scope_size );

    /* Now populate the scope with the current instance scope information */
    strcpy( scope, curr_inst_scope[0] );
    for( i=1; i<curr_inst_scope_size; i++ ) {
      strcat( scope, "." );
      strcat( scope, curr_inst_scope[i] );
    }

  }

  PROFILE_END;

  return scope;

}

/*!
 Synchronizes the curr_instance pointer to match the curr_inst_scope hierarchy.
*/
void db_sync_curr_instance() { PROFILE(DB_SYNC_CURR_INSTANCE);
 
  char  stripped_scope[4096];  /* Temporary string */
  char* scope;                 /* Current instance scope string */

  assert( db_list[curr_db]->leading_hier_num > 0 );

  if( (scope = db_gen_curr_inst_scope()) != NULL ) {

    if( scope[0] != '\0' ) {
      curr_instance = inst_link_find_by_scope( scope, db_list[curr_db]->inst_head, TRUE );
    }

    free_safe( scope, (strlen( scope ) + 1) );

  } else {

    curr_instance = NULL;

  }

  PROFILE_END;

} 

/*!
 Sets the curr_inst_scope global variable to the specified scope.
*/
void db_set_vcd_scope(
  const char* scope  /*!< Current VCD scope */
) { PROFILE(DB_SET_VCD_SCOPE);

  char tmp_scope[4096];
  int  tmp_index;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_set_vcd_scope, scope: %s", obf_inst( scope ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  assert( scope != NULL );

  /* Create a new scope item */
  curr_inst_scope = (char**)realloc_safe( curr_inst_scope, (sizeof( char* ) * curr_inst_scope_size), (sizeof( char* ) * (curr_inst_scope_size + 1)) );

  /* If this is a Verilator run and the scope refers to a generated scope, we need to switch the parenthesis to brackets */
  if( sscanf( scope, "%[^(](%d)", tmp_scope, &tmp_index ) == 2 ) {
    char         index_str[30];
    unsigned int rv;
    strcat( tmp_scope, "[" );
    rv = snprintf( index_str, 30, "%d", tmp_index );
    assert( rv < 30 );
    strcat( tmp_scope, index_str );
    strcat( tmp_scope, "]" );
    curr_inst_scope[curr_inst_scope_size] = strdup_safe( tmp_scope );
  } else {
    curr_inst_scope[curr_inst_scope_size] = strdup_safe( scope );
  }
  curr_inst_scope_size++;

  /* Synchronize the current instance to the value of curr_inst_scope */
  db_sync_curr_instance();

  PROFILE_END;

}

/*!
 Moves the curr_inst_scope up one level of hierarchy.  This function is called
 when the $upscope keyword is seen in a VCD file.
*/
void db_vcd_upscope() { PROFILE(DB_VCD_UPSCOPE);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char*        scope = db_gen_curr_inst_scope();
    unsigned int rv    = snprintf( user_msg, USER_MSG_LENGTH, "In db_vcd_upscope, curr_inst_scope: %s", obf_inst( scope ) );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    free_safe( scope, (strlen( scope ) + 1) );
  }
#endif

  /* Deallocate the last scope item */
  if( curr_inst_scope_size > 0 ) {

    curr_inst_scope_size--;
    free_safe( curr_inst_scope[curr_inst_scope_size], (strlen( curr_inst_scope[curr_inst_scope_size] ) + 1) );
    curr_inst_scope = (char**)realloc_safe( curr_inst_scope, (sizeof( char* ) * (curr_inst_scope_size + 1)), (sizeof( char* ) * curr_inst_scope_size) );

    db_sync_curr_instance();

  }

  PROFILE_END;

}

/*!
 Creates a new entry in the symbol table for the specified signal and symbol.
*/
void db_assign_symbol(
  char*       name,    /*!< Name of signal/expression to set value to */
  const char* symbol,  /*!< Symbol of the associated signal/expression symbol */
  int         msb,     /*!< Most significant bit of symbol to set */
  int         lsb      /*!< Least significant bit of symbol to set */
) { PROFILE(DB_ASSIGN_SYMBOL);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    char*        scope = db_gen_curr_inst_scope();
    unsigned int rv    = snprintf( user_msg, USER_MSG_LENGTH, "In db_assign_symbol, name: %s, symbol: %s, curr_inst_scope: %s, msb: %d, lsb: %d",
                                   obf_sig( name ), symbol, obf_inst( scope ), msb, lsb );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    free_safe( scope, (strlen( scope ) + 1) );
  }
#endif

  assert( name != NULL );

  if( (curr_instance != NULL) && (curr_instance->funit != NULL) ) {

    char* found_str;
    bool  internal_sig_found = ((found_str = strstr( name, "\\covered$" )) != NULL) || ((found_str = strstr( name, "covered$" )) != NULL);
    
    if( info_suppl.part.inlined && internal_sig_found ) {

      unsigned int index = (found_str - name) + ((name[found_str-name] == '\\') ? 9 : 8);
      char         type  = name[index];

      /* If the type is an x (temporary register) or a y (temporary wire), don't continue on */
      if( (type != 'x') && (type != 'X') && (type != 'i') && (type != 'I') && (type != 'Z') ) {

        expression*  exp;
        funit_inst*  curr_inst = curr_instance;
        unsigned int rv;

        /*
         If the covered string was not at the beginning of the name string, we have some hierarchical referencing in the signal name that
         we need to resolve.
        */
        if( found_str != name ) {
          char tscope[4096];
          name[(found_str-name)-1] = '\0';
          rv   = snprintf( tscope, 4096, "%s.%s", curr_instance->name, name );
          assert( rv < 4096 );
          curr_inst = instance_find_scope( curr_instance, tscope, FALSE );
        }

        /* Handle line coverage */
        if( type == 'L' ) {
      
          unsigned int fline;
          unsigned int lline;
          unsigned int col;
          char         scope[4096];
          funit_inst*  inst = curr_inst;

          /* Extract the line, first column and funit scope information from name */
          if( sscanf( (name + (index + 1)), "%u_%u_%x$%s", &fline, &lline, &col, scope ) == 4 ) {

            char tscope[4096];
            int  i;

#ifdef OBSOLETE
            /* Replace the '/' keyword with '.' in the scope */
            for( i=0; i<strlen( scope ); i++ ) {
              if( scope[i] == '/' ) {
                scope[i] = '.';
              }
            }
#endif

            /* Get the relative instance that contains the expression */
            rv   = snprintf( tscope, 4096, "%s.%s", curr_inst->name, scope );
            assert( rv < 4096 );
            inst = instance_find_scope( curr_inst, tscope, FALSE );

          } else {

            rv = sscanf( (name + (index + 1)), "%u_%u_%x", &fline, &lline, &col );
            assert( rv == 3 );

          }

          if( inst != NULL ) {

            unsigned int i = 0;
            /* Search the matching expression */
            while( (i < inst->funit->exp_size) &&
                   ((inst->funit->exps[i]->ppfline != fline)       ||
                    (inst->funit->exps[i]->col.all != col)         ||
                    (inst->funit->exps[i]->pplline != lline)       ||
                    !ESUPPL_IS_ROOT( inst->funit->exps[i]->suppl ) ||
                    (inst->funit->exps[i]->op == EXP_OP_FORK)) ) i++;

            assert( i < inst->funit->exp_size );

            /* Add the expression to the symtable */
            symtable_add_expression( symbol, inst->funit->exps[i], type );

          }

        } else if( type == 'F' ) {

          unsigned int id;

          rv = sscanf( (name + (index + 1)), "%u", &id );
          assert( rv == 1 );

          /* Add the FSM table to the symtable */
          symtable_add_fsm( symbol, curr_inst->funit->fsms[id-1], msb, lsb );

        } else if( (type == 'w') || (type == 'W') || (type == 'r') || (type == 'R') ) {

          unsigned int fline;
          unsigned int lline;
          unsigned int col;
          char         scope[4096];
          char         mname[4096];
          funit_inst*  inst = curr_inst;

          if( sscanf( (name + (index + 1)), "%u_%u_%x$%[^$]$%s", &fline, &lline, &col, mname, scope ) == 5 ) {

            char tscope[4096];
            int  i;

#ifdef OBSOLETE
            /* Replace the '/' keyword with '.' in the scope */
            for( i=0; i<strlen( scope ); i++ ) {
              if( scope[i] == '/' ) {
                scope[i] = '.';
              }
            }
#endif

            /* Get the relative instance that contains the expression */
            rv   = snprintf( tscope, 4096, "%s.%s", curr_inst->name, scope );
            assert( rv < 4096 );
            inst = instance_find_scope( curr_inst, tscope, FALSE );

          } else {

            rv = sscanf( (name + (index + 1)), "%u_%u_%x$%s", &fline, &lline, &col, mname );
            assert( rv == 4 );

          }

          if( inst != NULL ) {

            unsigned int i = 0;

            /* Search the matching expression */
            while( (i < inst->funit->exp_size) && 
                   ((inst->funit->exps[i]->ppfline != fline) ||
                    (inst->funit->exps[i]->col.all != col)   ||
                    (inst->funit->exps[i]->pplline != lline)) ) i++;

            assert( i < inst->funit->exp_size );

            /* Add the expression to the symtable */
            symtable_add_memory( symbol, inst->funit->exps[i], type, msb );

          }

        } else {

          unsigned int fline;
          unsigned int lline;
          unsigned int col;
          char         scope[4096];
          funit_inst*  inst = curr_inst;

          /* Extract the line and column (and possibly instance) information */
          if( sscanf( (name + (index + 1)), "%u_%u_%x$%s", &fline, &lline, &col, scope ) == 4 ) {

            char tscope[4096];
            int  i;

#ifdef obsolete
            /* Replace the '/' keyword with '.' in the scope */
            for( i=0; i<strlen( scope ); i++ ) {
              if( scope[i] == '/' ) {
                scope[i] = '.';
              }
            }
#endif

            /* Get the relative instance that contains the expression */
            rv   = snprintf( tscope, 4096, "%s.%s", curr_inst->name, scope );
            assert( rv < 4096 );
            inst = instance_find_scope( curr_inst, tscope, FALSE );

          } else {

            rv = sscanf( (name + (index + 1)), "%u_%u_%x", &fline, &lline, &col );
            assert( rv == 3 );
          
          }

          if( inst != NULL ) {

            unsigned int i = 0;

            /* Search the matching expression */
            while( (i < inst->funit->exp_size) &&
                   ((inst->funit->exps[i]->ppfline != fline) ||
                    (inst->funit->exps[i]->col.all != col)   ||
                    (inst->funit->exps[i]->pplline != lline) ||
                    (inst->funit->exps[i]->op == EXP_OP_FORK)) ) i++;

            assert( i < inst->funit->exp_size );
            exp = inst->funit->exps[i];

            /* If the found expression's parent is an AEDGE, use that expression instead */
            if( (ESUPPL_IS_ROOT( exp->suppl ) == 0) && (exp->parent->expr->op == EXP_OP_AEDGE) ) {
              exp = exp->parent->expr;
            }

            /* Add the expression to the symtable */
            symtable_add_expression( symbol, exp, type );

          }

        }

      }
        
    } else if( info_suppl.part.scored_toggle == 1 ) {

      vsignal*   sig;
      func_unit* found_funit;
      char       tmp_scope[4096];
      tmp_scope[0] = '\0';

      /*
       If we found an internal Covered signal in a CDD that is not expecting inlined data, the user specified a bad CDD file.
       Alert them to this issue and quit immediately.
      */
      if( !info_suppl.part.inlined && internal_sig_found ) {
        print_output( "The CDD file in use was not created for inlined coverage; however,",  FATAL,      __FILE__, __LINE__ );
        print_output( "Covered has detected an inlined signal from the specified dumpfile.", FATAL_WRAP, __FILE__, __LINE__ );
        print_output( "Please use a CDD file created for inlined coverage.",                 FATAL_WRAP, __FILE__, __LINE__ );
        Throw 0;
      }

      /* Find the signal that matches the specified signal name */
      if( ((sig = sig_link_find( name, curr_instance->funit->sigs, curr_instance->funit->sig_size )) != NULL) ||
          scope_find_signal( name, curr_instance->funit, &sig, &found_funit, 0 ) ) {

        /* Only add the symbol if we are not going to generate this value ourselves */
        if( SIGNAL_ASSIGN_FROM_DUMPFILE( sig ) ) {

          /* Add this signal */
          symtable_add_signal( symbol, sig, msb, lsb );

        }

      }

    }

  }

  PROFILE_END;

}

/*!
 Searches the timestep symtable followed by the VCD symbol table searching for
 the symbol that matches the specified argument.  Once a symbol is found, its value
 parameter is set to the specified character.  If the symbol was found in the VCD
 symbol table, it is copied to the timestep symbol table.
*/
void db_set_symbol_char(
  const char* sym,   /*!< Name of symbol to set character value to */
  char        value  /*!< String version of value to set symbol table entry to */
) { PROFILE(DB_SET_SYMBOL_CHAR);

  char val[2];  /* Value to store */

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_char, sym: %s, value: %c", sym, value );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Put together value string */
  val[0] = value;
  val[1] = '\0';

  /* Set value of all matching occurrences in current timestep. */
  symtable_set_value( sym, val );

  PROFILE_END;

}

/*!
 Searches the timestep symtable followed by the VCD symbol table searching for
 the symbol that matches the specified argument.  Once a symbol is found, its value
 parameter is set to the specified string.  If the symbol was found in the VCD
 symbol table, it is copied to the timestep symbol table.
*/
void db_set_symbol_string(
  const char* sym,   /*!< Name of symbol to set character value to */
  const char* value  /*!< String version of value to set symbol table entry to */
) { PROFILE(DB_SET_SYMBOL_STRING);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_string, sym: %s, value: %s", sym, value );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Set value of all matching occurrences in current timestep. */
  symtable_set_value( sym, value );

  PROFILE_END;

}

/*!
 \return Returns TRUE if simulation should continue to advance; otherwise, returns FALSE
         to indicate that simulation should stop immediately.

 \throws anonymous symtable_assign

 Cycles through expression queue, performing expression evaluations as we go.  If
 an expression has a parent expression, that parent expression is placed in the
 expression queue after that expression has completed its evaluation.  When the
 expression queue is empty, we are finished for this clock period.
*/
bool db_do_timestep(
  uint64 time,  /*!< Current time step value being performed */
  bool   final  /*!< Specifies that this is the final timestep */
) { PROFILE(DB_DO_TIMESTEP);

  bool            retval          = TRUE;
  static sim_time curr_time;
  static uint64   last_sim_update = 0;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    if( final ) {
      print_output( "Performing final timestep", DEBUG, __FILE__, __LINE__ );
    } else {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Performing timestep #%" FMT64 "u", time );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
  }
#endif

  num_timesteps++;

  curr_time.lo    = (time & 0xffffffffLL);
  curr_time.hi    = ((time >> 32) & 0xffffffffLL);
  curr_time.full  = time;
  curr_time.final = final;

  if( (timestep_update > 0) && ((time - last_sim_update) >= timestep_update) && !debug_mode && !final ) {
    unsigned int rv;
    last_sim_update = time;
    /*@-formattype -formatcode -duplicatequals@*/
    printf( "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bPerforming timestep %10" FMT64 "u", time );
    /*@=formattype =formatcode =duplicatequals@*/
    rv = fflush( stdout );
    assert( rv == 0 );
  }

  /* Only perform simulation if we are not scoring an inlined coverage design. */
  if( !info_suppl.part.inlined ) {

    /* Simulate the current timestep */
    retval = sim_simulate( &curr_time );

    /* If this is the last timestep, add the final list and do one more simulate */
    if( final && retval ) {
      curr_time.lo   = 0xffffffff;
      curr_time.hi   = 0xffffffff;
      curr_time.full = 0xffffffffffffffffLL;
      retval = sim_simulate( &curr_time );
    }

  }

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "Assigning postsimulation signals...", DEBUG, __FILE__, __LINE__ );
  }
#endif

  if( retval ) {

    /* Assign all stored values in current post-timestep to stored signals */
    symtable_assign( &curr_time );

    /* Perform non-blocking assignment */
    sim_perform_nba( &curr_time );

  }

  PROFILE_END;

  return( retval );

}

/*!
 Checks to make sure that if the current design has any signals that need to be assigned
 from the dumpfile that at least one of these signals was satisfied for this need.
*/
void db_check_dumpfile_scopes() { PROFILE(DB_CHECK_DUMPFILE_SCOPES);

  /* If no signals were used from the VCD dumpfile, check to see if any signals were needed */
  if( vcd_symtab_size == 0 ) {

    funit_link* funitl = db_list[curr_db]->funit_head;

    while( (funitl != NULL) && !funit_is_one_signal_assigned( funitl->funit ) ) {
      funitl = funitl->next;
    }

    /*
     If at least one functional unit contains a signal that needs to be assigned from the dumpfile,
     we have some bad/unuseful dumpfile results.
    */
    if( funitl != NULL ) {

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

  }

  PROFILE_END;

}
#endif /* RUNLIB */

#ifdef RUNLIB
/*!
 \return Returns TRUE if the given CDD file was read in without error.

 Reads the given CDD file and prepares the database for coverage accummulation.
*/
bool db_verilator_initialize(
  const char* cdd_name  /*!< Name of CDD file to open for coverage */
) { PROFILE(DB_VERILATOR_INITIALIZE);

  bool retval = TRUE;

  Try {
    db_read( cdd_name, 0 );
  } Catch_anonymous {
    retval = FALSE;
  }

  if( retval ) {
    Try {
      bind_perform( 1, 0 );
    } Catch_anonymous {
      retval = FALSE;
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the given CDD file was written without error.

 Writes the database to the given CDD filename and deallocates all memory
 associated with the database.
*/
bool db_verilator_close(
  const char* cdd_name  /*!< Name of CDD file to write */
) { PROFILE(DB_VERILATOR_CLOSE);

  bool retval = TRUE;

  /* Set the scored bit */
  info_set_scored();

  /* Write contents to database file */
  Try {
    db_write( cdd_name, 0, 0 );
  } Catch_anonymous {
    retval = FALSE;
  }

  /* Close the database regardless of error */
  db_close();

  PROFILE_END;

  return( retval );

}

/*!
 Gathers line coverage for the specified expression.
*/
bool db_add_line_coverage(
  uint32 inst_index,  /*!< Index of instance to lookup */
  uint32 expr_index   /*!< Index of expression to set */
) { PROFILE(DB_ADD_LINE_COVERAGE);

  bool retval = TRUE;

  /* Perform line coverage on the given expression */
  if( db_list != NULL ) {
    printf( "In db_add_line_coverage, inst_index: %u, expr_index: %u, %s\n", inst_index, expr_index, expression_string( db_list[curr_db]->insts[inst_index]->funit->exps[expr_index] ) );
    expression_set_line_coverage( db_list[curr_db]->insts[inst_index]->funit->exps[expr_index] );
  } else {
    print_output( "Attempting to gather coverage without calling covered_initialize(...)", FATAL, __FILE__, __LINE__ );
    retval = FALSE;
  }

  PROFILE_END;

  return( retval );

}
#endif /* RUNLIB */
