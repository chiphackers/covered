/*!
 \file     symtable.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2002
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "defines.h"
#include "symtable.h"
#include "util.h"
#include "signal.h"


/*!
 \param sym     VCD symbol for the specified signal.
 \param sig     Pointer to signal corresponding to the specified symbol.
 \param symtab  Pointer to symtable to store the symtable entry to.

 \return Returns pointer to newly created symtable entry.

 Using the symbol as a unique ID, creates a new symtable element for specified information
 and places it into the binary tree.
*/
symtable* symtable_add( char* sym, signal* sig, symtable** symtab ) {

  symtable* entry;    /* Pointer to new symtable entry           */
  symtable* curr;     /* Pointer to current symtable entry       */
  symtable* last;     /* Pointer to last symtable entry examined */

  /* Create new entry */
  entry        = (symtable*)malloc_safe( sizeof( symtable ) );
  entry->sym   = strdup( sym );
  entry->sig   = sig;
  entry->value = (char*)malloc_safe( sig->value->width + 1 );
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

  assert( entry->sig->value != NULL );

  return( entry );

}

/*!
 \param sym     Name of symbol to find in the table.
 \param symtab  Root of the symtable to search in.
 \param skip    Number of matching symbol values to skip.

 \return Returns pointer to found symtable entry or NULL if unable to find.

 Performs a binary search of the specified tree to find matching symtable entry.  If
 a match is found,returns a pointer to the found symtable entry; otherwise, returns
 a value of NULL to indicate that the symtable entry was not found.
*/
symtable* symtable_find( char* sym, symtable* symtab, int skip ) {

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

  return( curr );

}

/*!
 \param symtab  Root of the symtable to assign values to.

 Recursively traverses entire symtable tree, assigning stored string value to the
 stored signal.
*/
void symtable_assign( symtable* symtab ) {

  if( symtab != NULL ) {

    /* Assign current symbol table entry */
    signal_vcd_assign( symtab->sig, symtab->value );

    /* Assign children */
    symtable_assign( symtab->right );
    symtable_assign( symtab->left );

  }

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

/*
 $Log$
 Revision 1.5  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.4  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.3  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

