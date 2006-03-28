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
 \file     func_unit.c
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
#include "func_unit.h"
#include "util.h"
#include "expr.h"
#include "vsignal.h"
#include "statement.h"
#include "param.h"
#include "link.h"
#include "iter.h"
#include "fsm.h"
#include "race.h"


extern char        user_msg[USER_MSG_LENGTH];
extern funit_link* funit_head;


/*!
 \param funit  Pointer to functional unit to initialize.

 Initializes all contents to NULL.
*/  
void funit_init( func_unit* funit ) {
    
  funit->type       = FUNIT_MODULE;
  funit->name       = NULL;
  funit->filename   = NULL;
  funit->start_line = 0;
  funit->end_line   = 0;
  funit->stat       = NULL;
  funit->sig_head   = NULL;
  funit->sig_tail   = NULL;
  funit->exp_head   = NULL;
  funit->exp_tail   = NULL;
  funit->stmt_head  = NULL;
  funit->stmt_tail  = NULL;
  funit->fsm_head   = NULL;
  funit->fsm_tail   = NULL;
  funit->race_head  = NULL;
  funit->race_tail  = NULL;
  funit->param_head = NULL;
  funit->param_tail = NULL;
  funit->tf_head    = NULL;
  funit->tf_tail    = NULL;
  funit->parent     = NULL;

}

/*!
 \return Returns pointer to newly created functional unit element that has been
         properly initialized.

 Allocates memory from the heap for a functional unit element and initializes all
 contents to NULL.  Returns a pointer to the newly created functional unit.
*/
func_unit* funit_create() {

  func_unit* funit;   /* Pointer to newly created functional unit element */

  /* Create and initialize functional unit */
  funit = (func_unit*)malloc_safe( sizeof( func_unit ), __FILE__, __LINE__ );

  funit_init( funit );

  return( funit );

}

/*!
 \param funit  Pointer to functional unit to get its module from

 \return Returns a pointer to the module that contains the specified functional unit.

 Traverses up parent list until the FUNIT_MODULE is found (parent should be NULL).
*/
func_unit* funit_get_curr_module( func_unit* funit ) {

  assert( funit != NULL );

  while( funit->parent != NULL ) {
    funit = funit->parent;
  }

  return( funit );

}

/*!
 \param funit  Functional unit that may be nested in a function

 \return Returns a pointer to the function that contains the specified functional unit if
         one exists; otherwise, returns NULL.
*/
func_unit* funit_get_curr_function( func_unit* funit ) {

  assert( funit != NULL );

  while( (funit->type != FUNIT_FUNCTION) && (funit->type != FUNIT_MODULE) ) {
    funit = funit->parent;
  }

  return( (funit->type == FUNIT_FUNCTION) ? funit : NULL );

}

/*!
 \param funit  Pointer to functional unit to process

 \return Returns the number of input, output and inout ports specified in this functional unit
*/
int funit_get_port_count( func_unit* funit ) {

  sig_link* sigl;          /* Pointer to current signal link to examine */
  int       port_cnt = 0;  /* Return value for this function */

  assert( funit != NULL );

  sigl = funit->sig_head;
  while( sigl != NULL ) {
    if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_INPUT)  ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT) ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_INOUT) ) {
      port_cnt++;
    }
    sigl = sigl->next;
  }

  return( port_cnt );

}

/*!
 \param name   Name of parameter to search for
 \param funit  Functional unit to check for existence of named parameter

 \return Returns a pointer to the module parameter structure that contains the specified
         parameter name if it exists; otherwise, returns NULL.

 Recursively searches from the current functional unit up through its scope until either
 the parameter is found or until we have exhausted the scope.
*/
mod_parm* funit_find_param( char* name, func_unit* funit ) {

  mod_parm* mparm = NULL;  /* Pointer to found module parameter */

  if( funit != NULL ) {

    if( (mparm = mod_parm_find( name, funit->param_head )) == NULL ) {
      mparm = funit_find_param( name, funit->parent );
    }

  }

  return( mparm );

}

/*!
 \param orig_name  Verilog name of task, function or named-block.
 \param parent     Pointer to parent functional unit of this functional unit.

 \return Returns dynamically allocated string containing internally used task, function or named-block name.
*/
char* funit_gen_task_function_namedblock_name( char* orig_name, func_unit* parent ) {

  char full_name[4096];  /* Container for new name */

  assert( parent != NULL );
  assert( orig_name != NULL );

  /* Generate full name to use for the function/task */
  snprintf( full_name, 4096, "%s.%s", parent->name, orig_name );

  return( strdup_safe( full_name, __FILE__, __LINE__ ) );

}

/*!
 \param funit  Pointer to functional unit containing elements to resize.
 \param inst   Pointer to instance containing this functional unit.
 
 Resizes signals if they are contigent upon parameter values.  After
 all signals have been resized, the signal's corresponding expressions
 are resized.  This function should be called just prior to outputting
 this funtional unit's contents to the CDD file (after parsing phase only)
*/
void funit_size_elements( func_unit* funit, funit_inst* inst ) {
  
  inst_parm* curr_iparm;   /* Pointer to current instance parameter to evaluate */
  exp_link*  curr_exp;     /* Pointer to current expression link to evaluate */
  fsm_link*  curr_fsm;     /* Pointer to current FSM structure to evaluate */
  
  assert( funit != NULL );
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
        expression_set_value( curr_exp->exp, curr_iparm->sig->value );
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
  curr_exp = funit->exp_head;
  while( curr_exp != NULL ) {
    if( ESUPPL_IS_ROOT( curr_exp->exp->suppl ) ) {
      /* Perform an entire expression resize */
      expression_resize( curr_exp->exp, TRUE );
    }
    if( curr_exp->exp->sig != NULL ) {
      expression_set_value( curr_exp->exp, curr_exp->exp->sig->value );
      assert( curr_exp->exp->value->value != NULL );
    }
    curr_exp = curr_exp->next;
  }

  /*
   Third, size all FSMs.  Since the FSM structure is reliant on the size
   of the state variable signal to which it is attached, its tables
   cannot be created until the state variable size can be calculated.
   Since this has been done now, size the FSMs.
  */
  curr_fsm = funit->fsm_head;
  while( curr_fsm != NULL ) {
    fsm_create_tables( curr_fsm->table );
    curr_fsm = curr_fsm->next;
  }
    
}

/*!
 \param funit  Pointer to functional unit to write to output.
 \param scope  String version of functional unit scope in hierarchy.
 \param file   Pointer to specified output file to write contents.
 \param inst   Pointer to the current functional unit instance.

 \return Returns TRUE if file output was successful; otherwise, returns FALSE.

 Prints the database line for the specified functional unit to the specified database
 file.  If there are any problems with the write, returns FALSE; otherwise,
 returns TRUE.
*/
bool funit_db_write( func_unit* funit, char* scope, FILE* file, funit_inst* inst ) {

  bool       retval = TRUE;  /* Return value for this function */
  sig_link*  curr_sig;       /* Pointer to current functional unit sig_link element */
  exp_link*  curr_exp;       /* Pointer to current functional unit exp_link element */
  stmt_iter  curr_stmt;      /* Statement list iterator */
  inst_parm* curr_parm;      /* Pointer to current instance parameter */
  fsm_link*  curr_fsm;       /* Pointer to current functional unit fsm_link element */
  race_blk*  curr_race;      /* Pointer to current race condition block */

#ifdef DEBUG_MODE
  switch( funit->type ) {
    case FUNIT_MODULE      :  snprintf( user_msg, USER_MSG_LENGTH, "Writing module %s", funit->name );       break;
    case FUNIT_NAMED_BLOCK :  snprintf( user_msg, USER_MSG_LENGTH, "Writing named block %s", funit->name );  break;
    case FUNIT_FUNCTION    :  snprintf( user_msg, USER_MSG_LENGTH, "Writing function %s", funit->name );     break;
    case FUNIT_TASK        :  snprintf( user_msg, USER_MSG_LENGTH, "Writing task %s", funit->name );         break;
    default                :
      snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  Unknown functional unit type %d", funit->type );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      exit( 1 );
      break;
  }
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  fprintf( file, "%d %d %s %s %s %d %d\n",
    DB_TYPE_FUNIT,
    funit->type,
    funit->name,
    scope,
    funit->filename,
    funit->start_line,
    funit->end_line
  );

  /* Size all elements in this functional unit if we are in parse mode */
  if( inst != NULL ) {
    funit_size_elements( funit, inst );
  }
  
  /* Now print all expressions in functional unit */
  curr_exp = funit->exp_head;
  while( curr_exp != NULL ) {
    expression_db_write( curr_exp->exp, file, (inst != NULL) );
    curr_exp = curr_exp->next;
  }

  /* Now print all parameters in functional unit */
  if( inst != NULL ) {
    curr_parm = inst->param_head;
    while( curr_parm != NULL ) {
      param_db_write( curr_parm, file, (inst != NULL) );
      curr_parm = curr_parm->next;
    }
  }

  /* Now print all signals in functional unit */
  curr_sig = funit->sig_head;
  while( curr_sig != NULL ) {
    vsignal_db_write( curr_sig->sig, file );
    curr_sig = curr_sig->next; 
  }

  /* Now print all statements in functional unit */
  stmt_iter_reset( &curr_stmt, funit->stmt_head );
  while( curr_stmt.curr != NULL ) {
    statement_db_write( curr_stmt.curr->stmt, file, (inst != NULL) );
    stmt_iter_next( &curr_stmt );
  }

  /* Now print all FSM structures in functional unit */
  curr_fsm = funit->fsm_head;
  while( curr_fsm != NULL ) {
    fsm_db_write( curr_fsm->table, file, (inst != NULL) );
    curr_fsm = curr_fsm->next;
  }

  /* Now print all race condition block structures in functional unit (if we are a module) */
  if( funit->type == FUNIT_MODULE ) {
    curr_race = funit->race_head;
    while( curr_race != NULL ) {
      race_db_write( curr_race, file );
      curr_race = curr_race->next;
    }
  }

  return( retval );

}

/*!
 \param funit  Pointer to functional unit to read contents into.
 \param scope  Pointer to name of read functional unit scope.
 \param line   Pointer to current line to parse.

 \return Returns TRUE if read was successful; otherwise, returns FALSE.

 Reads the current line of the specified file and parses it for a functional unit.
 If all is successful, returns TRUE; otherwise, returns FALSE.
*/
bool funit_db_read( func_unit* funit, char* scope, char** line ) {

  bool retval = TRUE;  /* Return value for this function */
  int  chars_read;     /* Number of characters currently read */
  int  params;         /* Number of parameters in string that were parsed */

  if( (params = sscanf( *line, "%d %s %s %s %d %d%n", &(funit->type), funit->name, scope, funit->filename,
              &(funit->start_line), &(funit->end_line), &chars_read )) == 6 ) {

    *line = *line + chars_read;

  } else {

    snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Incorrect number of parameters for func_unit, should be 6 but is %d\n", params );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Module that will merge in that data from the in functional unit
 \param file  Pointer to CDD file handle to read.
 \param same  Specifies if functional unit to be merged should match existing functional unit exactly or not.

 \return Returns TRUE if parse and merge was successful; otherwise, returns FALSE.

 Parses specified line for functional unit information and performs a merge of the two 
 specified functional units, placing the resulting merge functional unit into the functional unit named base.
 If there are any differences between the two functional units, a warning or error will be
 displayed to the user.
*/
bool funit_db_merge( func_unit* base, FILE* file, bool same ) {

  bool      retval = TRUE;   /* Return value of this function */
  exp_link* curr_base_exp;   /* Pointer to current expression in base functional unit expression list */
  sig_link* curr_base_sig;   /* Pointer to current signal in base functional unit signal list */
  stmt_iter curr_base_stmt;  /* Statement list iterator */
  fsm_link* curr_base_fsm;   /* Pointer to current FSM in base functional unit FSM list */
  race_blk* curr_base_race;  /* Pointer to current race condition block in base module list  */
  char*     curr_line;       /* Pointer to current line being read from CDD */
  char*     rest_line;       /* Pointer to rest of read line */
  int       type;            /* Specifies currently read CDD type */
  int       chars_read;      /* Number of characters read from current CDD line */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all functional unit expressions */
  curr_base_exp = base->exp_head;
  while( (curr_base_exp != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
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
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_exp = curr_base_exp->next;
  }

  /* Handle all functional unit signals */
  curr_base_sig = base->sig_head;
  while( (curr_base_sig != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
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
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_sig = curr_base_sig->next;
  }

  /* Since statements don't get merged, we will just read these lines in */
  stmt_iter_reset( &curr_base_stmt, base->stmt_head );
  while( (curr_base_stmt.curr != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type != DB_TYPE_STATEMENT ) {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    stmt_iter_next( &curr_base_stmt );
  }

  /* Handle all functional unit FSMs */
  curr_base_fsm = base->fsm_head;
  while( (curr_base_fsm != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
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
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_fsm = curr_base_fsm->next;
  }

  /* Since race condition blocks don't get merged, we will just read these lines in */
  if( base->type == FUNIT_MODULE ) {
    curr_base_race = base->race_head;
    while( (curr_base_race != NULL) && retval ) {
      if( util_readline( file, &curr_line ) ) {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type != DB_TYPE_RACE ) {
            retval = FALSE;
          }
        } else {
          retval = FALSE;
        }
        free_safe( curr_line );
      } else {
        retval = FALSE;
      }
      curr_base_race = curr_base_race->next;
    }
  }

  return( retval );

}

/*!
 \param base  Functional unit that will be replaced with the new data.
 \param file  Pointer to CDD file handle to read.

 \return Returns TRUE if parse and merge was successful; otherwise, returns FALSE.

 Parses specified line for functional unit information and performs a replacement of the original
 functional unit with the contents of the new functional unit.  If there are any differences between the
 two functional units, a warning or error will be displayed to the user.
*/
bool funit_db_replace( func_unit* base, FILE* file ) {

  bool      retval = TRUE;   /* Return value of this function */
  exp_link* curr_base_exp;   /* Pointer to current expression in base functional unit expression list */
  sig_link* curr_base_sig;   /* Pointer to current signal in base functional unit signal list */
  stmt_iter curr_base_stmt;  /* Statement list iterator */
  fsm_link* curr_base_fsm;   /* Pointer to current FSM in base functional unit FSM list */
  race_blk* curr_base_race;  /* Pointer to current race condition block in base functional unit list  */
  char*     curr_line;       /* Pointer to current line being read from CDD */
  char*     rest_line;       /* Pointer to rest of read line */
  int       type;            /* Specifies currently read CDD type */
  int       chars_read;      /* Number of characters read from current CDD line */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all functional unit expressions */
  curr_base_exp = base->exp_head;
  while( (curr_base_exp != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
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
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_exp = curr_base_exp->next;
  }

  /* Handle all functional unit signals */
  curr_base_sig = base->sig_head;
  while( (curr_base_sig != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
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
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_sig = curr_base_sig->next;
  }

  /* Since statements don't get replaced, we will just read these lines in */
  stmt_iter_reset( &curr_base_stmt, base->stmt_head );
  while( (curr_base_stmt.curr != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type != DB_TYPE_STATEMENT ) {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    stmt_iter_next( &curr_base_stmt );
  }

  /* Handle all functional unit FSMs */
  curr_base_fsm = base->fsm_head;
  while( (curr_base_fsm != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
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
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_fsm = curr_base_fsm->next;
  }

  /* Since race condition blocks don't get replaced, we will just read these lines in */
  curr_base_race = base->race_head;
  while( (curr_base_race != NULL) && retval ) {
    if( util_readline( file, &curr_line ) ) {
      if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
        rest_line = curr_line + chars_read;
        if( type != DB_TYPE_RACE ) {
          retval = FALSE;
        }
      } else {
        retval = FALSE;
      }
      free_safe( curr_line );
    } else {
      retval = FALSE;
    }
    curr_base_race = curr_base_race->next;
  }

  return( retval );

}

/*!
 \param id  Expression/statement ID to search for
 
 \return Returns a pointer to the functional unit that contains the specified expression/statement
         ID if one exists; otherwise, returns NULL.

 Searches the functional units until one is found that contains the expression/statement identified
 by the specified ID and returns a pointer to this functional unit.  If no such ID exists in the
 design, a value of NULL is returned to the calling statement.
*/
func_unit* funit_find_by_id( int id ) {

  funit_link* funitl;       /* Temporary pointer to functional unit link */
  exp_link*   expl = NULL;  /* Temporary pointer to expression link */
  expression  exp;          /* Temporary expression used for comparison purposes */

  exp.id = id;

  funitl = funit_head;
  while( (funitl != NULL) && (expl == NULL) ) {
    if( (expl = exp_link_find( &exp, funitl->funit->exp_head )) == NULL ) {
      funitl = funitl->next;
    }
  }
      
  return( (funitl == NULL) ? NULL : funitl->funit );
    
}

/*!
 \param funit  Pointer to functional unit element to display signals.

 Iterates through signal list of specified functional unit, displaying each signal's
 name, width, lsb and value.
*/
void funit_display_signals( func_unit* funit ) {

  sig_link* sigl;  /* Pointer to current signal link element */

  switch( funit->type ) {
    case FUNIT_MODULE      :  printf( "Module => %s\n", funit->name );       break;
    case FUNIT_NAMED_BLOCK :  printf( "Named block => %s\n", funit->name );  break;
    case FUNIT_FUNCTION    :  printf( "Function => %s\n", funit->name );     break;
    case FUNIT_TASK        :  printf( "Task => %s\n", funit->name );         break;
    default                :  printf( "UNKNOWN => %s\n", funit->name );      break;
  }

  sigl = funit->sig_head;
  while( sigl != NULL ) {
    vsignal_display( sigl->sig );
    sigl = sigl->next;
  }

}

/*!
 \param funit  Pointer to functional unit element to display expressions

 Iterates through expression list of specified functional unit, displaying each expression's
 id.
*/
void funit_display_expressions( func_unit* funit ) {

  exp_link* expl;    /* Pointer to current expression link element */

  switch( funit->type ) {
    case FUNIT_MODULE      :  printf( "Module => %s\n", funit->name );       break;
    case FUNIT_NAMED_BLOCK :  printf( "Named block => %s\n", funit->name );  break;
    case FUNIT_FUNCTION    :  printf( "Function => %s\n", funit->name );     break;
    case FUNIT_TASK        :  printf( "Task => %s\n", funit->name );         break;
    default                :  printf( "UNKNOWN => %s\n", funit->name );      break;
  }

  expl = funit->exp_head;
  while( expl != NULL ) {
    expression_display( expl->exp );
    expl = expl->next;
  }

}

/*!
 \param funit  Pointer to functional unit element to clean.

 Deallocates functional unit contents: name and filename strings.
*/
void funit_clean( func_unit* funit ) {

  if( funit != NULL ) {

    /* Free functional unit name */
    if( funit->name != NULL ) {
      free_safe( funit->name );
      funit->name = NULL;
    }

    /* Free functional unit filename */
    if( funit->filename != NULL ) {
      free_safe( funit->filename );
      funit->filename = NULL;
    }

    /* Free signal list */
    sig_link_delete_list( funit->sig_head, TRUE );
    funit->sig_head = NULL;
    funit->sig_tail = NULL;

    /* Free FSM list */
    fsm_link_delete_list( funit->fsm_head );
    funit->fsm_head = NULL;
    funit->fsm_tail = NULL;

    /* Free expression list */
    exp_link_delete_list( funit->exp_head, TRUE );
    funit->exp_head = NULL;
    funit->exp_tail = NULL;

    /* Free statement list */
    stmt_link_delete_list( funit->stmt_head );
    funit->stmt_head = NULL;
    funit->stmt_tail = NULL;

    /* Free parameter list */
    mod_parm_dealloc( funit->param_head, TRUE );
    funit->param_head = NULL;
    funit->param_tail = NULL;

    /* Free race condition block list */
    race_blk_delete_list( funit->race_head );
    funit->race_head = NULL;
    funit->race_tail = NULL;

    /* Free statistic structure */
    statistic_dealloc( funit->stat );

    /* Free tf elements */
    funit_link_delete_list( funit->tf_head, FALSE );

  }

}

/*!
 \param funit  Pointer to functional unit element to deallocate.

 Deallocates functional unit; name and filename strings; and finally
 the structure itself from the heap.
*/
void funit_dealloc( func_unit* funit ) {

  if( funit != NULL ) {

    funit_clean( funit );

    /* Deallocate functional unit element itself */
    free_safe( funit );

  }

}


/*
 $Log$
 Revision 1.17  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.16  2006/02/01 15:13:11  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.15  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.14  2006/01/23 03:53:30  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.13  2006/01/20 19:15:23  phase1geo
 Fixed bug to properly handle the scoping of parameters when parameters are created/used
 in non-module functional units.  Added param10*.v diagnostics to regression suite to
 verify the behavior is correct now.

 Revision 1.12  2006/01/13 23:27:02  phase1geo
 Initial attempt to fix problem with handling functions/tasks/named blocks with
 the same name in the design.  Still have a few diagnostics failing in regressions
 to contend with.  Updating regression with these changes.

 Revision 1.11  2005/12/22 23:04:42  phase1geo
 More memory leak fixes.

 Revision 1.10  2005/12/19 23:11:27  phase1geo
 More fixes for memory faults.  Full regression passes.  Errors have now been
 eliminated from regression -- just left-over memory issues remain.

 Revision 1.9  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.8  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.7  2005/12/01 21:11:16  phase1geo
 Adding more error checking diagnostics into regression suite.  Full regression
 passes.

 Revision 1.6  2005/12/01 20:49:02  phase1geo
 Adding nested_block3 to verify nested named blocks in tasks.  Fixed named block
 usage to be FUNC_CALL or TASK_CALL -like based on its placement.

 Revision 1.5  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.4  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.3  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.2  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.1  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

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

