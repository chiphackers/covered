/*!
 \file     arc.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     8/25/2003
*/

#include <stdlib.h>

#include "defines.h"
#include "arc.h"
#include "util.h"
#include "vector.h"
#include "expr.h"

void arc_set_width( char* arcs, int width ) {

  arcs[0] = (char)(width & 0xff);
  arcs[1] = (char)((width >> 8) & 0xff);

}

int arc_get_width( char* arcs ) {

  int width = (((int)arcs[1] & 0xff) << 8) | ((int)arcs[0] & 0xff);
  return( width );

}

void arc_set_max_size( char* arcs, int size ) {

  arcs[2] = (char)(size & 0xff);
  arcs[3] = (char)((size >> 8) & 0xff);

}

int arc_get_max_size( char* arcs ) {

  int size = (((int)arcs[3] & 0xff) << 8) | ((int)arcs[2] & 0xff);
  return( size );

}

void arc_set_curr_size( char* arcs, int size ) {

  arcs[4] = (char)(size & 0xff);
  arcs[5] = (char)((size >> 8) & 0xff);

}

int arc_get_curr_size( char* arcs ) {

  int size = (((int)(arcs[5]) & 0xff) << 8) | ((int)(arcs[4]) & 0xff);
  return( size );

}

void arc_set_value( char* arcs, int curr, bool right, vector* vec ) {

  char   mask;   /* Mask to apply to current bit select            */
  vector value;  /* Vector to hold current bit select              */
  int    pos;    /* Current 8-bit boundary bit position            */
  int    bits;   /* Number of bits to select from vector           */
  int    i;      /* Loop iterator                                  */
  int    lsb;    /* Least significant bit of vector to select from */

  if( right ) {
    pos  = 0;
    bits = (arc_get_width( arcs ) > 8) ? 8 : arc_get_width( arcs );
  } else {
    pos   = (arc_get_width( arcs ) + 1) % 8;
    curr += (arc_get_width( arcs ) + 1) / 8;
    bits  = (arc_get_width( arcs ) > (8 - pos)) ? (8 - pos) : arc_get_width( arcs );
  }

  lsb = 0;
  while( lsb < arc_get_width( arcs ) ) {
    mask = 0;
    for( i=0; i<bits; i++ ) {
      mask <<= 1;
      mask |= (0x1 << pos);
    }
    vector_init( &value, vec->value, bits, lsb );
    arcs[curr] |= ((vector_to_int( &value ) << pos) & mask);
    lsb += bits;
    bits = ((arc_get_width( arcs ) - lsb) > 8) ? 8 : (arc_get_width( arcs ) - lsb);
    pos  = 0;
    curr++;
  }

}

void arc_set_hit( char* arcs, int curr, bool right, char val ) {

  if( right ) {
    arcs[curr + (arc_get_width( arcs ) / 8)] = 
        (arcs[curr + (arc_get_width( arcs ) / 8)] & ~(0x1 << (arc_get_width( arcs ) % 8))) |
        ((val & 0x1) << (arc_get_width( arcs ) % 8));
  } else {
    arcs[curr + (((arc_get_width( arcs ) * 2) + 1) / 8)] = 
        (arcs[curr + (((arc_get_width( arcs ) * 2) + 1) / 8)] & 
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
    entry_width = ((((width * 2) + 2) % 8) == 0) ? (((width * 2) + 2) / 8) : 
                                                  ((((width * 2) + 2) / 8) + 1);

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
    entry_width = ((((arc_get_width( tmp ) * 2) + 2) % 8) == 0) ? 
                           (((arc_get_width( tmp ) * 2) + 2) / 8) : 
                          ((((arc_get_width( tmp ) * 2) + 2) / 8) + 1);

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
    
  /* Initialize left value */
  arc_set_value( *arcs, arc_get_curr_size( *arcs ), FALSE, fr_st->value );
  arc_set_hit(   *arcs, arc_get_curr_size( *arcs ), FALSE, 0 );

  /* Initialize right value */
  arc_set_value( *arcs, arc_get_curr_size( *arcs ), TRUE, to_st->value );
  arc_set_hit(   *arcs, arc_get_curr_size( *arcs ), TRUE, 0 );

  arc_set_curr_size( *arcs, (arc_get_curr_size( *arcs ) + 1) );

}

int main() {

  char*       test  = NULL;
  expression* fr_st = expression_create( NULL, NULL, EXP_OP_STATIC, 0, 0, FALSE );
  expression* to_st = expression_create( NULL, NULL, EXP_OP_STATIC, 0, 0, FALSE );

  vector_from_int( fr_st->value, 0 );
  vector_from_int( to_st->value, 1 );

  arc_add( &test, 3, fr_st, to_st );

}

/*
 $Log$
*/

