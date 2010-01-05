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
 \file     obfuscate.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     9/16/2006
*/

#include <string.h>
#include <stdio.h>

#include "defines.h"
#include "obfuscate.h"
#include "util.h"
#include "tree.h"


/*!
 Pointer to the root of the obfuscation tree containing real to obfuscated name pairings.
*/
static tnode* obf_tree = NULL;

/*!
 Current obfuscation identifier.  Incremented by one each time that a new obfuscation occurs.
*/
static int obf_curr_id = 1000;

/*!
 Specifies obfuscation mode.
*/
bool obf_mode;


/*!
 Sets the global 'obf_mode' variable to the specified value.
*/
void obfuscate_set_mode(
  bool value  /*!< Boolean value to set obfuscation mode to */
) { PROFILE(OBFUSCATE_SET_MODE);

  obf_mode = value;

  PROFILE_END;

}

/*!
 \return Returns the obfuscated name for this object.

 Looks up the given real name in the obfuscation tree.  If it exists, simply
 return the given name; otherwise, create a new element in the tree to represent
 this new name.
*/
char* obfuscate_name(
  const char* real_name,  /*!< Name of actual object in design */
  char        prefix      /*!< Character representing the prefix of the obfuscated name */
) { PROFILE(OBFUSCATE_NAME);

  tnode*       obfnode;    /* Pointer to obfuscated tree node */
  char*        key;        /* Temporary name used for searching */
  char         tname[30];  /* Temporary name used for sizing obfuscation ID */
  unsigned int rv;         /* Return value from snprintf calls */
  unsigned int slen;       /* String length */

  /* Create temporary name */
  slen = strlen( real_name ) + 3;
  key = (char*)malloc_safe( slen );
  rv = snprintf( key, slen, "%s-%c", real_name, prefix );
  assert( rv < slen );

  /* If the name was not previously obfuscated, create a new obfuscated entry in the tree and return the new name */
  if( (obfnode = tree_find( key, obf_tree )) == NULL ) {

    /* Create obfuscated name */
    rv = snprintf( tname, 30, "%c%04d", prefix, obf_curr_id );
    assert( rv < 30 );
    obf_curr_id++;

    /* Add the obfuscated name to the tree and get the pointer to the new node */
    obfnode = tree_add( key, tname, FALSE, &obf_tree );

  }

  /* Deallocate key string */
  free_safe( key, (strlen( key ) + 1) );

  PROFILE_END;

  return( obfnode->value );

}

/*!
 Deallocates all memory associated with obfuscation.
*/
void obfuscate_dealloc() { PROFILE(OBFUSCATE_DEALLOC);

  tree_dealloc( obf_tree );

  PROFILE_END;

}

