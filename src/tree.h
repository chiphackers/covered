#ifndef __TREE_H__
#define __TREE_H__

/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     tree.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/4/2003
 \brief    Contains functions for adding, finding, and removing nodes from binary tree.
*/

#include "defines.h"

/*! \brief Adds specified key/value pair to tree as a node. */
tnode* tree_add( const char* key, const char* value, bool override, tnode** root );

/*! \brief Returns pointer to tree node that matches specified key */
tnode* tree_find( const char* key, tnode* root );

/*! \brief Removes specified tree node from tree. */
void tree_remove( const char* key, tnode** root );

/*! \brief Deallocates entire tree from memory. */
void tree_dealloc( /*@null@*/tnode* root );

/*
 $Log$
 Revision 1.2  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.1  2003/01/04 09:25:15  phase1geo
 Fixing file search algorithm to fix bug where unexpected module that was
 ignored cannot be found.  Added instance7.v diagnostic to verify appropriate
 handling of this problem.  Added tree.c and tree.h and removed define_t
 structure in lexer.

*/

#endif

