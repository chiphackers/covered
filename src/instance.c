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
#include "param.h"


/*!
 \param mod        Pointer to module to store in this instance.
 \param inst_name  Instantiated name of this instance.

 \return Returns pointer to newly created module instance.

 Creates a new module instance from heap, initializes its data and
 returns a pointer to it.
*/
mod_inst* instance_create( module* mod, char* inst_name ) {

  mod_inst* new_inst;   /* Pointer to new module instance */

  new_inst             = (mod_inst*)malloc_safe( sizeof( mod_inst ) );
  new_inst->mod        = mod;
  new_inst->name       = strdup( inst_name );
  new_inst->stat       = NULL;
  new_inst->param_head = NULL;
  new_inst->param_tail = NULL;
  new_inst->psig_head  = NULL;
  new_inst->psig_tail  = NULL;
  new_inst->pexp_head  = NULL;
  new_inst->pexp_tail  = NULL;
  new_inst->child_head = NULL;
  new_inst->child_tail = NULL;
  new_inst->next       = NULL;

  return( new_inst );

}

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
 \param root    Pointer to root module instance of tree.
 \param mod     Pointer to module to find in tree.
 \param ignore  Pointer to number of matches to ignore.

 \return Returns pointer to module instance found by scope.
 
 Searches the specified module instance tree for the specified
 module.  When a module instance is found that points to the specified
 module and the ignore value is 0, a pointer to that module instance is 
 passed back to the calling function; otherwise, the ignore count is
 decremented and the searching continues.
*/
mod_inst* instance_find_by_module( mod_inst* root, module* mod, int* ignore ) {

  mod_inst* match_inst = NULL;   /* Pointer to module instance that found a match      */
  mod_inst* curr_child;          /* Pointer to current instances child module instance */

  if( root != NULL ) {

    if( root->mod == mod ) {

      if( *ignore == 0 ) {
        match_inst = root;
      } else {
        (*ignore)--;
      }

    } else {

      curr_child = root->child_head;
      while( (curr_child != NULL) && (match_inst == NULL) ) {
        match_inst = instance_find_by_module( curr_child, mod, ignore );
        curr_child = curr_child->next;
      }

    }
    
  }

  return( match_inst );

}

/*!
 \param root       Root mod_inst pointer of module instance tree.
 \param parent     Pointer to parent module of specified child.
 \param child      Pointer to child module to add.
 \param inst_name  Name of new module instance.
 
 Adds the child module to the child module pointer list located in
 the module specified by the scope of parent in the module instance
 tree pointed to by root.  This function is used by the db_add_instance
 function during the parsing stage.
*/
void instance_parse_add( mod_inst** root, module* parent, module* child, char* inst_name ) {
  
  mod_inst* inst;      /* Temporary pointer to module instance to add to */
  mod_inst* new_inst;  /* Pointer to new module instance to add          */
  int       i;         /* Loop iterator                                  */
  int       ignore;    /* Number of matched instances to ignore          */

  if( *root == NULL ) {

    // printf( "In instance_parse_add, top instance name: %s\n", inst_name );
    *root = instance_create( child, inst_name );

  } else {

    assert( parent != NULL );
  
    // printf( "In instance_parse_add, parent name: %s, child instance: %s\n", parent->name, inst_name );

    i      = 0;
    ignore = 0;

    while( (inst = instance_find_by_module( *root, parent, &ignore )) != NULL ) {

      new_inst = instance_create( child, inst_name );

      if( inst->child_head == NULL ) {
        inst->child_head = new_inst;
        inst->child_tail = new_inst;
      } else {
        inst->child_tail->next = new_inst;
        inst->child_tail       = new_inst;
      }

      i++;
      ignore = i;

    }

    /* We should have found at least one parent instance */
    assert( i > 0 );
 
  }
  
}

/*!
 \param root    Pointer to root instance of module instance tree.
 \param parent  String scope of parent instance.
 \param child   Pointer to child module to add to specified parent's child list.
 \param inst_name  Instance name of this child module instance.

 Adds the child module to the child module pointer list located in
 the module specified by the scope of parent in the module instance
 tree pointed to by root.  This function is used by the db_read
 function during the CDD reading stage.
*/ 
void instance_read_add( mod_inst** root, char* parent, module* child, char* inst_name ) {

  mod_inst* inst;      /* Temporary pointer to module instance to add to */
  mod_inst* new_inst;  /* Pointer to new module instance to add          */

  new_inst = instance_create( child, inst_name );

  if( *root == NULL ) {

    *root = new_inst;

  } else {

    assert( parent != NULL );
  
    if( (inst = instance_find_scope( *root, parent )) != NULL ) {

      if( inst->child_head == NULL ) {
        inst->child_head = new_inst;
        inst->child_tail = new_inst;
      } else {
        inst->child_tail->next = new_inst;
        inst->child_tail       = new_inst;
      }

    } else {

      /* Unable to find parent of this child, something went in wrong in writing/reading CDD file. */
      assert( inst != NULL );

    }
 
  }

}

void instance_calc_params( mod_inst* inst ) {

    

}

/*!
 \param inst  Pointer to instance to generate parameters for.

 Iterates through entire instance parameter list, calling the param_generate
 routine for each parameter in this list.
*/
void instance_param_generate( mod_inst* inst ) {

  parameter* parm;    /* Pointer to current parameter in list */

  assert( inst != NULL );

  parm = inst->param_head;

  while( parm != NULL ) {
    param_generate( parm );
    parm = parm->next;
  }

}

/*!
 \param inst  Pointer to instance to destroy parameters for.

 Iterates through entire instance parameter list, calling the param_destroy
 routine for each parameter in this list.
*/
void instance_param_destroy( mod_inst* inst ) {

  parameter* parm;    /* Pointer to current parameter in list */

  assert( inst != NULL );

  parm = inst->param_head;

  while( parm != NULL ) {
    param_destroy( parm );
    parm = parm->next;
  }

}

/*!
 \param root        Root of module instance tree to write.
 \param file        Output file to display contents to.
 \param scope       Scope of this module.
 \param parse_mode  Specifies if we are parsing or scoring.

 Calls each module display function in instance tree, starting with
 the root module and ending when all of the leaf modules are output.
 Note:  the function that calls this function originally should set
 the value of scope to NULL.
*/
void instance_db_write( mod_inst* root, FILE* file, char* scope, bool parse_mode ) {

  char      full_scope[4096];  /* Full scope of module to write            */
  mod_inst* curr;              /* Pointer to current child module instance */

  assert( scope != NULL );

  /* Handle parameters at this time */
  if( parse_mode ) {
    instance_calc_params( root );
    instance_param_generate( root );
  }

  /* Display root module */
  module_db_write( root->mod, scope, file );

  if( parse_mode ) {
    instance_param_destroy( root );
  }

  /* Display children */
  curr = root->child_head;
  while( curr != NULL ) {
    assert( (strlen( scope ) + strlen( curr->name ) + 1) <= 4096 );
    snprintf( full_scope, 4096, "%s.%s", scope, curr->name );
    instance_db_write( curr, file, full_scope, parse_mode );
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

/* $Log$
/* Revision 1.11  2002/09/06 03:05:28  phase1geo
/* Some ideas about handling parameters have been added to these files.  Added
/* "Special Thanks" section in User's Guide for acknowledgements to people
/* helping in project.
/*
/* Revision 1.10  2002/08/19 04:34:07  phase1geo
/* Fixing bug in database reading code that dealt with merging modules.  Module
/* merging is now performed in a more optimal way.  Full regression passes and
/* own examples pass as well.
/*
/* Revision 1.9  2002/07/18 05:50:45  phase1geo
/* Fixes should be just about complete for instance depth problems now.  Diagnostics
/* to help verify instance handling are added to regression.  Full regression passes.
/* */
