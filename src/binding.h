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

//! Performs signal/expression bind (performed after parse completed).
void bind( int mode );

#endif

