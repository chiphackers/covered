#ifndef __STATIC_H__
#define __STATIC_H__

/*!
 \file     static.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/02/2002
 \brief    Contains functions for handling creation of static expressions.
*/

#include "defines.h"


//! Calculates new values for unary static expressions and returns the new static expression.
static_expr* static_expr_gen_unary( static_expr* stexp, int op, int line );

//! Calculates new values for static expression and returns the new static expression.
static_expr* static_expr_gen( static_expr* right, static_expr* left, int op, int line );

//! Deallocates static_expr memory from heap.
void static_expr_dealloc( static_expr* stexp, bool rm_exp );

/* $Log$
/* Revision 1.1  2002/10/02 18:55:29  phase1geo
/* Adding static.c and static.h files for handling static expressions found by
/* the parser.  Initial versions which compile but have not been tested.
/* */

#endif

