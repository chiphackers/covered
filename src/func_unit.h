#ifndef __FUNC_UNIT_H__
#define __FUNC_UNIT_H__

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
 \file     func_unit.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/7/2001
 \brief    Contains functions for handling functional units.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Creates new functional unit from heap and initializes structure. */
func_unit* funit_create();

/*! \brief Returns the parent module of the given functional unit. */
func_unit* funit_get_curr_module(
  func_unit* funit
);

/*! \brief Returns the parent module of the given functional unit (returning a const version). */
const func_unit* funit_get_curr_module_safe(
  const func_unit* funit
);

/*! \brief Returns the parent function of the given functional unit (if there is one) */
func_unit* funit_get_curr_function(
  func_unit* funit
);

/*! \brief Returns the parent task of the given functional unit (if there is one) */
func_unit* funit_get_curr_task(
  func_unit* funit
);

/*! \brief Returns the number of input, output and inout ports in the specified functional unit */
int funit_get_port_count(
  func_unit* funit
);

/*! \brief Returns the child functional unit by file position. */
func_unit* funit_find_by_position(
  func_unit*   parent,
  unsigned int first_line,
  unsigned int first_column
);

/*! \brief Finds specified module parameter given the current functional unit and its scope */
mod_parm* funit_find_param(
  char*      name,
  func_unit* funit
);

/*! \brief Finds specified signal given in the current functional unit */
vsignal* funit_find_signal(
  char*      name,
  func_unit* funit
);

/*! \brief Finds all expressions that call the given statement */
void funit_remove_stmt_blks_calling_stmt(
  func_unit* funit,
  statement* stmt
);

/*! \brief Generates the internally used task/function/named-block name for the specified functional unit */
char* funit_gen_task_function_namedblock_name(
  char*      orig_name,
  func_unit* parent
);

/*! \brief Sizes all elements for the current functional unit from the given instance */
void funit_size_elements(
  func_unit*  funit,
  funit_inst* inst,
  bool        gen_all,
  bool        alloc_exprs
);

/*! \brief Writes contents of provided functional unit to specified output. */
void funit_db_write(
  func_unit*  funit,
  char*       scope,
  bool        name_diff,
  FILE*       file,
  funit_inst* inst,
  bool        ids_issued
);

/*! \brief Read contents of current line from specified file, creates functional unit
           and adds to functional unit list. */
void funit_db_read(
            func_unit* funit,
  /*@out@*/ char*      scope,
  /*@out@*/ bool*      name_diff,
            char**     line
);

/*! \brief Reads the functional unit version information from the functional unit line and adds it to the current functional unit */
void funit_version_db_read(
  func_unit* funit,
  char**     line
);

/*! \brief Merges two functional units into the base functional unit. */
void funit_merge(
  func_unit* base,
  func_unit* other
);

/*! \brief Reads and merges two functional units into base functional unit. */
void funit_db_merge(
  func_unit* base,
  FILE*      file,
  bool       same
);

/*! \brief Flattens the functional unit name by removing all unnamed scope portions */
/*@shared@*/ char* funit_flatten_name(
  func_unit* funit
);

/*! \brief Finds the functional unit that contains the given statement/expression ID */
func_unit* funit_find_by_id(
  int id
);

/*! \brief Returns TRUE if the given functional unit does not contain any input, output or inout ports. */
bool funit_is_top_module(
  func_unit* funit
);

/*! \brief Returns TRUE if the given functional unit is an unnamed scope. */
bool funit_is_unnamed(
  func_unit* funit
);

/*! \brief Returns TRUE if the specified "parent" functional unit is a parent of the "child" functional unit */
bool funit_is_unnamed_child_of(
  func_unit* parent,
  func_unit* child
);

/*! \brief Returns TRUE if the specified "parent" functional unit is a parent of the "child" functional unit */
bool funit_is_child_of(
  func_unit* parent,
  func_unit* child
);

/*! \brief Displays signals stored in this functional unit. */
void funit_display_signals(
  func_unit* funit
);

/*! \brief Displays expressions stored in this functional unit. */
void funit_display_expressions(
  func_unit* funit
);

/*! \brief Adds given thread to functional unit's thread pointer/thread pointer list */
void funit_add_thread(
  func_unit* funit,
  thread*    thr
);

/*! \brief Pushes the threads associated with the given functional unit onto the active simulation queue */
void funit_push_threads(
  func_unit*       funit,
  const statement* stmt,
  const sim_time*  time
);

/*! \brief Removes given thread from the given functional unit's thread pointer/thread pointer list */
void funit_delete_thread(
  func_unit* funit,
  thread*    thr
);

/*! \brief Outputs dumpvars calls to the given file. */
void funit_output_dumpvars(
  FILE*       vfile,
  func_unit*  funit,
  const char* scope
);

/*! \brief Returns TRUE if at least one signal was found that needs to be assigned by the dumpfile. */
bool funit_is_one_signal_assigned(
  func_unit* funit
);

/*! \brief Deallocates functional unit element from heap. */
void funit_dealloc(
  func_unit* funit
);

#endif

