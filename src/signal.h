#ifndef __SIGNAL_H__
#define __SIGNAL_H__

/*!
 \file     signal.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains functions for handling signals.
*/

#include <stdio.h>

#include "defines.h"


//! Initializes specified signal with specified values.
void signal_init( signal* sig, char* name, vector* value );

//! Creates a new signal based on the information passed to this function.
signal* signal_create( char* name, int width, int lsb, int is_static );

//! Merges two signals, placing result into base signal.
void signal_merge( signal* base, signal* in );

//! Outputs this signal information to specified file.
void signal_db_write( signal* sig, FILE* file, char* modname );

//! Reads signal information from specified file.
bool signal_db_read( char** line, module* curr_mod );

//! Sets value of signal to specified value.
bool signal_set_value( signal* sig, nibble* value, int num_bits, int from_lsb, int to_lsb );

//! Adds an expression to the signal list.
void signal_add_expression( signal* sig, expression* expr );

//! Displays signal contents to standard output.
void signal_display( signal* sig );

//! Deallocates the memory used for this signal.
void signal_dealloc( signal* sig );

#endif

