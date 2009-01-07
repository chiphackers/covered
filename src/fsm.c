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

  int        iexp_id;        /* Input expression ID */
  int        oexp_id;        /* Output expression ID */
  exp_link*  iexpl;          /* Pointer to found state variable */
  exp_link*  oexpl;          /* Pointer to found state variable */
  int        chars_read;     /* Number of characters read from sscanf */
  fsm*       table;          /* Pointer to newly created FSM structure from CDD */
  int        is_table;       /* Holds value of is_table entry of FSM output */
 
  if( sscanf( *line, "%d %d %d%n", &iexp_id, &oexp_id, &is_table, &chars_read ) == 3 ) {

    *line = *line + chars_read + 1;

    if( funit == NULL ) {

      print_output( "Internal error:  FSM in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      /* Find specified signal */
      if( ((iexpl = exp_link_find( iexp_id, funit->exp_head )) != NULL) &&
          ((oexpl = exp_link_find( oexp_id, funit->exp_head )) != NULL) ) {

        /* Create new FSM */
        table = fsm_create( iexpl->exp, oexpl->exp, FALSE );

        /*
         If the input state variable is the same as the output state variable, create the new expression now.
        */
        if( iexp_id == oexp_id ) {
          Try {
            table->from_state = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, iexp_id, 0, 0, 0, FALSE );
          } Catch_anonymous {
            fsm_dealloc( table );
            Throw 0;
          }
          vector_dealloc( table->from_state->value );
          bind_append_fsm_expr( table->from_state, iexpl->exp, funit );
        } else {
          table->from_state = iexpl->exp;
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
        fsm_link_add( table, &(funit->fsm_head), &(funit->fsm_tail) );
 
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
    vector_vcd_assign( table->to_state->value, value, (table->to_state->value->width - 1), 0 );
  } else {
    vector_vcd_assign2( table->to_state->value, table->from_state->value, value, ((table->from_state->value->width + table->to_state->value->width) - 1), 0 );
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
            fsm_link* table,        /*!< Pointer to FSM to get statistics from */
  /*@out@*/ int*      state_hit,    /*!< Number of states reached in this FSM */
  /*@out@*/ int*      state_total,  /*!< Total number of states within this FSM */
  /*@out@*/ int*      arc_hit,      /*!< Number of arcs reached in this FSM */
  /*@out@*/ int*      arc_total,    /*!< Total number of arcs within this FSM */
  /*@out@*/ int*      arc_excluded  /*!< Total number of excluded arcs */
) { PROFILE(FSM_GET_STATS);

  fsm_link* curr;   /* Pointer to current FSM in table list */

  curr = table;
  while( curr != NULL ) {
    arc_get_stats( curr->table->table, state_hit, state_total, arc_hit, arc_total, arc_excluded );
    curr = curr->next;
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
            expression* expr,         /*!< Pointer to expression to get signals from */
  /*@out@*/ sig_link**  head,         /*!< Pointer to head of signal list to populate */
  /*@out@*/ sig_link**  tail,         /*!< Pointer to tail of signal list to populate */
            int         expr_id,      /*!< Expression ID of the statement containing this expression */
  /*@out@*/ int**       expr_ids,     /*!< Pointer to expression ID array */
  /*@out@*/ int*        expr_id_size  /*!< Number of elements currently stored in expr_ids array */
) { PROFILE(FSM_GATHER_SIGNALS);

  if( expr != NULL ) {

    if( expr->sig != NULL ) {

      /* Add this signal to the list */
      sig_link_add( expr->sig, head, tail );

      /* Add specified expression ID to the expression IDs array, if needed */
      if( expr_id >= 0 ) {
        (*expr_ids)                  = (int*)realloc_safe( *expr_ids, (sizeof( int ) * (*expr_id_size)), (sizeof( int ) * ((*expr_id_size) + 1)) );
        (*expr_ids)[(*expr_id_size)] = expr_id;
        (*expr_id_size)++;
      }

    } else {

      fsm_gather_signals( expr->left,  head, tail, expr_id, expr_ids, expr_id_size );
      fsm_gather_signals( expr->right, head, tail, expr_id, expr_ids, expr_id_size );

    }

  }

  PROFILE_END;

}

/*!
 Gathers the covered or uncovered FSM information, storing their expressions in the sig_head/sig_tail signal list.
 Used by the GUI for verbose FSM output.
*/
void fsm_collect(
            func_unit* funit,     /*!< Pointer to functional unit */
            int        cov,       /*!< Specifies if we are attempting to get uncovered (0) or covered (1) FSMs */
  /*@out@*/ sig_link** sig_head,  /*!< Pointer to the head of the signal list of covered FSM output states */
  /*@out@*/ sig_link** sig_tail,  /*!< Pointer to the tail of the signal list of covered FSM output states */
  /*@out@*/ int**      expr_ids,  /*!< Pointer to array of expression IDs for each uncovered signal */
  /*@out@*/ int**      excludes   /*!< Pointer to array of exclude values for each uncovered signal */
) { PROFILE(FSM_COLLECT);

  fsm_link* curr_fsm;  /* Pointer to current FSM link being evaluated */
  int       size = 0;  /* Number of expressions IDs stored in expr_ids array */

  /* Initialize list pointers */
  *sig_tail = *sig_head = NULL;
  *expr_ids = *excludes = NULL;

  curr_fsm = funit->fsm_head;
  while( curr_fsm != NULL ) {

    /* Get the state and arc statistics */
    int state_hit    = 0;
    int state_total  = 0;
    int arc_hit      = 0;
    int arc_total    = 0;
    int arc_excluded = 0;
    arc_get_stats( curr_fsm->table->table, &state_hit, &state_total, &arc_hit, &arc_total, &arc_excluded );

    /* Allocate some more memory for the excluded array */
    *excludes = (int*)realloc_safe( *excludes, (sizeof( int ) * size), (sizeof( int ) * (size + 1)) );

    /* If the total number of arcs is not known, consider this FSM as uncovered */
    if( (cov == 0) && ((arc_total == -1) || (arc_total != arc_hit)) ) {
      (*excludes)[size] = 0;
      fsm_gather_signals( curr_fsm->table->to_state, sig_head, sig_tail, curr_fsm->table->to_state->id, expr_ids, &size );
    } else {
      if( (cov == 0) && arc_are_any_excluded( curr_fsm->table->table ) ) {
        fsm_gather_signals( curr_fsm->table->to_state, sig_head, sig_tail, curr_fsm->table->to_state->id, expr_ids, &size );
        (*excludes)[size] = 1;
      } else if( cov == 1 ) {
        fsm_gather_signals( curr_fsm->table->to_state, sig_head, sig_tail, -1, expr_ids, &size );
      }
    }

    curr_fsm = curr_fsm->next;

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

  fsm_link* curr_fsm;     /* Pointer to current FSM link */
  int*      tmp_ids;      /* Temporary integer array */
  int*      tmp;          /* Temporary integer array */
  char**    tmp_reasons;  /* Temporary reason array */

  curr_fsm = funit->fsm_head;
  while( (curr_fsm != NULL) && (curr_fsm->table->to_state->id != expr_id) ) {
    curr_fsm = curr_fsm->next; 
  }

  assert( curr_fsm != NULL );

  /* Get state information */
  arc_get_states( total_fr_states, total_fr_state_num, total_to_states, total_to_state_num, curr_fsm->table->table, TRUE, TRUE ); 
  arc_get_states( hit_fr_states,   hit_fr_state_num,   hit_to_states,   hit_to_state_num,   curr_fsm->table->table, TRUE, FALSE );

  /* Get state transition information */
  arc_get_transitions( total_from_arcs, total_to_arcs, total_ids, excludes, reasons,      total_arc_num, curr_fsm->table->table, funit, TRUE, TRUE );
  arc_get_transitions( hit_from_arcs,   hit_to_arcs,   &tmp_ids,  &tmp,     &tmp_reasons, hit_arc_num,   curr_fsm->table->table, funit, TRUE, FALSE );

  /* Get input state code */
  codegen_gen_expr( curr_fsm->table->from_state, NULL, input_state, input_size );

  /* Get output state code */
  codegen_gen_expr( curr_fsm->table->to_state, NULL, output_state, output_size );

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

  if( db_is_unnamed_scope( pname ) || root->name_diff ) {
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

      miss_found |= fsm_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->filename ) ),
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
  arc_get_states( &fr_states, &fr_state_size, &to_states, &to_state_size, table->table, (report_covered || trans_unknown), FALSE );

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
  arc_get_transitions( &from_states, &to_states, &ids, &excludes, &reasons, &arc_size, table->table, funit, ((rtype == RPT_TYPE_HIT) || trans_unknown), FALSE );

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

  fsm_link*    head;         /* Pointer to current FSM link */
  char**       icode;        /* Verilog output of input state variable expression */
  unsigned int icode_depth;  /* Number of valid entries in the icode array */
  char**       ocode;        /* Verilog output of output state variable expression */
  unsigned int ocode_depth;  /* Number of valid entries in the ocode array */
  unsigned int i;            /* Loop iterator */

  head = funit->fsm_head;
  while( head != NULL ) {

    bool found_exclusion;

    if( head->table->from_state->id == head->table->to_state->id ) {
      codegen_gen_expr( head->table->to_state, NULL, &ocode, &ocode_depth );
      fprintf( ofile, "      FSM input/output state (%s)\n\n", ocode[0] );
      for( i=0; i<ocode_depth; i++ ) {
        free_safe( ocode[i], (strlen( ocode[i] ) + 1) );
      }
      free_safe( ocode, (sizeof( char* ) * ocode_depth) );
    } else {
      codegen_gen_expr( head->table->from_state, NULL, &icode, &icode_depth );
      codegen_gen_expr( head->table->to_state,   NULL, &ocode, &ocode_depth );
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

    fsm_display_state_verbose( ofile, head->table );
    found_exclusion = fsm_display_arc_verbose( ofile, head->table, funit, (report_covered ? RPT_TYPE_HIT : RPT_TYPE_MISS) );
    if( report_exclusions && found_exclusion ) {
      (void)fsm_display_arc_verbose( ofile, head->table, funit, RPT_TYPE_EXCL );
    }

    if( head->next != NULL ) {
      fprintf( ofile, "      - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" );
    }

    head = head->next;

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

  if( db_is_unnamed_scope( pname ) || root->name_diff ) {
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
    switch( root->funit->type ) {
      case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_AFUNCTION    :
      case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_ATASK        :
      case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
      default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, obf_file( root->funit->filename ), tmpname );
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
      switch( head->funit->type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", pname, obf_file( head->funit->filename ) );
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
      missed_found |= fsm_instance_summary( ofile, instl->inst, (instl->inst->name_diff ? "<NA>" : "*"), &acc_st_hits, &acc_st_total, &acc_arc_hits, &acc_arc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)fsm_display_instance_summary( ofile, "Accumulated", acc_st_hits, acc_st_total, acc_arc_hits, acc_arc_total );
   
    if( verbose && (missed_found || report_covered || report_exclusions) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        fsm_instance_verbose( ofile, instl->inst, (instl->inst->name_diff ? "<NA>" : "*") );
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

/*
 $Log$
 Revision 1.110  2008/12/24 21:19:01  phase1geo
 Initial work at getting FSM coverage put in (this looks to be working correctly
 to this point).  Updated regressions per fixes.  Checkpointing.

 Revision 1.109  2008/11/12 00:07:41  phase1geo
 More updates for complex merging algorithm.  Updating regressions per
 these changes.  Checkpointing.

 Revision 1.108  2008/11/08 00:09:04  phase1geo
 Checkpointing work on asymmetric merging algorithm.  Updated regressions
 per these changes.  We currently have 5 failures in the IV regression suite.

 Revision 1.107  2008/10/31 22:01:34  phase1geo
 Initial code changes to support merging two non-overlapping CDD files into
 one.  This functionality seems to be working but needs regression testing to
 verify that nothing is broken as a result.

 Revision 1.106  2008/09/08 22:15:17  phase1geo
 Regression updates and modifications for new FSM GUI output (this isn't complete
 at this time).  Checkpointing.

 Revision 1.105  2008/09/06 05:59:45  phase1geo
 Adding assertion exclusion reason support and have most code implemented for
 FSM exclusion reason support (still working on debugging this code).  I believe
 that assertions, FSMs and lines might suffer from the same problem...
 Checkpointing.

 Revision 1.104  2008/09/04 21:34:20  phase1geo
 Completed work to get exclude reason support to work with toggle coverage.
 Ground-work is laid for the rest of the coverage metrics.  Checkpointing.

 Revision 1.103  2008/09/04 04:15:08  phase1geo
 Adding -p option to exclude command.  Updating other files per this change.
 Checkpointing.

 Revision 1.102  2008/09/03 05:33:06  phase1geo
 Adding in FSM exclusion support to exclude and report -e commands.  Updating
 regressions per recent changes.  Checkpointing.

 Revision 1.101  2008/08/29 05:38:37  phase1geo
 Adding initial pass of FSM exclusion ID output.  Need to fix issues with the -e
 option usage for all metrics, I believe (certainly for FSM).  Checkpointing.

 Revision 1.100  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Revision 1.97.2.5  2008/08/07 20:51:04  phase1geo
 Fixing memory allocation/deallocation issues with GUI.  Also fixing some issues with FSM
 table output and exclusion.  Checkpointing.

 Revision 1.97.2.4  2008/08/07 06:39:10  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.97.2.3  2008/08/06 20:11:33  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.97.2.2  2008/07/29 04:40:25  phase1geo
 Updating regressions.  Full regressions should now pass.

 Revision 1.97.2.1  2008/07/10 22:43:50  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.98  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.97  2008/05/30 23:00:48  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.96  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.95.2.3  2008/05/27 04:29:31  phase1geo
 Fixing memory leak for an FSM arc parser error.  Adding diagnostics to regression
 suite for coverage purposes.

 Revision 1.95.2.2  2008/05/08 23:12:42  phase1geo
 Fixing several bugs and reworking code in arc to get FSM diagnostics
 to pass.  Checkpointing.

 Revision 1.95.2.1  2008/05/02 22:06:12  phase1geo
 Updating arc code for new data structure.  This code is completely untested
 but does compile and has been completely rewritten.  Checkpointing.

 Revision 1.95  2008/04/15 20:37:09  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.94  2008/04/15 06:08:46  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.93  2008/03/28 21:11:32  phase1geo
 Fixing memory leak issues with -ep option and embedded FSM attributes.

 Revision 1.92  2008/03/18 05:36:04  phase1geo
 More updates (regression still broken).

 Revision 1.91  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.90  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.89  2008/03/14 22:00:18  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.88  2008/03/11 22:21:01  phase1geo
 Continuing to do next round of exception handling.  Checkpointing.

 Revision 1.87  2008/03/11 22:06:47  phase1geo
 Finishing first round of exception handling code.

 Revision 1.86  2008/03/09 20:45:48  phase1geo
 More exception handling updates.

 Revision 1.85  2008/03/04 22:46:07  phase1geo
 Working on adding check_exceptions.pl script to help me make sure that all
 exceptions being thrown are being caught and handled appropriately.  Other
 code adjustments are made in regards to exception handling.

 Revision 1.84  2008/03/04 06:46:48  phase1geo
 More exception handling updates.  Still work to go.  Checkpointing.

 Revision 1.83  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.82  2008/02/01 06:37:08  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.81  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.80  2008/01/16 06:40:34  phase1geo
 More splint updates.

 Revision 1.79  2008/01/16 05:01:22  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.78  2008/01/15 23:01:14  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.77  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.76  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.75  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.74  2007/12/19 14:37:29  phase1geo
 More compiler fixes (still more to go).  Checkpointing.

 Revision 1.73  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.72  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.71  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.70  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.69  2007/07/16 22:24:38  phase1geo
 Fixed bugs in accumulated coverage output and updated regression files for this
 change.  VCS simulated results are not contained here, however.

 Revision 1.68  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.67  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.66  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.65  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.64  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.63  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.62  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.61  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.60  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.59  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.58  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.57  2006/06/29 16:48:14  phase1geo
 FSM exclusion code now complete.

 Revision 1.56  2006/06/29 04:26:02  phase1geo
 More updates for FSM coverage.  We are getting close but are just not to fully
 working order yet.

 Revision 1.55  2006/06/28 22:15:19  phase1geo
 Adding more code to support FSM coverage.  Still a ways to go before this
 is completed.

 Revision 1.54  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.53  2006/04/12 18:06:24  phase1geo
 Updating regressions for changes that were made to support multi-file merging.
 Also fixing output of FSM state transitions to be what they were.
 Regressions now pass; however, the support for multi-file merging (beyond two
 files) has not been tested to this point.

 Revision 1.52  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.51  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.50  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.49  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.48  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.47  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.46  2005/12/22 23:04:42  phase1geo
 More memory leak fixes.

 Revision 1.45  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.44  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.43  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.42  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.41  2004/04/19 04:54:56  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.40  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.39  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.38  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.37  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.36  2004/01/30 23:23:27  phase1geo
 More report output improvements.  Still not ready with regressions.

 Revision 1.35  2004/01/30 06:04:45  phase1geo
 More report output format tweaks.  Adjusted lines and spaces to make things
 look more organized.  Still some more to go.  Regression will fail at this
 point.

 Revision 1.34  2003/12/12 17:16:25  phase1geo
 Changing code generator to output logic based on user supplied format.  Full
 regression fails at this point due to mismatching report files.

 Revision 1.33  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.32  2003/11/16 04:03:39  phase1geo
 Updating development documentation for FSMs.

 Revision 1.31  2003/11/10 04:25:50  phase1geo
 Adding more FSM diagnostics to regression suite.  All major testing for
 current FSM code should be complete at this time.  A few bug fixes to files
 that were found during this regression testing.

 Revision 1.30  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.29  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.28  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.27  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.26  2003/10/16 12:27:19  phase1geo
 Fixing bug in arc.c related to non-zero LSBs.

 Revision 1.25  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.24  2003/10/14 04:02:44  phase1geo
 Final fixes for new FSM support.  Full regression now passes.  Need to
 add new diagnostics to verify new functionality, but at least all existing
 cases are supported again.

 Revision 1.23  2003/10/13 22:10:07  phase1geo
 More changes for FSM support.  Still not quite there.

 Revision 1.22  2003/10/13 12:27:25  phase1geo
 More fixes to FSM stuff.

 Revision 1.21  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.20  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.19  2003/10/03 03:08:44  phase1geo
 Modifying filename in summary output to only specify basename of file instead
 of entire path.  The verbose report contains the full pathname still, however.

 Revision 1.18  2003/09/23 12:28:08  phase1geo
 Fixes for development documentation.

 Revision 1.17  2003/09/22 19:42:31  phase1geo
 Adding print_output WARNING_WRAP and FATAL_WRAP lines to allow multi-line
 error output to be properly formatted to the output stream.

 Revision 1.16  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.15  2003/09/19 18:04:28  phase1geo
 Adding fsm3 diagnostic to check proper handling of wide state variables.
 Code fixes to support new diagnostic.

 Revision 1.14  2003/09/19 13:25:28  phase1geo
 Adding new FSM diagnostics including diagnostics to verify FSM merging function.
 FSM merging code was modified to work correctly.  Full regression passes.

 Revision 1.13  2003/09/19 02:34:51  phase1geo
 Added new fsm1.3 diagnostic to regress suite which found a bug in arc.c that is
 now fixed.  It had to do with resizing an arc array and copying its values.
 Additionally, some output was fixed in the FSM reports.

 Revision 1.12  2003/09/15 02:37:03  phase1geo
 Adding development documentation for functions that needed them.

 Revision 1.11  2003/09/14 01:09:20  phase1geo
 Added verbose output for FSMs.

 Revision 1.10  2003/09/13 19:53:59  phase1geo
 Adding correct way of calculating state and state transition totals.  Modifying
 FSM summary reporting to reflect these changes.  Also added function documentation
 that was missing from last submission.

 Revision 1.9  2003/09/13 02:59:34  phase1geo
 Fixing bugs in arc.c created by extending entry supplemental field to 5 bits
 from 3 bits.  Additional two bits added for calculating unique states.

 Revision 1.8  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.7  2003/08/26 21:53:23  phase1geo
 Added database read/write functions and fixed problems with other arc functions.

 Revision 1.6  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.5  2002/11/23 16:10:46  phase1geo
 Updating changelog and development documentation to include FSM description
 (this is a brainstorm on how to handle FSMs when we get to this point).  Fixed
 bug with code underlining function in handling parameter in reports.  Fixing bugs
 with MBIT/SBIT handling (this is not verified to be completely correct yet).

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

