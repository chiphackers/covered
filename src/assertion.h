#ifndef __ASSERTION_H__
#define __ASSERTION_H__

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
 \file     assertion.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/4/2006
 \brief    Contains functions for handling assertion coverage.
*/


#include <stdio.h>

#include "defines.h"


/*! \brief Parses -A command-line option to score command */
void assertion_parse( char* arg );

/*! \brief Parses an in-line attribute for assertion coverage information */
void assertion_parse_attr( attr_param* ap, func_unit* funit );

/*! Gather statistics for assertion coverage */
void assertion_get_stats( func_unit* funit, float* total, int* hit );

/*! Generates report output for assertion coverage */
void assertion_report( FILE* ofile, bool verbose );

/*! Retrieves the total and hit counts of assertions for the specified functional unit */
bool assertion_get_funit_summary( char* funit_name, int funit_type, int* total, int* hit );


/*
 $Log$
 Revision 1.3  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.2  2006/04/06 22:30:03  phase1geo
 Adding VPI capability back and integrating into autoconf/automake scheme.  We
 are getting close but still have a ways to go.

 Revision 1.1  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

*/

#endif

