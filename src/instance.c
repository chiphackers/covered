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
 \file     instance.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/11/2002
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "instance.h"
#include "link.h"
#include "param.h"
#include "static.h"
#include "util.h"


extern int        curr_expr_id;
extern inst_link* inst_head;
extern char       user_msg[USER_MSG_LENGTH];


bool instance_resolve_inst( funit_inst* root, funit_inst* curr );


/*!
 \param root    Pointer to functional unit instance to display
 \param prefix  Prefix string to be used when outputting (used to indent children)

 Helper function for the \ref instance_display_tree function.
*/
void instance_display_tree_helper( funit_inst* root, char* prefix ) {

  char        sp[4096];  /* Contains prefix for children */
  funit_inst* curr;      /* Pointer to current child instance */
  char*       piname;    /* Printable version of this instance */
  char*       pfname;    /* Printable version of this instance functional unit */

  assert( root != NULL );

  /* Get printable version of this instance and functional unit name */
  piname = scope_gen_printable( root->name );
  pfname = scope_gen_printable( root->funit->name );

  /* Display ourselves */
  printf( "%s%s (%s)\n", prefix, piname, pfname );

  /* Calculate prefix */
  snprintf( sp, 4096, "%s   ", prefix );

  /* Display our children */
  curr = root->child_head;
  while( curr != NULL ) {
    instance_display_tree_helper( curr, sp );
    curr = curr->next;
  }

  /* Deallocate memory */
  free_safe( piname );
  free_safe( pfname );

}

/*!
 \param root  Pointer to root instance to display

 Displays the given instance tree to standard output in a hierarchical format.  Shows
 instance names as well as associated module name.
*/
void instance_display_tree( funit_inst* root ) {

  instance_display_tree_helper( root, "" );

}

/*!
 \param funit      Pointer to functional unit to store in this instance.
 \param inst_name  Instantiated name of this instance.
 \param range      For arrays of instances, contains range information for this array.

 \return Returns pointer to newly created functional unit instance.

 Creates a new functional unit instance from heap, initializes its data and
 returns a pointer to it.
*/
funit_inst* instance_create( func_unit* funit, char* inst_name, vector_width* range ) {

  funit_inst* new_inst;  /* Pointer to new functional unit instance */

  new_inst             = (funit_inst*)malloc_safe( sizeof( funit_inst ), __FILE__, __LINE__ );
  new_inst->funit      = funit;
  new_inst->name       = strdup_safe( inst_name, __FILE__, __LINE__ );
  new_inst->stat       = NULL;
  new_inst->param_head = NULL;
  new_inst->param_tail = NULL;
  new_inst->gitem_head = NULL;
  new_inst->gitem_tail = NULL;
  new_inst->parent     = NULL;
  new_inst->child_head = NULL;
  new_inst->child_tail = NULL;
  new_inst->next       = NULL;

  /* Create range (get a copy since this memory is managed by the parser) */
  if( range == NULL ) {
    new_inst->range = NULL;
  } else {
    assert( range->left  != NULL );
    assert( range->right != NULL );
    new_inst->range             = (vector_width*)malloc_safe( sizeof( vector_width ), __FILE__, __LINE__ );
    new_inst->range->left       = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
    new_inst->range->left->num  = range->left->num;
    new_inst->range->left->exp  = range->left->exp;
    new_inst->range->right      = (static_expr*)malloc_safe( sizeof( static_expr ), __FILE__, __LINE__ );
    new_inst->range->right->num = range->right->num;
    new_inst->range->right->exp = range->right->exp;
  }

  return( new_inst );

}

/*!
 \param scope  String pointer to store generated scope (assumed to be allocated)
 \param leaf   Pointer to leaf instance in scope.

 Recursively travels up to the root of the instance tree, building the scope
 string as it goes.  When the root instance is reached, the string is returned.
 Assumes that scope is initialized to the NULL character.
*/
void instance_gen_scope( char* scope, funit_inst* leaf ) {

  if( leaf != NULL ) {

    /* Call parent instance first */
    instance_gen_scope( scope, leaf->parent );

    if( scope[0] != '\0' ) {
      strcat( scope, "." );
      strcat( scope, leaf->name );
    } else {
      strcpy( scope, leaf->name );
    }

  }

}

/*!
 \param inst_name  Instance name to compare to this instance's name (may contain array information)
 \param inst       Pointer to instance to compare name against.

 \return Returns TRUE if the given instance name and instance match.  If the specified instance is
         a part of an array of instances and the base name matches the base name of inst_name, we
         also check to make sure that the index of inst_name falls within the legal range of this
         instance.
*/
bool instance_compare( char* inst_name, funit_inst* inst ) {

  bool retval = FALSE;  /* Return value of this function */
  char bname[4096];     /* Base name of inst_name */
  int  index;           /* Index of inst_name */
  int  width;           /* Width of instance range */
  int  lsb;             /* LSB of instance range */
  int  big_endian;      /* Specifies endianness */

  /* If this instance has a range, handle it */
  if( inst->range != NULL ) {

    /* Extract the index portion of inst_name if there is one */
    if( sscanf( inst_name, "%[a-zA-Z0-9_]\[%d]", bname, &index ) == 2 ) {
      
      /* If the base names compare, check that the given index falls within this instance range */
      if( scope_compare( bname, inst->name ) ) {

        /* Get range information from instance */
        static_expr_calc_lsb_and_width_post( inst->range->left, inst->range->right, &width, &lsb, &big_endian );
        assert( width != -1 );
        assert( lsb   != -1 );

        retval = (index >= lsb) && (index < (lsb + width));

      }
      
    }

  } else {

    retval = scope_compare( inst_name, inst->name );

  }

  return( retval );

}

/*!
 \param root        Root of funit_inst tree to parse for scope.
 \param scope       Scope to search for.
 \param rm_unnamed  Set to TRUE if we need to remove unnamed scopes
 
 \return Returns pointer to functional unit instance found by scope.
 
 Searches the specified functional unit instance tree for the specified
 scope.  When the functional unit instance is found, a pointer to that
 functional unit instance is passed back to the calling function.
*/
funit_inst* instance_find_scope( funit_inst* root, char* scope, bool rm_unnamed ) {
 
  char        front[256];   /* Front of scope value */
  char        rest[4096];   /* Rest of scope value */
  funit_inst* inst = NULL;  /* Pointer to found instance */
  funit_inst* child;        /* Pointer to current child instance being traversed */

  assert( root != NULL );

  /* First extract the front scope */
  scope_extract_front( scope, front, rest );

  /* Skip this instance and move onto the children if we are an unnamed scope that does not contain signals */
  if( !rm_unnamed && db_is_unnamed_scope( root->name ) && !funit_is_unnamed( root->funit ) ) {
    child = root->child_head;
    while( (child != NULL) && ((inst = instance_find_scope( child, scope, rm_unnamed )) == NULL) ) {
      child = child->next;
    }

  /* Keep traversing if our name matches */
  } else if( instance_compare( front, root ) ) {
    if( rest[0] == '\0' ) {
      inst = root;
    } else {
      child = root->child_head;
      while( (child != NULL) && ((inst = instance_find_scope( child, rest, rm_unnamed )) == NULL) ) {
        child = child->next;
      }
    }
  }

  return( inst );

}

/*!
 \param root    Pointer to root functional unit instance of tree.
 \param funit   Pointer to functional unit to find in tree.
 \param ignore  Pointer to number of matches to ignore.

 \return Returns pointer to functional unit instance found by scope.
 
 Searches the specified functional unit instance tree for the specified
 functional unit.  When a functional unit instance is found that points to the specified
 functional unit and the ignore value is 0, a pointer to that functional unit instance is 
 passed back to the calling function; otherwise, the ignore count is
 decremented and the searching continues.
*/
funit_inst* instance_find_by_funit( funit_inst* root, func_unit* funit, int* ignore ) {

  funit_inst* match_inst = NULL;  /* Pointer to functional unit instance that found a match */
  funit_inst* curr_child;         /* Pointer to current instances child functional unit instance */

  if( root != NULL ) {

    if( root->funit == funit ) {

      if( *ignore == 0 ) {
        match_inst = root;
      } else {
        (*ignore)--;
      }

    } else {

      curr_child = root->child_head;
      while( (curr_child != NULL) && (match_inst == NULL) ) {
        match_inst = instance_find_by_funit( curr_child, funit, ignore );
        curr_child = curr_child->next;
      }

    }
    
  }

  return( match_inst );

}

/*!
 \param inst     Pointer to instance to add child instance to.
 \param child    Pointer to child functional unit to create instance for.
 \param name     Name of instance to add.
 \param range    For arrays of instances, contains the range of the instance array
 \param resolve  Set to TRUE if newly added instance should be immediately resolved
 
 \return Returns pointer to newly created functional unit instance if this instance name isn't already in
         use in the current instance; otherwise, returns NULL.
 
 Generates new instance, adds it to the child list of the inst functional unit
 instance, and resolves any parameters.
*/
funit_inst* instance_add_child( funit_inst* inst, func_unit* child, char* name, vector_width* range, bool resolve ) {

  funit_inst* new_inst;  /* Pointer to newly created instance to add */

  /* Check to see if this instance already exists */
  new_inst = inst->child_head;
  while( (new_inst != NULL) && (strcmp( new_inst->name, name ) != 0) ) {
    new_inst = new_inst->next;
  }

  /* If this instance already exists, don't add it again */
  if( new_inst == NULL ) {

    /* Generate new instance */
    new_inst = instance_create( child, name, range );

    /* Add new instance to inst child instance list */
    if( inst->child_head == NULL ) {
      inst->child_head       = new_inst;
      inst->child_tail       = new_inst;
    } else {
      inst->child_tail->next = new_inst;
      inst->child_tail       = new_inst;
    }

    /* Point this instance's parent pointer to its parent */
    new_inst->parent = inst;

    /* If the new instance needs to be resolved now, do so */
    if( resolve ) {
      inst_link* instl = inst_head;
      while( (instl != NULL) && !instance_resolve_inst( instl->inst, new_inst ) ) {
        instl = instl->next;
      }
    }

  } else {

    new_inst = NULL;

  }

  return( new_inst );

}

/*!
 \param from_inst  Pointer to instance tree to copy.
 \param to_inst    Pointer to instance to copy tree to.
 \param name       Instance name of current instance being copied.
 \param range      For arrays of instances, indicates the array range.
 \param resolve    Set to TRUE if newly added instance should be immediately resolved.
 
 Recursively copies the instance tree of from_inst to the instance 
 to_inst, allocating memory for the new instances and resolving parameters.
*/
void instance_copy( funit_inst* from_inst, funit_inst* to_inst, char* name, vector_width* range, bool resolve ) {

  funit_inst* curr;      /* Pointer to current functional unit instance to copy */
  funit_inst* new_inst;  /* Pointer to newly created functional unit instance */

  assert( from_inst != NULL );
  assert( to_inst   != NULL );
  assert( name      != NULL );

  /* Add new child instance */
  new_inst = instance_add_child( to_inst, from_inst->funit, name, range, resolve );

  /* Do not add children if no child instance was created */
  if( new_inst != NULL ) {

    /* Iterate through rest of current child's list of children */
    curr = from_inst->child_head;
    while( curr != NULL ) {
      instance_copy( curr, new_inst, curr->name, curr->range, resolve );
      curr = curr->next;
    }

  }

}

/*!
 \param root        Root funit_inst pointer of functional unit instance tree.
 \param parent      Pointer to parent functional unit of specified child.
 \param child       Pointer to child functional unit to add.
 \param inst_name   Name of new functional unit instance.
 \param range       For array of instances, specifies the name range.
 \param resolve     If set to TRUE, resolve any added instance.
 \param child_gend  If set to TRUE, specifies that child is a generated instance and should only be added once

 \return Returns TRUE if specified instance was successfully added to the specified instance tree;
         otherwise, returns FALSE.
 
 Adds the child functional unit to the child functional unit pointer list located in
 the functional unit specified by the scope of parent in the functional unit instance
 tree pointed to by root.  This function is used by the db_add_instance
 function during the parsing stage.
*/
bool instance_parse_add( funit_inst** root, func_unit* parent, func_unit* child, char* inst_name, vector_width* range,
                         bool resolve, bool child_gend ) {
  
  bool        retval = TRUE;  /* Return value for this function */
  funit_inst* inst;           /* Temporary pointer to functional unit instance to add to */
  funit_inst* cinst;          /* Pointer to instance of child functional unit */
  int         i;              /* Loop iterator */
  int         ignore;         /* Number of matched instances to ignore */

  if( *root == NULL ) {

    *root = instance_create( child, inst_name, range );

  } else {

    assert( parent != NULL );

    i      = 0;
    ignore = 0;

    /*
     Check to see if the child functional unit has already been parsed and, if so, find
     one of its instances for copying the instance tree below it.
    */
    cinst = instance_find_by_funit( *root, child, &ignore);
    
    /* Filename will be set to a value if the functional unit has been parsed */
    if( (cinst != NULL) && (cinst->funit->filename != NULL) ) { 

      ignore = 0;
      while( (ignore >= 0) && ((inst = instance_find_by_funit( *root, parent, &ignore )) != NULL) ) {
        instance_copy( cinst, inst, inst_name, range, resolve );
        i++;
        ignore = child_gend ? -1 : i;
      }

    } else {

      ignore = 0;
      while( (ignore >= 0) && ((inst = instance_find_by_funit( *root, parent, &ignore )) != NULL) ) {
        cinst = instance_add_child( inst, child, inst_name, range, resolve );
        i++;
        ignore = (child_gend && (cinst != NULL)) ? -1 : i;
      }

    }

    /* Everything went well with the add if we found at least one parent instance */
    retval = (i > 0);

  }

  return( retval );

}

/*!
 \param curr  Pointer to current instance to resolve

 \return Returns TRUE if instance was resolved; otherwise, returns FALSE.

 Checks the given instance to see if a range was specified in its instantiation.  If
 a range was found, create all of the instances for this range and add them to the instance
 tree.
*/
bool instance_resolve_inst( funit_inst* root, funit_inst* curr ) {

  int   width = -1;  /* Width of the instance range */
  int   lsb;         /* LSB of the instance range */
  int   big_endian;  /* Unused */
  char* name_copy;   /* Copy of the instance name being resolved */
  char* new_name;    /* New hierarchical name of the instance(s) being resolved */
  int   i;           /* Loop iterator */

  assert( curr != NULL );

  if( curr->range != NULL ) {

    /* Get LSB and width information */
    static_expr_calc_lsb_and_width_post( curr->range->left, curr->range->right, &width, &lsb, &big_endian );
    assert( width != -1 );
    assert( lsb   != -1 );

    /* Remove the range information from this instance */
    static_expr_dealloc( curr->range->left,  FALSE );
    static_expr_dealloc( curr->range->right, FALSE );
    free_safe( curr->range );
    curr->range = NULL;

    /* Copy and deallocate instance name */
    name_copy = strdup_safe( curr->name, __FILE__, __LINE__ );
    free_safe( curr->name );

    /* For the first instance, just modify the name */
    new_name   = (char*)malloc_safe( (strlen( name_copy ) + 23), __FILE__, __LINE__ );
    snprintf( new_name, (strlen( name_copy ) + 23), "%s[%d]", name_copy, lsb );
    curr->name = strdup_safe( new_name, __FILE__, __LINE__ );

    /* For all of the rest of the instances, do the instance_parse_add function call */
    for( i=1; i<width; i++ ) {

      /* Create the new name */
      snprintf( new_name, (strlen( curr->name ) + 23), "%s[%d]", name_copy, (lsb + i) );

      /* Add the instance */
      instance_parse_add( &root, ((curr->parent == NULL) ? NULL : curr->parent->funit), curr->funit, new_name, NULL, TRUE, FALSE );

    }

    /* Deallocate the new_name and name_copy pointers */
    free_safe( name_copy );
    free_safe( new_name );

  }
  
  return( width != -1 );

}

/*!
 \param root  Pointer to root of instance tree
 \param curr  Pointer to current instance

 Recursively iterates through the entire instance tree
*/
void instance_resolve_helper( funit_inst* root, funit_inst* curr ) {

  funit_inst* curr_child;  /* Pointer to current child */

  if( curr != NULL ) {

    /* Resolve all children first */
    curr_child = curr->child_head;
    while( curr_child != NULL ) {
      instance_resolve_helper( root, curr_child );
      curr_child = curr_child->next;
    }

    /* Now resolve this instance */
    instance_resolve_inst( root, curr );

  }

}

/*!
 \param root  Pointer to current functional unit instance to resolve.

 Recursively iterates through entire instance tree, resolving any instance arrays that are found.
*/
void instance_resolve( funit_inst* root ) {

  /* Resolve all instance names */
  instance_resolve_helper( root, root );

  /* Now resolve all of the rest of the parameters */
  // param_resolve( root );

}

/*!
 \param root       Pointer to root instance of functional unit instance tree.
 \param parent     String scope of parent instance.
 \param child      Pointer to child functional unit to add to specified parent's child list.
 \param inst_name  Instance name of this child functional unit instance.

 \return Returns TRUE if instance was added to the specified functional unit instance tree; otherwise,
         returns FALSE (indicates that the instance is from a different hierarchy).

 Adds the child functional unit to the child functional unit pointer list located in
 the functional unit specified by the scope of parent in the functional unit instance
 tree pointed to by root.  This function is used by the db_read
 function during the CDD reading stage.
*/ 
bool instance_read_add( funit_inst** root, char* parent, func_unit* child, char* inst_name ) {

  bool        retval = TRUE;  /* Return value for this function */
  funit_inst* inst;           /* Temporary pointer to functional unit instance to add to */
  funit_inst* new_inst;       /* Pointer to new functional unit instance to add */

  if( *root == NULL ) {

    *root = instance_create( child, inst_name, NULL );

  } else {

    assert( parent != NULL );
  
    if( (inst = instance_find_scope( *root, parent, TRUE )) != NULL ) {

      /* Create new instance */
      new_inst = instance_create( child, inst_name, NULL );

      if( inst->child_head == NULL ) {
        inst->child_head = new_inst;
        inst->child_tail = new_inst;
      } else {
        inst->child_tail->next = new_inst;
        inst->child_tail       = new_inst;
      }

      /* Set parent pointer of new instance */
      new_inst->parent = inst;

    } else {

      /* Unable to find parent of this child, needs to be added to a different instance tree */
      retval = FALSE;

    }
 
  }

  return( retval );

}

/*!
 \param root         Root of functional unit instance tree to write.
 \param file         Output file to display contents to.
 \param scope        Scope of this functional unit.
 \param parse_mode   Specifies if we are parsing or scoring.
 \param report_save  Specifies if we are saving a CDD file after modifying it with the report command

 Calls each functional unit display function in instance tree, starting with
 the root functional unit and ending when all of the leaf functional units are output.
 Note:  the function that calls this function originally should set
 the value of scope to NULL.
*/
void instance_db_write( funit_inst* root, FILE* file, char* scope, bool parse_mode, bool report_save ) {

  char        tscope[4096];  /* New scope of functional unit to write */
  funit_inst* curr;          /* Pointer to current child functional unit instance */
  exp_link*   expl;          /* Pointer to current expression link */
#ifndef VPI_ONLY
  gitem_link* gil;           /* Pointer to current generate item link */
#endif

  assert( scope != NULL );

  curr = parse_mode ? root : NULL;

  /* If we are in parse mode, re-issue expression IDs (we use the ulid field since it is not used in parse mode) */
  if( parse_mode ) {

    /* First issue IDs to the expressions within the functional unit */
    expl = root->funit->exp_head;
    while( expl != NULL ) {
      expl->exp->ulid = curr_expr_id;
      curr_expr_id++;
      expl = expl->next;
    }

#ifndef VPI_ONLY
    /* Then issue IDs to any generated expressions */
    gil = root->gitem_head;
    while( gil != NULL ) {
      gen_item_assign_expr_ids( gil->gi );
      gil = gil->next;
    }
#endif

  }

  /* Display root functional unit */
  funit_db_write( root->funit, scope, file, curr, report_save );

  /* Display children */
  curr = root->child_head;
  while( curr != NULL ) {
    snprintf( tscope, 4096, "%s.%s", scope, curr->name );
    instance_db_write( curr, file, tscope, parse_mode, report_save );
    curr = curr->next;
  }

}

/*!
 \param root  Pointer to current instance root

 Recursively iterates through instance tree, integrating all unnamed scopes that do
 not contain any signals into their parent modules.  This function only gets called
 during the report command.
*/
void instance_flatten( funit_inst* root ) {

  funit_inst* child;              /* Pointer to current child instance */
  funit_inst* last_child = NULL;  /* Pointer to the last child instance */
  funit_inst* tmp;                /* Temporary pointer to functional unit instance */
  funit_inst* grandchild;         /* Pointer to current granchild instance */
  char        back[4096];         /* Last portion of functional unit name */
  char        rest[4096];         /* Holds the rest of the functional unit name */

  if( root != NULL ) {

    /* Iterate through child instances */
    child = root->child_head;
    while( child != NULL ) {

      /* First, flatten the child instance */
      instance_flatten( child );

      /* Get the last portion of the child instance before this functional unit is removed */
      scope_extract_back( child->funit->name, back, rest );

      /*
       Next, fold this child instance into this instance if it is an unnamed scope
       that has no signals.
      */
      if( funit_is_unnamed( child->funit ) && (child->funit->sig_head == NULL) ) {

        /* Converge the child functional unit into this functional unit */
        funit_converge( root->funit, child->funit );

        /* Remove this child from the child list of this instance */
        if( child == root->child_head ) {
          if( child == root->child_tail ) {
            root->child_head = root->child_tail = NULL;
          } else {
            root->child_head = child->next;
          }
        } else {
          if( child == root->child_tail ) {
            root->child_tail = last_child;
            root->child_tail->next = NULL;
          } else {
            last_child->next = child->next;
          }
        }

        /* Add grandchildren to this parent */
        grandchild = child->child_head;
        if( grandchild != NULL ) {
          while( grandchild != NULL ) {
            grandchild->parent = root;
            funit_flatten_name( grandchild->funit, back );
            grandchild = grandchild->next;
          }
          if( root->child_head == NULL ) {
            root->child_head = root->child_tail = child->child_head;
          } else {
            root->child_tail->next = child->child_head;
            root->child_tail       = child->child_head;
          }
        }

        tmp   = child;
        child = child->next;

        /* Deallocate child instance */
        instance_dealloc_single( tmp );
      
      } else {

        last_child = child;
        child = child->next;

      }


    }

  }

}

/*!
 \param root  Pointer to root instance to remove statements from
 \param stmt  Pointer to statement to match

 TBD
*/
void instance_remove_stmt_blks_calling_stmt( funit_inst* root, statement* stmt ) {

  funit_inst* curr_child;  /* Pointer to current child instance to parse */
#ifndef VPI_ONLY
  gitem_link* gil;         /* Pointer to current generate item link */
#endif

  if( root != NULL ) {

    /* First, handle the current functional unit */
    funit_remove_stmt_blks_calling_stmt( root->funit, stmt );

#ifndef VPI_ONLY
    /* Second, handle all generate items in this instance */
    gil = root->gitem_head;
    while( gil != NULL ) {
      gen_item_remove_if_contains_expr_calling_stmt( gil->gi, stmt );
      gil = gil->next;
    }
#endif

    /* Parse children */
    curr_child = root->child_head;
    while( curr_child != NULL ) {
      instance_remove_stmt_blks_calling_stmt( curr_child, stmt );
      curr_child = curr_child->next;
    }

  }

}

/*!
 \param root  Pointer to functional unit instance to remove expression from
 \param exp   Pointer to expression to remove from list

 Recursively traverses the given instance tree, removing the given expression (and its sub
*/
void instance_remove_parms_with_expr( funit_inst* root, statement* stmt ) {

  funit_inst* curr_child;  /* Pointer to current child instance to traverse */
  inst_parm*  iparm;       /* Pointer to current instance parameter */
  exp_link*   expl;        /* Pointer to current expression link */
  exp_link*   texpl;       /* Temporary pointer to current expression link */

  /* Search for the given expression within the given instance parameter */
  iparm = root->param_head;
  while( iparm != NULL ) {
    if( iparm->sig != NULL ) {
      expl = iparm->sig->exp_head;
      while( expl != NULL ) {
        texpl = expl;
        expl  = expl->next;
        if( expression_find_expr( stmt->exp, texpl->exp ) ) {
          if( iparm->mparm != NULL ) {
            exp_link_remove( texpl->exp, &(iparm->mparm->exp_head), &(iparm->mparm->exp_tail), FALSE );
          }
          exp_link_remove( texpl->exp, &(iparm->sig->exp_head), &(iparm->sig->exp_tail), FALSE );
        }
      }
    }
    iparm = iparm->next;
  }

  /* Traverse children */
  curr_child = root->child_head;
  while( curr_child != NULL ) {
    instance_remove_parms_with_expr( curr_child, stmt );
    curr_child = curr_child->next;
  }

}

/*!
 \param inst  Pointer to instance to deallocate memory for

 Deallocates all memory allocated for the given instance.
*/
void instance_dealloc_single( funit_inst* inst ) {

  if( inst != NULL ) {

    /* Free up memory allocated for name */
    free_safe( inst->name );

    /* Free up memory allocated for statistic, if necessary */
    free_safe( inst->stat );

    /* Free up memory for range, if necessary */
    if( inst->range != NULL ) {
      static_expr_dealloc( inst->range->left,  FALSE );
      static_expr_dealloc( inst->range->right, FALSE );
      free_safe( inst->range );
    }

    /* Deallocate memory for instance parameter list */
    inst_parm_dealloc( inst->param_head, TRUE );

#ifndef VPI_ONLY
    /* Deallocate memory for generate item list */
    gitem_link_delete_list( inst->gitem_head, FALSE );
#endif

    /* Free up memory for this functional unit instance */
    free_safe( inst );

  }

}

/*!
 \param root  Pointer to root instance of functional unit instance tree to remove.

 Recursively traverses instance tree, deallocating heap memory used to store the
 the tree.
*/
void instance_dealloc_tree( funit_inst* root ) {

  funit_inst* curr;  /* Pointer to current instance to evaluate */
  funit_inst* tmp;   /* Temporary pointer to instance */

  if( root != NULL ) {

    /* Remove instance's children first */
    curr = root->child_head;
    while( curr != NULL ) {
      tmp = curr->next;
      instance_dealloc_tree( curr );
      curr = tmp;
    }

    /* Deallocate the instance memory */
    instance_dealloc_single( root );

  }

}

/*!
 \param root   Root of functional unit instance tree.
 \param scope  Scope of functional unit to remove from tree.
    
 Searches tree for specified functional unit.  If the functional unit instance is found,
 the functional unit instance is removed from the tree along with all of its
 child functional unit instances.
*/
void instance_dealloc( funit_inst* root, char* scope ) {
  
  funit_inst* inst;        /* Pointer to instance to remove */
  funit_inst* curr;        /* Pointer to current child instance to remove */
  funit_inst* last;        /* Last current child instance */
  char        back[256];   /* Highest level of hierarchy in hierarchical reference */
  char        rest[4096];  /* Rest of scope value */
  
  assert( root  != NULL );
  assert( scope != NULL );
  
  if( scope_compare( root->name, scope ) ) {
    
    /* We are the root so just remove the whole tree */
    instance_dealloc_tree( root );
    
  } else {
    
    /* 
     Find parent instance of given scope and remove this instance
     from its child list.
    */  
    scope_extract_back( scope, back, rest );
    assert( rest[0] != '\0' );

    inst = instance_find_scope( root, rest, TRUE );
    assert( inst != NULL );

    curr = inst->child_head;
    last = NULL;
    while( (curr != NULL) && !scope_compare( curr->name, scope ) ) {
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

  }

}

/*
 $Log$
 Revision 1.68  2007/03/15 22:39:05  phase1geo
 Fixing bug in unnamed scope binding.

 Revision 1.67  2006/12/19 06:06:05  phase1geo
 Shortening unnamed scope name from $unnamed_%d to $u%d.  Also fixed a few
 bugs in the instance_flatten function (still more debug work to go here).
 Checkpointing.

 Revision 1.66  2006/12/19 05:23:39  phase1geo
 Added initial code for handling instance flattening for unnamed scopes.  This
 is partially working at this point but still needs some debugging.  Checkpointing.

 Revision 1.65  2006/10/16 21:34:46  phase1geo
 Increased max bit width from 1024 to 65536 to allow for more room for memories.
 Fixed issue with enumerated values being explicitly assigned unknown values and
 created error output message when an implicitly assigned enum followed an explicitly
 assign unknown enum value.  Fixed problem with generate blocks in different
 instantiations of the same module.  Fixed bug in parser related to setting the
 curr_packed global variable correctly.  Added enum2 and enum2.1 diagnostics to test
 suite to verify correct enumerate behavior for the changes made in this checkin.
 Full regression now passes.

 Revision 1.64  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.63  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.62  2006/10/09 17:54:19  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.61  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.60  2006/09/22 04:23:04  phase1geo
 More fixes to support new signal range structure.  Still don't have full
 regressions passing at the moment.

 Revision 1.59  2006/09/08 22:39:50  phase1geo
 Fixes for memory problems.

 Revision 1.58  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.57  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.56  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.55  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.54  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.53  2006/07/21 20:12:46  phase1geo
 Fixing code to get generated instances and generated array of instances to
 work.  Added diagnostics to verify correct functionality.  Full regression
 passes.

 Revision 1.52  2006/07/18 19:03:21  phase1geo
 Sync'ing up to the scoping fixes from the 0.4.6 stable release.

 Revision 1.51  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.50  2006/07/12 22:16:18  phase1geo
 Fixing hierarchical referencing for instance arrays.  Also attempted to fix
 a problem found with unary1; however, the generated report coverage information
 does not look correct at this time.  Checkpointing what I have done for now.

 Revision 1.49  2006/07/11 04:59:08  phase1geo
 Reworking the way that instances are being generated.  This is to fix a bug and
 pave the way for generate loops for instances.  Code not working at this point
 and may cause serious problems for regression runs.

 Revision 1.48  2006/07/10 19:30:55  phase1geo
 Fixing bug in instance.c that ignored the LSB information for an instance
 array (this also needs to be fixed for the 0.4.6 stable release).  Added
 diagnostic to verify correctness of this behavior.  Also added case statement
 to the generate parser.

 Revision 1.47  2006/07/10 03:05:04  phase1geo
 Contains bug fixes for memory leaks and segmentation faults.  Also contains
 some starting code to support generate blocks.  There is absolutely no
 functionality here, however.

 Revision 1.46  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.45  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.44  2006/05/25 12:11:01  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.43  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.42  2006/04/08 03:23:28  phase1geo
 Adding support for CVER simulator VPI support.  I think I may have also fixed
 support for VCS also.  Recreated configuration/Makefiles with newer version of
 auto* tools.

 Revision 1.41  2006/04/07 22:31:07  phase1geo
 Fixes to get VPI to work with VCS.  Getting close but still some work to go to
 get the callbacks to start working.

 Revision 1.40.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.40  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.39  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.38  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.37  2006/01/24 23:24:38  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.36  2006/01/20 22:50:50  phase1geo
 Code cleanup.

 Revision 1.35  2006/01/20 22:44:51  phase1geo
 Moving parameter resolution to post-bind stage to allow static functions to
 be considered.  Regression passes without static function testing.  Static
 function support still has some work to go.  Checkpointing.

 Revision 1.34  2006/01/20 19:15:23  phase1geo
 Fixed bug to properly handle the scoping of parameters when parameters are created/used
 in non-module functional units.  Added param10*.v diagnostics to regression suite to
 verify the behavior is correct now.

 Revision 1.33  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.32  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.31  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.30  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.29  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.28  2003/01/14 05:52:16  phase1geo
 Fixing bug related to copying instance trees in modules that were previously
 parsed.  Added diagnostic param7.v to testsuite and regression.  Full
 regression passes.

 Revision 1.27  2003/01/13 14:30:05  phase1geo
 Initial code to fix problem with missing instances in CDD files.  Instance
 now shows up but parameters not calculated correctly.  Another checkin to
 follow will contain full fix.

 Revision 1.26  2003/01/04 03:56:27  phase1geo
 Fixing bug with parameterized modules.  Updated regression suite for changes.

 Revision 1.25  2003/01/03 05:53:19  phase1geo
 Removing unnecessary spaces.

 Revision 1.24  2002/12/05 14:45:17  phase1geo
 Removing assertion error from instance6.1 failure; however, this case does not
 work correctly according to instance6.2.v diagnostic.  Added @(...) output in
 report command for edge-triggered events.  Also fixed bug where a module could be
 parsed more than once.  Full regression does not pass at this point due to
 new instance6.2.v diagnostic.

 Revision 1.23  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.22  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.21  2002/10/31 23:13:51  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.20  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.19  2002/10/13 13:55:52  phase1geo
 Fixing instance depth selection and updating all configuration files for
 regression.  Full regression now passes.

 Revision 1.18  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.17  2002/09/25 05:36:08  phase1geo
 Initial version of parameter support is now in place.  Parameters work on a
 basic level.  param1.v tests this basic functionality and param1.cdd contains
 the correct CDD output from handling parameters in this file.  Yeah!

 Revision 1.16  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.15  2002/09/23 01:37:45  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.14  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.13  2002/09/21 04:11:32  phase1geo
 Completed phase 1 for adding in parameter support.  Main code is written
 that will create an instance parameter from a given module parameter in
 its entirety.  The next step will be to complete the module parameter
 creation code all the way to the parser.  Regression still passes and
 everything compiles at this point.

 Revision 1.12  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.11  2002/09/06 03:05:28  phase1geo
 Some ideas about handling parameters have been added to these files.  Added
 "Special Thanks" section in User's Guide for acknowledgements to people
 helping in project.

 Revision 1.10  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.9  2002/07/18 05:50:45  phase1geo
 Fixes should be just about complete for instance depth problems now.  Diagnostics
 to help verify instance handling are added to regression.  Full regression passes.
*/

