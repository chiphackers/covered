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
 \file     tree.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/4/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>

#include "defines.h"
#include "tree.h"
#include "util.h"


/*!
 \return Returns pointer to newly created tree node.
 
 Creates new node for this pairing and adds it to the binary tree
 for quick lookup.
*/
tnode* tree_add(
  const char* key,       /*!< String containing search key for node retrieval */
  const char* value,     /*!< Value associated with this node */
  bool        override,  /*!< If TRUE, causes new value to overwrite old value if match found */
  tnode**     root       /*!< Pointer to root of tree to add to */
) { PROFILE(TREE_ADD);
  
  tnode* node;            /* Pointer to newly created tree node */
  tnode* curr   = *root;  /* Pointer to current node */
  bool   placed = FALSE;  /* Sets to TRUE when node is placed in tree */
  int    comp;            /* Specifies compare value for string comparison */

  /* Allocate memory for tree node and populate */
  node        = (tnode*)malloc_safe( sizeof( tnode ) );
  node->name  = strdup_safe( key );
  node->value = strdup_safe( value );
  node->left  = NULL;
  node->right = NULL;
  node->up    = NULL;

  /* Add node to tree */
  if( *root == NULL ) {
    *root = node;
  } else {
    while( !placed ) {
      comp = strcmp( node->name, curr->name );
      if( comp == 0 ) {

        /* Match found, replace value with new value */
        if( override ) {
          free_safe( curr->value, (strlen( curr->value ) + 1) );
          curr->value = node->value;
        } else {
          free_safe( node->value, (strlen( node->value ) + 1) );
          node->value = NULL;
        }

        free_safe( node->name, (strlen( node->name ) + 1) );
        free_safe( node, sizeof( tnode ) );
        node   = curr;
        placed = TRUE;

      } else if( comp < 0 ) {

        if( curr->left == NULL ) {
          curr->left = node;
          node->up   = curr;
          placed     = TRUE;
        } else {
          curr = curr->left;
        }

      } else {
        
        if( curr->right == NULL ) {
          curr->right = node;
          node->up    = curr;
          placed      = TRUE;
        } else {
          curr        = curr->right;
        }

      }
    }
  }

  PROFILE_END;

  return( node );

}

/*!
 \return Returns pointer to found node or NULL if not found.
 
 Searches binary tree for key that matches the specified name parameter.
 If found, a pointer to the node is returned; otherwise, the value of NULL
 is returned.
*/
tnode* tree_find(
  const char* key,  /*!< Key value to search for in tree */
  tnode*      root  /*!< Pointer to root of binary tree to search */
) { PROFILE(TREE_FIND);

  int comp;  /* Value of string comparison */

  while( (root != NULL) && ((comp = strcmp( key, root->name )) != 0) ) {
    if( comp < 0 ) {
      root = root->left;
    } else {
      root = root->right;
    }
  }

  PROFILE_END;

  return( root );

}

/*!
 Looks up the specified node (based on key value) and removes it from
 the tree in such a was as to keep the integrity of the tree in check
 for continual quick searching.
*/
void tree_remove(
  const char* key,  /*!< Key to search for and remove from tree */
  tnode**     root  /*!< Pointer to root of tree to search */
) { PROFILE(TREE_REMOVE);
  
  tnode* node;  /* Pointer to found tree node to remove */
  tnode* tail;  /* Temporary pointer to tail node */
  
  /* Find undefined identifer string in table */
  node = tree_find( key, *root );

  /* If node is found, restitch the define tree. */
  if( node != NULL ) {

    /* If we are the root node in the tree */
    if( node->up == NULL ) {

      /* If we have no children */
      if( (node->left == NULL) && (node->right == NULL) ) {

        *root = NULL;

      } else if( node->left == NULL ) {

        *root = node->right;
        if( node->right ) {
          node->right->up = NULL;
        }

      } else if( node->right == NULL ) {

        assert( node->left != NULL );
        *root       = node->left;
        (*root)->up = NULL;

      } else {

        tail = node->left;
        while( tail->right ) {
          tail = tail->right;
        }

        tail->right     = node->right;
        tail->right->up = tail;
        *root           = node->left;
        (*root)->up     = NULL;

      }

    } else if( node->left == NULL ) {

      if( node->up->left == node ) {

        node->up->left = node->right;

      } else {

        assert( node->up->right == node );
        node->up->right = node->right;

      }

      if( node->right ) {
        node->right->up = node->up;
      }

    } else if( node->right == NULL ) {

      assert( node->left != NULL );

      if( node->up->left == node ) {

        node->up->left = node->left;

      } else {

        assert( node->up->right == node );
        node->up->right = node->left;

      }

      node->left->up = node->up;

    } else {

      tail = node->left;
      assert( (node->left != NULL) && (node->right != NULL) );

      while( tail->right ) {
        tail = tail->right;
      }

      tail->right     = node->right;
      tail->right->up = tail;

      if( node->up->left == node ) {

        node->up->left = node->left;

      } else {

        assert( node->up->right == node );
        node->up->right = node->left;

      }

      node->left->up = node->up;

    }

    free_safe( node->name, (strlen( node->name ) + 1) );
    free_safe( node->value, (strlen( node->value ) + 1) );
    free_safe( node, sizeof( tnode ) );

  }

  PROFILE_END;
  
}

/*!
 Recursively traverses specified tree, deallocating all memory associated with
 that tree.
*/
void tree_dealloc(
  tnode* root  /*!< Pointer to root of tree to deallocate */
) { PROFILE(TREE_DEALLOC);
  
  if( root != NULL ) {
    
    if( root->left != NULL ) {
      tree_dealloc( root->left );
    }
    
    if( root->right != NULL ) {
      tree_dealloc( root->right );
    }
    
    free_safe( root->name, (strlen( root->name ) + 1) );
    free_safe( root->value, (strlen( root->value ) + 1) );
    free_safe( root, sizeof( tnode ) );
    
  }

  PROFILE_END;
  
}

