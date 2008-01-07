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
 \file    vpi.c
 \author  Trevor Williams  (phase1geo@gmail.com)
 \date    5/6/2005
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vpi_user.h"
#ifdef CVER
#include "cv_vpi_user.h"
#endif

#include "binding.h"
#include "db.h"
#include "defines.h"
#include "instance.h"
#include "link.h"
#include "obfuscate.h"
#include "profiler.h"
#include "symtable.h"
#include "util.h"

struct sym_value_s;

/*!
 Renaming sym_value_s structure for convenience.
*/
typedef struct sym_value_s sym_value;

/*!
 Structure used for storing symbol and string value information.
*/
struct sym_value_s {
  char*      sym;                    /*!< Signal symbol */
  char*      value;                  /*!< Signal value to assign */
  sym_value* next;                   /*!< Pointer to next sym_value structure in list */
};

char       in_db_name[1024];       /*!< Name of input CDD file */
char       out_db_name[1024];      /*!< Name of output CDD file */
uint64     last_time     = 0;      /*!< Last simulation time seen from simulator */
bool       use_last_time = FALSE;  /*!< Set to TRUE if last_time has been set by the simulator */
sym_value* sv_head       = NULL;   /*!< Pointer to head of sym_value list */
sym_value* sv_tail       = NULL;   /*!< Pointer to tail of sym_value list */

/* These are needed for compile purposes only */
bool   report_gui          = FALSE;
bool   one_instance_found  = FALSE;
int    timestep_update     = 0;
bool   report_covered      = FALSE;
bool   flag_use_line_width = FALSE;
int    line_width          = DEFAULT_LINE_WIDTH;
bool   report_instance     = FALSE;
int    flag_race_check     = WARNING;
tnode* def_table           = NULL;
int    fork_depth          = -1;
int*   fork_block_depth;
int    block_depth         = 0;
int    merge_in_num;
char** merge_in            = NULL;
char*  top_module          = NULL;
char*  top_instance        = NULL;
int    flag_global_generation = GENERATION_SV;
int    generate_expr_mode  = 0;
bool   cli_debug_mode      = FALSE;
bool   flag_use_command_line_debug = FALSE;

extern bool        debug_mode;
extern symtable*   vcd_symtab;
extern int         vcd_symtab_size;
extern symtable**  timestep_tab;
extern char**      curr_inst_scope;
extern int         curr_inst_scope_size;
extern funit_inst* curr_instance;
extern char        user_msg[USER_MSG_LENGTH];
extern isuppl      info_suppl;


/*!
 \param msg  Message to output to standard output

 Outputs the given message to standard output using the vpi_printf function.
*/
void vpi_print_output( const char* msg ) {

  vpi_printf( "covered VPI: %s\n", msg );

}

/*!
 \param sym    Symbol string of signal to store.
 \param value  Initial signal value to store.

 Stores the given signal symbol and initial value in the sym_value list that
 will be assigned to the simulator once the timestep table has been allocated.
*/
void sym_value_store( char* sym, char* value ) { PROFILE(SYM_VALUE_STORE);

  sym_value* sval;  /* Pointer to newly allocated sym_value structure */

  /* Allocate and initialize the sym_value structure */
  sval = (sym_value*)malloc_safe( sizeof( sym_value ) );
  sval->sym   = strdup_safe( sym );
  sval->value = strdup_safe( value );
  sval->next  = NULL; 

  /* Add the newly created sym_value structure to the sv list */
  if( sv_head == NULL ) {
    sv_head = sv_tail = sval;
  } else {
    sv_tail->next = sval;
    sv_tail       = sval;
  }

  PROFILE_END;

}

/*!
 Iterates through the sym_value list, adding the information to Covered's simulation
 core and deallocating its memory.  Called by the covered_sim_calltf function.
*/
void add_sym_values_to_sim() { PROFILE(ADD_SYM_VALUES_TO_SIM);

  sym_value* sval;  /* Pointer to current sym_value structure */

  sval = sv_head;
  while( sv_head != NULL ) {

    /* Assign the current sv_head to the sval pointer */
    sval    = sv_head;
    sv_head = sv_head->next;

    /* Set the given symbol to the given value in Covered's simulator */
    db_set_symbol_string( sval->sym, sval->value );

    /* Deallocate all memory associated with this sym_table structure */
    free_safe( sval->sym );
    free_safe( sval->value );
    free_safe( sval );
  }

  PROFILE_END;

}

/*!
 \param cb  Pointer to callback data structure from vpi_user.h

 \return Returns 0.

 This callback function is called whenever a signal changes within the simulator.  It places
 this value in the Covered symtable and calls the db_do_timestep if this value change occurred
 on a new timestep.
*/
PLI_INT32 covered_value_change( p_cb_data cb ) { PROFILE(COVERED_VALUE_CHANGE);

#ifndef NOIV
  s_vpi_value value;

  /* Setup value */
  value.format = vpiBinStrVal;
  vpi_get_value( cb->obj, &value );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In covered_value_change, name: %s, time: %lld, value: %s",
            obf_sig( vpi_get_str( vpiFullName, cb->obj ) ), (((uint64)cb->time->high << 32) | (uint64)cb->time->low), value.value.str );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  if( ((cb->time->low  != (PLI_INT32)(last_time & 0xffffffff)) ||
       (cb->time->high != (PLI_INT32)((last_time >> 32) & 0xffffffff))) &&
      use_last_time ) {
    db_do_timestep( last_time, FALSE );
  }
  last_time     = ((uint64)cb->time->high << 32) | (uint64)cb->time->low;
  use_last_time = TRUE;
  
  /* Set symbol value */
  db_set_symbol_string( cb->user_data, value.value.str );
#else
#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In covered_value_change, name: %s, time: %d%0d, value: %s",
            obf_sig( vpi_get_str( vpiFullName, cb->obj ) ), (((uint64)cb->time->high << 32) | (uint64)cb->time->low), cb->value->value.str );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  if( ((cb->time->low  != (PLI_INT32)(last_time & 0xffffffff)) ||
       (cb->time->high != (PLI_INT32)((last_time >> 32) & 0xffffffff))) &&
      use_last_time ) {
    db_do_timestep( last_time, FALSE );
  }
  last_time     = ((uint64)cb->time->high << 32) | (uint64)cb->time->low;
  use_last_time = TRUE;

  /* Set symbol value */
  db_set_symbol_string( cb->user_data, cb->value->value.str );
#endif

  PROFILE_END;

  return( 0 );

}

/*!
*/
PLI_INT32 covered_end_of_sim( p_cb_data cb ) { PROFILE(COVERED_END_OF_SIM);

  p_vpi_time final_time;

  if( use_last_time ) {
    db_do_timestep( last_time, FALSE );
  }

  /* Get the final simulation time */
  final_time       = (p_vpi_time)malloc_safe( sizeof( s_vpi_time ) );
  final_time->type = vpiSimTime;
  vpi_get_time( NULL, final_time );

  /* Flush any pending statement trees that are waiting for delay */
  last_time = ((uint64)final_time->high << 32) | (uint64)final_time->low;
  db_do_timestep( last_time, FALSE );

  /* Perform one last simulation timestep */
  db_do_timestep( 0, TRUE );

  /* Indicate that this CDD contains scored information */
  info_suppl.part.scored = 1;

  /* Write contents to database file */
  if( !db_write( out_db_name, FALSE, FALSE ) ) {
    vpi_printf( "covered VPI: Unable to write database file\n" );
    exit( EXIT_FAILURE );
  } else {
    vpi_printf( "covered VPI: Output coverage information to %s\n", out_db_name );
  }

  /* Deallocate memory */
  symtable_dealloc( vcd_symtab );
  sim_dealloc();
  db_close();
  if( timestep_tab != NULL ) {
    free_safe( timestep_tab );
  }

  PROFILE_END;

  return( 0 );

}

PLI_INT32 covered_cb_error_handler( p_cb_data cb ) { PROFILE(COVERED_CB_ERROR_HANDLER);

  struct t_vpi_error_info einfotab;
  struct t_vpi_error_info *einfop;
  char   s1[128];

  einfop = &einfotab;
  vpi_chk_error( einfop );

  if( einfop->state == vpiCompile ) {
    strcpy( s1, "vpiCompile" );
  } else if( einfop->state == vpiPLI ) {
    strcpy( s1, "vpiPLI" );
  } else if( einfop->state == vpiRun ) {
    strcpy( s1, "vpiRun" );
  } else {
    strcpy( s1, "**unknown**" );
  }

  vpi_printf( "covered VPI: ERR(%s) %s (level %d) at **%s(%d):\n  %s\n",
    einfop->code, s1, einfop->level, obf_file( einfop->file ), einfop->line, einfop->message );

  /* If serious error give up */
  if( (einfop->level == vpiError) || (einfop->level == vpiSystem) || (einfop->level == vpiInternal) ) {
    vpi_printf( "covered VPI: FATAL: encountered error - giving up\n" );
    vpi_control( vpiFinish, 0 );
  }

  PROFILE_END;

  return( 0 );

}

char* gen_next_symbol() { PROFILE(GEN_NEXT_SYMBOL);

  static char symbol[21]   = {32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,'\0'};
  static int  symbol_index = 19;
  int         i            = 19;

  while( (i >= symbol_index) && (symbol[i] == 126) ) {
    symbol[i] = 33;
    if( (i - 1) < symbol_index ) {
      symbol_index--;
      if( symbol_index < 0 ) {
        return( NULL );
      }
    }
    i--;
  }
  symbol[i]++;

  PROFILE_END;

  return( strdup_safe( symbol + symbol_index ) );

}

/*!
 \param sig  Pointer to vpiHandle for a given signal.

 Finds the given VPI signal in Covered's database and creates a callback function that will
 be called whenever this signal changes value during simulation.  Also retrieves the initial
 value of the signal and stores it in the sym_value list and creates a symbol in the symtable
 structure for this signal.
*/
void covered_create_value_change_cb( vpiHandle sig ) { PROFILE(COVERED_CREATE_VALUE_CHANGE_CB);

  p_cb_data   cb;
  vsignal     vsig;
  sig_link*   vsigl;
  char*       symbol;
  s_vpi_value value;

  /* Find current signal in coverage database */
  vsig.name = vpi_get_str( vpiName, sig );

  /* Only add the signal if it is in our database and needs to be assigned from the simulator */
  if( ((vsigl = sig_link_find( &vsig, curr_instance->funit->sig_head )) != NULL) &&
      (vsigl->sig->suppl.part.assigned == 0) ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Adding callback for signal: %s", obf_sig( vsigl->sig->name ) );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Generate new symbol */
    if( (symbol = gen_next_symbol()) == NULL ) {
      vpi_printf( "covered VPI: INTERNAL ERROR:  Unable to generate unique symbol name\n" );
      /* Need to increase number of characters in symbol array */
      vpi_control( vpiFinish, 0 );
    }

    /* Add signal/symbol to symtab database */
    db_assign_symbol( vsigl->sig->name, symbol, ((vsigl->sig->value->width + vsigl->sig->dim[0].lsb) - 1), vsigl->sig->dim[0].lsb ); 

    /* Get initial value of this signal and store it for later retrieval */
    value.format = vpiBinStrVal;
    vpi_get_value( sig, &value );
    sym_value_store( symbol, value.value.str );

    /* Add a callback for a value change to this net */
    cb                   = (p_cb_data)malloc( sizeof( s_cb_data ) );
    cb->reason           = cbValueChange;
    cb->cb_rtn           = covered_value_change;
    cb->obj              = sig;
    cb->time             = (p_vpi_time)malloc( sizeof( s_vpi_time ) ); 
    cb->time->type       = vpiSimTime;
    cb->time->high       = 0;
    cb->time->low        = 0;
#ifdef NOIV
    cb->value            = (p_vpi_value)malloc( sizeof( s_vpi_value ) );
    cb->value->format    = vpiBinStrVal;
    cb->value->value.str = NULL;
#else
    cb->value            = NULL;
#endif
    cb->user_data        = symbol;
    vpi_register_cb( cb );

  }

  PROFILE_END;

}

void covered_parse_task_func( vpiHandle mod ) { PROFILE(COVERED_PARSE_TASK_FUNC);

  vpiHandle iter, handle;
  vpiHandle liter, scope;
  int       type;
	
  /* Parse all internal scopes for tasks and functions */
  if( (iter = vpi_iterate( vpiInternalScope, mod )) != NULL ) {

    while( (scope = vpi_scan( iter )) != NULL ) {
      
      type = vpi_get( vpiType, scope );

      if( (type == vpiTask) || (type == vpiFunction) || (type == vpiNamedBegin) ) {

#ifdef DEBUG_MODE
        snprintf( user_msg, USER_MSG_LENGTH, "Parsing task/function %s", obf_funit( vpi_get_str( vpiFullName, scope ) ) );
        print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

        /* Set current scope in database */
        if( curr_inst_scope[0] != NULL ) {
          free_safe( curr_inst_scope[0] );
        }
        curr_inst_scope[0]   = strdup_safe( vpi_get_str( vpiFullName, scope ) );
        curr_inst_scope_size = 1;

        /* Synchronize curr_instance to point to curr_inst_scope */
        db_sync_curr_instance();

        if( curr_instance != NULL ) {

          /* Parse signals */
          if( (liter = vpi_iterate( vpiReg, scope )) != NULL ) {
            while( (handle = vpi_scan( liter )) != NULL ) {
#ifdef DEBUG_MODE
              snprintf( user_msg, USER_MSG_LENGTH, "Found reg %s", obf_sig( vpi_get_str( vpiFullName, handle ) ) );
              print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
              covered_create_value_change_cb( handle );
            }
          }

          if( (liter = vpi_iterate( vpiVariables, scope )) != NULL ) {
            while( (handle = vpi_scan( liter )) != NULL ) {
              type = vpi_get( vpiType, handle );
#ifdef DEBUG_MODE
              if( type == vpiReg ) {
                snprintf( user_msg, USER_MSG_LENGTH, "Found reg %s", obf_sig( vpi_get_str( vpiFullName, handle ) ) );
              } else if( type == vpiIntegerVar ) {
                snprintf( user_msg, USER_MSG_LENGTH, "Found integer %s", obf_sig( vpi_get_str( vpiFullName, handle ) ) );
              } else if( type == vpiTimeVar ) {
                snprintf( user_msg, USER_MSG_LENGTH, "Found time %s", obf_sig( vpi_get_str( vpiFullName, handle ) ) );
              }
              print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
              covered_create_value_change_cb( handle );
            }
          }

          /* Recursively check scope */
          if( (liter = vpi_iterate( vpiInternalScope, scope )) != NULL ) {
            while( (handle = vpi_scan( liter )) != NULL ) {
              covered_parse_task_func( handle );
            }
          }

        }

      }

    }

  }

  PROFILE_END;

}

void covered_parse_signals( vpiHandle mod ) { PROFILE(COVERED_PARSE_SIGNALS);

  vpiHandle iter, handle;
  int       type;

  /* Parse nets */
  if( (iter = vpi_iterate( vpiNet, mod )) != NULL ) {
    while( (handle = vpi_scan( iter )) != NULL ) {
#ifdef DEBUG_MODE
      snprintf( user_msg, USER_MSG_LENGTH, "Found net: %s", obf_sig( vpi_get_str( vpiName, handle ) ) );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
      covered_create_value_change_cb( handle );
    }
  }

  /* Parse regs */
  if( (iter = vpi_iterate( vpiReg, mod )) != NULL ) {
    while( (handle = vpi_scan( iter )) != NULL ) {
#ifdef DEBUG_MODE
      snprintf( user_msg, USER_MSG_LENGTH, "Found reg: %s", obf_sig( vpi_get_str( vpiName, handle ) ) );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
      covered_create_value_change_cb( handle );
    }
  }

  /* Parse integers */
  if( (iter = vpi_iterate( vpiVariables, mod )) != NULL ) {
    while( (handle = vpi_scan( iter )) != NULL ) {
      type = vpi_get( vpiType, handle );
      if( (type == vpiIntegerVar) || (type == vpiTimeVar) || (type == vpiReg) ) {
#ifdef DEBUG_MODE
        if( type == vpiIntegerVar ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Found integer: %s", obf_sig( vpi_get_str( vpiName, handle ) ) );
        } else if( type == vpiTimeVar ) {
          snprintf( user_msg, USER_MSG_LENGTH, "Found time: %s", obf_sig( vpi_get_str( vpiName, handle ) ) );
        } else {
          snprintf( user_msg, USER_MSG_LENGTH, "Found reg: %s", obf_sig( vpi_get_str( vpiName, handle ) ) );
        }
        print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
        covered_create_value_change_cb( handle );
      }
    }
  }

  PROFILE_END;

}

void covered_parse_instance( vpiHandle inst ) { PROFILE(COVERED_PARSE_INSTANCE);

  vpiHandle iter, handle;

  /* Set current scope in database */
  if( curr_inst_scope[0] != NULL ) {
    free_safe( curr_inst_scope[0] );
  }
  curr_inst_scope[0] = strdup_safe( vpi_get_str( vpiFullName, inst ) );
  curr_inst_scope_size = 1;

  /* Synchronize curr_instance to point to curr_inst_scope */
  db_sync_curr_instance();

  /* If current instance is known, parse this instance */
  if( curr_instance != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Found module to be covered: %s, hierarchy: %s",
              obf_funit( vpi_get_str( vpiName, inst ) ), obf_inst( curr_inst_scope[0] ) );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /*
     Set one_instance_found to TRUE to indicate that we were successful in
     matching our design to the actual design
    */
    one_instance_found = TRUE;

    /* Parse all signals */
    covered_parse_signals( inst );

    /* Parse all functions/tasks */
    covered_parse_task_func( inst );

  }

  /* Search children modules */
  if( (iter = vpi_iterate( vpiModule, inst )) != NULL ) {

    while( (handle = vpi_scan( iter )) != NULL ) {
      covered_parse_instance( handle );
    }

  }

  PROFILE_END;

}

#ifdef NOIV
#ifdef VCS
PLI_INT32 covered_sim_calltf( PLI_BYTE8* name ) {
#else
PLI_INT32 covered_sim_calltf() {
#endif
#else
PLI_INT32 covered_sim_calltf( char* name ) {
#endif
  PROFILE(COVERED_SIM_CALLTF);

  vpiHandle       systf_handle, arg_iterator, module_handle;
  vpiHandle       arg_handle;
  s_vpi_vlog_info info;
  p_cb_data       cb;
  int             i;
  char*           argvptr;
  sig_link*       vsigl;
  s_vpi_value     value;

  systf_handle = vpi_handle( vpiSysTfCall, NULL );
  arg_iterator = vpi_iterate( vpiArgument, systf_handle );

  /* Create callback that will handle the end of simulation */
  cb            = (p_cb_data)malloc( sizeof( s_cb_data ) );
  cb->reason    = cbEndOfSimulation;
  cb->cb_rtn    = covered_end_of_sim;
  cb->obj       = NULL;
  cb->time      = NULL;
  cb->value     = NULL;
  cb->user_data = NULL;
  vpi_register_cb( cb );

#ifdef TBD
  /* Create error handling callback */
  cb            = (p_cb_data)malloc( sizeof( s_cb_data ) );
  cb->reason    = cbError;
  cb->cb_rtn    = covered_cb_error_handler;
  cb->obj       = NULL;
  cb->time      = NULL;
  cb->value     = NULL;
  cb->user_data = NULL;
  vpi_register_cb( cb );
#endif

  /* Get name of CDD database file from system call arguments */
  if( (arg_handle = vpi_scan( arg_iterator )) != NULL ) {
    s_vpi_value data;
    data.format = vpiStringVal;
    vpi_get_value( arg_handle, &data );
    strcpy( in_db_name, data.value.str );
  }

  /* Get name of CDD database to write to (default is cov.cdd) and debug mode */
  strcpy( out_db_name, "cov.cdd" );
  profiler_set_mode( FALSE );
  if( vpi_get_vlog_info( &info ) ) {
    for( i=1; i<info.argc; i++ ) {
      argvptr = info.argv[i];
      if( strncmp( "+covered_cdd=", argvptr, 13 ) == 0 ) {
        argvptr += 13;
        strcpy( out_db_name, argvptr );
      } else if( strncmp( "+covered_debug", argvptr, 14 ) == 0 ) {
        vpi_printf( "covered VPI: Turning debug mode on\n" );
        debug_mode = TRUE;
      } else if( strncmp( "+covered_profile=", argvptr, 17 ) == 0 ) {
        vpi_printf( "covered VPI: Turning profiler on.  Outputting to %s\n", argvptr + 17 );
        profiler_set_mode( TRUE );
        profiler_set_filename( argvptr + 17 );
      } else if( strncmp( "+covered_profile", argvptr, 16 ) == 0 ) {
        vpi_printf( "covered VPI: Turning profiler on.  Outputting to %s\n", PROFILING_OUTPUT_NAME );
        profiler_set_mode( TRUE );
        profiler_set_filename( PROFILING_OUTPUT_NAME );
      }
    }
  }

  /* Initialize all global information variables */
  info_initialize();
    
  /* Read in contents of specified database file */
  if( !db_read( in_db_name, READ_MODE_MERGE_NO_MERGE ) ) {
    vpi_printf( "covered VPI: Unable to read database file\n" );
    exit( EXIT_FAILURE );
  } else {
    vpi_printf( "covered VPI: Read design information from %s\n", in_db_name );
  }

  /* Bind expressions to signals/functional units */
  bind_perform( TRUE, 0 );

  /* Add static values to simulator */
  sim_initialize();

  /* Create initial symbol table */
  vcd_symtab = symtable_create();

  /* Initialize the curr_inst_scope structure */
  curr_inst_scope      = (char**)malloc( sizeof( char* ) );
  curr_inst_scope[0]   = NULL;
  curr_inst_scope_size = 1;

  /* Parse child instances - associate a signal in the design with a signal in Covered */
  while( (module_handle = vpi_scan( arg_iterator )) != NULL ) {
    covered_parse_instance( module_handle );
  }

  /* Create timestep symbol table array */
  if( vcd_symtab_size > 0 ) {
    timestep_tab = malloc_safe_nolimit( (sizeof( symtable*) * vcd_symtab_size) );
  }

  /* Add all of the sym_value structures to the simulation core */
  add_sym_values_to_sim();

  /* If we are Cver or VCS, perform an initial 0 timestep since this will not get called */
#ifdef NOIV
  db_do_timestep( 0, FALSE );
#endif

  PROFILE_END;

  return 0;

}

void covered_register() { PROFILE(COVERED_REGISTER);

  s_vpi_systf_data tf_data;

  tf_data.type      = vpiSysTask;
  tf_data.tfname    = "$covered_sim";
  tf_data.calltf    = covered_sim_calltf;
  tf_data.compiletf = 0;
  tf_data.sizetf    = 0;
  tf_data.user_data = "$covered_sim";
  vpi_register_systf( &tf_data );

  PROFILE_END;

}

#ifndef VCS
void (*vlog_startup_routines[])() = {
	covered_register,
	0
};
#endif

#ifdef CVER
void vpi_compat_bootstrap(void)
{
 int i;

 for (i = 0;; i++)
  {
   if (vlog_startup_routines[i] == NULL) break;
   vlog_startup_routines[i]();
  }
}
#endif

