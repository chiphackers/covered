#ifndef __STATEMENT_H__
#define __STATEMENT_H__

/*!
 \file     statement.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     5/1/2002
 \brief    Contains functions to create, manipulate and deallocate statements.
*/

#include "defines.h"
#include "expr.h"


//! Creates new statement structure.
statement* statement_create( expression* exp, int line_begin, int line_end );

//! Writes specified statement to the specified output file.
void statement_db_write( statement* stmt, FILE* ofile, char* scope );

//! Reads in statement line from specified string and stores statement in specified module.
bool statement_db_read( char** line, module* curr_mod );

//! Connects statement sequence to form a loop.
void statement_loopback( statement* stmt );

//! Deallocates statement memory and associated expression tree from the heap.
void statement_dealloc( statement* stmt );


/* $Log$
/* Revision 1.1  2002/05/02 03:27:42  phase1geo
/* Initial creation of statement structure and manipulation files.  Internals are
/* still in a chaotic state.
/* */

#endif
