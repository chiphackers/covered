/*!
 \file     symtable.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2002
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>

#include "defines.h"
#include "symtable.h"
#include "util.h"
#include "signal.h"


/*!
 \param sym     VCD symbol for the specified signal.
 \param sig     Pointer to signal corresponding to the specified symbol.
 \param msb     Most significant bit of variable to set.
 \param lsb     Least significant bit of variable to set.
 \param symtab  Pointer to symtable to store the symtable entry to.

 \return Returns pointer to newly created symtable entry.

 Using the symbol as a unique ID, creates a new symtable element for specified information
 and places it into the binary tree.
*/
symtable* symtable_add( char* sym, signal* sig, int msb, int lsb, symtable** symtab ) {

  symtable* entry;    /* Pointer to new symtable entry           */
  symtable* curr;     /* Pointer to current symtable entry       */
  symtable* last;     /* Pointer to last symtable entry examined */

  /* Create new entry */
  entry        = (symtable*)malloc_safe( sizeof( symtable ) );
  entry->sym   = strdup( sym );
  entry->sig   = sig;
  entry->msb   = msb;
  entry->lsb   = lsb;
  entry->value = (char*)malloc_safe( sig->value->width + 1 );
  entry->size  = (sig->value->width + 1);
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
 \param sym  Symbol to be searched for.
 \param symtab  Symbol table to search for specified symbol.

 \return Returns pointer to found signal or NULL if symbol not found in table.

 Searches specified symbol table for an entry that matches the specified symbol.
 If the symbol is found, the signal pointer from this entry is returned; otherwise,
 if the symbol is not found, a value of NULL is returned.
*/
signal* symtable_find_signal( char* sym, symtable* symtab ) {

  symtable* curr;        /* Pointer to current symtable       */
  int       comp;        /* Specifies symbol comparison value */
  signal*   sig = NULL;  /* Pointer to found signal           */

  curr = symtab;
  while( (curr != NULL) && (sig == NULL) ) {
    comp = strcmp( sym, curr->sym );
    if( comp == 0 ) {
      sig = curr->sig;
    } else if( comp > 0 ) {
      curr = curr->right;
    } else {
      curr = curr->left;
    }
  }

  return( sig ); 

}

/*!
 \param sym     Name of symbol to find in the table.
 \param symtab  Root of the symtable to search in.
 \param value   Value to set symtable entry to when match found.

 \return Returns the number of matches that were found.

 Performs a binary search of the specified tree to find all matching symtable entries.
 When a match is found, the specified value is assigned to the symtable entry.
*/
int symtable_find_and_set( char* sym, symtable* symtab, char* value ) {

  symtable* curr;         /* Pointer to current symtable            */
  int       comp;         /* Specifies symbol comparison value      */
  int       matches = 0;  /* Counts number of times we have matched */

  curr = symtab;
  while( curr != NULL ) {
    comp = strcmp( sym, curr->sym );
    if( comp == 0 ) {
      assert( strlen( value ) < curr->size );     // Useful for debugging but not necessary
      strcpy( curr->value, value );
      matches++;
    }
    if( comp > 0 ) {
      curr = curr->right;
    } else {
      curr = curr->left;
    }
  }

  return( matches );

}

/*!
 \param sym       Symbol name to search for.
 \param from_tab  Table to pull symbol table information from.
 \param value     Value to assign to newly created table entry.
 \param to_tab    Reference to table to place new entry into.
 
 Performs a binary search of the from_tab symtable in search of all entries that match
 the specified sym parameter.  Whenever a match is found, a new entry is created and placed
 in the to_tab symtable containing the contents of the found entry.  Additionally, the
 specified value is assigned to the new entry.
*/
void symtable_move_and_set( char* sym, symtable* from_tab, char* value, symtable** to_tab ) {
  
  symtable* curr;    /* Pointer to current symtable           */
  symtable* newsym;  /* Pointer to newly created symtab entry */
  int       comp;    /* Specifies symbol comparison value     */
  
  curr = from_tab;
  while( curr != NULL ) {
    comp = strcmp( sym, curr->sym );
    if( comp == 0 ) {
      assert( curr->sig->value != NULL );
      newsym = symtable_add( sym, curr->sig, curr->msb, curr->lsb, to_tab );
      strcpy( newsym->value, value );
    }
    if( comp > 0 ) {
      curr = curr->right;
    } else {
      curr = curr->left;
    }
  }
  
}

/*!
 \param symtab  Root of the symtable to assign values to.

 Recursively traverses entire symtable tree, assigning stored string value to the
 stored signal.
*/
void symtable_assign( symtable* symtab ) {

  if( symtab != NULL ) {

    /* Assign current symbol table entry */
    signal_vcd_assign( symtab->sig, symtab->value, symtab->msb, symtab->lsb );

    /* Assign children */
    symtable_assign( symtab->right );
    symtable_assign( symtab->left );

  }

}

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
 Revision 1.10  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.9  2003/01/03 05:52:00  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.8  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.7  2002/10/31 23:14:29  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

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

