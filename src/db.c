/*!
 \file     db.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "db.h"
#include "util.h"
#include "module.h"
#include "expr.h"
#include "signal.h"
#include "link.h"
#include "symtable.h"
#include "instance.h"


extern char*      top_module;

extern mod_inst* instance_root;
char*            curr_vcd_scope = NULL;

extern mod_link* mod_head;
extern mod_link* mod_tail;

str_link* modlist_head      = NULL;
str_link* modlist_tail      = NULL;

module*   curr_module       = NULL;

symtable* vcd_symtab        = NULL;
symtable* timestep_tab      = NULL;

exp_link* exp_queue_head    = NULL;
exp_link* exp_queue_tail    = NULL;

/*!
 This static value contains the current expression ID number to use for the next expression found, it
 is incremented by one when an expression is found.  This allows us to have a unique expression ID
 for each expression (since expressions have no intrinsic names).
*/
int       curr_expr_id  = 1;

/*!
 \param file  Name of database file to output contents to.

 \return Returns TRUE if database write was successful; otherwise, returns FALSE.

 Opens specified database for writing.  If database open successful,
 iterates through module, expression and signal lists, displaying each
 to the database file.  If database write successful, returns TRUE; otherwise,
 returns FALSE to the calling function.
*/
bool db_write( char* file ) {

  bool         retval = TRUE;  /* Return value for this function         */
  FILE*        db_handle;      /* Pointer to database file being written */
  exp_link*    ecur;           /* Pointer to current expression link     */
  sig_link*    scur;           /* Pointer to current signal link         */
  char         msg[256];       /* Error message to display               */

  if( (db_handle = fopen( file, "w" )) != NULL ) {

    /* Iterate through instance tree */
    assert( instance_root != NULL );
    instance_db_write( instance_root, db_handle, instance_root->name );
    fclose( db_handle );

  } else {

    snprintf( msg, 256, "Could not open %s for writing", file );
    print_output( msg, FATAL );
    retval = FALSE;

  }

  /* Remove memory allocated for instance_root and mod_head */
  instance_dealloc( instance_root, instance_root->mod->name );
  mod_link_delete_list( mod_head );

  instance_root = NULL;
  mod_head      = NULL;
  mod_tail      = NULL;

  return( retval );

}

/*!
 \param file       Name of database file to read contents from.
 \param read_mode  Specifies what to do with read data.  Values are
                   - 0 = Instance, no merge
                   - 1 = Instance, merge
                   - 2 = Module, merge

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
  char         msg[4096];            /* Error message string                           */
  module*      tmpmod;               /* Temporary module pointer                       */
  char*        curr_line;            /* Pointer to current line being read from db     */
  char*        rest_line;            /* Pointer to rest of the current line            */
  int          chars_read;           /* Number of characters currently read on line    */
  char         parent_scope[4096];   /* Scope of parent module to the current instance */
  char         back[4096];           /* Current module instance name                   */
  mod_link*    foundmod;             /* Found module link                              */
  mod_inst*    foundinst;            /* Found module instance                          */

  curr_module = NULL;

  if( (db_handle = fopen( file, "r" )) != NULL ) {

    while( readline( db_handle, &curr_line ) && retval ) {

      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {

        rest_line = curr_line + chars_read;

        if( type == DB_TYPE_SIGNAL ) {

          /* Parse rest of line for signal info */
          retval = signal_db_read( &rest_line, curr_module );
	    
        } else if( type == DB_TYPE_EXPRESSION ) {

          /* Parse rest of line for expression info */
          retval = expression_db_read( &rest_line, curr_module );

        } else if( type == DB_TYPE_MODULE ) {

          /* Parse rest of line for module info */
          if( retval = module_db_read( &tmpmod, &rest_line ) ) {

            assert( tmpmod != NULL );
            assert( tmpmod->scope != NULL );
            assert( tmpmod->name != NULL );

            if( curr_module != NULL ) {

              if( read_mode == READ_MODE_INST_MERGE ) {
              
                /* Find module in instance tree and do a module merge */
                if( (foundinst = instance_find_scope( instance_root, tmpmod->scope )) == NULL ) {
                  print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
                  retval = FALSE;
                } else {
                  /* Perform module merge */
                  module_merge( foundinst->mod, curr_module );
                }

              } else if( read_mode == READ_MODE_MOD_MERGE ) {

                if( (foundmod = mod_link_find( curr_module, mod_head )) == NULL ) {
                  mod_link_add( curr_module, &mod_head, &mod_tail );
                } else {
                  module_merge( foundmod->mod, curr_module );
                }

              }

              module_dealloc( curr_module );

            }

            curr_module = tmpmod;

            if( read_mode == READ_MODE_NO_MERGE ) {

	      /* Add module to instance tree and module list */
              scope_extract_back( tmpmod->scope, back, parent_scope );

              if( (parent_scope[0] != '\0') && ((foundinst = instance_find_scope( instance_root, parent_scope )) == NULL) ) {

                print_output( "Internal error:  module in database written before its parent module", FATAL );
                retval = FALSE;

              } else {

                /* Add module to instance tree and module list */
                instance_add( &instance_root, parent_scope, curr_module, back );
                mod_link_add( curr_module, &mod_head, &mod_tail );
              
              }

            }

	  }

        } else {

          snprintf( msg, 4096, "Unexpected type %d when parsing database file %s", type, file );
          print_output( msg, FATAL );
          retval = FALSE;

        }

      } else {

        snprintf( msg, 4096, "Unexpected line in database file %s", file );
        print_output( msg, FATAL );
        retval = FALSE;

      }

      free_safe( curr_line );

    }

    if( read_mode == READ_MODE_INST_MERGE ) {
              
      /* Find module in instance tree and do a module merge */
      if( (foundinst = instance_find_scope( instance_root, tmpmod->scope )) == NULL ) {
        print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
        retval = FALSE;
      } else {
        /* Perform module merge */
        module_merge( foundinst->mod, curr_module );
      }

    } else if( read_mode == READ_MODE_MOD_MERGE ) {

      if( (foundmod = mod_link_find( curr_module, mod_head )) == NULL ) {
        mod_link_add( curr_module, &mod_head, &mod_tail );
      } else {
        module_merge( foundmod->mod, curr_module );
      }

    }

    curr_module = NULL;

    fclose( db_handle );

  } else {

    snprintf( msg, 4096, "Could not open %s for reading", file );
    print_output( msg, FATAL );
    retval = FALSE;

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
  char      msg[4096];       /* Display message string                   */
  str_link* mod_in_list;     /* Pointer to found module name in modlist  */

  /* There should always be a parent so internal error if it does not exist. */
  assert( curr_module != NULL );

  snprintf( msg, 4096, "In db_add_instance, instance: %s, module: %s", scope, modname );
  print_output( msg, NORMAL );

  /* Create new module node */
  mod       = module_create();
  mod->name = strdup( modname );

  if( (found_mod_link = mod_link_find( mod, mod_head )) != NULL ) {

    instance_add( &instance_root, curr_module->name, found_mod_link->mod, scope );

    module_dealloc( mod );

  } else {

    // Add new module to module list.
    mod_link_add( mod, &mod_head, &mod_tail );

    // Add instance.
    instance_add( &instance_root, curr_module->name, mod, scope );

  }

  if( (mod_in_list = str_link_find( modname, modlist_head )) == NULL ) {
    str_link_add( modname, &modlist_head, &modlist_tail );
  }

}

/*!
 \param name   Name of module being added to tree.
 \param file   Filename that module is a part of.

 Creates a new module element with the contents specified by the parameters given
 and inserts this module into the module list.  This function can only be called when we
 are actually parsing a module which implies that we must have the name of the module
 at the head of the modlist linked-list structure.
*/
void db_add_module( char* name, char* file ) {

  module*   mod;            /* Pointer to newly created module */
  mod_link* modl;          /* Pointer to found tree node      */
  char      msg[4096];      /* Display message string          */

  snprintf( msg, 4096, "In db_add_module, module: %s, file: %s", name, file );
  print_output( msg, NORMAL );

  /* Make sure that modlist_head name is the same as the specified name */
  assert( strcmp( name, modlist_head->str ) == 0 );

  /* Set current module to this module */
  mod               = module_create();
  mod->name         = strdup( name );

  modl = mod_link_find( mod, mod_head );

  assert( modl != NULL );

  curr_module           = modl->mod;
  curr_module->filename = strdup( file );
  
  module_dealloc( mod );

}

/*!
 Updates the modlist for parsing purposes.
*/
void db_end_module() {

  str_link* str;    /* Temporary pointer to current modlist entry at head */

  print_output( "In db_end_module", NORMAL );

  str = modlist_head;

  /* Update modlist head */
  modlist_head = modlist_head->next;

  /* Remove old head */
  free_safe( str->str );
  free_safe( str );
  
}

/*!
 \param name       Name of signal being added.
 \param width      Bit width of signal being added.
 \param lsb        Least significant bit of signal.
 \param is_static  If set to non-zero value, sets static bit of this signal vector.

 Creates a new signal with the specified parameter information and adds this
 to the signal list if it does not already exist.
*/
void db_add_signal( char* name, int width, int lsb, int is_static ) {

  signal* sig;        /* Temporary pointer to newly created signal */
  char    msg[4096];  /* Display message string                    */

  snprintf( msg, 4096, "In db_add_signal, signal: %s, width: %d, lsb: %d", name, width, lsb );
  print_output( msg, NORMAL );

  sig = signal_create( name, width, lsb, is_static );

  if( sig_link_find( sig, curr_module->sig_head ) == NULL ) {
    sig_link_add( sig, &(curr_module->sig_head), &(curr_module->sig_tail) );
  } else {
    signal_dealloc( sig );
  }
  
}

/*!
 \param name  String name of signal to find in current module.

 \return Returns pointer to the found signal.  If signal is not found, internal error.

 Searches current module for signal matching the specified name.  If the signal is
 found, returns a pointer to the calling function for that signal.
*/
signal* db_find_signal( char* name ) {

  signal*   sig;         /* Temporary pointer to signal              */
  sig_link* sigl;        /* Temporary pointer to signal link element */
  char      msg[4096];   /* Display message string                   */

  snprintf( msg, 4096, "In db_find_signal, searching for signal %s", name );
  print_output( msg, NORMAL );

  // module_display_signals( curr_module );

  /* Create signal to find */
  sig = signal_create( name, 1, 0, TRUE );

  sigl = sig_link_find( sig, curr_module->sig_head );

  /* Free up memory from temporary signal value */
  signal_dealloc( sig );

  if( sigl == NULL ) {
    return( NULL );
  } else {
    return( sigl->sig );
  }

}

/*!
 \param right     Pointer to expression on right side of expression.
 \param left      Pointer to expression on left side of expression.
 \param op        Operation to perform on expression.
 \param line      Line number in Verilog file that expression is located (for line coverage).
 \param sig_name  Name of signal that expression is attached to (if valid).

 \return Returns pointer to newly created expression.

 Creates a new expression with the specified parameter information and returns a
 pointer to the newly created expression.
*/
expression* db_create_expression( expression* right, expression* left, int op, int line, char* sig_name ) {

  expression* expr;             /* Temporary pointer to newly created expression */
  char        msg[4096];        /* Display message string                        */

  snprintf( msg, 4096, "In db_create_expression, right: 0x%lx, left: 0x%lx, id: %d, op: %d, line: %d", 
                       right,
                       left,
                       curr_expr_id, 
                       op, 
                       line );
  print_output( msg, NORMAL );

  /* Create expression with next expression ID */
  expr = expression_create( right, left, op, curr_expr_id, line );
  curr_expr_id++;

  /* Set right and left side expression's (if they exist) parent pointer to this expression */
  if( right != NULL ) {
    if( (op != EXP_OP_COND_T) && (op != EXP_OP_COND_F) ) {
      assert( right->parent == NULL );
      right->parent = expr;
    }
  }

  if( left != NULL ) {
    if( (op != EXP_OP_COND_T) && (op != EXP_OP_COND_F) ) {
      assert( left->parent == NULL );
      left->parent = expr;
    }
  }

  /* Add expression and signal to binding list */
  if( sig_name != NULL ) {
    bind_add( sig_name, expr, curr_module->name );
  }

  return( expr );

}

/*!
 \param root      Pointer to root expression to add to module expression list.

 Adds the specified expression to the current module's expression list.
*/
void db_add_expression( expression* root ) {

  char msg[4096];   /* Display message string */

  if( root != NULL ) {

    if( exp_link_find( root, curr_module->exp_head ) == NULL ) {
    
      snprintf( msg, 4096, "In db_add_expression, id: %d, op: %d, line: %d", root->id, SUPPL_OP( root->suppl ), root->line );
      print_output( msg, NORMAL );
   
      // Add expression's children first.
      db_add_expression( root->right );
      db_add_expression( root->left ); 

      // Now add this expression to the list.
      exp_link_add( root, &(curr_module->exp_head), &(curr_module->exp_tail) );

    }

  }

  // module_display_expressions( curr_module );

}

/*!
 \param scope  Current VCD scope.

 Sets the curr_vcd_scope global variable to the specified scope.
*/
void db_set_vcd_scope( char* scope ) {

  char msg[4096];    /* Error/debug message               */
  int  scope_len;    /* Character length of current scope */

  snprintf( msg, 4096, "In db_set_vcd_scope, scope: %s", scope );
  print_output( msg, NORMAL );

  assert( scope != NULL );

  if( curr_vcd_scope != NULL ) {
    snprintf( curr_vcd_scope, strlen( curr_vcd_scope ), "%s.%s", curr_vcd_scope, scope );
  } else {
    if( instance_find_scope( instance_root, scope ) != NULL ) {
      curr_vcd_scope = (char*)malloc_safe( 4096 );
      strcpy( curr_vcd_scope, scope );
    }
  }

}

/*!
 Moves the curr_vcd_scope up one level of hierarchy.  This function is called
 when the $upscope keyword is seen in a VCD file.
*/
void db_vcd_upscope() {

  char msg[4096];    /* Error/debug message       */
  char back[4096];   /* Lowest level of hierarchy */
  char rest[4096];   /* Hierarchy up one level    */

  snprintf( msg, 4096, "In db_vcd_upscope, curr_vcd_scope: %s", curr_vcd_scope );
  print_output( msg, NORMAL );

  assert( curr_vcd_scope != NULL );
  
  scope_extract_back( curr_vcd_scope, back, rest );

  if( rest[0] != '\0' ) {
    strcpy( curr_vcd_scope, rest );
  }

}

/*!
 \param name    Name of signal to set value to.
 \param symbol  Symbol value of signal used in VCD dumpfile.

 Creates a new entry in the symbol table for the specified signal and symbol.
*/
void db_assign_symbol( char* name, char* symbol ) {

  sig_link*     slink;      /* Pointer to signal containing this symbol  */
  signal*       tmpsig;     /* Temporary pointer to signal to search for */
  char*         scope;      /* Scope of module containing signal         */
  char*         signame;    /* Name of signal to find in module          */
  char          msg[4096];  /* Display message string                    */
  mod_inst*     inst;       /* Found instance                            */

  snprintf( msg, 4096, "In db_assign_symbol, name: %s, symbol: %s", name, symbol );
  print_output( msg, NORMAL );

  assert( name != NULL );

  if( curr_vcd_scope != NULL ) {

    scope   = strdup( curr_vcd_scope );
    signame = strdup( name );

  } else {

    scope   = (char*)malloc_safe( strlen( name ) );
    signame = (char*)malloc_safe( strlen( name ) );

    scope_extract_back( name, signame, scope );

  }

  /* Find module with specified scope of this signal */
  if( (inst = instance_find_scope( instance_root, scope )) != NULL ) {

    tmpsig = signal_create( signame, 1, 0, FALSE );

    /* Find the signal that matches the specified signal name */
    if( (slink = sig_link_find( tmpsig, inst->mod->sig_head )) != NULL ) {

      /* Add this signal */
      symtable_add( symbol, slink->sig, &vcd_symtab );

    } else {

      symtable_add( symbol, NULL, &vcd_symtab );

    }

    signal_dealloc( tmpsig );

  } else {

    print_output( "VCD dumpfile does not match design file", FATAL );

  }

  free_safe( scope );
  free_safe( signame );

}

/*!
 \param symbol

 \return Returns TRUE if the symbol is found; otherwise, returns FALSE.

 Searches the current timestep symbol table to see if the specified signal exists
 in this list.  If the signal is not found, returns FALSE; otherwise, returns a
 value of TRUE.  Note:  this function should be called before db_find_set_add_signal.
*/
bool db_symbol_found( char* symbol ) {

  signal* sig;    /* Pointer to found signal */

  return( symtable_find( symbol, timestep_tab, &sig ) );

}

/*!
 \param symbol  VCD signal symbol from VCD dumpfile.
 \param vec     Specified value to set found signal to.

 Searches symbol table for this timestep.  If this signal does not already exist in here,
 look in VCD symbol table for specified signal.  Assign specified vector value to signal
 value and store expression pointers into expression queue to work on this timestep.
*/
void db_find_set_add_signal( char* symbol, vector* vec ) {

  signal*   sig;        /* Pointer to found signal                                      */
  exp_link* curr_exp;   /* Pointer to current expression link in signal expression list */
  char      msg[4096];  /* Display message string                                       */

  snprintf( msg, 4096, "In db_find_set_add_signal, addr: 0x%lx, symbol: %s", symbol, symbol );
  print_output( msg, NORMAL );

  if( !symtable_find( symbol, timestep_tab, &sig ) ) {

    if( !symtable_find( symbol, vcd_symtab, &sig ) ) {

      snprintf( msg, 4096, "VCD dumpfile symbol not found, symbol: %s", symbol );
      print_output( msg, FATAL );
      exit( 1 );

    } else {

      if( sig != NULL ) {

        signal_set_value( sig, vec->value, vec->width, 0, sig->value->lsb );

        /* Add signal's expressions to expression queue */
        curr_exp = sig->exp_head;
        while( curr_exp != NULL ) {

          /* Specify that this expression is currently in the expression queue */
          curr_exp->exp->suppl = curr_exp->exp->suppl | (0x1 << SUPPL_LSB_IN_QUEUE);

          /* Set signal expressions supplemental field TRUE/FALSE bits */
          if( (vec->value[0] & 0x3) == 0 ) {
            curr_exp->exp->suppl = curr_exp->exp->suppl | (0x1 << SUPPL_LSB_FALSE);
          } else if( (vec->value[0] & 0x3) == 1 ) {
            curr_exp->exp->suppl = curr_exp->exp->suppl | (0x1 << SUPPL_LSB_TRUE);
          }

          exp_link_add( curr_exp->exp, &(exp_queue_head), &(exp_queue_tail) );
          curr_exp = curr_exp->next;

        }

        symtable_add( symbol, sig, &timestep_tab );

      }

    }

  }

}

/*!
 \param time  Current time step value being performed.

 Cycles through expression queue, performing expression evaluations as we go.  If
 an expression has a parent expression, that parent expression is placed in the
 expression queue after that expression has completed its evaluation.  When the
 expression queue is empty, we are finished for this clock period.
*/
void db_do_timestep( int time ) {

  char      msg[4096];  /* Display message string                         */
  exp_link* head;       /* Current expression at head of expression queue */

  snprintf( msg, 4096, "Performing timestep #%d", time );
  print_output( msg, NORMAL );

  while( exp_queue_head != NULL ) {

//    exp_link_display( exp_queue_head );

    assert( exp_queue_head->exp != NULL );

    /* Perform expression operation */
//    printf( "Expression address: 0x%lx\n", exp_queue_head->exp );
//    printf( "Performing expression operation: %d, id: %d\n", exp_queue_head->exp->op, exp_queue_head->exp->id );
    expression_operate( exp_queue_head->exp );

    /* Indicate that this expression is no longer in the expression queue. */
    exp_queue_head->exp->suppl = exp_queue_head->exp->suppl & ~(0x1 << SUPPL_LSB_IN_QUEUE);

    /* Indicate that this expression has been executed */
    exp_queue_head->exp->suppl = exp_queue_head->exp->suppl | (0x1 << SUPPL_LSB_EXECUTED);

    /* If there is a parent, place parent expression in expression queue. */
    if( (exp_queue_head->exp->parent != NULL) && 
        ((exp_queue_head->exp->parent->suppl & (0x1 << SUPPL_LSB_IN_QUEUE)) == 0) ) {
      exp_queue_head->exp->parent->suppl = exp_queue_head->exp->parent->suppl | (0x1 << SUPPL_LSB_IN_QUEUE);
      exp_link_add( exp_queue_head->exp->parent, &(exp_queue_head), &(exp_queue_tail) );
    }

    /* Move head pointer in expression queue. */
    head           = exp_queue_head;
    exp_queue_head = head->next;
    free_safe( head );

  }

  /* Finally, clear timestep_tab */
  symtable_dealloc( timestep_tab );
  timestep_tab = NULL;

}

/*!
 \param symbol  Symbol of signal to find.

 \return Returns signal width of specified symbol or 0 if signal is not found.

 Searches symbol table for specified symbol.  If the corresponding signal is valid,
 returns the width value of the signal to the calling function.
*/
int db_get_signal_size( char* symbol ) {

  signal* sig;        /* Pointer to found signal      */
  char    msg[4096];  /* Debug/error message for user */

  snprintf( msg, 4096, "In db_get_signal_size, symbol: %s\n", symbol );
  print_output( msg, NORMAL );

  if( symtable_find( symbol, vcd_symtab, &sig ) ) {
    if( sig != NULL ) {
      assert( sig->value != NULL );
      return( sig->value->width );
    } else {
      return( 0 );
    }
  } else {
    return( 0 );
  }

}

