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
#include "instance.h"
#include "func_unit.h"
#include "util.h"
#include "param.h"


extern int curr_expr_id;


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

  /* If this instance has a range, handle it */
  if( inst->range != NULL ) {

    /* Extract the index portion of inst_name if there is one */
    if( sscanf( inst_name, "%[a-zA-Z0-9_]\[%d]", bname, &index ) == 2 ) {
      
      /* If the base names compare, check that the given index falls within this instance range */
      if( scope_compare( bname, inst->name ) ) {

        /* Get range information from instance */
        static_expr_calc_lsb_and_width_post( inst->range->left, inst->range->right, &width, &lsb );
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
 \param curr_scope  Scope name (instance name) of current instance.
 \param rest_scope  Rest of scope name.
 
 \return Returns pointer to functional unit instance found by scope.
 
 Searches the specified functional unit instance tree for the specified
 scope.  When the functional unit instance is found, a pointer to that
 functional unit instance is passed back to the calling function.
*/
funit_inst* instance_find_scope_helper( funit_inst* root, char* curr_scope, char* rest_scope ) {
 
  char        front[256];   /* Highest level of hierarchy in hierarchical reference */
  char        rest[4096];   /* Rest of scope value */
  funit_inst* inst = NULL;  /* Pointer to found instance */
  funit_inst* child;        /* Pointer to child instance of this functional unit instance */
    
  if( root != NULL ) {

    assert( curr_scope != NULL );

    if( instance_compare( curr_scope, root ) ) {
      if( rest_scope[0] == '\0' ) {
        return( root );
      } else {
        scope_extract_front( rest_scope, front, rest );
        child = root->child_head;
        while( (child != NULL) && ((inst = instance_find_scope_helper( child, front, rest )) == NULL) ) {
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
 \param root   Root of funit_inst tree to parse for scope.
 \param scope  Scope to search for.
 
 \return Returns pointer to functional unit instance found by scope.
 
 Searches the specified functional unit instance tree for the specified
 scope.  When the functional unit instance is found, a pointer to that
 functional unit instance is passed back to the calling function.
*/
funit_inst* instance_find_scope( funit_inst* root, char* scope ) {
 
  char tmp_scope[4096];  /* Rest of scope value */
  
  assert( root != NULL );

  /* Strip root name from scope */
  if( strncmp( scope, root->name, strlen( root->name ) ) == 0 ) {
    
    strcpy( tmp_scope, scope );
    
    if( strlen( root->name ) == strlen( scope ) ) {
      return( instance_find_scope_helper( root, tmp_scope, tmp_scope + strlen( root->name ) ) );
    } else {
      tmp_scope[ strlen( root->name ) ] = '\0';
      return( instance_find_scope_helper( root, tmp_scope, tmp_scope + strlen( root->name ) + 1 ) );
    }
    
  } else {
    
    return( NULL );
    
  }
  
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
 \param inst   Pointer to instance to add child instance to.
 \param child  Pointer to child functional unit to create instance for.
 \param name   Name of instance to add.
 \param range  For arrays of instances, contains the range of the instance array
 
 \return Returns pointer to newly created functional unit instance.
 
 Generates new instance, adds it to the child list of the inst functional unit
 instance, and resolves any parameters.
*/
funit_inst* instance_add_child( funit_inst* inst, func_unit* child, char* name, vector_width* range ) {

  funit_inst* new_inst;  /* Pointer to newly created instance to add */

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

  return( new_inst );

}

/*!
 \param from_inst  Pointer to instance tree to copy.
 \param to_inst    Pointer to instance to copy tree to.
 \param name       Instance name of current instance being copied.
 \param range      For arrays of instances, indicates the array range.
 
 Recursively copies the instance tree of from_inst to the instance 
 to_inst, allocating memory for the new instances and resolving parameters.
*/
void instance_copy( funit_inst* from_inst, funit_inst* to_inst, char* name, vector_width* range ) {

  funit_inst* curr;      /* Pointer to current functional unit instance to copy */
  funit_inst* new_inst;  /* Pointer to newly created functional unit instance */

  assert( from_inst != NULL );
  assert( to_inst   != NULL );
  assert( name      != NULL );

  /* Add new child instance */
  new_inst = instance_add_child( to_inst, from_inst->funit, name, range );

  /* Iterate through rest of current child's list of children */
  curr = from_inst->child_head;
  while( curr != NULL ) {
    instance_copy( curr, new_inst, curr->name, range );
    curr = curr->next;
  }

}

/*!
 \param root       Root funit_inst pointer of functional unit instance tree.
 \param parent     Pointer to parent functional unit of specified child.
 \param child      Pointer to child functional unit to add.
 \param inst_name  Name of new functional unit instance.
 \param range      For array of instances, specifies the name range.
 
 Adds the child functional unit to the child functional unit pointer list located in
 the functional unit specified by the scope of parent in the functional unit instance
 tree pointed to by root.  This function is used by the db_add_instance
 function during the parsing stage.
*/
void instance_parse_add( funit_inst** root, func_unit* parent, func_unit* child, char* inst_name, vector_width* range ) {
  
  funit_inst* inst;    /* Temporary pointer to functional unit instance to add to */
  funit_inst* cinst;   /* Pointer to instance of child functional unit */
  int         i;       /* Loop iterator */
  int         ignore;  /* Number of matched instances to ignore */

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
      while( (inst = instance_find_by_funit( *root, parent, &ignore )) != NULL ) {
        instance_copy( cinst, inst, inst_name, range );
        i++;
        ignore = i;
      }

    } else {

      ignore = 0;
      while( (inst = instance_find_by_funit( *root, parent, &ignore )) != NULL ) {
        instance_add_child( inst, child, inst_name, range );
        i++;
        ignore = i;
      }

    }

    /* We should have found at least one parent instance */
    assert( i > 0 );

  }

}

/*!
 \param root       Pointer to root instance of functional unit instance tree.
 \param parent     String scope of parent instance.
 \param child      Pointer to child functional unit to add to specified parent's child list.
 \param inst_name  Instance name of this child functional unit instance.

 Adds the child functional unit to the child functional unit pointer list located in
 the functional unit specified by the scope of parent in the functional unit instance
 tree pointed to by root.  This function is used by the db_read
 function during the CDD reading stage.
*/ 
void instance_read_add( funit_inst** root, char* parent, func_unit* child, char* inst_name ) {

  funit_inst* inst;      /* Temporary pointer to functional unit instance to add to */
  funit_inst* new_inst;  /* Pointer to new functional unit instance to add */

  new_inst = instance_create( child, inst_name, NULL );

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

      /* Set parent pointer of new instance */
      new_inst->parent = inst;

    } else {

      /* Unable to find parent of this child, something went in wrong in writing/reading CDD file. */
      assert( inst != NULL );

    }
 
  }

}

/*!
 \param root        Root of functional unit instance tree to write.
 \param file        Output file to display contents to.
 \param scope       Scope of this functional unit.
 \param parse_mode  Specifies if we are parsing or scoring.

 Calls each functional unit display function in instance tree, starting with
 the root functional unit and ending when all of the leaf functional units are output.
 Note:  the function that calls this function originally should set
 the value of scope to NULL.
*/
void instance_db_write( funit_inst* root, FILE* file, char* scope, bool parse_mode ) {

  char        tscope1[4096];   /* New scope of functional unit to write */
  char        tscope2[4096];   /* New scope of functional unit to write */
  funit_inst* curr;            /* Pointer to current child functional unit instance */
  exp_link*   expl;            /* Pointer to current expression link */
  int         width = 1;       /* If the current instance is an array of instances, this is the width of the array */
  int         lsb;             /* If the current instance is an array of instances, this is the LSB of the array */
  int         i;               /* Loop iterator */

  assert( scope != NULL );

  curr = parse_mode ? root : NULL;

  if( root->range != NULL ) {

    /* Get LSB and width information */
    static_expr_calc_lsb_and_width_post( root->range->left, root->range->right, &width, &lsb );
    assert( width != -1 );
    assert( lsb   != -1 );

  }

  /* Add this instance (or array of instances) to the CDD file */
  for( i=0; i<width; i++ ) {

    /* If we are in parse mode, re-issue expression IDs (we use the ulid field since it is not used in parse mode) */
    if( parse_mode ) {
      expl = root->funit->exp_head;
      while( expl != NULL ) {
        expl->exp->ulid = curr_expr_id;
        curr_expr_id++;
        expl = expl->next;
      }
    }

    /* Calculate this instance's name */
    if( root->range != NULL ) {
      snprintf( tscope1, 4096, "%s[%d]", scope, i );
    } else {
      strcpy( tscope1, scope );
    }

    /* Display root functional unit */
    funit_db_write( root->funit, tscope1, file, curr );

    /* Display children */
    curr = root->child_head;
    while( curr != NULL ) {
      snprintf( tscope2, 4096, "%s.%s", tscope1, curr->name );
      instance_db_write( curr, file, tscope2, parse_mode );
      curr = curr->next;
    }

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

    /* Free up memory allocated for name */
    free_safe( root->name );

    /* Free up memory allocated for statistic, if necessary */
    if( root->stat != NULL ) {
      free_safe( root->stat );
    }

    /* Free up memory for range, if necessary */
    if( root->range != NULL ) {
      static_expr_dealloc( root->range->left,  FALSE );
      static_expr_dealloc( root->range->right, FALSE );
      free_safe( root->range );
    }

    /* Deallocate memory for instance parameter list */
    inst_parm_dealloc( root->param_head, TRUE );
  
    /* Free up memory for this functional unit instance */
    free_safe( root );

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

    inst = instance_find_scope( root, rest );
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

