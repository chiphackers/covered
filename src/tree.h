#ifndef __TREE_H__
#define __TREE_H__

/*
 Copyright (c) 2006-2010 Trevor Williams

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
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/4/2003
 \brief    Contains functions for adding, finding, and removing nodes from binary tree.
*/

#include "defines.h"

/*! \brief Adds specified key/value pair to tree as a node. */
tnode* tree_add(
  const char* key,
  const char* value,
  bool        override,
  tnode**     root
);

/*! \brief Returns pointer to tree node that matches specified key */
tnode* tree_find(
  const char* key,
  tnode*      root
);

/*! \brief Removes specified tree node from tree. */
void tree_remove(
  const char* key,
  tnode**     root
);

/*! \brief Deallocates entire tree from memory. */
void tree_dealloc(
  /*@null@*/tnode* root
);

#endif

