#ifndef __GEN_ITEM_H__
#define __GEN_ITEM_H__

/*!
 \file     gen_item.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/10/2006
 \brief    Contains functions for handling generate items.
*/


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

/*! \brief Resolves a generate block */
void gen_item_resolve( gen_item* gi, funit_inst* inst );

/*! \brief Deallocates all associated memory for the given generate item */
void gen_item_dealloc( gen_item* gi, bool rm_elem );

/*
 $Log$
 Revision 1.1  2006/07/10 22:37:14  phase1geo
 Missed the actual gen_item files in the last submission.

*/

#endif
