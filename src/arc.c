/*!
 \file     arc.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     8/25/2003
*/

#include <stdlib.h>
#include <stdio.h>

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
        pos         = (arc_get_width( arcs ) + 1) % 8;
        curr        = start + ((arc_get_width( arcs ) + 1) / 8) + 6;
        value.width = (arc_get_width( arcs ) > (8 - pos)) ? (8 - pos) : arc_get_width( arcs );
        value.value = left->value;
      } else {
        pos          = 0;
        value.width  = (arc_get_width( arcs ) > 8) ? 8 : arc_get_width( arcs );
        curr         = start + 6;
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

void arc_set_hit( char* arcs, int curr, bool right, char val ) {

  if( right ) {
    arcs[curr + 6 + (arc_get_width( arcs ) / 8)] = 
        (arcs[curr + 6 + (arc_get_width( arcs ) / 8)] & ~(0x1 << (arc_get_width( arcs ) % 8))) |
        ((val & 0x1) << (arc_get_width( arcs ) % 8));
  } else {
    arcs[curr + 6 + (((arc_get_width( arcs ) * 2) + 1) / 8)] = 
        (arcs[curr + 6 + (((arc_get_width( arcs ) * 2) + 1) / 8)] & 
          ~(0x1 << (((arc_get_width( arcs ) * 2) + 1) / 8))) |
        ((val & 0x1) << (((arc_get_width( arcs ) * 2) + 1) % 8));
  }

}

void arc_add( char** arcs, int width, expression* fr_st, expression* to_st ) {

  char* tmp;          /* Temporary char array holder            */
  int   entry_width;  /* Width of a signal entry in the arc array */
  int   i;            /* Loop iterator                            */

  if( *arcs == NULL ) {
    
    /* Calculate the entry width */
    entry_width = arc_get_entry_width( width );

    /* The arcs char array is not allocated, allocate the default space here */
    *arcs = (char*)malloc_safe( (entry_width * width) + 6 );

    /* Initialize */
    arc_set_width( *arcs, width );     /* Signal width                           */
    arc_set_max_size( *arcs, width );  /* Number of entries in current arc array */
    arc_set_curr_size( *arcs, 0 );     /* Current entry pointer to store new     */

  } else if( (*arcs)[1] == (*arcs)[2] ) {

    /* We have maxed out the array, reallocate */
    tmp = *arcs;

    /* Calculate the entry width */
    entry_width = arc_get_entry_width( arc_get_width( tmp ) );

    /* Allocate new memory */
    *arcs = (char*)malloc_safe( (entry_width * arc_get_width( tmp )) + arc_get_width( tmp ) + 6 );

    arc_set_width( *arcs, arc_get_width( tmp ) );
    arc_set_max_size( *arcs, arc_get_max_size( tmp ) + arc_get_width( tmp ) );
    arc_set_curr_size( *arcs, arc_get_curr_size( tmp ) );
    for( i=6; i<((arc_get_max_size( tmp ) * entry_width) + 6); i++ ) {
      (*arcs)[i] = tmp[i];
    }

    /* Deallocate old memory */
    free_safe( tmp );

  }
    
  /* Initialize arc entry */
  arc_set_states( *arcs, arc_get_curr_size( *arcs ), fr_st->value, to_st->value );
  arc_set_hit(   *arcs, arc_get_curr_size( *arcs ), FALSE, 0 );
  arc_set_hit(   *arcs, arc_get_curr_size( *arcs ), TRUE,  0 );

  arc_set_curr_size( *arcs, (arc_get_curr_size( *arcs ) + 1) );

}

bool arc_db_write( char* arcs, FILE* file ) {

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator                  */

  for( i=0; i<(arc_get_curr_size( arcs ) * arc_get_entry_width( arc_get_width( arcs ) )) + 6; i++ ) {
    if( (int)arcs[i] == 0 ) {
      fprintf( file, "," );
    } else {
      fprintf( file, "%02x", (int)arcs[i] );
    }
  }

  return( retval );

}

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

bool arc_db_read( char** arcs, char** line ) {

  bool retval = TRUE;  /* Return value for this function */
  int  i;              /* Loop iterator                  */
  int  val;            /* Current character value        */
  int  width;          /* Arc signal width               */
  int  max_size;       /* Size of arc array              */
  int  curr_size;      /* Current size of arc array      */

  /* Get sizing information */
  width     =  (arc_read_get_next_value( line ) & 0xff) | 
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  max_size  =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);
  curr_size =  (arc_read_get_next_value( line ) & 0xff) |
              ((arc_read_get_next_value( line ) & 0xff) << 8);

  /* Allocate memory */
  *arcs = (char*)malloc_safe( (arc_get_entry_width( width ) * curr_size) + 6 );

  /* Initialize */
  arc_set_width( *arcs, width );
  arc_set_max_size( *arcs, curr_size );
  arc_set_curr_size( *arcs, curr_size );

  /* Read in rest of values */ 
  i = 6;
  while( (i < ((curr_size * arc_get_entry_width( width )) + 6)) && retval ) {
    if( (val = arc_read_get_next_value( line )) != -1 ) {
      (*arcs)[i] = (char)(val & 0xff);
    } else {
      retval = FALSE;
    }
    i++;
  }

  return( retval );

}

int main() {

  char*       test  = NULL;
  expression* fr_st = expression_create( NULL, NULL, EXP_OP_STATIC, 0, 0, FALSE );
  expression* to_st = expression_create( NULL, NULL, EXP_OP_STATIC, 0, 0, FALSE );
  vector*     vec1  = vector_create( 32, 0, TRUE );
  vector*     vec2  = vector_create( 32, 0, TRUE );
  int         i;
  FILE*       handle;
  char*       read = NULL;
  char*       curr_line;

  vector_from_int( vec1, 1 );
  free_safe( fr_st->value );
  fr_st->value = vec1;

  vector_from_int( vec2, 2 );
  free_safe( to_st->value );
  to_st->value = vec2;

  arc_add( &test, 9, fr_st, to_st );

  if( (handle = fopen( "foo", "w" )) != NULL ) {
    arc_db_write( test, handle );
    fprintf( handle, "\n" );
    fclose( handle );
  }
  if( (handle = fopen( "foo", "r" )) != NULL ) {
    readline( handle, &curr_line );
    arc_db_read( &read, &curr_line );
    fclose( handle );
  }
  arc_db_write( test, stdout );
  printf( "\n" );

}

/*
 $Log$
 Revision 1.1  2003/08/26 12:53:35  phase1geo
 Added initial versions of arc.c and arc.h though they are not even close to
 being finished at this point.

*/

