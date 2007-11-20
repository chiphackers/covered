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
 \param key       String containing search key for node retrieval.
 \param value     Value associated with this node.
 \param override  If TRUE, causes new value to overwrite old value if match found.
 \param root      Pointer to root of tree to add to.
 
 \return Returns pointer to newly created tree node.
 
 Creates new node for this pairing and adds it to the binary tree
 for quick lookup.
*/
tnode* tree_add( const char* key, const char* value, bool override, tnode** root ) {
  
  tnode* node;            /* Pointer to newly created tree node */
  tnode* curr   = *root;  /* Pointer to current node */
  bool   placed = FALSE;  /* Sets to TRUE when node is placed in tree */
  int    comp;            /* Specifies compare value for string comparison */

  /* Allocate memory for tree node and populate */
  node        = (tnode*)malloc_safe( sizeof( tnode ), __FILE__, __LINE__ );
  node->name  = strdup_safe( key, __FILE__, __LINE__ );
  node->value = strdup_safe( value, __FILE__, __LINE__ );
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
          free_safe( curr->value );
          curr->value = node->value;
        } else {
          free_safe( node->value );
        }

        free_safe( node->name );
        free_safe( node );
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

  return( node );

}

/*!
 \param key   Key value to search for in tree.
 \param root  Pointer to root of binary tree to search.
 
 \return Returns pointer to found node or NULL if not found.
 
 Searches binary tree for key that matches the specified name parameter.
 If found, a pointer to the node is returned; otherwise, the value of NULL
 is returned.
*/
tnode* tree_find( const char* key, tnode* root ) {

  int comp;  /* Value of string comparison */

  while( (root != NULL) && ((comp = strcmp( key, root->name )) != 0) ) {
    if( comp < 0 ) {
      root = root->left;
    } else {
      root = root->right;
    }
  }

  return( root );

}

/*!
 \param key   Key to search for and remove from tree.
 \param root  Pointer to root of tree to search.
 
 Looks up the specified node (based on key value) and removes it from
 the tree in such a was as to keep the integrity of the tree in check
 for continual quick searching.
*/
void tree_remove( const char* key, tnode** root ) {
  
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

    free_safe(node->name);
    free_safe(node->value);
    free_safe(node);

  }
  
}

/*!
 \param root  Pointer to root of tree to deallocate.
 
 Recursively traverses specified tree, deallocating all memory associated with
 that tree.
*/
void tree_dealloc( tnode* root ) {
  
  if( root != NULL ) {
    
    if( root->left != NULL ) {
      tree_dealloc( root->left );
    }
    
    if( root->right != NULL ) {
      tree_dealloc( root->right );
    }
    
    free_safe( root->name );
    free_safe( root->value );
    free_safe( root );
    
  }
  
}

/*
 $Log$
 Revision 1.5  2006/11/03 18:16:38  phase1geo
 Removing unnecessary spaces in comments.

 Revision 1.4  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.3  2005/12/19 23:11:27  phase1geo
 More fixes for memory faults.  Full regression passes.  Errors have now been
 eliminated from regression -- just left-over memory issues remain.

 Revision 1.2  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.1  2003/01/04 09:25:15  phase1geo
 Fixing file search algorithm to fix bug where unexpected module that was
 ignored cannot be found.  Added instance7.v diagnostic to verify appropriate
 handling of this problem.  Added tree.c and tree.h and removed define_t
 structure in lexer.

*/

