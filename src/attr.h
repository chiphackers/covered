#ifndef __COVERED_ATTR_H__
#define __COVERED_ATTR_H__

/*!
 \file     attr.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/25/2003
 \brief    Contains functions for handling attributes.
*/

#include "defines.h"


/*! \brief Creates new attribute parameter based on specified values. */
attr_param* attribute_create( char* name, expression* expr );

/*! \brief Parses and handles specified attribute parameter list. */
void attribute_parse( attr_param* ap, module* mod );

/*! \brief Deallocates entire attribute parameter list. */
void attribute_dealloc( attr_param* ap );


/*
 $Log$
 Revision 1.2  2003/11/15 04:21:57  phase1geo
 Fixing syntax errors found in Doxygen and GCC compiler.

 Revision 1.1  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

*/

#endif

