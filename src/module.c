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
#include "param.h"
#include "link.h"


/*!
 \param mod  Pointer to module to initialize.

 Initializes all contents to NULL.
*/  
void module_init( module* mod ) {
    
  mod->name       = NULL;
  mod->filename   = NULL;
  mod->stat       = NULL;
  mod->sig_head   = NULL;
  mod->sig_tail   = NULL;
  mod->exp_head   = NULL;
  mod->exp_tail   = NULL;
  mod->stmt_head  = NULL;
  mod->stmt_tail  = NULL;
  mod->param_head = NULL;
  mod->param_tail = NULL;

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
 \param mod    Pointer to module to write to output.
 \param scope  String version of module scope in hierarchy.
 \param file   Pointer to specified output file to write contents.
 \param inst   Pointer to the current module instance.

 \return Returns TRUE if file output was successful; otherwise, returns FALSE.

 Prints the database line for the specified module to the specified database
 file.  If there are any problems with the write, returns FALSE; otherwise,
 returns TRUE.
*/
bool module_db_write( module* mod, char* scope, FILE* file, mod_inst* inst ) {

  bool       retval = TRUE;   /* Return value for this function                 */
  sig_link*  curr_sig;        /* Pointer to current module sig_link element     */
  exp_link*  curr_exp;        /* Pointer to current module exp_link element     */
  stmt_link* curr_stmt;       /* Pointer to current module stmt_link element    */
  char       msg[4096];       /* Display message string                         */
  inst_parm* icurr;           /* Current instance parameter being evaluated     */
  expression tmpexp;          /* Temporary expression                           */
  int        old_suppl;       /* Contains supplemental value of parameter expr  */
  bool       param_op;        /* Specifies if current expression is a parameter */

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
    
    param_op = ((SUPPL_OP( curr_exp->exp->suppl ) == EXP_OP_PARAM)      ||
                (SUPPL_OP( curr_exp->exp->suppl ) == EXP_OP_PARAM_SBIT) ||
                (SUPPL_OP( curr_exp->exp->suppl ) == EXP_OP_PARAM_MBIT));

    /* If this expression is a parameter, find the associated instance parameter */
    if( param_op ) {
      tmpexp.id = curr_exp->exp->id;
      icurr     = inst->param_head;
      while( (icurr != NULL) && (exp_link_find( &tmpexp, icurr->mparm->exp_head ) == NULL) ) {
        icurr = icurr->next;
      }
      /* If parameter value was found (it should be), adjust expression for new value */
      if( icurr != NULL ) {
        expression_set_value_and_resize( curr_exp->exp, icurr->value );
      } else {
        assert( icurr != NULL );
      }
      /* Finally, change the expression operation to STATIC */
      old_suppl = curr_exp->exp->suppl;
      curr_exp->exp->suppl = (curr_exp->exp->suppl & ~(0x7f << SUPPL_LSB_OP)) | 
                             ((EXP_OP_STATIC & 0x7f) << SUPPL_LSB_OP);
    }

    expression_db_write( curr_exp->exp, file, scope );

    if( param_op ) {
      /* Set operation back to previous */
      curr_exp->exp->suppl = old_suppl;
    }

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
 \param mod    Pointer to module to read contents into.
 \param scope  Pointer to name of read module scope.
 \param line   Pointer to current line to parse.

 \return Returns TRUE if read was successful; otherwise, returns FALSE.

 Reads the current line of the specified file and parses it for a module.
 If all is successful, returns TRUE; otherwise, returns FALSE.
*/
bool module_db_read( module* mod, char* scope, char** line ) {

  bool    retval = TRUE;    /* Return value for this function      */
  int     chars_read;       /* Number of characters currently read */

  if( sscanf( *line, "%s %s %s%n", mod->name, scope, mod->filename, &chars_read ) == 3 ) {

    *line = *line + chars_read;

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Module that will merge in that data from the in module
 \param file  Pointer to CDD file handle to read.

 \return Returns TRUE if parse and merge was successful; otherwise, returns FALSE.

 Parses specified line for module information and performs a merge of the two 
 specified modules, placing the resulting merge module into the module named base.
 If there are any differences between the two modules, a warning or error will be
 displayed to the user.
*/
bool module_db_merge( module* base, FILE* file ) {

  bool       retval = TRUE;   /* Return value of this function                                */
  exp_link*  curr_base_exp;   /* Pointer to current expression in base module expression list */
  sig_link*  curr_base_sig;   /* Pointer to current signal in base module signal list         */
  stmt_link* curr_base_stmt;  /* Pointer to current statement in base module statement list   */
  char*      curr_line;       /* Pointer to current line being read from CDD                  */
  char*      rest_line;       /* Pointer to rest of read line                                 */
  int        type;            /* Specifies currently read CDD type                            */
  int        chars_read;      /* Number of characters read from current CDD line              */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all module expressions */
  curr_base_exp = base->exp_head;
  while( (curr_base_exp != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type == DB_TYPE_EXPRESSION ) {
       
          retval = expression_db_merge( curr_base_exp->exp, &rest_line );

        } else {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_exp = curr_base_exp->next;
  }

  /* Handle all module signals */
  curr_base_sig = base->sig_head;
  while( (curr_base_sig != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type == DB_TYPE_SIGNAL ) {
       
          retval = signal_db_merge( curr_base_sig->sig, &rest_line );

        } else {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_sig = curr_base_sig->next;
  }

  /* Since statements don't get merged, we will just read these lines in */
  curr_base_stmt = base->stmt_head;
  while( (curr_base_stmt != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type != DB_TYPE_STATEMENT ) {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_stmt = curr_base_stmt->next;
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
    // printf( "In module_dealloc, deleting expression list\n" );
    exp_link_delete_list( mod->exp_head, TRUE );
    mod->exp_head = NULL;
    mod->exp_tail = NULL;

    /* Free signal list */
    // printf( "In module_dealloc, deleting signal list\n" );
    sig_link_delete_list( mod->sig_head );
    mod->sig_head = NULL;
    mod->sig_tail = NULL;

    /* Free statement list */
    // printf( "In module_dealloc, deleting statement list\n" );
    stmt_link_delete_list( mod->stmt_head );
    mod->stmt_head = NULL;
    mod->stmt_tail = NULL;

    /* Free parameter list */
    // printf( "In module_dealloc, deleting mod_parm list\n" );
    mod_parm_dealloc( mod->param_head, TRUE );
    mod->param_head = NULL;
    mod->param_tail = NULL;

  }

}

/*!
 \param mod  Pointer to module element to deallocate.

 Deallocates module; name and filename strings; and finally
 the structure itself from the heap.
*/
void module_dealloc( module* mod ) {

  if( mod != NULL ) {

    // printf( "In module_dealloc, name: %s\n", mod->name );

    //module_display_expressions( mod );

    module_clean( mod );

    /* Deallocate module element itself */
    free_safe( mod );

  }

}


/* $Log$
/* Revision 1.16  2002/09/25 05:38:11  phase1geo
/* Cleaning things up a bit.
/*
/* Revision 1.15  2002/09/25 05:36:08  phase1geo
/* Initial version of parameter support is now in place.  Parameters work on a
/* basic level.  param1.v tests this basic functionality and param1.cdd contains
/* the correct CDD output from handling parameters in this file.  Yeah!
/*
/* Revision 1.13  2002/08/26 12:57:04  phase1geo
/* In the middle of adding parameter support.  Intermediate checkin but does
/* not break regressions at this point.
/*
/* Revision 1.12  2002/08/19 04:34:07  phase1geo
/* Fixing bug in database reading code that dealt with merging modules.  Module
/* merging is now performed in a more optimal way.  Full regression passes and
/* own examples pass as well.
/*
/* Revision 1.11  2002/07/23 12:56:22  phase1geo
/* Fixing some memory overflow issues.  Still getting core dumps in some areas.
/*
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

