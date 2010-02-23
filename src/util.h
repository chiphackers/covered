#ifndef __UTIL_H__
#define __UTIL_H__

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
 \file     util.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
 \brief    Contains miscellaneous global functions used by many functions.
*/

#include <stdio.h>

#include "defines.h"
#include "profiler.h"


/*! Overload for the malloc_safe function which includes profiling information */
#define malloc_safe(x)              malloc_safe1(x,__FILE__,__LINE__,profile_index)

/*! Overload for the malloc_safe_nolimit function which includes profiling information */
#define malloc_safe_nolimit(x)      malloc_safe_nolimit1(x,__FILE__,__LINE__,profile_index)

/*! Overload for the strdup_safe function which includes profiling information */
#define strdup_safe(x)              strdup_safe1(x,__FILE__,__LINE__,profile_index)

/*! Overload for the realloc_safe1 function which includes profiling information */
#define realloc_safe(x,y,z)         realloc_safe1(x,(((x)!=NULL)?y:0),z,__FILE__,__LINE__,profile_index)

/*! Overload for the realloc_safe_nolimit1 function which includes profiling information */
#define realloc_safe_nolimit(x,y,z) realloc_safe_nolimit1(x,(((x)!=NULL)?y:0),z,__FILE__,__LINE__,profile_index)

/*! Overload for the calloc_safe1 function which includes profiling information */
#define calloc_safe(x,y)            calloc_safe1(x,y,__FILE__,__LINE__,profile_index)

/*! Overload for the free-safe function which includes profiling information */
#ifdef TESTMODE
#define free_safe(x,y)              free_safe2(x,(((x)!=NULL)?y:0),__FILE__, __LINE__,profile_index)
#else
#define free_safe(x,y)              free_safe1(x,profile_index)
#endif


/*! \brief Sets quiet output variable to specified value */
void set_quiet(
  bool value
);

/*! \brief Sets terse output variable to specified value */
void set_terse(
  bool value
);

/*! \brief Sets global debug flag to specified value */
void set_debug(
  bool value
);

/*! \brief Sets the testmode global variable for outputting purposes */
void set_testmode();

/*! \brief Displays error message to standard output. */
void print_output(
  const char* msg,
  int         type,
  const char* file,
  int         line
);

/*! \brief Checks to make sure that a value was properly specified for a given option. */
bool check_option_value(
  int          argc,
  const char** argv,
  int          option_index
);

/*! \brief Returns TRUE if the specified string is a legal variable name. */
bool is_variable(
  const char* token
);

/*! \brief Returns TRUE if the specified string could be a valid filename. */
bool is_legal_filename(
  const char* token
);

/*! \brief Returns TRUE if the specified string is a legal functional unit value. */
bool is_func_unit(
  const char* token
);

/*! \brief Extracts filename from file pathname. */
const char* get_basename(
  /*@returned@*/ const char* str
);

/*! \brief Extracts directory path from file pathname. */
char* get_dirname(
  char* str
);

/*! \brief Allocates memory for and gets the absolute pathname for a given filename. */
char* get_absolute_path(
  const char* filename
);

/*! \brief Allocates memory for and gets the relative pathname for a given absolute filename. */
char* get_relative_path(
  const char* abs_path
);

/*! \brief Returns TRUE if the specified directory exists. */
bool directory_exists(
  const char* dir
);

/*! \brief Loads contents of specified directory to file list if extension is part of list. */
void directory_load(
  const char*     dir,
  const str_link* ext_head,
  str_link**      file_head,
  str_link**      file_tail
);

/*! \brief Returns TRUE if the specified file exists. */
bool file_exists(
  const char* file
);

/*! \brief Reads line from file and returns it in string form. */
bool util_readline(
            FILE*  file,
  /*@out@*/ char** line,
  /*@out@*/ unsigned int*   line_size
);

/*! \brief Reads in line from file and returns the contents of the quoted string following optional whitespace */
bool get_quoted_string(
            FILE* file,
  /*@out@*/ char* line
);

/*! \brief Searches the specified string for environment variables and substitutes their value if found */
char* substitute_env_vars(
  const char* value
);

/*! \brief Extracts highest level of hierarchy from specified scope. */
void scope_extract_front(
            const char* scope,
  /*@out@*/ char*       front,
  /*@out@*/ char*       rest
);

/*! \brief Extracts lowest level of hierarchy from specified scope. */
void scope_extract_back(
            const char* scope,
  /*@out@*/ char*       back,
  /*@out@*/ char*       rest
);

/*! \brief Extracts rest of scope not included in front. */
void scope_extract_scope(
            const char* scope,
            const char* front,
  /*@out@*/ char*       back
);

/*! \brief Generates printable version of given signal/instance string */
/*@only@*/ char* scope_gen_printable(
  const char* str
);

/*! \brief Compares two signal names or two instance names. */
bool scope_compare(
  const char* str1,
  const char* str2
);

/*! \brief Returns TRUE if specified scope is local (contains no periods). */
bool scope_local(
  const char* scope
);

/*! \brief Returns next Verilog file to parse. */
str_link* get_next_vfile(
  str_link*   curr,
  const char* mod
);

/*! \brief Performs safe malloc call. */
/*@only@*/ void* malloc_safe1(
  size_t       size,
  const char*  file,
  int          line,
  unsigned int profile_index
);

/*! \brief Performs safe malloc call without upper bound on byte allocation. */
/*@only@*/ void* malloc_safe_nolimit1(
  size_t       size,
  const char*  file,
  int          line,
  unsigned int profile_index
);

/*! \brief Performs safe deallocation of heap memory. */
void free_safe1(
  /*@only@*/ /*@out@*/ /*@null@*/ void*        ptr,
                                  unsigned int profile_index
) /*@releases ptr@*/;

/*! \brief Performs safe deallocation of heap memory. */
void free_safe2(
  /*@only@*/ /*@out@*/ /*@null@*/ void*        ptr,
                                  size_t       size,
                                  const char*  file,
                                  int          line,
                                  unsigned int profile_index
) /*@releases ptr@*/;

/*! \brief Safely allocates heap memory by performing a call to strdup */
/*@only@*/ char* strdup_safe1(
  const char*  str,
  const char*  file,
  int          line,
  unsigned int profile_index
);

/*! \brief Safely reallocates heap memory by performing a call to realloc */
/*@only@*/ void* realloc_safe1(
  /*@null@*/ void*        ptr,
             size_t       old_size,
             size_t       size,
             const char*  file,
             int          line,
             unsigned int profile_index
);

/*! \brief Safely reallocates heap memory by performing a call to realloc */
/*@only@*/ void* realloc_safe_nolimit1(
  /*@null@*/ void*        ptr,
             size_t       old_size,
             size_t       size,
             const char*  file,
             int          line,
             unsigned int profile_index
);

/*! \brief Safely callocs heap memory by performing a call to calloc */
/*@only@*/ void* calloc_safe1(
  size_t       num,
  size_t       size,
  const char*  file,
  int          line,
  unsigned int profile_index
);

/*! \brief Creates a string containing space characters. */
void gen_char_string(
  /*@out@*/ char* spaces,
            char  c,
            int   num_spaces
);

/*! \brief Removes underscores from the specified string. */
char* remove_underscores(
  char* str
);

#ifdef HAVE_SYS_TIME_H
/*! \brief Clears the timer, resetting the accumulated time information and allocating timer memory, if needed */
void timer_clear(
  /*@out@*/ timer** tm
);

/*! \brief Starts timing the specified timer structure and allocates/clears timer memory, if needed. */
void timer_start(
  /*@out@*/ timer** tm
);

/*! \brief Stops timing the specified timer structure. */
void timer_stop(
  /*@out@*/ timer** tm
);

/*! \brief Generates a human-readable time-of-day string from the given timer structure */
char* timer_to_string(
  timer* tm
);
#endif

/*! \brief Returns string representation of the specified functional unit type */
const char* get_funit_type(
  int type
);

/*! \brief Calculates miss and percent information from given hit and total information */
void calc_miss_percent(
            int    hits,
            int    total,
  /*@out@*/ int*   misses,
  /*@out@*/ float* percent );

/*! \brief Sets the given timestep to the correct value from VCD simulation file */
void set_timestep(
  sim_time* st,
  char*     value
);

/*! \brief Reads in contents of command file, substitutes environment variables and stores them to the arg_list array. */
void read_command_file(
            const char* cmd_file,
  /*@out@*/ char***     arg_list,
  /*@out@*/ int*        arg_num
);

/*! \brief Generates the exclusion identifier */
void gen_exclusion_id(
  char* excl_id,
  char  type,
  int   id
);

/*! \brief Converts a VCD string value to a legal unsigned 64-bit integer value. */        
bool convert_str_to_uint64(   
  const char* str,
  int         msb,
  int         lsb,
  uint64*     value
);

/*! \brief Converts an integer value to a string, allocating memory for the string. */
char* convert_int_to_str(
  int value
);

/*! \brief Calculates the number of bits needed to store the given number of values. */
int calc_num_bits_to_store(
  int values
);

#endif

