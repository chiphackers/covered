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

//! Returns TRUE if the specified file exists.
bool file_exists( char* file );

//! Reads line from file and returns it in string form.
bool readline( FILE* file, char** line );

//! Extracts highest level of hierarchy from specified scope.
void scope_extract_front( char* scope, char* front, char* rest );

//! Extracts lowest level of hierarchy from specified scope.
void scope_extract_back( char* scope, char* back, char* rest );

//! Performs safe malloc call.
void* malloc_safe( int size );

//! Performs safe deallocation of heap memory.
void free_safe( void* ptr );

//! Creates a string containing space characters.
void gen_space( char* spaces, int num_spaces );

#endif

