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

//! Deallocates memory used for expression.
void expression_dealloc( expression* expr, bool exp_only );

#endif

