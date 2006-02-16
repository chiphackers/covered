#ifndef __INSTANCE_H__
#define __INSTANCE_H__

/*!
 \file    instance.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/11/2002
 \brief    Contains functions for handling functional unit instances.
*/

#include "defines.h"


/*! \brief Displays the current state of the instance tree */
void instance_display_tree( funit_inst* root );

/*! \brief Builds full hierarchy from leaf node to root. */
void instance_gen_scope( char* scope, funit_inst* leaf );

/*! \brief Finds specified scope in functional unit instance tree. */
funit_inst* instance_find_scope( funit_inst* root, char* scope );

/*! \brief Returns instance that points to specified functional unit for each instance. */
funit_inst* instance_find_by_funit( funit_inst* root, func_unit* funit, int* ignore );

/*! \brief Adds new instance to specified instance tree during parse. */
void instance_parse_add( funit_inst** root, func_unit* parent, func_unit* child, char* inst_name, vector_width* range );

/*! \brief Adds new instance to specified instance tree during CDD read. */
void instance_read_add( funit_inst** root, char* parent, func_unit* child, char* inst_name );

/*! \brief Displays contents of functional unit instance tree to specified file. */
void instance_db_write( funit_inst* root, FILE* file, char* scope, bool parse_mode );

/*! \brief Removes specified instance from tree. */
void instance_dealloc( funit_inst* root, char* scope );


/*
 $Log$
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

