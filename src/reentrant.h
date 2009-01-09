#ifndef __REENTRANT_H__
#define __REENTRANT_H__

/*
 Copyright (c) 2006-2009 Trevor Williams

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
 \file     reentrant.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/11/2006
 \brief    Contains functions for handling re-entrant tasks and functions.
*/

#include "defines.h"


/*! \brief Allocates and initializes the reentrant structure for the given functional unit */
reentrant* reentrant_create( func_unit* funit );

/*! \brief Deallocates all memory associated with the given reentrant structure */
void reentrant_dealloc( reentrant* ren, func_unit* funit, expression* expr );


/*
 $Log$
 Revision 1.7  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.6  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.5  2007/07/30 22:42:02  phase1geo
 Making some progress on automatic function support.  Things currently don't compile
 but I need to checkpoint for now.

 Revision 1.4  2007/07/27 19:11:27  phase1geo
 Putting in rest of support for automatic functions/tasks.  Checked in
 atask1 diagnostic files.

 Revision 1.3  2007/04/20 22:56:46  phase1geo
 More regression updates and simulator core fixes.  Still a ways to go.

 Revision 1.2  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.1  2006/12/11 23:29:17  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.

*/

#endif

