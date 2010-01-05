#ifndef __INSTANCE_H__
#define __INSTANCE_H__

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
 \file     instance.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/11/2002
 \brief    Contains functions for handling functional unit instances.
*/

#include "defines.h"


/*! \brief Creates a new instance with the given information */
funit_inst* instance_create(
             func_unit*    funit,
             char*         inst_name,
             unsigned int  ppfline,
             int           fcol,
             bool          name_diff,
             bool          ignore,
             bool          gend_scope,
  /*@null@*/ vector_width* range
);

/*! \brief Displays the current state of the instance tree */
void instance_display_tree(
  funit_inst* root
);

/*! \brief Assign IDs to all instances */
void instance_assign_ids(
  funit_inst* root,
  int*        curr_id
);

/*! \brief Builds full hierarchy from leaf node to root. */
void instance_gen_scope(
  char*       scope,
  funit_inst* leaf,
  bool        flatten
);

/*! \brief Builds Verilator-flattened hierarchy from leaf node to (root - 1). */
void instance_gen_verilator_scope(
  char*       scope,
  funit_inst* leaf
);

/*! \brief Finds specified scope in functional unit instance tree. */
funit_inst* instance_find_scope(
  funit_inst* root,
  char*       scope,
  bool        rm_unnamed
);

/*! \brief Returns instance that points to specified functional unit for each instance. */
funit_inst* instance_find_by_funit(
            funit_inst*      root,
            const func_unit* funit,
  /*@out@*/ int*             ignore
);

/*! \brief Returns signal that matches the given exclusion ID */
vsignal* instance_find_signal_by_exclusion_id(
            funit_inst* root,
            int         id,
  /*@out@*/ func_unit** found_funit
);

/*! \brief Returns expression that matches the given exclusion ID */
expression* instance_find_expression_by_exclusion_id(
            funit_inst* root,
            int         id,
  /*@out@*/ func_unit** found_funit
);

/*! \brief Returns FSM that matches the given exclusion ID */
int instance_find_fsm_arc_index_by_exclusion_id(
            funit_inst* root,
            int         id,
  /*@out@*/ fsm_table** found_fsm,
  /*@out@*/ func_unit** found_funit
);

/*! \brief Copies the given from_inst as a child of the given to_inst */
funit_inst* instance_copy(
  funit_inst*   from_inst,
  funit_inst*   to_inst,
  char*         name,
  unsigned int  ppfline,
  int           fcol,
  vector_width* range,
  bool          resolve
);

/*! \brief Adds new instance to specified instance tree during parse. */
bool instance_parse_add(
  funit_inst**  root,
  func_unit*    parent,
  func_unit*    child,
  char*         inst_name,
  unsigned int  ppfline,
  int           fcol,
  vector_width* range,
  bool          resolve,
  bool          child_gend,
  bool          ignore_child,
  bool          gend_scope
);

/*! \brief Resolves all instance arrays. */
void instance_resolve(
  funit_inst* root
);

/*! \brief Adds new instance to specified instance tree during CDD read. */
funit_inst* instance_read_add(
  funit_inst** root,
  char*        parent,
  func_unit*   child,
  char*        inst_name
);

/*! \brief Gets the leading hierarchy scope and instance for a particular instance tree */
void instance_get_leading_hierarchy(
            funit_inst*  root,
  /*@out@*/ char*        leading_hierarchy,
  /*@out@*/ funit_inst** top_inst
);

/*! \brief Gets the Verilator leading hierarchy scope for a particular instance tree */
void instance_get_verilator_leading_hierarchy(
                 funit_inst*  root,
  /*@out null@*/ char*        leading_hierarchy,
  /*@out@*/      funit_inst** top_inst
);

/*! \brief Performs complex instance tree merging for two instance trees. */
bool instance_merge_two_trees(
  funit_inst* root1,
  funit_inst* root2
);

/*! \brief Displays contents of functional unit instance tree to specified file. */
void instance_db_write(
  funit_inst* root,
  FILE*       file,
  char*       scope,
  bool        parse_mode,
  bool        issue_ids
);

/*! \brief Reads in and handles an instance-only line from the database */
funit_inst* instance_only_db_read(
  char** line
);

/*! \brief Reads in and merges an instance-only line from the database */
void instance_only_db_merge(
  char** line
);

/*! \brief Removes all statement blocks that contain expressions that call the given statement */
void instance_remove_stmt_blks_calling_stmt(
  funit_inst* root,
  statement*  stmt
);

/*! \brief Removes expressions from instance parameters within the given instance that match the given expression */
void instance_remove_parms_with_expr(
  funit_inst* root,
  statement*  stmt
);

/*! \brief Outputs dumpvars to the given file for the given instance */
void instance_output_dumpvars(
  FILE*       vfile,
  funit_inst* root
);

/*! \brief Recursively deallocates all memory for the associated instance tree */
void instance_dealloc_tree(
  funit_inst* root
);

/*! \brief Removes specified instance from tree. */
void instance_dealloc(
  funit_inst* root,
  char*       scope
);

#endif

