#ifndef __INSTANCE_H__
#define __INSTANCE_H__

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
             bool          name_diff,
  /*@null@*/ vector_width* range
);

/*! \brief Displays the current state of the instance tree */
void instance_display_tree( funit_inst* root );

/*! \brief Builds full hierarchy from leaf node to root. */
void instance_gen_scope( char* scope, funit_inst* leaf, bool flatten );

/*! \brief Finds specified scope in functional unit instance tree. */
funit_inst* instance_find_scope( funit_inst* root, char* scope, bool rm_unnamed );

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
void instance_copy( funit_inst* from_inst, funit_inst* to_inst, char* name, vector_width* range, bool resolve );

/*! \brief Adds new instance to specified instance tree during parse. */
bool instance_parse_add( funit_inst** root, func_unit* parent, func_unit* child, char* inst_name, vector_width* range,
                         bool resolve, bool child_gend );

/*! \brief Resolves all instance arrays. */
void instance_resolve( funit_inst* root );

/*! \brief Adds new instance to specified instance tree during CDD read. */
bool instance_read_add( funit_inst** root, char* parent, func_unit* child, char* inst_name );

/*! \brief Gets the leading hierarchy scope and instance for a particular instance tree */
void instance_get_leading_hierarchy(
            funit_inst*  root,
  /*@out@*/ char*        leading_hierarchy,
  /*@out@*/ funit_inst** top_inst
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
  bool        issue_ids,
  bool        report_save
);

/*! \brief Reads in and handles an instance-only line from the database */
void instance_only_db_read( char** line );

/*! \brief Reads in and merges an instance-only line from the database */
void instance_only_db_merge( char** line );

/*! \brief Removes all statement blocks that contain expressions that call the given statement */
void instance_remove_stmt_blks_calling_stmt( funit_inst* root, statement* stmt );

/*! \brief Removes expressions from instance parameters within the given instance that match the given expression */
void instance_remove_parms_with_expr( funit_inst* root, statement* stmt );

/*! \brief Outputs dumpvars to the given file for the given instance */
void instance_output_dumpvars( FILE* vfile, funit_inst* root );

/*! \brief Recursively deallocates all memory for the associated instance tree */
void instance_dealloc_tree( funit_inst* root );

/*! \brief Removes specified instance from tree. */
void instance_dealloc( funit_inst* root, char* scope );


/*
 $Log$
 Revision 1.44  2008/11/12 19:57:07  phase1geo
 Fixing the rest of the issues from regressions in regards to the merge changes.
 Updating regression files.  IV and Cver regressions now pass.

 Revision 1.43  2008/11/12 00:07:41  phase1geo
 More updates for complex merging algorithm.  Updating regressions per
 these changes.  Checkpointing.

 Revision 1.42  2008/11/11 05:36:40  phase1geo
 Checkpointing merge code.

 Revision 1.41  2008/11/11 00:10:19  phase1geo
 Starting to work on instance tree merging algorithm (not complete at this point).
 Checkpointing.

 Revision 1.40  2008/11/08 00:09:04  phase1geo
 Checkpointing work on asymmetric merging algorithm.  Updated regressions
 per these changes.  We currently have 5 failures in the IV regression suite.

 Revision 1.39  2008/10/31 22:01:34  phase1geo
 Initial code changes to support merging two non-overlapping CDD files into
 one.  This functionality seems to be working but needs regression testing to
 verify that nothing is broken as a result.

 Revision 1.38  2008/10/07 05:24:18  phase1geo
 Adding -dumpvars option.  Need to resolve a few issues before this work is considered
 complete.

 Revision 1.37  2008/09/03 05:33:06  phase1geo
 Adding in FSM exclusion support to exclude and report -e commands.  Updating
 regressions per recent changes.  Checkpointing.

 Revision 1.36  2008/09/02 22:41:45  phase1geo
 Starting to work on adding exclusion reason output to report files.  Added
 support for exclusion reasons to CDD files.  Checkpointing.

 Revision 1.35  2008/09/02 05:20:41  phase1geo
 More updates for exclude command.  Updates to CVER regression.

 Revision 1.34  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.33  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.32  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.31  2007/07/18 22:39:17  phase1geo
 Checkpointing generate work though we are at a fairly broken state at the moment.

 Revision 1.30  2007/07/18 02:15:04  phase1geo
 Attempts to fix a problem with generating instances with hierarchy.  Also fixing
 an issue with named blocks in generate statements.  Still some work to go before
 regressions are passing again, however.

 Revision 1.29  2007/04/18 22:35:02  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.28  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.27  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.26  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.25  2007/03/16 21:41:09  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.24  2007/03/15 22:39:05  phase1geo
 Fixing bug in unnamed scope binding.

 Revision 1.23  2006/12/19 05:23:39  phase1geo
 Added initial code for handling instance flattening for unnamed scopes.  This
 is partially working at this point but still needs some debugging.  Checkpointing.

 Revision 1.22  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.21  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.20  2006/09/08 22:39:50  phase1geo
 Fixes for memory problems.

 Revision 1.19  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.18  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.17  2006/07/21 20:12:46  phase1geo
 Fixing code to get generated instances and generated array of instances to
 work.  Added diagnostics to verify correct functionality.  Full regression
 passes.

 Revision 1.16  2006/07/18 13:37:47  phase1geo
 Fixing compile issues.

 Revision 1.15  2006/07/12 22:16:18  phase1geo
 Fixing hierarchical referencing for instance arrays.  Also attempted to fix
 a problem found with unary1; however, the generated report coverage information
 does not look correct at this time.  Checkpointing what I have done for now.

 Revision 1.14  2006/07/11 04:59:08  phase1geo
 Reworking the way that instances are being generated.  This is to fix a bug and
 pave the way for generate loops for instances.  Code not working at this point
 and may cause serious problems for regression runs.

 Revision 1.13  2006/06/27 19:34:43  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.12  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.11  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.10  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.9  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.8  2002/10/31 23:13:52  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.7  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.6  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.5  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.4  2002/07/18 05:50:45  phase1geo
 Fixes should be just about complete for instance depth problems now.  Diagnostics
 to help verify instance handling are added to regression.  Full regression passes.

 Revision 1.3  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.
*/

#endif

