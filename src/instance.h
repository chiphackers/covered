#ifndef __INSTANCE_H__
#define __INSTANCE_H__

/*!
 \file    instance.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/11/2002
 \brief    Contains functions for handling module instances.
*/

#include "defines.h"
#include "module.h"


//! Finds specified scope in module instance tree.
mod_inst* instance_find_scope( mod_inst* root, char* scope );

//! Adds new instance to specified instance tree.
void instance_add( mod_inst** root, char* parent, module* child, char* inst_name );

//! Displays contents of module instance tree to specified file.
void instance_db_write( mod_inst* root, FILE* file, char* scope );

//! Removes specified instance from tree.
void instance_dealloc( mod_inst* root, char* scope );

/* $Log */

#endif

