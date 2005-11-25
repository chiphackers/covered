#ifndef __STMT_BLK_H__
#define __STMT_BLK_H__

/*!
 \file     stmt_blk.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/23/2005
 \brief    Contains functions for removing statement blocks from coverage consideration
*/

#include "defines.h"


/*!
 Adds the statement block containing the specified statement to the list of statement
 blocks to remove after parsing, binding and race condition checking has occurred.
*/
void stmt_blk_add_to_remove_list( statement* stmt );

/*!
 Removes all statement blocks listed for removal.
*/
void stmt_blk_remove();


/*
 $Log$
*/

#endif

