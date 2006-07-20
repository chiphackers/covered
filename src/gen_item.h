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

/*! \brief Outputs the current generate item to the given output file if it matches the type specified */
void gen_item_db_write( gen_item* gi, control type, FILE* file );

/*! \brief Connects a generate item block to a new generate item */
bool gen_item_connect( gen_item* gi1, gen_item* gi2, int conn_id );

/*! \brief Resolves a generate block */
void gen_item_resolve( gen_item* gi, funit_inst* inst );

/*! \brief Resolves all generate items in the design */
void generate_resolve( funit_inst* inst );

/*! \brief Deallocates all associated memory for the given generate item */
void gen_item_dealloc( gen_item* gi, bool rm_elem );

/*
 $Log$
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
