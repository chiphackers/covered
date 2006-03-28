#ifndef __SEARCH_H__
#define __SEARCH_H__

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
 \file     search.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
 \brief    Contains functions used for finding Verilog files in the search information
           provided on the command line.
*/

#include "defines.h"


/*! \brief Initializes search maintained pointers. */
void search_init();

/*! \brief Adds an include directory to the list of directories to search for `include directives. */
bool search_add_include_path( char* path );

/*! \brief Adds a directory to the list of directories to find unspecified Verilog modules. */
bool search_add_directory_path( char* path );

/*! \brief Adds a specific Verilog module to the list of modules to score. */
bool search_add_file( char* file );

/*! \brief Adds specified functional unit to list of functional units not to score. */
bool search_add_no_score_funit( char* funit );

/*! \brief Adds specified extensions to allowed file extension list. */
bool search_add_extensions( char* ext_list );

/*! \brief Deallocates all used memory for search lists. */
void search_free_lists();


/*
 $Log$
 Revision 1.6  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.5  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.4  2002/10/31 23:14:21  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.3  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

