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


/*! Builds full hierarchy from leaf node to root. */
void instance_gen_scope( char* scope, mod_inst* leaf );

/*! Finds specified scope in module instance tree. */
mod_inst* instance_find_scope( mod_inst* root, char* scope );

/*! Returns instance that points to specified module for each instance. */
mod_inst* instance_find_by_module( mod_inst* root, module* mod, int* ignore );

/*! Adds new instance to specified instance tree during parse. */
void instance_parse_add( mod_inst** root, module* parent, module* child, char* inst_name );

/*! Adds new instance to specified instance tree during CDD read. */
void instance_read_add( mod_inst** root, char* parent, module* child, char* inst_name );

/*! Displays contents of module instance tree to specified file. */
void instance_db_write( mod_inst* root, FILE* file, char* scope, bool parse_mode );

/*! Removes specified instance from tree. */
void instance_dealloc( mod_inst* root, char* scope );

/*
 $Log$
 Revision 1.7  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.6  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.5  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.4  2002/07/18 05:50:45  phase1geo
 Fixes should be just about complete for instance depth problems now.  Diagnostics
 to help verify instance handling are added to regression.  Full regression passes.

 Revision 1.3  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.
*/

#endif

