/*!
 \file     list.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/27/2006
*/

#include <stdlib.h>

#include "list.h"


/*!
 \param value  Value to add to the given list.
 \param l      Pointer to the list to add the given value to.

 If the specified list has not been allocated (its value is NULL), this function allocates memory for the
 list structure.  It then adds the given value to its list, allocating its own memory for the value.
*/
void list_add( char* value, list** l ) {

  printf( "HERE!?!\n" );
  if( *l == NULL ) {
    printf( "Allocating list\n" );
    *l = (list*)malloc( sizeof( list ) );
    (*l)->values = NULL;
  }

  printf( "HERE A\n" );
  (*l)->values = (char**)realloc( (*l)->values, (((*l)->num + 1) * sizeof( char* )) );
  printf( "HERE B\n" );
  (*l)->values[(*l)->num] = strdup( value );
  printf( "HERE C\n" );
  ((*l)->num)++;

}

/*!
 \param value  String to search for
 \param l      Pointer to list to search for given value.

 \return Returns 1 if the given value was found in the list; otherwise, returns 0.
*/
int list_find( char* value, list* l ) {

  int i = 0;  /* Loop iterator */

  printf( "In list_find...\n" );

  if( l != NULL ) {

    printf( "l: %p, l->num: %d\n", l, l->num );

    while( (i < l->num) && (strcmp( l->values[i], value ) != 0) ) {
      i++;
    }

    return( (i == l->num) ? 0 : 1 );

  } else {

    return( 0 );

  }

}

/*!
 \param l  Pointer to the list structure to display.

 Displays the contents of this list to standard output.
*/
void list_display( list* l ) {

  int i;  /* Loop iterator */

  if( l != NULL ) {

    for( i=0; i<l->num; i++ ) {
      printf( "%s\n", l->values[i] );
    }

  }

}

/*!
 \param l  Pointer to list to deallocate

 Completely deallocates all memory allocated for the given list pointer.
*/
void list_dealloc( list* l ) {

  int i;  /* Loop iterator */

  if( l != NULL ) {

    /* Deallocate all stored values */
    for( i=0; i<l->num; i++ ) {
      free( l->values[i] );
    }

    /* Deallocate the list array */
    free( l->values );

    /* Deallocate this list itself */
    free( l );

  } 

}

/*
 $Log$
*/

