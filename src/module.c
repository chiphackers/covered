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


/*!
 \param mod  Pointer to module to initialize.

 Initializes all contents to NULL.
*/  
void module_init( module* mod ) {
    
  mod->name     = NULL;
  mod->scope    = NULL;
  mod->filename = NULL;
  mod->sig_head = NULL;
  mod->sig_tail = NULL;
  mod->exp_head = NULL;
  mod->exp_tail = NULL;

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
  assert( base->scope != NULL );

  assert( in != NULL );
  assert( in->name != NULL );
  assert( in->scope != NULL );

  if( strcmp( base->name, in->name ) != 0 ) {

    snprintf( msg, 4096, "Modules with different names being merged (%s, %s)\n",
              base->name,
              in->name );
    print_output( msg, FATAL );
    exit( 1 );

  } else if( strcmp( base->scope, in->scope ) != 0 ) {
 
    snprintf( msg, 4096, "Modules with different scopes being merged (%s, %s)\n",
              base->scope,
              in->scope );
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
 \param mod   Pointer to module to write to output.
 \param file  Pointer to specified output file to write contents.
 \return Returns TRUE if file output was successful; otherwise, returns FALSE.

 Prints the database line for the specified module to the specified database
 file.  If there are any problems with the write, returns FALSE; otherwise,
 returns TRUE.
*/
bool module_db_write( module* mod, FILE* file ) {

  bool      retval = TRUE;    /* Return value for this function             */
  sig_link* curr_sig;         /* Pointer to current module sig_link element */
  exp_link* curr_exp;         /* Pointer to current module exp_link element */
  char      msg[4096];        /* Display message string                     */

  snprintf( msg, 4096, "Writing module %s", mod->name );
  print_output( msg, NORMAL );

  // module_display_signals( mod );

  fprintf( file, "%d %s %s %s\n",
    DB_TYPE_MODULE,
    mod->name,
    mod->scope,
    mod->filename
  );

  // module_display_expressions( mod );

  /* Now print all expressions in module */
  curr_exp = mod->exp_head;
  while( curr_exp != NULL ) {
    
    expression_db_write( curr_exp->exp, file, mod->scope );
    curr_exp = curr_exp->next;

  }

  // module_display_signals( mod );

  /* Now print all signals in module */
  curr_sig = mod->sig_head;
  while( curr_sig != NULL ) {

    signal_db_write( curr_sig->sig, file, mod->scope );
    curr_sig = curr_sig->next; 

  }

  return( retval );

}

/*!
 \param mod   Pointer to module to read input from.
 \param line  Pointer to current line to parse.
 \return Returns TRUE if read was successful; otherwise, returns FALSE.

 Reads the current line of the specified file and parses it for a module.
 Creates a new module and assigns the new module to the address specified
 by the mod pointer.  If all is successful, returns TRUE; otherwise, returns
 FALSE.
*/
bool module_db_read( module** mod, char** line ) {

  bool    retval = TRUE;    /* Return value for this function             */
  char    instance[256];    /* Holder for module instance name            */
  char    name[256];        /* Holder for module name                     */
  char    scope[4096];      /* Verilog hierarchical scope for this module */
  char    filename[256];    /* Holder for module filename                 */
  char    signame[256];     /* Holder for signal name                     */
  char    parent[256];      /* Holder for parent name                     */
  char    next[256];        /* Holder for next name                       */
  signal* sig;              /* Pointer to signal                          */
  int     chars_read;       /* Number of characters currently read        */
  int     t;

  *mod = module_create();

  if( (t = sscanf( *line, "%s %s %s%n", name, scope, filename, &chars_read )) == 3 ) {

    *line = *line + chars_read;

    (*mod)->name     = strdup( name );
    (*mod)->scope    = strdup( scope );
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

    /* Free scope name */
    if( mod->scope != NULL ) {
      free_safe( mod->scope );
      mod->scope = NULL;
    }

    /* Free module filename */
    if( mod->filename != NULL ) {
      free_safe( mod->filename );
      mod->filename = NULL;
    }

    /* Free signal list */
    sig_link_delete_list( mod->sig_head );
    mod->sig_head = NULL;

    // module_display_expressions( mod );

    /* Free expression list */
    exp_link_delete_list( mod->exp_head, TRUE );
    mod->exp_head = NULL;

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

