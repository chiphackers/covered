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
#include "module.h"
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


extern char*     top_module;
extern mod_inst* instance_root;
extern str_link* no_score_head;
extern mod_link* mod_head;
extern mod_link* mod_tail;
extern nibble    or_optab[16];
extern char      user_msg[USER_MSG_LENGTH];
extern bool      one_instance_found;
extern char      leading_hierarchy[4096];
extern bool      flag_scored;
extern int       timestep_update;
extern bool      debug_mode;

/*!
 Specifies the string Verilog scope that is currently specified in the VCD file.
*/
char* curr_inst_scope   = NULL;

/*!
 Pointer to the current instance selected by the VCD parser.  If this value is
 NULL, the current instance does not reside in the design specified for coverage.
*/
mod_inst* curr_instance = NULL;

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
 Pointer to the module structure for the module that is currently being parsed.
*/
module*   curr_module   = NULL;

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
 \param file        Name of database file to output contents to.
 \param parse_mode  Specifies if we are outputting parse data or score data.

 \return Returns TRUE if database write was successful; otherwise, returns FALSE.

 Opens specified database for writing.  If database open successful,
 iterates through module, expression and signal lists, displaying each
 to the database file.  If database write successful, returns TRUE; otherwise,
 returns FALSE to the calling function.
*/
bool db_write( char* file, bool parse_mode ) {

  bool  retval = TRUE;  /* Return value for this function         */
  FILE* db_handle;      /* Pointer to database file being written */

  if( (db_handle = fopen( file, "w" )) != NULL ) {

    /* Iterate through instance tree */
    assert( instance_root != NULL );
    info_db_write( db_handle );
    instance_db_write( instance_root, db_handle, instance_root->name, parse_mode );
    fclose( db_handle );

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Could not open %s for writing", file );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  /* Remove memory allocated for instance_root and mod_head */
  assert( instance_root->mod != NULL );
  instance_dealloc( instance_root, instance_root->name );
  mod_link_delete_list( mod_head );

  instance_root = NULL;
  mod_head      = NULL;
  mod_tail      = NULL;

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

  bool         retval = TRUE;        /* Return value for this function                 */
  FILE*        db_handle;            /* Pointer to database file being read            */
  int          type;                 /* Specifies object type                          */
  module       tmpmod;               /* Temporary module pointer                       */
  char*        curr_line;            /* Pointer to current line being read from db     */
  char*        rest_line;            /* Pointer to rest of the current line            */
  int          chars_read;           /* Number of characters currently read on line    */
  char         parent_scope[4096];   /* Scope of parent module to the current instance */
  char         back[4096];           /* Current module instance name                   */
  char         mod_scope[4096];      /* Current scope of module instance               */
  char         mod_name[256];        /* Current name of module instance                */
  char         mod_file[4096];       /* Current filename of module instance            */
  mod_link*    foundmod;             /* Found module link                              */
  mod_inst*    foundinst;            /* Found module instance                          */
  bool         merge_mode = FALSE;   /* If TRUE, we should currently be merging data   */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_read, file: %s, mode: %d", file, read_mode );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  /* Setup temporary module for storage */
  tmpmod.name     = mod_name;
  tmpmod.filename = mod_file;

  curr_module     = NULL;

  if( (db_handle = fopen( file, "r" )) != NULL ) {

    while( readline( db_handle, &curr_line ) && retval ) {

      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {

        rest_line = curr_line + chars_read;

        if( type == DB_TYPE_INFO ) {
          
          /* Parse rest of line for general info */
          retval = info_db_read( &rest_line );

          /* If we are in report mode and this CDD file has not been written bow out now */
          if( !flag_scored && 
              ((read_mode == READ_MODE_REPORT_NO_MERGE) ||
               (read_mode == READ_MODE_REPORT_MOD_MERGE)) ) {
            print_output( "Attempting to generate report on non-scored design.  Not supported.", FATAL, __FILE__, __LINE__ );
            retval = FALSE;
          }
          
        } else if( type == DB_TYPE_SIGNAL ) {

          assert( !merge_mode );

          /* Parse rest of line for signal info */
          retval = vsignal_db_read( &rest_line, curr_module );
	    
        } else if( type == DB_TYPE_EXPRESSION ) {

          assert( !merge_mode );

          /* Parse rest of line for expression info */
          retval = expression_db_read( &rest_line, curr_module, (read_mode == READ_MODE_MERGE_NO_MERGE) );

        } else if( type == DB_TYPE_STATEMENT ) {

          assert( !merge_mode );

          /* Parse rest of line for statement info */
          retval = statement_db_read( &rest_line, curr_module, read_mode );

        } else if( type == DB_TYPE_FSM ) {

          assert( !merge_mode );

          /* Parse rest of line for FSM info */
          retval = fsm_db_read( &rest_line, curr_module );

        } else if( type == DB_TYPE_RACE ) {

          assert( !merge_mode );

          /* Parse rest of line for race condition block info */
          retval = race_db_read( &rest_line, curr_module );

        } else if( type == DB_TYPE_MODULE ) {

          if( !merge_mode ) {

            /* Finish handling last module read from CDD file */
            if( curr_module != NULL ) {
              
              if( instance_root == NULL ) {
                
                instance_read_add( &instance_root, NULL, curr_module, mod_scope );
                
              } else {
                
                /* Add module to instance tree and module list */
                scope_extract_back( mod_scope, back, parent_scope );

                /* Make sure that module in database was not written before its parent module */
                assert( instance_find_scope( instance_root, parent_scope ) != NULL );

                /* Add module to instance tree and module list */
                instance_read_add( &instance_root, parent_scope, curr_module, back );
                
              }
              
              mod_link_add( curr_module, &mod_head, &mod_tail );
              curr_module = NULL;

            }

          } else {

            merge_mode = FALSE;

          }

          /* Now finish reading module line */
          if( (retval = module_db_read( &tmpmod, mod_scope, &rest_line )) == TRUE ) {
            
            if( (read_mode == READ_MODE_MERGE_INST_MERGE) && ((foundinst = instance_find_scope( instance_root, mod_scope )) != NULL) ) {
              merge_mode = TRUE;
              module_db_merge( foundinst->mod, db_handle, TRUE );
            } else if( read_mode == READ_MODE_REPORT_MOD_REPLACE ) {
              if( (foundmod = mod_link_find( &tmpmod, mod_head )) != NULL ) {
                merge_mode = TRUE;
                /*
                 If this module has been assigned a stat, remove it and replace it with the new module contents;
                 otherwise, merge the results of the new module with the old.
                */
                if( foundmod->mod->stat != NULL ) {
                  statistic_dealloc( foundmod->mod->stat );
                  foundmod->mod->stat = NULL;
                  module_db_replace( foundmod->mod, db_handle );
                } else {
                  module_db_merge( foundmod->mod, db_handle, FALSE );
                }
              } else {
                retval = FALSE;
              }
            } else if( (read_mode == READ_MODE_REPORT_MOD_MERGE) && ((foundmod = mod_link_find( &tmpmod, mod_head )) != NULL) ) {
              merge_mode = TRUE;
              module_db_merge( foundmod->mod, db_handle, FALSE );
            } else {
              curr_module             = module_create();
              curr_module->name       = strdup_safe( mod_name, __FILE__, __LINE__ );
              curr_module->filename   = strdup_safe( mod_file, __FILE__, __LINE__ );
              curr_module->start_line = tmpmod.start_line;
              curr_module->end_line   = tmpmod.end_line;
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

  /* If the last module was being read and not merged, add it now */
  if( !merge_mode && (curr_module != NULL) ) {

    if( instance_root == NULL ) {
      
      instance_read_add( &instance_root, NULL, curr_module, mod_scope );
      
    } else {
      
      /* Add module to instance tree and module list */
      scope_extract_back( mod_scope, back, parent_scope );
    
      /* Make sure that module in database not written before its parent module */
      if( instance_find_scope( instance_root, parent_scope ) != NULL ) {

        /* Add module to instance tree and module list */
        instance_read_add( &instance_root, parent_scope, curr_module, back );

      } else {

        print_output( "CDD file is not related to currently opened CDD file", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
 
      }
      
    }
    
    mod_link_add( curr_module, &mod_head, &mod_tail );
    curr_module = NULL;

  }

  return( retval );

}

/*!
 \param scope    Name of module node instance being added.
 \param modname  Name of module being instantiated.
 
 Creates a new module_node with the instantiation name, search for matching module.  If
 module hasn't been created previously, create it now without a filename associated (NULL).
 Add module_node to tree if there are no problems in doing so.
*/
void db_add_instance( char* scope, char* modname ) {

  module*   mod;             /* Pointer to module                        */
  mod_link* found_mod_link;  /* Pointer to found mod_link in module list */

  /* There should always be a parent so internal error if it does not exist. */
  assert( curr_module != NULL );

  /* If this module name is in our list of no_score modules, skip adding the instance */
  if( str_link_find( modname, no_score_head ) == NULL ) {

    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_instance, instance: %s, module: %s", scope, modname );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    /* Create new module node */
    mod       = module_create();
    mod->name = strdup_safe( modname, __FILE__, __LINE__ );

    if( (found_mod_link = mod_link_find( mod, mod_head )) != NULL ) {

      instance_parse_add( &instance_root, curr_module, found_mod_link->mod, scope );

      module_dealloc( mod );

    } else {

      /* Add new module to module list. */
      mod_link_add( mod, &mod_head, &mod_tail );

      /* Add instance. */
      instance_parse_add( &instance_root, curr_module, mod, scope );

      if( str_link_find( modname, modlist_head ) == NULL ) {
        str_link_add( modname, &modlist_head, &modlist_tail );
      }
      
    }

  }

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

  module    mod;   /* Temporary module for comparison */
  mod_link* modl;  /* Pointer to found tree node      */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_module, module: %s, file: %s, start_line: %d", name, file, start_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  /* Set current module to this module */
  mod.name = name;

  modl = mod_link_find( &mod, mod_head );

  assert( modl != NULL );

  curr_module             = modl->mod;
  curr_module->filename   = strdup_safe( file, __FILE__, __LINE__ );
  curr_module->start_line = start_line;
  
}

/*!
 \param end_line  Ending line number of specified module in file.

 Updates the modlist for parsing purposes.
*/
void db_end_module( int end_line ) {

  snprintf( user_msg, USER_MSG_LENGTH, "In db_end_module, end_line: %d", end_line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  curr_module->end_line = end_line;

  str_link_remove( curr_module->name, &modlist_head, &modlist_tail );

  /* mod_parm_display( curr_module->param_head ); */
  
}

/*!
 \param name  Name of declared parameter to add.
 \param expr  Expression containing value of this parameter.

 Searches current module to verify that specified parameter name has not been previously
 used in the module.  If the parameter name has not been found, it is created added to
 the current module's parameter list.
*/
void db_add_declared_param( char* name, expression* expr ) {

  char      scope[4096];  /* String containing current instance scope */
  mod_inst* inst;         /* Pointer to found module instance         */
  int       ignore;       /* Number of matching modules to ignore     */
  int       i;            /* Loop iterator                            */
  mod_parm* mparm;        /* Pointer to added module parameter        */

  assert( name != NULL );

  /* If a parameter value type is not supported, don't create this parameter */
  if( expr != NULL ) {

    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_declared_param, param: %s, expr: %d", name, expr->id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    if( mod_parm_find( name, curr_module->param_head ) == NULL ) {

      /* Add parameter to module parameter list */
      mparm = mod_parm_add( name, expr, PARAM_TYPE_DECLARED, &(curr_module->param_head), &(curr_module->param_tail) );

      /* Also add this to all associated instance parameter lists */
      i      = 0;
      ignore = 0;
      while( (inst = instance_find_by_module( instance_root, curr_module, &ignore )) != NULL ) {

        /* Reset scope */
        scope[0] = '\0';

        /* Find scope for current instance */
        instance_gen_scope( scope, inst );

        if( inst->parent == NULL ) {
          param_resolve_declared( scope, mparm, NULL, &(inst->param_head), &(inst->param_tail) );
        } else {
          param_resolve_declared( scope, mparm, inst->parent->param_head, &(inst->param_head), &(inst->param_tail) );
        }

        i++;
        ignore = i;

      }

    }

  }

}

/*!
 \param inst_name  Name of instance being overriddent.
 \param expr       Expression containing value of override parameter.

 Creates override parameter and stores this in the current module as well
 as all associated instances.
*/
void db_add_override_param( char* inst_name, expression* expr ) {

  mod_parm* mparm;   /* Pointer to module parameter added to current module */
  mod_inst* inst;    /* Pointer to current instance to add parameter to     */
  int       ignore;  /* Specifies how many matching instances to ignore     */
  int       i;       /* Loop iterator                                       */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_override_param, instance: %s", inst_name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  /* Add override parameter to module parameter list */
  mparm = mod_parm_add( inst_name, expr, PARAM_TYPE_OVERRIDE, &(curr_module->param_head), &(curr_module->param_tail) );

  /* Also add this to all associated instance parameter lists */
  i      = 0;
  ignore = 0;
  while( (inst = instance_find_by_module( instance_root, curr_module, &ignore )) != NULL ) {

    param_resolve_override( mparm, &(inst->param_head), &(inst->param_tail) );

    i++;
    ignore = i;

  }

}

/*!
 \param sig       Pointer to signal to attach parameter to.
 \param parm_exp  Expression containing value of vector parameter.
 \param type      Type of signal vector parameter to create (LSB or MSB).

 Creates a vector parameter for the specified signal or expression with the specified
 parameter expression.  This function is called by the parser.
*/
void db_add_vector_param( vsignal* sig, expression* parm_exp, int type ) {

  mod_parm* mparm;   /* Holds newly created module parameter                        */
  mod_inst* inst;    /* Pointer to instance that is found to contain current module */
  int       i;       /* Loop iterator                                               */
  int       ignore;  /* Number of matching instances to ignore before selecting     */

  assert( sig != NULL );
  assert( (type == PARAM_TYPE_SIG_LSB) || (type == PARAM_TYPE_SIG_MSB) );

  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_vector_param, signal: %s, type: %d", sig->name, type );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  /* Add signal vector parameter to module parameter list */
  mparm = mod_parm_add( NULL, parm_exp, type, &(curr_module->param_head), &(curr_module->param_tail) );

  /* Add signal to module parameter list */
  mparm->sig = sig;

  /* Also add this to all associated instance parameter lists */
  i      = 0;
  ignore = 0;
  while( (inst = instance_find_by_module( instance_root, curr_module, &ignore )) != NULL ) {

    param_resolve_override( mparm, &(inst->param_head), &(inst->param_tail) );

    i++;
    ignore = i;

  }

}

/*!
 \param name  Name of parameter value to override.
 \param expr  Expression value of parameter override.

 Adds specified parameter to the defparam list.
*/
void db_add_defparam( char* name, expression* expr ) {

  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_defparam, defparam: %s", name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  snprintf( user_msg, USER_MSG_LENGTH, "defparam construct is not supported, line: %d.  Use -P option to score instead", expr->line );
  print_output( user_msg, WARNING, __FILE__, __LINE__ );

  expression_dealloc( expr, FALSE );

}

/*!
 \param name    Name of signal being added.
 \param left    Specifies constant value for calculation of left-hand vector value.
 \param right   Specifies constant value for calculation of right-hand vector value.
 \param inport  Set to 1 if specified signal name is an input port.

 Creates a new signal with the specified parameter information and adds this
 to the signal list if it does not already exist.  If width == 0, the sig_msb
 and sig_lsb values are interrogated.  If sig_msb and/or is non-NULL, its value is
 add to the current module's parameter list and all associated instances are
 updated to contain new value.
*/
void db_add_signal( char* name, static_expr* left, static_expr* right, int inport ) {

  vsignal  tmpsig;  /* Temporary signal for signal searching */
  vsignal* sig;     /* Container for newly created signal    */
  int      lsb;     /* Signal LSB                            */
  int      width;   /* Signal width                          */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_add_signal, signal: %s", name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  tmpsig.name = name;

  /* Add signal to current module's signal list if it does not already exist */
  if( sig_link_find( &tmpsig, curr_module->sig_head ) == NULL ) {

    static_expr_calc_lsb_and_width( left, right, &width, &lsb );

    /* Check to make sure that signal width does not exceed maximum size */
    if( width > MAX_BIT_WIDTH ) {
      snprintf( user_msg, USER_MSG_LENGTH, "Signal (%s) width (%d) exceeds maximum allowed by Covered (%d).  Ignoring...", name, width, MAX_BIT_WIDTH );
      print_output( user_msg, WARNING, __FILE__, __LINE__ );
      width = -1;
      left  = NULL;
      right = NULL;
    }  

    if( (lsb != -1) && (width != -1) ) { 
      sig = vsignal_create( name, width, lsb );
    } else {
      sig = (vsignal*)malloc_safe( sizeof( vsignal ), __FILE__, __LINE__ );
      vsignal_init( sig, strdup_safe( name, __FILE__, __LINE__ ), (vector*)malloc_safe( sizeof( vector ), __FILE__, __LINE__ ), lsb );
      sig->value->width = width;      
      sig->value->value = NULL;
      if( (left != NULL) && (left->exp != NULL) ) {
        db_add_vector_param( sig, left->exp, PARAM_TYPE_SIG_MSB );
      }
      if( (right != NULL) && (right->exp != NULL) ) {
        db_add_vector_param( sig, right->exp, PARAM_TYPE_SIG_LSB );
      }
    }

    /* Add signal to current module's signal list */
    sig_link_add( sig, &(curr_module->sig_head), &(curr_module->sig_tail) );

    /* Indicate if signal is an input port or not */
    sig->value->suppl.part.inport = inport;

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

  snprintf( user_msg, USER_MSG_LENGTH, "In db_find_signal, searching for signal %s", name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  /* Create signal to find */
  sig.name = name;
  sigl = sig_link_find( &sig, curr_module->sig_head );

  if( sigl == NULL ) {
    return( NULL );
  } else {
    return( sigl->sig );
  }

}

int db_curr_signal_count() {

  int       sig_cnt = 0;  /* Holds number of signals in the current module */
  sig_link* sigl;         /* Pointer to current signal link                */

  sigl = curr_module->sig_head;
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

  expression* expr;                 /* Temporary pointer to newly created expression      */
  int         right_id;             /* ID of right expression                             */
  int         left_id;              /* ID of left expression                              */
  mod_parm*   mparm       = NULL;   /* Module parameter matching signal of current module */
  bool        sig_is_parm = FALSE;  /* Specifies if current signal is a module parameter  */

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

  snprintf( user_msg, USER_MSG_LENGTH, "In db_create_expression, right: %d, left: %d, id: %d, op: %d, lhs: %d, line: %d, first: %d, last: %d", 
                       right_id, left_id, curr_expr_id, op, lhs, line, first, last );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  /* Check to see if signal is a parameter in this module */
  if( sig_name != NULL ) {
    if( (mparm = mod_parm_find( sig_name, curr_module->param_head )) != NULL ) {
      sig_is_parm = TRUE;
      switch( op ) {
        case EXP_OP_SIG      :  op = EXP_OP_PARAM;       break;
        case EXP_OP_SBIT_SEL :  op = EXP_OP_PARAM_SBIT;  break;
        case EXP_OP_MBIT_SEL :  op = EXP_OP_PARAM_MBIT;  break;
        default :  
          assert( (op == EXP_OP_SIG) || (op == EXP_OP_SBIT_SEL) || (op == EXP_OP_MBIT_SEL) );
          break;
      }
      snprintf( user_msg, USER_MSG_LENGTH, "  Switching to parameter operation: %d", op );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
  }

  /* Create expression with next expression ID */
  expr = expression_create( right, left, op, lhs, curr_expr_id, line, first, last, FALSE );
  curr_expr_id++;

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

    /* Check to see if we are attaching to a parameter */
    if( sig_is_parm ) {

      /* Add to module parameter list */
      exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

    } else {

      /* If signal is located in this current module, bind now; else, bind later */
      if( scope_local( sig_name ) ) {
        if( !bind_perform( sig_name, expr, curr_module, curr_module, TRUE, FALSE ) ) {
          expression_dealloc( expr, FALSE );
          expr = NULL;
        }
      } else {
        bind_add( sig_name, expr, curr_module );
      }

    }

  } else {

#ifdef PERFORM_ASSIGNMENT
    /*
     If this is a blocking assignment, set the assigned vector attribute in all signals to the
     left of the blocking assignment operator to TRUE.
    */
    if( op == EXP_OP_BASSIGN ) {
      expression_set_assigned( expr->left );
    }
#endif

  }
 
  return( expr );

}

/*!
 \param root      Pointer to root expression to add to module expression list.

 Adds the specified expression to the current module's expression list.
*/
void db_add_expression( expression* root ) {

  if( root != NULL ) {

    if( exp_link_find( root, curr_module->exp_head ) == NULL ) {
    
      snprintf( user_msg, USER_MSG_LENGTH, "In db_add_expression, id: %d, op: %d, line: %d", 
                root->id, root->op, root->line );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );

      /* Add expression's children first. */
      db_add_expression( root->right );
      db_add_expression( root->left );

      /* Now add this expression to the list. */
      exp_link_add( root, &(curr_module->exp_head), &(curr_module->exp_tail) );

    }

  }

  /* module_display_expressions( curr_module ); */

}

/*!
 \param exp  Pointer to associated "root" expression.

 \return Returns pointer to created statement.

 Creates an statement structure and adds created statement to current
 module's statement list.
*/
statement* db_create_statement( expression* exp ) {

  statement* stmt;  /* Pointer to newly created statement */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_create_statement, id: %d, line: %d", exp->id, exp->line );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  stmt = statement_create( exp );

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

    snprintf( user_msg, USER_MSG_LENGTH, "In db_add_statement, id: %d, start id: %d", stmt->exp->id, start->exp->id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    /* Add TRUE and FALSE statement paths to list */
    if( (ESUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0) && (stmt->next_false != start) ) {
      db_add_statement( stmt->next_false, start );
    }

    if( (stmt->next_true != stmt->next_false) && (stmt->next_true != start) ) {
      db_add_statement( stmt->next_true, start );
    }

    /* Set ADDED bit of this statement */
    stmt->exp->suppl.part.stmt_added = 1;

    /* Now add current statement */
    stmt_link_add_tail( stmt, &(curr_module->stmt_head), &(curr_module->stmt_tail) );

  }

}

/*!
 \param stmt  Pointer to statement to remove from memory.

 Removes specified statement expression from the current module.  Called by statement_dealloc_recursive in
 statement.c in its deallocation algorithm.
*/
void db_remove_statement_from_current_module( statement* stmt ) {

  if( (stmt != NULL) && (stmt->exp != NULL) ) {

    snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement_from_current_module, stmt id: %d", stmt->exp->id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    /* Remove expression from any module parameter expression lists */
    mod_parm_find_expr_and_remove( stmt->exp, curr_module->param_head );

    /* Remove expression from current module expression list and delete expressions */
    exp_link_remove( stmt->exp, &(curr_module->exp_head), &(curr_module->exp_tail), TRUE );

    /* Remove this statement link from the current module's stmt_link list */
    stmt_link_unlink( stmt, &(curr_module->stmt_head), &(curr_module->stmt_tail) );

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

    snprintf( user_msg, USER_MSG_LENGTH, "In db_remove_statement, stmt id: %d, line: %d", 
              stmt->exp->id, stmt->exp->line );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

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

  int  next_id;  /* Statement ID of next TRUE statement */

  if( stmt != NULL ) {

    if( next_true == NULL ) {
      next_id = 0;
    } else {
      next_id = next_true->exp->id;
    }

    snprintf( user_msg, USER_MSG_LENGTH, "In db_connect_statement_true, id: %d, next: %d", stmt->exp->id, next_id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    stmt->next_true = next_true;

  }

}

/*!
 \param stmt        Pointer to statement to connect false path to.
 \param next_false  Pointer to statement to run if statement evaluates to FALSE.

 Connects the specified statement's false statement.
*/
void db_connect_statement_false( statement* stmt, statement* next_false ) {

  int  next_id;  /* Statement ID of next FALSE statement */

  if( stmt != NULL ) {

    if( next_false == NULL ) {
      next_id = 0;
    } else {
      next_id = next_false->exp->id;
    }

    snprintf( user_msg, USER_MSG_LENGTH, "In db_connect_statement_false, id: %d, next: %d", stmt->exp->id, next_id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );

    stmt->next_false = next_false;

  }

}

/*!
 \param curr_stmt  Pointer to current statement to attach.
 \param next_stmt  Pointer to next statement to attach to.

 Calls the statement_connect function located in statement.c with the specified parameters.
*/
void db_statement_connect( statement* curr_stmt, statement* next_stmt ) {

  int  curr_id;     /* Current statement ID       */
  int  next_id;     /* Next statement ID          */

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

  statement_connect( curr_stmt, next_stmt );

}

/*!
 \param stmt  Pointer to statement tree to traverse.
 \param post  Pointer to statement which stopped statements will be connected to.
 \param both  If TRUE, causes both true and false paths to set stop bits if
              connected to post statement.

 Calls the statement_set_stop function with the specified parameters.  This function is
 called by the parser after the call to db_statement_connect.
*/
void db_statement_set_stop( statement* stmt, statement* post, bool both ) {

  int  stmt_id;  /* Current statement ID    */
  int  post_id;  /* Statement ID after stop */

  if( stmt != NULL ) {

    stmt_id = stmt->exp->id;

    if( post == NULL ) {
      post_id = 0;
    } else {
      post_id = post->exp->id;
    }

    snprintf( user_msg, USER_MSG_LENGTH, "In db_statement_set_stop, stmt: %d, next_stmt: %d", stmt_id, post_id );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
 
    statement_set_stop( stmt, post, TRUE, both );

  }

}

/*!
 \param name  Attribute parameter identifier.
 \param expr  Pointer to constant expression that is assigned to the identifier.

 \return Returns a pointer to the newly created attribute parameter.

 Calls the attribute_create() function and returns the pointer returned by this function.
*/
attr_param* db_create_attr_param( char* name, expression* expr ) {

  attr_param* attr;  /* Pointer to newly allocated/initialized attribute parameter */

  if( expr != NULL ) {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_create_attr_param, name: %s, expr: %d", name, expr->id );
  } else {
    snprintf( user_msg, USER_MSG_LENGTH, "In db_create_attr_param, name: %s", name );
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  attr = attribute_create( name, expr );

  return( attr );

}

/*!
 \param ap  Pointer to attribute parameter list to parse.

 Calls the attribute_parse() function and deallocates this list.
*/
void db_parse_attribute( attr_param* ap ) {

  print_output( "In db_parse_attribute", DEBUG, __FILE__, __LINE__ );

  /* First, parse the entire attribute */
  attribute_parse( ap, curr_module );

  /* Then deallocate the structure */
  attribute_dealloc( ap );

}

/*!
 \param scope  Current VCD scope.

 Sets the curr_inst_scope global variable to the specified scope.
*/
void db_set_vcd_scope( char* scope ) {

  char stripped_scope[4096];  /* Temporary stripped scope */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_vcd_scope, scope: %s", scope );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  assert( scope != NULL );

  if( curr_inst_scope == NULL ) {
    
    curr_inst_scope = (char*)malloc_safe( 4096, __FILE__, __LINE__ );
    strcpy( curr_inst_scope, scope );

  } else {
    
    strcat( curr_inst_scope, "." );
    strcat( curr_inst_scope, scope );

  }
    
  if( strcmp( leading_hierarchy, "*" ) != 0 ) {
    scope_extract_scope( curr_inst_scope, leading_hierarchy, stripped_scope );
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
 Moves the curr_inst_scope up one level of hierarchy.  This function is called
 when the $upscope keyword is seen in a VCD file.
*/
void db_vcd_upscope() {

  char back[4096];   /* Lowest level of hierarchy */
  char rest[4096];   /* Hierarchy up one level    */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_vcd_upscope, curr_inst_scope: %s", curr_inst_scope );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );  

  if( curr_inst_scope != NULL ) {

    scope_extract_back( curr_inst_scope, back, rest );

    if( rest[0] != '\0' ) {
      strcpy( curr_inst_scope, rest );
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
  vsignal   tmpsig;  /* Temporary signal to search for           */

  snprintf( user_msg, USER_MSG_LENGTH, "In db_assign_symbol, name: %s, symbol: %s, curr_inst_scope: %s, msb: %d, lsb: %d",
            name, symbol, curr_inst_scope, msb, lsb );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  assert( name != NULL );

  if( curr_instance != NULL ) {
    
    tmpsig.name = name;

    /* Find the signal that matches the specified signal name */
    if( (slink = sig_link_find( &tmpsig, curr_instance->mod->sig_head )) != NULL ) {

      /* Only add the symbol if we are not going to generate this value ourselves */
      if( slink->sig->value->suppl.part.assigned == 0 ) {

        /* Add this signal */
        symtable_add( strdup_safe( symbol, __FILE__, __LINE__ ), slink->sig, msb, lsb );

      }

    } else {

      snprintf( user_msg, USER_MSG_LENGTH, "VCD signal \"%s.%s\" found that is not part of design", curr_inst_scope, name );
      print_output( user_msg, WARNING, __FILE__, __LINE__ );

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

  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_char, sym: %s, value: %c", sym, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

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

  snprintf( user_msg, USER_MSG_LENGTH, "In db_set_symbol_string, sym: %s, value: %s", sym, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

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

  snprintf( user_msg, USER_MSG_LENGTH, "Performing timestep #%d", time );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  curr_sim_time = time;

  if( (timestep_update > 0) && ((curr_sim_time - last_sim_update) >= timestep_update) && !debug_mode ) {
    last_sim_update = curr_sim_time;
    printf( "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bPerforming timestep %10d", curr_sim_time );
    fflush( stdout );
  }

  /* Assign all stored values in current pre-timestep to stored signals */
  print_output( "Assigning presimulation signals...", DEBUG, __FILE__, __LINE__ );
  symtable_assign( TRUE );

  /* Simulate the current timestep */
  sim_simulate();

  /* Assign all stored values in current post-timestep to stored signals */
  print_output( "Assigning postsimulation signals...", DEBUG, __FILE__, __LINE__ );
  symtable_assign( FALSE );

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

  // instance_dealloc( curr_instance );

}

/*
 $Log$
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
