#ifndef __EXPR_H__
#define __EXPR_H__

/*!
 \file     expr.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains functions for handling expressions.
*/

#include <stdio.h>

#include "defines.h"


//! Creates an expression value and initializes it.
void expression_create_value( expression* exp, int width, int lsb );

//! Creates new expression.
expression* expression_create( expression* right, expression* left, int op, int id, int line );

//! Merges two expressions and stores result in base expression.
void expression_merge( expression* base, expression* in );

//! Returns expression ID of this expression.
int expression_get_id( expression* expr );

//! Writes this expression to the specified database file.
void expression_db_write( expression* expr, FILE* file, char* scope );

//! Reads current line of specified file and parses for expression information.
bool expression_db_read( char** file, module* curr_mod );

//! Displays the specified expression information.
void expression_display( expression* expr );

//! Performs operation specified by parameter expression.
void expression_operate( expression* expr );

//! Returns a compressed, 1-bit representation of the value after a unary OR.
int expression_bit_value( expression* expr );

//! Deallocates memory used for expression.
void expression_dealloc( expression* expr, bool exp_only );


/* $Log$
/* Revision 1.7  2002/06/28 03:04:59  phase1geo
/* Fixing more errors found by diagnostics.  Things are running pretty well at
/* this point with current diagnostics.  Still some report output problems.
/*
/* Revision 1.6  2002/06/25 03:39:03  phase1geo
/* Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
/* Fixed some report bugs though there are still some remaining.
/*
/* Revision 1.5  2002/06/21 05:55:05  phase1geo
/* Getting some codes ready for writing simulation engine.  We should be set
/* now.
/*
/* Revision 1.4  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/*
/* Revision 1.3  2002/05/03 03:39:36  phase1geo
/* Removing all syntax errors due to addition of statements.  Added more statement
/* support code.  Still have a ways to go before we can try anything.  Removed lines
/* from expressions though we may want to consider putting these back for reporting
/* purposes.
/* */

#endif

