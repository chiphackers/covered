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
#include "expr.h"
#include "func_unit.h"
#include "fsm.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "util.h"
#include "vector.h"


extern inst_link*   inst_head;
extern funit_link*  funit_head;
extern bool         report_covered; 
extern unsigned int report_comb_depth;
extern bool         report_instance;
extern char**       leading_hierarchies;
extern int          leading_hier_num;
extern bool         leading_hiers_differ;
extern char         user_msg[USER_MSG_LENGTH];
extern isuppl       info_suppl;


/*!
 \param from_state  Pointer to expression that is input state variable for this FSM.
 \param to_state    Pointer to expression that is output state variable for this FSM.

 \return Returns a pointer to the newly allocated FSM structure.

 Allocates and initializes an FSM structure.
*/
fsm* fsm_create( expression* from_state, expression* to_state ) {

  fsm* table;  /* Pointer to newly created FSM */

  table             = (fsm*)malloc_safe( sizeof( fsm ), __FILE__, __LINE__ );
  table->name       = NULL;
  table->from_state = from_state;
  table->to_state   = to_state;
  table->arc_head   = NULL;
  table->arc_tail   = NULL;
  table->table      = NULL;

  return( table );

}

/*!
 \param table       Pointer to FSM structure to add new arc to.
 \param from_state  Pointer to from_state expression to add.
 \param to_state    Pointer to to_state expression to add.

 Adds new FSM arc structure to specified FSMs arc list.
*/
void fsm_add_arc( fsm* table, expression* from_state, expression* to_state ) {

  fsm_arc* arc;  /* Pointer to newly created FSM arc structure */

  /* Create an initialize specified arc */
  arc             = (fsm_arc*)malloc_safe( sizeof( fsm_arc ), __FILE__, __LINE__ );
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

}

/*!
 \param table  Pointer to FSM structure to set table sizes to.

 After the FSM signals are sized, this function is called to size
 an FSM structure (allocate memory for its tables) and the associated
 FSM arc list is parsed, setting the appropriate bit in the valid table.
*/
void fsm_create_tables( fsm* table ) {

  fsm_arc* curr_arc;    /* Pointer to current FSM arc structure */
  bool     set = TRUE;  /* Specifies if specified bit was set */

  /* Create the FSM arc transition table */
  assert( table != NULL );
  assert( table->to_state != NULL );
  assert( table->to_state->value != NULL );
  assert( table->table == NULL );
  table->table = arc_create( table->to_state->value->width );

  /* Set valid table */
  curr_arc = table->arc_head;
  while( (curr_arc != NULL) && set ) {

    /* Evaluate from and to state expressions */
    expression_operate( curr_arc->from_state, NULL );
    expression_operate( curr_arc->to_state, NULL );

    /* Set table entry in table, if possible */
    arc_add( &(table->table), curr_arc->from_state->value, curr_arc->to_state->value, 0 );

    curr_arc = curr_arc->next;

  } 

}

/*!
 \param table       Pointer to FSM structure to output.
 \param file        Pointer to file output stream to write to.
 \param parse_mode  Set to TRUE when we are writing immediately after parsing

 \return Returns TRUE if file writing is successful; otherwise, returns FALSE.

 Outputs the contents of the specified FSM to the specified CDD file.
*/
bool fsm_db_write( fsm* table, FILE* file, bool parse_mode ) {

  bool retval = TRUE;  /* Return value for this function */

  fprintf( file, "%d %d %d ",
    DB_TYPE_FSM,
    expression_get_id( table->from_state, parse_mode ),
    expression_get_id( table->to_state, parse_mode )
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

  return( retval );

} 

/*!
 \param line   Pointer to current line being read from the CDD file.
 \param funit  Pointer to current functional unit.

 \return Returns TRUE if read was successful; otherwise, returns FALSE.

 Reads in contents of FSM line from CDD file and stores newly created
 FSM into the specified functional unit.
*/
bool fsm_db_read( char** line, func_unit* funit ) {

  bool       retval = TRUE;  /* Return value for this function */
  expression iexp;           /* Temporary signal used for finding state variable */
  expression oexp;           /* Temporary signal used for finding state variable */
  exp_link*  iexpl;          /* Pointer to found state variable */
  exp_link*  oexpl;          /* Pointer to found state variable */
  int        chars_read;     /* Number of characters read from sscanf */
  fsm*       table;          /* Pointer to newly created FSM structure from CDD */
  int        is_table;       /* Holds value of is_table entry of FSM output */
 
  if( sscanf( *line, "%d %d %d%n", &(iexp.id), &(oexp.id), &is_table, &chars_read ) == 3 ) {

    *line = *line + chars_read + 1;

    if( funit == NULL ) {

      print_output( "Internal error:  FSM in database written before its functional unit", FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    } else {

      /* Find specified signal */
      if( ((iexpl = exp_link_find( &iexp, funit->exp_head )) != NULL) &&
          ((oexpl = exp_link_find( &oexp, funit->exp_head )) != NULL) ) {

        /* Create new FSM */
        table = fsm_create( iexpl->exp, oexpl->exp );

        /*
         If the input state variable is the same as the output state variable, create the new expression now.
        */
        if( iexp.id == oexp.id ) {
          table->from_state = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, iexp.id, 0, 0, 0, FALSE );
          vector_dealloc( table->from_state->value );
          bind_append_fsm_expr( table->from_state, iexpl->exp, funit );
        } else {
          table->from_state = iexpl->exp;
        }

        /* Set output expression tables to point to this FSM */
        table->to_state->table = table;

        /* Now read in set table */
        if( is_table == 1 ) {

          if( arc_db_read( &(table->table), line ) ) {

            /* Add fsm to current functional unit */
            fsm_link_add( table, &(funit->fsm_head), &(funit->fsm_tail) );
 
          } else {

            print_output( "Unable to read FSM state transition arc array", FATAL, __FILE__, __LINE__ );
            retval = FALSE;

          }

        } else {

          /* Add fsm to current functional unit */
          fsm_link_add( table, &(funit->fsm_head), &(funit->fsm_tail) );

        }
 
      } else {

        snprintf( user_msg, USER_MSG_LENGTH, "Unable to find state variable expressions (%d, %d) for current FSM", iexp.id, oexp.id );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        retval = FALSE;

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Pointer to FSM structure to merge data into.
 \param line  Pointer to read in line from CDD file to merge.
 \param same  Specifies if FSM to merge needs to be exactly the same as the existing FSM.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Parses specified line for FSM information and performs merge of the base
 and in FSMs, placing the resulting merged FSM into the base signal.  If
 the FSMs are found to be unalike (names are different), an error message
 is displayed to the user.  If both FSMs are the same, perform the merge
 on the FSM's tables.
*/
bool fsm_db_merge( fsm* base, char** line, bool same ) {

  bool   retval = TRUE;  /* Return value of this function */
  int    iid;            /* Input state variable expression ID */
  int    oid;            /* Output state variable expression ID */
  int    chars_read;     /* Number of characters read from line */
  int    is_table;       /* Holds value of is_table signifier */

  assert( base != NULL );
  assert( base->from_state != NULL );
  assert( base->to_state != NULL );

  if( sscanf( *line, "%d %d %d%n", &iid, &oid, &is_table, &chars_read ) == 3 ) {

    *line = *line + chars_read + 1;

#ifdef TBD
    if( (base->from_state->id != iid) || (base->to_state->id != oid) ) {

      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else if( is_table == 1 ) {
#endif
    if( is_table == 1 ) {

      arc_db_merge( &(base->table), line, same );
          
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param table  Pointer to FSM structure to set a state in.

 Taking the from and to state signal values, a new table entry is added
 to the specified FSM structure arc array (if an entry does not already
 exist in the array).
*/
void fsm_table_set( fsm* table ) {

  arc_add( &(table->table), table->from_state->value, table->to_state->value, 1 );

}

/*!
 \param table        Pointer to FSM to get statistics from.
 \param state_total  Total number of states within this FSM.
 \param state_hit    Number of states reached in this FSM.
 \param arc_total    Total number of arcs within this FSM.
 \param arc_hit      Number of arcs reached in this FSM.

 Recursive
*/
void fsm_get_stats( fsm_link* table, float* state_total, int* state_hit, float* arc_total, int* arc_hit ) {

  fsm_link* curr;   /* Pointer to current FSM in table list */

  curr = table;
  while( curr != NULL ) {
    arc_get_stats( curr->table->table, state_total, state_hit, arc_total, arc_hit );
    curr = curr->next;
  }

}

/*!
 \param funit_name  Name of functional unit to retrieve summary information for
 \param funit_type  Type of functional unit to retrieve summary information for
 \param total       Pointer to location to store the total number of state transitions for the specified functional unit
 \param hit         Pointer to location to store the number of hit state transitions for the specified functional unit

 \return Returns TRUE if the specified module was found; otherwise, returns FALSE.

 Retrieves the FSM summary information for the specified functional unit.
*/
bool fsm_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit ) {

  bool        retval = TRUE;  /* Return value of this function */
  func_unit   funit;          /* Functional unit used for searching */
  funit_link* funitl;         /* Pointer to found functional unit link */
  char        tmp[21];        /* Temporary string for total */

  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    snprintf( tmp, 21, "%20.0f", funitl->funit->stat->arc_total );
    assert( sscanf( tmp, "%d", total ) == 1 );
    *hit = funitl->funit->stat->arc_hit;

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param expr          Pointer to expression to get signals from
 \param head          Pointer to head of signal list to populate
 \param tail          Pointer to tail of signal list to populate
 \param expr_id       Expression ID of the statement containing this expression
 \param expr_ids      Pointer to expression ID array
 \param expr_id_size  Number of elements currently stored in expr_ids array

 Recursively iterates through specified expression, adding the signal of each expression that
 points to one to the specified signal list.  Also captures the expression ID of the statement
 containing this signal for each signal found (if expr_id is a non-negative value).
*/
void fsm_gather_signals( expression* expr, sig_link** head, sig_link** tail, int expr_id, int** expr_ids, int* expr_id_size ) {

  if( expr != NULL ) {

    if( expr->sig != NULL ) {

      /* Add this signal to the list */
      sig_link_add( expr->sig, head, tail );

      /* Add specified expression ID to the expression IDs array, if needed */
      if( expr_id >= 0 ) {
        (*expr_ids)                  = (int*)realloc( *expr_ids, (sizeof( int ) * ((*expr_id_size) + 1)) );
        (*expr_ids)[(*expr_id_size)] = expr_id;
        (*expr_id_size)++;
      }

    } else {

      fsm_gather_signals( expr->left,  head, tail, expr_id, expr_ids, expr_id_size );
      fsm_gather_signals( expr->right, head, tail, expr_id, expr_ids, expr_id_size );

    }

  }

}

/*!
 \param funit_name  Name of functional unit to collect combinational logic coverage information for
 \param funit_type  Type of functional unit to collect combinational logic coverage information for
 \param cov_head    Pointer to the head of the signal list of covered FSM output states
 \param cov_tail    Pointer to the tail of the signal list of covered FSM output states
 \param uncov_head  Pointer to the head of the signal list of uncovered FSM output states
 \param uncov_tail  Pointer to the tail of the signal list of uncovered FSM output states
 \param expr_ids    Pointer to array of expression IDs for each uncovered signal
 \param excludes    Pointer to array of exclude values for each uncovered signal

 \return Returns TRUE if FSM coverage information was found for the given functional unit; otherwise,
         returns FALSE to indicate that an error occurred.

 Gathers the covered and uncovered FSM information, storing their expressions in the cov and uncov signal lists.
 Used by the GUI for verbose FSM output.
*/
bool fsm_collect( char* funit_name, int funit_type, sig_link** cov_head, sig_link** cov_tail,
                  sig_link** uncov_head, sig_link** uncov_tail, int** expr_ids, int** excludes ) {

  bool        retval = TRUE;   /* Return value for this function */
  func_unit   funit;           /* Functional unit used for searching */
  funit_link* funitl;          /* Pointer to found functional unit link */
  fsm_link*   curr_fsm;        /* Pointer to current FSM link being evaluated */
  float       state_total;     /* Total number of states in current FSM */
  int         state_hit;       /* Number of states in current FSM hit */
  float       arc_total;       /* Total number of arcs in current FSM */
  int         arc_hit;         /* Number of arcs in current FSM hit */
  int         uncov_size = 0;  /* Number of expressions IDs stored in expr_ids array */

  /* First, find functional unit in functional unit array */
  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    /* Initialize list pointers */
    *cov_tail   = *cov_head   = NULL;
    *uncov_tail = *uncov_head = NULL;
    *expr_ids   = *excludes   = NULL;

    curr_fsm = funitl->funit->fsm_head;
    while( curr_fsm != NULL ) {

      /* Get the state and arc statistics */
      state_total = 0;
      state_hit   = 0;
      arc_total   = 0;
      arc_hit     = 0;
      arc_get_stats( curr_fsm->table->table, &state_total, &state_hit, &arc_total, &arc_hit );

      /* Allocate some more memory for the excluded array */
      *excludes = (int*)realloc( *excludes, (sizeof( int ) * (uncov_size + 1)) );

      /* If the total number of arcs is not known, consider this FSM as uncovered */
      if( (arc_total == -1) || (arc_total != arc_hit) ) {
        (*excludes)[uncov_size] = 0;
        fsm_gather_signals( curr_fsm->table->to_state, uncov_head, uncov_tail, curr_fsm->table->to_state->id, expr_ids, &uncov_size );
      } else {
        if( arc_are_any_excluded( curr_fsm->table->table ) ) {
          fsm_gather_signals( curr_fsm->table->to_state, uncov_head, uncov_tail, curr_fsm->table->to_state->id, expr_ids, &uncov_size );
          (*excludes)[uncov_size] = 1;
        } else {
          fsm_gather_signals( curr_fsm->table->to_state, cov_head, cov_tail, -1, expr_ids, &uncov_size );
        }
      }

      curr_fsm = curr_fsm->next;

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param funit_name       Name of functional unit containing FSM
 \param funit_type       Type of functional unit containing FSM
 \param expr_id          Expression ID of output state expression to find
 \param width            Pointer to width of FSM output state variable
 \param total_states     Pointer to a string array containing all possible states in this FSM
 \param total_state_num  Pointer to the number of elements in the total_states array
 \param hit_states       Pointer to a string array containing the hit states in this FSM
 \param hit_state_num    Pointer to the number of elements in the hit_states array
 \param total_from_arcs  Pointer to a string array containing all possible state transition from states
 \param total_to_arcs    Pointer to a string array containing all possible state transition to states
 \param excludes         Pointer to an integer array containing the exclude values for each state transition
 \param total_arc_num    Pointer to the number of elements in both the total_from_arcs, total_to_arcs and excludes arrays
 \param total_from_arcs  Pointer to a string array containing the hit state transition from states
 \param total_to_arcs    Pointer to a string array containing the hit state transition to states
 \param total_arc_num    Pointer to the number of elements in both the hit_from_arcs and hit_to_arcs arrays
 \param input_state      Pointer to a string array containing the code for the input state expression
 \param input_size       Pointer to the number of elements stored in the input state array
 \param output_state     Pointer to a string array containing the code for the output state expression
 \param output_size      Pointer to the number of elements stored in the output state array

 \return Returns TRUE if the specified functional unit was found; otherwise, returns FALSE.

 Gets the FSM coverage information for the specified FSM in the specified functional unit.  Used by the GUI
 for creating the contents of the verbose FSM viewer.
*/
bool fsm_get_coverage( char* funit_name, int funit_type, int expr_id, int* width,
                       char*** total_states, int* total_state_num,
                       char*** hit_states, int* hit_state_num,
                       char*** total_from_arcs, char*** total_to_arcs, int** excludes, int* total_arc_num,
                       char*** hit_from_arcs, char*** hit_to_arcs, int* hit_arc_num,
                       char*** input_state, int* input_size, char*** output_state, int* output_size ) {

  bool        retval = FALSE;  /* Return value for this function */
  func_unit   funit;           /* Functional unit structure used for searching */
  funit_link* funitl;          /* Pointer to found functional unit link */
  fsm_link*   curr_fsm;        /* Pointer to current FSM link */
  int*        tmp;             /* Temporary integer array */

  /* First, find functional unit in functional unit array */
  funit.name = funit_name;
  funit.type = funit_type;

  if( (funitl = funit_link_find( &funit, funit_head )) != NULL ) {

    curr_fsm = funitl->funit->fsm_head;
    while( (curr_fsm != NULL) && (curr_fsm->table->to_state->id != expr_id) ) {
      curr_fsm = curr_fsm->next; 
    }

    /* If we found a matching FSM, store values */
    if( curr_fsm != NULL ) {

      /* Get width */
      *width = curr_fsm->table->to_state->value->width;

      /* Get state information */
      arc_get_states( total_states, total_state_num, curr_fsm->table->table, TRUE, TRUE ); 
      arc_get_states( hit_states,   hit_state_num,   curr_fsm->table->table, TRUE, FALSE );

      /* Get state transition information */
      arc_get_transitions( total_from_arcs, total_to_arcs, excludes, total_arc_num, curr_fsm->table->table, TRUE, TRUE );
      arc_get_transitions( hit_from_arcs,   hit_to_arcs,   &tmp,     hit_arc_num,   curr_fsm->table->table, TRUE, FALSE );

      /* Get input state code */
      codegen_gen_expr( curr_fsm->table->from_state, curr_fsm->table->from_state->op, input_state, input_size, NULL );

      /* Get output state code */
      codegen_gen_expr( curr_fsm->table->to_state, curr_fsm->table->to_state->op, output_state, output_size, NULL );

      retval = TRUE;

    }

  }

  return( retval );

}

bool fsm_display_instance_summary( FILE* ofile, char* name, int state_hit, float state_total, int arc_hit, float arc_total ) {

  float state_percent;  /* Percentage of states hit */
  float arc_percent;    /* Percentage of arcs hit */
  float state_miss;     /* Number of states missed */
  float arc_miss;       /* Number of arcs missed */

  if( (state_total == -1) || (arc_total == -1) ) {
    fprintf( ofile, "  %-43.43s    %4d/  ? /  ?        ? %%         %4d/  ? /  ?        ? %%\n",
             name, state_hit, arc_hit );
    state_miss = arc_miss = 1;
  } else {
    calc_miss_percent( state_hit, state_total, &state_miss, &state_percent );
    calc_miss_percent( arc_hit, arc_total, &arc_miss, &arc_percent );
    fprintf( ofile, "  %-43.43s    %4d/%4.0f/%4.0f      %3.0f%%         %4d/%4.0f/%4.0f      %3.0f%%\n",
             name, state_hit, state_miss, state_total, state_percent, arc_hit, arc_miss, arc_total, arc_percent );
  }

  return( (state_miss > 0) || (arc_miss > 0) );

}

/*!
 \param ofile        Pointer to output file to display report contents to.
 \param root         Pointer to current root of instance tree to report.
 \param parent_inst  String containing Verilog hierarchy of this instance's parent.
 \param state_hits   Pointer to total number of states hit in design.
 \param state_total  Pointer to total number of states in design.
 \param arc_hits     Pointer to total number of arcs traversed.
 \param arc_total    Pointer to total number of arcs in design.

 \return Returns TRUE if any FSM states/arcs were found missing; otherwise, returns FALSE.

 Generates an instance summary report of the current FSM states and arcs hit during simulation.
*/
bool fsm_instance_summary( FILE* ofile, funit_inst* root, char* parent_inst, int* state_hits, float* state_total, int* arc_hits, float* arc_total ) {

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder for instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Set to TRUE if at least state or arc was not hit */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Generate printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname ); 
  }

  free_safe( pname );

  if( root->stat->show && !funit_is_unnamed( root->funit ) &&
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

  return( miss_found );

}

/*!
 \param ofile  Pointer to file stream to output summary information to
 \param name   Name of functional unit being reported
 \param fname  Filename containing the functional unit being reported
 \param state_hits  Number of FSM states that were hit in this functional unit during simulation
 \param state_total  Number of total FSM states that exist in the given functional unit
 \param arc_hits     Number of FSM arcs that were hit in this functional unit during simulation
 \param arc_total    Number of total FSM arcs that exist in the given functional unit

 \return Returns TRUE if at least one FSM state or FSM arc was missed during simulation for this functional unit; otherwise, returns FALSE.

 Outputs the summary FSM state/arc information for a given functional unit to the given output stream.
*/
bool fsm_display_funit_summary( FILE* ofile, const char* name, const char* fname, int state_hits, float state_total, int arc_hits, float arc_total ) {

  float state_percent;  /* Percentage of states hit */
  float arc_percent;    /* Percentage of arcs hit */
  float state_miss;     /* Number of states missed */
  float arc_miss;       /* Number of arcs missed */

  if( (state_total == -1) || (arc_total == -1) ) {
    fprintf( ofile, "  %-20.20s    %-20.20s   %4d/  ? /  ?        ? %%         %4d/  ? /  ?        ? %%\n",
             name, fname, state_hits, arc_hits );
    state_miss = arc_miss = 1;
  } else {
    calc_miss_percent( state_hits, state_total, &state_miss, &state_percent );
    calc_miss_percent( arc_hits, arc_total, &arc_miss, &arc_percent );
    fprintf( ofile, "  %-20.20s    %-20.20s   %4d/%4.0f/%4.0f      %3.0f%%         %4d/%4.0f/%4.0f      %3.0f%%\n",
             name, fname, state_hits, state_miss, state_total, state_percent, arc_hits, arc_miss, arc_total, arc_percent );
  }

  return( (state_miss > 0) || (arc_miss > 0) );

}

/*!
 \param ofile  Pointer to output file to display report contents to.
 \param head   Pointer to functional unit list to traverse.

 \return Returns TRUE if any FSM states/arcs were found missing; otherwise, returns FALSE.

 Generates a functional unit summary report of the current FSM states and arcs hit during simulation.
*/
bool fsm_funit_summary( FILE* ofile, funit_link* head, int* state_hits, float* state_total, int* arc_hits, float* arc_total ) {

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

      free_safe( pname );

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile  File handle of output file to send report output to.
 \param table  Pointer to FSM structure to output.

 Displays verbose information for hit/missed states to the specified
 output file.
*/
void fsm_display_state_verbose( FILE* ofile, fsm* table ) {

  bool   trans_known;  /* Set to TRUE if all legal arc transitions are known */
  char** states;       /* String array of all states */
  int    state_size;   /* Contains the number of elements in the states array */
  int    i;            /* Loop iterator */

  /* Figure out if transitions were known */
  trans_known = (arc_get_suppl( table->table, ARC_TRANS_KNOWN ) == 0) ? TRUE : FALSE;

  if( report_covered || trans_known ) {
    fprintf( ofile, "        Hit States\n\n" );
  } else {
    fprintf( ofile, "        Missed States\n\n" );
  }

  /* Create format string */
  fprintf( ofile, "          States\n" );
  fprintf( ofile, "          ======\n" );

  /* Get all of the states in string form */
  arc_get_states( &states, &state_size, table->table, (report_covered || trans_known), FALSE );

  /* Display all of the found states */
  for( i=0; i<state_size; i++ ) {
    fprintf( ofile, "          %d'h%s\n", arc_get_width( table->table ), states[i] );
    free_safe( states[i] );
  }

  fprintf( ofile, "\n" );

  /* Deallocate the states array */
  if( state_size > 0 ) {
    free_safe( states );
  }

}

/*!
 \param ofile  File handle of output file to send report output to.
 \param table  Pointer to FSM structure to output.

 Displays verbose information for hit/missed state transitions to
 the specified output file.
*/
void fsm_display_arc_verbose( FILE* ofile, fsm* table ) {

  bool   trans_known;   /* Set to TRUE if the number of state transitions is known */
  char   fstr[100];     /* Format string */
  char   tmp[20];       /* Temporary string */
  int    width;         /* Width (in characters) of the entire output value */
  int    val_width;     /* Number of bits in output state expression */
  int    len_width;     /* Number of characters needed to store the width of the output state expression */
  char** from_states;   /* String array containing from_state information */
  char** to_states;     /* String array containing to_state information */
  int*   excludes;      /* Temporary container (not used in this functio) */
  int    arc_size;      /* Number of elements in the from_states and to_states arrays */
  int    i;             /* Loop iterator */
  char   tmpfst[4096];  /* Temporary string holder for from_state value */
  char   tmptst[4096];  /* Temporary string holder for to_state value */

  /* Figure out if transactions were known */
  trans_known = (arc_get_suppl( table->table, ARC_TRANS_KNOWN ) == 0) ? TRUE : FALSE;

  if( report_covered || trans_known ) {
    fprintf( ofile, "        Hit State Transitions\n\n" );
  } else {
    fprintf( ofile, "        Missed State Transitions\n\n" );
  }

  val_width = table->to_state->value->width;

  /* Calculate width of length string */
  snprintf( tmp, 20, "%d", val_width );
  len_width = strlen( tmp );

  /* Create format string to hold largest output value */
  width = ((val_width % 4) == 0) ? (val_width / 4) : ((val_width / 4) + 1);
  width = width + len_width + 2;
  width = (width > 10) ? width : 10;
  snprintf( fstr, 100, "          %%-%d.%ds    %%-%d.%ds\n", width, width, width, width );

  fprintf( ofile, fstr, "From State", "To State" );
  fprintf( ofile, fstr, "==========", "==========" );

  /* Get the state transition information */
  arc_get_transitions( &from_states, &to_states, &excludes, &arc_size, table->table, (report_covered || trans_known), FALSE );

  /* Output the information to the specified output stream */
  snprintf( fstr, 100, "          %%-%d.%ds -> %%-%d.%ds\n", width, width, width, width );
  for( i=0; i<arc_size; i++ ) {
    snprintf( tmpfst, 4096, "%d'h%s", arc_get_width( table->table ), from_states[i] );
    snprintf( tmptst, 4096, "%d'h%s", arc_get_width( table->table ), to_states[i] );
    fprintf( ofile, fstr, tmpfst, tmptst );
    free_safe( from_states[i] );
    free_safe( to_states[i] );
  }

  fprintf( ofile, "\n" );

  /* Deallocate memory */
  if( arc_size > 0 ) {
    free_safe( from_states );
    free_safe( to_states );
  }

}

/*!
 \param ofile  File handle of output file to send report output to.
 \param head   Pointer to head of FSM link for a functional unit.

 Displays the verbose FSM state and state transition information to the specified
 output file.
*/
void fsm_display_verbose( FILE* ofile, fsm_link* head ) {

  char** icode;        /* Verilog output of input state variable expression */
  int    icode_depth;  /* Number of valid entries in the icode array */
  char** ocode;        /* Verilog output of output state variable expression */
  int    ocode_depth;  /* Number of valid entries in the ocode array */
  int    i;            /* Loop iterator */

  while( head != NULL ) {

    if( head->table->from_state->id == head->table->to_state->id ) {
      codegen_gen_expr( head->table->to_state, head->table->to_state->op, &ocode, &ocode_depth, NULL );
      fprintf( ofile, "      FSM input/output state (%s)\n\n", ocode[0] );
      for( i=0; i<ocode_depth; i++ ) {
        free_safe( ocode[i] );
      }
      free_safe( ocode );
    } else {
      codegen_gen_expr( head->table->from_state, head->table->from_state->op, &icode, &icode_depth, NULL );
      codegen_gen_expr( head->table->to_state,   head->table->to_state->op,   &ocode, &ocode_depth, NULL );
      fprintf( ofile, "      FSM input state (%s), output state (%s)\n\n", icode[0], ocode[0] );
      for( i=0; i<icode_depth; i++ ) {
        free_safe( icode[i] );
      }
      free_safe( icode );
      for( i=0; i<ocode_depth; i++ ) {
        free_safe( ocode[i] );
      }
      free_safe( ocode );
    }

    fsm_display_state_verbose( ofile, head->table );
    fsm_display_arc_verbose( ofile, head->table );

    if( head->next != NULL ) {
      fprintf( ofile, "      - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" );
    }

    head = head->next;

  }

}

/*!
 \param ofile  Pointer to output file to display report contents to.
 \param root   Pointer to root of instance tree to traverse.
 \param parent_inst  String containing name of this instance's parent instance.

 Generates an instance verbose report of the current FSM states and arcs hit during simulation.
*/
void fsm_instance_verbose( FILE* ofile, funit_inst* root, char* parent_inst ) {

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder for instance */
  char*       pname;          /* Printable version of instance name */

  assert( root != NULL );

  /* Get printable version of instance name */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
  }

  free_safe( pname );

  if( !funit_is_unnamed( root->funit ) &&
      ((((root->stat->state_hit < root->stat->state_total) || (root->stat->arc_hit < root->stat->arc_total)) && !report_covered) ||
         (root->stat->state_total == -1) ||
         (root->stat->arc_total   == -1) ||
       (((root->stat->state_hit > 0) || (root->stat->arc_hit > 0)) && report_covered)) ) {

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

    free_safe( pname );

    fsm_display_verbose( ofile, root->funit->fsm_head );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    fsm_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

}

/*! 
 \param ofile  Pointer to output file to display report contents to.
 \param head   Pointer to head of functional unit list to traverse.

 Generates a functional unit verbose report of the current FSM states and arcs hit during simulation.
*/
void fsm_funit_verbose( FILE* ofile, funit_link* head ) {

  char* pname;  /* Printable version of functional unit name */

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        ((((head->funit->stat->state_hit < head->funit->stat->state_total) || 
           (head->funit->stat->arc_hit < head->funit->stat->arc_total)) && !report_covered) ||
           (head->funit->stat->state_total == -1) ||
           (head->funit->stat->arc_total   == -1) ||
         (((head->funit->stat->state_hit > 0) || (head->funit->stat->arc_hit > 0)) && report_covered)) ) {

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

      free_safe( pname );

      fsm_display_verbose( ofile, head->funit->fsm_head );

    }

    head = head->next;

  }

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information
 
 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the FSM coverage for each functional unit encountered.  The parent functional unit will
 specify its own FSM coverage along with a total FSM coverage including its 
 children.
*/
void fsm_report( FILE* ofile, bool verbose ) {

  bool       missed_found  = FALSE;  /* If set to TRUE, FSM cases were found to be missed */
  char       tmp[4096];              /* Temporary string value */
  inst_link* instl;                  /* Pointer to current instance link */
  int        acc_st_hits   = 0;      /* Accumulated number of states hit */
  float      acc_st_total  = 0;      /* Accumulated number of states in design */
  int        acc_arc_hits  = 0;      /* Accumulated number of arcs hit */
  float      acc_arc_total = 0;      /* Accumulated number of arcs in design */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   FINITE STATE MACHINE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( leading_hiers_differ ) {
      strcpy( tmp, "<NA>" );
    } else {
      assert( leading_hier_num > 0 );
      strcpy( tmp, leading_hierarchies[0] );
    }

    fprintf( ofile, "                                                               State                             Arc\n" );
    fprintf( ofile, "Instance                                          Hit/Miss/Total    Percent hit    Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = inst_head;
    while( instl != NULL ) {
      missed_found |= fsm_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*"), &acc_st_hits, &acc_st_total, &acc_arc_hits, &acc_arc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    fsm_display_instance_summary( ofile, "Accumulated", acc_st_hits, acc_st_total, acc_arc_hits, acc_arc_total );
   
    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = inst_head;
      while( instl != NULL ) {
        fsm_instance_verbose( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                               State                             Arc\n" );
    fprintf( ofile, "Module/Task/Function      Filename                Hit/Miss/Total    Percent Hit    Hit/Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = fsm_funit_summary( ofile, funit_head, &acc_st_hits, &acc_st_total, &acc_arc_hits, &acc_arc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    fsm_display_funit_summary( ofile, "Accumulated", "", acc_st_hits, acc_st_total, acc_arc_hits, acc_arc_total );

    if( verbose && (missed_found || report_covered) ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      fsm_funit_verbose( ofile, funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}

/*!
 \param table  Pointer to FSM structure to deallocate.

 Deallocates all allocated memory for the specified FSM structure.
*/
void fsm_dealloc( fsm* table ) {

  fsm_arc* tmp;  /* Temporary pointer to current FSM arc structure to deallocate */

  if( table != NULL ) {

    /* Free name if one was specified */
    if( table->name != NULL ) {
      free_safe( table->name );
    }

    /* Deallocate tables */
    arc_dealloc( table->table );

    /* Deallocate FSM arc structure */
    while( table->arc_head != NULL ) {
      tmp = table->arc_head;
      table->arc_head = table->arc_head->next;
      free_safe( tmp );
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
    free_safe( table );
      
  }

}

/*
 $Log$
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

