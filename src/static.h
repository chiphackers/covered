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
static_expr* static_expr_gen( static_expr* left, static_expr* right, int op, int line );

/* $Log$ */

#endif

