/*!
 \file     fsm_sig.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/3/2003
*/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "defines.h"
#include "fsm_sig.h"
#include "util.h"


/*!
 \param fs_head  Pointer to head of FSM signal list to add to.
 \param fs_tail  Pointer to tail of FSM signal list to add to.
 \param name     Name of signal for new FSM signal.
 \param width    Width of signal for new FSM signal.
 \param lsb      Least-significant bit of signal for new FSM signal.

 Allocates and initializes new FSM signal with specified information and
 adds the new FSM signal to the list specified by fs_head and fs_tail.
*/
void fsm_sig_add( fsm_sig** fs_head, fsm_sig** fs_tail, char* name, int width, int lsb ) {

  fsm_sig* new_fs;  /* Pointer to newly created FSM signal element */

  /* Allocated and initialize new FSM signal */
  new_fs        = (fsm_sig*)malloc_safe( sizeof( fsm_sig ) );
  new_fs->name  = strdup( name );
  new_fs->width = width;
  new_fs->lsb   = lsb;
  new_fs->next  = NULL;

  /* Add new FSM signal to specified FSM variable */
  if( *fs_head == NULL ) {
    *fs_head = *fs_tail = new_fs;
  } else {
    (*fs_tail)->next = new_fs;
    *fs_tail         = new_fs;
  }

}

/*!
 \param fs_head  Pointer to head of FSM signal list to search.
 \param name     Name of signal to find.

 \return Returns pointer to found FSM signal or NULL if no match is found.

 Searches specified FSM signal list for element that matches the specified
 name string.  If a match is found, return a pointer to the matching FSM
 signal; otherwise, return a value of NULL.
*/
fsm_sig* fsm_sig_find( fsm_sig* fs_head, char* name ) {

  fsm_sig* curr;  /* Pointer to current FSM signal being evaluated */

  curr = fs_head;
  while( (curr != NULL) && (strcmp( curr->name, name ) != 0) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param fs  Pointer to FSM signal to deallocate.

 Deallocates all memory associated with the specified FSM signal.
*/
void fsm_sig_dealloc( fsm_sig* fs ) {

  if( fs != NULL ) {

    free_safe( fs->name );
    free_safe( fs );    

  }

}

/*!
 \param fs  Pointer to head of FSM signal list to deallocate.

 Deallocates entire FSM signal list associated with the specified argument.
*/
void fsm_sig_delete_list( fsm_sig* fs ) {

  if( fs != NULL ) {

    fsm_sig_delete_list( fs->next );
    fsm_sig_dealloc( fs );

  }

}

/*
 $Log$
*/

