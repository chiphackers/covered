#ifndef __SEARCH_H__
#define __SEARCH_H__

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
 \file     search.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
 \brief    Contains functions used for finding Verilog files in the search information
           provided on the command line.
*/

#include "defines.h"


/*! \brief Initializes search maintained pointers. */
void search_init();

/*! \brief Adds an include directory to the list of directories to search for `include directives. */
void search_add_include_path( const char* path );

/*! \brief Adds a directory to the list of directories to find unspecified Verilog modules. */
void search_add_directory_path( const char* path );

/*! \brief Adds a specific Verilog module to the list of modules to score. */
void search_add_file( const char* file );

/*! \brief Adds specified functional unit to list of functional units not to score. */
void search_add_no_score_funit( const char* funit );

/*! \brief Adds specified extensions to allowed file extension list. */
void search_add_extensions( const char* ext_list );

/*! \brief Deallocates all used memory for search lists. */
void search_free_lists();

#endif

