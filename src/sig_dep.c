/*!
 \file    sig_dep.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/5/2003
*/

#include <assert.h>

#include "defines.h"
#include "sig_dep.h"

/*!
 \param sig   Pointer to signal to populate dep_list array.
 \param root  Pointer to root of current statement tree.
 \param curr  Pointer to current statement that this signal is a dependent of.
 
 \return Returns TRUE if parent statement should add its dependents to this signal's dependency list.
 
 Recursively searches specified statement tree, searching for dependents and adding them
 to the current signal dependecy list.
*/
bool sig_dep_add_list( signal* sig, statement* root, statement* curr, statement** last ) {

  bool       ret_val = TRUE;  /* Return value of this function                          */
  bool       found_true;      /* Set to TRUE if curr statement found in true path       */
  sig_link*  dep;             /* Pointer to current dependency signal in root statement */
  statement* last_true;       /* Pointer to last statement in true path                 */

  assert( curr != NULL );

  if( root != NULL ) {

    if( SUPPL_IS_STMT_STOP( root->exp->suppl ) == 1 ) {

      *last   = root->next_true;
      ret_val = FALSE;

    } else {

      if( *last == root ) {
        *last = NULL;
      }

      if( root != curr ) {

        if( found_true = sig_dep_create( sig, root->next_true, curr, &last_true ) ) {
          dep = root->dep_head;
          while( dep != NULL ) {
            sig_link_add( dep->sig, &(sig->dep_head), &(sig->dep_tail) );
            dep = dep->next;
          }
        }

        if( !found_true && 
            (root->next_true != root->next_false) && 
            sig_dep_create( sig, root->next_false, curr, &last_true ) ) {
          dep = root->dep_head;
          while( dep != NULL ) {
            sig_link_add( dep->sig, &(sig->dep_head), &(sig->dep_tail) );
            dep = dep->next;
          }
        } else {
          ret_val = FALSE;
        }

      }

    }

  }

  return( retval );

}

/*
 $Log$
*/

