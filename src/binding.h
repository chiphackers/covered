#ifndef __BINDING_H__
#define __BINDING_H__

/*!
 \file     binding.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/4/2002
 \brief    Contains all functions for signal/expression binding.
*/

#include "signal.h"
#include "expr.h"
#include "module.h"

//! Adds signal and expression to binding list.
void bind_add( char* sig_name, expression* exp, char* mod_name );

//! Removes the expression with ID of id from binding list.
void bind_remove( int id );

//! Finds signal in module and bind the expression to this signal.
void bind_perform( char* sig_name, expression* exp, module* mod, bool implicit_allowed );

//! Performs signal/expression bind (performed after parse completed).
void bind();

/* $Log$
/* Revision 1.3  2002/07/18 22:02:35  phase1geo
/* In the middle of making improvements/fixes to the expression/signal
/* binding phase.
/* */

#endif

