#ifndef __SEARCH_H__
#define __SEARCH_H__

/*!
 \file     search.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
 \brief    Contains functions used for finding Verilog files in the search information
           provided on the command line.
*/

#include "defines.h"


//! Initializes search maintained pointers.
void search_init();

//! Adds an include directory to the list of directories to search for `include directives.
bool search_add_include_path( char* path );

//! Adds a directory to the list of directories to find unspecified Verilog modules.
bool search_add_directory_path( char* path );

//! Adds a specific Verilog module to the list of modules to score.
bool search_add_file( char* file );

//! Adds specified module to list of modules not to score.
bool search_add_no_score_module( char* module );

//! Adds specified extensions to allowed file extension list.
bool search_add_extensions( char* ext_list );

//! Deallocates all used memory for search lists.
void search_free_lists();

#endif

