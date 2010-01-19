/*
 Copyright (c) 2006-2010 Trevor Williams

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
extern isuppl       info_suppl;


/*!
 Initializes all contents to NULL.
*/  
static void funit_init(
  func_unit* funit  /*!< Pointer to functional unit to initialize */
) { PROFILE(FUNIT_INIT);
    
  funit->suppl.all       = 0;
  funit->suppl.part.type = FUNIT_MODULE;
  funit->name            = NULL;
  funit->id              = -1;
  funit->orig_fname      = NULL;
  funit->incl_fname      = NULL;
  funit->version         = NULL;
  funit->start_line      = 0;
  funit->end_line        = 0;
  funit->start_col       = 0;
  funit->stat            = NULL;
  funit->sigs            = NULL;
  funit->sig_size        = 0;
  funit->sig_no_rm_index = 1;
  funit->exps            = NULL;
  funit->exp_size        = 0;
  funit->first_stmt      = NULL;
  funit->stmt_head       = NULL;
  funit->stmt_tail       = NULL;
  funit->fsms            = NULL;
  funit->fsm_size        = 0;
  funit->race_head       = NULL;
  funit->race_tail       = NULL;
  funit->param_head      = NULL;
  funit->param_tail      = NULL;
  funit->gitem_head      = NULL;
  funit->gitem_tail      = NULL;
  funit->tf_head         = NULL;
  funit->tf_tail         = NULL;
  funit->tdi_head        = NULL;
  funit->tdi_tail        = NULL;
  funit->ei_head         = NULL;
  funit->ei_tail         = NULL;
  funit->er_head         = NULL;
  funit->er_tail         = NULL;
  funit->parent          = NULL;
  funit->elem.thr        = NULL;

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

#ifndef RUNLIB
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
#endif /* RUNLIB */

/*!
 \return Returns a pointer to the function that contains the specified functional unit if
         one exists; otherwise, returns NULL.
*/
func_unit* funit_get_curr_function(
  func_unit* funit  /*!< Functional unit that may be nested in a function */
) { PROFILE(FUNIT_GET_CURR_FUNCTION);

  assert( funit != NULL );

  while( (funit->suppl.part.type != FUNIT_FUNCTION)  &&
         (funit->suppl.part.type != FUNIT_AFUNCTION) &&
         (funit->suppl.part.type != FUNIT_MODULE) ) {
    funit = funit->parent;
  }

  PROFILE_END;

  return( ((funit->suppl.part.type == FUNIT_FUNCTION) || (funit->suppl.part.type == FUNIT_AFUNCTION)) ? funit : NULL );

}

#ifndef RUNLIB
/*!
 \return Returns a pointer to the function that contains the specified functional unit if
         one exists; otherwise, returns NULL.
*/
func_unit* funit_get_curr_task(
  func_unit* funit  /*!< Functional unit that may be nested in a task */
) { PROFILE(FUNIT_GET_CURR_TASK);

  assert( funit != NULL );

  while( (funit->suppl.part.type != FUNIT_TASK)  &&
         (funit->suppl.part.type != FUNIT_ATASK) &&
         (funit->suppl.part.type != FUNIT_MODULE) ) {
    funit = funit->parent;
  }

  PROFILE_END;

  return( ((funit->suppl.part.type == FUNIT_TASK) || (funit->suppl.part.type == FUNIT_ATASK)) ? funit : NULL );

}

/*!
 \return Returns the number of input, output and inout ports specified in this functional unit
*/
int funit_get_port_count(
  func_unit* funit  /*!< Pointer to functional unit to process */
) { PROFILE(FUNIT_GET_PORT_COUNT);

  int          port_cnt = 0;  /* Return value for this function */
  unsigned int i;

  assert( funit != NULL );

  for( i=0; i<funit->sig_size; i++ ) {
    if( (funit->sigs[i]->suppl.part.type == SSUPPL_TYPE_INPUT_NET)  ||
        (funit->sigs[i]->suppl.part.type == SSUPPL_TYPE_INPUT_REG)  ||
        (funit->sigs[i]->suppl.part.type == SSUPPL_TYPE_OUTPUT_NET) ||
        (funit->sigs[i]->suppl.part.type == SSUPPL_TYPE_OUTPUT_REG) ||
        (funit->sigs[i]->suppl.part.type == SSUPPL_TYPE_INOUT_NET)  ||
        (funit->sigs[i]->suppl.part.type == SSUPPL_TYPE_INOUT_REG) ) {
      port_cnt++;
    }
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
#endif /* RUNLIB */

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
#ifndef VPI_ONLY
#ifndef RUNLIB
  gen_item*   gi;                /* Pointer to temporary generate item */
  gen_item*   found_gi;          /* Pointer to found generate item */
  gitem_link* gil;               /* Pointer to found generate item link */
#endif /* RUNLIB */
#endif /* VPI_ONLY */

  sig.name = name;

  /* Search for signal in given functional unit signal list */
  if( (found_sig = sig_link_find( name, funit->sigs, funit->sig_size )) == NULL ) {

#ifndef VPI_ONLY
#ifndef RUNLIB
    /* If it was not found, search in the functional unit generate item list */
    gi = gen_item_create_sig( &sig );

    if( ((gil = gitem_link_find( gi, funit->gitem_head )) != NULL) && ((found_gi = gen_item_find( gil->gi, gi )) != NULL) ) {
      found_sig = found_gi->elem.sig;
    }

    /* Deallocate temporary generate item */
    gen_item_dealloc( gi, FALSE );
#endif /* RUNLIB */
#endif /* VPI_ONLY */

  }

  PROFILE_END;

  return( found_sig );

}

#ifndef RUNLIB
/*!
 Searches all statement blocks in the given functional unit that have expressions that call
 the functional unit containing the given statement as its first statement.
*/
void funit_remove_stmt_blks_calling_stmt(
  func_unit* funit,  /*!< Pointer to functional unit to search in */
  statement* stmt    /*!< Pointer to statement to search for */
) { PROFILE(FUNIT_REMOVE_STMT_BLKS_CALLING_STMT);

  if( funit != NULL ) {

    stmt_link* curr = funit->stmt_head;

    /* Search all of the statement blocks */
    while( curr != NULL ) {
      if( (curr->stmt->suppl.part.head == 1) && statement_contains_expr_calling_stmt( curr->stmt, stmt ) ) {
        stmt_blk_add_to_remove_list( curr->stmt );
      }
      curr = curr->next;
    }

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
  
  inst_parm*   curr_iparm;       /* Pointer to current instance parameter to evaluate */
#ifndef VPI_ONLY
  gitem_link*  curr_gi;          /* Pointer to current generate item link to evaluate */
#endif
  bool         resolve = FALSE;  /* If set to TRUE, perform one more parameter resolution */
  unsigned int i;

  assert( funit != NULL );
  assert( inst != NULL );

  /*
   First, traverse through current instance's parameter list and resolve
   any unresolved parameters created via generate statements.
  */
  curr_iparm = inst->param_head;
  while( curr_iparm != NULL ) {
    if( curr_iparm->mparm == NULL ) {
      for( i=0; i<curr_iparm->sig->exp_size; i++ ) {
        expression* exp = curr_iparm->sig->exps[i];
        if( exp->suppl.part.gen_expr == 0 ) {
          expression_set_value( exp, curr_iparm->sig, funit );
          resolve = TRUE;
        }
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
        for( i=0; i<curr_iparm->mparm->exp_size; i++ ) {
          expression_set_value( curr_iparm->mparm->exps[i], curr_iparm->sig, funit );
        }
      }
    }
    curr_iparm = curr_iparm->next;
  }

  /* Traverse through all signals, calculating and creating their vector values */
  for( i=0; i<funit->sig_size; i++ ) {
    vsignal_create_vec( funit->sigs[i] );
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
  for( i=0; i<funit->exp_size; i++ ) {
    expression* exp = funit->exps[i];
    if( ESUPPL_IS_ROOT( exp->suppl ) ) {
      /* Perform an entire expression resize */
      expression_resize( exp, funit, TRUE, alloc_exprs );
    }
    if( (exp->sig != NULL) &&
        (exp->op != EXP_OP_FUNC_CALL) &&
        (exp->op != EXP_OP_PASSIGN) ) {
      expression_set_value( exp, exp->sig, funit );
      assert( exp->value->value.ul != NULL );
    }
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
    for( i=0; i<funit->fsm_size; i++ ) {
      fsm_create_tables( funit->fsms[i] );
    }

  }

  PROFILE_END;
    
}
#endif /* RUNLIB */

/*!
 \throws anonymous funit_size_elements

 Prints the database line for the specified functional unit to the specified database
 file.  If there are any problems with the write, returns FALSE; otherwise,
 returns TRUE.
*/
void funit_db_write(
  func_unit*  funit,        /*!< Pointer to functional unit to write to output */
  char*       scope,        /*!< String version of functional unit scope in hierarchy */
  bool        name_diff,    /*!< Specifies that this instance has an inaccurate way */
  FILE*       file,         /*!< Pointer to specified output file to write contents */
  funit_inst* inst,         /*!< Pointer to the current functional unit instance */
  bool        ids_issued    /*!< Specifies if IDs have been issued prior to calling this function */
) { PROFILE(FUNIT_DB_WRITE);

  stmt_link*      curr_stmt;      /* Statement list iterator */
  inst_parm*      curr_parm;      /* Pointer to current instance parameter */
  race_blk*       curr_race;      /* Pointer to current race condition block */
#ifndef VPI_ONLY
  gitem_link*     curr_gi;        /* Pointer to current gitem_link element */
#endif
  exclude_reason* curr_er;        /* Pointer to current exclude reason element */
  char            modname[4096];  /* Name of module */
  char            tmp[4096];      /* Temporary string holder */
  str_link*       strl;           /* Pointer to string link */
  unsigned int    i;

  if( funit->suppl.part.type != FUNIT_NO_SCORE ) {

#ifdef DEBUG_MODE
    assert( (funit->suppl.part.type == FUNIT_MODULE)    || (funit->suppl.part.type == FUNIT_NAMED_BLOCK) ||
            (funit->suppl.part.type == FUNIT_FUNCTION)  || (funit->suppl.part.type == FUNIT_TASK)        ||
            (funit->suppl.part.type == FUNIT_AFUNCTION) || (funit->suppl.part.type == FUNIT_ATASK)       ||
            (funit->suppl.part.type == FUNIT_ANAMED_BLOCK) );
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Writing %s %s", get_funit_type( funit->suppl.part.type ), obf_funit( funit->name ) );
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
      while( parent_inst->funit->suppl.part.type != FUNIT_MODULE ) {
        unsigned int rv = snprintf( tmp, 4096, "%s.%s", parent_inst->name, modname );
        assert( rv < 4096 );
        strcpy( modname, tmp );
        parent_inst = parent_inst->parent;
      }
      rv = snprintf( tmp, 4096, "%s.%s", parent_inst->funit->name, modname );
      assert( rv < 4096 );
      strcpy( modname, tmp );
    }

#ifndef RUNLIB
    /* Size all elements in this functional unit and calculate timescale if we are in parse mode */
    if( inst != NULL ) {
      funit_size_elements( funit, inst, TRUE, FALSE );
      funit->timescale = db_scale_to_precision( (uint64)1, funit );
    }
#endif /* RUNLIB */
  
    /*@-duplicatequals -formattype -formatcode@*/
    fprintf( file, "%d %x %s \"%s\" %d %s %u %u %" FMT64 "u %s\n",
      DB_TYPE_FUNIT,
      (funit->suppl.all & FUNIT_MASK),
      modname,
      scope,
      name_diff,
      funit->orig_fname,
      funit->start_line,
      funit->end_line,
      funit->timescale,
      (funit->suppl.part.included ? funit->incl_fname : "")
    );
    /*@=duplicatequals =formattype =formatcode@*/

    /* Figure out if a file version exists for this functional unit */
    if( (funit->version == NULL) && ((strl = str_link_find( funit->orig_fname, db_list[curr_db]->fver_head )) != NULL) ) {
      funit->version = strdup_safe( strl->str2 );
    }

    /* If a version was specified for this functional unit, write it now */
    if( funit->version != NULL ) {
      fprintf( file, "%d %s\n", DB_TYPE_FUNIT_VERSION, funit->version );
    }

    /* Now print all expressions in functional unit */
    for( i=0; i<funit->exp_size; i++ ) {
      expression_db_write( funit->exps[i], file, (inst != NULL), ids_issued );
    }

#ifndef RUNLIB
#ifndef VPI_ONLY
    /* Now print all expressions within generated statements in functional unit */
    if( inst != NULL ) {
      curr_gi = inst->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_db_write_expr_tree( curr_gi->gi, file );
        curr_gi = curr_gi->next;
      }
    }
#endif /* VPI_ONLY */
#endif /* RUNLIB */

    /* Now print all signals in functional unit */
    for( i=0; i<funit->sig_size; i++ ) {
      if( i < funit->sig_no_rm_index ) {
        vsignal_db_write( funit->sigs[i], file );
      }
    }

#ifndef RUNLIB
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
#endif /* VPI_ONLY */
#endif /* RUNLIB */

    /* Now print all statements in functional unit */
    curr_stmt = funit->stmt_head;
    while( curr_stmt != NULL ) {
      if( curr_stmt->rm_stmt ) {
        statement_db_write( curr_stmt->stmt, file, ids_issued );
      }
      curr_stmt = curr_stmt->next;
    }

#ifndef RUNLIB
#ifndef VPI_ONLY
    /* Now print any generated statements in the current instance */
    if( inst != NULL ) {
      curr_gi = inst->gitem_head;
      while( curr_gi != NULL ) {
        gen_item_db_write( curr_gi->gi, GI_TYPE_STMT, file );
        curr_gi = curr_gi->next;
      }
    }
#endif /* VPI_ONLY */
#endif /* RUNLIB */

    /* Now print all FSM structures in functional unit */
    for( i=0; i<funit->fsm_size; i++ ) {
      fsm_db_write( funit->fsms[i], file, ids_issued );
    }

    /* Now print all race condition block structures in functional unit (if we are a module) */
    if( funit->suppl.part.type == FUNIT_MODULE ) {
      curr_race = funit->race_head;
      while( curr_race != NULL ) {
        race_db_write( curr_race, file );
        curr_race = curr_race->next;
      }
    }

#ifndef RUNLIB
    /* Now print all of the exclusion reasons in the functional unit */
    curr_er = funit->er_head;
    while( curr_er != NULL ) {
      exclude_db_write( curr_er, file );
      curr_er = curr_er->next;
    }
#endif

  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads the current line of the specified file and parses it for a functional unit.
 If all is successful, returns TRUE; otherwise, returns FALSE.
*/
void funit_db_read(
            func_unit* funit,      /*!< Pointer to functional unit to read contents into */
            char*      scope,      /*!< Pointer to name of read functional unit scope */
  /*@out@*/ bool*      name_diff,  /*!< Will cause the name_diff value of the instance to get set to the same value */
            char**     line        /*!< Pointer to current line to parse */
) { PROFILE(FUNIT_DB_READ);

  int  chars_read;   /* Number of characters currently read */
  int  params;       /* Number of parameters in string that were parsed */

  /*@-duplicatequals -formattype -formatcode@*/
  if( (params = sscanf( *line, "%" FMT8 "x %s \"%[^\"]\" %d %s %u %u %" FMT64 "u%n", 
                        &(funit->suppl.all), funit->name, scope, (int*)name_diff, funit->orig_fname,
                        &(funit->start_line), &(funit->end_line), &(funit->timescale), &chars_read )) == 8 ) {
  /*@=duplicatequals =formattype =formatcode@*/

    *line = *line + chars_read;

    /* If an include filename string should be present, attempt to parse it */
    if( funit->suppl.part.included == 1 ) {
      if( sscanf( *line, "%s%n", funit->incl_fname, &chars_read ) == 1 ) {
        *line += chars_read;
      } else {
        print_output( "Internal Error:  Incorrect number of parameters for func_unit", FATAL, __FILE__, __LINE__ );
        Throw 0;
      }
    } else {
      strcpy( funit->incl_fname, funit->orig_fname );
    }

  } else {

    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal Error:  Incorrect number of parameters for func_unit, should be 7 but is %d\n",
                                params );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
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

#ifndef RUNLIB
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

  stmt_link*   curr_base_stmt;    /* Statement list link */
  race_blk*    curr_base_race;    /* Pointer to current race condition block in base module list  */
  char*        curr_line = NULL;  /* Pointer to current line being read from CDD */
  unsigned int curr_line_size;    /* Number of bytes allocated for curr_line */
  char*        rest_line;         /* Pointer to rest of read line */
  int          type;              /* Specifies currently read CDD type */
  int          chars_read;        /* Number of characters read from current CDD line */
  unsigned int i;

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
  for( i=0; i<base->exp_size; i++ ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_EXPRESSION ) {
            expression_db_merge( base->exps[i], &rest_line, same );
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

  /* Handle all functional unit signals */
  for( i=0; i<base->sig_size; i++ ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_SIGNAL ) {
            vsignal_db_merge( base->sigs[i], &rest_line, same );
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

  /* Since statements don't get merged, we will just read these lines in */
  curr_base_stmt = base->stmt_head;
  while( curr_base_stmt != NULL ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type != DB_TYPE_STATEMENT ) {
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
    curr_base_stmt = curr_base_stmt->next;
  }

  /* Handle all functional unit FSMs */
  for( i=0; i<base->fsm_size; i++ ) {
    if( util_readline( file, &curr_line, &curr_line_size ) ) {
      Try {
        if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
          rest_line = curr_line + chars_read;
          if( type == DB_TYPE_FSM ) {
            fsm_db_merge( base->fsms[i], &rest_line );
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

  /* Since race condition blocks don't get merged, we will just read these lines in */
  if( base->suppl.part.type == FUNIT_MODULE ) {
    curr_base_race = base->race_head;
    while( curr_base_race != NULL ) {
      if( util_readline( file, &curr_line, &curr_line_size ) ) {
        Try {
          if( sscanf( curr_line, "%d%n", &type, &chars_read ) == 1 ) {
            rest_line = curr_line + chars_read;
            if( type != DB_TYPE_RACE ) {
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
      curr_base_race = curr_base_race->next;
    }
  }

  /* Deallocate memory */
  free_safe( curr_line, curr_line_size );

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

  exclude_reason* curr_other_er;
  unsigned int    i;

  assert( base != NULL );
  assert( base->name != NULL );

  /* Handle all functional unit expressions */
  assert( base->exp_size == other->exp_size );
  for( i=0; i<base->exp_size; i++ ) {
    expression_merge( base->exps[i], other->exps[i] );
  }

  /* Handle all functional unit signals */
  assert( base->sig_size == other->sig_size );
  for( i=0; i<base->sig_size; i++ ) {
    vsignal_merge( base->sigs[i], other->sigs[i] );
  }

  /* Handle all functional unit FSMs */
  assert( base->fsm_size == other->fsm_size );
  for( i=0; i<base->fsm_size; i++ ) {
    fsm_merge( base->fsms[i], other->fsms[i] );
  }

  /* Handle all functional unit exclusion reasons */
  curr_other_er = other->er_head;
  while( curr_other_er != NULL ) {
    exclude_merge( base, curr_other_er );
    curr_other_er = curr_other_er->next;
  }

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

  funit_link* funitl;      /* Temporary pointer to functional unit link */
  expression* exp = NULL;  /* Temporary pointer to expression link */

  funitl = db_list[curr_db]->funit_head;
  while( (funitl != NULL) && (exp == NULL) ) {
    if( (exp = exp_link_find( id, funitl->funit->exps, funitl->funit->exp_size )) == NULL ) {
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

  bool retval = FALSE;  /* Return value for this function */

  assert( funit != NULL );

  /* Only check the signal list if we are a MODULE type */
  if( funit->suppl.part.type == FUNIT_MODULE ) {

    unsigned int i = 0;

    while( (i < funit->sig_size) &&
           (funit->sigs[i]->suppl.part.type != SSUPPL_TYPE_INPUT_NET)  &&
           (funit->sigs[i]->suppl.part.type != SSUPPL_TYPE_INPUT_REG)  &&
           (funit->sigs[i]->suppl.part.type != SSUPPL_TYPE_OUTPUT_NET) &&
           (funit->sigs[i]->suppl.part.type != SSUPPL_TYPE_OUTPUT_REG) &&
           (funit->sigs[i]->suppl.part.type != SSUPPL_TYPE_INOUT_NET)  &&
           (funit->sigs[i]->suppl.part.type != SSUPPL_TYPE_INOUT_REG) ) i++;

    retval = (i == funit->sig_size);

  }

  PROFILE_END;

  return( retval );

}
#endif /* RUNLIB */

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

  if( funit != NULL ) {

    /* Only begin..end blocks can be unnamed scopes */
    if( (funit->suppl.part.type == FUNIT_NAMED_BLOCK) || (funit->suppl.part.type == FUNIT_ANAMED_BLOCK) ) {
      scope_extract_back( funit->name, back, rest );
      retval = db_is_unnamed_scope( back );
    }

  }

  PROFILE_END;

  return( retval );

}

#ifndef RUNLIB
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

  unsigned int i;

  printf( "%s => %s", get_funit_type( funit->suppl.part.type ), obf_funit( funit->name ) );

  for( i=0; i<funit->sig_size; i++ ) {
    vsignal_display( funit->sigs[i] );
  }

  PROFILE_END;

}

/*!
 \param funit  Pointer to functional unit element to display expressions

 Iterates through expression list of specified functional unit, displaying each expression's
 id.
*/
void funit_display_expressions( func_unit* funit ) { PROFILE(FUNIT_DISPLAY_EXPRESSIONS);

  unsigned int i;

  printf( "%s => %s", get_funit_type( funit->suppl.part.type ), obf_funit( funit->name ) );

  for( i=0; i<funit->exp_size; i++ ) {
    expression_display( funit->exps[i] );
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
  if( funit->suppl.part.etype == 0 ) {

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
      funit->elem.tlist       = tlist;
      funit->suppl.part.etype = 1;
  
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

  if( funit->suppl.part.etype == 0 ) {
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
  if( funit->suppl.part.etype == 0 ) {
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

  bool         first = TRUE;
  unsigned int i;

  for( i=0; i<funit->sig_size; i++ ) {
    vsignal* sig = funit->sigs[i];
    if( (sig->suppl.part.assigned == 0) &&
        (sig->suppl.part.type != SSUPPL_TYPE_PARAM)      &&
        (sig->suppl.part.type != SSUPPL_TYPE_PARAM_REAL) &&
        (sig->suppl.part.type != SSUPPL_TYPE_ENUM)       &&
        (sig->suppl.part.type != SSUPPL_TYPE_MEM)        &&
        (sig->suppl.part.type != SSUPPL_TYPE_GENVAR)     &&
        (sig->suppl.part.type != SSUPPL_TYPE_EVENT) ) {
      if( first ) {
        first = FALSE;
        fprintf( vfile, "  $dumpvars( 1, %s.%s", scope, sig->name );
      } else {
        fprintf( vfile, ",\n                %s.%s", scope, sig->name );
      }
    }
  } 

  if( !first ) {
    fprintf( vfile, " );\n" );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if at least one signal was found that needs to be assigned by the dumpfile.
*/
bool funit_is_one_signal_assigned(
  func_unit* funit  /*!< Pointer to functional unit to check */
) { PROFILE(FUNIT_IS_ONE_SIGNAL_ASSIGNED);

  unsigned int i = 0;

  while( (i < funit->sig_size) && ((funit->sigs[i]->exp_size == 0) || !SIGNAL_ASSIGN_FROM_DUMPFILE( funit->sigs[i] )) ) i++;

  PROFILE_END;

  return( i < funit->sig_size );

}
#endif /* RUNLIB */

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
    sig_link_delete_list( funit->sigs, funit->sig_size, funit->sig_no_rm_index, TRUE );
    funit->sigs            = NULL;
    funit->sig_size        = 0;
    funit->sig_no_rm_index = 1;

    /* Free FSM list */
    fsm_link_delete_list( funit->fsms, funit->fsm_size );
    funit->fsms     = NULL;
    funit->fsm_size = 0;

    /* Free expression list */
    exp_link_delete_list( funit->exps, funit->exp_size, TRUE );
    funit->exps     = NULL;
    funit->exp_size = 0;

    /* Free statement list */
    stmt_link_delete_list( funit->stmt_head );
    funit->stmt_head = NULL;
    funit->stmt_tail = NULL;

#ifndef RUNLIB
    /* Free parameter list */
    mod_parm_dealloc( funit->param_head, TRUE );
    funit->param_head = NULL;
    funit->param_tail = NULL;
#endif /* RUNLIB */

    /* Free race condition block list */
    race_blk_delete_list( funit->race_head );
    funit->race_head = NULL;
    funit->race_tail = NULL;

#ifndef RUNLIB
#ifndef VPI_ONLY
    /* Free generate item list */
    gitem_link_delete_list( funit->gitem_head, TRUE );
    funit->gitem_head = NULL;
    funit->gitem_tail = NULL;
#endif /* VPI_ONLY */

    /* Free statistic structure */
    statistic_dealloc( funit->stat );
#endif /* RUNLIB */

    /* Free tf elements */
    funit_link_delete_list( &(funit->tf_head), &(funit->tf_tail), FALSE );

#ifndef RUNLIB
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
#endif

    /* Free functional unit name */
    free_safe( funit->name, (strlen( funit->name ) + 1) );
    funit->name = NULL;

    /* Free original filename */
    free_safe( funit->orig_fname, (strlen( funit->orig_fname ) + 1) );
    funit->orig_fname = NULL;

    /* Free include filename */
    free_safe( funit->incl_fname, (strlen( funit->incl_fname ) + 1) );
    funit->incl_fname = NULL;

    /* Free functional unit version */
    free_safe( funit->version, (strlen( funit->version ) + 1) );
    funit->version = NULL;

    /* Free thread list, if available */
    if( funit->suppl.part.etype == 1 ) {
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
