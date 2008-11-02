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

#include "db.h"
#include "defines.h"
#include "enumerate.h"
#include "exclude.h"
#include "expr.h"
#include "fsm.h"
#include "func_unit.h"
#include "gen_item.h"
#include "instance.h"
#include "iter.h"
#include "link.h"
#include "obfuscate.h"
#include "param.h"
#include "parser_misc.h"
#include "race.h"
#include "sim.h"
#include "stat.h"
#include "statement.h"
#include "stmt_blk.h"
#include "util.h"
#include "vsignal.h"


extern char         user_msg[USER_MSG_LENGTH];
extern db**         db_list;
extern unsigned int curr_db;
extern func_unit*   curr_funit;


/*!
 Initializes all contents to NULL.
*/  
static void funit_init(
  func_unit* funit  /*!< Pointer to functional unit to initialize */
) { PROFILE(FUNIT_INIT);
    
  funit->type       = FUNIT_MODULE;
  funit->name       = NULL;
  funit->filename   = NULL;
  funit->version    = NULL;
  funit->start_line = 0;
  funit->end_line   = 0;
  funit->stat       = NULL;
  funit->sig_head   = NULL;
  funit->sig_tail   = NULL;
  funit->exp_head   = NULL;
  funit->exp_tail   = NULL;
  funit->first_stmt = NULL;
  funit->stmt_head  = NULL;
  funit->stmt_tail  = NULL;
  funit->fsm_head   = NULL;
  funit->fsm_tail   = NULL;
  funit->race_head  = NULL;
  funit->race_tail  = NULL;
  funit->param_head = NULL;
  funit->param_tail = NULL;
  funit->gitem_head = NULL;
  funit->gitem_tail = NULL;
  funit->tf_head    = NULL;
  funit->tf_tail    = NULL;
  funit->tdi_head   = NULL;
  funit->tdi_tail   = NULL;
  funit->ei_head    = NULL;
  funit->ei_tail    = NULL;
  funit->er_head    = NULL;
  funit->er_tail    = NULL;
  funit->parent     = NULL;
  funit->elem_type  = 0;
  funit->elem.thr   = NULL;

  PROFILE_END;

}

/*!
 \return Returns pointer to newly created functional unit element that has been
         properly initialized.

 Allocates memory from the heap for a functional unit element and initializes all
 contents to NULL.  Returns a pointer to the newly created functional unit.
*/
func_unit* funit_create() { PROFILE(FUNIT_CREATE);

  func_unit* funit;   /* Pointer to newly created functional unit element */

  /* Create and initialize functional unit */
  funit = (func_unit*)malloc_safe( sizeof( func_unit ) );

  funit_init( funit );

  PROFILE_END;

  return( funit );

}

/*!
 \return Returns a pointer to the module that contains the specified functional unit.

 Traverses up parent list until the FUNIT_MODULE is found (parent should be NULL).
*/
func_unit* funit_get_curr_module(
  func_unit* funit  /*!< Pointer to functional unit to get its module from */
) { PROFILE(FUNIT_GET_CURR_MODULE);

  assert( funit != NULL );

  while( funit->parent != NULL ) {
    funit = funit->parent;
  }

  PROFILE_END;

  return( funit );

}

/*!
 \return Returns a const pointer to the module that contains the specified functional unit.

 Traverses up parent list until the FUNIT_MODULE is found (parent should be NULL).  Does this
 in a way that guarantees that the found functional unit will not be modified.
*/
const func_unit* funit_get_curr_module_safe(
  const func_unit* funit  /*!< Pointer to functional unit to get its module from */
) { PROFILE(FUNIT_GET_CURR_MODULE_SAFE);

  assert( funit != NULL );

  while( funit->parent != NULL ) {
    funit = funit->parent;
  }

  PROFILE_END;

  return( funit );

}

/*!
 \return Returns a pointer to the function that contains the specified functional unit if
         one exists; otherwise, returns NULL.
*/
func_unit* funit_get_curr_function(
  func_unit* funit  /*!< Functional unit that may be nested in a function */
) { PROFILE(FUNIT_GET_CURR_FUNCTION);

  assert( funit != NULL );

  while( (funit->type != FUNIT_FUNCTION) && (funit->type != FUNIT_AFUNCTION) && (funit->type != FUNIT_MODULE) ) {
    funit = funit->parent;
  }

  PROFILE_END;

  return( ((funit->type == FUNIT_FUNCTION) || (funit->type == FUNIT_AFUNCTION)) ? funit : NULL );

}

/*!
 \return Returns a pointer to the function that contains the specified functional unit if
         one exists; otherwise, returns NULL.
*/
func_unit* funit_get_curr_task(
  func_unit* funit  /*!< Functional unit that may be nested in a function */
) { PROFILE(FUNIT_GET_CURR_TASK);

  assert( funit != NULL );

  while( (funit->type != FUNIT_TASK) && (funit->type != FUNIT_ATASK) && (funit->type != FUNIT_MODULE) ) {
    funit = funit->parent;
  }

  PROFILE_END;

  return( ((funit->type == FUNIT_TASK) || (funit->type == FUNIT_ATASK)) ? funit : NULL );

}

/*!
 \return Returns the number of input, output and inout ports specified in this functional unit
*/
int funit_get_port_count(
  func_unit* funit  /*!< Pointer to functional unit to process */
) { PROFILE(FUNIT_GET_PORT_COUNT);

  sig_link* sigl;          /* Pointer to current signal link to examine */
  int       port_cnt = 0;  /* Return value for this function */

  assert( funit != NULL );

  sigl = funit->sig_head;
  while( sigl != NULL ) {
    if( (sigl->sig->suppl.part.type == SSUPPL_TYPE_INPUT_NET)  ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_INPUT_REG)  ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT_NET) ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT_REG) ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_INOUT_NET)  ||
        (sigl->sig->suppl.part.type == SSUPPL_TYPE_INOUT_REG) ) {
      port_cnt++;
    }
    sigl = sigl->next;
  }

  PROFILE_END;

  return( port_cnt );

}

/*!
 \return Returns a pointer to the module parameter structure that contains the specified
         parameter name if it exists; otherwise, returns NULL.

 Recursively searches from the current functional unit up through its scope until either
 the parameter is found or until we have exhausted the scope.
*/
mod_parm* funit_find_param(
  char*      name,  /*!< Name of parameter to search for */
  func_unit* funit  /*!< Functional unit to check for existence of named parameter */
) { PROFILE(FUNIT_FIND_PARAM);

  mod_parm* mparm = NULL;  /* Pointer to found module parameter */

  if( funit != NULL ) {

    if( (mparm = mod_parm_find( name, funit->param_head )) == NULL ) {
      mparm = funit_find_param( name, funit->parent );
    }

  }

  PROFILE_END;

  return( mparm );

}

/*!
 \return Returns a pointer to the found signal in the given functional unit; otherwise,
         returns NULL if the signal could not be found.

 Searches the signal list in the given functional unit for the specified signal name.  If
 it isn't found there, we look in the generate item list for the same signal.
*/
vsignal* funit_find_signal(
  char*      name,  /*!< Name of the signal that we are searching for */
  func_unit* funit  /*!< Pointer to functional unit to search in */
) { PROFILE(FUNIT_FIND_SIGNAL);

  vsignal*    found_sig = NULL;  /* Pointer to the found signal */
  vsignal     sig;               /* Holder for signal to search for */
  sig_link*   sigl;              /* Pointer to signal link */
#ifndef VPI_ONLY
  gen_item*   gi;                /* Pointer to temporary generate item */
  gen_item*   found_gi;          /* Pointer to found generate item */
  gitem_link* gil;               /* Pointer to found generate item link */
#endif

  sig.name = name;

  /* Search for signal in given functional unit signal list */
  if( (sigl = sig_link_find( name, funit->sig_head )) != NULL ) {

    found_sig = sigl->sig;

#ifndef VPI_ONLY
  } else {

    /* If it was not found, search in the functional unit generate item list */
    gi = gen_item_create_sig( &sig );

    if( ((gil = gitem_link_find( gi, funit->gitem_head )) != NULL) && ((found_gi = gen_item_find( gil->gi, gi )) != NULL) ) {
      found_sig = found_gi->elem.sig;
    }

    /* Deallocate temporary generate item */
    gen_item_dealloc( gi, FALSE );
#endif

  }

  PROFILE_END;

  return( found_sig );

}

/*!
 Searches all statement blocks in the given functional unit that have expressions that call
 the functional unit containing the given statement as its first statement.
*/
void funit_remove_stmt_blks_calling_stmt(
  func_unit* funit,  /*!< Pointer to functional unit to search in */
  statement* stmt    /*!< Pointer to statement to search for */
) { PROFILE(FUNIT_REMOVE_STMT_BLKS_CALLING_STMT);

  stmt_iter si;  /* Statement list iterator */

  /* Search all of the statement blocks */
  stmt_iter_reset( &si, funit->stmt_head );
  while( si.curr != NULL ) {
    if( (si.curr->stmt->suppl.part.head == 1) && statement_contains_expr_calling_stmt( si.curr->stmt, stmt ) ) {
      stmt_blk_add_to_remove_list( si.curr->stmt );
    }
    stmt_iter_next( &si );
  }

  PROFILE_END;

}

/*!
 \return Returns dynamically allocated string containing internally used task, function or named-block name.
*/
char* funit_gen_task_function_namedblock_name(
  char*      orig_name,  /*!< Verilog name of task, function or named-block */
  func_unit* parent      /*!< Pointer to parent functional unit of this functional unit */
) { PROFILE(FUNIT_GEN_TASK_FUNCTION_NAMEDBLOCK_NAME);

  char         full_name[4096];  /* Container for new name */
  unsigned int rv;               /* Return value for snprintf calls */

  assert( parent != NULL );
  assert( orig_name != NULL );

  /* Generate full name to use for the function/task */
  rv = snprintf( full_name, 4096, "%s.%s", parent->name, orig_name );
  assert( rv < 4096 );

  PROFILE_END;

  return( strdup_safe( full_name ) );

}

/*!
 \throws anonymous expression_resize enumerate_resolve param_resolve expression_set_value expression_set_value expression_set_value vsignal_create_vec gen_item_resize_stmts_and_sigs
 
 Resizes signals if they are contigent upon parameter values.  After
 all signals have been resized, the signal's corresponding expressions
 are resized.  This function should be called just prior to outputting
 this funtional unit's contents to the CDD file (after parsing phase only)
*/
void funit_size_elements(
  func_unit*  funit,       /*!< Pointer to functional unit containing elements to resize */
  funit_inst* inst,        /*!< Pointer to instance containing this functional unit */
  bool        gen_all,     /*!< Set to TRUE to generate all components (this should only be set
                                by the funit_db_write function) */
  bool        alloc_exprs  /*!< Allocates vector data for all expressions if set to TRUE */
) { PROFILE(FUNIT_SIZE_ELEMENTS);
  
  inst_parm*  curr_iparm;       /* Pointer to current instance parameter to evaluate */
  exp_link*   curr_exp;         /* Pointer to current expression link to evaluate */
  fsm_link*   curr_fsm;         /* Pointer to current FSM structure to evaluate */
#ifndef VPI_ONLY
  gitem_link* curr_gi;          /* Pointer to current generate item link to evaluate */
#endif
  sig_link*   curr_sig;         /* Pointer to current signal link to evaluate */
  bool        resolve = FALSE;  /* If set to TRUE, perform one more parameter resolution */

  assert( funit != NULL );
  assert( inst != NULL );

  /*
   First, traverse through current instance's parameter list and resolve
   any unresolved parameters created via generate statements.
  */
  curr_iparm = inst->param_head;
  while( curr_iparm != NULL ) {
    if( curr_iparm->mparm == NULL ) {
      curr_exp = curr_iparm->sig->exp_head;
      while( curr_exp != NULL ) {
        if( curr_exp->exp->suppl.part.gen_expr == 0 ) {
          expression_set_value( curr_exp->exp, curr_iparm->sig, funit );
          resolve = TRUE;
        }
        curr_exp = curr_exp->next;
      }
    }
    curr_iparm = curr_iparm->next;
  }
  
  /* If we need to do another parameter resolution for generate blocks, do it now */
  if( resolve ) {
    param_resolve( inst );
  }

#ifndef VPI_ONLY
  /*
   Second, traverse through any BIND generate items and update the expression name.
  */
  curr_gi = inst->gitem_head;
  while( curr_gi != NULL ) {
    gen_item_bind( curr_gi->gi );
    curr_gi = curr_gi->next;
  }
#endif

  /* 
   Third, traverse through current instance's instance parameter list and
   set sizes of signals and expressions.
  */
  curr_iparm = inst->param_head;
  while( curr_iparm != NULL ) {
    inst_parm_bind( curr_iparm );
    if( curr_iparm->mparm != NULL ) {
      /* This parameter sizes a signal so perform the signal size */
      if( curr_iparm->mparm->sig != NULL ) {
        param_set_sig_size( curr_iparm->mparm->sig, curr_iparm );
      } else {
        /* This parameter attaches to an expression tree */
        curr_exp = curr_iparm->mparm->exp_head;
        while( curr_exp != NULL ) {
          expression_set_value( curr_exp->exp, curr_iparm->sig, funit );
          curr_exp = curr_exp->next;
        }
      }
    }
    curr_iparm = curr_iparm->next;
  }

  /* Traverse through all signals, calculating and creating their vector values */
  curr_sig = funit->sig_head;
  while( curr_sig != NULL ) {
    vsignal_create_vec( curr_sig->sig );
    curr_sig = curr_sig->next;
  }

  /*
   Fourth, resolve all enumerated values for this functional unit
  */
  enumerate_resolve( inst );

  /*
   Fifth, traverse all expressions and set expressions to specified
   signals.  Makes the assumption that all children expressions come
   before the root expression in the list (this is currently the case).
  */
  curr_exp = funit->exp_head;
  while( curr_exp != NULL ) {
    if( ESUPPL_IS_ROOT( curr_exp->exp->suppl ) ) {
      /* Perform an entire expression resize */
      expression_resize( curr_exp->exp, funit, TRUE, alloc_exprs );
    }
    if( (curr_exp->exp->sig != NULL) &&
        (curr_exp->exp->op != EXP_OP_FUNC_CALL) &&
        (curr_exp->exp->op != EXP_OP_PASSIGN) ) {
      expression_set_value( curr_exp->exp, curr_exp->exp->sig, funit );
      assert( curr_exp->exp->value->value.ul != NULL );
    }
    curr_exp = curr_exp->next;
  }

#ifndef VPI_ONLY
  /* Sixth, traverse all generate items and resize all expressions and signals. */
  curr_gi = inst->gitem_head;
  while( curr_gi != NULL ) {
    gen_item_resize_stmts_and_sigs( curr_gi->gi, funit );
    curr_gi = curr_gi->next;
  }
#endif

  if( gen_all ) {

    /*
     Last, size all FSMs.  Since the FSM structure is reliant on the size
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

  PROFILE_END;
    
}

/*!
 \throws anonymous funit_size_elements

 Prints the database line for the specified functional unit to the specified database
 file.  If there are any problems with the write, returns FALSE; otherwise,
 returns TRUE.
*/
void funit_db_write(
  func_unit*  funit,        /*!< Pointer to functional unit to write to output */
  char*       scope,        /*!< String version of functional unit scope in hierarchy */
  FILE*       file,         /*!< Pointer to specified output file to write contents */
  funit_inst* inst,         /*!< Pointer to the current functional unit instance */
  bool        report_save,  /*!< Specifies that we are attempting to save a CDD after modifying the database in
                                 the report command */
  bool        ids_issued    /*!< Specifies if IDs have been issued prior to calling this function */
) { PROFILE(FUNIT_DB_WRITE);

  sig_link*       curr_sig;       /* Pointer to current functional unit sig_link element */
  exp_link*       curr_exp;       /* Pointer to current functional unit exp_link element */
  stmt_iter       curr_stmt;      /* Statement list iterator */
  inst_parm*      curr_parm;      /* Pointer to current instance parameter */
  fsm_link*       curr_fsm;       /* Pointer to current functional unit fsm_link element */
  race_blk*       curr_race;      /* Pointer to current race condition block */
#ifndef VPI_ONLY
  gitem_link*     curr_gi;        /* Pointer to current gitem_link element */
#endif
  exclude_reason* curr_er;        /* Pointer to current exclude reason element */
  char            modname[4096];  /* Name of module */
  char            tmp[4096];      /* Temporary string holder */

  if( funit->type != FUNIT_NO_SCORE ) {

#ifdef DEBUG_MODE
    assert( (funit->type == FUNIT_MODULE)    || (funit->type == FUNIT_NAMED_BLOCK) ||
            (funit->type == FUNIT_FUNCTION)  || (funit->type == FUNIT_TASK)        ||
            (funit->type == FUNIT_AFUNCTION) || (funit->type == FUNIT_ATASK)       ||
            (funit->type == FUNIT_ANAMED_BLOCK) );
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Writing %s %s", get_funit_type( funit->type ), obf_funit( funit->name ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Calculate module name to display */
    if( scope_local( funit->name ) || (inst == NULL) ) {
      strcpy( modname, funit->name );
    } else {
      funit_inst*  parent_inst = inst->parent;
      unsigned int rv;
      strcpy( modname, inst->name );
      assert( parent_inst != NULL );
      while( parent_inst->funit->type != FUNIT_MODULE ) {
        unsigned int rv = snprintf( tmp, 4096, "%s.%s", parent_inst->name, modname );
        assert( rv < 4096 );
        strcpy( modname, tmp );
        parent_inst = parent_inst->parent;
      }
      rv = snprintf( tmp, 4096, "%s.%s", parent_inst->funit->name, modname );
      assert( rv < 4096 );
      strcpy( modname, tmp );
    }

    /* Size all elements in this functional unit and calculate timescale if we are in parse mode */
    if( inst != NULL ) {
      funit_size_elements( funit, inst, TRUE, FALSE );
      funit->timescale = db_scale_to_precision( (uint64)1, funit );
    }
  
    /*@-duplicatequals -formattype@*/
    fprintf( file, "%d %d %s \"%s\" %s %d %d %llu\n",
      DB_TYPE_FUNIT,
      funit->type,
      modname,
      scope,
      funit->filename,
      funit->start_line,
      funit->end_line,
      funit->timescale
    );
    /*@=duplicatequals =formattype@*/

    /* If a version was specified for this functional unit, write it now */
    if( funit->version != NULL ) {
      fprintf( file, "%d %s\n", DB_TYPE_FUNIT_VERSION, funit->version );
    }

    /* Now print all expressions in functional unit */
    curr_exp = funit->exp_head;
    while( curr_exp != NULL ) {
      expression_db_write( curr_exp->exp, file, (inst != NULL), ids_issued );
      curr_exp = curr_exp->next;
    }

#ifndef VPI_ONLY
    /* Now print all expressions within generated statements in functional unit */
    if( inst != NULL ) {
      curr_gi = inst->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_db_write_expr_tree( curr_gi->gi, file );
        curr_gi = curr_gi->next;
      }
    }
#endif

    /* Now print all signals in functional unit */
    curr_sig = funit->sig_head;
    while( curr_sig != NULL ) {
      vsignal_db_write( curr_sig->sig, file );
      curr_sig = curr_sig->next; 
    }

    /* Now print all parameters in functional unit */
    if( inst != NULL ) {
      curr_parm = inst->param_head;
      while( curr_parm != NULL ) {
        param_db_write( curr_parm, file );
        curr_parm = curr_parm->next;
      }
    }

#ifndef VPI_ONLY
    /* Now print any generated signals in the current instance */
    if( inst != NULL ) {
      curr_gi = inst->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_db_write( curr_gi->gi, GI_TYPE_SIG, file );
        curr_gi = curr_gi->next;
      }
    }
#endif

    /* Now print all statements in functional unit */
    if( report_save ) {
      stmt_iter_reset( &curr_stmt, funit->stmt_tail );
    } else {
      stmt_iter_reset( &curr_stmt, funit->stmt_head );
    }
    while( curr_stmt.curr != NULL ) {
      statement_db_write( curr_stmt.curr->stmt, file, ids_issued );
      stmt_iter_next( &curr_stmt );
    }

#ifndef VPI_ONLY
    /* Now print any generated statements in the current instance */
    if( inst != NULL ) {
      curr_gi = inst->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_db_write( curr_gi->gi, GI_TYPE_STMT, file );
        curr_gi = curr_gi->next;
      }
    }
#endif

    /* Now print all FSM structures in functional unit */
    curr_fsm = funit->fsm_head;
    while( curr_fsm != NULL ) {
      fsm_db_write( curr_fsm->table, file, ids_issued );
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

    /* Now print all of the exclusion reasons in the functional unit */
    curr_er = funit->er_head;
    while( curr_er != NULL ) {
      exclude_db_write( curr_er, file );
      curr_er = curr_er->next;
    }

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads the current line of the specified file and parses it for a functional unit.
 If all is successful, returns TRUE; otherwise, returns FALSE.
*/
void funit_db_read(
  func_unit* funit,  /*!< Pointer to functional unit to read contents into */
  char*      scope,  /*!< Pointer to name of read functional unit scope */
  char**     line    /*!< Pointer to current line to parse */
) { PROFILE(FUNIT_DB_READ);

  int  chars_read;   /* Number of characters currently read */
  int  params;       /* Number of parameters in string that were parsed */

  /*@-duplicatequals -formattype@*/
  if( (params = sscanf( *line, "%d %s \"%[^\"]\" %s %d %d %llu%n", &(funit->type), funit->name, scope, funit->filename,
                        &(funit->start_line), &(funit->end_line), &(funit->timescale), &chars_read )) == 7 ) {
  /*@=duplicatequals =formattype@*/

    *line = *line + chars_read;

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Incorrect number of parameters for func_unit, should be 7 but is %d\n",
                                params );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    printf( "func_unit Throw A\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Reads in the functional unit version information from the specified CDD line.
*/
void funit_version_db_read(
  func_unit* funit,  /*!< Pointer to current functional unit to read version into */
  char**     line    /*!< Pointer to current line to parse */
) { PROFILE(FUNIT_VERSION_DB_READ);

  /* The current functional unit version must not have already been set */
  assert( funit->version == NULL );

  /* Strip the leading whitespace */
  while( **line == ' ' ) (*line)++;
  
  /* The rest of the line will be the version information (internal whitespace is allowed) */
  funit->version = strdup_safe( *line );

  PROFILE_END;

}

/*!
 \throws anonymous fsm_db_merge Throw Throw expression_db_merge vsignal_db_merge

 Parses specified line for functional unit information and performs a merge of the two 
 specified functional units, placing the resulting merge functional unit into the functional unit named base.
 If there are any differences between the two functional units, a warning or error will be
 displayed to the user.
*/
void funit_db_merge(
  func_unit* base,  /*!< Module that will merge in that data from the in functional unit */
  FILE*      file,  /*!< Pointer to CDD file handle to read */
  bool       same   /*!< Specifies if functional unit to be merged should match existing functional unit exactly or not */
) { PROFILE(FUNIT_DB_MERGE);

  exp_link*    curr_base_exp;   /* Pointer to current expression in base functional unit expression list */
  sig_link*    curr_base_sig;   /* Pointer to current signal in base functional unit signal list */
  stmt_iter    curr_base_stmt;  /* Statement list iterator */
  fsm_link*    curr_base_fsm;   /* Pointer to current FSM in base functional unit FSM list */
  race_blk*    curr_base_race;  /* Pointer to current race condition block in base module list  */
  char*        curr_line;       /* Pointer to current line being read from CDD */
  unsigned int curr_line_size;  /* Number of bytes allocated for curr_line */
  char*        rest_line;       /* Pointer to rest of read line */
  int          type;            /* Specifies currently read CDD type */
  int          chars_read;      /* Number of characters read from current CDD line */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle the functional unit version, if specified */
  if( base->version != NULL ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_FUNIT_VERSION ) {
            while( *rest_line == ' ' ) rest_line++;
            if( strcmp( base->version, rest_line ) != 0 ) {
              print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
              Throw 0;
            }
          } else {
            print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
            Throw 0;
          }
        } else {
          print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
      } Catch_anonymous {
        free_safe( curr_line, curr_line_size );
        Throw 0;
      }
    } else {
      print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* Handle all functional unit expressions */
  curr_base_exp = base->exp_head;
  while( curr_base_exp != NULL ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_EXPRESSION ) {
            expression_db_merge( curr_base_exp->exp, &rest_line, same );
          } else {
            print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
            printf( "func_unit Throw B\n" );
            Throw 0;
          }
        } else {
          print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
          printf( "func_unit Throw C\n" );
          Throw 0;
        }
      } Catch_anonymous {
        free_safe( curr_line, curr_line_size );
        Throw 0;
      }
      free_safe( curr_line, curr_line_size );
    } else {
      print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
      printf( "func_unit Throw E\n" );
      Throw 0;
    }
    curr_base_exp = curr_base_exp->next;
  }

  /* Handle all functional unit signals */
  curr_base_sig = base->sig_head;
  while( curr_base_sig != NULL ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_SIGNAL ) {
            vsignal_db_merge( curr_base_sig->sig, &rest_line, same );
          } else {
            print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
            printf( "func_unit Throw F\n" );
            Throw 0;
          }
        } else {
          print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
          printf( "func_unit Throw G\n" );
          Throw 0;
        }
      } Catch_anonymous {
        free_safe( curr_line, curr_line_size );
        Throw 0;
      }
      free_safe( curr_line, curr_line_size );
    } else {
      print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
      printf( "func_unit Throw I\n" );
      Throw 0;
    }
    curr_base_sig = curr_base_sig->next;
  }

  /* Since statements don't get merged, we will just read these lines in */
  stmt_iter_reset( &curr_base_stmt, base->stmt_head );
  while( curr_base_stmt.curr != NULL ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type != DB_TYPE_STATEMENT ) {
            print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
            printf( "func_unit Throw J\n" );
            Throw 0;
          }
        } else {
          print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
          printf( "func_unit Throw K\n" );
          Throw 0;
        }
      } Catch_anonymous {
        free_safe( curr_line, curr_line_size );
        printf( "func_unit Throw L\n" );
        Throw 0;
      }
      free_safe( curr_line, curr_line_size );
    } else {
      print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
      printf( "func_unit Throw M\n" );
      Throw 0;
    }
    stmt_iter_next( &curr_base_stmt );
  }

  /* Handle all functional unit FSMs */
  curr_base_fsm = base->fsm_head;
  while( curr_base_fsm != NULL ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_FSM ) {
            fsm_db_merge( curr_base_fsm->table, &rest_line );
          } else {
            print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
            printf( "func_unit Throw N\n" );
            Throw 0;
          }
        } else {
          print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
          printf( "func_unit Throw O\n" );
          Throw 0;
        }
      } Catch_anonymous {
        free_safe( curr_line, curr_line_size );
        printf( "func_unit Throw P\n" );
        Throw 0;
      }
      free_safe( curr_line, curr_line_size );
    } else {
      print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
      printf( "func_unit Throw Q\n" );
      Throw 0;
    }
    curr_base_fsm = curr_base_fsm->next;
  }

  /* Since race condition blocks don't get merged, we will just read these lines in */
  if( base->type == FUNIT_MODULE ) {
    curr_base_race = base->race_head;
    while( curr_base_race != NULL ) {
      if( util_readline( file, &curr_line, &curr_line_size ) ) {
        Try {
          if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
            rest_line = curr_line + chars_read;
            if( type != DB_TYPE_RACE ) {
              print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
              printf( "func_unit Throw R\n" );
              Throw 0;
            }
          } else {
            print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
            printf( "func_unit Throw S\n" );
            Throw 0;
          }
        } Catch_anonymous {
          free_safe( curr_line, curr_line_size );
          printf( "func_unit Throw T\n" );
          Throw 0;
        }
        free_safe( curr_line, curr_line_size );
      } else {
        print_output( "Databases being merged are incompatible.", FATAL, __FILE__, __LINE__ );
        printf( "func_unit Throw U\n" );
        Throw 0;
      }
      curr_base_race = curr_base_race->next;
    }
  }

  PROFILE_END;

}

/*!
 Merges two functional units into the base functional unit.  Used for creating merged results
 for GUI usage.
*/
void funit_merge(
  func_unit* base,   /*!< Base functional unit that will contain the merged results */
  func_unit* other   /*!< Other functional unit that will be merged */
) { PROFILE(FUNIT_MERGE);

  exp_link* curr_base_exp;   /* Pointer to current expression in base functional unit expression list */
  exp_link* curr_other_exp;  /* Pointer to current expression in other functional unit expression list */
  sig_link* curr_base_sig;   /* Pointer to current signal in base functional unit signal list */
  sig_link* curr_other_sig;  /* Pointer to current signal in other functional unit signal list */
  fsm_link* curr_base_fsm;   /* Pointer to current FSM in base functional unit FSM list */
  fsm_link* curr_other_fsm;  /* Pointer to current FSM in other functional unit FSM list */

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all functional unit expressions */
  curr_base_exp  = base->exp_head;
  curr_other_exp = other->exp_head;
  while( (curr_base_exp != NULL) && (curr_other_exp != NULL) ) {
    expression_merge( curr_base_exp->exp, curr_other_exp->exp );
    curr_base_exp  = curr_base_exp->next;
    curr_other_exp = curr_other_exp->next;
  }
  assert( (curr_base_exp == NULL) && (curr_other_exp == NULL) );

  /* Handle all functional unit signals */
  curr_base_sig  = base->sig_head;
  curr_other_sig = other->sig_head;
  while( (curr_base_sig != NULL) && (curr_other_sig != NULL) ) {
    vsignal_merge( curr_base_sig->sig, curr_other_sig->sig );
    curr_base_sig  = curr_base_sig->next;
    curr_other_sig = curr_other_sig->next;
  }
  assert( (curr_base_sig == NULL) && (curr_other_exp == NULL) );

  /* Handle all functional unit FSMs */
  curr_base_fsm  = base->fsm_head;
  curr_other_fsm = base->fsm_head;
  while( (curr_base_fsm != NULL) && (curr_other_fsm != NULL) ) {
    fsm_merge( curr_base_fsm->table, curr_other_fsm->table );
    curr_base_fsm  = curr_base_fsm->next;
    curr_other_fsm = curr_other_fsm->next;
  }
  assert( (curr_base_fsm == NULL) && (curr_other_fsm == NULL) );

  PROFILE_END;

}

/*!
 \return Returns the flattened name of the given functional unit
*/
char* funit_flatten_name(
  func_unit* funit  /*!< Pointer to functional unit to flatten name */
) { PROFILE(FUNIT_FLATTEN_NAME);

  static char fscope[4096];  /* Flattened scope name */
  char        tmp[4096];     /* Temporary string storage */
  char        front[4096];   /* First portion of scope name */
  char        rest[4096];    /* Last portion of scope name */

  assert( funit != NULL );

  scope_extract_front( funit->name, fscope, rest );
  strcpy( tmp, rest );
  scope_extract_front( tmp, front, rest );

  while( front[0] != '\0' ) {
    if( !db_is_unnamed_scope( front ) ) {
      strcat( fscope, "." );
      strcat( fscope, front );
    }
    strcpy( tmp, rest );
    scope_extract_front( tmp, front, rest ); 
  }

  PROFILE_END;

  return fscope;

}

/*!
 \return Returns a pointer to the functional unit that contains the specified expression/statement
         ID if one exists; otherwise, returns NULL.

 Searches the functional units until one is found that contains the expression/statement identified
 by the specified ID and returns a pointer to this functional unit.  If no such ID exists in the
 design, a value of NULL is returned to the calling statement.
*/
func_unit* funit_find_by_id(
  int id  /*!< Expression/statement ID to search for */
) { PROFILE(FUNIT_FIND_BY_ID);

  funit_link* funitl;       /* Temporary pointer to functional unit link */
  exp_link*   expl = NULL;  /* Temporary pointer to expression link */

  funitl = db_list[curr_db]->funit_head;
  while( (funitl != NULL) && (expl == NULL) ) {
    if( (expl = exp_link_find( id, funitl->funit->exp_head )) == NULL ) {
      funitl = funitl->next;
    }
  }

  PROFILE_END;
      
  return( (funitl == NULL) ? NULL : funitl->funit );
    
}

/*!
 \return Returns TRUE if the specified functional unit does not contain any inputs, outputs or
         inouts and is of type MODULE.
*/
bool funit_is_top_module(
  func_unit* funit  /*!< Pointer to functional unit to check */
) { PROFILE(FUNIT_IS_TOP_MODULE);

  bool      retval = FALSE;  /* Return value for this function */
  sig_link* sigl;            /* Pointer to current signal link */

  assert( funit != NULL );

  /* Only check the signal list if we are a MODULE type */
  if( funit->type == FUNIT_MODULE ) {

    sigl = funit->sig_head;
    while( (sigl != NULL) &&
           (sigl->sig->suppl.part.type != SSUPPL_TYPE_INPUT_NET)  &&
           (sigl->sig->suppl.part.type != SSUPPL_TYPE_INPUT_REG)  &&
           (sigl->sig->suppl.part.type != SSUPPL_TYPE_OUTPUT_NET) &&
           (sigl->sig->suppl.part.type != SSUPPL_TYPE_OUTPUT_REG) &&
           (sigl->sig->suppl.part.type != SSUPPL_TYPE_INOUT_NET)  &&
           (sigl->sig->suppl.part.type != SSUPPL_TYPE_INOUT_REG) ) {
      sigl = sigl->next;
    }

    retval = (sigl == NULL);

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param funit  Pointer to functional unit to check.

 \return Returns TRUE if the specified functional unit is an unnamed scope; otherwise,
         returns FALSE.

 A functional unit is considered to be an unnamed scope if it is of type FUNIT_NAMED_BLOCK
 and the last portion of its functional unit name returns TRUE after calling the 
 db_is_unnamed_scope() function.
*/
bool funit_is_unnamed( func_unit* funit ) { PROFILE(FUNIT_IS_UNNAMED);

  bool retval = FALSE;  /* Return value for this function */
  char back[256];       /* Last portion of functional unit name */
  char rest[4096];      /* Rest of functional unit name */

  /* Only begin..end blocks can be unnamed scopes */
  if( (funit->type == FUNIT_NAMED_BLOCK) || (funit->type == FUNIT_ANAMED_BLOCK) ) {
    scope_extract_back( funit->name, back, rest );
    retval = db_is_unnamed_scope( back );
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param parent  Potential parent functional unit to check for relationship to child
 \param child   Potential child functional unit to check for relationship to parent

 \return Returns TRUE if the relationship of the "parent" and "child" is just that.
*/
bool funit_is_unnamed_child_of( func_unit* parent, func_unit* child ) { PROFILE(FUNIT_IS_UNNAMED_CHILD_OF);

  while( (child->parent != NULL) && (child->parent != parent) && funit_is_unnamed( child->parent ) ) {
    child = child->parent;
  }

  PROFILE_END;

  return( child->parent == parent );

}

/*!
 \param parent  Potential parent functional unit to check for relationship to child
 \param child   Potential child functional unit to check for relationship to parent

 \return Returns TRUE if the relationship of the "parent" and "child" is just that.
*/
bool funit_is_child_of( func_unit* parent, func_unit* child ) { PROFILE(FUNIT_IS_CHILD_OF);

  while( (child->parent != NULL) && (child->parent != parent) ) {
    child = child->parent;
  }

  PROFILE_END;

  return( child->parent == parent );

}

/*!
 \param funit  Pointer to functional unit element to display signals.

 Iterates through signal list of specified functional unit, displaying each signal's
 name, width, lsb and value.
*/
void funit_display_signals( func_unit* funit ) { PROFILE(FUNIT_DISPLAY_SIGNALS);

  sig_link* sigl;  /* Pointer to current signal link element */

  printf( "%s => %s", get_funit_type( funit->type ), obf_funit( funit->name ) );

  sigl = funit->sig_head;
  while( sigl != NULL ) {
    vsignal_display( sigl->sig );
    sigl = sigl->next;
  }

  PROFILE_END;

}

/*!
 \param funit  Pointer to functional unit element to display expressions

 Iterates through expression list of specified functional unit, displaying each expression's
 id.
*/
void funit_display_expressions( func_unit* funit ) { PROFILE(FUNIT_DISPLAY_EXPRESSIONS);

  exp_link* expl;    /* Pointer to current expression link element */

  printf( "%s => %s", get_funit_type( funit->type ), obf_funit( funit->name ) );

  expl = funit->exp_head;
  while( expl != NULL ) {
    expression_display( expl->exp );
    expl = expl->next;
  }

  PROFILE_END;

}

/*!
 \param funit  Pointer to functional unit to add given thread pointer to
 \param thr    Pointer to thread associated with this statement

 Adds the given thread to the given functional unit's thread/thread list element pointer, allocating memory as needed
 and adjusting values as needed.
*/
void funit_add_thread(
  func_unit* funit,
  thread*    thr
) { PROFILE(STATEMENT_ADD_THREAD);

  assert( funit != NULL );
  assert( thr != NULL );

  /* Statement element should point to a thread */
  if( funit->elem_type == 0 ) {

    /* If the statement element currently points to nothing, simply point the statement element to the given thread */
    if( funit->elem.thr == NULL ) {
      funit->elem.thr = thr;

    /* Otherwise, change the type to a thread list, create a thread list, initialize it and continue */
    } else {

      thr_list* tlist;  /* Pointer to thread list */

      /* Allocate memory for the thread list */
      tlist = (thr_list*)malloc_safe( sizeof( thr_list ) );

      /* Create new thread link for existing thread */
      tlist->head      = (thr_link*)malloc_safe( sizeof( thr_link ) );
      tlist->head->thr = funit->elem.thr;
  
      /* Create new thread link for specified thread */
      tlist->tail       = (thr_link*)malloc_safe( sizeof( thr_link ) );
      tlist->tail->thr  = thr;
      tlist->tail->next = NULL;
      tlist->head->next = tlist->tail;

      /* Specify the next pointer to be NULL (to indicate that there aren't any available thread links to use) */
      tlist->next = NULL;
    
      /* Repopulate the functional unit */
      funit->elem.tlist = tlist;
      funit->elem_type  = 1;
  
    }

  /* Otherwise, the statement element is a pointer to a thread list already */
  } else {

    /* If there are no thread links already allocated and available, allocate a new thread link */
    if( funit->elem.tlist->next == NULL ) {

      thr_link* thrl;  /* Pointer to a thread link */
    
      /* Allocate and initialize thread link */
      thrl       = (thr_link*)malloc_safe( sizeof( thr_link ) );
      thrl->thr  = thr;
      thrl->next = NULL;

      /* Add the new thread link to the list */
      funit->elem.tlist->tail->next = thrl;
      funit->elem.tlist->tail       = thrl;

    /* Otherwise, use the link pointed at by next and adjust next */
    } else {

      funit->elem.tlist->next->thr = thr;
      funit->elem.tlist->next      = funit->elem.tlist->next->next;

    }

  }

}

/*!
 Adds all of the given functional unit threads to the active simulation queue.
*/
void funit_push_threads(
  func_unit*       funit,  /*!< Pointer of functional unit to push threads from */
  const statement* stmt,   /*!< Pointer to the statement to search for in functional unit threads */
  const sim_time*  time    /*!< Pointer to current simulation time */
) { PROFILE(FUNIT_PUSH_THREADS);

  assert( funit != NULL );

  if( funit->elem_type == 0 ) {
    if( (funit->elem.thr != NULL) && (funit->elem.thr->suppl.part.state == THR_ST_WAITING) && (funit->elem.thr->curr == stmt) ) {
      sim_thread_push( funit->elem.thr, time );
    }
  } else {
    thr_link* curr = funit->elem.tlist->head;
    while( (curr != NULL) && (curr->thr != NULL) ) {
      if( (curr->thr != NULL) && (curr->thr->suppl.part.state == THR_ST_WAITING) && (curr->thr->curr == stmt) ) {
        sim_thread_push( curr->thr, time );
      }
      curr = curr->next;
    }
  }

  PROFILE_END;

}

/*!
 Searches the given functional unit thread element for the given thread.  When the thread is found,
 its corresponding thread link is moved to the end of the thread list, the next pointer is updated
 accordingly and the thread pointer is set to NULL.  This function will be called whenever a thread
 is killed in the simulator.
*/
void funit_delete_thread(
  func_unit* funit,  /*!< Pointer to functional unit to delete thread from thread pointer/thread list */
  thread*    thr     /*!< Pointer to thread to remove from the given statement */
) { PROFILE(STATEMENT_DELETE_THREAD);

  assert( funit != NULL );
  assert( thr != NULL );

  /* If the statement element type is a thread pointer, simply clear the thread pointer */
  if( funit->elem_type == 0 ) {
    funit->elem.thr = NULL;

  /* Otherwise, find the given thread in the statement thread list and remove it */
  } else {

    thr_link* curr = funit->elem.tlist->head;
    thr_link* last = NULL;

    /* Search the thread list for the matching thread */
    while( (curr != NULL) && (curr->thr != thr) ) {
      last = curr;
      curr = curr->next;
    }

    /* We should have found the thread in the statement list */
    assert( curr != NULL );

    /* Move this thread link to the end of the thread link list and clear out its values */
    if( funit->elem.tlist->tail != curr ) {
      if( funit->elem.tlist->head == curr ) {
        funit->elem.tlist->head = curr->next;
      } else {
        last->next = curr->next;
      }
      funit->elem.tlist->tail->next = curr;
      funit->elem.tlist->tail       = curr;
      curr->next = NULL;
    }
  
    /* Clear the thread pointer */
    curr->thr = NULL; 
  
    /* If the thread list next pointer is NULL, set it to the freed thread link structure */
    if( funit->elem.tlist->next == NULL ) {
      funit->elem.tlist->next = curr;
    }
  
  } 

  PROFILE_END;
    
}

/*!
 Outputs all of the needed signals to the specified file.
*/
void funit_output_dumpvars( 
  FILE*       vfile,  /*!< Pointer to file to output dumpvars information to */
  func_unit*  funit,  /*!< Pointer to functional unit to output */
  const char* scope   /*!< Instance scope */
) { PROFILE(FUNIT_OUTPUT_DUMPVARS);

  sig_link* sigl  = funit->sig_head;
  bool      first = TRUE;

  while( sigl != NULL ) {
    if( (sigl->sig->suppl.part.assigned == 0) &&
        (sigl->sig->suppl.part.type != SSUPPL_TYPE_PARAM)      &&
        (sigl->sig->suppl.part.type != SSUPPL_TYPE_PARAM_REAL) &&
        (sigl->sig->suppl.part.type != SSUPPL_TYPE_ENUM)       &&
        (sigl->sig->suppl.part.type != SSUPPL_TYPE_MEM)        &&
        (sigl->sig->suppl.part.type != SSUPPL_TYPE_GENVAR)     &&
        (sigl->sig->suppl.part.type != SSUPPL_TYPE_EVENT) ) {
      if( first ) {
        first = FALSE;
        fprintf( vfile, "  $dumpvars( 1, %s.%s", scope, sigl->sig->name );
      } else {
        fprintf( vfile, ",\n                %s.%s", scope, sigl->sig->name );
      }
    }
    sigl = sigl->next;
  } 

  if( !first ) {
    fprintf( vfile, " );\n" );
  }

  PROFILE_END;

}

/*!
 Deallocates functional unit contents: name and filename strings.
*/
static void funit_clean(
  func_unit* funit  /*!< Pointer to functional unit element to clean */
) { PROFILE(FUNIT_CLEAN);

  func_unit*      old_funit = curr_funit;  /* Holds the original functional unit in curr_funit */
  typedef_item*   tdi;                     /* Pointer to current typedef item */
  typedef_item*   ttdi;                    /* Pointer to temporary typedef item */
  exclude_reason* er;                      /* Pointer to current exclude reason item */
  exclude_reason* ter;                     /* Pointer to temporary exclude reason item */

  if( funit != NULL ) {

    /* Set the global curr_funit to be the same as this funit */
    curr_funit = funit;

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

#ifndef VPI_ONLY
    /* Free generate item list */
    gitem_link_delete_list( funit->gitem_head, TRUE );
    funit->gitem_head = NULL;
    funit->gitem_tail = NULL;
#endif

    /* Free statistic structure */
    statistic_dealloc( funit->stat );

    /* Free tf elements */
    funit_link_delete_list( &(funit->tf_head), &(funit->tf_tail), FALSE );

    /* Free typedef items */
    tdi = funit->tdi_head;
    while( tdi != NULL ) {
      ttdi = tdi;
      tdi  = tdi->next;
      free_safe( ttdi->name, (strlen( ttdi->name ) + 1) );
      parser_dealloc_sig_range( ttdi->prange, TRUE );
      parser_dealloc_sig_range( ttdi->urange, TRUE );
      free_safe( ttdi, sizeof( typedef_item ) );
    }
    funit->tdi_head = NULL;
    funit->tdi_tail = NULL;

    /* Free exclusion reason items */
    er = funit->er_head;
    while( er != NULL ) {
      ter = er;
      er  = er->next;
      free_safe( ter->reason, (strlen( ter->reason ) + 1) );
      free_safe( ter, sizeof( exclude_reason ) );
    }
    funit->er_head = NULL;
    funit->er_tail = NULL;

    /* Free enumerated elements */
    enumerate_dealloc_list( funit );

    /* Free functional unit name */
    if( funit->name != NULL ) {
      free_safe( funit->name, (strlen( funit->name ) + 1) );
      funit->name = NULL;
    }

    /* Free functional unit filename */
    if( funit->filename != NULL ) {
      free_safe( funit->filename, (strlen( funit->filename ) + 1) );
      funit->filename = NULL;
    }

    /* Free functional unit version */
    if( funit->version != NULL ) {
      free_safe( funit->version, (strlen( funit->version ) + 1) );
      funit->version = NULL;
    }

    /* Free thread list, if available */
    if( funit->elem_type == 1 ) {
      thr_link* thrl = funit->elem.tlist->head;
      thr_link* tmpl;
      while( thrl != NULL ) {
        tmpl = thrl;
        thrl = thrl->next;
        free_safe( tmpl, sizeof( thr_link ) );
      }
      free_safe( funit->elem.tlist, sizeof( thr_list ) );
    }

    /* Reset curr_funit */
    curr_funit = old_funit;

  }

  PROFILE_END;

}

/*!
 Deallocates functional unit; name and filename strings; and finally
 the structure itself from the heap.
*/
void funit_dealloc(
  func_unit* funit  /*!< Pointer to functional unit element to deallocate */
) { PROFILE(FUNIT_DEALLOC);

  if( funit != NULL ) {

    /* Deallocate the contents of the functional unit itself */
    funit_clean( funit );

    /* Deallocate functional unit element itself */
    free_safe( funit, sizeof( func_unit ) );

  }

  PROFILE_END;

}


/*
 $Log$
 Revision 1.116  2008/10/31 22:01:34  phase1geo
 Initial code changes to support merging two non-overlapping CDD files into
 one.  This functionality seems to be working but needs regression testing to
 verify that nothing is broken as a result.

 Revision 1.115  2008/10/23 20:54:52  phase1geo
 Adding support for real parameters.  Added more real number diagnostics to
 regression suite.

 Revision 1.114  2008/10/07 05:24:17  phase1geo
 Adding -dumpvars option.  Need to resolve a few issues before this work is considered
 complete.

 Revision 1.113  2008/09/22 22:15:03  phase1geo
 Initial code for supporting the merging and resolution of exclusion reasons.
 This code is completely untested at this point but does compile.  Checkpointing.

 Revision 1.112  2008/09/22 05:04:49  phase1geo
 Adding parsing support for new -er option.

 Revision 1.111  2008/09/21 13:36:59  phase1geo
 Completing code to get functional unit version information into CDD file and
 functional units.  Added new version1 and version1.1 diagnostics to verify that
 this functionality works as expected.  Still need to make use of this
 information.

 Revision 1.110  2008/09/19 22:59:16  phase1geo
 Adding initial support for module version information.  Verified that the new
 code does not break existing regression.  Also added new Covered banner.

 Revision 1.109  2008/09/02 22:41:45  phase1geo
 Starting to work on adding exclusion reason output to report files.  Added
 support for exclusion reasons to CDD files.  Checkpointing.

 Revision 1.108  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.104.2.2  2008/08/06 20:11:33  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.104.2.1  2008/07/10 22:43:51  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.106  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.105  2008/06/19 16:14:55  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.104  2008/06/02 21:34:29  phase1geo
 Fixing bug 1981073.  Adding new tests to verify this fix.

 Revision 1.103  2008/05/30 23:00:48  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.102  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.101.2.5  2008/05/29 06:46:25  phase1geo
 Finishing last submission.

 Revision 1.101.2.4  2008/05/28 05:57:10  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.101.2.3  2008/05/26 05:42:09  phase1geo
 Adding new error merge diagnostics to regression suite to verify missing vector_db_merge
 error cases.  Full regression passes.

 Revision 1.101.2.2  2008/05/23 14:50:22  phase1geo
 Optimizing vector_op_add and vector_op_subtract algorithms.  Also fixing issue with
 vector set bit.  Updating regressions per this change.

 Revision 1.101.2.1  2008/04/22 23:01:43  phase1geo
 More updates.  Completed initial pass of expr.c and fsm_arg.c.  Working
 on memory.c.  Checkpointing.

 Revision 1.101  2008/04/15 20:37:10  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.100  2008/04/15 06:08:46  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.99  2008/03/27 06:09:58  phase1geo
 Fixing some regression errors.  Checkpointing.

 Revision 1.98  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.97  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.96  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.95  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.94  2008/03/04 00:09:20  phase1geo
 More exception handling.  Checkpointing.

 Revision 1.93  2008/02/29 00:08:31  phase1geo
 Completed optimization code in simulator.  Still need to verify that code
 changes enhanced performances as desired.  Checkpointing.

 Revision 1.92  2008/02/28 07:54:09  phase1geo
 Starting to add functionality for simulation optimization in the sim_expr_changed
 function (feature request 1897410).

 Revision 1.91  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.90  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.89  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.88  2008/01/16 23:10:30  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.87  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.86  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.85  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.84  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.83  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.82  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.81  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.80  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.79  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.78  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.77  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.76  2007/07/30 22:42:02  phase1geo
 Making some progress on automatic function support.  Things currently don't compile
 but I need to checkpoint for now.

 Revision 1.75  2007/07/30 20:36:14  phase1geo
 Fixing rest of issues pertaining to new implementation of function calls.
 Full regression passes (with the exception of afunc1 which we do not expect
 to pass with these changes as of yet).

 Revision 1.74  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.73  2007/07/26 20:12:45  phase1geo
 Fixing bug related to failure of hier1.1 diagnostic.  Placing functional unit
 scope in quotes for cases where backslashes are used in the scope names (requiring
 spaces in the names to escape the backslash).  Incrementing CDD version and
 regenerated all regression files.  Only atask1 is currently failing in regressions
 now.

 Revision 1.72  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.71  2007/04/18 22:34:58  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.70  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.69  2007/04/10 22:10:11  phase1geo
 Fixing some more simulation issues.

 Revision 1.68  2007/04/10 03:56:18  phase1geo
 Completing majority of code to support new simulation core.  Starting to debug
 this though we still have quite a ways to go here.  Checkpointing.

 Revision 1.67  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.66  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.65  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.64  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.63  2007/04/02 04:50:04  phase1geo
 Adding func_iter files to iterate through a functional unit for reporting
 purposes.  Updated affected files.

 Revision 1.62  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.61  2007/03/19 22:52:50  phase1geo
 Attempting to fix problem with line ordering for a named block that is
 in the middle of another statement block.  Also fixed a problem with FORK
 expressions not being bound early enough.  Run currently segfaults but
 I need to checkpoint at the moment.

 Revision 1.60  2007/03/19 20:30:31  phase1geo
 More fixes to report command for instance flattening.  This seems to be
 working now as far as I can tell.  Regressions still have about 8 diagnostics
 failing with report errors.  Checkpointing.

 Revision 1.59  2007/03/19 03:30:16  phase1geo
 More fixes to instance flattening algorithm.  Still much more work to do here.
 Checkpointing.

 Revision 1.58  2007/03/16 22:28:14  phase1geo
 Checkpointing again.  Still having quite a few issues with getting good coverage
 reports.  Fixing a few more problems that the exclude3 diagnostic complained
 about.

 Revision 1.57  2007/03/16 21:41:09  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.56  2006/12/23 04:44:48  phase1geo
 Fixing build problems on cygwin.  Fixing compile errors with VPI and fixing
 segmentation fault in the funit_converge function.  Regression is far from
 passing at this point.  Checkpointing.

 Revision 1.55  2006/12/19 05:23:38  phase1geo
 Added initial code for handling instance flattening for unnamed scopes.  This
 is partially working at this point but still needs some debugging.  Checkpointing.

 Revision 1.54  2006/12/11 23:29:16  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.

 Revision 1.53.2.1  2007/04/17 16:31:53  phase1geo
 Fixing bug 1698806 by rebinding a parameter signal to its list of expressions
 prior to writing the initial CDD file (elaboration phase).  Added param16
 diagnostic to regression suite to verify the fix.  Full regressions pass.

 Revision 1.53  2006/11/25 21:29:01  phase1geo
 Adding timescale diagnostics to regression suite and fixing bugs in core associated
 with this code.  Full regression now passes for IV and Cver (not in VPI mode).

 Revision 1.52  2006/11/25 04:24:40  phase1geo
 Adding initial code to fully support the timescale directive and its usage.
 Added -vpi_ts score option to allow the user to specify a top-level timescale
 value for the generated VPI file (this code has not been tested at this point,
 however).  Also updated diagnostic Makefile to get the VPI shared object files
 from the current lib directory (instead of the checked in one).

 Revision 1.51  2006/11/03 23:36:36  phase1geo
 Fixing bug 1590104.  Updating regressions per this change.

 Revision 1.50  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.49  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.48  2006/10/09 17:54:19  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.47  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.46  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.45  2006/09/22 04:23:04  phase1geo
 More fixes to support new signal range structure.  Still don't have full
 regressions passing at the moment.

 Revision 1.44  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.43  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.42  2006/09/06 22:09:22  phase1geo
 Fixing bug with multiply-and-op operation.  Also fixing bug in gen_item_resolve
 function where an instance was incorrectly being placed into a new instance tree.
 Full regression passes with these changes.  Also removed verbose output.

 Revision 1.41  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.40  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.39  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.38  2006/08/24 22:25:12  phase1geo
 Fixing issue with generate expressions within signal hierarchies.  Also added
 ability to parse implicit named and * port lists.  Added diagnostics to regressions
 to verify this new ability.  Full regression passes.

 Revision 1.37  2006/08/24 03:39:02  phase1geo
 Fixing some issues with new static_lexer/parser.  Working on debugging issue
 related to the generate variable mysteriously losing its vector data.

 Revision 1.36  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.35  2006/08/14 04:19:56  phase1geo
 Fixing problem with generate11* diagnostics (generate variable used in
 signal name).  These tests pass now but full regression hasn't been verified
 at this point.

 Revision 1.34  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.33  2006/08/01 18:05:13  phase1geo
 Adding more diagnostics to test generate item structure connectivity.  Fixing
 bug in funit_find_signal function to search the function (instead of the instance
 for for finding a signal to bind).

 Revision 1.32  2006/08/01 04:38:20  phase1geo
 Fixing issues with binding to non-module scope and not binding references
 that reference a "no score" module.  Full regression passes.

 Revision 1.31  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.30  2006/07/28 16:30:53  phase1geo
 Fixing one last regression error.  We are now ready to make a tag.

 Revision 1.29  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.28  2006/07/27 02:14:52  phase1geo
 Cleaning up verbose output and fixing a few bugs for regression.  IV
 regression passes at this point.

 Revision 1.27  2006/07/27 02:04:30  phase1geo
 Fixing problem with parameter usage in a generate block for signal sizing.

 Revision 1.26  2006/07/26 06:22:27  phase1geo
 Fixing rest of issues with generate6 diagnostic.  Still need to know if I
 have broken regressions or not and there are plenty of cases in this area
 to test before I call things good.

 Revision 1.25  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.24  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.23  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.22  2006/07/21 15:52:41  phase1geo
 Checking in an initial working version of the generate structure.  Diagnostic
 generate1 passes.  Still a lot of work to go before we fully support generate
 statements, but this marks a working version to enhance on.  Full regression
 passes as well.

 Revision 1.21  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.20  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.19  2006/06/27 19:34:42  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.18  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

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

