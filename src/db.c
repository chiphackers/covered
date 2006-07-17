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
 \author   Trevor Williams  (trevorw@charter.net)
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

#include "defines.h"
#include "db.h"
#include "util.h"
#include "func_unit.h"
#include "expr.h"
#include "vsignal.h"
#include "link.h"
#include "symtable.h"
#include "instance.h"
#include "statement.h"
#include "sim.h"
#include "binding.h"
#include "param.h"
#include "static.h"
#include "stat.h"
#include "fsm.h"
#include "info.h"
#include "attr.h"
#include "race.h"
#include "scope.h"
#include "ovl.h"
#include "gen_item.h"


extern char*       top_module;
extern funit_inst* instance_root;
extern str_link*   no_score_head;
extern funit_link* funit_head;
extern funit_link* funit_tail;
extern nibble      or_optab[16];
extern char        user_msg[USER_MSG_LENGTH];
extern bool        one_instance_found;
extern char**      leading_hierarchies;
extern int         leading_hier_num;
extern isuppl      info_suppl;
extern int         timestep_update;
extern bool        debug_mode;
extern int*        fork_block_depth;
extern int         fork_depth;
extern int         block_depth;
extern tnode*      def_table;
extern char**      score_args;
extern int         score_arg_num;
extern int         generate_mode;

/*!
 Specifies the string Verilog scope that is currently specified in the VCD file.
*/
char* curr_inst_scope = NULL;

/*!
 Pointer to the current instance selected by the VCD parser.  If this value is
 NULL, the current instance does not reside in the design specified for coverage.
*/
funit_inst* curr_instance = NULL;

/*!
 Pointer to head of list of module names that need to be parsed yet.  These names
 are added in the db_add_instance function and removed in the db_end_module function.
*/
str_link* modlist_head  = NULL;

/*!
 Pointer to tail of list of module names that need to be parsed yet.  These names
 are added in the db_add_instance function and removed in the db_end_module function.
*/
str_link* modlist_tail  = NULL;

/*!
 Pointer to the functional unit structure for the functional unit that is currently being parsed.
*/
func_unit* curr_funit   = NULL;

/*!
 This static value contains the current expression ID number to use for the next expression found, it
 is incremented by one when an expression is found.  This allows us to have a unique expression ID
 for each expression (since expressions have no intrinsic names).
*/
int       curr_expr_id  = 1;

/*!
 This static value contains the current simulation time which is specified by the db_do_timestep
 function.  It is used for calculating delay expressions in the simulation engine.
*/
int       curr_sim_time   = 0;

/*!
 Contains timestep value when simulation was last performed.  This value is used to determine
 if the current timestep needs to be printed to standard output (if the -ts option is specified
 to the score command.
*/
int       last_sim_update = 0;

/*!
 Specifies current connection ID to use for connecting statements.  This value should be passed
 to the statement_connect function and incremented immediately after.
*/
int       stmt_conn_id    = 1;


/*!
 Deallocates all memory associated with the database.
*/
void db_close() {
  
  int i;  /* Loop iterator */

  if( instance_root != NULL ) {

    /* Remove memory allocated for instance_root and mod_head */
    assert( instance_root->funit != NULL );
    instance_dealloc( instance_root, instance_root->name );
    funit_link_delete_list( funit_head, TRUE );

    /* Deallocate preprocessor define tree */
    tree_dealloc( def_table );

    /* Deallocate the binding list */
    bind_dealloc();
    
    for( i=0; i<score_arg_num; i++ ) {
      free_safe( score_args[i] );
    }
    if( score_arg_num > 0 ) {
      free_safe( score_args );
    }

    instance_root = NULL;
    funit_head    = NULL;
    funit_tail    = NULL;
    def_table     = NULL;
    score_args    = NULL;
    score_arg_num = 0;

  }

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
bool db_write( char* file, bool parse_mode, bool report_save ) {

  bool  retval = TRUE;  /* Return value for this function */
  FILE* db_handle;      /* Pointer to database file being written */

  if( (db_handle = fopen( file, "w" )) != NULL ) {

    /* Reset expression IDs */
    curr_expr_id = 1;

    /* Iterate through instance tree */
    assert( instance_root != NULL );
    info_db_write( db_handle );
    instance_db_write( instance_root, db_handle, instance_root->name, parse_mode, report_save );
    fclose( db_handle );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for writing", file );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

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
bool db_read( char* file, int read_mode ) {

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
  snprintf( user_msg, USER_MSG_LENGTH, "In db_read, file: %s, mode: %d", file, read_mode );
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
              
            if( instance_root == NULL ) {
                
              instance_read_add( &instance_root, NULL, curr_funit, funit_scope );
                
            } else {
                
              /* Add functional unit to instance tree and functional unit list */
              scope_extract_back( funit_scope, back, parent_scope );

              /* Make sure that functional unit in database was not written before its parent functional unit */
              assert( instance_find_scope( instance_root, parent_scope ) != NULL );

              /* Add functional unit to instance tree and functional unit list */
              instance_read_add( &instance_root, parent_scope, curr_funit, back );
                
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
            if( (read_mode == READ_MODE_MERGE_INST_MERGE) && ((foundinst = instance_find_scope( instance_root, funit_scope )) != NULL) ) {
              merge_mode = TRUE;
              curr_funit = foundinst->funit;
              funit_db_merge( foundinst->funit, db_handle, TRUE );
            } else if( read_mode == READ_MODE_REPORT_MOD_REPLACE ) {
              if( (foundfunit = funit_link_find( &tmpfunit, funit_head )) != NULL ) {
                merge_mode = TRUE;
                curr_funit = foundfunit->funit;
                /*
                 If this functional unit has been assigned a stat, remove it and replace it with the new functional unit contents;
                 otherwise, merge the results of the new functional unit with the old.
                */
                if( foundfunit->funit->stat != NULL ) {
                  statistic_dealloc( foundfunit->funit->stat );
                  foundfunit->funit->stat = NULL;
                  funit_db_replace( foundfunit->funit, db_handle );
                } else {
                  funit_db_merge( foundfunit->funit, db_handle, FALSE );
                }
              } else {
                retval = FALSE;
              }
            } else if( (read_mode == READ_MODE_REPORT_MOD_MERGE) && ((foundfunit = funit_link_find( &tmpfunit, funit_head )) != NULL) ) {
              merge_mode = TRUE;
              curr_funit = foundfunit->funit;
              funit_db_merge( foundfunit->funit, db_handle, FALSE );
            } else {
              curr_funit             = funit_create();
              curr_funit->name       = strdup_safe( funit_name, __FILE__, __LINE__ );
              curr_funit->type       = tmpfunit.type;
              curr_funit->filename   = strdup_safe( funit_file, __FILE__, __LINE__ );
              curr_funit->start_line = tmpfunit.start_line;
              curr_funit->end_line   = tmpfunit.end_line;
              if( tmpfunit.type != FUNIT_MODULE ) {
                curr_funit->parent = scope_get_parent_funit( funit_scope );
                parent_mod         = scope_get_parent_module( funit_scope );
                funit_link_add( curr_funit, &(parent_mod->tf_head), &(parent_mod->tf_tail) );
              }
            }

          }

        } else {

          snprintf( user_msg, USER_MSG_LENGTH, "Unexpected type %d when parsing database file %s", type, file );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          retval = FALSE;

        }

      } else {

        snprintf( user_msg, USER_MSG_LENGTH, "Unexpected line in database file %s", file );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;

      }

      free_safe( curr_line );

    }

    fclose( db_handle );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for reading", file );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  /* If the last functional unit was being read, add it now */
  if( curr_funit != NULL ) {

    if( instance_root == NULL ) {
      
      instance_read_add( &instance_root, NULL, curr_funit, funit_scope );
      
    } else {
      
      /* Add functional unit to instance tree and functional unit list */
      scope_extract_back( funit_scope, back, parent_scope );
    
      /* Make sure that functional unit in database not written before its parent functional unit */
      if( instance_find_scope( instance_root, parent_scope ) != NULL ) {

        /* Add functional unit to instance tree and functional unit list */
        instance_read_add( &instance_root, parent_scope, curr_funit, back );

      } else {

        print_output( "CDD file is not related to currently opened CDD file", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
 
      }
      
    }
    
    /* If the current functional unit was being merged, don't add it to the functional unit list again */
    if( !merge_mode ) {
      funit_link_add( curr_funit, &funit_head, &funit_tail );
    }

    curr_funit = NULL;

  }

#ifdef DEBUG_MODE
  /* Display the instance tree, if we are debugging */
  if( debug_mode && retval ) {
    instance_display_tree( instance_root );
  }
#endif

  return( retval );

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
func_unit* db_add_instance( char* scope, char* name, int type, vector_width* range ) {

  func_unit*  funit = NULL;      /* Pointer to functional unit */
  funit_link* found_funit_link;  /* Pointer to found funit_link in functional unit list */

  /* There should always be a parent so internal error if it does not exist. */
  assert( curr_funit != NULL );

  /* If this functional unit name is in our list of no_score functional units, skip adding the instance */
  if( str_link_find( name, no_score_head ) == NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_instance, instance: %s, %s:  %s", scope, get_funit_type( type ), name );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Create new functional unit node */
    funit       = funit_create();
    funit->name = strdup_safe( name, __FILE__, __LINE__ );
    funit->type = type;

    /* If a range has been specified, calculate its width and lsb now */
    if( range != NULL ) {
      if( (range->left != NULL) && (range->left->exp != NULL) ) {
        mod_parm_add( NULL, NULL, NULL, FALSE, range->left->exp, PARAM_TYPE_INST_MSB, curr_funit, scope );
      }
      if( (range->right != NULL) && (range->right->exp != NULL) ) {
        mod_parm_add( NULL, NULL, NULL, FALSE, range->right->exp, PARAM_TYPE_INST_LSB, curr_funit, scope );
      }
    }

    if( (found_funit_link = funit_link_find( funit, funit_head )) != NULL ) {

      if( type != FUNIT_MODULE ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Multiple identical task/function/named-begin-end names (%s) found in module %s, file %s\n",
                  scope, curr_funit->name, curr_funit->filename );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      }

      /* If we are currently within a generate block, create a generate item for this instance to resolve it later */
      if( generate_mode > 0 ) {
        gitem_link_add( gen_item_create_inst( instance_create( found_funit_link->funit, scope, range ) ),
                        &(curr_funit->gitem_head), &(curr_funit->gitem_tail) );
      } else {
        instance_parse_add( &instance_root, curr_funit, found_funit_link->funit, scope, range, FALSE );
      }

      funit_dealloc( funit );

    } else {

      /* If we are currently within a generate block, create a generate item for this instance to resolve it later */
      if( generate_mode > 0 ) {
      
        gitem_link_add( gen_item_create_inst( instance_create( funit, scope, range ) ),
                        &(curr_funit->gitem_head), &(curr_funit->gitem_tail) );

      } else {

        /* Add new functional unit to functional unit list. */
        funit_link_add( funit, &funit_head, &funit_tail );

        /* Add instance. */
        instance_parse_add( &instance_root, curr_funit, funit, scope, range, FALSE );

      }

      if( (type == FUNIT_MODULE) && (str_link_find( name, modlist_head ) == NULL) ) {
        str_link_add( strdup_safe( name, __FILE__, __LINE__ ), &modlist_head, &modlist_tail );
      }
      
    }

  }

  return( funit );

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
void db_add_module( char* name, char* file, int start_line ) {

  func_unit   mod;   /* Temporary module for comparison */
  funit_link* modl;  /* Pointer to found tree node */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_module, module: %s, file: %s, start_line: %d", name, file, start_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Set current module to this module */
  mod.name = name;
  mod.type = FUNIT_MODULE;

  modl = funit_link_find( &mod, funit_head );

  assert( modl != NULL );

  curr_funit             = modl->funit;
  curr_funit->filename   = strdup_safe( file, __FILE__, __LINE__ );
  curr_funit->start_line = start_line;
  
}

/*!
 \param end_line  Ending line number of specified module in file.

 Updates the modlist for parsing purposes.
*/
void db_end_module( int end_line ) {

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_end_module, end_line: %d", end_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  curr_funit->end_line = end_line;

  str_link_remove( curr_funit->name, &modlist_head, &modlist_tail );

  curr_funit = NULL;

}

/*!
 \param type        Specifies type of functional unit being added (function, task or named_block)
 \param name        Name of functional unit
 \param file        File containing the specified functional unit
 \param start_line  Starting line number of functional unit

 \return Returns TRUE if specified function, task or named block was added to the design.  If
         it was not, returns FALSE to indicate that this block should be ignored.
*/
bool db_add_function_task_namedblock( int type, char* name, char* file, int start_line ) {

  func_unit* tf;         /* Pointer to created functional unit */
  func_unit* parent;     /* Pointer to parent module for the newly created functional unit */
  char*      full_name;  /* Full name of function/task/namedblock which includes the parent module name */

  assert( (type == FUNIT_FUNCTION) || (type == FUNIT_TASK) || (type == FUNIT_NAMED_BLOCK) );

#ifdef DEBUG_MODE
  switch( type ) {
    case FUNIT_FUNCTION :
      snprintf( user_msg, USER_MSG_LENGTH, "In db_add_function_task_namedblock, function: %s, file: %s, start_line: %d", name, file, start_line );
      break;
    case FUNIT_TASK :
      snprintf( user_msg, USER_MSG_LENGTH, "In db_add_function_task_namedblock, task: %s, file: %s, start_line: %d", name, file, start_line );
      break;
    case FUNIT_NAMED_BLOCK :
      snprintf( user_msg, USER_MSG_LENGTH, "In db_add_function_task_namedblock, named_block: %s, file: %s, start_line: %d", name, file, start_line );
      break;
    default :  break;
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Generate full name to use for the function/task */
  full_name = funit_gen_task_function_namedblock_name( name, curr_funit );

  /* Add this as an instance so we can get scope */
  if( (tf = db_add_instance( name, full_name, type, NULL )) != NULL ) {

    /* Get parent */
    parent = funit_get_curr_module( curr_funit );

    if( generate_mode > 0 ) {
      /* Add this tfn as a generate item to the parent module list - TBD */
    } else {
      /* Store this functional unit in the parent module list */
      funit_link_add( tf, &(parent->tf_head), &(parent->tf_tail) );
    }

    /* Set our parent pointer to the current functional unit */
    tf->parent = curr_funit;

    /* Set current functional unit to this functional unit */
    curr_funit             = tf;
    curr_funit->filename   = strdup_safe( file, __FILE__, __LINE__ );
    curr_funit->start_line = start_line;
    
  }

  free_safe( full_name );

  return( tf != NULL );

}

/*!
 \param end_line  Line number of end of this task/function
*/
void db_end_function_task_namedblock( int end_line ) {

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_end_function_task_namedblock, end_line: %d", end_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Store last line information */
  curr_funit->end_line = end_line;

  /* Set the current functional unit to the parent module */
  curr_funit = curr_funit->parent;

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
void db_add_declared_param( bool is_signed, static_expr* msb, static_expr* lsb, char* name, expression* expr, bool local ) {

  mod_parm* mparm;  /* Pointer to added module parameter */

  assert( name != NULL );

  /* If a parameter value type is not supported, don't create this parameter */
  if( expr != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_declared_param, param: %s, expr: %d, local: %d", name, expr->id, local );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    if( mod_parm_find( name, curr_funit->param_head ) == NULL ) {

      /* Add parameter to module parameter list */
      mparm = mod_parm_add( name, msb, lsb, is_signed, expr, (local ? PARAM_TYPE_DECLARED_LOCAL : PARAM_TYPE_DECLARED), curr_funit, NULL );

    }

  }

}

/*!
 \param inst_name   Name of instance being overridden.
 \param expr        Expression containing value of override parameter.
 \param param_name  Name of parameter being overridden (for parameter_value_byname syntax)

 Creates override parameter and stores this in the current module as well
 as all associated instances.
*/
void db_add_override_param( char* inst_name, expression* expr, char* param_name ) {

  mod_parm* mparm;  /* Pointer to module parameter added to current module */

#ifdef DEBUG_MODE
  if( param_name != NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s, param_name: %s", inst_name, param_name );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s", inst_name );
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Add override parameter to module parameter list */
  mparm = mod_parm_add( param_name, NULL, NULL, FALSE, expr, PARAM_TYPE_OVERRIDE, curr_funit, inst_name );

}

/*!
 \param sig       Pointer to signal to attach parameter to.
 \param parm_exp  Expression containing value of vector parameter.
 \param type      Type of signal vector parameter to create (LSB or MSB).

 Creates a vector parameter for the specified signal or expression with the specified
 parameter expression.  This function is called by the parser.
*/
void db_add_vector_param( vsignal* sig, expression* parm_exp, int type ) {

  mod_parm* mparm;  /* Holds newly created module parameter */

  assert( sig != NULL );
  assert( (type == PARAM_TYPE_SIG_LSB) || (type == PARAM_TYPE_SIG_MSB) );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_vector_param, signal: %s, type: %d", sig->name, type );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Add signal vector parameter to module parameter list */
  mparm = mod_parm_add( NULL, NULL, NULL, FALSE, parm_exp, type, curr_funit, NULL );

  /* Add signal to module parameter list */
  mparm->sig = sig;

}

/*!
 \param name  Name of parameter value to override.
 \param expr  Expression value of parameter override.

 Adds specified parameter to the defparam list.
*/
void db_add_defparam( char* name, expression* expr ) {

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_defparam, defparam: %s", name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  snprintf( user_msg, USER_MSG_LENGTH, "defparam construct is not supported, line: %d.  Use -P option to score instead", expr->line );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );

  expression_dealloc( expr, FALSE );

}

/*!
 \param name       Name of signal being added.
 \param type       Type of signal being added.
 \param left       Specifies constant value for calculation of left-hand vector value.
 \param right      Specifies constant value for calculation of right-hand vector value.
 \param is_signed  Specifies that this signal is signed (TRUE) or not (FALSE)
 \param mba        Set to TRUE if specified signal must be assigned by simulated results.
 \param line       Line number where signal was declared.
 \param col        Starting column where signal was declared.

 Creates a new signal with the specified parameter information and adds this
 to the signal list if it does not already exist.  If width == 0, the sig_msb
 and sig_lsb values are interrogated.  If sig_msb and/or is non-NULL, its value is
 add to the current module's parameter list and all associated instances are
 updated to contain new value.
*/
void db_add_signal( char* name, int type, static_expr* left, static_expr* right, bool is_signed, bool mba, int line, int col ) {

  vsignal  tmpsig;      /* Temporary signal for signal searching */
  vsignal* sig;         /* Container for newly created signal */
  int      lsb;         /* Signal LSB */
  int      width;       /* Signal width */
  int      big_endian;  /* Signal endianness */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_signal, signal: %s, line: %d, col: %d", name, line, col );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  tmpsig.name = name;

  /* Add signal to current module's signal list if it does not already exist */
  if( sig_link_find( &tmpsig, curr_funit->sig_head ) == NULL ) {

    static_expr_calc_lsb_and_width_pre( left, right, &width, &lsb, &big_endian );

    /* Check to make sure that signal width does not exceed maximum size */
    if( width > MAX_BIT_WIDTH ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Signal (%s) width (%d) exceeds maximum allowed by Covered (%d).  Ignoring...", name, width, MAX_BIT_WIDTH );
      print_output( user_msg, WARNING, __FILE__, __LINE__ );
      width = -1;
      left  = NULL;
      right = NULL;
    }  

    if( (lsb != -1) && (width != -1) ) { 
      sig = vsignal_create( name, type, width, lsb, line, col, big_endian );
    } else {
      sig = (vsignal*)malloc_safe( sizeof( vsignal ), __FILE__, __LINE__ );
      vsignal_init( sig, strdup_safe( name, __FILE__, __LINE__ ), type,
                    (vector*)malloc_safe( sizeof( vector ), __FILE__, __LINE__ ), lsb, line, col, big_endian );
      vector_init( sig->value, NULL, width );
      if( (left != NULL) && (left->exp != NULL) ) {
        db_add_vector_param( sig, left->exp, PARAM_TYPE_SIG_MSB );
      }
      if( (right != NULL) && (right->exp != NULL) ) {
        db_add_vector_param( sig, right->exp, PARAM_TYPE_SIG_LSB );
      }
    }

    if( generate_mode > 0 ) {
      /* Add signal to current module's generate item list */
      gitem_link_add( gen_item_create_sig( sig ), &(curr_funit->gitem_head), &(curr_funit->gitem_tail) );
    } else {
      /* Add signal to current module's signal list */
      sig_link_add( sig, &(curr_funit->sig_head), &(curr_funit->sig_tail) );
    }

    /* Indicate if signal must be assigned by simulated results or not */
    if( mba ) {
      sig->value->suppl.part.mba      = 1;
      sig->value->suppl.part.assigned = 1;
    }

    /* Indicate signed attribute */
    sig->value->suppl.part.is_signed = is_signed;

  }
  
}

/*!
 \param name  String name of signal to find in current module.

 \return Returns pointer to the found signal.  If signal is not found, internal error.

 Searches current module for signal matching the specified name.  If the signal is
 found, returns a pointer to the calling function for that signal.
*/
vsignal* db_find_signal( char* name ) {

  vsignal   sig;   /* Temporary signal for comparison purposes */
  sig_link* sigl;  /* Temporary pointer to signal link element */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_find_signal, searching for signal %s", name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Create signal to find */
  sig.name = name;
  sigl = sig_link_find( &sig, curr_funit->sig_head );

  if( sigl == NULL ) {
    return( NULL );
  } else {
    return( sigl->sig );
  }

}

/*!
 \return Returns the number of signals in the current function unit.
*/
int db_curr_signal_count() {

  int       sig_cnt = 0;  /* Holds number of signals in the current functional unit */
  sig_link* sigl;         /* Pointer to current signal link */

  sigl = curr_funit->sig_head;
  while( sigl != NULL ) {
    sig_cnt++;
    sigl = sigl->next;
  }

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
expression* db_create_expression( expression* right, expression* left, int op, bool lhs, int line, int first, int last, char* sig_name ) {

  expression* expr;                 /* Temporary pointer to newly created expression */
  mod_parm*   mparm       = NULL;   /* Module parameter matching signal of current module */
  bool        sig_is_parm = FALSE;  /* Specifies if current signal is a module parameter */
  func_unit*  func_funit;           /* Pointer to function, if we are nested in one */
#ifdef DEBUG_MODE
  int         right_id;             /* ID of right expression */
  int         left_id;              /* ID of left expression */

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

#ifdef OBSOLETE
  /* Check to see if signal is a parameter in this module */
  if( sig_name != NULL ) {
    if( (mparm = funit_find_param( sig_name, curr_funit )) != NULL ) {
      sig_is_parm = TRUE;
      switch( op ) {
        case EXP_OP_SIG      :  op = EXP_OP_PARAM;           break;
        case EXP_OP_SBIT_SEL :  op = EXP_OP_PARAM_SBIT;      break;
        case EXP_OP_MBIT_SEL :  op = EXP_OP_PARAM_MBIT;      break;
        case EXP_OP_MBIT_POS :  op = EXP_OP_PARAM_MBIT_POS;  break;
        case EXP_OP_MBIT_NEG :  op = EXP_OP_PARAM_MBIT_NEG;  break;
        default :  
          assert( (op == EXP_OP_SIG) || (op == EXP_OP_SBIT_SEL) || (op == EXP_OP_MBIT_SEL) );
          break;
      }
#ifdef DEBUG_MODE
      snprintf( user_msg, USER_MSG_LENGTH, "  Switching to parameter operation: %d", op );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
    }
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
    snprintf( user_msg, USER_MSG_LENGTH, "Attempting to use a delay, task call, non-blocking assign or event controls in function %s, file %s, line %d", func_funit->name, curr_funit->filename, line );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    exit( 1 );
  }

  /* Create expression with next expression ID */
  expr = expression_create( right, left, op, lhs, curr_expr_id, line, first, last, FALSE );
  curr_expr_id++;

  /* If current functional unit is nested in a function, set the IN_FUNC supplemental field bit */
  expr->suppl.part.in_func = (func_funit != NULL) ? 1 : 0;

  /* Set right and left side expression's (if they exist) parent pointer to this expression */
  if( right != NULL ) {
    assert( right->parent->expr == NULL );
    right->parent->expr = expr;
  }

  if( (left != NULL) &&
      (expr->op != EXP_OP_CASE) &&
      (expr->op != EXP_OP_CASEX) &&
      (expr->op != EXP_OP_CASEZ) ) {
    assert( left->parent->expr == NULL );
    left->parent->expr = expr;
  }

  /* Add expression and signal to binding list */
  if( sig_name != NULL ) {

    switch( op ) {
      case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    sig_name, expr, curr_funit );  break;
      case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        sig_name, expr, curr_funit );  break;
      case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, sig_name, expr, curr_funit );  break;
      case EXP_OP_DISABLE   :  bind_add( 1,                 sig_name, expr, curr_funit );  break;
      default               :  bind_add( 0,                 sig_name, expr, curr_funit );  break;
    }

  }
 
  return( expr );

}

/*!
 \param root      Pointer to root expression to add to module expression list.

 Adds the specified expression to the current module's expression list.
*/
void db_add_expression( expression* root ) {

  if( root != NULL ) {

    if( exp_link_find( root, curr_funit->exp_head ) == NULL ) {
    
#ifdef DEBUG_MODE
      snprintf( user_msg, USER_MSG_LENGTH, "In db_add_expression, id: %d, op: %s, line: %d", 
                root->id, expression_string_op( root->op ), root->line );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

      if( generate_mode > 0 ) {

        /* Add root expression to the generate item list for the current functional unit */
        gitem_link_add( gen_item_create_expr( root ), &(curr_funit->gitem_head), &(curr_funit->gitem_tail) );

      } else {

        /* Add expression's children first. */
        db_add_expression( root->right );
        db_add_expression( root->left );

        /* Now add this expression to the list. */
        exp_link_add( root, &(curr_funit->exp_head), &(curr_funit->exp_tail) );

      }

    }

  }

  /* module_display_expressions( curr_funit ); */

}

/*!
 \param stmt  Pointer to statement block to parse.

 \return Returns expression tree to execute a sensitivity list for the given statement block.
*/
expression* db_create_sensitivity_list( statement* stmt ) {

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

  return( expc );

}


/*!
 \param stmt  Pointer to statement to check for parallelization

 \return Returns pointer to parallelized statement block
*/
statement* db_parallelize_statement( statement* stmt ) {

  expression* exp;  /* Expression containing FORK statement */

  /* If we are a parallel statement, create a FORK statement for this statement block */
  if( (stmt != NULL) && (fork_depth != -1) && (fork_block_depth[fork_depth] == block_depth) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_parallelize_statement, id: %d, %s, line: %d, fork_depth: %d, block_depth: %d, fork_block_depth: %d",
              stmt->exp->id, expression_string_op( stmt->exp->op ), stmt->exp->line, fork_depth, block_depth, fork_block_depth[fork_depth] );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Create a thread block for this statement block */
    stmt->exp->suppl.part.stmt_head      = 1;
    stmt->exp->suppl.part.stmt_is_called = 1;
    db_add_statement( stmt, stmt );

    /* Create FORK expression */
    exp = db_create_expression( NULL, NULL, EXP_OP_FORK, FALSE, stmt->exp->line, ((stmt->exp->col & 0xffff0000) >> 16), (stmt->exp->col & 0xffff), NULL );

    /* Bind the FORK expression to this statement */
    bind_add_stmt( stmt->exp->id, exp, curr_funit );

    /* Reduce fork and block depth for the new statement */
    fork_depth--;
    block_depth--;

    /* Create FORK statement and add the expression */
    stmt = db_create_statement( exp );
    db_add_expression( exp );

    /* Restore fork and block depth values for parser */
    fork_depth++;
    block_depth++;

  }

  return( stmt );

}

/*!
 \param exp  Pointer to associated "root" expression.

 \return Returns pointer to created statement.

 Creates an statement structure and adds created statement to current
 module's statement list.
*/
statement* db_create_statement( expression* exp ) {

  statement* stmt;  /* Pointer to newly created statement */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_create_statement, id: %d, line: %d", exp->id, exp->line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Create the given statement */
  stmt = statement_create( exp );

  /* If we are a parallel statement, create a FORK statement for this statement block */
  stmt = db_parallelize_statement( stmt );

  return( stmt );

}

/*!
 \param stmt   Pointer to statement add to current module's statement list.
 \param start  Pointer to starting statement of statement tree.

 Adds the specified statement tree to the tail of the current module's statement list.
 The start statement is specified to avoid infinite looping.
*/
void db_add_statement( statement* stmt, statement* start ) {
 
  if( (stmt != NULL) && (stmt->exp->suppl.part.stmt_added == 0) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_statement, id: %d, start id: %d", stmt->exp->id, start->exp->id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Add TRUE and FALSE statement paths to list */
    if( (ESUPPL_IS_STMT_STOP_FALSE( stmt->exp->suppl ) == 0) && (stmt->next_false != start) ) {
      db_add_statement( stmt->next_false, start );
    }

    if( (ESUPPL_IS_STMT_STOP_TRUE( stmt->exp->suppl ) == 0) && (stmt->next_true != stmt->next_false) && (stmt->next_true != start) ) {
      db_add_statement( stmt->next_true, start );
    }

    /* Set ADDED bit of this statement */
    stmt->exp->suppl.part.stmt_added = 1;

    /* Now add current statement */
    stmt_link_add_tail( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

  }

}

/*!
 \param stmt  Pointer to statement to remove from memory.

 Removes specified statement expression from the current functional unit.  Called by statement_dealloc_recursive in
 statement.c in its deallocation algorithm.
*/
void db_remove_statement_from_current_funit( statement* stmt ) {

  if( (stmt != NULL) && (stmt->exp != NULL) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement_from_current_module %s, stmt id: %d, %s, line: %d",
              curr_funit->name, stmt->exp->id, expression_string_op( stmt->exp->op ), stmt->exp->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Remove expression from any module parameter expression lists */
    mod_parm_find_expr_and_remove( stmt->exp, curr_funit->param_head );

    /* Remove expression from current module expression list and delete expressions */
    exp_link_remove( stmt->exp, &(curr_funit->exp_head), &(curr_funit->exp_tail), TRUE );

    /* Remove this statement link from the current module's stmt_link list */
    stmt_link_unlink( stmt, &(curr_funit->stmt_head), &(curr_funit->stmt_tail) );

  }

}

/*!
 \param stmt  Pointer to statement to remove from memory.

 Removes specified statement expression and its tree from current module expression list and deallocates
 both the expression and statement from heap memory.  Called when a statement structure is
 found to contain a statement that is not supported by Covered.
*/
void db_remove_statement( statement* stmt ) {

  if( stmt != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement, stmt id: %d, line: %d", 
              stmt->exp->id, stmt->exp->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Call the recursive statement deallocation function */
    statement_dealloc_recursive( stmt );

  }

}

/*!
 \param stmt       Pointer to statement to connect true path to.
 \param next_true  Pointer to statement to run if statement evaluates to TRUE.

 Connects the specified statement's true statement.
*/
void db_connect_statement_true( statement* stmt, statement* next_true ) {

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

}

/*!
 \param stmt        Pointer to statement to connect false path to.
 \param next_false  Pointer to statement to run if statement evaluates to FALSE.

 Connects the specified statement's false statement.
*/
void db_connect_statement_false( statement* stmt, statement* next_false ) {

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
bool db_statement_connect( statement* curr_stmt, statement* next_stmt ) {

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
              next_stmt->exp->line, curr_funit->filename );
    print_output( user_msg, WARNING, __FILE__, __LINE__ );

  }

  /* Increment stmt_conn_id for next statement connection */
  stmt_conn_id++;

  return( retval );

}

/*!
 \param name  Attribute parameter identifier.
 \param expr  Pointer to constant expression that is assigned to the identifier.

 \return Returns a pointer to the newly created attribute parameter.

 Calls the attribute_create() function and returns the pointer returned by this function.
*/
attr_param* db_create_attr_param( char* name, expression* expr ) {

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

  return( attr );

}

/*!
 \param ap  Pointer to attribute parameter list to parse.

 Calls the attribute_parse() function and deallocates this list.
*/
void db_parse_attribute( attr_param* ap ) {

#ifdef DEBUG_MODE
  print_output( "In db_parse_attribute", DEBUG, __FILE__, __LINE__ );
#endif

  /* First, parse the entire attribute */
  attribute_parse( ap, curr_funit );

  /* Then deallocate the structure */
  attribute_dealloc( ap );

}

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
exp_link* db_get_exprs_with_statement( statement* stmt ) {

  exp_link*   exp_head = NULL;  /* Pointer to head of expression list */
  exp_link*   exp_tail = NULL;  /* Pointer to tail of expression list */
  funit_link* funitl;           /* Pointer to current functional unit link being examined */
  exp_link*   expl;             /* Pointer to current expression link being examined */

  assert( stmt != NULL );
  
  funitl = funit_head;

  while( funitl != NULL ) {
    expl = funitl->funit->exp_head;
    while( expl != NULL ) {
      if( expl->exp->stmt == stmt ) {
        exp_link_add( expl->exp, &exp_head, &exp_tail );
      }
      expl = expl->next;
    }
    funitl = funitl->next;
  }

  return( exp_head );

}

/*!
 Synchronizes the curr_instance pointer to match the curr_inst_scope hierarchy.
*/
void db_sync_curr_instance() {
 
  char stripped_scope[4096];  /* Temporary string */

  assert( leading_hier_num > 0 );

  if( strcmp( leading_hierarchies[0], "*" ) != 0 ) {
    scope_extract_scope( curr_inst_scope, leading_hierarchies[0], stripped_scope );
  } else {
    strcpy( stripped_scope, curr_inst_scope );
  }

  if( stripped_scope[0] != '\0' ) {

    curr_instance = instance_find_scope( instance_root, stripped_scope );

    /* If we have found at least one matching instance, set the one_instance_found flag */
    if( curr_instance != NULL ) {
      one_instance_found = TRUE;
    }

  }

} 

/*!
 \param scope  Current VCD scope.

 Sets the curr_inst_scope global variable to the specified scope.
*/
void db_set_vcd_scope( char* scope ) {

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_vcd_scope, scope: %s", scope );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  assert( scope != NULL );

  if( curr_inst_scope == NULL ) {
    
    curr_inst_scope = (char*)malloc_safe( 4096, __FILE__, __LINE__ );
    strcpy( curr_inst_scope, scope );

  } else {
    
    strcat( curr_inst_scope, "." );
    strcat( curr_inst_scope, scope );

  }

  /* Synchronize the current instance to the value of curr_inst_scope */
  db_sync_curr_instance();

}

/*!
 Moves the curr_inst_scope up one level of hierarchy.  This function is called
 when the $upscope keyword is seen in a VCD file.
*/
void db_vcd_upscope() {

  char back[4096];   /* Lowest level of hierarchy */
  char rest[4096];   /* Hierarchy up one level */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_vcd_upscope, curr_inst_scope: %s", curr_inst_scope );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );  
#endif

  if( curr_inst_scope != NULL ) {

    scope_extract_back( curr_inst_scope, back, rest );

    if( rest[0] != '\0' ) {
      strcpy( curr_inst_scope, rest );
      db_sync_curr_instance();
    } else {
      free_safe( curr_inst_scope );
      curr_inst_scope = NULL;
    }

  }

}

/*!
 \param name    Name of signal to set value to.
 \param symbol  Symbol value of signal used in VCD dumpfile.
 \param msb     Most significant bit of symbol to set.
 \param lsb     Least significant bit of symbol to set.

 Creates a new entry in the symbol table for the specified signal and symbol.
*/
void db_assign_symbol( char* name, char* symbol, int msb, int lsb ) {

  sig_link* slink;   /* Pointer to signal containing this symbol */
  vsignal   tmpsig;  /* Temporary signal to search for */

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_assign_symbol, name: %s, symbol: %s, curr_inst_scope: %s, msb: %d, lsb: %d",
            name, symbol, curr_inst_scope, msb, lsb );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  assert( name != NULL );

  if( curr_instance != NULL ) {
    
    tmpsig.name = name;

    /* Find the signal that matches the specified signal name */
    if( (slink = sig_link_find( &tmpsig, curr_instance->funit->sig_head )) != NULL ) {

      /* Only add the symbol if we are not going to generate this value ourselves */
      if( slink->sig->value->suppl.part.assigned == 0 ) {

        /* Add this signal */
        symtable_add( symbol, slink->sig, msb, lsb );

      }

    }

  }

}

/*!
 \param sym    Name of symbol to set character value to.
 \param value  String version of value to set symbol table entry to.
 
 Searches the timestep symtable followed by the VCD symbol table searching for
 the symbol that matches the specified argument.  Once a symbol is found, its value
 parameter is set to the specified character.  If the symbol was found in the VCD
 symbol table, it is copied to the timestep symbol table.
*/
void db_set_symbol_char( char* sym, char value ) {

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

}

/*!
 \param sym    Name of symbol to set character value to.
 \param value  String version of value to set symbol table entry to.
 
 Searches the timestep symtable followed by the VCD symbol table searching for
 the symbol that matches the specified argument.  Once a symbol is found, its value
 parameter is set to the specified string.  If the symbol was found in the VCD
 symbol table, it is copied to the timestep symbol table.
*/
void db_set_symbol_string( char* sym, char* value ) {

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_string, sym: %s, value: %s", sym, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Set value of all matching occurrences in current timestep. */
  symtable_set_value( sym, value );

}

/*!
 \param time  Current time step value being performed.

 Cycles through expression queue, performing expression evaluations as we go.  If
 an expression has a parent expression, that parent expression is placed in the
 expression queue after that expression has completed its evaluation.  When the
 expression queue is empty, we are finished for this clock period.
*/
void db_do_timestep( int time ) {

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "Performing timestep #%d", time );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  curr_sim_time = time;

  if( (timestep_update > 0) && ((curr_sim_time - last_sim_update) >= timestep_update) && !debug_mode ) {
    last_sim_update = curr_sim_time;
    printf( "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bPerforming timestep %10d", curr_sim_time );
    fflush( stdout );
  }

  /* Simulate the current timestep */
  sim_simulate();

#ifdef DEBUG_MODE
  print_output( "Assigning postsimulation signals...", DEBUG, __FILE__, __LINE__ );
#endif

  /* Assign all stored values in current post-timestep to stored signals */
  symtable_assign();

}

/*!
 Handles the freeing of all allocated memory associated with the currently loaded design.
 This function is used when a design needs to be closed and a new one opened in the same
 process.
*/
void db_dealloc_global_vars() {

  if( curr_inst_scope != NULL ) {
    free_safe( curr_inst_scope );
  }

}

/*
 $Log$
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
