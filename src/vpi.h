#ifndef __VPI_H__
#define __VPI_H__

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
 \file     vpi.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/21/2005
 \brief    Contains functions for handling VPI-related activity.
*/

/*! \brief Displays the given message to standard output */
void vpi_print_output( const char* msg );

/*
 $Log$
 Revision 1.4  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.3  2007/09/14 21:22:46  phase1geo
 Fixing compile issues.

 Revision 1.2  2006/10/13 22:46:32  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.1  2006/04/06 22:31:22  phase1geo
 Adding vpi.c and vpi.h to src directory.

*/

#endif

