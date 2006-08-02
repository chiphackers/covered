#ifndef __GEN_ITEM_H__
#define __GEN_ITEM_H__

/*!
 \file     gen_item.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/10/2006
 \brief    Contains functions for handling generate items.
*/


#include <stdio.h>
#include "defines.h"


/*! \brief Displays the specified generate item to standard output */
void gen_item_display( gen_item* gi );

/*! \brief Displays an entire generate block */
void gen_item_display_block( gen_item* root );

/*! \brief Searches for a generate item in the generate block of root that matches gi */
gen_item* gen_item_find( gen_item* root, gen_item* gi );

/*! \brief Returns TRUE if the specified variable name contains a generate variable within it */
bool gen_item_varname_contains_genvar( char* name );

/*! \brief Creates a generate item for an expression */
gen_item* gen_item_create_expr( expression* expr );

/*! \brief Creates a generate item for a signal */
gen_item* gen_item_create_sig( vsignal* sig );

/*! \brief Creates a generate item for a statement */
gen_item* gen_item_create_stmt( statement* stmt );

/*! \brief Creates a generate item for an instance */
gen_item* gen_item_create_inst( funit_inst* inst );

/*! \brief Creates a generate item for a namespace */
gen_item* gen_item_create_tfn( funit_inst* inst );

/*! \brief Creates a generate item for a binding */
gen_item* gen_item_create_bind( char* name, expression* expr );

/*! \brief Resizes all expressions in the given generate item block */
void gen_item_resize_exprs( gen_item* gi );

/*! \brief Assigns unique expression IDs to all expressions for specified statement block */
void gen_item_assign_expr_ids( gen_item* gi );

/*! \brief Outputs the current generate item to the given output file if it matches the type specified */
void gen_item_db_write( gen_item* gi, control type, FILE* file );

/*! \brief Outputs the entire expression tree from the given generate statement */
void gen_item_db_write_expr_tree( gen_item* gi, FILE* file );

/*! \brief Connects a generate item block to a new generate item */
bool gen_item_connect( gen_item* gi1, gen_item* gi2, int conn_id );

/*! \brief Resolves a generate block */
void gen_item_resolve( gen_item* gi, funit_inst* inst, bool add );

/*! \brief Checks generate item and if it is a bind, adds it to binding pool and returns TRUE */
bool gen_item_bind( gen_item* gi, func_unit* funit );

/*! \brief Resolves all generate items in the design */
void generate_resolve( funit_inst* inst );

/*! \brief Deallocates all associated memory for the given generate item */
void gen_item_dealloc( gen_item* gi, bool rm_elem );

/*
 $Log$
 Revision 1.11  2006/07/29 20:53:43  phase1geo
 Fixing some code related to generate statements; however, generate8.1 is still
 not completely working at this point.  Full regression passes for IV.

 Revision 1.10  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.9  2006/07/25 21:35:54  phase1geo
 Fixing nested namespace problem with generate blocks.  Also adding support
 for using generate values in expressions.  Still not quite working correctly
 yet, but the format of the CDD file looks good as far as I can tell at this
 point.

 Revision 1.8  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.7  2006/07/21 22:39:01  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.6  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.5  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.4  2006/07/20 04:55:18  phase1geo
 More updates to support generate blocks.  We seem to be passing the parser
 stage now.  Getting segfaults in the generate_resolve code, presently.

 Revision 1.3  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.2  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.1  2006/07/10 22:37:14  phase1geo
 Missed the actual gen_item files in the last submission.

*/

#endif
