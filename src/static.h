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

//! Calculates LSB and width for specified left/right pair for vector.
void static_expr_calc_lsb_and_width( static_expr* left, static_expr* right, int* width, int* lsb );

//! Deallocates static_expr memory from heap.
void static_expr_dealloc( static_expr* stexp, bool rm_exp );

/* $Log$
/* Revision 1.2  2002/10/11 04:24:02  phase1geo
/* This checkin represents some major code renovation in the score command to
/* fully accommodate parameter support.  All parameter support is in at this
/* point and the most commonly used parameter usages have been verified.  Some
/* bugs were fixed in handling default values of constants and expression tree
/* resizing has been optimized to its fullest.  Full regression has been
/* updated and passes.  Adding new diagnostics to test suite.  Fixed a few
/* problems in report outputting.
/*
/* Revision 1.1  2002/10/02 18:55:29  phase1geo
/* Adding static.c and static.h files for handling static expressions found by
/* the parser.  Initial versions which compile but have not been tested.
/* */

#endif

