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


/*! \brief Initializes all values of module. */
void module_init( module* mod );

/*! \brief Creates new module from heap and initializes structure. */
module* module_create();

/*! \brief Writes contents of provided module to specified output. */
bool module_db_write( module* mod, char* scope, FILE* file, mod_inst* inst );

/*! \brief Read contents of current line from specified file, creates module and adds to module list. */
bool module_db_read( module* mod, char* scope, char** line );

/*! \brief Reads and merges two modules into base module. */
bool module_db_merge( module* base, FILE* file, bool same );

/*! \brief Reads and replaces original module with contents of new module. */
bool module_db_replace( module* base, FILE* file );

/*! \brief Displays signals stored in this module. */
void module_display_signals( module* mod );

/*! \brief Displays expressions stored in this module. */
void module_display_expressions( module* mod );

/*! \brief Deallocates module element contents only from heap. */
void module_clean( module* mod );

/*! \brief Deallocates module element from heap. */
void module_dealloc( module* mod );


/*
 $Log$
 Revision 1.9  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.8  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.7  2002/10/31 23:14:00  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.4  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.3  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

