/*!
 \file     instance.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/11/2002
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "instance.h"
#include "module.h"
#include "util.h"


/*!
 \param root    Root of mod_inst tree to parse for scope.
 \param scope   Scope to search for.
 
 \return Returns pointer to module instance found by scope.
 
 Searches the specified module instance tree for the specified
 scope.  When the module instance is found, a pointer to that
 module instance is passed back to the calling function.
*/
mod_inst* instance_find_scope( mod_inst* root, char* scope ) {
 
  int       i;           /* Loop iterator                                        */
  char      front[256];  /* Highest level of hierarchy in hierarchical reference */
  char      rest[4096];  /* Rest of scope value                                  */
  mod_inst* inst;        /* Pointer to found instance                            */
  mod_inst* child;       /* Pointer to child instance of this module instance    */
  
  if( root != NULL ) {

    assert( scope != NULL );

    scope_extract_front( scope, front, rest );
  
    if( strcmp( front, root->name ) == 0 ) {
      if( rest[0] == '\0' ) {
        return( root );
      } else {
        child = root->child_head;
        while( (child != NULL) && ((inst = instance_find_scope( child, rest )) == NULL) ) {
	  child = child->next;
        }
        if( child == NULL ) {
	  return( NULL );
        } else {
	  return( inst );
        }
      }
    } else {
      return( NULL );
    }

  } else {

    return( NULL );

  }
				
}

/*!
 \param root       Root mod_inst pointer of module instance tree.
 \param parent     Scope of parent module.
 \param child      Pointer to child module to add.
 \param inst_name  Name of new module instance.
 
 Adds the child module to the child module pointer list located in
 the module specified by the scope of parent in the module instance
 tree pointed to by root.
*/
void instance_add( mod_inst** root, char* parent, module* child, char* inst_name ) {
  
  mod_inst* inst;      /* Temporary pointer to module instance to add to */
  mod_inst* new_inst;  /* Pointer to new module instance to add          */
  
  printf( "In instance_add, parent scope: %s, child instance: %s\n", parent, inst_name );

  new_inst             = (mod_inst*)malloc_safe( sizeof( mod_inst ) );
  new_inst->mod        = child;
  new_inst->name       = strdup( inst_name );
  new_inst->stat       = NULL;
  new_inst->child_head = NULL;
  new_inst->child_tail = NULL;
  new_inst->next       = NULL;
  
  if( *root == NULL ) {

    *root = new_inst;

  } else {

    inst = instance_find_scope( *root, parent );
  
    assert( inst != NULL );

    if( inst->child_head == NULL ) {
      inst->child_head = new_inst;
      inst->child_tail = new_inst;
    } else {
      inst->child_tail->next = new_inst;
      inst->child_tail       = new_inst;
    }
 
  }
  
}

/*!
 \param root   Root of module instance tree to write.
 \param file   Output file to display contents to.
 \param scope  Scope of this module.

 Calls each module display function in instance tree, starting with
 the root module and ending when all of the leaf modules are output.
 Note:  the function that calls this function originally should set
 the value of scope to NULL.
*/
void instance_db_write( mod_inst* root, FILE* file, char* scope ) {

  char      full_scope[4096];  /* Full scope of module to write            */
  mod_inst* curr;              /* Pointer to current child module instance */

  if( root->mod->scope != NULL ) {
    free_safe( root->mod->scope );
  }

  assert( scope != NULL );

  /* Display root module */
  root->mod->scope = strdup( scope ); 
  module_db_write( root->mod, file );

  /* Display children */
  curr = root->child_head;
  while( curr != NULL ) {
    snprintf( full_scope, 4096, "%s.%s", scope, curr->name );
    instance_db_write( curr, file, full_scope );
    curr = curr->next;
  }

}

/*!
 \param root  Pointer to root instance of module instance tree to remove.

 Recursively traverses instance tree, deallocating heap memory used to store the
 the tree.
*/
void instance_dealloc_tree( mod_inst* root ) {

  mod_inst* curr;        /* Pointer to current instance to evaluate */
  mod_inst* tmp;         /* Temporary pointer to instance           */

  if( root != NULL ) {

    /* Remove instance's children first */
    curr = root->child_head;
    while( curr != NULL ) {
      tmp = curr->next;
      instance_dealloc_tree( curr );
      curr = tmp;
    }

    /* Free up memory allocated for name */
    free_safe( root->name );

    /* Free up memory allocated for statistic, if necessary */
    if( root->stat != NULL ) {
      free_safe( root->stat );
    }
  
    /* Free up memory for this module instance */
    free_safe( root );

  }

}

/*!
 \param root   Root of module instance tree.
 \param scope  Scope of module to remove from tree.
    
 Searches tree for specified module.  If the module instance is found,
 the module instance is removed from the tree along with all of its
 child module instances.
*/
void instance_dealloc( mod_inst* root, char* scope ) {
  
  mod_inst* inst;        /* Pointer to instance to remove                        */
  mod_inst* curr;        /* Pointer to current child instance to remove          */
  mod_inst* last;        /* Last current child instance                          */
  char      back[256];   /* Highest level of hierarchy in hierarchical reference */
  char      rest[4096];  /* Rest of scope value                                  */
  
  /* 
   First, find parent instance of given scope and remove this instance
   from its child list.
  */  
  scope_extract_back( scope, back, rest );

  if( rest[0] == '\0' ) {
    /* Current scope is root */
    inst = NULL;
  } else {
    inst = instance_find_scope( root, rest );
  }

  /* If inst is NULL, the scope is the root instance so no child removal is necessary */
  if( inst != NULL ) {

    curr = inst->child_head;
    last = NULL;
    while( (curr != NULL) && (strcmp( curr->name, scope ) != 0) ) {
      last = curr;
      curr = curr->next;
    }

    if( curr != NULL ) {
      if( last != NULL ) {
        last->next = curr->next;
      }
      if( curr == inst->child_head ) {
        /* Move parent head pointer */
        inst->child_head = curr->next;
      }
      if( curr == inst->child_tail ) {
        /* Move parent tail pointer */
        inst->child_tail = last;
      }
    }

    instance_dealloc_tree( curr );

  } else {

    instance_dealloc_tree( root );

  }

}

/* $Log */
