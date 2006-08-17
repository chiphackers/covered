/*!
 \file     obfuscate.c
 \author   Trevor Williams  (trevorw@charter.net)
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
tnode* obf_tree    = NULL;

/*!
 Current obfuscation identifier.  Incremented by one each time that a new obfuscation occurs.
*/
int    obf_curr_id = 0;


/*!
 \param real_name  Name of actual object in design
 \param prefix     Character representing the prefix of the obfuscated name

 \return Returns the obfuscated name for this object.

 Looks up the given real name in the obfuscation tree.  If it exists, simply
 return the given name; otherwise, create a new element in the tree to represent
 this new name.
*/
char* obfuscate_get_name( char* real_name, char prefix ) {

  tnode* obfnode;    /* Pointer to obfuscated tree node */
  char*  obfname;    /* Obfuscated name */
  char*  key;        /* Temporary name used for searching */
  char   tname[30];  /* Temporary name used for sizing obfuscation ID */

  /* Create temporary name */
  key = (char*)malloc_safe( (strlen( real_name ) + 3), __FILE__, __LINE__ );
  snprintf( key, (strlen( real_name ) + 2), "%s-%c", real_name, prefix );

  /* If the name was previously obfuscated, return that name */
  if( (obfnode = tree_find( key, obf_tree )) != NULL ) {

    obfname = strdup_safe( obfnode->value, __FILE__, __LINE__ );

  /* Otherwise, create a new obfuscated entry in the tree and return the new name */
  } else {

    /* Calculate the size needed for storing the obfuscated name */
    snprintf( tname, 30, "%04d", obf_curr_id );

    /* Create obfuscated name */
    obfname = (char*)malloc_safe( (strlen( tname ) + 2), __FILE__, __LINE__ );
    snprintf( obfname, (strlen( tname ) + 2), "%c%04d", prefix, obf_curr_id );
    obf_curr_id++;

    /* Add the obfuscated name to the tree */
    tree_add( key, obfname, FALSE, &obf_tree );

  }  

  return( obfname );

}

/*!
 Deallocates all memory associated with obfuscation.
*/
void obfuscate_dealloc() {

  tree_dealloc( obf_tree );

}


/*
 $Log$
 Revision 1.1.2.1  2006/08/17 04:17:37  phase1geo
 Adding files to obfuscate actual names when outputting any user-visible
 information.

*/

