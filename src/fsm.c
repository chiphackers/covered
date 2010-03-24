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
 \file     fsm.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 
 \par What is an FSM?
 An Finite State Machine (FSM) consists of basically three elements:
 -# Pointer to an input state expression
 -# Pointer to an output state expression
 -# Pointer to an FSM arc (please see arc.c for more information)
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "arc.h"
#include "binding.h"
#include "codegen.h"
#include "db.h"
#include "defines.h"
#include "exclude.h"
#include "expr.h"
#include "func_unit.h"
#include "fsm.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "report.h"
#include "sim.h"
#include "util.h"
#include "vector.h"


extern db**         db_list;
extern unsigned int curr_db;
extern bool         report_covered; 
extern unsigned int report_comb_depth;
extern bool         report_instance;
extern char         user_msg[USER_MSG_LENGTH];
extern isuppl       info_suppl;
extern bool         report_exclusions;
extern bool         flag_output_exclusion_ids;


/*!
 \return Returns a pointer to the newly allocated FSM structure.

 Allocates and initializes an FSM structure.
*/
fsm* fsm_create(
  expression* from_state,  /*!< Pointer to expression that is input state variable for this FSM */
  expression* to_state,    /*!< Pointer to expression that is output state variable for this FSM */
  bool        exclude      /*!< Value to set the exclude bit to */
) { PROFILE(FSM_CREATE);

  fsm* table;  /* Pointer to newly created FSM */

  table             = (fsm*)malloc_safe( sizeof( fsm ) );
  table->name       = NULL;
  table->from_state = from_state;
  table->to_state   = to_state;
  table->arc_head   = NULL;
  table->arc_tail   = NULL;
  table->table      = NULL;
  table->exclude    = exclude;

  return( table );

}

#ifndef RUNLIB
/*!
 Adds new FSM arc structure to specified FSMs arc list.
*/
void fsm_add_arc(
  fsm*        table,       /*!< Pointer to FSM structure to add new arc to */
  expression* from_state,  /*!< Pointer to from_state expression to add */
  expression* to_state     /*!< Pointer to to_state expression to add */
) { PROFILE(FSM_ADD_ARC);

  fsm_arc* arc;  /* Pointer to newly created FSM arc structure */

  /* Create an initialize specified arc */
  arc             = (fsm_arc*)malloc_safe( sizeof( fsm_arc ) );
  arc->from_state = from_state;
  arc->to_state   = to_state;
  arc->next       = NULL;

  /* Add new arc to specified FSM structure */
  if( table->arc_head == NULL ) {
    table->arc_head = table->arc_tail = arc;
  } else {
    table->arc_tail->next = arc;
    table->arc_tail       = arc;
  }

  PROFILE_END;

}

/*!
 After the FSM signals are sized, this function is called to size
 an FSM structure (allocate memory for its tables) and the associated
 FSM arc list is parsed, setting the appropriate bit in the valid table.
*/
void fsm_create_tables(
  fsm* table  /*!< Pointer to FSM structure to set table sizes to */
) { PROFILE(FSM_CREATE_TABLES);

  fsm_arc* curr_arc;    /* Pointer to current FSM arc structure */
  bool     set = TRUE;  /* Specifies if specified bit was set */
  sim_time time;        /* Current simulation time */

  /* Create the FSM arc transition table */
  assert( table != NULL );
  assert( table->to_state != NULL );
  assert( table->to_state->value != NULL );
  assert( table->table == NULL );
  table->table = arc_create( table->to_state->value->width );

  /* Initialize the current time */
  time.lo    = 0;
  time.hi    = 0;
  time.full  = 0;
  time.final = FALSE;

  /* Set valid table */
  curr_arc = table->arc_head;
  while( (curr_arc != NULL) && set ) {

    /* Evaluate from and to state expressions */
    (void)expression_operate( curr_arc->from_state, NULL, &time );
    (void)expression_operate( curr_arc->to_state, NULL, &time );

    /* Set table entry in table, if possible */
    arc_add( table->table, curr_arc->from_state->value, curr_arc->to_state->value, 0, table->exclude );

    curr_arc = curr_arc->next;

  } 

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Outputs the contents of the specified FSM to the specified CDD file.
*/
void fsm_db_write(
  fsm*  table,      /*!< Pointer to FSM structure to output */
  FILE* file,       /*!< Pointer to file output stream to write to */
  bool  ids_issued  /*!< Set to TRUE if expression IDs were just issued */
) { PROFILE(FSM_DB_WRITE);

  fprintf( file, "%d %d %d ",
    DB_TYPE_FSM,
    expression_get_id( table->from_state, ids_issued ),
    expression_get_id( table->to_state, ids_issued )
  );

  /* Print set table */
  if( table->table != NULL ) {
    fprintf( file, "1 " );
    arc_db_write( table->table, file );

    /* Deallocate the given table after writing it */
    if( table->table != NULL ) {
      arc_dealloc( table->table );
      table->table = NULL;
    }
  } else {
    fprintf( file, "0" );
  }

  fprintf( file, "\n" );

  PROFILE_END;

} 

/*!
 \throws anonymous expression_create Throw Throw Throw Throw arc_db_read

 Reads in contents of FSM line from CDD file and stores newly created
 FSM into the specified functional unit.
*/
void fsm_db_read(
  /*@out@*/ char**     line,  /*!< Pointer to current line being read from the CDD file */
            func_unit* funit  /*!< Pointer to current functional unit */
) { PROFILE(FSM_DB_READ);

  int         iexp_id;        /* Input expression ID */
  int         oexp_id;        /* Output expression ID */
  expression* iexp;
  expression* oexp;
  int         chars_read;     /* Number of characters read from sscanf */
  fsm*        table;          /* Pointer to newly created FSM structure from CDD */
  int         is_table;       /* Holds value of is_table entry of FSM output */
 
  if( sscanf( *line, "%d %d %d%n", &iexp_id, &oexp_id, &is_table, &chars_read ) == 3 ) {

    *line = *line + chars_read + 1;

    if( funit == NULL ) {

      print_output( "Internal error:  FSM in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      /* Find specified signal */
      if( ((iexp = exp_link_find( iexp_id, funit->exps, funit->exp_size )) != NULL) &&
          ((oexp = exp_link_find( oexp_id, funit->exps, funit->exp_size )) != NULL) ) {

        /* Create new FSM */
        table = fsm_create( iexp, oexp, FALSE );

        /*
         If the input state variable is the same as the output state variable, create the new expression now.
        */
        if( iexp_id == oexp_id ) {
          Try {
            table->from_state = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, iexp_id, 0, 0, 0, 0, 0, FALSE );
          } Catch_anonymous {
            fsm_dealloc( table );
            Throw 0;
          }
          vector_dealloc( table->from_state->value );
          bind_append_fsm_expr( table->from_state, iexp, funit );
        } else {
          table->from_state = iexp;
        }

        /* Set input/output expression tables to point to this FSM */
        table->from_state->table = table;
        table->to_state->table   = table;
  
        /* Now read in set table */
        if( is_table == 1 ) {

          Try {
            arc_db_read( &(table->table), line );
          } Catch_anonymous {
            fsm_dealloc( table );
            Throw 0;
          }

        }

        /* Add fsm to current functional unit */
        fsm_link_add( table, &(funit->fsms), &(funit->fsm_size) );
 
      } else {

        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to find state variable expressions (%d, %d) for current FSM", iexp_id, oexp_id );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;

      }

    }

  } else {

    print_output( "Unable to parse FSM line in database file.  Unable to read.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \throws anonymous arc_db_merge Throw

 Parses specified line for FSM information and performs merge of the base
 and in FSMs, placing the resulting merged FSM into the base signal.  If
 the FSMs are found to be unalike (names are different), an error message
 is displayed to the user.  If both FSMs are the same, perform the merge
 on the FSM's tables.
*/
void fsm_db_merge(
  fsm*   base,  /*!< Pointer to FSM structure to merge data into */
  char** line   /*!< Pointer to read in line from CDD file to merge */
) { PROFILE(FSM_DB_MERGE);

  int iid;         /* Input state variable expression ID */
  int oid;         /* Output state variable expression ID */
  int chars_read;  /* Number of characters read from line */
  int is_table;    /* Holds value of is_table signifier */

  assert( base != NULL );
  assert( base->from_state != NULL );
  assert( base->to_state != NULL );

  if( sscanf( *line, "%d %d %d%n", &iid, &oid, &is_table, &chars_read ) == 3 ) {

    *line = *line + chars_read + 1;

    if( is_table == 1 ) {

      arc_db_merge( base->table, line );
          
    }

  } else {

    print_output( "Database being merged is not compatible with the original database.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Merges two FSMs, placing the resulting FSM into the base.  This function is called when merging modules for
 the GUI.
*/
void fsm_merge(
  fsm* base,  /*!< Base FSM to store merged results */
  fsm* other  /*!< Other FSM that will be merged with the base FSM */
) { PROFILE(FSM_MERGE);

  assert( base != NULL );
  assert( base->from_state != NULL );
  assert( base->to_state != NULL );
  assert( other != NULL );
  assert( other->from_state != NULL );
  assert( other->to_state != NULL );

  if( base->table != NULL ) {
    assert( other->table != NULL );
    arc_merge( base->table, other->table );
  }

  PROFILE_END;

}

/*!
 Taking the from and to state signal values, a new table entry is added
 to the specified FSM structure arc array (if an entry does not already
 exist in the array).
*/
void fsm_table_set(
  expression*     expr,  /*!< Pointer to expression that contains FSM table to modify */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(FSM_TABLE_SET);

  /* If the expression is the input state expression, make sure that the output state expression is simulated this clock period */
  if( (expr->table->from_state->id == expr->id) && (expr->table->from_state->id != expr->table->to_state->id) ) {

    sim_expr_changed( expr->table->to_state, time );

  /* Otherwise, add the state/state transition */
  } else {

    /* Add the states and state transition */
    arc_add( expr->table->table, expr->table->from_state->value, expr->table->to_state->value, 1, expr->table->exclude );

    /* If from_state was not specified, we need to copy the current contents of to_state to from_state */
    if( expr->table->from_state->id == expr->id ) {
      vector_copy( expr->value, expr->table->from_state->value );
    }

  }

  PROFILE_END;

}

/*!
 Assigns the given value to the FSM structure and evaluates it for coverage information.
*/
void fsm_vcd_assign(
  fsm*  table,  /*!< Pointer to the FSM table to set */
  char* value   /*!< String version of value to set to the FSM table */
) { PROFILE(FSM_VCD_ASSIGN);

  /* Assign the string value to the given state vectors */
  if( table->from_state->id == table->to_state->id ) {
    (void)vector_vcd_assign( table->to_state->value, value, (table->to_state->value->width - 1), 0 );
  } else {
    (void)vector_vcd_assign2( table->to_state->value, table->from_state->value, value, ((table->from_state->value->width + table->to_state->value->width) - 1), 0 );
  }

  /* Add the states and state transition */
  arc_add( table->table, table->from_state->value, table->to_state->value, 1, table->exclude );

  /* If from_state was not specified, we need to copy the current contents of to_state to from_state */
  if( table->from_state->id == table->to_state->id ) {
    vector_copy( table->to_state->value, table->from_state->value );
  }

  PROFILE_END;

}

/*!
 Gathers the FSM state and state transition statistics for the given table and assigns this
 information to the specified pointers.
*/
void fsm_get_stats(
            fsm**        table,        /*!< Pointer to FSM array to get statistics from */
            unsigned int table_size,   /*!< Number of elements in table array */
  /*@out@*/ int*         state_hit,    /*!< Number of states reached in this FSM */
  /*@out@*/ int*         state_total,  /*!< Total number of states within this FSM */
  /*@out@*/ int*         arc_hit,      /*!< Number of arcs reached in this FSM */
  /*@out@*/ int*         arc_total,    /*!< Total number of arcs within this FSM */
  /*@out@*/ int*         arc_excluded  /*!< Total number of excluded arcs */
) { PROFILE(FSM_GET_STATS);

  unsigned int i;

  for( i=0; i<table_size; i++ ) {
    arc_get_stats( table[i]->table, state_hit, state_total, arc_hit, arc_total, arc_excluded );
  }

  PROFILE_END;

}

/*!
 Retrieves the FSM summary information for the specified functional unit.
*/
void fsm_get_funit_summary(
            func_unit* funit,     /*!< Pointer to functional unit */
  /*@out@*/ int*       hit,       /*!< Pointer to location to store the number of hit state transitions for the specified functional unit */
  /*@out@*/ int*       excluded,  /*!< Pointer to number of excluded arcs */
  /*@out@*/ int*       total      /*!< Pointer to location to store the total number of state transitions for the specified functional unit */
) { PROFILE(FSM_GET_FUNIT_SUMMARY);

  *hit      = funit->stat->arc_hit;
  *excluded = funit->stat->arc_excluded;
  *total    = funit->stat->arc_total;

  PROFILE_END;

}

/*!
 Retrieves the FSM summary information for the specified functional unit instance.
*/
void fsm_get_inst_summary(
            funit_inst* inst,      /*!< Pointer to functional unit instance */
  /*@out@*/ int*        hit,       /*!< Pointer to location to store the number of hit state transitions for the specified functional unit */
  /*@out@*/ int*        excluded,  /*!< Pointer to number of excluded arcs */
  /*@out@*/ int*        total      /*!< Pointer to location to store the total number of state transitions for the specified functional unit */
) { PROFILE(FSM_GET_INST_SUMMARY);

  *hit      = inst->stat->arc_hit;
  *excluded = inst->stat->arc_excluded;
  *total    = inst->stat->arc_total;

  PROFILE_END;

}

/*!
 Recursively iterates through specified expression, adding the signal of each expression that
 points to one to the specified signal list.  Also captures the expression ID of the statement
 containing this signal for each signal found (if expr_id is a non-negative value).
*/
static void fsm_gather_signals(
            expression*   expr,             /*!< Pointer to expression to get signals from */
  /*@out@*/ vsignal***    sigs,             /*!< Pointer to head of signal list to populate */
  /*@out@*/ unsigned int* sig_size,         /*!< Pointer to tail of signal list to populate */
  /*@out@*/ unsigned int* sig_no_rm_index,  /*!< Pointer to starting index of signals to not remove */
            int           expr_id,          /*!< Expression ID of the statement containing this expression */
  /*@out@*/ int**         expr_ids,         /*!< Pointer to expression ID array */
  /*@out@*/ int*          expr_id_size      /*!< Number of elements currently stored in expr_ids array */
) { PROFILE(FSM_GATHER_SIGNALS);

  if( expr != NULL ) {

    if( expr->sig != NULL ) {

      /* Add this signal to the list */
      sig_link_add( expr->sig, TRUE, sigs, sig_size, sig_no_rm_index );

      /* Add specified expression ID to the expression IDs array, if needed */
      if( expr_id >= 0 ) {
        (*expr_ids)                  = (int*)realloc_safe( *expr_ids, (sizeof( int ) * (*expr_id_size)), (sizeof( int ) * ((*expr_id_size) + 1)) );
        (*expr_ids)[(*expr_id_size)] = expr_id;
        (*expr_id_size)++;
      }

    } else {

      fsm_gather_signals( expr->left,  sigs, sig_size, sig_no_rm_index, expr_id, expr_ids, expr_id_size );
      fsm_gather_signals( expr->right, sigs, sig_size, sig_no_rm_index, expr_id, expr_ids, expr_id_size );

    }

  }

  PROFILE_END;

}

/*!
 Gathers the covered or uncovered FSM information, storing their expressions in the sig_head/sig_tail signal list.
 Used by the GUI for verbose FSM output.
*/
void fsm_collect(
            func_unit*    funit,            /*!< Pointer to functional unit */
            int           cov,              /*!< Specifies if we are attempting to get uncovered (0) or covered (1) FSMs */
  /*@out@*/ vsignal***    sigs,             /*!< Pointer to the signal array of covered FSM output states */
  /*@out@*/ unsigned int* sig_size,         /*!< Pointer to number of elements in sigs array */
  /*@out@*/ unsigned int* sig_no_rm_index,  /*!< Pointer to index of first element that should not be removed */
  /*@out@*/ int**         expr_ids,         /*!< Pointer to array of expression IDs for each uncovered signal */
  /*@out@*/ int**         excludes          /*!< Pointer to array of exclude values for each uncovered signal */
) { PROFILE(FSM_COLLECT);

  int          size = 0;  /* Number of expressions IDs stored in expr_ids array */
  unsigned int i;

  /* Initialize list pointers */
  *sigs            = NULL;
  *sig_size        = 0;
  *sig_no_rm_index = 1;
  *expr_ids = *excludes = NULL;

  for( i=0; i<funit->fsm_size; i++ ) {

    /* Get the state and arc statistics */
    int state_hit    = 0;
    int state_total  = 0;
    int arc_hit      = 0;
    int arc_total    = 0;
    int arc_excluded = 0;
    arc_get_stats( funit->fsms[i]->table, &state_hit, &state_total, &arc_hit, &arc_total, &arc_excluded );

    /* Allocate some more memory for the excluded array */
    *excludes = (int*)realloc_safe( *excludes, (sizeof( int ) * size), (sizeof( int ) * (size + 1)) );

    /* If the total number of arcs is not known, consider this FSM as uncovered */
    if( (cov == 0) && ((arc_total == -1) || (arc_total != arc_hit)) ) {
      (*excludes)[size] = 0;
      fsm_gather_signals( funit->fsms[i]->to_state, sigs, sig_size, sig_no_rm_index, funit->fsms[i]->to_state->id, expr_ids, &size );
    } else {
      if( (cov == 0) && arc_are_any_excluded( funit->fsms[i]->table ) ) {
        fsm_gather_signals( funit->fsms[i]->to_state, sigs, sig_size, sig_no_rm_index, funit->fsms[i]->to_state->id, expr_ids, &size );
        (*excludes)[size] = 1;
      } else if( cov == 1 ) {
        fsm_gather_signals( funit->fsms[i]->to_state, sigs, sig_size, sig_no_rm_index, -1, expr_ids, &size );
      }
    }

  }

  PROFILE_END;

}

/*!
 Gets the FSM coverage information for the specified FSM in the specified functional unit.  Used by the GUI
 for creating the contents of the verbose FSM viewer.
*/
void fsm_get_coverage(
            func_unit*    funit,               /*!< Pointer to functional unit */
            int           expr_id,             /*!< Expression ID of output state expression to find */
  /*@out@*/ char***       total_fr_states,     /*!< Pointer to a string array containing all possible from states in this FSM */
  /*@out@*/ unsigned int* total_fr_state_num,  /*!< Pointer to the number of elements in the total_fr_states array */
  /*@out@*/ char***       total_to_states,     /*!< Pointer to a string array containing all possible to states in this FSM */
  /*@out@*/ unsigned int* total_to_state_num,  /*!< Pointer to the number of elements in the total_to_states array */
  /*@out@*/ char***       hit_fr_states,       /*!< Pointer to a string array containing the hit fr_states in this FSM */
  /*@out@*/ unsigned int* hit_fr_state_num,    /*!< Pointer to the number of elements in the hit_fr_states array */
  /*@out@*/ char***       hit_to_states,       /*!< Pointer to a string array containing the hit to_states in this FSM */
  /*@out@*/ unsigned int* hit_to_state_num,    /*!< Pointer to the number of elements in the hit_to_states array */
  /*@out@*/ char***       total_from_arcs,     /*!< Pointer to a string array containing all possible state transition from states */
  /*@out@*/ char***       total_to_arcs,       /*!< Pointer to a string array containing all possible state transition to states */
  /*@out@*/ int**         total_ids,           /*!< Pointer to an integer array containing the arc transition IDs for each transition */
  /*@out@*/ int**         excludes,            /*!< Pointer to an integer array containing the exclude values for each state transition */
  /*@out@*/ char***       reasons,             /*!< Pointer to a string array containing exclusion reasons */
  /*@out@*/ int*          total_arc_num,       /*!< Pointer to the number of elements in both the total_from_arcs, total_to_arcs and excludes arrays */
  /*@out@*/ char***       hit_from_arcs,       /*!< Pointer to a string array containing the hit state transition from states */
  /*@out@*/ char***       hit_to_arcs,         /*!< Pointer to a string array containing the hit state transition to states */
  /*@out@*/ int*          hit_arc_num,         /*!< Pointer to the number of elements in both the hit_from_arcs and hit_to_arcs arrays */
  /*@out@*/ char***       input_state,         /*!< Pointer to a string array containing the code for the input state expression */
  /*@out@*/ unsigned int* input_size,          /*!< Pointer to the number of elements stored in the input state array */
  /*@out@*/ char***       output_state,        /*!< Pointer to a string array containing the code for the output state expression */
  /*@out@*/ unsigned int* output_size          /*!< Pointer to the number of elements stored in the output state array */
) { PROFILE(FSM_GET_COVERAGE);

  int*         tmp_ids;
  int*         tmp;
  char**       tmp_reasons;
  unsigned int i = 0;
  unsigned int fr_width;
  unsigned int to_width;

  while( (i < funit->fsm_size) && (funit->fsms[i]->to_state->id != expr_id) ) i++;
  assert( i < funit->fsm_size );

  /* Calculate the from and to width values */
  fr_width = funit->fsms[i]->from_state->value->width;
  to_width = funit->fsms[i]->to_state->value->width;

  /* Get state information */
  arc_get_states( total_fr_states, total_fr_state_num, total_to_states, total_to_state_num, funit->fsms[i]->table, TRUE, TRUE,  fr_width, to_width ); 
  arc_get_states( hit_fr_states,   hit_fr_state_num,   hit_to_states,   hit_to_state_num,   funit->fsms[i]->table, TRUE, FALSE, fr_width, to_width );

  /* Get state transition information */
  arc_get_transitions( total_from_arcs, total_to_arcs, total_ids, excludes, reasons,      total_arc_num, funit->fsms[i]->table, funit, TRUE, TRUE,  fr_width, to_width );
  arc_get_transitions( hit_from_arcs,   hit_to_arcs,   &tmp_ids,  &tmp,     &tmp_reasons, hit_arc_num,   funit->fsms[i]->table, funit, TRUE, FALSE, fr_width, to_width );

  /* Get input state code */
  codegen_gen_expr( funit->fsms[i]->from_state, NULL, input_state, input_size );

  /* Get output state code */
  codegen_gen_expr( funit->fsms[i]->to_state, NULL, output_state, output_size );

  /* Deallocate unused state information */
  if( *hit_arc_num > 0 ) {
    unsigned int i;
    free_safe( tmp_ids, (sizeof( int ) * (*hit_arc_num)) );
    free_safe( tmp,     (sizeof( int ) * (*hit_arc_num)) );
    for( i=0; i<(*hit_arc_num); i++ ) {
      free_safe( tmp_reasons[i], (strlen( tmp_reasons[i] ) + 1) );
    }
    free_safe( tmp_reasons, (sizeof( char* ) * (*hit_arc_num)) );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if at least one FSM state or FSM state transition was found to be missed

 Calculates and displays the FSM state and state transition instance summary information for the
 given instance.
*/
static bool fsm_display_instance_summary(
  FILE*       ofile,        /*!< Pointer to output file to write data to */
  const char* name,         /*!< Name of instance to display */
  int         state_hit,    /*!< Number of FSM states hit in the given instance */
  int         state_total,  /*!< Total number of FSM states in the given instance */
  int         arc_hit,      /*!< Number of FSM state transitions in the given instance */
  int         arc_total     /*!< Total number of FSM state transitions in the given instance */
) { PROFILE(FSM_DISPLAY_INSTANCE_SUMMARY);

  float state_percent;  /* Percentage of states hit */
  float arc_percent;    /* Percentage of arcs hit */
  int   state_miss;     /* Number of states missed */
  int   arc_miss;       /* Number of arcs missed */

  if( (state_total == -1) || (arc_total == -1) ) {
    fprintf( ofile, "  %-43.43s    %4d/  ? /  ?        ? %%         %4d/  ? /  ?        ? %%\n",
             name, state_hit, arc_hit );
    state_miss = arc_miss = 1;
  } else {
    calc_miss_percent( state_hit, state_total, &state_miss, &state_percent );
    calc_miss_percent( arc_hit, arc_total, &arc_miss, &arc_percent );
    fprintf( ofile, "  %-43.43s    %4d/%4d/%4d      %3.0f%%         %4d/%4d/%4d      %3.0f%%\n",
             name, state_hit, state_miss, state_total, state_percent, arc_hit, arc_miss, arc_total, arc_percent );
  }

  PROFILE_END;

  return( (state_miss > 0) || (arc_miss > 0) );

}

/*!
 \return Returns TRUE if any FSM states/arcs were found missing; otherwise, returns FALSE.

 Generates an instance summary report of the current FSM states and arcs hit during simulation.
*/
static bool fsm_instance_summary(
            FILE*       ofile,        /*!< Pointer to output file to display report contents to */
            funit_inst* root,         /*!< Pointer to current root of instance tree to report */
            char*       parent_inst,  /*!< String containing Verilog hierarchy of this instance's parent */
  /*@out@*/ int*        state_hits,   /*!< Pointer to total number of states hit in design */
  /*@out@*/ int*        state_total,  /*!< Pointer to total number of states in design */
  /*@out@*/ int*        arc_hits,     /*!< Pointer to total number of arcs traversed */
  /*@out@*/ int*        arc_total     /*!< Pointer to total number of arcs in design */
) { PROFILE(FSM_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder for instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if at least state or arc was not hit */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Generate printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) || root->suppl.name_diff ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname ); 
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && root->stat->show && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    miss_found |= fsm_display_instance_summary( ofile, tmpname, root->stat->state_hit, root->stat->state_total, root->stat->arc_hit, root->stat->arc_total );

    /* Update accumulated information */
    *state_hits += root->stat->state_hit; 
    if( (root->stat->state_total == -1) || (*state_total == -1) ) {
      *state_total = -1;
    } else {
      *state_total += root->stat->state_total;
    }
    *arc_hits += root->stat->arc_hit;
    if( (root->stat->arc_total == -1) || (*arc_total == -1) ) {
      *arc_total = -1;
    } else {
      *arc_total += root->stat->arc_total;
    }

  }

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= fsm_instance_summary( ofile, curr, tmpname, state_hits, state_total, arc_hits, arc_total );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 \return Returns TRUE if at least one FSM state or FSM arc was missed during simulation for this functional unit; otherwise, returns FALSE.

 Outputs the summary FSM state/arc information for a given functional unit to the given output stream.
*/
static bool fsm_display_funit_summary(
  FILE*       ofile,        /*!< Pointer to file stream to output summary information to */
  const char* name,         /*!< Name of functional unit being reported */
  const char* fname,        /*!< Filename containing the functional unit being reported */
  int         state_hits,   /*!< Number of FSM states that were hit in this functional unit during simulation */
  int         state_total,  /*!< Number of total FSM states that exist in the given functional unit */
  int         arc_hits,     /*!< Number of FSM arcs that were hit in this functional unit during simulation */
  int         arc_total     /*!< Number of total FSM arcs that exist in the given functional unit */
) { PROFILE(FSM_DISPLAY_FUNIT_SUMMARY);

  float state_percent;  /* Percentage of states hit */
  float arc_percent;    /* Percentage of arcs hit */
  int   state_miss;     /* Number of states missed */
  int   arc_miss;       /* Number of arcs missed */

  if( (state_total == -1) || (arc_total == -1) ) {
    fprintf( ofile, "  %-20.20s    %-20.20s   %4d/  ? /  ?        ? %%         %4d/  ? /  ?        ? %%\n",
             name, fname, state_hits, arc_hits );
    state_miss = arc_miss = 1;
  } else {
    calc_miss_percent( state_hits, state_total, &state_miss, &state_percent );
    calc_miss_percent( arc_hits, arc_total, &arc_miss, &arc_percent );
    fprintf( ofile, "  %-20.20s    %-20.20s   %4d/%4d/%4d      %3.0f%%         %4d/%4d/%4d      %3.0f%%\n",
             name, fname, state_hits, state_miss, state_total, state_percent, arc_hits, arc_miss, arc_total, arc_percent );
  }

  PROFILE_END;

  return( (state_miss > 0) || (arc_miss > 0) );

}

/*!
 \return Returns TRUE if any FSM states/arcs were found missing; otherwise, returns FALSE.

 Generates a functional unit summary report of the current FSM states and arcs hit during simulation.
*/
static bool fsm_funit_summary(
            FILE*       ofile,        /*!< Pointer to output file to display report contents to */
            funit_link* head,         /*!< Pointer to functional unit list to traverse */
  /*@out@*/ int*        state_hits,   /*!< Pointer to number of states that were hit in all functional units */
  /*@out@*/ int*        state_total,  /*!< Pointer to total number of states found in all functional units */
  /*@out@*/ int*        arc_hits,     /*!< Pointer to number of state transitions found in all functional units */
  /*@out@*/ int*        arc_total     /*!< Pointer to total number of state transitions found in all functional units */
) { PROFILE(FSM_FUNIT_SUMMARY);

  bool  miss_found = FALSE;  /* Set to TRUE if state/arc was found to be missed */
  char* pname;               /* Printable version of functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= fsm_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->orig_fname ) ),
                                               head->funit->stat->state_hit, head->funit->stat->state_total,
                                               head->funit->stat->arc_hit, head->funit->stat->arc_total );

      /* Update accumulated information */
      *state_hits += head->funit->stat->state_hit;
      if( (head->funit->stat->state_total == -1) || (*state_total == -1) ) {
        *state_total = -1;
      } else {
        *state_total += head->funit->stat->state_total;
      }
      *arc_hits += head->funit->stat->arc_hit;
      if( (head->funit->stat->arc_total == -1) || (*arc_total == -1) ) {
        *arc_total = -1;
      } else {
        *arc_total += head->funit->stat->arc_total;
      }

      free_safe( pname, (strlen( pname ) + 1) );

    }

    head = head->next;

  }

  PROFILE_END;

  return( miss_found );

}

/*!
 Displays verbose information for hit/missed states to the specified
 output file.
*/
static void fsm_display_state_verbose(
  FILE* ofile,  /*!< File handle of output file to send report output to */
  fsm*  table   /*!< Pointer to FSM structure to output */
) { PROFILE(FSM_DISPLAY_STATE_VERBOSE);

  bool         trans_unknown;  /* Set to TRUE if legal arc transitions are unknown */
  char**       fr_states;      /* String array of all from states */
  unsigned int fr_state_size;  /* Contains the number of elements in the fr_states array */
  char**       to_states;      /* String array of all to states */
  unsigned int to_state_size;  /* Contains the number of elements in the to_states array */
  unsigned int i;              /* Loop iterator */

  /* Figure out if transitions were unknown */
  trans_unknown = (table->table->suppl.part.known == 0);

  if( report_covered || trans_unknown ) {
    fprintf( ofile, "        Hit States\n\n" );
  } else {
    fprintf( ofile, "        Missed States\n\n" );
  }

  /* Create format string */
  fprintf( ofile, "          States\n" );
  fprintf( ofile, "          ======\n" );

  /* Get all of the states in string form */
  arc_get_states( &fr_states, &fr_state_size, &to_states, &to_state_size, table->table, (report_covered || trans_unknown), FALSE,
                  table->from_state->value->width, table->to_state->value->width );

  /* Display all of the found states */
  for( i=0; i<fr_state_size; i++ ) {
    fprintf( ofile, "          %s\n", fr_states[i] );
    free_safe( fr_states[i], (strlen( fr_states[i] ) + 1) );
  }

  fprintf( ofile, "\n" );

  /* Deallocate the states array */
  if( fr_state_size > 0 ) {
    free_safe( fr_states, (sizeof( char* ) * fr_state_size) );
  }
  if( to_state_size > 0 ) {
    for( i=0; i<to_state_size; i++ ) {
      free_safe( to_states[i], (strlen( to_states[i] ) + 1) );
    }
    free_safe( to_states, (sizeof( char* ) * to_state_size) );
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if at least one arc transition was excluded.

 Displays verbose information for hit/missed state transitions to
 the specified output file.
*/
static bool fsm_display_arc_verbose(
  FILE*      ofile,  /*!< File handle of output file to send report output to */
  fsm*       table,  /*!< Pointer to FSM structure to output */
  func_unit* funit,  /*!< Pointer to functional unit containing this FSM */
  rpt_type   rtype   /*!< Specifies the type of report to generate */
) { PROFILE(FSM_DISPLAY_ARC_VERBOSE);

  bool         retval = FALSE;  /* Return value for this function */
  bool         trans_unknown;   /* Set to TRUE if the number of state transitions is known */
  char         fstr[100];       /* Format string */
  char         tmp[20];         /* Temporary string */
  int          width;           /* Width (in characters) of the entire output value */
  int          val_width;       /* Number of bits in output state expression */
  int          len_width;       /* Number of characters needed to store the width of the output state expression */
  char**       from_states;     /* String array containing from_state information */
  char**       to_states;       /* String array containing to_state information */
  int*         ids;             /* List of exclusion IDs per from/to state transition */
  int*         excludes;        /* List of excluded arcs */
  char**       reasons;         /* Exclusion reasons */
  int          arc_size;        /* Number of elements in the from_states and to_states arrays */
  int          i;               /* Loop iterator */
  char         tmpfst[4096];    /* Temporary string holder for from_state value */
  char         tmptst[4096];    /* Temporary string holder for to_state value */
  unsigned int rv;              /* Return value from snprintf calls */
  char         spaces[30];      /* Placeholder for spaces */
  unsigned int eid_size;
  char*        eid;

  /* Figure out if transactions were known */
  trans_unknown = (table->table->suppl.part.known == 0);

  spaces[0] = '\0';

  if( (rtype == RPT_TYPE_HIT) || trans_unknown ) {
    fprintf( ofile, "        Hit State Transitions\n\n" );
  } else if( rtype == RPT_TYPE_MISS ) {
    fprintf( ofile, "        Missed State Transitions\n\n" );
  } else if( rtype == RPT_TYPE_EXCL ) {
    fprintf( ofile, "        Excluded State Transitions\n\n" );
  }

  val_width = table->to_state->value->width;

  /* Calculate width of length string */
  rv = snprintf( tmp, 20, "%d", val_width );
  assert( rv < 20 );
  len_width = strlen( tmp );

  /* Create format string to hold largest output value */
  width = ((val_width % 4) == 0) ? (val_width / 4) : ((val_width / 4) + 1);
  width = width + len_width + 2;
  width = (width > 10) ? width : 10;

  /* Generate format string */
  rv = snprintf( fstr, 100, "          %%s%%-%d.%ds %%s %%-%d.%ds\n", width, width, width, width );
  assert( rv < 100 );

  if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) && !trans_unknown ) {
    gen_char_string( spaces, ' ', ((db_get_exclusion_id_size() - 1) + 4) );
    eid_size = db_get_exclusion_id_size() + 4;
    eid      = (char*)malloc_safe( eid_size );
  } else {
    eid_size = 1;
    eid      = (char*)malloc_safe( eid_size );
    eid[0]   = '\0';
  }

  fprintf( ofile, fstr, spaces, "From State", "  ", "To State" );
  fprintf( ofile, fstr, spaces, "==========", "  ", "==========" );

  /* Get the state transition information */
  arc_get_transitions( &from_states, &to_states, &ids, &excludes, &reasons, &arc_size, table->table, funit, ((rtype == RPT_TYPE_HIT) || trans_unknown), FALSE,
                       table->from_state->value->width, table->to_state->value->width );

  /* Output the information to the specified output stream */
  for( i=0; i<arc_size; i++ ) {
    exclude_reason* er;
    retval |= excludes[i];
    if( ((rtype != RPT_TYPE_EXCL) && (excludes[i] == 0)) ||
        ((rtype == RPT_TYPE_EXCL) && (excludes[i] == 1)) ) {
      rv = snprintf( tmpfst, 4096, "%s", from_states[i] );
      assert( rv < 4096 );
      rv = snprintf( tmptst, 4096, "%s", to_states[i] );
      assert( rv < 4096 );
      if( flag_output_exclusion_ids && (rtype != RPT_TYPE_HIT) && !trans_unknown ) {
        rv = snprintf( eid, eid_size, "(%s)  ", db_gen_exclusion_id( 'F', ids[i] ) );
        assert( rv < eid_size );
      }
      fprintf( ofile, fstr, eid, tmpfst, "->", tmptst );
    }
    if( (rtype == RPT_TYPE_EXCL) && (reasons[i] != NULL) ) {
      if( flag_output_exclusion_ids ) {
        report_output_exclusion_reason( ofile, (16 + (db_get_exclusion_id_size() - 1)), reasons[i], TRUE );
      } else {
        report_output_exclusion_reason( ofile, 12, reasons[i], TRUE );
      }
    }
    free_safe( from_states[i], (strlen( from_states[i] ) + 1) );
    free_safe( to_states[i], (strlen( to_states[i] ) + 1) );
    free_safe( reasons[i], (strlen( reasons[i] ) + 1) );
  }

  fprintf( ofile, "\n" );

  /* Deallocate memory */
  if( arc_size > 0 ) {
    free_safe( from_states, (sizeof( char* ) * arc_size) );
    free_safe( to_states, (sizeof( char* ) * arc_size) );
    free_safe( ids, (sizeof( int ) * arc_size) );
    free_safe( excludes, (sizeof( int ) * arc_size) );
    free_safe( reasons, (sizeof( char* ) * arc_size) );
  }
  free_safe( eid, eid_size );

  PROFILE_END;

  return( retval );

}

/*!
 Displays the verbose FSM state and state transition information to the specified
 output file.
*/
static void fsm_display_verbose(
  FILE*      ofile,  /*!< File handle of output file to send report output to */
  func_unit* funit   /*!< Pointer to functional unit containing the FSMs to display */
) { PROFILE(FSM_DISPLAY_VERBOSE);

  char**       icode;        /* Verilog output of input state variable expression */
  unsigned int icode_depth;  /* Number of valid entries in the icode array */
  char**       ocode;        /* Verilog output of output state variable expression */
  unsigned int ocode_depth;  /* Number of valid entries in the ocode array */
  unsigned int i;
  unsigned int j;

  for( j=0; j<funit->fsm_size; j++ ) {

    bool found_exclusion;

    if( funit->fsms[j]->from_state->id == funit->fsms[j]->to_state->id ) {
      codegen_gen_expr( funit->fsms[j]->to_state, NULL, &ocode, &ocode_depth );
      fprintf( ofile, "      FSM input/output state (%s)\n\n", ocode[0] );
      for( i=0; i<ocode_depth; i++ ) {
        free_safe( ocode[i], (strlen( ocode[i] ) + 1) );
      }
      free_safe( ocode, (sizeof( char* ) * ocode_depth) );
    } else {
      codegen_gen_expr( funit->fsms[j]->from_state, NULL, &icode, &icode_depth );
      codegen_gen_expr( funit->fsms[j]->to_state,   NULL, &ocode, &ocode_depth );
      fprintf( ofile, "      FSM input state (%s), output state (%s)\n\n", icode[0], ocode[0] );
      for( i=0; i<icode_depth; i++ ) {
        free_safe( icode[i], (strlen( icode[i] ) + 1) );
      }
      free_safe( icode, (sizeof( char* ) * icode_depth) );
      for( i=0; i<ocode_depth; i++ ) {
        free_safe( ocode[i], (strlen( ocode[i] ) + 1) );
      }
      free_safe( ocode, (sizeof( char* ) * ocode_depth) );
    }

    fsm_display_state_verbose( ofile, funit->fsms[j] );
    found_exclusion = fsm_display_arc_verbose( ofile, funit->fsms[j], funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    if( report_exclusions && found_exclusion ) {
      (void)fsm_display_arc_verbose( ofile, funit->fsms[j], funit, RPT_TYPE_EXCL );
    }

    if( (j + 1) < funit->fsm_size ) {
      fprintf( ofile, "      - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" );
    }

  }

  PROFILE_END;

}

/*!
 Generates an instance verbose report of the current FSM states and arcs hit during simulation.
*/
static void fsm_instance_verbose(
  FILE*       ofile,       /*!< Pointer to output file to display report contents to */
  funit_inst* root,        /*!< Pointer to root of instance tree to traverse */
  char*       parent_inst  /*!< String containing name of this instance's parent instance */
) { PROFILE(FSM_INSTANCE_VERBOSE);

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder for instance */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );

  /* Get printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) || root->suppl.name_diff ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( (root->funit != NULL) && !funit_is_unnamed( root->funit ) &&
      ((((root->stat->state_hit < root->stat->state_total) || (root->stat->arc_hit < root->stat->arc_total)) && !report_covered) ||
         (root->stat->state_total == -1) ||
         (root->stat->arc_total   == -1) ||
       (((root->stat->state_hit > 0) || (root->stat->arc_hit > 0)) && report_covered) ||
       ((root->stat->arc_excluded > 0) && report_exclusions)) ) {

    /* Get printable version of functional unit name */
    pname = scope_gen_printable( funit_flatten_name( root->funit ) );

    fprintf( ofile, "\n" );
    switch( root->funit->suppl.part.type ) {
      case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_AFUNCTION    :
      case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_ATASK        :
      case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
      default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, obf_file( root->funit->orig_fname ), tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

    free_safe( pname, (strlen( pname ) + 1) );

    fsm_display_verbose( ofile, root->funit );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    fsm_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

  PROFILE_END;

}

/*! 
 Generates a functional unit verbose report of the current FSM states and arcs hit during simulation.
*/
static void fsm_funit_verbose(
  FILE*       ofile,  /*!< Pointer to output file to display report contents to */
  funit_link* head    /*!< Pointer to head of functional unit list to traverse */
) { PROFILE(FSM_FUNIT_VERBOSE);

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        ((((head->funit->stat->state_hit < head->funit->stat->state_total) || 
           (head->funit->stat->arc_hit < head->funit->stat->arc_total)) && !report_covered) ||
           (head->funit->stat->state_total == -1) ||
           (head->funit->stat->arc_total   == -1) ||
         (((head->funit->stat->state_hit > 0) || (head->funit->stat->arc_hit > 0)) && report_covered) ||
         ((head->funit->stat->arc_excluded > 0) && report_exclusions)) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      fprintf( ofile, "\n" );
      switch( head->funit->suppl.part.type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", pname, obf_file( head->funit->orig_fname ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      free_safe( pname, (strlen( pname ) + 1) );

      fsm_display_verbose( ofile, head->funit );

    }

    head = head->next;

  }

  PROFILE_END;

}

/*!
 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the FSM coverage for each functional unit encountered.  The parent functional unit will
 specify its own FSM coverage along with a total FSM coverage including its 
 children.
*/
void fsm_report(
  FILE* ofile,   /*!< Pointer to file to output results to */
  bool  verbose  /*!< Specifies whether or not to provide verbose information */
) { PROFILE(FSM_REPORT);

  bool       missed_found  = FALSE;  /* If set to TRUE, FSM cases were found to be missed */
  inst_link* instl;                  /* Pointer to current instance link */
  int        acc_st_hits   = 0;      /* Accumulated number of states hit */
  int        acc_st_total  = 0;      /* Accumulated number of states in design */
  int        acc_arc_hits  = 0;      /* Accumulated number of arcs hit */
  int        acc_arc_total = 0;      /* Accumulated number of arcs in design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   FINITE STATE MACHINE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    fprintf( ofile, "                                                               State                             Arc\n" );
    fprintf( ofile, "Instance                                          Hit/Miss/Total    Percent hit    Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= fsm_instance_summary( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*"), &acc_st_hits, &acc_st_total, &acc_arc_hits, &acc_arc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)fsm_display_instance_summary( ofile, "Accumulated", acc_st_hits, acc_st_total, acc_arc_hits, acc_arc_total );
   
    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        fsm_instance_verbose( ofile, instl->inst, (instl->inst->suppl.name_diff ? "<NA>" : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                               State                             Arc\n" );
    fprintf( ofile, "Module/Task/Function      Filename                Hit/Miss/Total    Percent Hit    Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = fsm_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_st_hits, &acc_st_total, &acc_arc_hits, &acc_arc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)fsm_display_funit_summary( ofile, "Accumulated", "", acc_st_hits, acc_st_total, acc_arc_hits, acc_arc_total );

    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      fsm_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Deallocates all allocated memory for the specified FSM structure.
*/
void fsm_dealloc(
  fsm* table  /*!< Pointer to FSM structure to deallocate */
) { PROFILE(FSM_DEALLOC);

  fsm_arc* tmp;  /* Temporary pointer to current FSM arc structure to deallocate */

  if( table != NULL ) {

    /* Free name if one was specified */
    if( table->name != NULL ) {
      free_safe( table->name, (strlen( table->name ) + 1) );
    }

    /* Deallocate tables */
    arc_dealloc( table->table );

    /* Deallocate FSM arc structure */
    while( table->arc_head != NULL ) {
      tmp = table->arc_head;
      table->arc_head = table->arc_head->next;
      expression_dealloc( tmp->to_state, FALSE );
      expression_dealloc( tmp->from_state, FALSE );
      free_safe( tmp, sizeof( fsm_arc ) );
    }

    /*
     Deallocate from_state if it is the same expression ID as the to_state expression and is
     not the same expression structure
    */
    if( (table->from_state != NULL)             &&
        (table->to_state != NULL)               &&
        (table->from_state != table->to_state ) &&
        (table->from_state->id == table->to_state->id) ) {
      expression_dealloc( table->from_state, FALSE );
    }

    /* Deallocate this structure */
    free_safe( table, sizeof( fsm ) );
      
  }

  PROFILE_END;

}

