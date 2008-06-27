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

#include "defines.h"
#include "arc.h"
#include "profiler.h"
#include "util.h"
#include "vector.h"
#include "expr.h"


/*!
 Displays the given state transition arcs in a human-readable format.
*/
/*@unused@*/ static void arc_display(
  const fsm_table* table  /*!< Pointer to state transition arc array to display */
) {

  unsigned int i;  /* Loop iterator */

  assert( table != NULL );

  for( i=0; i<table->num_arcs; i++ ) {

    char* lvec = vector_to_string( table->fr_states[table->arcs[i]->from], HEXIDECIMAL, TRUE );
    char* rvec = vector_to_string( table->to_states[table->arcs[i]->to],     HEXIDECIMAL, TRUE );

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
 \return Returns the index of the found arc in the arcs array if it is found, otherwise, returns -1.

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
  table->fr_states     = NULL;
  table->num_fr_states = 0;
  table->to_states     = NULL;
  table->num_to_states = 0;
  table->arcs          = NULL;
  table->num_arcs      = 0;

  PROFILE_END;

  return( table );

}

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
 Calculates values for all specified totals from given state transition arc array.
 If the state and state transition totals are not known (i.e., user specified state
 variables without specifying legal states and state transitions and/or the user
 specified state variables and state table was not able to be automatically extracted),
 return a value of -1 for total values to indicate to the calling function that a
 different report output is required.
*/
void arc_get_stats(
            const fsm_table* table,        /*!< Pointer to FSM table */
  /*@out@*/ int*             state_total,  /*!< Pointer to total number of states in table */
  /*@out@*/ int*             state_hits,   /*!< Pointer to total number of states hit during simulation */
  /*@out@*/ int*             arc_total,    /*!< Pointer to total number of state transitions in table */
  /*@out@*/ int*             arc_hits      /*!< Pointer to total number of state transitions hit during simulation */
) { PROFILE(ARC_GET_STATS);

  /* First get hits */
  *state_hits += arc_state_hits( table );
  *arc_hits   += arc_transition_hits( table );
  
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
    arc_add( base, table->fr_states[table->arcs[i]->from], table->to_states[table->arcs[i]->to], table->arcs[i]->suppl.part.hit, FALSE );
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
    arc_add( base, other->fr_states[other->arcs[i]->from], other->to_states[other->arcs[i]->to], other->arcs[i]->suppl.part.hit, FALSE );
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
            bool             any             /*!< Specifies if we should gather any transition or only the type specified by hit */
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
      (*fr_states)[(*fr_state_size)] = vector_to_string( table->fr_states[i], HEXIDECIMAL, TRUE );
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
      (*to_states)[(*to_state_size)] = vector_to_string( table->to_states[i], HEXIDECIMAL, TRUE );
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
  /*@out@*/ int**            excludes,     /*!< Pointer to integer array containing exclude values */
  /*@out@*/ int*             arc_size,     /*!< Number of elements in both the from_states and to_states arrays */
            const fsm_table* table,        /*!< Pointer to FSM table */
            bool             hit,          /*!< Specifies if hit or missed transitions should be gathered */
            bool             any           /*!< Specifies if all arc transitions or just the ones that meet the hit criteria should be gathered */
) { PROFILE(ARC_GET_TRANSITIONS);

  unsigned int i;  /* Loop iterator */

  assert( table != NULL );

  /* Initialize state arrays and arc_size */
  *from_states = NULL;
  *to_states   = NULL;
  *excludes    = NULL;
  *arc_size    = 0;

  /* Iterate through arc transitions */
  for( i=0; i<table->num_arcs; i++ ) {

    if( (table->arcs[i]->suppl.part.hit == hit) || any ) {
      *from_states                = (char**)realloc_safe( *from_states, (sizeof( char* ) * (*arc_size)), (sizeof( char* ) * (*arc_size + 1)) );
      *to_states                  = (char**)realloc_safe( *to_states,   (sizeof( char* ) * (*arc_size)), (sizeof( char* ) * (*arc_size + 1)) );
      if( any ) {
        *excludes = (int*)realloc_safe( *excludes, (sizeof( int ) * (*arc_size)), (sizeof( int ) * (*arc_size + 1)) );
        (*excludes)[(*arc_size)] = table->arcs[i]->suppl.part.excluded;
      }
      (*from_states)[(*arc_size)] = vector_to_string( table->fr_states[table->arcs[i]->from], HEXIDECIMAL, TRUE );
      (*to_states)[(*arc_size)]   = vector_to_string( table->to_states[table->arcs[i]->to],   HEXIDECIMAL, TRUE );
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

/*
 $Log$
 Revision 1.63  2008/06/19 16:14:53  phase1geo
 leaned up all warnings in source code from -Wall.  This also seems to have cleared
 up a few runtime issues.  Full regression passes.

 Revision 1.62  2008/05/30 23:00:47  phase1geo
 Fixing Doxygen comments to eliminate Doxygen warning messages.

 Revision 1.61  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.60.2.15  2008/05/28 05:57:09  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.60.2.14  2008/05/27 04:29:31  phase1geo
 Fixing memory leak for an FSM arc parser error.  Adding diagnostics to regression
 suite for coverage purposes.

 Revision 1.60.2.13  2008/05/23 14:50:20  phase1geo
 Optimizing vector_op_add and vector_op_subtract algorithms.  Also fixing issue with
 vector set bit.  Updating regressions per this change.

 Revision 1.60.2.12  2008/05/09 15:22:25  phase1geo
 Fixing memory initialization issue in arc.c that leads to inconsistent
 results.  Checkpointing.

 Revision 1.60.2.11  2008/05/08 23:12:38  phase1geo
 Fixing several bugs and reworking code in arc to get FSM diagnostics
 to pass.  Checkpointing.

 Revision 1.60.2.10  2008/05/08 03:56:38  phase1geo
 Updating regression files and reworking arc_find and arc_add functionality.
 Checkpointing.

 Revision 1.60.2.9  2008/05/07 21:09:10  phase1geo
 Added functionality to allow to_string to output full vector bits (even
 non-significant bits) for purposes of reporting for FSMs (matches original
 behavior).

 Revision 1.60.2.8  2008/05/07 05:57:25  phase1geo
 First attempt to fix FSM reporting issues.  Updating regression files and
 checkpointing.

 Revision 1.60.2.7  2008/05/05 19:49:59  phase1geo
 Updating regressions, fixing bugs and added new diagnostics.  Checkpointing.

 Revision 1.60.2.6  2008/05/04 05:48:40  phase1geo
 Attempting to fix expression_assign.  Updated regression files.

 Revision 1.60.2.5  2008/05/03 20:10:37  phase1geo
 Fixing some bugs, completing initial pass of vector_op_multiply and updating
 regression files accordingly.  Checkpointing.

 Revision 1.60.2.4  2008/05/03 04:06:54  phase1geo
 Fixing some arc bugs and updating regressions accordingly.  Checkpointing.

 Revision 1.60.2.3  2008/05/02 22:06:10  phase1geo
 Updating arc code for new data structure.  This code is completely untested
 but does compile and has been completely rewritten.  Checkpointing.

 Revision 1.60.2.2  2008/04/21 04:37:22  phase1geo
 Attempting to get other files (besides vector.c) to compile with new vector
 changes.  Still work to go here.  The initial pass through vector.c is not
 complete at this time as I am attempting to get what I have completed
 debugged.  Checkpointing work.

 Revision 1.60.2.1  2008/04/20 05:43:45  phase1geo
 More work on the vector file.  Completed initial pass of conversion operations,
 bitwise operations and comparison operations.

 Revision 1.60  2008/04/15 06:08:46  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.59  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.58  2008/03/18 21:36:24  phase1geo
 Updates from regression runs.  Regressions still do not completely pass at
 this point.  Checkpointing.

 Revision 1.57  2008/03/18 05:36:04  phase1geo
 More updates (regression still broken).

 Revision 1.56  2008/03/17 22:02:30  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.55  2008/03/17 05:26:15  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.54  2008/03/14 22:00:17  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.53  2008/03/10 22:00:31  phase1geo
 Working on more exception handling (script is finished now).  Starting to work
 on code enhancements again :)  Checkpointing.

 Revision 1.52  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.51  2008/02/02 14:11:54  phase1geo
 Adding new diagnostic for exclusion testing purposes.

 Revision 1.50  2008/02/02 05:50:16  phase1geo
 Adding more exclusion diagnostics to regression suite.  Fixed bug in arc
 state and state transition hit calculator.  Added arc_display function for
 debugging purposes.

 Revision 1.49  2008/02/01 06:37:07  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.48  2008/01/16 06:40:33  phase1geo
 More splint updates.

 Revision 1.47  2008/01/16 05:01:21  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.46  2008/01/10 04:59:03  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.45  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.44  2008/01/07 05:01:57  phase1geo
 Cleaning up more splint errors.

 Revision 1.43  2008/01/04 23:07:58  phase1geo
 More splint updates.

 Revision 1.42  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.41  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.40  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.39  2006/10/12 22:48:45  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.38  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.37  2006/06/29 22:44:57  phase1geo
 Fixing newly introduced bug in FSM report handler.  Also adding pointers back
 to main text window when exclusion properties are changed.  Fixing toggle
 coverage retension.  This is partially working but doesn't seem to want to
 save/restore properly at this point.

 Revision 1.36  2006/06/29 16:48:14  phase1geo
 FSM exclusion code now complete.

 Revision 1.35  2006/06/29 04:26:02  phase1geo
 More updates for FSM coverage.  We are getting close but are just not to fully
 working order yet.

 Revision 1.34  2006/06/28 22:15:19  phase1geo
 Adding more code to support FSM coverage.  Still a ways to go before this
 is completed.

 Revision 1.33  2006/06/23 19:45:26  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.32  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

 Revision 1.31  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.30  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.29  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.28  2006/02/02 22:37:40  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.27  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.26  2005/02/05 04:13:27  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.25  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.24  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.23  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.22  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.21  2003/11/16 04:03:38  phase1geo
 Updating development documentation for FSMs.

 Revision 1.20  2003/11/10 04:25:50  phase1geo
 Adding more FSM diagnostics to regression suite.  All major testing for
 current FSM code should be complete at this time.  A few bug fixes to files
 that were found during this regression testing.

 Revision 1.19  2003/11/08 04:21:26  phase1geo
 Adding several new FSM diagnostics to regression suite to verify inline
 attributes.  Fixed bug in arc.c where states were not being identified
 correctly as unique or not.

 Revision 1.18  2003/11/08 03:38:50  phase1geo
 Adding new FSM diagnostics to regression suite and fixing problem that was causing
 regressions to fail.

 Revision 1.17  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.16  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.15  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.14  2003/10/16 12:27:19  phase1geo
 Fixing bug in arc.c related to non-zero LSBs.

 Revision 1.13  2003/09/19 18:04:28  phase1geo
 Adding fsm3 diagnostic to check proper handling of wide state variables.
 Code fixes to support new diagnostic.

 Revision 1.12  2003/09/19 13:25:28  phase1geo
 Adding new FSM diagnostics including diagnostics to verify FSM merging function.
 FSM merging code was modified to work correctly.  Full regression passes.

 Revision 1.11  2003/09/19 02:34:51  phase1geo
 Added new fsm1.3 diagnostic to regress suite which found a bug in arc.c that is
 now fixed.  It had to do with resizing an arc array and copying its values.
 Additionally, some output was fixed in the FSM reports.

 Revision 1.10  2003/09/15 02:37:03  phase1geo
 Adding development documentation for functions that needed them.

 Revision 1.9  2003/09/15 01:13:57  phase1geo
 Fixing bug in vector_to_int() function when LSB is not 0.  Fixing
 bug in arc_state_to_string() function in creating string version of state.

 Revision 1.8  2003/09/14 01:09:20  phase1geo
 Added verbose output for FSMs.

 Revision 1.7  2003/09/13 19:53:59  phase1geo
 Adding correct way of calculating state and state transition totals.  Modifying
 FSM summary reporting to reflect these changes.  Also added function documentation
 that was missing from last submission.

 Revision 1.6  2003/09/13 06:05:12  phase1geo
 Adding code to properly count unique hit states.

 Revision 1.5  2003/09/13 02:59:34  phase1geo
 Fixing bugs in arc.c created by extending entry supplemental field to 5 bits
 from 3 bits.  Additional two bits added for calculating unique states.

 Revision 1.4  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.3  2003/08/29 12:52:06  phase1geo
 Updating comments for functions.

 Revision 1.2  2003/08/26 21:53:23  phase1geo
 Added database read/write functions and fixed problems with other arc functions.

 Revision 1.1  2003/08/26 12:53:35  phase1geo
 Added initial versions of arc.c and arc.h though they are not even close to
 being finished at this point.

*/

