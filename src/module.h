#ifndef __MODULE_H__
#define __MODULE_H__

/*!
 \file     module.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
 \brief    Contains functions for handling modules.
*/

#include <stdio.h>

#include "defines.h"


//! Initializes all values of module.
void module_init( module* mod );

//! Creates new module from heap and initializes structure.
module* module_create();

//! Merges two modules into base module.
void module_merge( module* base, module* in );

//! Writes contents of provided module to specified output.
bool module_db_write( module* mod, FILE* file );

//! Read contents of current line from specified file, creates module and adds to module list.
bool module_db_read( module** mod, char** line );

//! Displays signals stored in this module.
void module_display_signals( module* mod );

//! Displays expressions stored in this module.
void module_display_expressions( module* mod );

//! Deallocates module element contents only from heap.
void module_clean( module* mod );

//! Deallocates module element from heap.
void module_dealloc( module* mod );

#endif

