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
statement* statement_create( expression* exp );

//! Writes specified statement to the specified output file.
void statement_db_write( statement* stmt, FILE* ofile, char* scope );

//! Reads in statement line from specified string and stores statement in specified module.
bool statement_db_read( char** line, module* curr_mod, int read_mode );

//! Connects statement sequence to next statement.
void statement_connect( statement* curr_stmt, statement* next_stmt, bool set_stop );

//! Deallocates statement memory and associated expression tree from the heap.
void statement_dealloc( statement* stmt );


/* $Log$
/* Revision 1.5  2002/06/27 12:36:47  phase1geo
/* Fixing bugs with scoring.  I think I got it this time.
/*
/* Revision 1.4  2002/06/24 04:54:48  phase1geo
/* More fixes and code additions to make statements work properly.  Still not
/* there at this point.
/*
/* Revision 1.3  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/*
/* Revision 1.2  2002/05/03 03:39:36  phase1geo
/* Removing all syntax errors due to addition of statements.  Added more statement
/* support code.  Still have a ways to go before we can try anything.  Removed lines
/* from expressions though we may want to consider putting these back for reporting
/* purposes.
/*
/* Revision 1.1  2002/05/02 03:27:42  phase1geo
/* Initial creation of statement structure and manipulation files.  Internals are
/* still in a chaotic state.
/* */

#endif
