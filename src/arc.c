/*!
 \file     arc.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     8/25/2003
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "defines.h"
#include "arc.h"
#include "util.h"
#include "vector.h"
#include "expr.h"


/*!
 \param width  Width of state machine variable.
 
 \return Returns the width (in characters) required to store one
         bidirectional state transition arc.

 Calculates the amount of storage (in numbers of characters) needed
 to store one bidirectional state transition arc.
*/
int arc_get_entry_width( int width ) {

  return( ((((width * 2) + ARC_ENTRY_SUPPL_SIZE) % 8) == 0) ?
                 (((width * 2) + ARC_ENTRY_SUPPL_SIZE) / 8) :
                 ((((width * 2) + ARC_ENTRY_SUPPL_SIZE) / 8) + 1) );

}

/*!
 \param arcs   Pointer to state transition arc array.
 \param width  Value to set width to.

 Stores the width of the state variable in the specified
 state transition arc array.  Note that the width cannot exceed
 2^16.
*/
void arc_set_width( char* arcs, int width ) {

  arcs[0] = (char)(width & 0xff);
  arcs[1] = (char)((width >> 8) & 0xff);

}

/*!
 \param arcs  Pointer to state transition arc array.

 \return Returns the width stored in the specified state transition
         arc array.

 Retrieves the previously stored value of the state transition arc
 width.
*/
int arc_get_width( char* arcs ) {

  int width = (((int)arcs[1] & 0xff) << 8) | ((int)arcs[0] & 0xff);
  return( width );

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param size  Current allocated size of the state transition arc array
              (in entry widths) not including the header information.

 Sets the current allocated size of the state transition arc array
 in the state transition arc array.  The value of size is the number of
 bidirectional state transition arc entries within the table.
*/
void arc_set_max_size( char* arcs, int size ) {

  arcs[2] = (char)(size & 0xff);
  arcs[3] = (char)((size >> 8) & 0xff);

}

/*!
 \param arcs  Pointer to state transition arc array.
 
 \return Returns the current allocated size of the state transition arc array.

 Retrieves the previously stored current allocated state transition arc
 array size that is encoded into the arcs array.  This value is the number
 of bidirectional state transition arc entries allocated in this arc array.
*/
int arc_get_max_size( char* arcs ) {

  int size = (((int)arcs[3] & 0xff) << 8) | ((int)arcs[2] & 0xff);
  return( size );

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param size  Current used size of the state transition arc array (in entry widths)
              not including the header information.

 Sets the current used size of the state transition arc array
 in the state transition arc array.  The value of size is the number of
 bidirectional state transition arc entries currently used within the table.
*/
void arc_set_curr_size( char* arcs, int size ) {

  arcs[4] = (char)(size & 0xff);
  arcs[5] = (char)((size >> 8) & 0xff);

}

/*!
 \param arcs  Pointer to state transition arc array.

 \return Returns the number of used bidirectional state transition arc
         entries in the specified arcs array.

 Retrieves the previously stored value of the number of currently used
 bidirectional state transition arc entries in the specified arcs array.
*/
int arc_get_curr_size( char* arcs ) {

  int size = (((int)(arcs[5]) & 0xff) << 8) | ((int)(arcs[4]) & 0xff);
  return( size );

}

/*!
 \param arcs   Pointer to state transition arc array.
 \param suppl  Value of supplemental field to store.

 Sets the supplemental control field of the specified arc array to
 the specified value.
*/
void arc_set_suppl( char* arcs, int suppl ) {

  arcs[6] = (char)(suppl & 0xff);

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param type  Specifies bit to retrieve.

 \return Returns the value of the supplemental field of the specified
         arc array.

 Retrieves the value of the supplemental field in the specified arc
 array.
*/
int arc_get_suppl( char* arcs, int type ) {

  return( ((int)arcs[6] >> type) & 0x1 );

}

/*!
 \param arcs   Pointer to state transition arc array.
 \param start  Starting index of bidirectional state transition entry to set.
 \param left   Pointer to left state vector value.
 \param right  Pointer to right state vector value.

 \return Returns TRUE if value was properly stored; otherwise, returns FALSE.

 Sets the state value of 
*/
bool arc_set_states( char* arcs, int start, vector* left, vector* right ) {

  bool   retval = TRUE;  /* Return value of this function                  */
  char   mask;           /* Mask to apply to current bit select            */
  vector value;          /* Vector to hold current bit select              */
  int    pos;            /* Current 8-bit boundary bit position            */
  int    i;              /* Loop iterator                                  */
  int    j;              /* Loop iterator                                  */
  int    curr;           /* Current index of arc array to set              */
  int    entry_size;     /* Number of characters needed to store one entry */
  int    index;          /* Current index of vector to extract             */

  /* Check specified vector for unknown information */
  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = FALSE;

  } else {

    // printf( "Setting state (%d) to (%d)\n", vector_to_int( left ), vector_to_int( right ) );

    entry_size = arc_get_entry_width( arc_get_width( arcs ) );

    for( j=0; j<2; j++ ) {
  
      if( j == 0 ) {
        pos         = (arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) % 8;
        curr        = (start * entry_size) + ((arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) / 8) + ARC_STATUS_SIZE;
        value.value = left->value;
      } else {
        pos         = ARC_ENTRY_SUPPL_SIZE;
        curr        = (start * entry_size) + ARC_STATUS_SIZE;
        value.value = right->value;
      }

      value.width = (arc_get_width( arcs ) > (8 - pos)) ? (8 - pos) : arc_get_width( arcs );
      index       = 0;

      while( index < arc_get_width( arcs ) ) {
        mask = 0;
        for( i=0; i<value.width; i++ ) {
          mask <<= 1;
          mask  |= (0x1 << pos);
        }
        arcs[curr] |= ((vector_to_int( &value ) << pos) & mask);
        // printf( "arcs[%d]: %x, value: %x\n", curr, ((int)arcs[curr] & 0xff), ((vector_to_int( &value ) << pos) & mask) );
        index       += value.width;
        value.value += value.width;
        value.width  = ((arc_get_width( arcs ) - index) > 8) ? 8 : (arc_get_width( arcs ) - index);
        pos          = 0;
        curr++;
      }

    }

  }

  return( retval );

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param curr  Current arc entry to modify.
 \param type  Supplemental bit type to modify.
 \param val   Value to set.

 Sets the value of the specified type bit in the arc entry specified
 by arcs and curr.
*/
void arc_set_entry_suppl( char* arcs, int curr, int type, char val ) {

  int entry_size = arc_get_entry_width( arc_get_width( arcs ) );

  arcs[(curr * entry_size) + ARC_STATUS_SIZE] = arcs[(curr * entry_size) + ARC_STATUS_SIZE] | ((val & 0x1) << type);

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param curr  Current arc entry to modify.
 \param type  Supplemental bit type to modify.

 \return Returns the 1-bit value of the entry supplemental field specified
         by type.

 Retrieves the arc entry supplemental field bit specified by the function
 parameters.
*/
int arc_get_entry_suppl( char* arcs, int curr, int type ) {

  int entry_size = arc_get_entry_width( arc_get_width( arcs ) );

  return( (int)((arcs[(curr * entry_size) + ARC_STATUS_SIZE] >> type) & 0x1) );

}

/*!
 \param arcs     Pointer to arc array to search in.
 \param from_st  From state to use for matching.
 \param to_st    To state to use for matching.
 \param ptr      Pointer to index of matched entry containing the suppl field.

 \return Returns a value of 0 if the match was from_st/to_st, returns a value of
         1 indicating that the match was to_st/from_st, or returns a value of 2 indicating
         that no match occurred.

 Searches specified arc array for an entry that matches the from_st/to_st combination.
 Because from/to combinations are only stored once in an arc array, we need to check
 for matches with from/to and to/from.  Once a match has been found, the calling function
 will either modify the supplemental data that is associated with this function's return
 value or it will check to see if another arc entry is required to be added.
*/
int arc_find( char* arcs, vector* from_st, vector* to_st, int* ptr ) {

  char  tmp[264];    /* Temporary arc array entry for comparison purposes              */
  int   curr_size;   /* Current number of entries in this arc array                    */
  int   entry_size;  /* Number of characters for a given entry                         */
  int   i;           /* Loop iterator                                                  */
  int   j;           /* Loop iterator                                                  */
  int   k;           /* Loop iterator                                                  */
  int   type;        /* Search type (0 = match bidir only if bidir set, 1 = match all) */

  type       = *ptr;
  curr_size  = arc_get_curr_size( arcs );
  entry_size = arc_get_entry_width( arc_get_width( arcs ) );
  *ptr       = -1;

  /* Initialize tmp */
  for( i=0; i<264; i++ ) {
    tmp[i] = 0;
  }
  arc_set_width( tmp, arc_get_width( arcs ) );

  i = 0;
  while( (i < 2) && (*ptr == -1) ) {
    if( i == 0 ) {
      /* Create from/to combination */
      if( arc_set_states( tmp, 0, from_st, to_st ) ) {
        j = 0;
        while( (j < curr_size) && (*ptr == -1) ) {
          k = 0;
          while( (k < entry_size) &&
                 (((k == 0) && ((arcs[(((j * entry_size) + k) + ARC_STATUS_SIZE)] & 0xe0) == (tmp[k + ARC_STATUS_SIZE] & 0xe0))) ||
                  ((k != 0) &&  (arcs[(((j * entry_size) + k) + ARC_STATUS_SIZE)]         ==  tmp[k + ARC_STATUS_SIZE]))) ) {
            k++;
          }
          if( k == entry_size ) {
            *ptr = (j * entry_size);
          }
          j++;
        }
      }
    } else {
      /* Create to/from combination -- bidirectional case */
      if( arc_set_states( tmp, 0, to_st, from_st ) ) {
        j = 0;
        while( (j < curr_size) && (*ptr == -1) ) {
          if( (type == 1) || ((type == 0) && (arc_get_entry_suppl( arcs, j, ARC_BIDIR ) == 1)) ) {
            k = 0;
            while( (k < entry_size) && 
                   (((k == 0) && ((arcs[(((j * entry_size) + k) + ARC_STATUS_SIZE)] & 0xe0) == (tmp[k + ARC_STATUS_SIZE] & 0xe0))) ||
                    ((k != 0) &&  (arcs[(((j * entry_size) + k) + ARC_STATUS_SIZE)]         ==  tmp[k + ARC_STATUS_SIZE]))) ) {
              k++;
            }
            if( k == entry_size ) {
              *ptr = (j * entry_size);
            }
          }
          j++;
        }
      }
    }
    i++;
  }

  if( (i != 2) || (*ptr != -1) ) {
    i = i - 1;
  }

  return( i );

}

/*!
 \param width  Width of arc array to create.

 \return Returns a pointer to the newly created arc transition structure.

 Allocates memory for a new state transition arc array and initializes its
 contents.
*/
char* arc_create( int width ) {

  char* arcs;  /* Pointer to newly create state transition array */
  int   i;     /* Loop iterator                                  */

  /* The arcs char array is not allocated, allocate the default space here */
  arcs = (char*)malloc_safe( (arc_get_entry_width( width ) * width) + ARC_STATUS_SIZE );

  /* Initialize */
  arc_set_width( arcs, width );     /* Signal width                                   */
  arc_set_max_size( arcs, width );  /* Number of entries in current arc array         */
  arc_set_curr_size( arcs, 0 );     /* Current entry pointer to store new             */
  arc_set_suppl( arcs, 0 );         /* Initialize supplemental field to zeros for now */

  for( i=0; i<(arc_get_entry_width( width ) * width); i++ ) {
    arcs[i+ARC_STATUS_SIZE] = 0;
  }

  return( arcs );

}

/*!
 \param arcs   Pointer to state transition arc array to add to.
 \param width  Width of state variable for this FSM.
 \param fr_st  Pointer to vector containing the from state.
 \param to_st  Pointer to vector containing the to state.
 \param hit    Specifies if arc entry should be marked as hit.

 If specified arcs array has not been created yet (value is set to NULL),
 allocate enough memory in the arc array to hold width number of state transitions.
 If the specified arcs array has been created but is currently full (arc array
 max_size and curr_size are equal to each other), add width number of more
 arc array entries to the current array.  After memory has been allocated, create
 a state transition entry from the fr_st and to_st expressions, setting the
 hit bits in the entry to 0.
*/
void arc_add( char** arcs, int width, vector* fr_st, vector* to_st, int hit ) {

  char* tmp;          /* Temporary char array holder              */
  int   entry_width;  /* Width of a signal entry in the arc array */
  int   i;            /* Loop iterator                            */
  int   ptr;          /* Pointer to entry index of matched entry  */
  int   side;         /* Specifies the direction of matched entry */

  assert( *arcs != NULL );

  ptr  = 1;   /* Tell find function to check for a match even if opposite bit is not set. */
  side = arc_find( *arcs, fr_st, to_st, &ptr );

  if( (side == 2) && (arc_get_curr_size( *arcs ) == arc_get_max_size( *arcs )) ) {

    /* We have maxed out the array, reallocate */
    tmp = *arcs;

    /* Calculate the entry width */
    entry_width = arc_get_entry_width( arc_get_width( tmp ) );

    /* Allocate new memory */
    *arcs = (char*)malloc_safe( (entry_width * (arc_get_max_size( tmp ) + arc_get_width( tmp ))) + ARC_STATUS_SIZE );

    arc_set_width( *arcs, arc_get_width( tmp ) );
    arc_set_max_size( *arcs, arc_get_max_size( tmp ) + arc_get_width( tmp ) );
    arc_set_curr_size( *arcs, arc_get_curr_size( tmp ) );
    arc_set_suppl( *arcs, ((int)tmp[6] & 0xff) );
    for( i=0; i<(entry_width * (arc_get_max_size( tmp ) + arc_get_width( tmp ))); i++ ) {
      if( i < ((arc_get_max_size( tmp ) * entry_width)) ) {
        (*arcs)[i+ARC_STATUS_SIZE] = tmp[i+ARC_STATUS_SIZE];
      } else {
        (*arcs)[i+ARC_STATUS_SIZE] = 0;
      }
    }

    /* Deallocate old memory */
    free_safe( tmp );

  }

  if( side == 2 ) {

    /* Initialize arc entry */
    // printf( "Actually setting state now, hit: %d, %d\n", hit, arc_get_curr_size( *arcs ) );
    if( arc_set_states( *arcs, arc_get_curr_size( *arcs ), fr_st, to_st ) ) {
      arc_set_entry_suppl( *arcs, arc_get_curr_size( *arcs ), ARC_HIT_F, hit );
      arc_set_entry_suppl( *arcs, arc_get_curr_size( *arcs ), ARC_HIT_R, 0 );
      arc_set_curr_size( *arcs, (arc_get_curr_size( *arcs ) + 1) );
    }

  } else if( side == 1 ) {

    arc_set_entry_suppl( *arcs, ptr, ARC_BIDIR, 1 );
    arc_set_entry_suppl( *arcs, ptr, ARC_HIT_R, hit );

  } else if( side == 0 ) {

    arc_set_entry_suppl( *arcs, ptr, ARC_HIT_F, hit );

  }

}

/*!
 \param arcs    Pointer to state transition arc array.
 \param index1  Index of first state to compare.
 \param pos1    Starting bit position to compare of the first state.
 \param index2  Index of second state to compare.
 \param pos2    Starting bit position to compare of the second state.

 \return Returns a value of TRUE if the two state values are equal; otherwise,
         returns a value of FALSE.

 Performs a bitwise comparison of the two states indicated by their index and pos
 values.  If both states compare, return TRUE; otherwise, return FALSE.
*/
bool arc_compare_states( char* arcs, int index1, int pos1, int index2, int pos2 ) {

  int i;  /* Loop iterator */

  i = 0;
  while( (i < arc_get_width( arcs )) && (((arcs[index1] >> pos1) & 0x1) == ((arcs[index2] >> pos2) & 0x1)) ) {
    pos1   = (pos1 + 1) % 8;
    pos2   = (pos2 + 1) % 8;
    index1 = (pos1 == 0) ? (index1 + 1) : index1;
    index2 = (pos2 == 0) ? (index2 + 1) : index2;
    i++;
  }

  return( i == arc_get_width( arcs ) );

}

/*!
 \param arcs   Pointer to state transition arc array.
 \param start  Specifies entry index to being comparing from.
 \param left   Specifies to use the left state as the state to compare against.

 Walks through the entire arc table starting at the index specified by the
 start parameter.  If the left state is chosen to be compared against, take
 the right state of the same entry and initially compare this state.  If the
 state is the same, set the right state ARC_NOT_UNIQUE_R bit to a value of one
 to indicate that this state is known to not be unique.  Perform the same comparison
 process to all of the rest of the states in all entries after this entry.
 If the right state is chosen to be used as the initial comparison state value,
 start comparing the left state at the next index of the table and continue for
 the rest of the table.
*/
void arc_compare_all_states( char* arcs, int start, bool left ) {

  int state1_pos;    /* Bit position of current state        */
  int state1_index;  /* Character position of current state  */
  int state2_pos;    /* Bit position of state to check       */
  int state2_index;  /* Character position of state to check */
  int entry_size;    /* Characters needed to store one entry */
  int i;             /* Loop iterator                        */
  int j;             /* Loop iterator                        */

  entry_size = arc_get_entry_width( arc_get_width( arcs ) );

  // printf( "Comparing against start: %d, left: %d\n", start, left );

  if( left ) {
    state1_pos   = (arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) % 8;
    state1_index = (start * entry_size) + ((arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) / 8) + ARC_STATUS_SIZE;
    j            = 1;
  } else {
    state1_pos   = ARC_ENTRY_SUPPL_SIZE;
    state1_index = (start * entry_size) + ARC_STATUS_SIZE;
    j            = 0;
    start++;
  }

  for( i=start; i<arc_get_curr_size( arcs ); i++ ) {
    for( ; j<2; j++ ) {
   
      // printf( "Comparing with i: %d, j: %d\n", i, j );

      /* Left */
      if( j == 0 ) {
        state2_pos   = (arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) % 8;
        state2_index = (i * entry_size) + ((arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) / 8) + ARC_STATUS_SIZE;
      } else {
        state2_pos   = ARC_ENTRY_SUPPL_SIZE;
        state2_index = (i * entry_size) + ARC_STATUS_SIZE;
      }

      if( arc_compare_states( arcs, state1_index, state1_pos, state2_index, state2_pos ) ) {
        // printf( "Found match\n" );
        if( j == 0 ) {
          arc_set_entry_suppl( arcs, i, ARC_NOT_UNIQUE_L, 1 );
        } else {
          arc_set_entry_suppl( arcs, i, ARC_NOT_UNIQUE_R, 1 );
        }
      }

    }
    j = 0;
  }

}

/*!
 \param arcs  Pointer to state transition arc array.

 \return Returns total number of states in specified arc array.

 Accumulates the total number of unique states in the specified arc
 array.  This function should only be called if the ARC_TRANS_KNOWN bit
 is set; otherwise, an incorrect value will be reported.
*/
float arc_state_total( char* arcs ) {

  float total = 0;  /* Total number of states hit during simulation */
  int   i;          /* Loop iterator                                */

  for( i=0; i<arc_get_curr_size( arcs ); i++ ) {
    if( arc_get_entry_suppl( arcs, i, ARC_NOT_UNIQUE_L ) == 0 ) {
      total++;
    }
    if( arc_get_entry_suppl( arcs, i, ARC_NOT_UNIQUE_R ) == 0 ) {
      total++;
    }
  }

  return( total );

}

/*!
 \param arcs  Pointer to state transition arc array.

 \return Returns number of unique states hit during simulation.

 Traverses through specified state transition table, figuring out what
 states in the table are unique.  This is done by traversing through
 the entire arc array, comparing states that have their ARC_NOT_UNIQUE_x
 bits set to a value 0.  If both states contain the same value, the
 second state has its ARC_NOT_UNIQUE_x set to indicate that this state
 is not unique to the table.
*/
int arc_state_hits( char* arcs ) {

  int hit = 0;     /* Number of states hit */
  int i;           /* Loop iterator        */
  int j;           /* Loop iterator        */

  for( i=0; i<arc_get_curr_size( arcs ); i++ ) {
    for( j=0; j<2; j++ ) {

      /* Do left first */
      if( j == 0 ) {
        if( arc_get_entry_suppl( arcs, i, ARC_NOT_UNIQUE_L ) == 0 ) {
          arc_compare_all_states( arcs, i, TRUE );
          if( arc_get_entry_suppl( arcs, i, ARC_HIT_F ) == 1 ) {
            hit++;
          }
          // printf( "1 Hit: %d\n", hit );
        }
      } else {
        if( arc_get_entry_suppl( arcs, i, ARC_NOT_UNIQUE_R ) == 0 ) {
          if( (i + 1) < arc_get_curr_size( arcs ) ) {
            arc_compare_all_states( arcs, i, FALSE );
          }
          if( arc_get_entry_suppl( arcs, i, ARC_HIT_F ) == 1 ) {
            hit++;
          }
          // printf( "2 Hit: %d\n", hit );
        }
      }

    }
  }

  return( hit );

}

/*!
 \param arcs  Pointer to state transition arc array.

 \return Returns the total number of state transitions found in
         the specified arc array.

 Accumulates the total number of state transitions specified in
 the given state transition arc array.  This function should only
 be called if the ARC_TRANS_KNOWN bit is set in the supplemental
 field of the arc array; otherwise, its value will be incorrect.
 For consistency, this function should be called after calling
 arc_transition_hits().
*/
float arc_transition_total( char* arcs ) {

  float total;  /* Number of total state transitions in arc array */
  int   i;      /* Loop iterator                                  */

  /* To start, get the current number of entries in the arc */
  total = arc_get_curr_size( arcs );

  /* Now just add to it the number of bidirectional entries */
  for( i=0; i<arc_get_curr_size( arcs ); i++ ) {
    if( arc_get_entry_suppl( arcs, i, ARC_BIDIR ) == 1 ) {
      total++;
    }
  }

  return( total );

}

/*!
 \param arcs  Pointer to state transition arc array.

 \return Returns the number of hit state transitions in the specified
         arc array.

 Iterates through arc array, accumulating the number of state
 transitions that were hit in simulation.
*/
int arc_transition_hits( char* arcs ) {

  int hit = 0;     /* Number of arcs hit                          */
  int i;           /* Loop iterator                               */
  int curr_size;   /* Current size of arc array                   */
  int entry_size;  /* Number of characters that contain one entry */

  /* Get some size values */
  curr_size  = arc_get_curr_size( arcs );
  entry_size = arc_get_entry_width( arc_get_width( arcs ) );

  /* Count the number of hits in the FSM arc */
  for( i=0; i<curr_size; i++ ) {
    hit += arc_get_entry_suppl( arcs, i, ARC_HIT_F );
    hit += arc_get_entry_suppl( arcs, i, ARC_HIT_R );
  }

  return( hit );

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param state_total  Pointer to total number of states in table.
 \param state_hits   Pointer to total number of states hit during simulation.
 \param arc_total    Pointer to total number of state transitions in table.
 \param arc_hits     Pointer to total number of state transitions hit during simulation.

 Calculates values for all specified totals from given state transition arc array.
 If the state and state transition totals are not known (i.e., user specified state
 variables without specifying legal states and state transitions and/or the user
 specified state variables and state table was not able to be automatically extracted),
 return a value of -1 for total values to indicate to the calling function that a
 different report output is required.
*/
void arc_get_stats( char* arcs, float* state_total, int* state_hits, float* arc_total, int* arc_hits ) {

  /* First get hits */
  *state_hits = arc_state_hits( arcs );
  *arc_hits   = arc_transition_hits( arcs );
  
  /* If the state transitions are known, calculate them; otherwise, return -1 for totals */
  if( arc_get_suppl( arcs, ARC_TRANS_KNOWN ) == 0 ) {
    *state_total = -1;
    *arc_total   = -1;
  } else {
    *state_total = arc_state_total( arcs );
    *arc_total   = arc_transition_total( arcs );
  }

}

/*!
 \param arcs  Pointer to state transition arc array to write.
 \param file  Pointer to CDD file to write.

 \return Returns TRUE if write was successful; otherwise, returns FALSE.

 Writes the specified arcs array to the specified CDD output file.  An arc array
 is output in a special format that is described in the above documentation for
 this file.
*/
bool arc_db_write( char* arcs, FILE* file ) {

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator                  */

  for( i=0; i<(arc_get_curr_size( arcs ) * arc_get_entry_width( arc_get_width( arcs ) )) + ARC_STATUS_SIZE; i++ ) {
    // printf( "arcs[%d]; %x\n", i, (int)arcs[i] & 0xff );
    if( (int)arcs[i] == 0 ) {
      fprintf( file, "," );
    } else {
      fprintf( file, "%02x", (((int)arcs[i]) & 0xff) );
    }
  }

  return( retval );

}

/*!
 \param line  Pointer to current string being parsed from CDD file.

 \return Returns the integer value of the next "character" from the string
         or a value of -1 if there was an error in reading the next character.

 Parses the specified string for the next arc "character" and returns its
 value to the calling function.  An arc character is defined as either a
 combination of two hexidecimal digits (which correspond to an 8-bit ASCII
 character) or the comma (,) character which represents a value of 0.
*/
int arc_read_get_next_value( char** line ) {

  char tmp[3];
  int  value;

  if( (*line)[0] == ',' ) {
    value = 0;
    *line = *line + 1;
  } else {
    tmp[0] = (*line)[0];
    tmp[1] = (*line)[1];
    tmp[2] = '\0';

    if( sscanf( tmp, "%x", &value ) != 1 ) {
      value = -1;
    } else {
      *line = *line + 2;
    }
  }

  return( value );

}

/*!
 \param arcs  Pointer to state transition arc array.
 \param line  String containing current CDD line of arc information.

 \return Returns TRUE if arc was read and stored correctly; otherwise,
         returns FALSE.

 Reads in specified state transition arc table, allocating the appropriate
 space to hold the table.  Returns TRUE if the specified line contained an
 appropriately written arc transition table; otherwise, returns FALSE.
*/
bool arc_db_read( char** arcs, char** line ) {

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator                  */
  int  val;            /* Current character value        */
  int  width;          /* Arc signal width               */
  int  max_size;       /* Size of arc array              */
  int  curr_size;      /* Current size of arc array      */
  int  suppl;          /* Supplemental field             */

  /* Get sizing information */
  width     =  (arc_read_get_next_value( line ) & 0xff) | 
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  max_size  =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  curr_size =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  suppl     =  (arc_read_get_next_value( line ) & 0xff);

  /* Allocate memory */
  *arcs = (char*)malloc_safe( (arc_get_entry_width( width ) * curr_size) + ARC_STATUS_SIZE );

  /* Initialize */
  arc_set_width( *arcs, width );
  arc_set_max_size( *arcs, curr_size );
  arc_set_curr_size( *arcs, curr_size );
  arc_set_suppl( *arcs, suppl );

  /* Read in rest of values */ 
  i = ARC_STATUS_SIZE;
  while( (i < ((curr_size * arc_get_entry_width( width )) + ARC_STATUS_SIZE)) && retval ) {
    if( (val = arc_read_get_next_value( line )) != -1 ) {
      (*arcs)[i] = (char)(val & 0xff);
    } else {
      retval = FALSE;
    }
    i++;
  }

  return( retval );

}

/*!
 \param arcs   Pointer to state transition arc array.
 \param index  Arc entry index to convert,
 \param left   If true, converts left state; otherwise, converts right state of entry.
 \param str    String to store converted value to.

 Converts the state specified by index and left parameters from its compacted bit format
 to a hexidecimal string format.
*/
void arc_state_to_string( char* arcs, int index, bool left, char* str ) {

  char tmp[2];       /* Temporary string holder                       */
  int  val;          /* Temporary storage for integer value of 4-bits */
  int  str_index;    /* Index to store next character into str        */
  int  pos;          /* Specifies current bit position to extract     */
  int  entry_width;  /* Character width to store one arc entry        */
  int  i;            /* Loop iterator                                 */

  /* Initialize variables */
  str_index   = ((arc_get_width( arcs ) % 4) == 0) ? (arc_get_width( arcs ) / 4) : ((arc_get_width( arcs ) / 4) + 1);
  val         = 0;
  entry_width = arc_get_entry_width( arc_get_width( arcs ) );

  if( left ) {
    index = (index * entry_width) + ((arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) / 8) + ARC_STATUS_SIZE;
    pos   = (arc_get_width( arcs ) + ARC_ENTRY_SUPPL_SIZE) % 8;
  } else {
    index = (index * entry_width) + ARC_STATUS_SIZE;
    pos   = ARC_ENTRY_SUPPL_SIZE;
  }

  /* Store the NULL character to the string */
  str[str_index] = '\0';
  str_index--;

  /* Make a bit-wise extraction of current state */
  for( i=0; i<arc_get_width( arcs ); i++ ) {
    val >>= 1;
    val  |= ((int)(arcs[index] >> pos) & 0x1) << 3;  
    if( (i % 4) == 3 ) {
      snprintf( tmp, 2, "%x", val );
      str[str_index] = tmp[0];
      str_index--;
      val = 0;
    }
    pos   = (pos + 1) % 8;
    index = (pos == 0) ? (index + 1) : index;
  } 
 
  if( (i % 4) != 0 ) {
    val >>= (4 - i);
    snprintf( tmp, 2, "%x", val );
    str[str_index] = tmp[0];
  }

}

/*!
 \param base  Pointer to arc table to merge data into.
 \param line  Pointer to read in line from CDD file to merge.
 \param same  Specifies if arc table to merge needs to be exactly the same as the existing arc table.

 \return Returns TRUE if line was read in correctly; otherwise, returns FALSE.

*/
bool arc_db_merge( char** base, char** line, bool same ) {

  bool    retval = TRUE;  /* Return value for this function     */
  char*   arcs;           /* Read arc array                     */
  char*   strl;           /* Left state value string            */
  char*   strr;           /* Right state value string           */
  char*   tmpl;           /* Temporary left state value string  */
  char*   tmpr;           /* Temporary right state value string */
  vector* vecl;           /* Left state vector value            */
  vector* vecr;           /* Right state vector value           */
  int     i;              /* Loop iterator                      */
  char    str_width[20];  /* Temporary string holder            */

  if( arc_db_read( &arcs, line ) ) {

    /* Check to make sure that arc arrays are compatible */
    if( same && (arc_get_width( *base ) != arc_get_width( arcs )) ) {
      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
      exit( 1 );
    }

    /* Calculate strlen of arc array width */
    snprintf( str_width, 20, "%d", arc_get_width( arcs ) );

    /* Allocate string to hold value string */
    strl = (char*)malloc_safe( (arc_get_width( arcs ) / 4) + 4 + strlen( str_width ) );
    strr = (char*)malloc_safe( (arc_get_width( arcs ) / 4) + 4 + strlen( str_width ) );

    /* Get prefix of left and right state value strings ready */
    snprintf( strl, ((arc_get_width( arcs ) / 4) + 4 + strlen( str_width )), "%s'h", str_width );
    snprintf( strr, ((arc_get_width( arcs ) / 4) + 4 + strlen( str_width )), "%s'h", str_width );

    tmpl = strl;
    tmpr = strr;

    for( i=0; i<arc_get_curr_size( arcs ); i++ ) {

      /* Get string versions of state values */
      arc_state_to_string( arcs, i, TRUE,  (strl + 2 + strlen( str_width )) );      
      arc_state_to_string( arcs, i, FALSE, (strr + 2 + strlen( str_width )) );      

      /* Convert these strings to vectors */
      vecl = vector_from_string( &strl );
      vecr = vector_from_string( &strr );

      /* Add these states to the base arc array */
      arc_add( base, arc_get_width( arcs ), vecl, vecr, arc_get_entry_suppl( arcs, i, ARC_HIT_F ) );
      if( arc_get_entry_suppl( arcs, i, ARC_BIDIR ) == 1 ) {
        arc_add( base, arc_get_width( arcs ), vecr, vecl, arc_get_entry_suppl( arcs, i, ARC_HIT_R ) );
      }

      strl = tmpl;
      strr = tmpr;

      vector_dealloc( vecl );
      vector_dealloc( vecr );

    }

    free_safe( strl );
    free_safe( strr );
    free_safe( arcs );

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param ofile  Pointer to file handle for report output.
 \param fstr   Formatting string for string output.
 \param arcs   Pointer to state transition arc array.
 \param hit    Specifies if hit or missed transitions should be displayed.

 Traverses entire arc array, displaying all states that were hit
 during simulation (if hit parameter is true) or missed during simulation
 (if hit parameter is false).  All output is sent to the file ofile using
 the formatting string specified by fstr.
*/
void arc_display_states( FILE* ofile, char* fstr, char* arcs, bool hit ) {

  char* str;  /* Holder for string value of current state */
  int   i;    /* Loop iterator                            */
  int   j;    /* Loop iterator                            */

  str = (char*)malloc_safe( (arc_get_width( arcs ) / 4) + 2 );

  for( i=0; i<arc_get_curr_size( arcs ); i++ ) {
    for( j=0; j<2; j++ ) {

      /* Check left first */
      if( j == 0 ) {
        if( arc_get_entry_suppl( arcs, i, ARC_NOT_UNIQUE_L ) == 0 ) {
          if( arc_get_entry_suppl( arcs, i, ARC_HIT_F ) == hit ) {
            arc_state_to_string( arcs, i, TRUE, str );
            fprintf( ofile, fstr, arc_get_width( arcs ), str );
          }
        }
      } else {
        if( arc_get_entry_suppl( arcs, i, ARC_NOT_UNIQUE_R ) == 0 ) {
          if( arc_get_entry_suppl( arcs, i, ARC_HIT_F ) == hit ) {
            arc_state_to_string( arcs, i, FALSE, str );
            fprintf( ofile, fstr, arc_get_width( arcs ), str );
          }
        }
      }    

    }
  }

  free_safe( str );

}

/*!
 \param ofile  Pointer to file handle for report output.
 \param fstr   Formatting string for string output.
 \param arcs   Pointer to state transition arc array.
 \param hit    Specifies if hit or missed transitions should be displayed.

 Traverses entire arc array, displaying all state transitions that were hit
 during simulation (if hit parameter is true) or missed during simulation
 (if hit parameter is false).  All output is sent to the file ofile using
 the formatting string specified by fstr.
*/
void arc_display_transitions( FILE* ofile, char* fstr, char* arcs, bool hit ) {

  char* strl;
  char* strr;
  int   i;     /* Loop iterator */

  strl = (char*)malloc_safe( (arc_get_width( arcs ) / 4) + 2 );
  strr = (char*)malloc_safe( (arc_get_width( arcs ) / 4) + 2 );

  for( i=0; i<arc_get_curr_size( arcs ); i++ ) {

    /* Check forward first */
    if( arc_get_entry_suppl( arcs, i, ARC_HIT_F ) == hit ) {
      arc_state_to_string( arcs, i, TRUE, strl );
      arc_state_to_string( arcs, i, FALSE, strr );
      fprintf( ofile, fstr, strl, strr );
    }

    if( arc_get_entry_suppl( arcs, i, ARC_HIT_R ) == hit ) {
      arc_state_to_string( arcs, i, TRUE,  strl );
      arc_state_to_string( arcs, i, FALSE, strr );
      fprintf( ofile, fstr, strr, strl );
    }

  }

}

/*!
 \param arcs  Pointer to state transition arc array.

 Deallocates all allocated memory for the specified state transition
 arc array.
*/
void arc_dealloc( char* arcs ) {

  if( arcs != NULL ) {
    free_safe( arcs );
  }

}

/*
 $Log$
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

