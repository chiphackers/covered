#ifndef __CLI_H__
#define __CLI_H__

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
 \file     cli.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/11/2007
 \brief    Contains functions for handling the score command CLI debugger.
*/


#include "defines.h"


/*! \brief Performs CLI management. */
void cli_execute( const sim_time* time, bool force );

/*! \brief Reads in given history file from -cli option */
void cli_read_hist_file( const char* fname );

/*
 $Log$
 Revision 1.7  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.6  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.5  2008/01/21 21:39:55  phase1geo
 Bug fix for bug 1876376.

 Revision 1.4  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.3  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.2  2007/04/12 03:46:30  phase1geo
 Fixing bugs with CLI.  History and history file saving/loading is implemented
 and working as desired.

 Revision 1.1  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

*/

#endif

