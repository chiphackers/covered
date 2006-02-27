#ifndef __LIST_H__
#define __LIST_H__

/*!
 \file     list.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     2/27/2006
 \brief    Contains functions for manipulating a string list.
*/

/*!
 String list structure.
*/
typedef struct list_s {
  char** values;            /*!< Array of string values */
  int    num;               /*!< Number of elements in the values array */
} list;


/*! \brief Adds specified element to the given list. */
void list_add( char* value, list** l );

/*! \brief Searches the given list for the given value. */
int list_find( char* value, list* l );

/*! \brief Displays the specified list to standard output. */
void list_display( list* l );

/*! \brief Deallocates all memory associated with the given list. */
void list_dealloc( list* l );


/*
 $Log$
*/

#endif

