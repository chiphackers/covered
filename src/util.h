#ifndef __UTIL_H__
#define __UTIL_H__

/*!
 \file     util.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
 \brief    Contains miscellaneous global functions used by many functions.
*/

#include <stdio.h>

#include "defines.h"


//! Sets error suppression to specified value
void set_output_suppression( bool value );

//! Displays error message to standard output.
void print_output( char* msg, int type );

//! Returns TRUE if the specified string is a legal variable name.
bool is_variable( char* token );

//! Returns TRUE if the specified string is a legal directory string.
bool is_directory( char* token );

//! Returns TRUE if the specified directory exists.
bool directory_exists( char* dir );

//! Loads contents of specified directory to file list if extension is part of list.
void directory_load( char* dir, str_link* ext_head, str_link** file_head, str_link** file_tail );

//! Returns TRUE if the specified file exists.
bool file_exists( char* file );

//! Reads line from file and returns it in string form.
bool readline( FILE* file, char** line );

//! Extracts highest level of hierarchy from specified scope.
void scope_extract_front( char* scope, char* front, char* rest );

//! Extracts lowest level of hierarchy from specified scope.
void scope_extract_back( char* scope, char* back, char* rest );

//! Returns TRUE if specified scope is local (contains no periods).
bool scope_local( char* scope );

//! Performs safe malloc call.
void* malloc_safe(size_t size );

//! Performs safe deallocation of heap memory.
void free_safe( void* ptr );

//! Creates a string containing space characters.
void gen_space( char* spaces, int num_spaces );

/* $Log$
/* Revision 1.5  2002/07/18 22:02:35  phase1geo
/* In the middle of making improvements/fixes to the expression/signal
/* binding phase.
/*
/* Revision 1.4  2002/07/08 12:35:31  phase1geo
/* Added initial support for library searching.  Code seems to be broken at the
/* moment.
/*
/* Revision 1.3  2002/07/03 03:31:11  phase1geo
/* Adding RCS Log strings in files that were missing them so that file version
/* information is contained in every source and header file.  Reordering src
/* Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
/* */

#endif

