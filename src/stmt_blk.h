#ifndef __STMT_BLK_H__
#define __STMT_BLK_H__

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
 \file     stmt_blk.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/23/2005
 \brief    Contains functions for removing statement blocks from coverage consideration
*/

#include "defines.h"


/*!
 \brief Adds the statement block containing the specified statement to the list of statement
        blocks to remove after parsing, binding and race condition checking has occurred.
*/
void stmt_blk_add_to_remove_list(
  statement* stmt
);

/*!
 \brief Removes all statement blocks listed for removal.
*/
void stmt_blk_remove();

/*!
 \brief Outputs the reason why a logic block is being removed from coverage consideration.
*/    
void stmt_blk_specify_removal_reason(
  logic_rm_type type,   /*!< Reason for removing the logic block */
  const char*   file,   /*!< Filename containing logic block being removed */
  int           line,   /*!< Line containing logic that is causing logic block removal */
  const char*   cfile,  /*!< File containing removal line */
  int           cline   /*!< Line containing removal line */
);

#endif

