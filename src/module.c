/*!
 \file     module.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "module.h"
#include "util.h"
#include "expr.h"
#include "signal.h"
#include "statement.h"


/*!
 \param mod  Pointer to module to initialize.

 Initializes all contents to NULL.
*/  
void module_init( module* mod ) {
    
  mod->name      = NULL;
  mod->filename  = NULL;
  mod->stat      = NULL;
  mod->sig_head  = NULL;
  mod->sig_tail  = NULL;
  mod->exp_head  = NULL;
  mod->exp_tail  = NULL;
  mod->stmt_head = NULL;
  mod->stmt_tail = NULL;

}

/*!
 \return Returns pointer to newly created module element that has been
         properly initialized.

 Allocates memory from the heap for a module element and initializes all
 contents to NULL.  Returns a pointer to the newly created module.
*/
module* module_create() {

  module* mod;   /* Pointer to newly created module element */

  /* Create and initialize module */
  mod = (module*)malloc_safe( sizeof( module ) );

  module_init( mod );

  return( mod );

}

/*!
 \param base  Module that will merge in that data from the in module
 \param in    Module that will be merged into the base module.

 Performs a merge of the two specified modules, placing the resulting
 merge module into the module named base.  If there are any differences between
 the two modules, a warning or error will be displayed to the user.
*/
void module_merge( module* base, module* in ) {

  char msg[4096];            /* Warning/error message to display to user                     */
  exp_link* curr_base_exp;   /* Pointer to current expression in base module expression list */
  exp_link* curr_in_exp;     /* Pointer to current expression in in module expression list   */
  sig_link* curr_base_sig;   /* Pointer to current signal in base module signal list         */
  sig_link* curr_in_sig;     /* Pointer to current signal in in module signal list           */

  assert( base != NULL );
  assert( base->name != NULL );

  assert( in != NULL );
  assert( in->name != NULL );

  printf( "In module_merge, merging module: %s\n", base->name );

  if( strcmp( base->name, in->name ) != 0 ) {

    snprintf( msg, 4096, "Modules with different names being merged (%s, %s)\n",
              base->name,
              in->name );
    print_output( msg, FATAL );
    exit( 1 );

  } else {

    /* Merge module expressions */
    curr_base_exp = base->exp_head;
    curr_in_exp   = in->exp_head;
    
    while( (curr_base_exp != NULL) && (curr_in_exp != NULL) ) {
      expression_merge( curr_base_exp->exp, curr_in_exp->exp );
      curr_base_exp = curr_base_exp->next;
      curr_in_exp   = curr_in_exp->next;
    }

    if( ((curr_base_exp == NULL) && (curr_in_exp != NULL)) ||
        ((curr_base_exp != NULL) && (curr_in_exp == NULL)) ) {
   
      snprintf( msg, 4096, "Expression lists for module %s are not equivalent.  Unable to merge.",
                base->name );
      print_output( msg, FATAL );
      exit( 1 );
    
    }

    /* Merge module signals */
    curr_base_sig = base->sig_head;
    curr_in_sig   = in->sig_head;

    printf( "base:\n" );  sig_link_display( curr_base_sig );
    printf( "in:\n" );    sig_link_display( curr_in_sig );

    while( (curr_base_sig != NULL) && (curr_in_sig != NULL) ) {
      signal_merge( curr_base_sig->sig, curr_in_sig->sig );
      curr_base_sig = curr_base_sig->next;
      curr_in_sig   = curr_in_sig->next;
    }

    if( ((curr_base_sig == NULL) && (curr_in_sig != NULL)) ||
        ((curr_base_sig != NULL) && (curr_in_sig == NULL)) ) {
   
      snprintf( msg, 4096, "Signal lists for module %s are not equivalent.  Unable to merge.",
                base->name );
      print_output( msg, FATAL );
      exit( 1 );
    
    }

  }

}

/*!
 \param mod    Pointer to module to write to output.
 \param scope  String version of module scope in hierarchy.
 \param file   Pointer to specified output file to write contents.
 \return Returns TRUE if file output was successful; otherwise, returns FALSE.

 Prints the database line for the specified module to the specified database
 file.  If there are any problems with the write, returns FALSE; otherwise,
 returns TRUE.
*/
bool module_db_write( module* mod, char* scope, FILE* file ) {

  bool       retval = TRUE;    /* Return value for this function              */
  sig_link*  curr_sig;         /* Pointer to current module sig_link element  */
  exp_link*  curr_exp;         /* Pointer to current module exp_link element  */
  stmt_link* curr_stmt;        /* Pointer to current module stmt_link element */
  char       msg[4096];        /* Display message string                      */

  snprintf( msg, 4096, "Writing module %s", mod->name );
  print_output( msg, DEBUG );

  fprintf( file, "%d %s %s %s\n",
    DB_TYPE_MODULE,
    mod->name,
    scope,
    mod->filename
  );

  // module_display_expressions( mod );

  /* Now print all expressions in module */
  curr_exp = mod->exp_head;
  while( curr_exp != NULL ) {
    
    expression_db_write( curr_exp->exp, file, scope );
    curr_exp = curr_exp->next;

  }

  // module_display_signals( mod );

  /* Now print all signals in module */
  curr_sig = mod->sig_head;
  while( curr_sig != NULL ) {

    signal_db_write( curr_sig->sig, file, scope );
    curr_sig = curr_sig->next; 

  }

  // module_display_statements( mod );

  /* Now print all statements in module */
  curr_stmt = mod->stmt_head;
  while( curr_stmt != NULL ) {

    statement_db_write( curr_stmt->stmt, file, scope );
    curr_stmt = curr_stmt->next;

  }

  return( retval );

}

/*!
 \param mod    Pointer to module to read input from.
 \param scope  Pointer to string to store module instance scope.
 \param line   Pointer to current line to parse.
 \return Returns TRUE if read was successful; otherwise, returns FALSE.

 Reads the current line of the specified file and parses it for a module.
 Creates a new module and assigns the new module to the address specified
 by the mod pointer.  If all is successful, returns TRUE; otherwise, returns
 FALSE.
*/
bool module_db_read( module** mod, char** scope, char** line ) {

  bool    retval = TRUE;    /* Return value for this function             */
  char    name[256];        /* Holder for module name                     */
  char    curr_scope[4096]; /* Verilog hierarchical scope for this module */
  char    filename[256];    /* Holder for module filename                 */
  int     chars_read;       /* Number of characters currently read        */

  *mod = module_create();

  if( sscanf( *line, "%s %s %s%n", name, curr_scope, filename, &chars_read ) == 3 ) {

    *line = *line + chars_read;

    (*mod)->name     = strdup( name );
    *scope           = strdup( curr_scope );
    (*mod)->filename = strdup( filename );

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param mod  Pointer to module element to display signals.

 Iterates through signal list of specified module, displaying each signal's
 name, width, lsb and value.
*/
void module_display_signals( module* mod ) {

  sig_link* sigl;    /* Pointer to current signal link element */

  printf( "Module => %s\n", mod->name );

  sigl = mod->sig_head;
  while( sigl != NULL ) {
    signal_display( sigl->sig );
    sigl = sigl->next;
  }

}

/*!
 \param mod  Pointer to module element to display expressions

 Iterates through expression list of specified module, displaying each expression's
 id.
*/
void module_display_expressions( module* mod ) {

  exp_link* expl;    /* Pointer to current expression link element */

  printf( "Module => %s\n", mod->name );

  expl = mod->exp_head;
  while( expl != NULL ) {
    expression_display( expl->exp );
    expl = expl->next;
  }

}

/*!
 \param mod  Pointer to module element to clean.

 Deallocates module contents: name and filename strings.
*/
void module_clean( module* mod ) {

  if( mod != NULL ) {

    /* Free module name */
    if( mod->name != NULL ) {
      free_safe( mod->name );
      mod->name = NULL;
    }

    /* Free module filename */
    if( mod->filename != NULL ) {
      free_safe( mod->filename );
      mod->filename = NULL;
    }

    /* Free expression list */
    exp_link_delete_list( mod->exp_head, TRUE );
    mod->exp_head = NULL;

    /* Free signal list */
    sig_link_delete_list( mod->sig_head );
    mod->sig_head = NULL;

    /* Free statement list */
    stmt_link_delete_list( mod->stmt_head );
    mod->stmt_head = NULL;

  }

}

/*!
 \param mod  Pointer to module element to deallocate.

 Deallocates module; name and filename strings; and finally
 the structure itself from the heap.
*/
void module_dealloc( module* mod ) {

  if( mod != NULL ) {

    //module_display_expressions( mod );

    module_clean( mod );

    /* Deallocate module element itself */
    free_safe( mod );

  }

}


/* $Log$
/* Revision 1.10  2002/07/20 18:46:38  phase1geo
/* Causing fully covered modules to not be output in reports.  Adding
/* instance3.v diagnostic to verify this works correctly.
/*
/* Revision 1.9  2002/07/18 02:33:24  phase1geo
/* Fixed instantiation addition.  Multiple hierarchy instantiation trees should
/* now work.
/*
/* Revision 1.8  2002/07/14 05:10:42  phase1geo
/* Added support for signal concatenation in score and report commands.  Fixed
/* bugs in this code (and multiplication).
/*
/* Revision 1.7  2002/07/09 04:46:26  phase1geo
/* Adding -D and -Q options to covered for outputting debug information or
/* suppressing normal output entirely.  Updated generated documentation and
/* modified Verilog diagnostic Makefile to use these new options.
/*
/* Revision 1.6  2002/06/26 03:45:48  phase1geo
/* Fixing more bugs in simulator and report functions.  About to add support
/* for delay statements.
/*
/* Revision 1.5  2002/06/25 02:02:04  phase1geo
/* Fixing bugs with writing/reading statements and with parsing design with
/* statements.  We now get to the scoring section.  Some problems here at
/* the moment with the simulator.
/*
/* Revision 1.4  2002/06/24 04:54:48  phase1geo
/* More fixes and code additions to make statements work properly.  Still not
/* there at this point.
/*
/* Revision 1.3  2002/05/02 03:27:42  phase1geo
/* Initial creation of statement structure and manipulation files.  Internals are
/* still in a chaotic state.
/* */

