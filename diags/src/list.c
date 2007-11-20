/*!
 \file     list.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/27/2006
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"


/*!
 \param str  String value to assign to this list
 \param num  Numerical value to assign to this list
 \param l    Pointer to the list to add the given value to.

 If the specified list has not been allocated (its value is NULL), this function allocates memory for the
 list structure.  It then adds the given value to its list, allocating its own memory for the value.
*/
void list_add( char* str, int num, list** l ) {

  list_elem* elem;  /* Pointer to newly created list element */

  if( *l == NULL ) {
    *l = (list*)malloc( sizeof( list ) );
    (*l)->elems = NULL;
    (*l)->size  = 0;
  }

  /* Create new element */
  elem      = (list_elem*)malloc( sizeof( list_elem ) );
  elem->str = strdup( str );
  elem->num = num;

  (*l)->elems = (list_elem**)realloc( (*l)->elems, (((*l)->size + 1) * sizeof( list_elem* )) );
  (*l)->elems[(*l)->size] = elem;
  ((*l)->size)++;

}

void list_set_str( char* value, int index, list* l ) {

  if( (l != NULL) && (index >= 0) && (index < l->size) ) {

    if( l->elems[index]->str != NULL ) {
      free( l->elems[index]->str );
    }

    l->elems[index]->str = strdup( value );

  }

}

void list_set_num( int value, int index, list* l ) {

  if( (l != NULL) && (index >= 0) && (index < l->size) ) {

    l->elems[index]->num = value;

  }

}

char* list_get_str( int index, list* l ) {

  if( (l != NULL) && (index >= 0) && (index < l->size) ) {

    return( l->elems[index]->str );

  } else {

    return( NULL );

  }

}

/*!
 \param index  Index of element in given list to get numerical value of
 \param l      Pointer to list to retrieve numerical value from

 \return Returns the numerical value stored in the list element at the given index.
*/
int list_get_num( int index, list* l ) {

  if( (l != NULL) && (index >= 0) && (index < l->size) ) {

    return( l->elems[index]->num );

  } else {

    return( -1 );

  }

}

/*!
 \param l  Pointer to list to get size of.

 \return Returns the number of elements stored in the specified list.
*/
int list_get_size( list* l ) {

  return( l->size );

}

/*!
 \param value  String to search for
 \param l      Pointer to list to search for given value.

 \return Returns the index of the found list element; otherwise, returns -1.
*/
int list_find_str( char* value, list* l ) {

  int i = 0;  /* Loop iterator */

  if( l != NULL ) {

    while( (i < l->size) && (strcmp( l->elems[i]->str, value ) != 0) ) {
      i++;
    }

    return( (i == l->size) ? -1 : i );

  } else {

    return( -1 );

  }

}

/*!
 \param value  String to search for
 \param l      Pointer to list to search for given value.

 \return Returns the index of the found list element; otherwise, returns -1.
*/
int list_find_num( int value, list* l ) {

  int i = 0;  /* Loop iterator */

  if( l != NULL ) {

    while( (i < l->size) && (l->elems[i]->num != value) ) {
      i++;
    }

    return( (i == l->size) ? -1 : i );

  } else {

    return( -1 );

  }

}

/*!
 \param l  Pointer to the list structure to display.

 Displays the contents of this list to standard output.
*/
void list_display( list* l ) {

  int i;  /* Loop iterator */

  if( l != NULL ) {

    for( i=0; i<l->size; i++ ) {
      printf( "    %s\n", l->elems[i]->str );
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

    /* Deallocate all stored elems */
    for( i=0; i<l->size; i++ ) {

      /* Deallocate the string */
      if( l->elems[i]->str != NULL ) {
        free( l->elems[i]->str );
      }

      free( l->elems[i] );

    }

    /* Deallocate the list array */
    free( l->elems );

    /* Deallocate this list itself */
    free( l );

  } 

}

/*
 $Log$
 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/

