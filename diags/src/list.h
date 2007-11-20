#ifndef __LIST_H__
#define __LIST_H__

/*!
 \file     list.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/27/2006
 \brief    Contains functions for manipulating a string list.
*/

/*!
 List element structure.
*/
typedef struct list_elem_s {
  char* str;           /*!< String value */
  int   num;           /*!< Integer value */
} list_elem;

/*!
 String list structure.
*/
typedef struct list_s {
  list_elem** elems;   /*!< Array of string values */
  int         size;    /*!< Number of elements in the values array */
} list;


/*! \brief Adds specified element to the given list. */
void list_add( char* str, int num, list** l );

void list_set_str( char* value, int index, list* l );

void list_set_num( int value, int index, list* l );

char* list_get_str( int index, list* l );

int list_get_num( int index, list* l );

/*! \brief Returns the number of elements stored in the given list. */
int list_get_size( list* l );

/*! \brief Searches the given list for the given value. */
int list_find_str( char* value, list* l );

/*! \brief Searches the given list for the given value. */
int list_find_num( int value, list* l );

/*! \brief Displays the specified list to standard output. */
void list_display( list* l );

/*! \brief Deallocates all memory associated with the given list. */
void list_dealloc( list* l );


/*
 $Log$
 Revision 1.2  2006/03/03 23:24:53  phase1geo
 Fixing C-based run script.  This is now working for all but one diagnostic to this
 point.  There is still some work to do here, however.

 Revision 1.1  2006/02/27 23:22:10  phase1geo
 Working on C-version of run command.  Initial version only -- does not work
 at this point.

*/

#endif

