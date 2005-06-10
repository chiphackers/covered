/*!
 \file     module.c
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
#include "module.h"
#include "util.h"
#include "expr.h"
#include "vsignal.h"
#include "statement.h"
#include "param.h"
#include "link.h"
#include "iter.h"
#include "fsm.h"
#include "race.h"


extern char user_msg[USER_MSG_LENGTH];


/*!
 \param mod  Pointer to module to initialize.

 Initializes all contents to NULL.
*/  
void module_init( module* mod ) {
    
  mod->name       = NULL;
  mod->filename   = NULL;
  mod->start_line = 0;
  mod->end_line   = 0;
  mod->stat       = NULL;
  mod->sig_head   = NULL;
  mod->sig_tail   = NULL;
  mod->exp_head   = NULL;
  mod->exp_tail   = NULL;
  mod->stmt_head  = NULL;
  mod->stmt_tail  = NULL;
  mod->fsm_head   = NULL;
  mod->fsm_tail   = NULL;
  mod->race_head  = NULL;
  mod->race_tail  = NULL;
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
  mod = (module*)malloc_safe( sizeof( module ), __FILE__, __LINE__ );

  module_init( mod );

  return( mod );

}

/*!
 \param mod   Pointer to module containing elements to resize.
 \param inst  Pointer to instance containing this module.
 
 Resizes signals if they are contigent upon parameter values.  After
 all signals have been resized, the signal's corresponding expressions
 are resized.  This function should be called just prior to outputting
 this module's contents to the CDD file (after parsing phase only)
*/
void module_size_elements( module* mod, mod_inst* inst ) {
  
  inst_parm* curr_iparm;   /* Pointer to current instance parameter to evaluate */
  exp_link*  curr_exp;     /* Pointer to current expression link to evaluate    */
  fsm_link*  curr_fsm;     /* Pointer to current FSM structure to evaluate      */
  
  assert( mod  != NULL );
  assert( inst != NULL );
  
  /* 
   First, traverse through current instance's instance parameter list and
   set sizes of signals and expressions.
  */
  curr_iparm = inst->param_head;
  while( curr_iparm != NULL ) {
    assert( curr_iparm->mparm != NULL );
    
    /* This parameter sizes a signal so perform the signal size */
    if( curr_iparm->mparm->sig != NULL ) {
      param_set_sig_size( curr_iparm->mparm->sig, curr_iparm );
    } else {
      /* This parameter attaches to an expression tree */
      curr_exp = curr_iparm->mparm->exp_head;
      while( curr_exp != NULL ) {
        expression_set_value( curr_exp->exp, curr_iparm->value );
        curr_exp = curr_exp->next;
      }
    }
    
    curr_iparm = curr_iparm->next;
  }
  
  /*
   Second, traverse all expressions and set expressions to specified
   signals.  Makes the assumption that all children expressions come
   before the root expression in the list (this is currently the case).
  */
  curr_exp = mod->exp_head;
  while( curr_exp != NULL ) {
    if( ESUPPL_IS_ROOT( curr_exp->exp->suppl ) ) {
      /* Perform an entire expression resize */
      expression_resize( curr_exp->exp, TRUE );
    }
    if( curr_exp->exp->sig != NULL ) {
      expression_set_value( curr_exp->exp, curr_exp->exp->sig->value );
    }
    curr_exp = curr_exp->next;
  }

  /*
   Third, size all FSMs.  Since the FSM structure is reliant on the size
   of the state variable signal to which it is attached, its tables
   cannot be created until the state variable size can be calculated.
   Since this has been done now, size the FSMs.
  */
  curr_fsm = mod->fsm_head;
  while( curr_fsm != NULL ) {
    fsm_create_tables( curr_fsm->table );
    curr_fsm = curr_fsm->next;
  }
    
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

  bool       retval = TRUE;   /* Return value for this function             */
  sig_link*  curr_sig;        /* Pointer to current module sig_link element */
  exp_link*  curr_exp;        /* Pointer to current module exp_link element */
  stmt_iter  curr_stmt;       /* Statement list iterator                    */
  inst_parm* curr_parm;       /* Pointer to current instance parameter      */
  fsm_link*  curr_fsm;        /* Pointer to current module fsm_link element */
  race_blk*  curr_race;       /* Pointer to current race condition block    */

  snprintf( user_msg, USER_MSG_LENGTH, "Writing module %s", mod->name );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );

  fprintf( file, "%d %s %s %s %d %d\n",
    DB_TYPE_MODULE,
    mod->name,
    scope,
    mod->filename,
    mod->start_line,
    mod->end_line
  );

  /* Size all elements in this module if we are in parse mode */
  if( inst != NULL ) {
    module_size_elements( mod, inst );
  }
  
  /* Now print all expressions in module */
  curr_exp = mod->exp_head;
  while( curr_exp != NULL ) {
    expression_db_write( curr_exp->exp, file );
    curr_exp = curr_exp->next;
  }

  /* Now print all parameters in module */
  if( inst != NULL ) {
    curr_parm = inst->param_head;
    while( curr_parm != NULL ) {
      param_db_write( curr_parm, file );
      curr_parm = curr_parm->next;
    }
  }

  /* Now print all signals in module */
  curr_sig = mod->sig_head;
  while( curr_sig != NULL ) {
    vsignal_db_write( curr_sig->sig, file );
    curr_sig = curr_sig->next; 
  }

  /* Now print all statements in module */
  stmt_iter_reset( &curr_stmt, mod->stmt_head );
  while( curr_stmt.curr != NULL ) {
    statement_db_write( curr_stmt.curr->stmt, file );
    stmt_iter_next( &curr_stmt );
  }

  /* Now print all FSM structures in module */
  curr_fsm = mod->fsm_head;
  while( curr_fsm != NULL ) {
    fsm_db_write( curr_fsm->table, file );
    curr_fsm = curr_fsm->next;
  }

  /* Now print all race condition block structures in module */
  curr_race = mod->race_head;
  while( curr_race != NULL ) {
    race_db_write( curr_race, file );
    curr_race = curr_race->next;
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

  if( sscanf( *line, "%s %s %s %d %d%n", mod->name, scope, mod->filename, &(mod->start_line), &(mod->end_line), &chars_read ) == 5 ) {

    *line = *line + chars_read;

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Module that will merge in that data from the in module
 \param file  Pointer to CDD file handle to read.
 \param same  Specifies if module to be merged should match existing module exactly or not.

 \return Returns TRUE if parse and merge was successful; otherwise, returns FALSE.

 Parses specified line for module information and performs a merge of the two 
 specified modules, placing the resulting merge module into the module named base.
 If there are any differences between the two modules, a warning or error will be
 displayed to the user.
*/
bool module_db_merge( module* base, FILE* file, bool same ) {

  bool      retval = TRUE;   /* Return value of this function                                */
  exp_link* curr_base_exp;   /* Pointer to current expression in base module expression list */
  sig_link* curr_base_sig;   /* Pointer to current signal in base module signal list         */
  stmt_iter curr_base_stmt;  /* Statement list iterator                                      */
  fsm_link* curr_base_fsm;   /* Pointer to current FSM in base module FSM list               */
  race_blk* curr_base_race;  /* Pointer to current race condition block in base module list  */
  char*     curr_line;       /* Pointer to current line being read from CDD                  */
  char*     rest_line;       /* Pointer to rest of read line                                 */
  int       type;            /* Specifies currently read CDD type                            */
  int       chars_read;      /* Number of characters read from current CDD line              */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all module expressions */
  curr_base_exp = base->exp_head;
  while( (curr_base_exp != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type == DB_TYPE_EXPRESSION ) {
          retval = expression_db_merge( curr_base_exp->exp, &rest_line, same );
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
          retval = vsignal_db_merge( curr_base_sig->sig, &rest_line, same );
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
  stmt_iter_reset( &curr_base_stmt, base->stmt_head );
  while( (curr_base_stmt.curr != NULL) && retval ) {
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
    stmt_iter_next( &curr_base_stmt );
  }

  /* Handle all module FSMs */
  curr_base_fsm = base->fsm_head;
  while( (curr_base_fsm != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type == DB_TYPE_FSM ) {
          retval = fsm_db_merge( curr_base_fsm->table, &rest_line, same );
        } else {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_fsm = curr_base_fsm->next;
  }

  /* Since race condition blocks don't get merged, we will just read these lines in */
  curr_base_race = base->race_head;
  while( (curr_base_race != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type != DB_TYPE_RACE ) {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_race = curr_base_race->next;
  } 

  return( retval );

}

/*!
 \param base  Module that will be replaced with the new data.
 \param file  Pointer to CDD file handle to read.

 \return Returns TRUE if parse and merge was successful; otherwise, returns FALSE.

 Parses specified line for module information and performs a replacement of the original
 module with the contents of the new module.  If there are any differences between the
 two modules, a warning or error will be displayed to the user.
*/
bool module_db_replace( module* base, FILE* file ) {

  bool      retval = TRUE;   /* Return value of this function                                */
  exp_link* curr_base_exp;   /* Pointer to current expression in base module expression list */
  sig_link* curr_base_sig;   /* Pointer to current signal in base module signal list         */
  stmt_iter curr_base_stmt;  /* Statement list iterator                                      */
  fsm_link* curr_base_fsm;   /* Pointer to current FSM in base module FSM list               */
  race_blk* curr_base_race;  /* Pointer to current race condition block in base module list  */
  char*     curr_line;       /* Pointer to current line being read from CDD                  */
  char*     rest_line;       /* Pointer to rest of read line                                 */
  int       type;            /* Specifies currently read CDD type                            */
  int       chars_read;      /* Number of characters read from current CDD line              */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all module expressions */
  curr_base_exp = base->exp_head;
  while( (curr_base_exp != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type == DB_TYPE_EXPRESSION ) {
          retval = expression_db_replace( curr_base_exp->exp, &rest_line );
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
          retval = vsignal_db_replace( curr_base_sig->sig, &rest_line );
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

  /* Since statements don't get replaced, we will just read these lines in */
  stmt_iter_reset( &curr_base_stmt, base->stmt_head );
  while( (curr_base_stmt.curr != NULL) && retval ) {
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
    stmt_iter_next( &curr_base_stmt );
  }

  /* Handle all module FSMs */
  curr_base_fsm = base->fsm_head;
  while( (curr_base_fsm != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type == DB_TYPE_FSM ) {
          retval = fsm_db_replace( curr_base_fsm->table, &rest_line );
        } else {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_fsm = curr_base_fsm->next;
  }

  /* Since race condition blocks don't get replaced, we will just read these lines in */
  curr_base_race = base->race_head;
  while( (curr_base_race != NULL) && retval ) {
    if( readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type != DB_TYPE_RACE ) {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }
    curr_base_race = curr_base_race->next;
  }

  return( retval );

}

/*!
 \param mod  Pointer to module element to display signals.

 Iterates through signal list of specified module, displaying each signal's
 name, width, lsb and value.
*/
void module_display_signals( module* mod ) {

  sig_link* sigl;  /* Pointer to current signal link element */

  printf( "Module => %s\n", mod->name );

  sigl = mod->sig_head;
  while( sigl != NULL ) {
    vsignal_display( sigl->sig );
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
    mod->exp_tail = NULL;

    /* Free signal list */
    sig_link_delete_list( mod->sig_head, TRUE );
    mod->sig_head = NULL;
    mod->sig_tail = NULL;

    /* Free statement list */
    stmt_link_delete_list( mod->stmt_head );
    mod->stmt_head = NULL;
    mod->stmt_tail = NULL;

    /* Free parameter list */
    mod_parm_dealloc( mod->param_head, TRUE );
    mod->param_head = NULL;
    mod->param_tail = NULL;

    /* Free FSM list */
    fsm_link_delete_list( mod->fsm_head );
    mod->fsm_head = NULL;
    mod->fsm_tail = NULL;

    /* Free race condition block list */
    race_blk_delete_list( mod->race_head );
    mod->race_head = NULL;
    mod->race_tail = NULL;

  }

}

/*!
 \param mod  Pointer to module element to deallocate.

 Deallocates module; name and filename strings; and finally
 the structure itself from the heap.
*/
void module_dealloc( module* mod ) {

  if( mod != NULL ) {

    module_clean( mod );

    /* Deallocate module element itself */
    free_safe( mod );

  }

}


/*
 $Log$
 Revision 1.38  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.37  2005/02/04 23:55:53  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.36  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.35  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.34  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.33  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.32  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.31  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.30  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.29  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.28  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.27  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.26  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.25  2002/10/31 23:13:57  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.24  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.23  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.22  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.21  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.20  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.19  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.18  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.17  2002/09/26 13:43:45  phase1geo
 Making code adjustments to correctly support parameter overriding.  Added
 parameter tests to verify supported functionality.  Full regression passes.

 Revision 1.16  2002/09/25 05:38:11  phase1geo
 Cleaning things up a bit.

 Revision 1.15  2002/09/25 05:36:08  phase1geo
 Initial version of parameter support is now in place.  Parameters work on a
 basic level.  param1.v tests this basic functionality and param1.cdd contains
 the correct CDD output from handling parameters in this file.  Yeah!

 Revision 1.13  2002/08/26 12:57:04  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.12  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.11  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.10  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.9  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.8  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.7  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.6  2002/06/26 03:45:48  phase1geo
 Fixing more bugs in simulator and report functions.  About to add support
 for delay statements.

 Revision 1.5  2002/06/25 02:02:04  phase1geo
 Fixing bugs with writing/reading statements and with parsing design with
 statements.  We now get to the scoring section.  Some problems here at
 the moment with the simulator.

 Revision 1.4  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.3  2002/05/02 03:27:42  phase1geo
 Initial creation of statement structure and manipulation files.  Internals are
 still in a chaotic state.
*/

