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
 \file     instance.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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

#include "arc.h"
#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "instance.h"
#include "link.h"
#include "param.h"
#include "static.h"
#include "util.h"


extern int          curr_expr_id;
extern db**         db_list;
extern unsigned int curr_db;
extern char         user_msg[USER_MSG_LENGTH];
extern bool         debug_mode;


/*!
 Signal ID that is used for identification purposes (each signal will receive a unique ID).
*/
int curr_sig_id = 1;


static bool instance_resolve_inst( funit_inst*, funit_inst* );
static void instance_dealloc_single( funit_inst* );


/*!
 Helper function for the \ref instance_display_tree function.
*/
static void instance_display_tree_helper(
  funit_inst* root,   /*!< Pointer to functional unit instance to display */
  char*       prefix  /*!< Prefix string to be used when outputting (used to indent children) */
) { PROFILE(INSTANCE_DISPLAY_TREE_HELPER);

  char         sp[4096];  /* Contains prefix for children */
  funit_inst*  curr;      /* Pointer to current child instance */
  unsigned int rv;        /* Return value from snprintf calls */

  assert( root != NULL );

  /* Get printable version of this instance and functional unit name */
  if( root->funit != NULL ) {
    char* piname = scope_gen_printable( root->name );
    char* pfname = scope_gen_printable( root->funit->name );
    /*@-formatcode@*/
    printf( "%s%s [%d, %u, %d] (%s) - %p (ign: %hhu, gend: %hhu)\n", prefix, piname, root->id, root->ppfline, root->fcol, pfname, root, root->suppl.ignore, root->suppl.gend_scope );
    /*@=formatcode@*/
    free_safe( piname, (strlen( piname ) + 1) );
    free_safe( pfname, (strlen( pfname ) + 1) );
  } else {
    char* piname = scope_gen_printable( root->name );
    /*@-formatcode@*/
    printf( "%s%s [%d, %u, %d] () - %p (ign: %hhu, gend: %hhu)\n", prefix, piname, root->id, root->ppfline, root->fcol, root, root->suppl.ignore, root->suppl.gend_scope );
    /*@=formatcode@*/
    free_safe( piname, (strlen( piname ) + 1) );
  }

  /* Calculate prefix */
  rv = snprintf( sp, 4096, "%s   ", prefix );
  assert( rv < 4096 );

  /* Display our children */
  curr = root->child_head;
  while( curr != NULL ) {
    instance_display_tree_helper( curr, sp );
    curr = curr->next;
  }


  PROFILE_END;

}

/*!
 Displays the given instance tree to standard output in a hierarchical format.  Shows
 instance names as well as associated module name.
*/
void instance_display_tree(
  funit_inst* root  /*!< Pointer to root instance to display */
) { PROFILE(INSTANCE_DISPLAY_TREE);

  instance_display_tree_helper( root, "" );

  PROFILE_END;

}

/*!
 \return Returns pointer to newly created functional unit instance.

 Creates a new functional unit instance from heap, initializes its data and
 returns a pointer to it.
*/
funit_inst* instance_create(
  func_unit*    funit,       /*!< Pointer to functional unit to store in this instance */
  char*         inst_name,   /*!< Instantiated name of this instance */
  unsigned int  ppfline,     /*!< First line of instance from preprocessor */
  int           fcol,        /*!< First column of instantiation */
  bool          name_diff,   /*!< Specifies if the inst_name provided is not accurate due to merging */
  bool          ignore,      /*!< Specifies that this instance is just a placeholder, not to be written to CDD */
  bool          gend_scope,  /*!< Specifies if this instance is a generated scope */
  vector_width* range        /*!< For arrays of instances, contains range information for this array */
) { PROFILE(INSTANCE_CREATE);

  funit_inst* new_inst;  /* Pointer to new functional unit instance */

  new_inst                   = (funit_inst*)malloc_safe( sizeof( funit_inst ) );
  new_inst->funit            = funit;
  new_inst->name             = strdup_safe( inst_name );
  new_inst->id               = -1;
  new_inst->ppfline          = ppfline;
  new_inst->fcol             = fcol;
  new_inst->suppl.name_diff  = name_diff;
  new_inst->suppl.ignore     = ignore;
  new_inst->suppl.gend_scope = gend_scope;
  new_inst->stat             = NULL;
  new_inst->param_head       = NULL;
  new_inst->param_tail       = NULL;
  new_inst->gitem_head       = NULL;
  new_inst->gitem_tail       = NULL;
  new_inst->parent           = NULL;
  new_inst->child_head       = NULL;
  new_inst->child_tail       = NULL;
  new_inst->next             = NULL;

  /* Create range (get a copy since this memory is managed by the parser) */
  if( range == NULL ) {
    new_inst->range = NULL;
  } else {
    assert( range->left  != NULL );
    assert( range->right != NULL );
    new_inst->range             = (vector_width*)malloc_safe( sizeof( vector_width ) );
    new_inst->range->left       = (static_expr*)malloc_safe( sizeof( static_expr ) );
    new_inst->range->left->num  = range->left->num;
    new_inst->range->left->exp  = range->left->exp;
    new_inst->range->right      = (static_expr*)malloc_safe( sizeof( static_expr ) );
    new_inst->range->right->num = range->right->num;
    new_inst->range->right->exp = range->right->exp;
  }

  PROFILE_END;

  return( new_inst );

}

#ifndef RUNLIB
/*!
 Assign ID numbers to all instances in the design.
*/
void instance_assign_ids(
  funit_inst* root,    /*!< Pointer to the current instance to assign an ID to */
  int*        curr_id  /*!< Pointer to current ID */
) { PROFILE(INSTANCE_ASSIGN_IDS);

  if( root != NULL ) {

    bool stop_recursive = ((root->funit != NULL) && (root->funit->suppl.part.type == FUNIT_NO_SCORE)) || root->suppl.ignore;

    /* Only create an instance ID if this will be written to the CDD file */
    if( (root->funit == NULL) || !stop_recursive ) {
      root->id = (*curr_id)++;
    }

    /* Assign children first */
    if( !stop_recursive ) {
      funit_inst* child = root->child_head;
      while( child != NULL ) {
        instance_assign_ids( child, curr_id );
        child = child->next;
      }
    }

  }

  PROFILE_END;

}

/*!
 Recursively travels up to the root of the instance tree, building the scope
 string as it goes.  When the root instance is reached, the string is returned.
 Assumes that scope is initialized to the NULL character.
*/
void instance_gen_scope(
  char*       scope,   /*!< String pointer to store generated scope (assumed to be allocated) */
  funit_inst* leaf,    /*!< Pointer to leaf instance in scope */
  bool        flatten  /*!< Causes all unnamed scopes to be removed from generated scope if set to TRUE */
) { PROFILE(INSTANCE_GEN_SCOPE);

  if( leaf != NULL ) {

    /* Call parent instance first */
    instance_gen_scope( scope, leaf->parent, flatten );

    if( !flatten || !db_is_unnamed_scope( leaf->name ) ) {
      if( scope[0] != '\0' ) {
        strcat( scope, "." );
        strcat( scope, leaf->name );
      } else {
        strcpy( scope, leaf->name );
      }
    }

  }

  PROFILE_END;

}

/*!
 Recreates the Verilator flattened scope for the given instance.
*/
void instance_gen_verilator_scope(
  char*       scope,
  funit_inst* leaf
) { PROFILE(INSTANCE_GEN_VERILATOR_SCOPE);

  /* Do not look at the first scope (TOP) */
  if( leaf != NULL ) {

    /* Call parent instance first */
    instance_gen_verilator_scope( scope, leaf->parent );

    if( scope[0] != '\0' ) {
      char         name[256];
      int          index;
      char         index_str[30];
      unsigned int rv;
      strcat( scope, "__DOT__" );
      if( sscanf( leaf->name, "%[^[][%d]", name, &index ) == 2 ) {
        strcat( scope, name );
        strcat( scope, "__BRA__" );
        rv = snprintf( index_str, 30, "%d", index );
        assert( rv < 30 );
        strcat( scope, index_str );
        strcat( scope, "__KET__" );
      } else if( sscanf( leaf->name, "u$%d", &index ) == 1 ) {
        strcat( scope, "u__024" );
        rv = snprintf( index_str, 30, "%d", index );
        assert( rv < 30 );
        strcat( scope, index_str );
      } else {
        strcat( scope, leaf->name );
      }
    } else {
      strcpy( scope, leaf->name );
    }

  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \return Returns TRUE if the given instance name and instance match.  If the specified instance is
         a part of an array of instances and the base name matches the base name of inst_name, we
         also check to make sure that the index of inst_name falls within the legal range of this
         instance.
*/
static bool instance_compare(
  char*             inst_name,  /*!< Instance name to compare to this instance's name (may contain array information) */
  const funit_inst* inst        /*!< Pointer to instance to compare name against */
) { PROFILE(INSTANCE_COMPARE);

  bool         retval = FALSE;  /* Return value of this function */
  char         bname[4096];     /* Base name of inst_name */
  int          index;           /* Index of inst_name */
  unsigned int width;           /* Width of instance range */
  int          lsb;             /* LSB of instance range */
  int          big_endian;      /* Specifies endianness */

  /* If this instance has a range, handle it */
  if( inst->range != NULL ) {

    /* Extract the index portion of inst_name if there is one */
    if( sscanf( inst_name, "%[a-zA-Z0-9_]\[%d]", bname, &index ) == 2 ) {
      
      /* If the base names compare, check that the given index falls within this instance range */
      if( scope_compare( bname, inst->name ) ) {

        /* Get range information from instance */
        static_expr_calc_lsb_and_width_post( inst->range->left, inst->range->right, &width, &lsb, &big_endian );
        assert( width != 0 );
        assert( lsb   != -1 );

        retval = (index >= lsb) && (index < (lsb + (int)width));

      }
      
    }

  } else {

    retval = scope_compare( inst_name, inst->name );

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns pointer to functional unit instance found by scope.
 
 Searches the specified functional unit instance tree for the specified
 scope.  When the functional unit instance is found, a pointer to that
 functional unit instance is passed back to the calling function.
*/
funit_inst* instance_find_scope(
  funit_inst* root,       /*!< Root of funit_inst tree to parse for scope */
  char*       scope,      /*!< Scope to search for */
  bool        rm_unnamed  /*!< Set to TRUE if we need to remove unnamed scopes */
) { PROFILE(INSTANCE_FIND_SCOPE);
 
  char        front[256];   /* Front of scope value */
  char        rest[4096];   /* Rest of scope value */
  funit_inst* inst = NULL;  /* Pointer to found instance */
  funit_inst* child;        /* Pointer to current child instance being traversed */

  assert( root != NULL );

  /* First extract the front scope */
  scope_extract_front( scope, front, rest );

  /* Skip this instance and move onto the children if we are an unnamed scope that does not contain signals */
  if( rm_unnamed && !db_is_unnamed_scope( front ) && funit_is_unnamed( root->funit ) ) {
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

  PROFILE_END;

  return( inst );

}

/*!
 \return Returns pointer to functional unit instance found by scope.
 
 Searches the specified functional unit instance tree for the specified
 functional unit.  When a functional unit instance is found that points to the specified
 functional unit and the ignore value is 0, a pointer to that functional unit instance is 
 passed back to the calling function; otherwise, the ignore count is
 decremented and the searching continues.
*/
funit_inst* instance_find_by_funit(
            funit_inst*      root,   /*!< Pointer to root functional unit instance of tree */
            const func_unit* funit,  /*!< Pointer to functional unit to find in tree */
  /*@out@*/ int*             ignore  /*!< Pointer to number of matches to ignore */
) { PROFILE(INSTANCE_FIND_BY_FUNIT);

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

  PROFILE_END;

  return( match_inst );

}

#ifndef RUNLIB
/*!
 Recursively searches the given instance tree, setting match_inst and matches if a matched functional unit name was found.
*/
static void instance_find_by_funit_name(
            funit_inst*   root,        /*!< Pointer to root functional unit instance to search */
            const char*   funit_name,  /*!< Name of module to find */
  /*@out@*/ funit_inst**  match_inst,  /*!< Pointer to matched functional unit instance */
  /*@out@*/ unsigned int* matches      /*!< Specifies the number of matched modules */
) { PROFILE(INSTANCE_FIND_BY_FUNIT_NAME_IF_ONE_HELPER);

  if( root != NULL ) {

    funit_inst* child;

    if( strcmp( root->funit->name, funit_name ) == 0 ) {
      (*matches)++;
      *match_inst = root;
    }

    child = root->child_head;
    while( child != NULL ) {
      instance_find_by_funit_name( child, funit_name, match_inst, matches );
      child = child->next;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the found instance, if one exists; otherwise, returns NULL.
*/
static funit_inst* instance_find_by_funit_name_if_one(
  funit_inst* root,       /*!< Pointer to root functional unit instance to search */
  const char* funit_name  /*!< Name of module to find */
) { PROFILE(INSTANCE_FIND_BY_FUNIT_NAME_IF_ONE);

  funit_inst*  match_inst = NULL;
  unsigned int matches    = 0;

  instance_find_by_funit_name( root, funit_name, &match_inst, &matches );

  PROFILE_END;

  return( (matches == 1) ? match_inst : NULL );

}

/*!
 \return Returns the pointer to the signal that contains the same exclusion ID.

 Recursively searches the given instance tree to find the signal that has the same
 exclusion ID as the one specified.
*/
vsignal* instance_find_signal_by_exclusion_id(
            funit_inst* root,        /*!< Pointer to root instance */
            int         id,          /*!< Exclusion ID to search for */
  /*@out@*/ func_unit** found_funit  /*!< Pointer to functional unit containing this signal */
) { PROFILE(INSTANCE_FIND_SIGNAL_BY_EXCLUSION_ID);
 
  vsignal* sig = NULL;  /* Pointer to found signal */

  if( root != NULL ) {

    if( (root->funit != NULL) &&
        (root->funit->sigs != NULL) &&
        (root->funit->sigs[0]->id <= id) &&
        (root->funit->sigs[root->funit->sig_size-1]->id >= id) ) {

      unsigned int i = 0;

      while( (i < root->funit->sig_size) && (root->funit->sigs[i]->id != id) ) i++;
      assert( i < root->funit->sig_size );
      sig          = root->funit->sigs[i];
      *found_funit = root->funit;

    } else {

      funit_inst* child = root->child_head;
      while( (child != NULL) && ((sig = instance_find_signal_by_exclusion_id( child, id, found_funit )) == NULL) ) {
        child = child->next;
      }

    }
    
  }

  PROFILE_END;

  return( sig );

}

/*!
 \return Returns the pointer to the expression that contains the same exclusion ID. 
                                        
 Recursively searches the given instance tree to find the expression that has the same
 exclusion ID as the one specified.
*/
expression* instance_find_expression_by_exclusion_id(
            funit_inst* root,        /*!< Pointer to root instance */
            int         id,          /*!< Exclusion ID to search for */
  /*@out@*/ func_unit** found_funit  /*!< Pointer to functional unit containing this expression */
) { PROFILE(INSTANCE_FIND_EXPRESSION_BY_EXCLUSION_ID); 
    
  expression* exp = NULL;  /* Pointer to found expression */
    
  if( root != NULL ) {

    if( (root->funit != NULL) &&
        (root->funit->exps != NULL) && 
        (root->funit->exps[0]->id <= id) && 
        (root->funit->exps[root->funit->exp_size-1]->id >= id) ) {

      unsigned int i = 0;

      while( (i < root->funit->exp_size) && (root->funit->exps[i]->id != id) ) i++;
      assert( i < root->funit->exp_size );
      exp          = root->funit->exps[i];
      *found_funit = root->funit;

    } else {

      funit_inst* child = root->child_head;
      while( (child != NULL) && ((exp = instance_find_expression_by_exclusion_id( child, id, found_funit )) == NULL) ) {
        child = child->next;
      }

    }
    
  }
  
  PROFILE_END; 
  
  return( exp );
  
}

/*!
 \return Returns the index of the state transition in the arcs array of the found_fsm in the found_funit that matches the
         given exclusion ID (if one is found); otherwise, returns -1.
*/
int instance_find_fsm_arc_index_by_exclusion_id(
            funit_inst* root,
            int         id,
  /*@out@*/ fsm_table** found_fsm,
  /*@out@*/ func_unit** found_funit
) { PROFILE(INSTANCE_FIND_FSM_ARC_INDEX_BY_EXCLUSION_ID);

  int arc_index = -1;  /* Index of found FSM arc */

  if( root != NULL ) {

    unsigned int i = 0;

    if( root->funit != NULL ) {
      while( (i < root->funit->fsm_size) && ((arc_index = arc_find_arc_by_exclusion_id( root->funit->fsms[i]->table, id )) == -1) ) i++;
    }

    if( arc_index != -1 ) {
      *found_fsm   = root->funit->fsms[i]->table;
      *found_funit = root->funit;
    } else {
      funit_inst* child = root->child_head;
      while( (child != NULL) && ((arc_index = instance_find_fsm_arc_index_by_exclusion_id( child, id, found_fsm, found_funit )) == -1) ) {
        child = child->next;
      }
    }

  }

  PROFILE_END;

  return( arc_index );

}

/*!
 \return Returns pointer to newly created functional unit instance if this instance name isn't already in
         use in the current instance; otherwise, returns NULL.
 
 Generates new instance, adds it to the child list of the inst functional unit
 instance, and resolves any parameters.
*/
static funit_inst* instance_add_child(
  funit_inst*   inst,          /*!< Pointer to instance to add child instance to */
  func_unit*    child,         /*!< Pointer to child functional unit to create instance for */
  char*         name,          /*!< Name of instance to add */
  unsigned int  ppfline,       /*!< First line of instantiation from preprocessor */
  int           fcol,          /*!< First column of instantiation */
  vector_width* range,         /*!< For arrays of instances, contains the range of the instance array */
  bool          resolve,       /*!< Set to TRUE if newly added instance should be immediately resolved */
  bool          ignore_child,  /*!< Set to TRUE if the child to be added should not be written to a CDD file */
  bool          gend_scope     /*!< Set to TRUE if the child scope is a generated scope */
) { PROFILE(INSTANCE_ADD_CHILD);

  funit_inst* new_inst;  /* Pointer to newly created instance to add */

  /* Check to see if this instance already exists */
  new_inst = inst->child_head;
  while( (new_inst != NULL) && ((strcmp( new_inst->name, name ) != 0) || (new_inst->funit != child)) ) {
    new_inst = new_inst->next;
  }

  /* If this instance already exists (unless the existing and new child is a placeholder), don't add it again */
  if( (new_inst == NULL) || (new_inst->suppl.ignore && ignore_child) ) {

    /* Generate new instance */
    new_inst = instance_create( child, name, ppfline, fcol, FALSE, ignore_child, gend_scope, range );

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
      inst_link* instl = db_list[curr_db]->inst_head;
      while( (instl != NULL) && !instance_resolve_inst( instl->inst, new_inst ) ) {
        instl = instl->next;
      }
    }

  } else {

    /* Set the ignore value in the instance to FALSE */
    new_inst->suppl.ignore = FALSE;

    new_inst = NULL;

  }

  PROFILE_END;

  return( new_inst );

}

/*!
 \return Returns a pointer to the newly allocated instance

 Recursively copies the instance tree of from_inst to the instance
 to_inst, allocating memory for the new instances and resolving parameters.
*/
static funit_inst* instance_copy_helper(
  funit_inst*   from_inst,  /*!< Pointer to instance tree to copy */
  funit_inst*   to_inst,    /*!< Pointer to instance to copy tree to */
  char*         name,       /*!< Instance name of current instance being copied */
  unsigned int  ppfline,    /*!< First line of instantiation from the preprocessor */
  int           fcol,       /*!< First column of instantiation */
  vector_width* range,      /*!< For arrays of instances, indicates the array range */
  bool          resolve,    /*!< Set to TRUE if newly added instance should be immediately resolved */
  bool          is_root     /*!< Set to TRUE if the from_inst is the root instance */
) { PROFILE(INSTANCE_COPY_HELPER);

  funit_inst* curr;      /* Pointer to current functional unit instance to copy */
  funit_inst* new_inst;  /* Pointer to newly created functional unit instance */

  assert( from_inst != NULL );
  assert( to_inst   != NULL );
  assert( name      != NULL );

  /* Add new child instance */
  new_inst = instance_add_child( to_inst, from_inst->funit, name, ppfline, fcol, range, resolve,
                                 (from_inst->suppl.ignore && from_inst->suppl.gend_scope && !is_root), from_inst->suppl.gend_scope );

  /* Do not add children if no child instance was created */
  if( new_inst != NULL ) {

    /* Iterate through rest of current child's list of children */
    curr = from_inst->child_head;
    while( curr != NULL ) {
      (void)instance_copy_helper( curr, new_inst, curr->name, curr->ppfline, curr->fcol, curr->range, resolve, FALSE );
      curr = curr->next;
    }

  }

  PROFILE_END;

  return( new_inst );

}

/*!
 \return Returns a pointer to the newly added instance; otherwise, returns NULL if a new instance was not added.

 Recursively copies the instance tree of from_inst to the instance
 to_inst, allocating memory for the new instances and resolving parameters.
*/
funit_inst* instance_copy(
  funit_inst*   from_inst,  /*!< Pointer to instance tree to copy */
  funit_inst*   to_inst,    /*!< Pointer to instance to copy tree to */
  char*         name,       /*!< Instance name of current instance being copied */
  unsigned int  ppfline,    /*!< First line of instantiation from the preprocessor */
  int           fcol,       /*!< First column of instantiation */
  vector_width* range,      /*!< For arrays of instances, indicates the array range */
  bool          resolve     /*!< Set to TRUE if newly added instance should be immediately resolved */
) { PROFILE(INSTANCE_COPY);

  funit_inst* new_inst;

  new_inst = instance_copy_helper( from_inst, to_inst, name, ppfline, fcol, range, resolve, TRUE );

  PROFILE_END;

  return( new_inst );

}

/*!
 \return Returns TRUE if specified instance was successfully added to the specified instance tree;
         otherwise, returns FALSE.
 
 Adds the child functional unit to the child functional unit pointer list located in
 the functional unit specified by the scope of parent in the functional unit instance
 tree pointed to by root.  This function is used by the db_add_instance
 function during the parsing stage.
*/
bool instance_parse_add(
  funit_inst**  root,          /*!< Root funit_inst pointer of functional unit instance tree */
  func_unit*    parent,        /*!< Pointer to parent functional unit of specified child */
  func_unit*    child,         /*!< Pointer to child functional unit to add */
  char*         inst_name,     /*!< Name of new functional unit instance */
  unsigned int  ppfline,       /*!< First line of instantiation from the preprocessor */
  int           fcol,          /*!< First column of instantiation */
  vector_width* range,         /*!< For array of instances, specifies the name range */
  bool          resolve,       /*!< If set to TRUE, resolve any added instance */
  bool          child_gend,    /*!< If set to TRUE, specifies that child is a generated instance and should only be added once */
  bool          ignore_child,  /*!< If set to TRUE, causes the child instance to be ignored when writing to CDD file */
  bool          gend_scope     /*!< If set to TRUE, the child instance is a generated scope */
) { PROFILE(INSTANCE_PARSE_ADD);
  
  bool        retval = TRUE;  /* Return value for this function */
  funit_inst* inst;           /* Temporary pointer to functional unit instance to add to */
  funit_inst* cinst;          /* Pointer to instance of child functional unit */
  int         i;              /* Loop iterator */
  int         ignore;         /* Number of matched instances to ignore */

  if( *root == NULL ) {

    *root = instance_create( child, inst_name, ppfline, fcol, FALSE, ignore_child, gend_scope, range );

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
    if( (cinst != NULL) && (cinst->funit->orig_fname != NULL) ) { 

      ignore = 0;
      while( (ignore >= 0) && ((inst = instance_find_by_funit( *root, parent, &ignore )) != NULL) ) {
        (void)instance_copy( cinst, inst, inst_name, ppfline, fcol, range, resolve );
        i++;
        ignore = child_gend ? -1 : i;
      }

    } else {

      ignore = 0;
      while( (ignore >= 0) && ((inst = instance_find_by_funit( *root, parent, &ignore )) != NULL) ) {
        cinst = instance_add_child( inst, child, inst_name, ppfline, fcol, range, resolve, ignore_child, gend_scope );
        i++;
        ignore = (child_gend && (cinst != NULL)) ? -1 : i;
      }

    }

    /* Everything went well with the add if we found at least one parent instance */
    retval = (i > 0);

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if instance was resolved; otherwise, returns FALSE.

 Checks the given instance to see if a range was specified in its instantiation.  If
 a range was found, create all of the instances for this range and add them to the instance
 tree.
*/
bool instance_resolve_inst(
  funit_inst* root,  /*!< Pointer to root functional unit to traverse */
  funit_inst* curr   /*!< Pointer to current instance to resolve */
) { PROFILE(INSTANCE_RESOLVE_INST);

  unsigned int width = 0;   /* Width of the instance range */
  int          lsb;         /* LSB of the instance range */
  int          big_endian;  /* Unused */
  char*        name_copy;   /* Copy of the instance name being resolved */
  char*        new_name;    /* New hierarchical name of the instance(s) being resolved */
  unsigned int i;           /* Loop iterator */

  assert( curr != NULL );

  if( curr->range != NULL ) {

    unsigned int rv;
    unsigned int slen;

    /* Get LSB and width information */
    static_expr_calc_lsb_and_width_post( curr->range->left, curr->range->right, &width, &lsb, &big_endian );
    assert( width != 0 );
    assert( lsb != -1 );

    /* Remove the range information from this instance */
    static_expr_dealloc( curr->range->left,  FALSE );
    static_expr_dealloc( curr->range->right, FALSE );
    free_safe( curr->range, sizeof( vector_width ) );
    curr->range = NULL;

    /* Copy and deallocate instance name */
    name_copy = strdup_safe( curr->name );
    free_safe( curr->name, (strlen( curr->name ) + 1) );

    /* For the first instance, just modify the name */
    slen     = strlen( name_copy ) + 23;
    new_name = (char*)malloc_safe( slen );
    rv = snprintf( new_name, slen, "%s[%d]", name_copy, lsb );
    assert( rv < slen );
    curr->name = strdup_safe( new_name );

    /* For all of the rest of the instances, do the instance_parse_add function call */
    for( i=1; i<width; i++ ) {

      /* Create the new name */
      rv = snprintf( new_name, slen, "%s[%d]", name_copy, (lsb + i) );
      assert( rv < slen );

      /* Add the instance */
      (void)instance_parse_add( &root, ((curr->parent == NULL) ? NULL : curr->parent->funit), curr->funit, new_name, curr->ppfline, curr->fcol, NULL, TRUE, FALSE, FALSE, FALSE );

    }

    /* Deallocate the new_name and name_copy pointers */
    free_safe( name_copy, (strlen( name_copy ) + 1) );
    free_safe( new_name, slen );

  }

  PROFILE_END;
  
  return( width != 0 );

}

/*!
 Recursively iterates through the entire instance tree
*/
static void instance_resolve_helper(
  funit_inst* root,  /*!< Pointer to root of instance tree */
  funit_inst* curr   /*!< Pointer to current instance */
) { PROFILE(INSTANCE_RESOLVE_HELPER);

  funit_inst* curr_child;  /* Pointer to current child */

  if( curr != NULL ) {

    /* Resolve parameters */
#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Resolving parameters for instance %s...", curr->name );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif
    param_resolve_inst( curr );

    /* Resolve generate blocks */
#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Resolving generate statements for instance %s...", curr->name );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif
    generate_resolve_inst( curr );

    /* Resolve all children */
    curr_child = curr->child_head;
    while( curr_child != NULL ) {
      instance_resolve_helper( root, curr_child );
      curr_child = curr_child->next;
    }

    /* Now resolve this instance's arrays */
#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Resolving instance arrays for instance %s...", curr->name );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif
    (void)instance_resolve_inst( root, curr );

  }

  PROFILE_END;

}

/*!
 Recursively iterates through entire instance tree, resolving any instance arrays that are found.
*/
void instance_resolve(
  funit_inst* root  /*!< Pointer to current functional unit instance to resolve */
) { PROFILE(INSTANCE_RESOLVE);

  /* Resolve all instance names */
  instance_resolve_helper( root, root );

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \return Returns a pointer to the instance that was added if it was added; otherwise, returns NULL (indicates
         that the instance is from a different hierarchy).

 Adds the child functional unit to the child functional unit pointer list located in
 the functional unit specified by the scope of parent in the functional unit instance
 tree pointed to by root.  This function is used by the db_read
 function during the CDD reading stage.
*/ 
funit_inst* instance_read_add(
  funit_inst** root,      /*!< Pointer to root instance of functional unit instance tree */
  char*        parent,    /*!< String scope of parent instance */
  func_unit*   child,     /*!< Pointer to child functional unit to add to specified parent's child list */
  char*        inst_name  /*!< Instance name of this child functional unit instance */
) { PROFILE(INSTANCE_READ_ADD);

  funit_inst* inst;             /* Temporary pointer to functional unit instance to add to */
  funit_inst* new_inst = NULL;  /* Pointer to new functional unit instance to add */

  if( *root == NULL ) {

    *root = new_inst = instance_create( child, inst_name, 0, 0, FALSE, FALSE, FALSE, NULL );

  } else {

    assert( parent != NULL );
  
    if( (inst = instance_find_scope( *root, parent, FALSE )) != NULL ) {

      /* Create new instance */
      new_inst = instance_create( child, inst_name, 0, 0, FALSE, FALSE, FALSE, NULL );

      if( inst->child_head == NULL ) {
        inst->child_head = new_inst;
        inst->child_tail = new_inst;
      } else {
        inst->child_tail->next = new_inst;
        inst->child_tail       = new_inst;
      }

      /* Set parent pointer of new instance */
      new_inst->parent = inst;

    }
 
  }

  PROFILE_END;

  return( new_inst );

}

#ifndef RUNLIB
/*!
 \return Returns TRUE if the two trees were successfully merged.

 Merges to instance trees that have the same instance root.
*/
static bool instance_merge_tree(
  funit_inst* root1,  /*!< Pointer to root of first instance tree to merge */
  funit_inst* root2   /*!< Pointer to root of second instance tree to merge */
) { PROFILE(INSTANCE_MERGE);

  funit_inst* child2;
  funit_inst* last2  = NULL;
  bool        retval = TRUE;

  /* Perform functional unit merging */
  if( root1->funit != NULL ) {
    if( root2->funit != NULL ) {
      if( strcmp( root1->funit->name, root2->funit->name ) == 0 ) {
        funit_merge( root1->funit, root2->funit );
      } else {
        retval = FALSE;
      }
    }
  } else if( root2->funit != NULL ) {
    root1->funit = root2->funit;
    root2->funit = NULL;
  }

  /* Recursively merge the child instances */
  child2 = root2->child_head;
  while( (child2 != NULL) && retval ) {
    funit_inst* child1 = root1->child_head;
    while( (child1 != NULL) && (strcmp( child1->name, child2->name ) != 0) ) {
      child1 = child1->next;
    }
    if( child1 != NULL ) {
      retval = instance_merge_tree( child1, child2 );
      last2  = child2;
      child2 = child2->next;
    } else {
      funit_inst* tmp = child2->next;
      child2->next   = NULL;
      child2->parent = root1;
      if( root1->child_head == NULL ) {
        root1->child_head = child2;
        root1->child_tail = child2;
      } else {
        root1->child_tail->next = child2;
        root1->child_tail       = child2;
      }
      if( last2 == NULL ) {
        root2->child_head = tmp;
        if( tmp == NULL ) {
          root2->child_tail = NULL;
        }
      } else if( tmp == NULL ) {
        root2->child_tail = last2;
        last2->next = NULL;
      } else {
        last2->next = tmp;
      }
      child2 = tmp;
    }
  }

  PROFILE_END;

  return( retval );

}

/*!
 Retrieves the leading hierarchy string and the pointer to the top-most populated instance
 given the specified instance tree.

 \note
 This function requires that the leading_hierarchy string be previously allocated and initialized
 to the NULL string.
*/
void instance_get_leading_hierarchy(
                 funit_inst*  root,               /*!< Pointer to instance tree to get information from */
  /*@out null@*/ char*        leading_hierarchy,  /*!< Leading hierarchy to first populated instance */
  /*@out@*/      funit_inst** top_inst            /*!< Pointer to first populated instance */
) { PROFILE(INSTANCE_GET_LEADING_HIERARCHY);

  if( leading_hierarchy != NULL ) {
    strcat( leading_hierarchy, root->name );
  }

  *top_inst = root;

  if( root->funit == NULL ) {

    do {
      root = root->child_head;
      if( leading_hierarchy != NULL ) {
        strcat( leading_hierarchy, "." );
        strcat( leading_hierarchy, root->name );
      }
      *top_inst = root;
    } while( (root != NULL) && (root->funit == NULL) );

  }

  PROFILE_END;

}

/*!
 Retrieves the leading hierarchy string for a Verilator signal
 given the specified instance tree.

 \note
 This function requires that the leading_hierarchy string be previously allocated and initialized
 to the NULL string.
*/
void instance_get_verilator_leading_hierarchy(
                 funit_inst*  root,               /*!< Pointer to instance tree to get information from */
  /*@out null@*/ char*        leading_hierarchy,  /*!< Leading hierarchy to first populated instance */
  /*@out@*/      funit_inst** top_inst            /*!< Pointer to first populated instance */
) { PROFILE(INSTANCE_GET_VERILATOR_LEADING_HIERARCHY);

  if( leading_hierarchy != NULL ) {
    strcat( leading_hierarchy, root->name );
  }

  *top_inst = root;

  if( root->funit == NULL ) {

    do {
      root = root->child_head;
      if( leading_hierarchy != NULL ) {
        strcat( leading_hierarchy, "__DOT__" );
        strcat( leading_hierarchy, root->name );
      }
      *top_inst = root;
    } while( (root != NULL) && (root->funit == NULL) );

  }

  PROFILE_END;

}

/*!
 Iterates up the scope for both functional unit
*/
static void instance_mark_lhier_diffs(
  funit_inst* root1,
  funit_inst* root2
) { PROFILE(INSTANCE_MARK_LHIER_DIFFS);

  /* Move up the scope hierarchy looking for a difference in instance names */
  while( (root1 != NULL) && (root2 != NULL) && (strcmp( root1->name, root2->name ) == 0) ) {
    root1 = root1->parent;
    root2 = root2->parent;
  }

  /*
   Iterate up root1 instance, setting the name_diff variable to TRUE to specify that the instance name is really
   not accurate since its child tree with a child tree with a differen parent scope.
  */
  while( root1 != NULL ) {
    root1->suppl.name_diff = TRUE;
    root1 = root1->parent;
  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if the second instance tree should have its link removed from the
         instance tree list for the current database; otherwise, returns FALSE.

 Performs comples merges two instance trees into one instance tree.
*/
bool instance_merge_two_trees(
  funit_inst* root1,  /*!< Pointer to first instance tree to merge */
  funit_inst* root2   /*!< Pointer to second instance tree to merge */
) { PROFILE(INSTANCE_MERGE_TWO_TREES);

  bool        retval = TRUE;
  char        lhier1[4096];
  char        lhier2[4096];
  funit_inst* tinst1 = NULL;
  funit_inst* tinst2 = NULL;

  lhier1[0] = '\0';
  lhier2[0] = '\0';

  /* Get leading hierarchy information */
  instance_get_leading_hierarchy( root1, lhier1, &tinst1 );
  instance_get_leading_hierarchy( root2, lhier2, &tinst2 );

  /* If the top-level modules are the same, just merge them */
  if( (tinst1->funit != NULL) && (tinst2->funit != NULL) && (strcmp( tinst1->funit->name, tinst2->funit->name ) == 0) ) {

    if( strcmp( lhier1, lhier2 ) == 0 ) {

      bool rv = instance_merge_tree( tinst1, tinst2 );
      assert( rv );

    } else if( strcmp( root1->name, root2->name ) == 0 ) {

      bool rv = instance_merge_tree( root1, root2 );
      assert( rv );

    } else {
      
      bool rv = instance_merge_tree( tinst1, tinst2 );
      assert( rv );
      instance_mark_lhier_diffs( tinst1, tinst2 );

    }

  /* If the two trees share the same root name, merge them */
  } else if( (strcmp( root1->name, root2->name ) == 0) && instance_merge_tree( root1, root2 ) ) {

    /* We have already merged so there's nothing left to do */

  /* Check to see if the module pointed to by tinst1 exists within the tree of tinst2 */
  } else if( (root2 = instance_find_by_funit_name_if_one( tinst2, tinst1->funit->name )) != NULL ) {

    bool rv = instance_merge_tree( tinst1, root2 );
    assert( rv );
    instance_mark_lhier_diffs( tinst1, root2 );

  /* Check to see if the module pointed to by tinst2 exists within the tree of tinst1 */
  } else if( (root1 = instance_find_by_funit_name_if_one( tinst1, tinst2->funit->name )) != NULL ) {

    bool rv = instance_merge_tree( root1, tinst2 );
    assert( rv );
    instance_mark_lhier_diffs( root1, tinst2 );

  /* Otherwise, we cannot merge the two CDD files so don't */
  } else {

    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}
#endif /* RUNLIB */

/*!
 \throws anonymous gen_item_assign_expr_ids instance_db_write funit_db_write

 Calls each functional unit display function in instance tree, starting with
 the root functional unit and ending when all of the leaf functional units are output.
 Note:  the function that calls this function originally should set
 the value of scope to NULL.
*/
void instance_db_write(
  funit_inst*   root,        /*!< Root of functional unit instance tree to write */
  FILE*         file,        /*!< Output file to display contents to */
  char*         scope,       /*!< Scope of this functional unit */
  bool          parse_mode,  /*!< Specifies if we are parsing or scoring */
  bool          issue_ids    /*!< Specifies that we need to issue expression and signal IDs */
) { PROFILE(INSTANCE_DB_WRITE);

  bool stop_recursive = FALSE;

  assert( root != NULL );

  if( root->funit != NULL ) {

    if( (root->funit->suppl.part.type != FUNIT_NO_SCORE) && !root->suppl.ignore ) {

      funit_inst* curr = parse_mode ? root : NULL;

      assert( scope != NULL );

#ifndef RUNLIB
      /* If we are in parse mode, re-issue expression IDs (we use the ulid field since it is not used in parse mode) */
      if( issue_ids && (root->funit != NULL) ) {

        unsigned int i;
#ifndef VPI_ONLY
        gitem_link* gil;
#endif /* VPI_ONLY */

        /* First issue IDs to the expressions within the functional unit */
        for( i=0; i<root->funit->exp_size; i++ ) {
          root->funit->exps[i]->ulid = curr_expr_id++;
        }

        /* Only assign IDs to signals that are within a generated functional unit signal list */
        for( i=0; i<root->funit->sig_size; i++ ) {
          if( i < root->funit->sig_no_rm_index ) {
            root->funit->sigs[i]->id = curr_sig_id++;
          }
        }
    
#ifndef VPI_ONLY
        /* Then issue IDs to any generated expressions/signals */
        gil = root->gitem_head;
        while( gil != NULL ) {
          gen_item_assign_ids( gil->gi, root->funit );
          gil = gil->next;
        }
#endif /* VPI_ONLY */

      }
#endif /* RUNLIB */

      /* Display root functional unit */
      funit_db_write( root->funit, scope, root->suppl.name_diff, file, curr, issue_ids );

    } else {

      stop_recursive = TRUE;

    }

  } else {

    /*@-formatcode@*/
    fprintf( file, "%d %s %hhu\n", DB_TYPE_INST_ONLY, scope, root->suppl.name_diff );
    /*@=formatcode@*/

  }

  if( !stop_recursive ) {
 
    char tscope[4096];

    /* Display children */
    funit_inst* curr = root->child_head;
    while( curr != NULL ) {
      unsigned int rv = snprintf( tscope, 4096, "%s.%s", scope, curr->name );
      assert( rv < 4096 );
      instance_db_write( curr, file, tscope, parse_mode, issue_ids );
      curr = curr->next;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the allocated instance.

 Parses an instance-only database line and adds a "placeholder" instance in the instance tree.
*/
funit_inst* instance_only_db_read(
  char** line  /*!< Pointer to line being read from database file */
) { PROFILE(INSTANCE_ONLY_DB_READ);

  char        scope[4096];
  int         chars_read;
  bool        name_diff;
  funit_inst* child = NULL;

  if( sscanf( *line, "%s %d%n", scope, (int*)&name_diff, &chars_read ) == 2 ) {

    char* back = strdup_safe( scope );
    char* rest = strdup_safe( scope );

    *line += chars_read;

    scope_extract_back( scope, back, rest ); 

    /* Create "placeholder" instance */
    child = instance_create( NULL, back, 0, 0, name_diff, FALSE, FALSE, NULL );

    /* If we are the top-most instance, just add ourselves to the instance link list */
    if( rest[0] == '\0' ) {
      (void)inst_link_add( child, &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );

    /* Otherwise, find our parent instance and attach the new instance to it */
    } else {
      funit_inst* parent;
      if( (parent = inst_link_find_by_scope( rest, db_list[curr_db]->inst_tail, FALSE )) != NULL ) {
        if( parent->child_head == NULL ) {
          parent->child_head = parent->child_tail = child;
        } else {
          parent->child_tail->next = child;
          parent->child_tail       = child;
        }
        child->parent = parent;
      } else {
        print_output( "Unable to find parent instance of instance-only line in database file.", FATAL, __FILE__, __LINE__ );
        Throw 0;
      }
    }

    /* Deallocate memory */
    free_safe( back, (strlen( scope ) + 1) );
    free_safe( rest, (strlen( scope ) + 1) );

  } else {

    print_output( "Unable to read instance-only line in database file.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

  return( child );

}

#ifndef RUNLIB
/*!
 Merges instance-only constructs from two CDD files.
*/
void instance_only_db_merge(
  char** line  /*!< Pointer to line being read from database file */
) { PROFILE(INSTANCE_ONLY_DB_MERGE);

  char scope[4096];
  int  chars_read;
  bool name_diff;

  if( sscanf( *line, "%s %d%n", scope, (int*)&name_diff, &chars_read ) == 2 ) {

    char*       back = strdup_safe( scope );
    char*       rest = strdup_safe( scope );
    funit_inst* child;

    *line += chars_read;

    scope_extract_back( scope, back, rest );

    /* Create "placeholder" instance */
    child = instance_create( NULL, back, 0, 0, name_diff, FALSE, FALSE, NULL );

    /* If we are the top-most instance, just add ourselves to the instance link list */
    if( rest[0] == '\0' ) {

      /* Add a new instance link if was not able to be found in the instance linked list */
      if( inst_link_find_by_scope( scope, db_list[curr_db]->inst_head, FALSE ) == NULL ) {
        (void)inst_link_add( child, &(db_list[curr_db]->inst_head), &(db_list[curr_db]->inst_tail) );
      }

    /* Otherwise, find our parent instance and attach the new instance to it */
    } else {
      funit_inst* parent;
      if( (parent = inst_link_find_by_scope( rest, db_list[curr_db]->inst_head, FALSE )) != NULL ) {
        if( parent->child_head == NULL ) {
          parent->child_head = parent->child_tail = child;
        } else {
          parent->child_tail->next = child;
          parent->child_tail       = child;
        }
        child->parent = parent;
      } else {
        print_output( "Unable to find parent instance of instance-only line in database file.", FATAL, __FILE__, __LINE__ );
        Throw 0;
      }
    }

    /* Deallocate memory */
    free_safe( back, (strlen( scope ) + 1) );
    free_safe( rest, (strlen( scope ) + 1) );

  } else {

    print_output( "Unable to merge instance-only line in database file.", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Removes all statement blocks in the design that call that specified statement.
*/
void instance_remove_stmt_blks_calling_stmt(
  funit_inst* root,  /*!< Pointer to root instance to remove statements from */
  statement*  stmt   /*!< Pointer to statement to match */
) { PROFILE(INSTANCE_REMOVE_STMT_BLKS_CALLING_STMT);

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

  PROFILE_END;

}

/*!
 Recursively traverses the given instance tree, removing the given statement.
*/
void instance_remove_parms_with_expr(
  funit_inst* root,  /*!< Pointer to functional unit instance to remove expression from */
  statement*  stmt   /*!< Pointer to statement to remove from list */
) { PROFILE(INSTANCE_REMOVE_PARMS_WITH_EXPR);

  funit_inst* curr_child;  /* Pointer to current child instance to traverse */
  inst_parm*  iparm;       /* Pointer to current instance parameter */

  /* Search for the given expression within the given instance parameter */
  iparm = root->param_head;
  while( iparm != NULL ) {
    if( iparm->sig != NULL ) {
      unsigned int i = 0;
      while( i < iparm->sig->exp_size ) {
        if( expression_find_expr( stmt->exp, iparm->sig->exps[i] ) ) {
          if( iparm->mparm != NULL ) {
            exp_link_remove( iparm->sig->exps[i], &(iparm->mparm->exps), &(iparm->mparm->exp_size), FALSE );
          }
          exp_link_remove( iparm->sig->exps[i], &(iparm->sig->exps), &(iparm->sig->exp_size), FALSE );
        } else {
          i++;
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

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Deallocates all memory allocated for the given instance.
*/
void instance_dealloc_single(
  funit_inst* inst  /*!< Pointer to instance to deallocate memory for */
) { PROFILE(INSTANCE_DEALLOC_SINGLE);

  if( inst != NULL ) {

    /* Free up memory allocated for name */
    free_safe( inst->name, (strlen( inst->name ) + 1) );

    /* Free up memory allocated for statistic, if necessary */
    free_safe( inst->stat, sizeof( statistic ) );

    /* Free up memory for range, if necessary */
    if( inst->range != NULL ) {
      static_expr_dealloc( inst->range->left,  FALSE );
      static_expr_dealloc( inst->range->right, FALSE );
      free_safe( inst->range, sizeof( vector_width ) );
    }

#ifndef RUNLIB
    /* Deallocate memory for instance parameter list */
    inst_parm_dealloc( inst->param_head, TRUE );

#ifndef VPI_ONLY
    /* Deallocate memory for generate item list */
    gitem_link_delete_list( inst->gitem_head, FALSE );
#endif /* VPI_ONLY */
#endif /* RUNLIB */

    /* Free up memory for this functional unit instance */
    free_safe( inst, sizeof( funit_inst ) );

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Outputs dumpvars to the specified file.
*/
void instance_output_dumpvars(
  FILE*       vfile,  /*!< Pointer to file to output dumpvars to */
  funit_inst* root    /*!< Pointer to current instance */
) { PROFILE(INSTANCE_OUTPUT_DUMPVARS);

  funit_inst* child = root->child_head;
  char        scope[4096];

  /* Generate instance scope */
  scope[0] = '\0';
  instance_gen_scope( scope, root, FALSE );

  /* Outputs dumpvars for the given functional unit */
  funit_output_dumpvars( vfile, root->funit, scope );

  /* Outputs all children instances */
  while( child != NULL ) {
    instance_output_dumpvars( vfile, child );
    child = child->next;
  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 Recursively traverses instance tree, deallocating heap memory used to store the
 the tree.
*/
void instance_dealloc_tree(
  funit_inst* root  /*!< Pointer to root instance of functional unit instance tree to remove */
) { PROFILE(INSTANCE_DEALLOC_TREE);

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

  PROFILE_END;

}

/*!
 Searches tree for specified functional unit.  If the functional unit instance is found,
 the functional unit instance is removed from the tree along with all of its
 child functional unit instances.
*/
void instance_dealloc(
  funit_inst* root,  /*!< Root of functional unit instance tree */
  char*       scope  /*!< Scope of functional unit to remove from tree */
) { PROFILE(INSTANCE_DEALLOC);
  
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

  PROFILE_END;

}
