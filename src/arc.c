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
 \file     arc.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/25/2003

 \par What is an arc?
 This group of functions handles the allocation, initialization, deallocation, and manipulation
 of an arc array.  An arc array is a specialized, variable sized, compact byte array that contains
 all of the information necessary for storing state and state transition information for a single
 FSM within a design.  To store this information into as compact a representation as possible, the
 data is bit-packed into the array.

 \par
 The first 7 bytes of the array  are known as the "arc header".  Every arc array is required to have
 this information.  The layout of the header is shown below:

 \par
 <table>
   <tr> <td><strong>Byte</strong></td> <td><strong>Description</strong></td> </tr>
   <tr> <td> 0 </td> <td> Bits [ 7:0] of output state variable width </td> </tr>
   <tr> <td> 1 </td> <td> Bits [15:8] of output state variable width </td> </tr>
   <tr> <td> 2 </td> <td> Bits [ 7:0] of number of state transition entries currently allocated </td> </tr>
   <tr> <td> 3 </td> <td> Bits [15:8] of number of state transition entries currently allocated </td> </tr>
   <tr> <td> 4 </td> <td> Bits [ 7:0] of number of state transition entries currently occupied </td> </tr>
   <tr> <td> 5 </td> <td> Bits [15:8] of number of state transition entries currently occupied </td> </tr>
   <tr> <td> 6 </td> <td> Bits [ 7:0] of arc array supplemental field </td> </tr>
 </table>

 \par
 Note:  The arc array supplemental field contains miscellaneous information about this arc array.
 Currently, only bit 0 is used to indicate if the user has specified state transitions manually (using
 the Verilog-2001 inline attribute mechanism) or whether no state transitions are known prior to simulation.

 \par
 The rest of the bytes in the arc array represent a list of state transition <strong>entries</strong>.
 An entry contains enough information to describe a bidirectional state transition with coverage
 information.  The bit-width of an entry is determined by taking bytes 0 & 1 of the header (the output
 state variable width, multiplying this value by 2, and adding the number of entry supplemental bits
 for an entry (currently this value is 6).  Each entry is byte-aligned in the arc array.  The fields that
 comprise an entry are described below.

 \par
 <table>
   <tr> <th>Bits</th> <th>Description</th> </tr>
   <tr> <td> 0 </td> <td> Set to 1 if the forward bidirectional state transition was hit; otherwise, set to 0. </td> </tr>
   <tr> <td> 1 </td> <td> Set to 1 if the reverse bidirectional state transition was hit; otherwise, set to 0. </td> </tr>
   <tr> <td> 2 </td> <td> Set to 1 if this entry is bidirectional (reverse is a transition); otherwise, only forward is valid. </td> </tr>
   <tr> <td> 3 </td> <td> Set to 1 if the output state of the forward transition is a new state in the arc array. </td> </tr>
   <tr> <td> 4 </td> <td> Set to 1 if the input state of the forward transition is a new state in the arc array. </td> </tr>
   <tr> <td> 5 </td> <td> Set to 1 if the forward state transition is excluded from coverage. </td> </tr>
   <tr> <td> 6 </td> <td> Set to 1 if the reverse state transition is excluded from coverage. </td> </tr>
   <tr> <td> (width + 6):7 </td> <td> Bit value of output state of the forward transition. </td> </tr>
   <tr> <td> ((width * 2) + 6):(width + 7) </td> <td> Bit value of input state of the forward transition. </td> </tr>
 </table>

 \par Adding State Transitions
 When an state transition occurs during simulation, the arc array is searched to see if an entry already exists that
 contains the same state transition.  If no matching entry is found, a new entry is added to the existing arc array.
 If the arc array allocated is currently full (current allocated size in header equals the current number of used
 entries), the array is grown.  The number of entries that the array grows is equal to the bit width of the output
 state variable.  When the new state is added, the entry is stored as a "forward" entry.  A forward entry is an entry
 that contains the input state value in the upper bits of the entry and the output state value in the lower bits of
 the entry.  If the state is added prior to simulation, bit 0 (hit forward) of the entry is set to 0; otherwise, it
 is set to a value of 1, indicating that the forward version of the state transition was hit.  If a state transition
 is found in the array but the order of the input and output values are reversed from the stored entry, the entry
 is stored into the same entry, setting bit 2 (bidirectional entry bit) to a value of 1.  This indicates that the
 current entry contains a forward entry and a reverse entry (input state value is stored in lower order bits and
 output state value is stored in upper order bits of the entry).  If this type of entry is stored prior to simulation,
 bit 1 (reverse hit bit) is set to 0; otherwise, the bit is set to a value of 1.  The following example shows an arc
 array as state transitions are added to it during simulation.

 \par
 <table>
   <caption align=bottom>HF: Forward direction hit, HR: Reverse direction hit, BD: Entry is bidirectional</caption>
   <tr> <th>Event</th> <th colspan=3>Entry</th> <th>Action</th> </tr>
   <tr> <td></td> <td>0</td> <td>1</td> <td>2</td> </tr>
   <tr> <td>Add 0-&gt;1</td> <td>0-&gt;1,HF=1,HR=0,BD=0</td> <td></td> <td></td> <td>0-&gt;1 not found in table, add as forward in entry 0</td> </tr>
   <tr> <td>Add 1-&gt;2</td> <td>0-&gt;1,HF=1,HR=0,BD=0</tr> <td>1-&gt;2,HF=1,HR=0,BD=0</td> <td></td> <td>1-&gt;2 not found in table, add as forward in entry 1</td> </tr>
   <tr> <td>Add 1-&gt;0</td> <td>0-&gt;1,HF=1,HR=1,BD=1</tr> <td>1-&gt;2,HF=1,HR=0,BD=0</td> <td></td> <td>1-&gt;0 found in entry 0, add as reverse and set bidirectional entry bit</td></tr>
 </table>

 \par Calculating FSM State Coverage Metrics
 To calculate the number of FSM states that are in an arc array, we first need to figure out what state values
 are unique in the arc array.  Since state transitions contain two state values (input and output), we need to
 compare each state whether it is an input or an output state to every other state value in the arc array.  To
 do this, we start with the input state (in the forward direction -- value in upper bits of entry) in entry 0
 and compare it to each and every state in the arc array.  If the current state value matches the first entry
 and the current state value is in the upper bits of the entry, bit 4 is set in the entry's supplemental field
 to indicate that its state is not unique.  If the current state value matches the first entry and the current
 state value is in the lower bits of the entry, bit 5 is set in the entry's supplemental field to indicate that
 its state is not unique.  If the current value does not match the first state value, nothing happens.  After
 all states have been compared, the right value (if bit 5 in its entry supplemental field is not set) is
 compared to all of the rest of the state values (if their bit 4 or bit 5 in their supplemental field is not
 already set) are compared.  This continues for all state values.  After the state comparing phase has occurred,
 we can simply walk through the arc array counting all of the states that do not have their corresponding bit 4
 or bit 5 set.

 \par Calculating FSM State Transition Coverage Metrics
 To calculate state transition coverage for the arc array is simple.  Just count the number of entries that have
 bit 0 set to 1 and bits 1 and 2 set to 1.  This will give you the number of state transitions that were hit
 during simulation.

 \par Outputting an Arc to a File
 Writing and reading an arc to and from a file is accomplished by writing each byte in the arc array to the file
 in hexidecimal format (zero-filling the output) with no spaces between the bytes.  Additionally, if a byte
 contains the hexidecimal value of 0x0, simply output a comma character (this saves one character in the file for
 each byte that contains a value of 0x0).  Keeping the output of an arc array to as small as possible in the file
 is necessary due to the large memory footprint that just one FSM can take.
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "arc.h"
#include "defines.h"
#include "exclude.h"
#include "expr.h"
#include "profiler.h"
#include "util.h"
#include "vector.h"


/*!
 Unique identifier for each arc in the design (used for exclusion purposes).  This value is
 assigned to an arc when it is read from the CDD file.
*/
int curr_arc_id = 1;


#ifndef RUNLIB
/*!
 Displays the given state transition arcs in a human-readable format.
*/
/*@unused@*/ static void arc_display(
  const fsm_table* table,     /*!< Pointer to state transition arc array to display */
  unsigned int     fr_width,  /*!< Width of "from" state */
  unsigned int     to_width   /*!< Width of "to" state */
) {

  unsigned int i;  /* Loop iterator */

  assert( table != NULL );

  for( i=0; i<table->num_arcs; i++ ) {

    char* lvec = vector_to_string( table->fr_states[table->arcs[i]->from], HEXIDECIMAL, TRUE, fr_width );
    char* rvec = vector_to_string( table->to_states[table->arcs[i]->to],   HEXIDECIMAL, TRUE, to_width );

    printf( "       entry %u: ", i );

    /* Output the state transition */
    printf( "%s", lvec );
    printf( " --> " );
    printf( "%s", rvec );

    /* Now output the relevant supplemental information */
    printf( "  (%s %s)\n",
            ((table->arcs[i]->suppl.part.excluded == 1) ? "E" : "  "),
            ((table->arcs[i]->suppl.part.hit      == 1) ? "H" : "  ") );

    
    /* Deallocate strings */
    free_safe( lvec, (strlen( lvec ) + 1) );
    free_safe( rvec, (strlen( rvec ) + 1) );

  }

}

/*!
 \return Returns the index of the found from_state in the fr_states array if one is found; otherwise,
         returns -1 to indicate that a match could not be found.

 Searches the list of FROM states for a match to the given vector value.
*/
int arc_find_from_state(
  const fsm_table* table,  /*!< Pointer to FSM table to search in */
  const vector*    st      /*!< State to search for */
) { PROFILE(ARC_FIND_FROM_STATE);

  int          index = -1;  /* Return value for this function */
  unsigned int i     = 0;   /* Loop iterator */

  assert( table != NULL );

  while( (i < table->num_fr_states) && !vector_ceq_ulong( st, table->fr_states[i] ) ) i++;
  if( i < table->num_fr_states ) {
    index = i;
  }

  PROFILE_END;

  return( index );

}

/*!
 \return Returns the index of the found to_state in the to_states array if one is found; otherwise,
         returns -1 to indicate that a match could not be found.
         that no match occurred.

 Searches the list of TO states for a match to the given vector value.
*/
int arc_find_to_state(
  const fsm_table* table,  /*!< Pointer to FSM table to search in */
  const vector*    st      /*!< State to search for */
) { PROFILE(ARC_FIND_TO_STATE);

  int          index = -1;  /* Return value for this function */
  unsigned int i     = 0;   /* Loop iterator */

  assert( table != NULL );

  while( (i < table->num_to_states) && !vector_ceq_ulong( st, table->to_states[i] ) ) i++;
  if( i < table->num_to_states ) {
    index = i;
  }

  PROFILE_END;

  return( index );

}

/*!
 \return Returns the index of the found arc in the arcs array if it is found; otherwise, returns -1.

 Searches for the arc in the arcs array of the given FSM table specified by the given state indices.
*/
int arc_find_arc(
  const fsm_table* table,     /*!< Pointer to FSM table to search in */
  unsigned int     fr_index,  /*!< Index of from state to find */
  unsigned int     to_index   /*!< Index of to state to find */
) { PROFILE(ARC_FIND_ARC);

  int          index = -1;
  unsigned int i     = 0;

  while( (i < table->num_arcs) && (index == -1) ) {
    if( (table->arcs[i]->from == fr_index) && (table->arcs[i]->to == to_index) ) {
      index = i;
    }
    i++;
  }

  PROFILE_END;

  return( index );

}

/*!
 \return Returns the index of the found arc in the arcs array if it is found; otherwise, returns -1. 
*/
int arc_find_arc_by_exclusion_id(
  const fsm_table* table,  /*!< Pointer to FSM table structure to search */
  int              id      /*!< Exclusion ID to search for */
) { PROFILE(ARC_FIND_ARC_BY_EXCLUSION_ID);

  int index = -1;

  if( (table->id <= id) && ((table->id + table->num_arcs) > id) ) {
    index = id - table->id;
  }

  PROFILE_END;

  return( index );

}
#endif /* RUNLIB */

/*!
 \return Returns a pointer to the newly created arc transition structure.

 Allocates memory for a new state transition arc array and initializes its
 contents.
*/
fsm_table* arc_create() { PROFILE(ARC_CREATE);

  fsm_table* table;  /* Pointer to newly created FSM table */

  /* Allocate memory for the new table here */
  table = (fsm_table*)malloc_safe( sizeof( fsm_table ) );

  /* Initialize */
  table->suppl.all     = 0;
  table->id            = 0;
  table->fr_states     = NULL;
  table->num_fr_states = 0;
  table->to_states     = NULL;
  table->num_to_states = 0;
  table->arcs          = NULL;
  table->num_arcs      = 0;

  PROFILE_END;

  return( table );

}

#ifndef RUNLIB
/*!
 If specified arcs array has not been created yet (value is set to NULL),
 allocate enough memory in the arc array to hold width number of state transitions.
 If the specified arcs array has been created but is currently full (arc array
 max_size and curr_size are equal to each other), add width number of more
 arc array entries to the current array.  After memory has been allocated, create
 a state transition entry from the fr_st and to_st expressions, setting the
 hit bits in the entry to 0.
*/
void arc_add(
  fsm_table*    table,   /*!< Pointer to FSM table to add state transition arc array to */
  const vector* fr_st,   /*!< Pointer to vector containing the from state */
  const vector* to_st,   /*!< Pointer to vector containing the to state */
  int           hit,     /*!< Specifies if arc entry should be marked as hit */
  bool          exclude  /*!< If new arc is created, sets the exclude bit to this value */
) { PROFILE(ARC_ADD);

  int from_index;  /* Index of found from_state in states array */
  int to_index;    /* Index of found to_state in states array */
  int arcs_index;  /* Index of found state transition in arcs array */

  assert( table != NULL );

  if( (hit == 0) || (!vector_is_unknown( fr_st ) && !vector_is_unknown( to_st )) ) {

    /* Search for the from_state vector in the states array */
    if( (from_index = arc_find_from_state( table, fr_st )) == -1 ) {
      table->fr_states = (vector**)realloc_safe( table->fr_states, (sizeof( vector* ) * table->num_fr_states), (sizeof( vector* ) * (table->num_fr_states + 1)) );
      from_index    = table->num_fr_states;
      table->fr_states[from_index] = vector_create( fr_st->width, VTYPE_VAL, fr_st->suppl.part.data_type, TRUE );
      vector_copy( fr_st, table->fr_states[from_index] );
      table->num_fr_states++;
    }

    /* Search for the to_state vector in the states array */
    if( (to_index = arc_find_to_state( table, to_st )) == -1 ) {
      table->to_states = (vector**)realloc_safe( table->to_states, (sizeof( vector* ) * table->num_to_states), (sizeof( vector* ) * (table->num_to_states + 1)) );
      to_index      = table->num_to_states;
      table->to_states[to_index] = vector_create( to_st->width, VTYPE_VAL, to_st->suppl.part.data_type, TRUE );
      vector_copy( to_st, table->to_states[to_index] );
      table->num_to_states++;
    }

    /* If we need to add a new arc, do so now */
    if( (arcs_index = arc_find_arc( table, from_index, to_index )) == -1 ) {

      table->arcs = (fsm_table_arc**)realloc_safe( table->arcs, (sizeof( fsm_table_arc* ) * table->num_arcs), (sizeof( fsm_table_arc* ) * (table->num_arcs + 1)) );
      table->arcs[table->num_arcs] = (fsm_table_arc*)malloc_safe( sizeof( fsm_table_arc ) );
      table->arcs[table->num_arcs]->suppl.all           = 0;
      table->arcs[table->num_arcs]->suppl.part.hit      = hit;
      table->arcs[table->num_arcs]->suppl.part.excluded = exclude;
      table->arcs[table->num_arcs]->from                = from_index;
      table->arcs[table->num_arcs]->to                  = to_index;
      arcs_index = table->num_arcs;
      table->num_arcs++;

    /* Otherwise, adjust hit and exclude information */
    } else {

      table->arcs[arcs_index]->suppl.part.hit      |= hit;
      table->arcs[arcs_index]->suppl.part.excluded |= exclude;

    }

    /* If we have set a side with hit equal to 0, we are specifying a known transition. */
    if( hit == 0 ) {
      table->suppl.part.known = 1;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns number of unique states hit during simulation.

 Traverses through specified state transition table, figuring out what
 states in the table are unique.  This is done by traversing through
 the entire arc array, comparing states that have their ARC_NOT_UNIQUE_x
 bits set to a value 0.  If both states contain the same value, the
 second state has its ARC_NOT_UNIQUE_x set to indicate that this state
 is not unique to the table.
*/
static int arc_state_hits(
  const fsm_table* table  /*!< Pointer to state transition arc array */
) { PROFILE(ARC_STATE_HITS);

  int          hit = 0;     /* Number of states hit */
  unsigned int i;           /* Loop iterators */
  int*         state_hits;  /* Contains state hit information */

  assert( table != NULL );

  /* First, create and intialize a state table to hold hit counts */
  state_hits = (int*)malloc_safe( sizeof( int ) * table->num_fr_states );
  for( i=0; i<table->num_fr_states; i++ ) {
    state_hits[i] = 0;
  }
  
  /* Iterate through arc transition array and count unique hits */
  for( i=0; i<table->num_arcs; i++ ) {
    if( (table->arcs[i]->suppl.part.hit || table->arcs[i]->suppl.part.excluded) ) {
      hit += (state_hits[table->arcs[i]->from]++ == 0) ? 1 : 0; 
    }
  }

  /* Deallocate state_hits */
  free_safe( state_hits, (sizeof( int ) * table->num_fr_states) );

  PROFILE_END;

  return( hit );

}

/*!
 \return Returns the number of hit state transitions in the specified
         arc array.

 Iterates through arc array, accumulating the number of state
 transitions that were hit in simulation.
*/
static int arc_transition_hits(
  const fsm_table* table  /*!< Pointer to state transition arc array */
) { PROFILE(ARC_TRANSITION_HITS);

  int          hit = 0;  /* Number of arcs hit */
  unsigned int i;        /* Loop iterator */

  assert( table != NULL );

  for( i=0; i<table->num_arcs; i++ ) {
    hit += table->arcs[i]->suppl.part.hit | table->arcs[i]->suppl.part.excluded;
  }

  PROFILE_END;

  return( hit );

}

/*!
 \return Returns the number of state transitions that are excluded from coverage.
*/
static int arc_transition_excluded(
  const fsm_table* table  /*!< Pointer to state transition arc array */
) { PROFILE(ARC_TRANSITION_EXCLUDED);

  int          excluded = 0;  /* Number of arcs excluded */
  unsigned int i;             /* Loop iterator */

  assert( table != NULL );

  for( i=0; i<table->num_arcs; i++ ) {
    excluded += table->arcs[i]->suppl.part.excluded;
  }

  PROFILE_END;

  return( excluded );

}

/*!
 Calculates values for all specified totals from given state transition arc array.
 If the state and state transition totals are not known (i.e., user specified state
 variables without specifying legal states and state transitions and/or the user
 specified state variables and state table was not able to be automatically extracted),
 return a value of -1 for total values to indicate to the calling function that a
 different report output is required.
*/
void arc_get_stats(
            const fsm_table* table,        /*!< Pointer to FSM table */
  /*@out@*/ int*             state_hits,   /*!< Pointer to total number of states hit during simulation */
  /*@out@*/ int*             state_total,  /*!< Pointer to total number of states in table */
  /*@out@*/ int*             arc_hits,     /*!< Pointer to total number of state transitions hit during simulation */
  /*@out@*/ int*             arc_total,    /*!< Pointer to total number of state transitions in table */
  /*@out@*/ int*             arc_excluded  /*!< Pointer to total number of excluded arcs */
) { PROFILE(ARC_GET_STATS);

  /* First get hits */
  *state_hits   += arc_state_hits( table );
  *arc_hits     += arc_transition_hits( table );
  *arc_excluded += arc_transition_excluded( table );
  
  /* If the state transitions are known, calculate them; otherwise, return -1 for totals */
  if( table->suppl.part.known == 0 ) {
    *state_total = -1;
    *arc_total   = -1;
  } else {
    *state_total += table->num_fr_states;
    *arc_total   += table->num_arcs;
  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Writes the specified arcs array to the specified CDD output file.  An arc array
 is output in a special format that is described in the above documentation for
 this file.
*/
void arc_db_write(
  const fsm_table* table,  /*!< Pointer to state transition arc array to write */
  FILE*            file    /*!< Pointer to CDD file to write */
) { PROFILE(ARC_DB_WRITE);

  unsigned int  i;   /* Loop iterator */

  assert( table != NULL );

  /*@-formatcode@*/
  fprintf( file, " %hhx %u %u ", table->suppl.all, table->num_fr_states, table->num_to_states );
  /*@=formatcode@*/

  /* Output state information */
  for( i=0; i<table->num_fr_states; i++ ) {
    vector_db_write( table->fr_states[i], file, TRUE, FALSE );
    fprintf( file, "  " );
  }
  for( i=0; i<table->num_to_states; i++ ) {
    vector_db_write( table->to_states[i], file, TRUE, FALSE );
    fprintf( file, "  " );
  }

  /* Output arc information */
  fprintf( file, " %u", table->num_arcs );
  for( i=0; i<table->num_arcs; i++ ) {
    /*@-formatcode@*/
    fprintf( file, "  %u %u %hhx", table->arcs[i]->from, table->arcs[i]->to, table->arcs[i]->suppl.all );
    /*@=formatcode@*/
  }

  PROFILE_END;

}

/*!
 \throws anonymous Throw

 Reads in specified state transition arc table, allocating the appropriate
 space to hold the table.  Returns TRUE if the specified line contained an
 appropriately written arc transition table; otherwise, returns FALSE.
*/
void arc_db_read(
  /*@out@*/ fsm_table** table,  /*!< Pointer to state transition arc array */
            char**      line    /*!< String containing current CDD line of arc information */
) { PROFILE(ARC_DB_READ);

  /* Allocate table */
  *table = arc_create();

  Try {

    unsigned int num_fr_states;
    unsigned int num_to_states;
    int          chars_read;

    /*@-formatcode@*/
    if( sscanf( *line, "%hhx %u %u%n", &((*table)->suppl.all), &num_fr_states, &num_to_states, &chars_read ) == 3 ) {
    /*@=formatcode@*/

      unsigned int i;
      unsigned int num_arcs;

      *line += chars_read;

      /* Set exclusion ID */
      (*table)->id = curr_arc_id;

      /* Allocate and initialize fr_states array */
      (*table)->fr_states     = (vector**)malloc_safe( sizeof( vector* ) * num_fr_states );
      (*table)->num_fr_states = num_fr_states;
      for( i=0; i<num_fr_states; i++ ) {
        (*table)->fr_states[i] = NULL;
      }

      /* Read in from vectors */
      for( i=0; i<num_fr_states; i++ ) {
        vector_db_read( &((*table)->fr_states[i]), line );
      }

      /* Allocate and initialize to_states array */
      (*table)->to_states     = (vector**)malloc_safe( sizeof( vector* ) * num_to_states );
      (*table)->num_to_states = num_to_states;
      for( i=0; i<num_to_states; i++ ) {
        (*table)->to_states[i] = NULL;
      }

      /* Read in vectors */
      for( i=0; i<num_to_states; i++ ) {
        vector_db_read( &((*table)->to_states[i]), line );
      }

      if( sscanf( *line, "%u%n", &num_arcs, &chars_read ) == 1 ) {

        *line += chars_read;

        /* Allocate arcs array */
        (*table)->arcs     = (fsm_table_arc**)malloc_safe( sizeof( fsm_table_arc* ) * num_arcs );
        (*table)->num_arcs = num_arcs;
        for( i=0; i<num_arcs; i++ ) {
          (*table)->arcs[i] = NULL;
        }

        for( i=0; i<num_arcs; i++ ) {

          /* Allocate fsm_table_arc */
          (*table)->arcs[i] = (fsm_table_arc*)malloc_safe( sizeof( fsm_table_arc ) );

          /*@-formatcode@*/
          if( sscanf( *line, "%u %u %hhx%n", &((*table)->arcs[i]->from), &((*table)->arcs[i]->to), &((*table)->arcs[i]->suppl.all), &chars_read ) != 3 ) {
          /*@=formatcode@*/
            print_output( "Unable to parse FSM table information from database.  Unable to read.", FATAL, __FILE__, __LINE__ );
            Throw 0;
          } else {
            *line += chars_read;
            curr_arc_id++;
          }

        }

      } else {
        print_output( "Unable to parse FSM table information from database.  Unable to read.", FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

    } else {
      print_output( "Unable to parse FSM table information from database.  Unable to read.", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

  } Catch_anonymous {
    arc_dealloc( *table );
    *table = NULL;
    Throw 0;
  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \throws anonymous Throw arc_db_read

 Merges the specified FSM arc information from the current line into the base FSM arc information.
*/
void arc_db_merge(
  fsm_table* base,  /*!< Pointer to arc table to merge data into */
  char**     line   /*!< Pointer to read in line from CDD file to merge */
) { PROFILE(ARC_DB_MERGE);

  /*@-mustfreeonly -mustfreefresh@*/

  fsm_table*   table;  /* Currently read FSM table */
  unsigned int i;      /* Loop iterator */

  /* Read in the table */
  arc_db_read( &table, line );

  /* Merge state transitions */
  for( i=0; i<table->num_arcs; i++ ) {
    arc_add( base, table->fr_states[table->arcs[i]->from], table->to_states[table->arcs[i]->to], table->arcs[i]->suppl.part.hit, table->arcs[i]->suppl.part.excluded );
  }

  /* Deallocate the merged table */
  arc_dealloc( table );

  PROFILE_END;

  /*@=mustfreeonly =mustfreefresh@*/

}

/*!
 Merges two FSM arcs into one, placing the result back into the base FSM arc.  This function is used to
 calculate module coverage for the GUI.
*/
void arc_merge(
  fsm_table*        base,
  const fsm_table*  other
) { PROFILE(ARC_MERGE);

  unsigned int i;  /* Loop iterator */

  /* Merge state transitions */
  for( i=0; i<other->num_arcs; i++ ) {
    arc_add( base, other->fr_states[other->arcs[i]->from], other->to_states[other->arcs[i]->to], other->arcs[i]->suppl.part.hit, other->arcs[i]->suppl.part.excluded );
  }

  PROFILE_END;

}

/*!
 Traverses entire arc array, storing all states that were hit
 during simulation (if hit parameter is true or the any parameter is true) or missed during simulation
 (if hit parameter is false or the any parameter is true).
*/
void arc_get_states(
  /*@out@*/ char***          fr_states,      /*!< Pointer to string array containing stringified state information */
  /*@out@*/ unsigned int*    fr_state_size,  /*!< Pointer to number of elements stored in states array */
  /*@out@*/ char***          to_states,      /*!< Pointer to string array containing stringified state information */
  /*@out@*/ unsigned int*    to_state_size,  /*!< Pointer to number of elements stored in states array */
            const fsm_table* table,          /*!< Pointer to FSM table */
            bool             hit,            /*!< Specifies if hit or missed transitions should be gathered */
            bool             any,            /*!< Specifies if we should gather any transition or only the type specified by hit */
            unsigned int     fr_width,       /*!< Width of "from" FSM state to output */
            unsigned int     to_width        /*!< Width of "to" FSM state to output */
) { PROFILE(ARC_GET_STATES);

  unsigned int i, j;  /* Loop iterator */

  /*@-nullstate@*/

  assert( fr_states != NULL );
  assert( fr_state_size != NULL );
  assert( to_states != NULL );
  assert( to_state_size != NULL );

  /* Initialize states array pointers and sizes */
  *fr_states     = NULL;
  *fr_state_size = 0;
  *to_states     = NULL;
  *to_state_size = 0;

  /* Iterate through the fr_states array, gathering all matching states */
  for( i=0; i<table->num_fr_states; i++ ) {
    bool state_hit = any;
    for( j=0; j<table->num_arcs; j++ ) {
      if( table->arcs[j]->from == i ) {
        state_hit = state_hit || (table->arcs[j]->suppl.part.hit == 1);
      }
    }
    if( state_hit == hit ) {
      *fr_states                     = (char**)realloc_safe( *fr_states, (sizeof( char* ) * (*fr_state_size)), (sizeof( char* ) * ((*fr_state_size) + 1)) );
      (*fr_states)[(*fr_state_size)] = vector_to_string( table->fr_states[i], HEXIDECIMAL, TRUE, fr_width );
      (*fr_state_size)++;
    }
  }

  /* Iterate through the to_states array, gathering all matching states */
  for( i=0; i<table->num_to_states; i++ ) {
    bool state_hit = any;
    for( j=0; j<table->num_arcs; j++ ) { 
      if( table->arcs[j]->to == i ) {
        state_hit = state_hit || (table->arcs[j]->suppl.part.hit == 1);
      }
    }
    if( state_hit == hit ) {
      *to_states                     = (char**)realloc_safe( *to_states, (sizeof( char* ) * (*to_state_size)), (sizeof( char* ) * ((*to_state_size) + 1)) );
      (*to_states)[(*to_state_size)] = vector_to_string( table->to_states[i], HEXIDECIMAL, TRUE, to_width );
      (*to_state_size)++;
    }
  }

  PROFILE_END;

}

/*!
 Traverses entire arc array, storing all state transitions that were hit
 during simulation (if hit parameter is true or the any parameter is true) or missed during simulation
 (if hit parameter is false or the any parameter is true).
*/
void arc_get_transitions(
  /*@out@*/ char***          from_states,  /*!< Pointer to string array containing from_state values */
  /*@out@*/ char***          to_states,    /*!< Pointer to string array containing to_state values */
  /*@out@*/ int**            ids,          /*!< List of arc IDs */
  /*@out@*/ int**            excludes,     /*!< Pointer to integer array containing exclude values */
  /*@out@*/ char***          reasons,      /*!< Pointer to string array containing exclude reasons */
  /*@out@*/ int*             arc_size,     /*!< Number of elements in both the from_states and to_states arrays */
            const fsm_table* table,        /*!< Pointer to FSM table */
            func_unit*       funit,        /*!< Pointer to functional unit containing this FSM */
            bool             hit,          /*!< Specifies if hit or missed transitions should be gathered */
            bool             any,          /*!< Specifies if all arc transitions or just the ones that meet the hit criteria should be gathered */
            unsigned int     fr_width,     /*!< Width of "from" FSM state to output */
            unsigned int     to_width      /*!< Width of "to" FSM state to output */
) { PROFILE(ARC_GET_TRANSITIONS);

  unsigned int i;  /* Loop iterator */

  assert( table != NULL );

  /* Initialize state arrays and arc_size */
  *from_states = NULL;
  *to_states   = NULL;
  *ids         = NULL;
  *excludes    = NULL;
  *reasons     = NULL;
  *arc_size    = 0;

  /* Iterate through arc transitions */
  for( i=0; i<table->num_arcs; i++ ) {

    if( (table->arcs[i]->suppl.part.hit == hit) || any ) {
      exclude_reason* er;

      *from_states                = (char**)realloc_safe( *from_states, (sizeof( char* ) * (*arc_size)), (sizeof( char* ) * (*arc_size + 1)) );
      *to_states                  = (char**)realloc_safe( *to_states,   (sizeof( char* ) * (*arc_size)), (sizeof( char* ) * (*arc_size + 1)) );
      *ids                        = (int*)realloc_safe( *ids, (sizeof( int ) * (*arc_size)), (sizeof( int ) * (*arc_size + 1)) );
      *excludes                   = (int*)realloc_safe( *excludes, (sizeof( int ) * (*arc_size)), (sizeof( int ) * (*arc_size + 1)) );
      *reasons                    = (char**)realloc_safe( *reasons, (sizeof( char* ) * (*arc_size)), (sizeof( char* ) * (*arc_size + 1)) );
      (*from_states)[(*arc_size)] = vector_to_string( table->fr_states[table->arcs[i]->from], HEXIDECIMAL, TRUE, fr_width );
      (*to_states)[(*arc_size)]   = vector_to_string( table->to_states[table->arcs[i]->to],   HEXIDECIMAL, TRUE, to_width );
      (*ids)[(*arc_size)]         = table->id + i;
      (*excludes)[(*arc_size)]    = table->arcs[i]->suppl.part.excluded;

      /* If the assertion is currently excluded, check to see if there's a reason associated with it */
      if( (table->arcs[i]->suppl.part.excluded == 1) && ((er = exclude_find_exclude_reason( 'F', (table->id + i), funit )) != NULL) ) {
        (*reasons)[(*arc_size)] = strdup_safe( er->reason );
      } else {
        (*reasons)[(*arc_size)] = NULL;
      }

      (*arc_size)++;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if any state transitions were excluded from coverage; otherwise,
         returns FALSE.
*/
bool arc_are_any_excluded(
  const fsm_table* table  /*!< Pointer to state transition arc array */
) { PROFILE(ARC_ARE_ANY_EXCLUDED);

  unsigned int i = 0;  /* Loop iterator */

  assert( table != NULL );

  while( (i < table->num_arcs) && (table->arcs[i]->suppl.part.excluded == 0) ) i++;

  PROFILE_END;

  return( i < table->num_arcs );

}
#endif /* RUNLIB */

/*!
 Deallocates all allocated memory for the specified state transition
 arc array.
*/
void arc_dealloc(
  fsm_table* table  /*!< Pointer to state transition arc array */
) { PROFILE(ARC_DEALLOC);

  if( table != NULL ) {

    unsigned int i;

    /* Deallocate fr_states */
    for( i=0; i<table->num_fr_states; i++ ) {
      vector_dealloc( table->fr_states[i] );
    }
    free_safe( table->fr_states, (sizeof( vector* ) * table->num_fr_states) );

    /* Deallocate to_states */
    for( i=0; i<table->num_to_states; i++ ) {
      vector_dealloc( table->to_states[i] );
    }
    free_safe( table->to_states, (sizeof( vector* ) * table->num_to_states) );

    /* Deallocate arcs */
    for( i=0; i<table->num_arcs; i++ ) {
      free_safe( table->arcs[i], sizeof( fsm_table_arc ) );
    }
    free_safe( table->arcs, (sizeof( fsm_table_arc* ) * table->num_arcs) );

    /* Now deallocate ourself */
    free_safe( table, sizeof( fsm_table ) );

  }

  PROFILE_END;

}
