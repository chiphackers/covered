/*!
 \file     symtable.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2002
*/

#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "symtable.h"
#include "util.h"


/*!
 \param sym     VCD symbol for the specified signal.
 \param sig     Pointer to signal corresponding to the specified symbol.
 \param symtab  Pointer to symtable to store the symtable entry to.

 Using the symbol as a unique ID, creates a new symtable element for specified information
 and places it into the binary tree.
*/
void symtable_add( char* sym, signal* sig, symtable** symtab ) {

  symtable* entry;    /* Pointer to new symtable entry           */
  symtable* curr;     /* Pointer to current symtable entry       */
  symtable* last;     /* Pointer to last symtable entry examined */

  /* Create new entry */
  entry        = (symtable*)malloc_safe( sizeof( symtable ) );
  entry->sym   = strdup( sym );
  entry->sig   = sig;
  entry->right = NULL;
  entry->left  = NULL;

  curr = *symtab;
  last = NULL;

  /* Parse tree evaluating current sym name to the new sym name */
  while( curr != NULL ) {
    last = curr;
    if( strcmp( sym, curr->sym ) > 0 ) {
      curr = curr->right;
    } else { 
      curr = curr->left;
    }
  }

  /* Place the new entry into the tree */
  if( last == NULL ) {
    /* symtab has no entries */
    *symtab = entry;
  } else {
    if( strcmp( sym, last->sym ) > 0 ) {
      last->right = entry;
    } else {
      last->left  = entry;
    }
  }

}

/*!
 \param sym     Name of symbol to find in the table.
 \param symtab  Root of the symtable to search in.
 \param sig     Pointer to found signal.
 \param skip    Number of matching symbol values to skip.

 \return Returns TRUE if symbol was found; otherwise, returns FALSE.

 Performs a binary search of the specified tree to find matching symtable entry.  If
 a match is found, assigns sig parameter to the signal represented by the symbol and
 returns TRUE; otherwise, simply returns FALSE.
*/
bool symtable_find( char* sym, symtable* symtab, signal** sig, int skip ) {

  symtable* curr;         /* Pointer to current symtable            */
  bool      unmatched;    /* If TRUE, current symbol does not match */
  int       skipped = 0;  /* Number of matching symbols skipped     */

  curr = symtab;
  while( (curr != NULL) && ((unmatched = (strcmp( sym, curr->sym ) != 0)) || (skipped < skip)) ) {
    if( !unmatched ) { skipped++; }
    if( strcmp( sym, curr->sym ) > 0 ) {
      curr = curr->right;
    } else {
      curr = curr->left;
    }
  }

  if( curr == NULL ) {
    *sig = NULL;
  } else {
    *sig = curr->sig;
  }

  return( curr != NULL );

}

//! Deallocates all symtable entries for specified symbol table.
/*!
 \param symtab  Pointer to root of symtable to clear.

 Recursively deallocates all elements of specifies symbol table.
*/ 
void symtable_dealloc( symtable* symtab ) {

  if( symtab != NULL ) {

    symtable_dealloc( symtab->right );
    symtable_dealloc( symtab->left  );
 
    if( symtab->sym != NULL ) {
      free_safe( symtab->sym );
    }

    free_safe( symtab );

  }

}

