#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

/*!
 \file     symtable.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/3/2002
 \brief    Contains functions for manipulating a symtable structure.
*/

#include "defines.h"


//! Creates a new symtable entry and adds it to the specified symbol table.
void symtable_add( char* sym, signal* sig, symtable** symtab );

//! Finds specified symbol in specified symbol table and returns pointer to signal.
bool symtable_find( char* sym, symtable* symtab, signal** sig );

//! Deallocates all symtable entries for specified symbol table.
void symtable_dealloc( symtable* symtab );

#endif

