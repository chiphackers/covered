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

  return( ((((width * 2) + 2) % 8) == 0) ? (((width * 2) + 2) / 8) : ((((width * 2) + 2) / 8) + 1) );

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

 \return Returns the value of the supplemental field of the specified
         arc array.

 Retrieves the value of the supplemental field in the specified arc
 array.
*/
int arc_get_suppl( char* arcs ) {

  return( (int)arcs[6] );

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

  bool   retval = TRUE;  /* Return value of this function       */
  char   mask;           /* Mask to apply to current bit select */
  vector value;          /* Vector to hold current bit select   */
  int    pos;            /* Current 8-bit boundary bit position */
  int    i;              /* Loop iterator                       */
  int    j;              /* Loop iterator                       */
  int    curr;           /* Current index of arc array to set   */

  /* Check specified vector for unknown information */
  if( vector_is_unknown( left ) || vector_is_unknown( right ) ) {

    retval = FALSE;

  } else {

    for( j=0; j<2; j++ ) {
  
      if( j == 0 ) {
        pos         = (arc_get_width( arcs ) + 3) % 8;
        curr        = start + ((arc_get_width( arcs ) + 3) / 8) + 7;
        value.width = (arc_get_width( arcs ) > (8 - pos)) ? (8 - pos) : arc_get_width( arcs );
        value.value = left->value;
      } else {
        pos          = 3;
        value.width  = (arc_get_width( arcs ) > 8) ? 8 : arc_get_width( arcs );
        curr         = start + 7;
        value.value  = right->value;
      }

      value.lsb = 0;
      while( value.lsb < arc_get_width( arcs ) ) {
        mask = 0;
        for( i=0; i<value.width; i++ ) {
          mask <<= 1;
          mask  |= (0x1 << pos);
        }
        arcs[curr] |= ((vector_to_int( &value ) << pos) & mask);
        value.lsb  += value.width;
        value.width = ((arc_get_width( arcs ) - value.lsb) > 8) ? 8 : (arc_get_width( arcs ) - value.lsb);
        pos         = 0;
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

  arcs[curr + 7] = arcs[curr + 7] | ((val & 0x1) << type);

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

  return( (int)((arcs[curr + 7] >> type) & 0x1) );

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
  arc_set_width( tmp, 1 );

  i = 0;
  while( (i < 2) && (*ptr == -1) ) {
    if( i == 0 ) {
      /* Create from/to combination */
      if( arc_set_states( tmp, 0, from_st, to_st ) ) {
        j = 0;
        while( (j < curr_size) && (*ptr == -1) ) {
          k = 0;
          while( (k < entry_size) &&
                 (((k == 0) && ((arcs[(((j * entry_size) + k) + 7)] & 0xf8) == (tmp[k + 7] & 0xf8))) ||
                  ((k != 0) &&  (arcs[(((j * entry_size) + k) + 7)]         ==  tmp[k + 7]))) ) {
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
          if( (type == 1) || ((type == 0) && (arc_get_entry_suppl( arcs, (j * entry_size), ARC_BIDIR ) == 1)) ) {
            k = 0;
            while( (k < entry_size) && 
                   (((k == 0) && ((arcs[(((j * entry_size) + k) + 7)] & 0xf8) == (tmp[k + 7] & 0xf8))) ||
                    ((k != 0) &&  (arcs[(((j * entry_size) + k) + 7)]         ==  tmp[k + 7]))) ) {
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

  return( i );

}

/*!
 \param width  Width of arc array to create.

 \return Returns a pointer to the newly created arc transition structure.

 Allocates memory for a new state transition arc array and initializes its
 contents.
*/
char* arc_create( int width ) {

  char* arcs;  /* Pointer to newly create state transition array   */

  /* The arcs char array is not allocated, allocate the default space here */
  arcs = (char*)malloc_safe( (arc_get_entry_width( width ) * width) + 7 );

  /* Initialize */
  arc_set_width( arcs, width );     /* Signal width                           */
  arc_set_max_size( arcs, width );  /* Number of entries in current arc array */
  arc_set_curr_size( arcs, 0 );     /* Current entry pointer to store new     */

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
    *arcs = (char*)malloc_safe( (entry_width * arc_get_width( tmp )) + arc_get_width( tmp ) + 7 );

    arc_set_width( *arcs, arc_get_width( tmp ) );
    arc_set_max_size( *arcs, arc_get_max_size( tmp ) + arc_get_width( tmp ) );
    arc_set_curr_size( *arcs, arc_get_curr_size( tmp ) );
    for( i=7; i<((arc_get_max_size( tmp ) * entry_width) + 7); i++ ) {
      (*arcs)[i] = tmp[i];
    }

    /* Deallocate old memory */
    free_safe( tmp );

  }

  if( side == 2 ) {

    /* Initialize arc entry */
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

int arc_state_hit_total( char* arcs ) {

  int hit = 0;     /* Number of states hit */
  int i;           /* Loop iterator        */

  return( hit );

}

int arc_transition_hit_total( char* arcs ) {

  int hit = 0;     /* Number of arcs hit                          */
  int i;           /* Loop iterator                               */
  int curr_size;   /* Current size of arc array                   */
  int entry_size;  /* Number of characters that contain one entry */

  /* Get some size values */
  curr_size  = arc_get_curr_size( arcs );
  entry_size = arc_get_entry_width( arc_get_width( arcs ) );

  /* Count the number of hits in the FSM arc */
  for( i=0; i<curr_size; i++ ) {
    hit += arc_get_entry_suppl( arcs, (i * entry_size), ARC_HIT_F );
    hit += arc_get_entry_suppl( arcs, (i * entry_size), ARC_HIT_R );
  }

  return( hit );

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

  for( i=0; i<(arc_get_curr_size( arcs ) * arc_get_entry_width( arc_get_width( arcs ) )) + 7; i++ ) {
    if( (int)arcs[i] == 0 ) {
      fprintf( file, "," );
    } else {
      fprintf( file, "%02x", (int)arcs[i] );
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
  *arcs = (char*)malloc_safe( (arc_get_entry_width( width ) * curr_size) + 7 );

  /* Initialize */
  arc_set_width( *arcs, width );
  arc_set_max_size( *arcs, curr_size );
  arc_set_curr_size( *arcs, curr_size );
  arc_set_suppl( *arcs, suppl );

  /* Read in rest of values */ 
  i = 7;
  while( (i < ((curr_size * arc_get_entry_width( width )) + 7)) && retval ) {
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
 \param base  Pointer to arc table to merge data into.
 \param line  Pointer to read in line from CDD file to merge.
 \param same  Specifies if arc table to merge needs to be exactly the same as the existing arc table.

 \return Returns TRUE if line was read in correctly; otherwise, returns FALSE.

*/
bool arc_db_merge( char* base, char** line, bool same ) {

  bool retval = TRUE;  /* Return value for this function */
  int  width;          /* State variable width of table  */
  int  max_size;       /* Largest size of table          */
  int  curr_size;      /* Size of arc table              */
  int  i;              /* Loop iterator                  */
  int  val;            /* Value of current character     */
  int  entry_size;     /* Character size of arc entry    */
  int  suppl;          /* Supplemental field value       */

  /* Get sizing information */
  width     =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  max_size  =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  curr_size =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  suppl     =  (arc_read_get_next_value( line ) & 0xff);

  if( same ) {
    if( width != arc_get_width( base ) ) {
      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
      exit( 1 );
    }
    if( curr_size != arc_get_curr_size( base ) ) {
      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
      exit( 1 );
    }
  }

  entry_size = arc_get_entry_width( width );
  i          = 7;
  while( (i < ((curr_size * arc_get_entry_width( width )) + 7)) && retval ) {
    if( (val = arc_read_get_next_value( line )) != -1 ) {
      if( same ) {
        if( ((((i - 7) % entry_size) == 0) && ((char)(val & 0xfc) != (base[i] & 0xfc))) || ((char)val != base[i]) ) {
          print_output( "Attempting to merge two databases derived from different designs.  Unable to merge", FATAL );
          exit( 1 );
        }
      }
      base[i] = base[i] | (char)(val & 0xff);
    } else {
      retval = FALSE;
    }
    i++;
  }

  return( retval );

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
 Revision 1.3  2003/08/29 12:52:06  phase1geo
 Updating comments for functions.

 Revision 1.2  2003/08/26 21:53:23  phase1geo
 Added database read/write functions and fixed problems with other arc functions.

 Revision 1.1  2003/08/26 12:53:35  phase1geo
 Added initial versions of arc.c and arc.h though they are not even close to
 being finished at this point.

*/

