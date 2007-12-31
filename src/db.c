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
extern /*@null@*/inst_link*  inst_head;
extern /*@null@*/inst_link*  inst_tail;
extern str_link*   no_score_head;
extern funit_link* funit_head;
extern funit_link* funit_tail;
extern nibble      or_optab[16];
extern char        user_msg[USER_MSG_LENGTH];
extern bool        one_instance_found;
extern char**      leading_hierarchies;
extern int         leading_hier_num;
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

/*!
 Specifies the string Verilog scope that is currently specified in the VCD file.
*/
/*@null@*/char** curr_inst_scope = NULL;

/*!
 Current size of curr_inst_scope array
*/
int curr_inst_scope_size = 0;

/*!
 Pointer to the current instance selected by the VCD parser.  If this value is
 NULL, the current instance does not reside in the design specified for coverage.
*/
/*@null@*/funit_inst* curr_instance = NULL;

/*!
 Pointer to head of list of module names that need to be parsed yet.  These names
 are added in the db_add_instance function and removed in the db_end_module function.
*/
/*@null@*/str_link* modlist_head  = NULL;

/*!
 Pointer to tail of list of module names that need to be parsed yet.  These names
 are added in the db_add_instance function and removed in the db_end_module function.
*/
/*@null@*/str_link* modlist_tail  = NULL;

/*!
 Pointer to the functional unit structure for the functional unit that is currently being parsed.
*/
/*@null@*/func_unit* curr_funit   = NULL;

/*!
 Pointer to the global function unit that is available in SystemVerilog.
*/
/*@null@*/func_unit* global_funit = NULL;

/*!
 This static value contains the current expression ID number to use for the next expression found, it
 is incremented by one when an expression is found.  This allows us to have a unique expression ID
 for each expression (since expressions have no intrinsic names).
*/
int       curr_expr_id  = 1;

/*!
 Specifies current connection ID to use for connecting statements.  This value should be passed
 to the statement_connect function and incremented immediately after.
*/
int       stmt_conn_id    = 1;

/*!
 Specifies current connection ID to use for connecting generate items.
*/
int       gi_conn_id      = 1;

/*!
 Pointer to current implicitly connected generate item block.
*/
/*@null@*/gen_item* curr_gi_block   = NULL;

/*!
 Pointer to the most recently created generate item.
*/
/*@null@*/gen_item* last_gi         = NULL;

/*!
 Specifies the current timescale unit shift value.
*/
int current_timescale_unit      = 0;

/*!
 Specifies the global timescale precision shift value.
*/
int global_timescale_precision  = 0;


/*!
 Deallocates all memory associated with the database.
*/
void db_close() { PROFILE(DB_CLOSE);
  
  int i;  /* Loop iterator */

  if( inst_head != NULL ) {

    /* Remove memory allocated for inst_head */
    inst_link_delete_list( inst_head );
    inst_head = NULL;
    inst_tail = NULL;

    /* Remove memory allocated for all functional units */
    funit_link_delete_list( &funit_head, &funit_tail, TRUE );
    global_funit = NULL;

    /* Deallocate preprocessor define tree */
    tree_dealloc( def_table );
    def_table = NULL;

    /* Deallocate the binding list */
    bind_dealloc();
    
    /* Deallocate the information section memory */
    info_dealloc();

    /* Free memory associated with current instance scope */
    for( i=0; i<curr_inst_scope_size; i++ ) {
      free_safe( curr_inst_scope[i] );
    }
    free_safe( curr_inst_scope );

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the top module specified in the -t option is the top-level module
         of the simulation; otherwise, returns FALSE.

 Iterates through the signal list of the top-level module.  Returns TRUE if no signals with
 type INPUT, OUTPUT or INOUT were found; otherwise, returns FALSE.  Called by the parse_design()
 function.
*/
bool db_check_for_top_module() { PROFILE(DB_CHECK_FOR_TOP_MODULE);

  bool       retval = FALSE;  /* Return value for this function */
  inst_link* instl;           /* Pointer to current instance link being checked */

  instl = inst_head;
  while( (instl != NULL) && (strcmp( instl->inst->funit->name, top_module ) != 0) ) {
    instl = instl->next;
  }

  /*
   If we found the top module specified by the -t option, iterate through the
   functional unit's signal list.
  */
  if( instl != NULL ) {

    retval = funit_is_top_module( instl->inst->funit );

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param file         Name of database file to output contents to.
 \param parse_mode   Specifies if we are outputting parse data or score data.
 \param report_save  Specifies if we are attempting to "save" a CDD file modified in the report command

 \return Returns TRUE if database write was successful; otherwise, returns FALSE.

 Opens specified database for writing.  If database open successful,
 iterates through functional unit, expression and signal lists, displaying each
 to the database file.  If database write successful, returns TRUE; otherwise,
 returns FALSE to the calling function.
*/
bool db_write( char* file, bool parse_mode, bool report_save ) { PROFILE(DB_WRITE);

  bool       retval = TRUE;  /* Return value for this function */
  FILE*      db_handle;      /* Pointer to database file being written */
  inst_link* instl;          /* Pointer to current instance link */

  if( (db_handle = fopen( file, "w" )) != NULL ) {

    /* Reset expression IDs */
    curr_expr_id = 1;

    /* Iterate through instance tree */
    assert( inst_head != NULL );
    info_db_write( db_handle );

    instl = inst_head;
    while( instl != NULL ) {
      instance_db_write( instl->inst, db_handle, instl->inst->name, parse_mode, report_save );
      instl = instl->next;
    }
    assert( fclose( db_handle ) == 0 );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for writing", obf_file( file ) );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param file       Name of database file to read contents from.
 \param read_mode  Specifies what to do with read data.  Values are
                   - 0 = Instance, no merge, merge command
                   - 1 = Instance, no merge, report command
                   - 2 = Instance, merge, merge command
                   - 3 = Module, merge, report command

 \return Returns TRUE if database read was successful; otherwise, returns FALSE.

 Opens specified database file for reading.  Reads in each line from the
 file examining its contents and creating the appropriate type to store
 the specified information and stores it into the appropriate internal
 list.  If there are any problems opening the file for reading or parsing
 errors, returns FALSE; otherwise, returns TRUE.
*/
bool db_read( char* file, int read_mode ) { PROFILE(DB_READ);

  bool         retval = TRUE;        /* Return value for this function */
  FILE*        db_handle;            /* Pointer to database file being read */
  int          type;                 /* Specifies object type */
  func_unit    tmpfunit;             /* Temporary functional unit pointer */
  char*        curr_line;            /* Pointer to current line being read from db */
  char*        rest_line;            /* Pointer to rest of the current line */
  int          chars_read;           /* Number of characters currently read on line */
  char         parent_scope[4096];   /* Scope of parent functional unit to the current instance */
  char         back[4096];           /* Current functional unit instance name */
  char         funit_scope[4096];    /* Current scope of functional unit instance */
  char         funit_name[256];      /* Current name of functional unit instance */
  char         funit_file[4096];     /* Current filename of functional unit instance */
  funit_link*  foundfunit;           /* Found functional unit link */
  funit_inst*  foundinst;            /* Found functional unit instance */
  bool         merge_mode = FALSE;   /* If TRUE, we should currently be merging data */
  func_unit*   parent_mod;           /* Pointer to parent module of this functional unit */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_read, file: %s, mode: %d", obf_file( file ), read_mode );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Setup temporary module for storage */
  tmpfunit.name     = funit_name;
  tmpfunit.filename = funit_file;

  curr_funit  = NULL;

  if( (db_handle = fopen( file, "r" )) != NULL ) {

    while( util_readline( db_handle, &curr_line ) && retval ) {

      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {

        rest_line = curr_line + chars_read;

        if( type == DB_TYPE_INFO ) {
          
          /* Parse rest of line for general info */
          retval = info_db_read( &rest_line );

          /* If we are in report mode and this CDD file has not been written bow out now */
          if( (info_suppl.part.scored == 0) && 
              ((read_mode == READ_MODE_REPORT_NO_MERGE) ||
               (read_mode == READ_MODE_REPORT_MOD_MERGE)) ) {
            print_output( "Attempting to generate report on non-scored design.  Not supported.", FATAL, __FILE__, __LINE__ );
            retval = FALSE;
          }
          
        } else if( type == DB_TYPE_SCORE_ARGS ) {
          
          assert( !merge_mode );
          
          /* Parse rest of line for argument info */
          retval = args_db_read( &rest_line );
          
        } else if( type == DB_TYPE_SIGNAL ) {

          assert( !merge_mode );

          /* Parse rest of line for signal info */
          retval = vsignal_db_read( &rest_line, curr_funit );
	    
        } else if( type == DB_TYPE_EXPRESSION ) {

          assert( !merge_mode );

          /* Parse rest of line for expression info */
          retval = expression_db_read( &rest_line, curr_funit, (read_mode == READ_MODE_MERGE_NO_MERGE) );

        } else if( type == DB_TYPE_STATEMENT ) {

          assert( !merge_mode );

          /* Parse rest of line for statement info */
          retval = statement_db_read( &rest_line, curr_funit, read_mode );

        } else if( type == DB_TYPE_FSM ) {

          assert( !merge_mode );

          /* Parse rest of line for FSM info */
          retval = fsm_db_read( &rest_line, curr_funit );

        } else if( type == DB_TYPE_RACE ) {

          assert( !merge_mode );

          /* Parse rest of line for race condition block info */
          retval = race_db_read( &rest_line, curr_funit );

        } else if( type == DB_TYPE_FUNIT ) {

          /* Finish handling last functional unit read from CDD file */
          if( curr_funit != NULL ) {
              
            if( read_mode != READ_MODE_MERGE_INST_MERGE ) {

              inst_link* instl = inst_head;  /* Pointer to current instance link */

              /* Get the scope of the parent module */
              scope_extract_back( funit_scope, back, parent_scope );

              /* Attempt to add it to each instance tree until a suitable one is found */
              while( (instl != NULL) && !instance_read_add( &(instl->inst), parent_scope, curr_funit, back ) ) {
                instl = instl->next;
              }
              if( instl == NULL ) {
                (void)inst_link_add( instance_create( curr_funit, funit_scope, NULL ), &inst_head, &inst_tail );
              }

            }

            /* If the current functional unit is a merged unit, don't add it to the funit list again */
            if( !merge_mode ) {
              funit_link_add( curr_funit, &funit_head, &funit_tail );
            }

          }

          /* Reset merge mode */
          merge_mode = FALSE;

          /* Now finish reading functional unit line */
          if( (retval = funit_db_read( &tmpfunit, funit_scope, &rest_line )) == TRUE ) {
            if( (read_mode == READ_MODE_MERGE_INST_MERGE) && ((foundinst = inst_link_find_by_scope( funit_scope, inst_head )) != NULL) ) {
              merge_mode = TRUE;
              curr_funit = foundinst->funit;
              funit_db_merge( foundinst->funit, db_handle, TRUE );
            } else if( (read_mode == READ_MODE_REPORT_MOD_MERGE) && ((foundfunit = funit_link_find( &tmpfunit, funit_head )) != NULL) ) {
              merge_mode = TRUE;
              curr_funit = foundfunit->funit;
              funit_db_merge( foundfunit->funit, db_handle, FALSE );
            } else {
              curr_funit             = funit_create();
              curr_funit->name       = strdup_safe( funit_name );
              curr_funit->type       = tmpfunit.type;
              curr_funit->filename   = strdup_safe( funit_file );
              curr_funit->start_line = tmpfunit.start_line;
              curr_funit->end_line   = tmpfunit.end_line;
              curr_funit->timescale  = tmpfunit.timescale;
              if( tmpfunit.type != FUNIT_MODULE ) {
                curr_funit->parent = scope_get_parent_funit( funit_scope );
                parent_mod         = scope_get_parent_module( funit_scope );
                funit_link_add( curr_funit, &(parent_mod->tf_head), &(parent_mod->tf_tail) );
              }
            }

            /* Set global functional unit, if it has been found */
            if( (curr_funit != NULL) && (strncmp( curr_funit->name, "$root", 5 ) == 0) ) {
              global_funit = curr_funit;
            }

          }

        } else {

          snprintf( user_msg, USER_MSG_LENGTH, "Unexpected type %d when parsing database file %s", type, obf_file( file ) );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          retval = FALSE;

        }

      } else {

        snprintf( user_msg, USER_MSG_LENGTH, "Unexpected line in database file %s", obf_file( file ) );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;

      }

      free_safe( curr_line );

    }

    fclose( db_handle );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for reading", obf_file( file ) );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  /* If the last functional unit was being read, add it now */
  if( curr_funit != NULL ) {

    if( read_mode != READ_MODE_MERGE_INST_MERGE ) {

      inst_link* instl = inst_head;  /* Pointer to current instance link */

      /* Get the scope of the parent module */
      scope_extract_back( funit_scope, back, parent_scope );

      /* Attempt to add it to each instance tree until a suitable one is found */
      while( (instl != NULL) && !instance_read_add( &(instl->inst), parent_scope, curr_funit, back ) ) {
        instl = instl->next;
      }
      if( instl == NULL ) {
        inst_link_add( instance_create( curr_funit, funit_scope, NULL ), &inst_head, &inst_tail );
      }

    }

    /* If the current functional unit was being merged, don't add it to the functional unit list again */
    if( !merge_mode ) {
      funit_link_add( curr_funit, &funit_head, &funit_tail );
    }

    curr_funit = NULL;

  }

#ifdef DEBUG_MODE
  /* Display the instance trees, if we are debugging */
  if( debug_mode && retval ) {
    inst_link_display( inst_head );
  }
#endif

  PROFILE_END;

  return( retval );

}

/*!
 \param value  Delay value to scale.
 \param funit  Pointer to current functional unit of expression to scale.

 \return Returns scaled version of input value.

 Takes in specified delay value and scales it to the correct timescale for the given
 module.
*/
uint64 db_scale_to_precision( uint64 value, func_unit* funit ) { PROFILE(DB_SCALE_TO_PRECISION);

  int units = funit->ts_unit;

  assert( units >= global_timescale_precision );

  while( units > global_timescale_precision ) {
    units--;
    value *= (uint64)10;
  }

  PROFILE_END;

  return( value );

}

/*!
 \return Returns a scope name for an unnamed scope.  Only called for parsing purposes.
*/
char* db_create_unnamed_scope() { PROFILE(DB_CREATE_UNNAMED_SCOPE);

  static int unique_id = 0;

  /* Allocate memory for the unnamed scope name */
  char* name = (char*)malloc_safe( 30 );

  /* Create unnamed scope name */
  snprintf( name, 30, "$u%d", unique_id );
  unique_id++;

  PROFILE_END;

  return( name );

}

/*!
 \param scope name to check

 \return Returns TRUE if the given scope is an unnamed scope name; otherwise, returns FALSE.
*/
bool db_is_unnamed_scope( char* scope ) { PROFILE(DB_IS_UNNAMED_SCOPE);

  bool is_unnamed = (scope != NULL) && (scope[0] == '$') && (scope[1] == 'u');

  PROFILE_END;

  return( is_unnamed );

}

#ifndef VPI_ONLY
/*!
 \param unit       Timescale unit offset value
 \param precision  Timescale precision offset value

 Sets the global timescale unit and precision variables.
*/
void db_set_timescale( int unit, int precision ) { PROFILE(DB_SET_TIMESCALE);

  current_timescale_unit = unit;

  /* Set the global precision value to the lowest precision value specified */
  if( precision < global_timescale_precision ) {
    global_timescale_precision = precision;
  }

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
 \param scope  Name of functional unit instance being added.
 \param name   Name of functional unit being instantiated.
 \param type   Type of functional unit being instantiated.
 \param range  Optional range (used for arrays of instances).

 \return Returns a pointer to the created functional unit if the instance was added to the hierarchy;
         otherwise, returns NULL.
 
 Creates a new functional unit node with the instantiation name, search for matching functional unit.  If
 functional unit hasn't been created previously, create it now without a filename associated (NULL).
 Add functional unit node to tree if there are no problems in doing so.
*/
func_unit* db_add_instance( char* scope, char* name, int type, vector_width* range ) { PROFILE(DB_ADD_INSTANCE);

  func_unit*  funit = NULL;      /* Pointer to functional unit */
  funit_link* found_funit_link;  /* Pointer to found funit_link in functional unit list */
  bool        score;             /* Specifies if this module should be scored */

  /* There should always be a parent so internal error if it does not exist. */
  assert( curr_funit != NULL );

  /* If this functional unit name is in our list of no_score functional units, skip adding the instance */
  score = str_link_find( name, no_score_head ) == NULL;

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_instance, instance: %s, %s: %s (curr_funit: %s)",
            obf_inst( scope ), get_funit_type( type ), obf_funit( name ), obf_funit( curr_funit->name ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Create new functional unit node */
  funit       = funit_create();
  funit->name = strdup_safe( name );
  funit->type = score ? type : FUNIT_NO_SCORE;

  /* If a range has been specified, calculate its width and lsb now */
  if( (range != NULL) && score ) {
    if( (range->left != NULL) && (range->left->exp != NULL) ) {
      mod_parm_add( NULL, NULL, NULL, FALSE, range->left->exp, PARAM_TYPE_INST_MSB, curr_funit, scope );
    }
    if( (range->right != NULL) && (range->right->exp != NULL) ) {
      mod_parm_add( NULL, NULL, NULL, FALSE, range->right->exp, PARAM_TYPE_INST_LSB, curr_funit, scope );
    }
  }

  if( ((found_funit_link = funit_link_find( funit, funit_head )) != NULL) && (generate_top_mode == 0) ) {

    if( type != FUNIT_MODULE ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Multiple identical task/function/named-begin-end names (%s) found in module %s, file %s\n",
                scope, obf_funit( curr_funit->name ), obf_file( curr_funit->filename ) );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
    }

    /* If we are currently within a generate block, create a generate item for this instance to resolve it later */
    if( generate_top_mode > 0 ) {
      last_gi = gen_item_create_inst( instance_create( found_funit_link->funit, scope, range ) );
      if( curr_gi_block != NULL ) {
        db_gen_item_connect( curr_gi_block, last_gi );
      } else {
        curr_gi_block = last_gi;
      }
    } else {
      if( (last_gi == NULL) || (last_gi->suppl.part.type != GI_TYPE_INST) || !instance_parse_add( &last_gi->elem.inst, curr_funit, found_funit_link->funit, scope, range, FALSE, TRUE ) ) {
        inst_link* instl = inst_head;
        while( (instl != NULL) && !instance_parse_add( &instl->inst, curr_funit, found_funit_link->funit, scope, range, FALSE, FALSE ) ) {
          instl = instl->next;
        }
        if( instl == NULL ) {
          inst_link_add( instance_create( found_funit_link->funit, scope, range ), &inst_head, &inst_tail );
        }
      }
    }

    funit_dealloc( funit );

  } else {

    /* Add new functional unit to functional unit list. */
    funit_link_add( funit, &funit_head, &funit_tail );

    /* If we are currently within a generate block, create a generate item for this instance to resolve it later */
    if( generate_top_mode > 0 ) {
      last_gi = gen_item_create_inst( instance_create( funit, scope, range ) );
      if( curr_gi_block != NULL ) {
        db_gen_item_connect( curr_gi_block, last_gi );
      } else {
        curr_gi_block = last_gi;
      }
    } else {
      if( (last_gi == NULL) || (last_gi->suppl.part.type != GI_TYPE_INST) || !instance_parse_add( &last_gi->elem.inst, curr_funit, funit, scope, range, FALSE, TRUE ) ) {
        inst_link* instl = inst_head;
        while( (instl != NULL) && !instance_parse_add( &instl->inst, curr_funit, funit, scope, range, FALSE, FALSE ) ) {
          instl = instl->next;
        }
        if( instl == NULL ) {
          inst_link_add( instance_create( funit, scope, range ), &inst_head, &inst_tail );
        }
      }
    }

    if( (type == FUNIT_MODULE) && score && (str_link_find( name, modlist_head ) == NULL) ) {
      str_link_add( strdup_safe( name ), &modlist_head, &modlist_tail );
    }
      
  }

  PROFILE_END;

  return( score ? funit : NULL );

}

/*!
 \param name        Name of module being added to tree.
 \param file        Filename that module is a part of.
 \param start_line  Starting line number of this module in the file.

 Creates a new module element with the contents specified by the parameters given
 and inserts this module into the module list.  This function can only be called when we
 are actually parsing a module which implies that we must have the name of the module
 at the head of the modlist linked-list structure.
*/
void db_add_module( char* name, char* file, int start_line ) { PROFILE(DB_ADD_MODULE);

  func_unit   mod;   /* Temporary module for comparison */
  funit_link* modl;  /* Pointer to found tree node */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_module, module: %s, file: %s, start_line: %d",
            obf_funit( name ), obf_file( file ), start_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Set current module to this module */
  mod.name = name;
  mod.type = FUNIT_MODULE;

  modl = funit_link_find( &mod, funit_head );

  assert( modl != NULL );

  curr_funit             = modl->funit;
  curr_funit->filename   = strdup_safe( file );
  curr_funit->start_line = start_line;
  curr_funit->ts_unit    = current_timescale_unit;

  PROFILE_END;
  
}

/*!
 \param end_line  Ending line number of specified module in file.

 Updates the modlist for parsing purposes.
*/
void db_end_module( int end_line ) { PROFILE(DB_END_MODULE);

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_end_module, end_line: %d", end_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  curr_funit->end_line = end_line;

  str_link_remove( curr_funit->name, &modlist_head, &modlist_tail );

  /* Return the current functional unit to the global functional unit, if it exists */
  curr_funit = global_funit;

  PROFILE_END;

}

/*!
 \param type        Specifies type of functional unit being added (function, task or named_block)
 \param name        Name of functional unit
 \param file        File containing the specified functional unit
 \param start_line  Starting line number of functional unit

 \return Returns TRUE if the new functional unit was added to the design; otherwise, returns FALSE
         to indicate that this block should be ignored.
*/
bool db_add_function_task_namedblock( int type, char* name, char* file, int start_line ) { PROFILE(DB_ADD_FUNCTION_TASK_NAMEDBLOCK);

  func_unit* tf;         /* Pointer to created functional unit */
  func_unit* parent;     /* Pointer to parent module for the newly created functional unit */
  char*      full_name;  /* Full name of function/task/namedblock which includes the parent module name */

  assert( (type == FUNIT_FUNCTION)  || (type == FUNIT_TASK) || (type == FUNIT_NAMED_BLOCK) ||
          (type == FUNIT_AFUNCTION) || (type == FUNIT_ATASK) );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_function_task_namedblock, %s: %s, file: %s, start_line: %d",
            get_funit_type( type ), obf_funit( name ), obf_file( file ), start_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Generate full name to use for the function/task */
  full_name = funit_gen_task_function_namedblock_name( name, curr_funit );

  /* Add this as an instance so we can get scope */
  if( (tf = db_add_instance( name, full_name, type, NULL )) != NULL ) {

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
    if( (curr_funit->type == FUNIT_AFUNCTION) ||
        (curr_funit->type == FUNIT_ATASK) ||
        (curr_funit->type == FUNIT_ANAMED_BLOCK) ) {
      assert( tf->type == FUNIT_NAMED_BLOCK );
      tf->type = FUNIT_ANAMED_BLOCK;
    }

    /* Set current functional unit to this functional unit */
    curr_funit             = tf;
    curr_funit->filename   = strdup_safe( file );
    curr_funit->start_line = start_line;
    curr_funit->ts_unit    = current_timescale_unit;
    
  }

  free_safe( full_name );

  PROFILE_END;

  return( tf != NULL );

}

/*!
 \param end_line  Line number of end of this task/function
*/
void db_end_function_task_namedblock( int end_line ) { PROFILE(DB_END_FUNCTION_TASK_NAMEDBLOCK);

  stmt_iter si;  /* Statement iterator for finding the first statement of the functional unit */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_end_function_task_namedblock, end_line: %d", end_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Store last line information */
  curr_funit->end_line = end_line;

  /* Set the first statement pointer */
  if( curr_funit->stmt_head != NULL ) {

    assert( curr_funit->stmt_head->stmt != NULL );

    /* Set functional unit's first_stmt pointer to its head statement */
    stmt_iter_reset( &si, curr_funit->stmt_tail );
    stmt_iter_find_head( &si, FALSE );

    if( si.curr->stmt != NULL ) {
      curr_funit->first_stmt = si.curr->stmt;
    }

  }

  /* Set the current functional unit to the parent module */
  curr_funit = curr_funit->parent;

  PROFILE_END;

}

/*!
 \param is_signed  Specified if the declared parameter needs to be handled as a signed value
 \param msb        Static expression containing MSB of this declared parameter
 \param lsb        Static expression containing LSB of this declared parameter
 \param name       Name of declared parameter to add.
 \param expr       Expression containing value of this parameter.
 \param local      If TRUE, specifies that this parameter is a local parameter.

 Searches current module to verify that specified parameter name has not been previously
 used in the module.  If the parameter name has not been found, it is created added to
 the current module's parameter list.
*/
void db_add_declared_param( bool is_signed, static_expr* msb, static_expr* lsb, char* name, expression* expr, bool local ) { PROFILE(DB_ADD_DECLARED_PARAM);

  mod_parm* mparm;  /* Pointer to added module parameter */

  assert( name != NULL );

  /* If a parameter value type is not supported, don't create this parameter */
  if( expr != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_declared_param, param: %s, expr: %d, local: %d", obf_sig( name ), expr->id, local );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    if( mod_parm_find( name, curr_funit->param_head ) == NULL ) {

      /* Add parameter to module parameter list */
      mparm = mod_parm_add( name, msb, lsb, is_signed, expr, (local ? PARAM_TYPE_DECLARED_LOCAL : PARAM_TYPE_DECLARED), curr_funit, NULL );

    }

  }

  PROFILE_END;

}

/*!
 \param inst_name   Name of instance being overridden.
 \param expr        Expression containing value of override parameter.
 \param param_name  Name of parameter being overridden (for parameter_value_byname syntax)

 Creates override parameter and stores this in the current module as well
 as all associated instances.
*/
void db_add_override_param( char* inst_name, expression* expr, char* param_name ) { PROFILE(DB_ADD_OVERRIDE_PARAM);

  mod_parm* mparm;  /* Pointer to module parameter added to current module */

#ifdef DEBUG_MODE
  if( param_name != NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s, param_name: %s",
              obf_inst( inst_name ), obf_sig( param_name ) );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s", obf_inst( inst_name ) );
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Add override parameter to module parameter list */
  mparm = mod_parm_add( param_name, NULL, NULL, FALSE, expr, PARAM_TYPE_OVERRIDE, curr_funit, inst_name );

  PROFILE_END;

}

/*!
 \param sig        Pointer to signal to attach parameter to.
 \param parm_exp   Expression containing value of vector parameter.
 \param type       Type of signal vector parameter to create (LSB or MSB).
 \param dimension  Specifies the signal dimension to solve for.

 Creates a vector parameter for the specified signal or expression with the specified
 parameter expression.  This function is called by the parser.
*/
void db_add_vector_param( vsignal* sig, expression* parm_exp, int type, int dimension ) { PROFILE(DB_ADD_VECTOR_PARAM);

  mod_parm* mparm;  /* Holds newly created module parameter */

  assert( sig != NULL );
  assert( (type == PARAM_TYPE_SIG_LSB) || (type == PARAM_TYPE_SIG_MSB) );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_vector_param, signal: %s, type: %d", obf_sig( sig->name ), type );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
 \param name  Name of parameter value to override.
 \param expr  Expression value of parameter override.

 Adds specified parameter to the defparam list.
*/
void db_add_defparam( char* name, expression* expr ) { PROFILE(DB_ADD_DEFPARAM);

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_defparam, defparam: %s", obf_sig( name ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  snprintf( user_msg, USER_MSG_LENGTH, "defparam construct is not supported, line: %d.  Use -P option to score instead", expr->line );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );

  expression_dealloc( expr, FALSE );

  PROFILE_END;

}

/*!
 \param name       Name of signal being added.
 \param type       Type of signal being added.
 \param prange     Specifies packed signal range information.
 \param urange     Specifies unpacked signal range information.
 \param is_signed  Specifies that this signal is signed (TRUE) or not (FALSE)
 \param mba        Set to TRUE if specified signal must be assigned by simulated results.
 \param line       Line number where signal was declared.
 \param col        Starting column where signal was declared.
 \param gi         Pointer to created generate item
 \param handled    Specifies if this signal is handled by Covered or not.

 Creates a new signal with the specified parameter information and adds this
 to the signal list if it does not already exist.  If width == 0, the sig_msb
 and sig_lsb values are interrogated.  If sig_msb and/or is non-NULL, its value is
 add to the current module's parameter list and all associated instances are
 updated to contain new value.
*/
void db_add_signal( char* name, int type, sig_range* prange, sig_range* urange, bool is_signed, bool mba, int line, int col, bool handled ) { PROFILE(DB_ADD_SIGNAL);

  vsignal   tmpsig;      /* Temporary signal for signal searching */
  vsignal*  sig = NULL;  /* Container for newly created signal */
  sig_link* sigl;        /* Pointer to found signal link */
  int       i;           /* Loop iterator */
  int       j = 0;       /* Loop iterator */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_signal, signal: %s, type: %d, line: %d, col: %d", obf_sig( name ), type, line, col );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  tmpsig.name = name;

  /* Add signal to current module's signal list if it does not already exist */
  if( (sigl = sig_link_find( &tmpsig, curr_funit->sig_head )) == NULL ) {

    /* Create the signal */
    if( type == SSUPPL_TYPE_GENVAR ) {
      /* For genvars, set the size to 32, automatically */
      sig = vsignal_create( name, type, 32, line, col );
    } else {
      /* For normal signals just make the width a value of 1 for now -- it will be resized during funit_resize_elements */
      sig = vsignal_create( name, type, 1, line, col );
    }

  /* If the signal has currently existed, check to see if the signal is unsized, and, if so, size it now */
  } else if( sigl->sig->suppl.part.implicit_size ) {

    sig = sigl->sig;
    sig->suppl.part.implicit_size = 0;

  }

  /* Check all of the dimensions within range and create vector parameters, if necessary */
  if( sig != NULL ) {

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

  }

  /* Only do the following if the signal was not previously found */
  if( sigl == NULL ) {

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
      sig_link_add( sig, &(curr_funit->sig_head), &(curr_funit->sig_tail) );
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
    sig->suppl.part.implicit_size = (((type == SSUPPL_TYPE_INPUT) || (type == SSUPPL_TYPE_OUTPUT) || (type == SSUPPL_TYPE_INOUT)) &&
                                     (prange != NULL) && prange->dim[0].implicit &&
                                     (prange->dim[0].left->exp == NULL) && (prange->dim[0].left->num == 0) &&
                                     (prange->dim[0].right->exp == NULL) && (prange->dim[0].right->num == 0)) ? 1 : 0;

  }

  PROFILE_END;
  
}

/*!
 \param enum_sig  Pointer to signal created for the given enumerated value
 \param value     Value to later assign to the enum_sig (during elaboration)

 Allocates and adds an enum_item to the current module's list to be elaborated later.
*/
void db_add_enum( vsignal* enum_sig, static_expr* value ) { PROFILE(DB_ADD_ENUM);

  assert( enum_sig != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_enum, sig_name: %s", enum_sig->name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  enumerate_add_item( enum_sig, value, curr_funit );

  PROFILE_END;

}

/*!
 Called after an entire enum list has been parsed and added to the database.
*/
void db_end_enum_list() { PROFILE(DB_END_ENUM_LIST);

#ifdef DEBUG_MODE
  print_output( "In db_end_enum_list", DEBUG, __FILE__, __LINE__ );
#endif 

  enumerate_end_list( curr_funit );

  PROFILE_END;

}

/*!
 \param name_list    List of typedef names for this type
 \param is_signed    Specifies if this typedef is signed or not
 \param is_handled   Specifies if this typedef is handled or not
 \param is_sizeable  Specifies if a range can be later placed on this value
 \param prange       Dimensional packed range information for this value
 \param urange       Dimensional unpacked range information for this value

 Adds the given names and information to the list of typedefs for the current module.
*/
void db_add_typedef( char* name, bool is_signed, bool is_handled, bool is_sizeable, sig_range* prange, sig_range* urange ) { PROFILE(DB_ADD_TYPEDEF);

  typedef_item* tdi;   /* Typedef item to create */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_typedef, name: %s, is_signed: %d, is_handled: %d, is_sizeable: %d",
            name, is_signed, is_handled, is_sizeable );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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

  PROFILE_END;

}

/*!
 \param name               String name of signal to find in current module.
 \param okay_if_not_found  If set to TRUE, does not emit error message if signal is not found (returns NULL)

 \return Returns pointer to the found signal.

 Searches signal matching the specified name using normal scoping rules.  If the signal is
 found, returns a pointer to the calling function for that signal.  If the signal is not
 found, emits a user error and immediately halts execution.
*/
vsignal* db_find_signal( char* name, bool okay_if_not_found ) { PROFILE(DB_FIND_SIGNAL);

  vsignal*   found_sig;    /* Pointer to found signal (return value) */
  func_unit* found_funit;  /* Pointer to found functional unit (not used) */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_find_signal, searching for signal %s", obf_sig( name ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  if( !scope_find_signal( name, curr_funit, &found_sig, &found_funit, 0 ) && !okay_if_not_found ) {

    snprintf( user_msg, USER_MSG_LENGTH, "Unable to find variable %s in module %s", obf_sig( name ), obf_funit( curr_funit->name ) );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );

  }

  PROFILE_END;

  return( found_sig );

}

/*!
 \param gi  Pointer to the head of a generate item block to add

 Adds the specified generate item block to the list of generate blocks for
 the current functional unit.
*/
void db_add_gen_item_block( gen_item* gi ) { PROFILE(DB_ADD_GEN_ITEM_BLOCK);

  if( gi != NULL ) {

    /* Add the generate block to the list of generate blocks for this functional unit */
    gitem_link_add( gi, &(curr_funit->gitem_head), &(curr_funit->gitem_tail) );

  }

  PROFILE_END;

}

/*!
 \param root  Pointer to root generate item to start searching in
 \param gi    Pointer to created generate item to search for

 \return Returns pointer to found matching generate item (NULL if not found)

 Searches the current functional unit for the generate item that matches the
 specified generate item.  If it is found, a pointer to the stored generate
 item is returned.  If it is not found, a value of NULL is returned.  Additionally,
 the specified generate item is automatically deallocated on behalf of the caller.
 This function should only be called during the parsing stage.
*/
gen_item* db_find_gen_item( gen_item* root, gen_item* gi ) { PROFILE(DB_FIND_GEN_ITEM);

  gen_item* found;  /* Return value for this function */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_find_gen_item, type %d", gi->suppl.part.type );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Search for the specified generate item */
  found = gen_item_find( root, gi );

  /* Deallocate the user-specified generate item */
  gen_item_dealloc( gi, FALSE );

  PROFILE_END;

  return( found );

}

/*!
 \param name  Name of typedef to search for

 \return Returns pointer to found typedef item or NULL if none was found.

 Searches for the given typedef name in the current module.
*/
typedef_item* db_find_typedef( const char* name ) { PROFILE(DB_FIND_TYPEDEF);

  func_unit*    parent;      /* Pointer to parent module */
  typedef_item* tdi = NULL;  /* Pointer to current typedef item */

  assert( name != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_find_typedef, searching for name: %s", name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
  print_output( "In db_get_curr_gen_block", DEBUG, __FILE__, __LINE__ );
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

  int       sig_cnt = 0;  /* Holds number of signals in the current functional unit */
  sig_link* sigl;         /* Pointer to current signal link */

  sigl = curr_funit->sig_head;
  while( sigl != NULL ) {
    sig_cnt++;
    sigl = sigl->next;
  }

  PROFILE_END;

  return( sig_cnt );

}

/*!
 \param right     Pointer to expression on right side of expression.
 \param left      Pointer to expression on left side of expression.
 \param op        Operation to perform on expression.
 \param lhs       Specifies this expression is a left-hand-side assignment expression.
 \param line      Line number of current expression.
 \param first     Column index of first character in this expression
 \param last      Column index of last character in this expression
 \param sig_name  Name of signal that expression is attached to (if valid).

 \return Returns pointer to newly created expression.

 Creates a new expression with the specified parameter information and returns a
 pointer to the newly created expression.
*/
expression* db_create_expression( expression* right, expression* left, int op, bool lhs, int line, int first, int last, char* sig_name ) { PROFILE(DB_CREATE_EXPRESSION);

  expression* expr;        /* Temporary pointer to newly created expression */
  func_unit*  func_funit;  /* Pointer to function, if we are nested in one */
#ifdef DEBUG_MODE
  int         right_id;    /* ID of right expression */
  int         left_id;     /* ID of left expression */

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
    snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expression, right: %d, left: %d, id: %d, op: %s, lhs: %d, line: %d, first: %d, last: %d", 
              right_id, left_id, curr_expr_id, expression_string_op( op ), lhs, line, first, last );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expression, right: %d, left: %d, id: %d, op: %s, lhs: %d, line: %d, first: %d, last: %d, sig_name: %s",
              right_id, left_id, curr_expr_id, expression_string_op( op ), lhs, line, first, last, sig_name );
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
    snprintf( user_msg, USER_MSG_LENGTH, "Attempting to use a delay, task call, non-blocking assign or event controls in function %s, file %s, line %d", obf_funit( func_funit->name ), obf_file( curr_funit->filename ), line );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  /* Create expression with next expression ID */
  expr = expression_create( right, left, op, lhs, curr_expr_id, line, first, last, FALSE );
  curr_expr_id++;

  /* If current functional unit is nested in a function, set the IN_FUNC supplemental field bit */
  expr->suppl.part.in_func = (func_funit != NULL) ? 1 : 0;

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
        case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    sig_name, expr, curr_funit );  break;
        case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        sig_name, expr, curr_funit );  break;
        case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, sig_name, expr, curr_funit );  break;
        case EXP_OP_DISABLE   :  bind_add( 1,                 sig_name, expr, curr_funit );  break;
        default               :  bind_add( 0,                 sig_name, expr, curr_funit );  break;
      }
    }

  }

  PROFILE_END;
 
  return( expr );

}

/*!
 \param root      Pointer to root of expression tree to bind
 \param sig_name  Name of signal to bind to

 Recursively iterates through the entire expression tree binding all selection expressions within that tree
 to the given signal.
*/
void db_bind_expr_tree( expression* root, char* sig_name ) { PROFILE(DB_BIND_EXPR_TREE);

  assert( sig_name != NULL );

  if( root != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_bind_expr_tree, root id: %d, sig_name: %s", root->id, sig_name );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Bind the children first */
    db_bind_expr_tree( root->left,  sig_name );
    db_bind_expr_tree( root->right, sig_name );

    /* Now bind ourselves if necessary */
    if( (root->op == EXP_OP_SBIT_SEL) ||
        (root->op == EXP_OP_MBIT_SEL) ||
        (root->op == EXP_OP_MBIT_POS) ||
        (root->op == EXP_OP_MBIT_NEG) ) {
      bind_add( 0, sig_name, root, curr_funit );
    }

  }

  PROFILE_END;

}

/*!
 \param se  Pointer to static expression structure
 
 \return Returns a pointer to an expression that represents the static expression specified
*/
expression* db_create_expr_from_static( static_expr* se, int line, int first_col, int last_col ) { PROFILE(DB_CREATE_EXPR_FROM_STATIC);

  expression* expr;  /* Return value for this function */
  vector*     vec;   /* Temporary vector */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expr_from_static, se: %p, line: %d, first_col: %d, last_col: %d",
            se, line, first_col, last_col );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  if( se->exp == NULL ) {

    /* This static expression is a static value so create a static expression from its value */
    expr = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, line, first_col, last_col, NULL );

    /* Create the new vector */
    vec = vector_create( 32, VTYPE_VAL, TRUE );
    vector_from_int( vec, se->num );

    /* Assign the new vector to the expression's vector (after deallocating the expression's old vector) */
    assert( expr->value->value == NULL );
    free_safe( expr->value );
    expr->value = vec;

  } else {

    /* The static expression is unresolved, so just get its expression */
    expr = se->exp;

  }

  /* Deallocate static expression */
  static_expr_dealloc( se, FALSE );

  PROFILE_END;

  return( expr );

}

/*!
 \param root  Pointer to root expression to add to module expression list.
 \param gi    Pointer to created generate item

 Adds the specified expression to the current module's expression list.
*/
void db_add_expression( expression* root ) { PROFILE(DB_ADD_EXPRESSION);

  if( (root != NULL) && (root->suppl.part.exp_added == 0) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_expression, id: %d, op: %s, line: %d", 
              root->id, expression_string_op( root->op ), root->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
      exp_link_add( root, &(curr_funit->exp_head), &(curr_funit->exp_tail) );

    }

    /* Specify that this expression has already been added */
    root->suppl.part.exp_added = 1;

  }

  PROFILE_END;

}

/*!
 \param stmt  Pointer to statement block to parse.

 \return Returns expression tree to execute a sensitivity list for the given statement block.
*/
expression* db_create_sensitivity_list( statement* stmt ) { PROFILE(DB_CREATE_SENSITIVITY_LIST);

  str_link*   sig_head = NULL;  /* Pointer to head of signal name list containing RHS used signals */
  str_link*   sig_tail = NULL;  /* Pointer to tail of signal name list containing RHS used signals */
  str_link*   strl;             /* Pointer to current signal name link */
  expression* exps;             /* Pointer to created expression for type SIG */
  expression* expl;             /* Pointer to created expression for type LAST */
  expression* expa;             /* Pointer to created expression for type AEDGE */
  expression* expe;             /* Pointer to created expression for type EOR */
  expression* expc     = NULL;  /* Pointer to left child expression */

  /* Get the list of all RHS signals in the given statement block */
  statement_find_rhs_sigs( stmt, &sig_head, &sig_tail );

  /* Create sensitivity expression tree for the list of RHS signals */
  if( sig_head != NULL ) {

    strl = sig_head;
    while( strl != NULL ) {

      /* Create AEDGE and EOR for subsequent signals */
      exps = db_create_expression( NULL, NULL, EXP_OP_SIG,   FALSE, 0, 0, 0, strl->str );
      expl = db_create_expression( NULL, NULL, EXP_OP_LAST,  FALSE, 0, 0, 0, NULL );
      expa = db_create_expression( exps, expl, EXP_OP_AEDGE, FALSE, 0, 0, 0, NULL );

      /* If we have a child expression already, create the EOR expression to connect them */
      if( expc != NULL ) {
        expe = db_create_expression( expa, expc, EXP_OP_EOR, FALSE, 0, 0, 0, NULL );
        expc = expe;
      } else {
        expc = expa;
      }

      strl = strl->next;

    }

    /* Deallocate string list */
    str_link_delete_list( sig_head );

  }

  PROFILE_END;

  return( expc );

}


/*!
 \param stmt  Pointer to statement to check for parallelization

 \return Returns pointer to parallelized statement block
*/
statement* db_parallelize_statement( statement* stmt ) { PROFILE(DB_PARALLELIZE_STATEMENT);

  expression* exp;    /* Expression containing FORK statement */
  char*       scope;  /* Name of current parallelized statement scope */
  char        back[4096];  /* Last portion of scope */

  /* If we are a parallel statement, create a FORK statement for this statement block */
  if( (stmt != NULL) && (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_parallelize_statement, id: %d, %s, line: %d, fork_depth: %d, block_depth: %d, fork_block_depth: %d",
              stmt->exp->id, expression_string_op( stmt->exp->op ), stmt->exp->line, fork_depth, block_depth, fork_block_depth[fork_depth] );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Create FORK expression */
    exp = db_create_expression( NULL, NULL, EXP_OP_FORK, FALSE, stmt->exp->line, ((stmt->exp->col & 0xffff0000) >> 16), (stmt->exp->col & 0xffff), NULL );

    /* Create unnamed scope */
    scope = db_create_unnamed_scope();
    if( db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, scope, curr_funit->filename, stmt->exp->line ) ) {

      /* Create a thread block for this statement block */
      stmt->exp->suppl.part.stmt_head      = 1;
      stmt->exp->suppl.part.stmt_is_called = 1;
      db_add_statement( stmt, stmt );

      /* Bind the FORK expression now */
      exp->elem.funit      = curr_funit;
      exp->suppl.part.type = ETYPE_FUNIT;
      exp->name            = strdup( scope );

      /* Restore the original functional unit */
      db_end_function_task_namedblock( stmt->exp->line );

      /* Bind the FORK expression to this statement */
      //bind_add( FUNIT_NAMED_BLOCK, scope, exp, curr_funit );

    }
    free_safe( scope );

    /* Reduce fork and block depth for the new statement */
    fork_depth--;
    block_depth--;

    /* Create FORK statement and add the expression */
    stmt = db_create_statement( exp );

    /* Restore fork and block depth values for parser */
    fork_depth++;
    block_depth++;

  }

  PROFILE_END;

  return( stmt );

}

/*!
 \param exp  Pointer to associated "root" expression.

 \return Returns pointer to created statement.

 Creates an statement structure and adds created statement to current
 module's statement list.
*/
statement* db_create_statement( expression* exp ) { PROFILE(DB_CREATE_STATEMENT);

  statement* stmt;  /* Pointer to newly created statement */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_create_statement, id: %d, line: %d", exp->id, exp->line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Create the given statement */
  stmt = statement_create( exp );

  /* If we are a parallel statement, create a FORK statement for this statement block */
  stmt = db_parallelize_statement( stmt );

  PROFILE_END;

  return( stmt );

}

/*!
 \param stmt   Pointer to statement add to current module's statement list.
 \param start  Pointer to starting statement of statement tree.

 Adds the specified statement tree to the tail of the current module's statement list.
 The start statement is specified to avoid infinite looping.
*/
void db_add_statement( statement* stmt, statement* start ) { PROFILE(DB_ADD_STATEMENT);
 
  if( (stmt != NULL) && (stmt->exp->suppl.part.stmt_added == 0) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_statement, id: %d, start id: %d", stmt->exp->id, start->exp->id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

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
      if( (ESUPPL_IS_STMT_STOP_FALSE( stmt->exp->suppl ) == 0) && (stmt->next_false != start) ) {
        db_add_statement( stmt->next_false, start );
      }

      if( (ESUPPL_IS_STMT_STOP_TRUE( stmt->exp->suppl ) == 0) && (stmt->next_true != stmt->next_false) && (stmt->next_true != start) ) {
        db_add_statement( stmt->next_true, start );
      }

      /* Set ADDED bit of this statement */
      stmt->exp->suppl.part.stmt_added = 1;

      /* Finally, add the statement to the functional unit statement list */
      stmt_link_add_tail( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

    }

  }

  PROFILE_END;

}
#endif

/*!
 \param stmt  Pointer to statement to remove from memory.

 Removes specified statement expression from the current functional unit.  Called by statement_dealloc_recursive in
 statement.c in its deallocation algorithm.
*/
void db_remove_statement_from_current_funit( statement* stmt ) { PROFILE(DB_REMOVE_STATEMENT_FROM_CURRENT_FUNIT);

  inst_link* instl;  /* Pointer to current functional unit instance */

  if( (stmt != NULL) && (stmt->exp != NULL) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement_from_current_funit %s, stmt id: %d, %s, line: %d",
              obf_funit( curr_funit->name ), stmt->exp->id, expression_string_op( stmt->exp->op ), stmt->exp->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /*
     Get a list of all parameters within the given statement expression tree and remove them from
     an instance and module parameters.
    */
    instl = inst_head;
    while( instl != NULL ) {
      instance_remove_parms_with_expr( instl->inst, stmt );
      instl = instl->next;
    }

    /* Remove expression from current module expression list and delete expressions */
    exp_link_remove( stmt->exp, &(curr_funit->exp_head), &(curr_funit->exp_tail), TRUE );

    /* Remove this statement link from the current module's stmt_link list */
    stmt_link_unlink( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

  }

  PROFILE_END;

}

#ifndef VPI_ONLY
/*!
 \param stmt  Pointer to statement to remove from memory.

 Removes specified statement expression and its tree from current module expression list and deallocates
 both the expression and statement from heap memory.  Called when a statement structure is
 found to contain a statement that is not supported by Covered.
*/
void db_remove_statement( statement* stmt ) { PROFILE(DB_REMOVE_STATEMENT);

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement, stmt id: %d, line: %d", 
              stmt->exp->id, stmt->exp->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Call the recursive statement deallocation function */
    statement_dealloc_recursive( stmt );

  }

  PROFILE_END;

}

/*!
 \param stmt       Pointer to statement to connect true path to.
 \param next_true  Pointer to statement to run if statement evaluates to TRUE.

 Connects the specified statement's true statement.
*/
void db_connect_statement_true( statement* stmt, statement* next_true ) { PROFILE(DB_CONNECT_STATEMENT_TRUE);

#ifdef DEBUG_MODE
  int next_id;  /* Statement ID of next TRUE statement */
#endif

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    if( next_true == NULL ) {
      next_id = 0;
    } else {
      next_id = next_true->exp->id;
    }

    snprintf( user_msg, USER_MSG_LENGTH, "In db_connect_statement_true, id: %d, next: %d", stmt->exp->id, next_id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    stmt->next_true = next_true;

  }

  PROFILE_END;

}

/*!
 \param stmt        Pointer to statement to connect false path to.
 \param next_false  Pointer to statement to run if statement evaluates to FALSE.

 Connects the specified statement's false statement.
*/
void db_connect_statement_false( statement* stmt, statement* next_false ) { PROFILE(DB_CONNECT_STATEMENT_FALSE);

#ifdef DEBUG_MODE
  int next_id;  /* Statement ID of next FALSE statement */
#endif

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    if( next_false == NULL ) {
      next_id = 0;
    } else {
      next_id = next_false->exp->id;
    }

    snprintf( user_msg, USER_MSG_LENGTH, "In db_connect_statement_false, id: %d, next: %d", stmt->exp->id, next_id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    stmt->next_false = next_false;

  }

  PROFILE_END;

}

/*!
 \param gi1  Pointer to generate item holding next_true
 \param gi2  Pointer to generate item to connect

 Connects gi2 to gi1's next_true pointer.
*/
void db_gen_item_connect_true( gen_item* gi1, gen_item* gi2 ) { PROFILE(DB_GEN_ITEM_CONNECT_TRUE);

  assert( gi1 != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_gen_item_connect_true, gi1: %p, gi2: %p", gi1, gi2 );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  gi1->next_true = gi2;  

  PROFILE_END;

}

/*!
 \param gi1  Pointer to generate item holding next_false
 \param gi2  Pointer to generate item to connect

 Connects gi2 to gi1's next_false pointer.
*/
void db_gen_item_connect_false( gen_item* gi1, gen_item* gi2 ) { PROFILE(DB_GEN_ITEM_CONNECT_FALSE);

  assert( gi1 != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_gen_item_connect_false, gi1: %p, gi2: %p", gi1, gi2 );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  gi1->next_false = gi2;

  PROFILE_END;

}

/*!
 \param gi1  Pointer to generate item block to connect to gi2
 \param gi2  Pointer to generate item that will be connected to gi1

 \return Returns TRUE if a generate item connection was properly established; otherwise,
         returns FALSE.
*/
bool db_gen_item_connect( gen_item* gi1, gen_item* gi2 ) { PROFILE(DB_GEN_ITEM_CONNECT);

  bool retval;  /* Return value for this function */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_gen_item_connect, gi1: %p, gi2: %p, conn_id: %d", gi1, gi2, gi_conn_id );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Connect generate items */
  retval = gen_item_connect( gi1, gi2, gi_conn_id );

  /* Increment gi_conn_id for next connection */
  gi_conn_id++;

  PROFILE_END;

  return( retval );

}

/*!
 \param curr_stmt  Pointer to current statement to attach.
 \param next_stmt  Pointer to next statement to attach to.

 \return Returns TRUE if statement was properly connected to the given statement list; otherwise,
         returns FALSE.

 Calls the statement_connect function located in statement.c with the specified parameters.  If
 the statement connection was not achieved, displays warning to user and returns FALSE.  The calling
 function should throw this statement away.
*/
bool db_statement_connect( statement* curr_stmt, statement* next_stmt ) { PROFILE(DB_STATEMENT_CONNECT);

  bool retval;  /* Return value for this function */

#ifdef DEBUG_MODE
  int curr_id;  /* Current statement ID */
  int next_id;  /* Next statement ID */

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

  snprintf( user_msg, USER_MSG_LENGTH, "In db_statement_connect, curr_stmt: %d, next_stmt: %d", curr_id, next_id );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /*
   Connect statement, if it was not successful, add it to the functional unit's statement list immediately
   as it will not be later on.
  */
  if( !(retval = statement_connect( curr_stmt, next_stmt, stmt_conn_id )) ) {

    snprintf( user_msg, USER_MSG_LENGTH, "Unreachable statement found starting at line %d in file %s.  Ignoring...",
              next_stmt->exp->line, obf_file( curr_funit->filename ) );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  /* Increment stmt_conn_id for next statement connection */
  stmt_conn_id++;

  PROFILE_END;

  return( retval );

}

/*!
 \param name  Attribute parameter identifier.
 \param expr  Pointer to constant expression that is assigned to the identifier.

 \return Returns a pointer to the newly created attribute parameter.

 Calls the attribute_create() function and returns the pointer returned by this function.
*/
attr_param* db_create_attr_param( char* name, expression* expr ) { PROFILE(DB_CREATE_ATTR_PARAM);

  attr_param* attr;  /* Pointer to newly allocated/initialized attribute parameter */

#ifdef DEBUG_MODE
  if( expr != NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_create_attr_param, name: %s, expr: %d", name, expr->id );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_create_attr_param, name: %s", name );
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  attr = attribute_create( name, expr );

  PROFILE_END;

  return( attr );

}

/*!
 \param ap  Pointer to attribute parameter list to parse.

 Calls the attribute_parse() function and deallocates this list.
*/
void db_parse_attribute( attr_param* ap ) { PROFILE(DB_PARSE_ATTRIBUTE);

#ifdef DEBUG_MODE
  print_output( "In db_parse_attribute", DEBUG, __FILE__, __LINE__ );
#endif

  /* First, parse the entire attribute */
  attribute_parse( ap, curr_funit );

  /* Then deallocate the structure */
  attribute_dealloc( ap );

  PROFILE_END;

}
#endif /* VPI_ONLY */

/*!
 \param stmt  Pointer to statement to compare with all expressions
 
 \return Returns a pointer to a list of all expressions found that call
         the specified statement.  Returns NULL if no expressions were
         found in the design that match this statement.

 Searches the list of all expressions in all functional units that call
 the specified statement and returns these in a list format to the calling
 function.  This function should only be called after the entire design has
 been parsed to be completely correct.
*/
void db_remove_stmt_blks_calling_statement( statement* stmt ) { PROFILE(DB_REMOVE_STMT_BLKS_CALLING_STATEMENT);

  inst_link* instl;  /* Pointer to current instance */ 

  assert( stmt != NULL );

  instl = inst_head;
  while( instl != NULL ) {
    instance_remove_stmt_blks_calling_stmt( instl->inst, stmt );
    instl = instl->next;
  }

  PROFILE_END;

}

/*!
 \return Returns the string version of the current instance scope (memory allocated).
*/
char* db_gen_curr_inst_scope() { PROFILE(DB_GEN_CURR_INST_SCOPE);

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

  assert( leading_hier_num > 0 );

  if( (scope = db_gen_curr_inst_scope()) != NULL ) {

    if( strcmp( leading_hierarchies[0], "*" ) != 0 ) {
      scope_extract_scope( scope, leading_hierarchies[0], stripped_scope );
    } else {
      strcpy( stripped_scope, scope );
    }

    free_safe( scope );

    if( stripped_scope[0] != '\0' ) {

      curr_instance = inst_link_find_by_scope( stripped_scope, inst_head );

      /* If we have found at least one matching instance, set the one_instance_found flag */
      if( curr_instance != NULL ) {
        one_instance_found = TRUE;
      }

    }

  } else {

    curr_instance = NULL;

  }

  PROFILE_END;

} 

/*!
 \param scope  Current VCD scope.

 Sets the curr_inst_scope global variable to the specified scope.
*/
void db_set_vcd_scope( char* scope ) { PROFILE(DB_SET_VCD_SCOPE);

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_vcd_scope, scope: %s", obf_inst( scope ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  assert( scope != NULL );

  /* Create a new scope item */
  curr_inst_scope = (char**)realloc( curr_inst_scope, (sizeof( char* ) * (curr_inst_scope_size + 1)) );
  curr_inst_scope[curr_inst_scope_size] = strdup_safe( scope );
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

  char back[4096];   /* Lowest level of hierarchy */
  char rest[4096];   /* Hierarchy up one level */

#ifdef DEBUG_MODE
  char* scope = db_gen_curr_inst_scope();
  snprintf( user_msg, USER_MSG_LENGTH, "In db_vcd_upscope, curr_inst_scope: %s", obf_inst( scope ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  free_safe( scope );
#endif

  /* Deallocate the last scope item */
  if( curr_inst_scope_size > 0 ) {

    curr_inst_scope_size--;
    free_safe( curr_inst_scope[curr_inst_scope_size] );

    db_sync_curr_instance();

  }

  PROFILE_END;

}

/*!
 \param name    Name of signal to set value to.
 \param symbol  Symbol value of signal used in VCD dumpfile.
 \param msb     Most significant bit of symbol to set.
 \param lsb     Least significant bit of symbol to set.

 Creates a new entry in the symbol table for the specified signal and symbol.
*/
void db_assign_symbol( char* name, char* symbol, int msb, int lsb ) { PROFILE(DB_ASSIGN_SYMBOL);

  sig_link* slink;   /* Pointer to signal containing this symbol */
  vsignal   tmpsig;  /* Temporary signal to search for */

#ifdef DEBUG_MODE
  char* scope = db_gen_curr_inst_scope();
  snprintf( user_msg, USER_MSG_LENGTH, "In db_assign_symbol, name: %s, symbol: %s, curr_inst_scope: %s, msb: %d, lsb: %d",
            obf_sig( name ), symbol, obf_inst( scope ), msb, lsb );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  free_safe( scope );
#endif

  assert( name != NULL );

  if( curr_instance != NULL ) {
    
    tmpsig.name = name;

    /* Find the signal that matches the specified signal name */
    if( (slink = sig_link_find( &tmpsig, curr_instance->funit->sig_head )) != NULL ) {

      /* Only add the symbol if we are not going to generate this value ourselves */
      if( (slink->sig->suppl.part.assigned == 0)              &&
          (slink->sig->suppl.part.type != SSUPPL_TYPE_PARAM)  &&
          (slink->sig->suppl.part.type != SSUPPL_TYPE_ENUM)   &&
          (slink->sig->suppl.part.type != SSUPPL_TYPE_MEM)    &&
          (slink->sig->suppl.part.type != SSUPPL_TYPE_GENVAR) &&
          (slink->sig->suppl.part.type != SSUPPL_TYPE_EVENT) ) {

        /* Add this signal */
        symtable_add( symbol, slink->sig, msb, lsb );

      }

    }

  }

  PROFILE_END;

}

/*!
 \param sym    Name of symbol to set character value to.
 \param value  String version of value to set symbol table entry to.
 
 Searches the timestep symtable followed by the VCD symbol table searching for
 the symbol that matches the specified argument.  Once a symbol is found, its value
 parameter is set to the specified character.  If the symbol was found in the VCD
 symbol table, it is copied to the timestep symbol table.
*/
void db_set_symbol_char( char* sym, char value ) { PROFILE(DB_SET_SYMBOL_CHAR);

  char val[2];  /* Value to store */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_char, sym: %s, value: %c", sym, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Put together value string */
  val[0] = value;
  val[1] = '\0';

  /* Set value of all matching occurrences in current timestep. */
  symtable_set_value( sym, val );

  PROFILE_END;

}

/*!
 \param sym    Name of symbol to set character value to.
 \param value  String version of value to set symbol table entry to.
 
 Searches the timestep symtable followed by the VCD symbol table searching for
 the symbol that matches the specified argument.  Once a symbol is found, its value
 parameter is set to the specified string.  If the symbol was found in the VCD
 symbol table, it is copied to the timestep symbol table.
*/
void db_set_symbol_string( char* sym, char* value ) { PROFILE(DB_SET_SYMBOL_STRING);

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_string, sym: %s, value: %s", sym, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Set value of all matching occurrences in current timestep. */
  symtable_set_value( sym, value );

  PROFILE_END;

}

/*!
 \param time   Current time step value being performed.
 \param final  Specifies that this is the final timestep.

 Cycles through expression queue, performing expression evaluations as we go.  If
 an expression has a parent expression, that parent expression is placed in the
 expression queue after that expression has completed its evaluation.  When the
 expression queue is empty, we are finished for this clock period.
*/
void db_do_timestep( uint64 time, bool final ) { PROFILE(DB_DO_TIMESTEP);

  static sim_time curr_time;
  static uint64   last_sim_update = 0;

#ifdef DEBUG_MODE
  if( final ) {
    print_output( "Performing final timestep", DEBUG, __FILE__, __LINE__ );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "Performing timestep #%lld", time );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  curr_time.lo    = (time & 0xffffffffLL);
  curr_time.hi    = ((time >> 32) & 0xffffffffLL);
  curr_time.full  = time;
  curr_time.final = final;

  if( (timestep_update > 0) && ((time - last_sim_update) >= timestep_update) && !debug_mode && !final ) {
    last_sim_update = time;
    printf( "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bPerforming timestep %10lld", time );
    fflush( stdout );
  }

  /* Simulate the current timestep */
  sim_simulate( &curr_time );

  /* If this is the last timestep, add the final list and do one more simulate */
  if( final ) {
    curr_time.lo   = 0xffffffff;
    curr_time.hi   = 0xffffffff;
    curr_time.full = 0xffffffffffffffffLL;
    sim_simulate( &curr_time );
  }

#ifdef DEBUG_MODE
  print_output( "Assigning postsimulation signals...", DEBUG, __FILE__, __LINE__ );
#endif

  /* Assign all stored values in current post-timestep to stored signals */
  symtable_assign( &curr_time );

  PROFILE_END;

}


/*
 $Log$
 Revision 1.268  2007/12/20 04:47:50  phase1geo
 Fixing the last of the regression failures from previous changes.  Removing unnecessary
 output used for debugging.

 Revision 1.267  2007/12/19 22:54:34  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.266  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.265  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.264  2007/12/12 07:23:18  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.263  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.262  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.261  2007/09/12 05:40:11  phase1geo
 Adding support for bool and char types in FOR loop initialization blocks.  Adding
 a plethora of new diagnostics to completely verify this new functionality.  These
 new diagnostics have not been run yet so they will fail in the next run of regression
 but should be ready to be checked out.

 Revision 1.260  2007/09/05 21:07:36  phase1geo
 Fixing bug 1788991.  Full regression passes.  Removed excess output used for
 debugging.  May want to create a new development release with these changes.

 Revision 1.259  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.258  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.257  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.256  2007/07/24 22:52:26  phase1geo
 More clean-up for VCS regressions.  Still not fully passing yet.

 Revision 1.255  2007/07/18 22:39:17  phase1geo
 Checkpointing generate work though we are at a fairly broken state at the moment.

 Revision 1.254  2007/07/18 02:15:04  phase1geo
 Attempts to fix a problem with generating instances with hierarchy.  Also fixing
 an issue with named blocks in generate statements.  Still some work to go before
 regressions are passing again, however.

 Revision 1.253  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.252  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.251  2007/04/02 04:50:04  phase1geo
 Adding func_iter files to iterate through a functional unit for reporting
 purposes.  Updated affected files.

 Revision 1.250  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.249  2007/03/19 22:52:50  phase1geo
 Attempting to fix problem with line ordering for a named block that is
 in the middle of another statement block.  Also fixed a problem with FORK
 expressions not being bound early enough.  Run currently segfaults but
 I need to checkpoint at the moment.

 Revision 1.248  2007/03/16 21:41:08  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.247  2007/03/08 05:17:29  phase1geo
 Various code fixes.  Full regression does not yet pass.

 Revision 1.246  2006/12/23 04:44:45  phase1geo
 Fixing build problems on cygwin.  Fixing compile errors with VPI and fixing
 segmentation fault in the funit_converge function.  Regression is far from
 passing at this point.  Checkpointing.

 Revision 1.245  2006/12/19 06:06:05  phase1geo
 Shortening unnamed scope name from $unnamed_%d to $u%d.  Also fixed a few
 bugs in the instance_flatten function (still more debug work to go here).
 Checkpointing.

 Revision 1.244  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.243  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.242  2006/12/14 23:46:57  phase1geo
 Fixing remaining compile issues with support for functional unit pointers in
 expressions and unnamed scope handling.  Starting to debug run-time issues now.
 Added atask1 diagnostic to begin this verification process.  Checkpointing.

 Revision 1.241  2006/12/12 06:20:22  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.240  2006/12/11 23:29:16  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.

 Revision 1.239  2006/11/29 23:15:45  phase1geo
 Major overhaul to simulation engine by including an appropriate delay queue
 mechanism to handle simulation timing for delay operations.  Regression not
 fully passing at this moment but enough is working to checkpoint this work.

 Revision 1.238  2006/11/28 16:39:46  phase1geo
 More updates for regressions due to changes in delay handling.  Still work
 to go.

 Revision 1.237  2006/11/27 04:11:41  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.236  2006/11/25 21:29:01  phase1geo
 Adding timescale diagnostics to regression suite and fixing bugs in core associated
 with this code.  Full regression now passes for IV and Cver (not in VPI mode).

 Revision 1.235  2006/11/25 04:24:39  phase1geo
 Adding initial code to fully support the timescale directive and its usage.
 Added -vpi_ts score option to allow the user to specify a top-level timescale
 value for the generated VPI file (this code has not been tested at this point,
 however).  Also updated diagnostic Makefile to get the VPI shared object files
 from the current lib directory (instead of the checked in one).

 Revision 1.234  2006/11/22 20:20:01  phase1geo
 Updates to properly support 64-bit time.  Also starting to make changes to
 simulator to support "back simulation" for when the current simulation time
 has advanced out quite a bit of time and the simulator needs to catch up.
 This last feature is not quite working at the moment and regressions are
 currently broken.  Checkpointing.

 Revision 1.233  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.232  2006/11/17 23:17:12  phase1geo
 Fixing bug in score command where parameter override values were not being saved
 off properly in the CDD file.  Also fixing bug when a parameter is found in a VCD
 file (ignoring its usage).  Updated regressions for these changes.

 Revision 1.231  2006/11/03 23:36:36  phase1geo
 Fixing bug 1590104.  Updating regressions per this change.

 Revision 1.230  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.229  2006/10/12 22:48:45  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.228  2006/10/09 17:54:19  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.227  2006/10/06 17:18:12  phase1geo
 Adding support for the final block type.  Added final1 diagnostic to regression
 suite.  Full regression passes.

 Revision 1.226  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.225  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.224  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.223  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.222  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.221  2006/09/08 22:39:50  phase1geo
 Fixes for memory problems.

 Revision 1.220  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.219  2006/09/05 21:00:44  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.218  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.217  2006/09/01 04:06:36  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.216  2006/08/31 22:32:17  phase1geo
 Things are in a state of flux at the moment.  I have added proper parsing support
 for assertions, properties and sequences.  Also added partial support for the $root
 space (though I need to work on figuring out how to handle it in terms of the
 instance tree) and all that goes along with that.  Add parsing support with an
 error message for multi-dimensional array declarations.  Regressions should not be
 expected to run correctly at the moment.

 Revision 1.215  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.214  2006/08/29 02:51:33  phase1geo
 Adding enumeration parsing support to parser.  No functionality at this point, however.

 Revision 1.213  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.212  2006/08/25 22:49:45  phase1geo
 Adding support for handling generated hierarchical names in signals that are outside
 of generate blocks.  Added support for op-and-assigns in generate for loops as well
 as normal for loops.  Added generate11.4 and for3 diagnostics to regression suite
 to verify this new behavior.  Full regressions have not been verified with these
 changes however.  Checkpointing.

 Revision 1.211  2006/08/20 03:20:59  phase1geo
 Adding support for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, <<<=, >>>=, ++
 and -- operators.  The op-and-assign operators are currently good for
 simulation and code generation purposes but still need work in the comb.c
 file for proper combinational logic underline and reporting support.  The
 increment and decrement operations should be fully supported with the exception
 of their use in FOR loops (I'm not sure if this is supported by SystemVerilog
 or not yet).  Also started adding support for delayed assignments; however, I
 need to rework this completely as it currently causes segfaults.  Added lots of
 new diagnostics to verify this new functionality and updated regression for
 these changes.  Full IV regression now passes.

 Revision 1.210  2006/08/18 22:07:44  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.209  2006/08/15 16:21:53  phase1geo
 Fixing bug for generate4 diagnostic which incorrectly added a BIND element
 when not in a generate block.  Full regression now passes.

 Revision 1.208  2006/08/11 15:16:48  phase1geo
 Joining slist3.3 diagnostic to latest development branch.  Adding changes to
 fix memory issues from bug 1535412.

 Revision 1.207  2006/08/10 22:35:14  phase1geo
 Updating with fixes for upcoming 0.4.7 stable release.  Updated regressions
 for this change.  Full regression still fails due to an unrelated issue.

 Revision 1.206  2006/08/02 22:28:31  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.205  2006/08/01 04:38:20  phase1geo
 Fixing issues with binding to non-module scope and not binding references
 that reference a "no score" module.  Full regression passes.

 Revision 1.204  2006/07/31 22:11:07  phase1geo
 Fixing bug with generated tasks.  Added diagnostic to test generate functions
 (this is currently failing with a binding issue).

 Revision 1.203  2006/07/30 04:59:51  phase1geo
 Modifying db_find_signal to use scope lookup function (for upwards name
 referencing purposes).  Emits user error if specified signal could not be
 found (we are understanding that if the db_find_signal function is used, the
 signal better be within the current scope (no hierarchical referencing allowed).
 Diagnostic generate8.2 should work now with VCS.

 Revision 1.202  2006/07/29 21:15:08  phase1geo
 Fixing last issue with generate8.1.v.  Full regression passes with IV.  Still
 need to check VCS regression run.

 Revision 1.201  2006/07/29 20:53:42  phase1geo
 Fixing some code related to generate statements; however, generate8.1 is still
 not completely working at this point.  Full regression passes for IV.

 Revision 1.200  2006/07/27 16:27:16  phase1geo
 Adding diagnostics to verify basic generate case blocks.  Full regression
 passes.

 Revision 1.199  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.198  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.197  2006/07/21 22:39:00  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.196  2006/07/21 17:47:09  phase1geo
 Simple if and if-else generate statements are now working.  Added diagnostics
 to regression suite to verify these.  More testing to follow.

 Revision 1.195  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.194  2006/07/20 20:11:08  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.193  2006/07/20 04:55:18  phase1geo
 More updates to support generate blocks.  We seem to be passing the parser
 stage now.  Getting segfaults in the generate_resolve code, presently.

 Revision 1.192  2006/07/19 22:30:45  phase1geo
 More work done for generate support.  Still have a ways to go.

 Revision 1.191  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.190  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.189  2006/07/15 05:49:04  phase1geo
 Removed the old manstyle documentation directory as this tool is no longer
 used to generate user documentation.  Created new keywords files to break
 out 1995, 2001 and SystemVerilog syntax.  Added several new diagnostics
 to regression suite to verify this new feature (still more parser code to
 add to completely support this feature).  Also fixed several bugs that
 existed in the 0.4.5 stable release and added those tests.  Full regression
 is passing.  Finally, updated the manpage for the new -g option to the score
 command.  Still need to document this new option in the user's guide.  Added
 ability to verify the -g option diagnostics to the regression suite.

 Revision 1.188  2006/07/14 18:53:32  phase1geo
 Fixing -g option for keywords.  This seems to be working and I believe that
 regressions are passing here as well.

 Revision 1.187  2006/07/13 21:03:44  phase1geo
 Fixing bug in db_read function to set curr_funit to the correct structure
 when merging functional units.  Updated regressions per this fix.

 Revision 1.186  2006/07/12 22:16:18  phase1geo
 Fixing hierarchical referencing for instance arrays.  Also attempted to fix
 a problem found with unary1; however, the generated report coverage information
 does not look correct at this time.  Checkpointing what I have done for now.

 Revision 1.185  2006/07/11 02:32:47  phase1geo
 Fixing memory leak problem in db_close function (with score_args).

 Revision 1.184  2006/06/27 19:34:42  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.183  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.182  2006/05/25 12:10:57  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.181  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.180  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.179  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.178  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.177  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.176  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.175.4.1.4.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.175.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.175  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.174  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.173  2006/02/02 22:37:40  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.172  2006/02/01 15:13:10  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.171  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.170  2006/01/26 22:40:13  phase1geo
 Fixing last LXT bug.

 Revision 1.169  2006/01/25 16:51:27  phase1geo
 Fixing performance/output issue with hierarchical references.  Added support
 for hierarchical references to parser.  Full regression passes.

 Revision 1.168  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.167  2006/01/20 22:44:51  phase1geo
 Moving parameter resolution to post-bind stage to allow static functions to
 be considered.  Regression passes without static function testing.  Static
 function support still has some work to go.  Checkpointing.

 Revision 1.166  2006/01/20 19:15:23  phase1geo
 Fixed bug to properly handle the scoping of parameters when parameters are created/used
 in non-module functional units.  Added param10*.v diagnostics to regression suite to
 verify the behavior is correct now.

 Revision 1.165  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.164  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.163  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.162  2006/01/12 22:53:01  phase1geo
 Adding support for localparam construct.  Added tests to regression suite to
 verify correct functionality.  Full regression passes.

 Revision 1.161  2006/01/12 22:14:45  phase1geo
 Completed code for handling parameter value pass by name Verilog-2001 syntax.
 Added diagnostics to regression suite and updated regression files for this
 change.  Full regression now passes.

 Revision 1.160  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.159  2006/01/10 05:56:36  phase1geo
 In the middle of adding support for event sensitivity lists to score command.
 Regressions should pass but this code is not complete at this time.

 Revision 1.158  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.157  2006/01/03 22:59:16  phase1geo
 Fixing bug in expression_assign function -- removed recursive assignment when
 the LHS expression is a signal, single-bit, multi-bit or static value (only
 recurse when the LHS is a CONCAT or LIST).  Fixing bug in db_close function to
 check if the instance tree has been populated before deallocating memory for it.
 Fixing bug in report help information when Tcl/Tk is not available.  Added bassign2
 diagnostic to regression suite to verify first described bug.

 Revision 1.156  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

 Revision 1.155  2005/12/17 05:47:36  phase1geo
 More memory fault fixes.  Regression runs cleanly and we have verified
 no memory faults up to define3.v.  Still have a ways to go.

 Revision 1.154  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.153  2005/12/13 23:15:14  phase1geo
 More fixes for memory leaks.  Regression fully passes at this point.

 Revision 1.152  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.151  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.150  2005/12/07 20:23:38  phase1geo
 Fixing case where statement is unconnectable.  Full regression now passes.

 Revision 1.149  2005/12/05 22:02:24  phase1geo
 Added initial support for disable expression.  Added test to verify functionality.
 Full regression passes.

 Revision 1.148  2005/12/05 21:28:07  phase1geo
 Getting fork statements with scope to work.  Added test to regression to verify
 this functionality.  Fixed bug in binding expression to named block.

 Revision 1.147  2005/12/05 20:26:55  phase1geo
 Fixing bugs in code to remove statement blocks that are pointed to by expressions
 in NB_CALL and FORK cases.  Fixed bugs in fork code -- this is now working at the
 moment.  Updated regressions which now fully pass.

 Revision 1.146  2005/12/02 19:58:36  phase1geo
 Added initial support for FORK/JOIN expressions.  Code is not working correctly
 yet as we need to determine if a statement should be done in parallel or not.

 Revision 1.145  2005/12/02 12:03:17  phase1geo
 Adding support for excluding functions, tasks and named blocks.  Added tests
 to regression suite to verify this support.  Full regression passes.

 Revision 1.144  2005/12/01 21:11:16  phase1geo
 Adding more error checking diagnostics into regression suite.  Full regression
 passes.

 Revision 1.143  2005/12/01 20:49:02  phase1geo
 Adding nested_block3 to verify nested named blocks in tasks.  Fixed named block
 usage to be FUNC_CALL or TASK_CALL -like based on its placement.

 Revision 1.142  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.141  2005/11/30 18:25:55  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.140  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.139  2005/11/29 19:04:47  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.138  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

 Revision 1.137  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.136  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.135  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.134  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.133  2005/11/16 23:02:23  phase1geo
 Added new diagnostics to check for functions that contain time-consuming expressions (this
 is not allowed by Covered -- and not by most commercial simulators either).  Updated check_test
 to do error checking and added error outputs.  Full regression passes at this point -- 225 diagnostics.

 Revision 1.132  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.131  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.130  2005/11/11 22:53:40  phase1geo
 Updated bind process to allow binding of structures from different hierarchies.
 Added task port signals to get added.

 Revision 1.129  2005/11/10 19:28:22  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.128  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.127  2005/02/11 22:50:31  phase1geo
 Fixing bug with removing statement blocks that contain statements that cannot
 currently be handled by Covered correctly.  There was a problem when the bad statement
 was the last statement in the statement block.  Updated regression accordingly.
 Added race condition diagnostics that currently are not in regression due to lack
 of code support for them.  Ifdef'ed out the BASSIGN stuff for this checkin.

 Revision 1.126  2005/02/09 14:12:20  phase1geo
 More code for supporting expression assignments.

 Revision 1.125  2005/02/08 23:18:22  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.124  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.123  2005/02/04 23:55:47  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.122  2005/01/25 13:42:27  phase1geo
 Fixing segmentation fault problem with race condition checking.  Added race1.1
 to regression.  Removed unnecessary output statements from previous debugging
 checkins.

 Revision 1.121  2005/01/10 23:03:37  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.120  2005/01/10 02:59:24  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.119  2005/01/07 17:59:50  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.118  2004/12/20 04:12:00  phase1geo
 A bit more race condition checking code added.  Still not there yet.

 Revision 1.117  2004/12/18 16:23:16  phase1geo
 More race condition checking updates.

 Revision 1.116  2004/04/21 05:14:03  phase1geo
 Adding report_gui checking to print_output and adding error handler to TCL
 scripts.  Any errors occurring within the code will be propagated to the user.

 Revision 1.115  2004/04/19 04:54:55  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.114  2004/04/17 14:07:54  phase1geo
 Adding replace and merge options to file menu.

 Revision 1.113  2004/04/07 11:36:03  phase1geo
 Changes made to allow opening multiple CDD files without needing to deallocate
 all memory for one before reallocating memory for another.  Things are not
 entirely working at this point.

 Revision 1.112  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.111  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.110  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.109  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.108  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.107  2003/11/12 17:34:03  phase1geo
 Fixing bug where signals are longer than allowable bit width.

 Revision 1.106  2003/10/28 01:09:38  phase1geo
 Cleaning up unnecessary output.

 Revision 1.105  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.104  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.103  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.102  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.101  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.100  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.99  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.98  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.97  2003/08/20 22:08:39  phase1geo
 Fixing problem with not closing VCD file after VCD parsing is completed.
 Also fixed memory problem with symtable.c to cause timestep_tab entries
 to only be loaded if they have not already been loaded during this timestep.
 Also added info.h to include list of db.c.

 Revision 1.96  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.95  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.94  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.93  2003/08/07 15:41:43  phase1geo
 Adding -ts option to score command to allow the current timestep to be
 output during the simulation phase.

 Revision 1.92  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.91  2003/02/19 00:47:08  phase1geo
 Getting things ready for next prelease.  Fixing bug with db_remove_statement
 function.

 Revision 1.90  2003/02/18 20:17:01  phase1geo
 Making use of scored flag in CDD file.  Causing report command to exit early
 if it is working on a CDD file which has not been scored.  Updated testsuite
 for these changes.

 Revision 1.89  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.88  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.87  2003/02/12 14:56:22  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

 Revision 1.86  2003/02/10 06:08:55  phase1geo
 Lots of parser updates to properly handle UDPs, escaped identifiers, specify blocks,
 and other various Verilog structures that Covered was not handling correctly.  Fixes
 for proper event type handling.  Covered can now handle most of the IV test suite from
 a parsing perspective.

 Revision 1.85  2003/02/08 21:54:04  phase1geo
 Fixing memory problems with db_remove_statement function.  Updating comments
 in statement.c to explain some of the changes necessary to properly remove
 a statement tree.

 Revision 1.84  2003/02/07 23:12:29  phase1geo
 Optimizing db_add_statement function to avoid memory errors.  Adding check
 for -i option to avoid user error.

 Revision 1.83  2003/02/07 02:28:23  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.82  2003/02/05 22:50:56  phase1geo
 Some minor tweaks to debug output and some minor bug "fixes".  At this point
 regression isn't stable yet.

 Revision 1.81  2003/02/03 17:17:38  phase1geo
 Fixing bug with statement deallocation for NULL statements.

 Revision 1.80  2003/01/25 22:39:02  phase1geo
 Fixing case where statement is found to be unsupported in middle of statement
 tree.  The entire statement tree is removed from consideration for simulation.

 Revision 1.79  2003/01/06 00:44:21  phase1geo
 Updates to NEWS, ChangeLog, development documentation and user documentation
 for new 0.2pre1_20030105 release.

 Revision 1.78  2003/01/05 22:25:23  phase1geo
 Fixing bug with declared integers, time, real, realtime and memory types where
 they are confused with implicitly declared signals and given 1-bit value types.
 Updating regression for changes.

 Revision 1.77  2003/01/03 05:52:00  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.76  2003/01/03 02:07:40  phase1geo
 Fixing segmentation fault in lexer caused by not closing the temporary
 input file before unlinking it.  Fixed case where module was parsed but not
 at the head of the module list.

 Revision 1.75  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.74  2002/12/11 14:51:57  phase1geo
 Fixes compiler errors from last checkin.

 Revision 1.73  2002/12/11 14:49:24  phase1geo
 Minor tweaks to parameter handling; however, problem with instance6.2.v not
 fixed at this time.

 Revision 1.72  2002/12/05 14:45:17  phase1geo
 Removing assertion error from instance6.1 failure; however, this case does not
 work correctly according to instance6.2.v diagnostic.  Added @(...) output in
 report command for edge-triggered events.  Also fixed bug where a module could be
 parsed more than once.  Full regression does not pass at this point due to
 new instance6.2.v diagnostic.

 Revision 1.71  2002/12/03 14:25:24  phase1geo
 Fixing bug in db_add_statement function.  Not parsing FALSE path if the next_false
 is the starting statement.

 Revision 1.70  2002/12/03 06:01:15  phase1geo
 Fixing bug where delay statement is the last statement in a statement list.
 Adding diagnostics to verify this functionality.

 Revision 1.69  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.68  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.67  2002/10/31 23:13:24  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.66  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.65  2002/10/23 03:39:06  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.64  2002/10/13 13:55:52  phase1geo
 Fixing instance depth selection and updating all configuration files for
 regression.  Full regression now passes.

 Revision 1.63  2002/10/12 22:21:35  phase1geo
 Making code fix for parameters when parameter is used in calculation of
 signal size.  Also adding parse ability for real numbers in a VCD file
 (though real number support is still avoided).

 Revision 1.62  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.61  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.60  2002/10/01 13:21:24  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.59  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.58  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.57  2002/09/23 01:37:44  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.56  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.55  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.54  2002/09/06 03:05:27  phase1geo
 Some ideas about handling parameters have been added to these files.  Added
 "Special Thanks" section in User's Guide for acknowledgements to people
 helping in project.

 Revision 1.53  2002/08/26 12:57:03  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.52  2002/08/23 12:55:32  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.51  2002/08/19 04:34:06  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.50  2002/08/14 04:52:48  phase1geo
 Removing unnecessary calls to signal_dealloc function and fixing bug
 with signal_dealloc function.

 Revision 1.49  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.48  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.47  2002/07/20 22:22:52  phase1geo
 Added ability to create implicit signals for local signals.  Added implicit1.v
 diagnostic to test for correctness.  Full regression passes.  Other tweaks to
 output information.

 Revision 1.46  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.45  2002/07/18 05:50:45  phase1geo
 Fixes should be just about complete for instance depth problems now.  Diagnostics
 to help verify instance handling are added to regression.  Full regression passes.

 Revision 1.44  2002/07/18 02:33:23  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.43  2002/07/17 21:45:56  phase1geo
 Fixing case where `define does not set to a value.  Looking into problem
 with embedded instances (at least 3 deep).

 Revision 1.42  2002/07/17 00:13:57  phase1geo
 Added support for -e option and informally tested.

 Revision 1.41  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.39  2002/07/13 05:35:52  phase1geo
 Cause warning message to be displayed for a signal found in the VCD dumpfile
 that is in a covered scope but is not part of the design.  It could be that
 the design and VCD file do not match.

 Revision 1.37  2002/07/12 04:53:29  phase1geo
 Removing counter code that was used for debugging infinite loops in code
 previously.

 Revision 1.36  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.35  2002/07/09 03:24:48  phase1geo
 Various fixes for module instantiantion handling.  This now works.  Also
 modified report output for toggle, line and combinational information.
 Regression passes.

 Revision 1.34  2002/07/09 02:04:25  phase1geo
 Fixing segmentation fault error due to deallocating a module before we
 have completed using it.

 Revision 1.33  2002/07/08 19:02:10  phase1geo
 Adding -i option to properly handle modules specified for coverage that
 are instantiated within a design without needing to parse parent modules.

 Revision 1.32  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.31  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.30  2002/07/05 00:10:18  phase1geo
 Adding report support for case statements.  Everything outputs fine; however,
 I want to remove CASE, CASEX and CASEZ expressions from being reported since
 it causes redundant and misleading information to be displayed in the verbose
 reports.  New diagnostics to check CASE expressions have been added and pass.

 Revision 1.29  2002/07/04 23:10:12  phase1geo
 Added proper support for case, casex, and casez statements in score command.
 Report command still incorrect for these statement types.

 Revision 1.28  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.27  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.26  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.25  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.24  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.23  2002/06/30 22:23:20  phase1geo
 Working on fixing looping in parser.  Statement connector needs to be revamped.

 Revision 1.22  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.21  2002/06/28 00:40:37  phase1geo
 Cleaning up extraneous output from debugging.

 Revision 1.20  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.19  2002/06/27 12:36:47  phase1geo
 Fixing bugs with scoring.  I think I got it this time.

 Revision 1.18  2002/06/26 04:59:50  phase1geo
 Adding initial support for delays.  Support is not yet complete and is
 currently untested.

 Revision 1.17  2002/06/26 03:45:48  phase1geo
 Fixing more bugs in simulator and report functions.  About to add support
 for delay statements.

 Revision 1.16  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.15  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.14  2002/06/24 12:34:56  phase1geo
 Fixing the set of the STMT_HEAD and STMT_STOP bits.  We are getting close.

 Revision 1.13  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.12  2002/06/22 21:08:23  phase1geo
 Added simulation engine and tied it to the db.c file.  Simulation engine is
 currently untested and will remain so until the parser is updated correctly
 for statements.  This will be the next step.

 Revision 1.11  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.10  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.9  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.8  2002/05/02 03:27:42  phase1geo
 Initial creation of statement structure and manipulation files.  Internals are
 still in a chaotic state.

 Revision 1.7  2002/04/30 05:04:25  phase1geo
 Added initial go-round of adding statement handling to parser.  Added simple
 Verilog test to check correct statement handling.  At this point there is a
 bug in the expression write function (we need to display statement trees in
 the proper order since they are unlike normal expression trees.)
*/
