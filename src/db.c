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
#include "statement.h"
#include "sim.h"


extern char*      top_module;

extern mod_inst* instance_root;

/*!
 Specifies the string Verilog scope that is currently specified in the VCD file.
*/
char* curr_vcd_scope = NULL;

/*!
 Pointer to the current instance selected by the VCD parser.  If this value is
 NULL, the current instance does not reside in the design specified for coverage.
*/
mod_inst* curr_instance  = NULL;

extern mod_link* mod_head;
extern mod_link* mod_tail;

str_link* modlist_head      = NULL;
str_link* modlist_tail      = NULL;

module*   curr_module       = NULL;

symtable* vcd_symtab        = NULL;
symtable* timestep_tab      = NULL;

extern nibble or_optab[16];

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
int       curr_sim_time = 0;

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
  assert( instance_root->mod != NULL );
  instance_dealloc( instance_root, instance_root->mod->scope );
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

        } else if( type == DB_TYPE_STATEMENT ) {

          /* Parse rest of line for statement info */
          retval = statement_db_read( &rest_line, curr_module, read_mode );

        } else if( type == DB_TYPE_MODULE ) {

          /* Parse rest of line for module info */
          if( retval = module_db_read( &tmpmod, &rest_line ) ) {

            assert( tmpmod != NULL );
            assert( tmpmod->scope != NULL );
            assert( tmpmod->name != NULL );

            if( curr_module != NULL ) {

              if( read_mode == READ_MODE_MERGE_INST_MERGE ) {
              
                /* Find module in instance tree and do a module merge */
                if( (foundinst = instance_find_scope( instance_root, tmpmod->scope )) == NULL ) {
                  print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
                  retval = FALSE;
                } else {
                  /* Perform module merge */
                  module_merge( foundinst->mod, curr_module );
                }

              } else if( read_mode == READ_MODE_REPORT_MOD_MERGE ) {

                if( (foundmod = mod_link_find( curr_module, mod_head )) == NULL ) {
                  mod_link_add( curr_module, &mod_head, &mod_tail );
                } else {
                  module_merge( foundmod->mod, curr_module );
                }

              }

              // Temporarily commenting out to correct no-merge cases.
              // module_dealloc( curr_module );

            }

            curr_module = tmpmod;

            if( (read_mode == READ_MODE_MERGE_NO_MERGE) || (read_mode == READ_MODE_REPORT_NO_MERGE) ) {

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

    if( read_mode == READ_MODE_MERGE_INST_MERGE ) {
              
      /* Find module in instance tree and do a module merge */
      if( (foundinst = instance_find_scope( instance_root, tmpmod->scope )) == NULL ) {
        print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
        retval = FALSE;
      } else {
        /* Perform module merge */
        module_merge( foundinst->mod, curr_module );
      }

    } else if( read_mode == READ_MODE_REPORT_MOD_MERGE ) {

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
  print_output( msg, DEBUG );

  /* Create new module node */
  mod       = module_create();
  mod->name = strdup( modname );

  if( (found_mod_link = mod_link_find( mod, mod_head )) != NULL ) {

    printf( "In db_add_instance 1, curr_module->name: %s\n", curr_module->name );
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
  print_output( msg, DEBUG );

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

  print_output( "In db_end_module", DEBUG );

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
  print_output( msg, DEBUG );

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
  print_output( msg, DEBUG );

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
 \param line      Line number of current expression.
 \param sig_name  Name of signal that expression is attached to (if valid).

 \return Returns pointer to newly created expression.

 Creates a new expression with the specified parameter information and returns a
 pointer to the newly created expression.
*/
expression* db_create_expression( expression* right, expression* left, int op, int line, char* sig_name ) {

  expression* expr;             /* Temporary pointer to newly created expression */
  char        msg[4096];        /* Display message string                        */
  int         right_id;         /* ID of right expression                        */
  int         left_id;          /* ID of left expression                         */

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

  snprintf( msg, 4096, "In db_create_expression, right: %d, left: %d, id: %d, op: %d, line: %d", 
                       right_id,
                       left_id,
                       curr_expr_id, 
                       op,
                       line );
  print_output( msg, DEBUG );

  /* Create expression with next expression ID */
  expr = expression_create( right, left, op, curr_expr_id, line );
  curr_expr_id++;

  /* Set right and left side expression's (if they exist) parent pointer to this expression */
  if( right != NULL ) {
    assert( right->parent->expr == NULL );
    right->parent->expr = expr;
  }

  if( (left != NULL) &&
      (SUPPL_OP( expr->suppl ) != EXP_OP_CASE) &&
      (SUPPL_OP( expr->suppl ) != EXP_OP_CASEX) &&
      (SUPPL_OP( expr->suppl ) != EXP_OP_CASEZ) ) {
    assert( left->parent->expr == NULL );
    left->parent->expr = expr;
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
    
      snprintf( msg, 4096, "In db_add_expression, id: %d, op: %d", root->id, SUPPL_OP( root->suppl ) );
      print_output( msg, DEBUG );
   
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
 \param exp      Pointer to associated "root" expression.

 \return Returns pointer to created statement.

 Creates an statement structure and adds created statement to current
 module's statement list.
*/
statement* db_create_statement( expression* exp ) {

  statement* stmt;       /* Pointer to newly created statement */
  char       msg[4096];  /* Message to display to user         */

  snprintf( msg, 4096, "In db_create_statement, id: %d", exp->id );
  print_output( msg, DEBUG );

  stmt = statement_create( exp );

  return( stmt );

}

/*!
 \param stmt  Pointer to statement add to current module's statement list.

 Adds the specified statement tree to the tail of the current module's statement list.
*/
void db_add_statement( statement* stmt ) {

  char msg[4096];    /* Message to display to user */
/*
  static int count = 0;

  if( count > 70 ) {
    assert( count == 0 );
  } else {
    count++;
  }
*/
 
  if( stmt != NULL ) {

    snprintf( msg, 4096, "In db_add_statement, id: %d", stmt->exp->id );
    print_output( msg, DEBUG );

    /* Add TRUE and FALSE statement paths to list */
    if( SUPPL_IS_STMT_STOP( stmt->exp->suppl ) == 0 ) {
      db_add_statement( stmt->next_false );
    }

    if( stmt->next_true != stmt->next_false ) {
      db_add_statement( stmt->next_true );
    }

    /* Now add current statement */
    stmt_link_add_tail( stmt, &(curr_module->stmt_head), &(curr_module->stmt_tail) );

  }

}

/*!
 \param stmt       Pointer to statement to connect true path to.
 \param next_true  Pointer to statement to run if statement evaluates to TRUE.

 Connects the specified statement's true statement.
*/
void db_connect_statement_true( statement* stmt, statement* next_true ) {

  char msg[4096];   /* Message to display to user          */
  int  next_id;     /* Statement ID of next TRUE statement */

  if( stmt != NULL ) {

    if( next_true == NULL ) {
      next_id = 0;
    } else {
      next_id = next_true->exp->id;
    }

    snprintf( msg, 4096, "In db_connect_statement_true, id: %d, next: %d", stmt->exp->id, next_id );
    print_output( msg, DEBUG );

    stmt->next_true = next_true;

  }

}

/*!
 \param stmt        Pointer to statement to connect false path to.
 \param next_false  Pointer to statement to run if statement evaluates to FALSE.

 Connects the specified statement's false statement.
*/
void db_connect_statement_false( statement* stmt, statement* next_false ) {

  char msg[4096];   /* Message to display to user           */
  int  next_id;     /* Statement ID of next FALSE statement */

  if( stmt != NULL ) {

    if( next_false == NULL ) {
      next_id = 0;
    } else {
      next_id = next_false->exp->id;
    }

    snprintf( msg, 4096, "In db_connect_statement_false, id: %d, next: %d", stmt->exp->id, next_id );
    print_output( msg, DEBUG );

    stmt->next_false = next_false;

  }

}

/*!
 \param curr_stmt  Pointer to current statement to attach.
 \param next_stmt  Pointer to next statement to attach to.

 Calls the statement_connect function located in statement.c with the specified parameters.
*/
void db_statement_connect( statement* curr_stmt, statement* next_stmt ) {

  char msg[4096];   /* Message to display to user */
  int  curr_id;     /* Current statement ID       */
  int  next_id;     /* Next statement ID          */

  if( curr_stmt == NULL ) {    curr_id = 0;
  } else {
    curr_id = curr_stmt->exp->id;
  }

  if( next_stmt == NULL ) {
    next_id = 0;
  } else {
    next_id = next_stmt->exp->id;
  }

  snprintf( msg, 4096, "In db_statement_connect, curr_stmt: %d, next_stmt: %d", curr_id, next_id );
  print_output( msg, DEBUG );

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

  char msg[4096];    /* Message to display to user */
  int  stmt_id;      /* Current statement ID       */
  int  post_id;      /* Statement ID after stop    */

  if( stmt != NULL ) {

    stmt_id = stmt->exp->id;

    if( post == NULL ) {
      post_id = 0;
    } else {
      post_id = post->exp->id;
    }

    snprintf( msg, 4096, "In db_statement_set_stop, stmt: %d, next_stmt: %d", stmt_id, post_id );
    print_output( msg, DEBUG );
 
    statement_set_stop( stmt, post, TRUE, both );

  }

}

/*!
 \param scope  Current VCD scope.

 Sets the curr_vcd_scope global variable to the specified scope.
*/
void db_set_vcd_scope( char* scope ) {

  char  msg[4096];    /* Error/debug message                           */
  int   scope_len;    /* Character length of current scope             */
  char* tmpscope;     /* Temporary string holder for current VCD scope */

  snprintf( msg, 4096, "In db_set_vcd_scope, scope: %s", scope );
  print_output( msg, DEBUG );

  assert( scope != NULL );

  if( curr_vcd_scope != NULL ) {

    scope_len = strlen( curr_vcd_scope ) + strlen( scope ) + 2; 
    tmpscope  = (char*)malloc_safe( scope_len );
    snprintf( tmpscope, scope_len, "%s.%s", curr_vcd_scope, scope );

    free_safe( curr_vcd_scope );
    curr_vcd_scope = tmpscope;

    curr_instance = instance_find_scope( instance_root, tmpscope );

  } else {

    tmpscope = strdup( scope );

    if( (curr_instance = instance_find_scope( instance_root, tmpscope )) != NULL ) {

      free_safe( curr_vcd_scope );
      curr_vcd_scope = tmpscope;

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
  print_output( msg, DEBUG );  

  if( curr_vcd_scope != NULL ) {

    scope_extract_back( curr_vcd_scope, back, rest );

    if( rest[0] != '\0' ) {
      strcpy( curr_vcd_scope, rest );
    } else {
      free_safe( curr_vcd_scope );
      curr_vcd_scope = NULL;
    }

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
  char*         signame;    /* Name of signal to find in module          */
  char          msg[4096];  /* Display message string                    */
  mod_inst*     inst;       /* Found instance                            */

  snprintf( msg, 4096, "In db_assign_symbol, name: %s, symbol: %s, curr_vcd_scope: %s", name, symbol, curr_vcd_scope );
  print_output( msg, DEBUG );

  assert( name != NULL );

  if( curr_instance != NULL ) {
    
    signame = strdup( name );
    tmpsig  = signal_create( signame, 1, 0, FALSE );

    /* Find the signal that matches the specified signal name */
    if( (slink = sig_link_find( tmpsig, curr_instance->mod->sig_head )) != NULL ) {

      /* Add this signal */
      symtable_add( symbol, slink->sig, &vcd_symtab );

    } else {

      snprintf( msg, 4096, "VCD signal \"%s.%s\" found that is not part of design", curr_vcd_scope, name );
      print_output( msg, WARNING );

    }

    signal_dealloc( tmpsig );
    free_safe( signame );

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

  symtable* symtab;      /* Pointer to found symbol table entry                             */
  symtable* new_symtab;  /* Pointer to newly create symbol table entry                      */
  int       skip = 0;    /* Specifies number of symbols to skip in search that match symbol */
  char      msg[4096];   /* Message to user                                                 */

  snprintf( msg, 4096, "In db_set_symbol_char, sym: %s, value: %c", sym, value );
  print_output( msg, DEBUG );

  while( (symtab = symtable_find( sym, timestep_tab, skip )) != NULL ) {

    /* Update value */
    symtab->value[0] = value;
    symtab->value[1] = '\0';

    skip++;
 
  }

  /* Only search VCD table if symbol has never been moved over */
  if( skip == 0 ) {

    while( (symtab = symtable_find( sym, vcd_symtab, skip )) != NULL ) {

      assert( symtab->sig->value != NULL );

      /* Add to timestep table */
      new_symtab = symtable_add( sym, symtab->sig, &(timestep_tab) );

      /* Update value */
      new_symtab->value[0] = value;
      new_symtab->value[1] = '\0';

      skip++;

    }

  }

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

  symtable* symtab;      /* Pointer to found symbol table entry                             */
  symtable* new_symtab;  /* Pointer to newly create symbol table entry                      */
  int       skip = 0;    /* Specifies number of symbols to skip in search that match symbol */
  char      msg[4096];   /* Message to user                                                 */

  snprintf( msg, 4096, "In db_set_symbol_string, sym: %s, value: %s", sym, value );
  print_output( msg, DEBUG );

  while( (symtab = symtable_find( sym, timestep_tab, skip )) != NULL ) {

    /* Update value */
    strcpy( symtab->value, value ); 

    skip++;
 
  }

  /* Only search VCD table if symbol has never been moved over */
  if( skip == 0 ) {

    while( (symtab = symtable_find( sym, vcd_symtab, skip )) != NULL ) {

      assert( symtab->sig->value != NULL );

      /* Add to timestep table */
      new_symtab = symtable_add( sym, symtab->sig, &(timestep_tab) );

      /* Update value */
      strcpy( new_symtab->value, value );

      skip++;

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
  print_output( msg, DEBUG );

  curr_sim_time = time;

  /* Assign all stored values in current timestep to stored signals */
  symtable_assign( timestep_tab );

  /* Simulate the current timestep */
  sim_simulate();

  /* Finally, clear timestep_tab */
  symtable_dealloc( timestep_tab );
  timestep_tab = NULL;

}

/* $Log$
/* Revision 1.37  2002/07/12 04:53:29  phase1geo
/* Removing counter code that was used for debugging infinite loops in code
/* previously.
/*
/* Revision 1.36  2002/07/09 04:46:26  phase1geo
/* Adding -D and -Q options to covered for outputting debug information or
/* suppressing normal output entirely.  Updated generated documentation and
/* modified Verilog diagnostic Makefile to use these new options.
/*
/* Revision 1.35  2002/07/09 03:24:48  phase1geo
/* Various fixes for module instantiantion handling.  This now works.  Also
/* modified report output for toggle, line and combinational information.
/* Regression passes.
/*
/* Revision 1.34  2002/07/09 02:04:25  phase1geo
/* Fixing segmentation fault error due to deallocating a module before we
/* have completed using it.
/*
/* Revision 1.33  2002/07/08 19:02:10  phase1geo
/* Adding -i option to properly handle modules specified for coverage that
/* are instantiated within a design without needing to parse parent modules.
/*
/* Revision 1.32  2002/07/08 12:35:31  phase1geo
/* Added initial support for library searching.  Code seems to be broken at the
/* moment.
/*
/* Revision 1.31  2002/07/05 16:49:47  phase1geo
/* Modified a lot of code this go around.  Fixed VCD reader to handle changes in
/* the reverse order (last changes are stored instead of first for timestamp).
/* Fixed problem with AEDGE operator to handle vector value changes correctly.
/* Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
/* Full regression passes; however, the recent changes seem to have impacted
/* performance -- need to look into this.
/*
/* Revision 1.30  2002/07/05 00:10:18  phase1geo
/* Adding report support for case statements.  Everything outputs fine; however,
/* I want to remove CASE, CASEX and CASEZ expressions from being reported since
/* it causes redundant and misleading information to be displayed in the verbose
/* reports.  New diagnostics to check CASE expressions have been added and pass.
/*
/* Revision 1.29  2002/07/04 23:10:12  phase1geo
/* Added proper support for case, casex, and casez statements in score command.
/* Report command still incorrect for these statement types.
/*
/* Revision 1.28  2002/07/03 21:30:52  phase1geo
/* Fixed remaining issues with always statements.  Full regression is running
/* error free at this point.  Regenerated documentation.  Added EOR expression
/* operation to handle the or expression in event lists.
/*
/* Revision 1.27  2002/07/03 19:54:36  phase1geo
/* Adding/fixing code to properly handle always blocks with the event control
/* structures attached.  Added several new diagnostics to test this ability.
/* always1.v is still failing but the rest are passing.
/*
/* Revision 1.26  2002/07/02 19:52:50  phase1geo
/* Removing unecessary diagnostics.  Cleaning up extraneous output and
/* generating new documentation from source.  Regression passes at the
/* current time.
/*
/* Revision 1.25  2002/07/02 18:42:18  phase1geo
/* Various bug fixes.  Added support for multiple signals sharing the same VCD
/* symbol.  Changed conditional support to allow proper simulation results.
/* Updated VCD parser to allow for symbols containing only alphanumeric characters.
/*
/* Revision 1.24  2002/07/01 15:10:42  phase1geo
/* Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
/* seem to be passing with these fixes.
/*
/* Revision 1.23  2002/06/30 22:23:20  phase1geo
/* Working on fixing looping in parser.  Statement connector needs to be revamped.
/*
/* Revision 1.22  2002/06/28 03:04:59  phase1geo
/* Fixing more errors found by diagnostics.  Things are running pretty well at
/* this point with current diagnostics.  Still some report output problems.
/*
/* Revision 1.21  2002/06/28 00:40:37  phase1geo
/* Cleaning up extraneous output from debugging.
/*
/* Revision 1.20  2002/06/27 20:39:43  phase1geo
/* Fixing scoring bugs as well as report bugs.  Things are starting to work
/* fairly well now.  Added rest of support for delays.
/*
/* Revision 1.19  2002/06/27 12:36:47  phase1geo
/* Fixing bugs with scoring.  I think I got it this time.
/*
/* Revision 1.18  2002/06/26 04:59:50  phase1geo
/* Adding initial support for delays.  Support is not yet complete and is
/* currently untested.
/*
/* Revision 1.17  2002/06/26 03:45:48  phase1geo
/* Fixing more bugs in simulator and report functions.  About to add support
/* for delay statements.
/*
/* Revision 1.16  2002/06/25 21:46:10  phase1geo
/* Fixes to simulator and reporting.  Still some bugs here.
/*
/* Revision 1.15  2002/06/25 03:39:03  phase1geo
/* Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
/* Fixed some report bugs though there are still some remaining.
/*
/* Revision 1.14  2002/06/24 12:34:56  phase1geo
/* Fixing the set of the STMT_HEAD and STMT_STOP bits.  We are getting close.
/*
/* Revision 1.13  2002/06/24 04:54:48  phase1geo
/* More fixes and code additions to make statements work properly.  Still not
/* there at this point.
/*
/* Revision 1.12  2002/06/22 21:08:23  phase1geo
/* Added simulation engine and tied it to the db.c file.  Simulation engine is
/* currently untested and will remain so until the parser is updated correctly
/* for statements.  This will be the next step.
/*
/* Revision 1.11  2002/06/22 05:27:30  phase1geo
/* Additional supporting code for simulation engine and statement support in
/* parser.
/*
/* Revision 1.10  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/*
/* Revision 1.9  2002/05/03 03:39:36  phase1geo
/* Removing all syntax errors due to addition of statements.  Added more statement
/* support code.  Still have a ways to go before we can try anything.  Removed lines
/* from expressions though we may want to consider putting these back for reporting
/* purposes.
/*
/* Revision 1.8  2002/05/02 03:27:42  phase1geo
/* Initial creation of statement structure and manipulation files.  Internals are
/* still in a chaotic state.
/*
/* Revision 1.7  2002/04/30 05:04:25  phase1geo
/* Added initial go-round of adding statement handling to parser.  Added simple
/* Verilog test to check correct statement handling.  At this point there is a
/* bug in the expression write function (we need to display statement trees in
/* the proper order since they are unlike normal expression trees.)
/* */
