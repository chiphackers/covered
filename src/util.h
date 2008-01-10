#ifndef __UTIL_H__
#define __UTIL_H__

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
 \file     util.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
 \brief    Contains miscellaneous global functions used by many functions.
*/

#include <stdio.h>

#include "defines.h"
#include "profiler.h"


/*! Overload for the malloc_safe function which includes profiling information */
#define malloc_safe(x)         malloc_safe1(x,__FILE__,__LINE__,profile_index)

/*! Overload for the malloc_safe_nolimit function which includes profiling information */
#define malloc_safe_nolimit(x) malloc_safe_nolimit1(x,__FILE__,__LINE__,profile_index)

/*! Overload for the strdup_safe function which includes profiling information */
#define strdup_safe(x)         strdup_safe1(x,__FILE__,__LINE__,profile_index)

/*! Overload for the free-safe function which includes profiling information */
#define free_safe(x)           free_safe1(x,profile_index)


/*! \brief Sets error suppression to specified value */
void set_output_suppression( bool value );

/*! \brief Sets global debug flag to specified value */
void set_debug( bool value );

/*! \brief Displays error message to standard output. */
void print_output( const char* msg, int type, const char* file, int line );

/*! \brief Checks to make sure that a value was properly specified for a given option. */
bool check_option_value( int argc, const char** argv, int option_index );

/*! \brief Returns TRUE if the specified string is a legal variable name. */
bool is_variable( const char* token );

/*! \brief Returns TRUE if the specified string could be a valid filename. */
bool is_legal_filename( const char* token );

/*! \brief Returns TRUE if the specified string is a legal functional unit value. */
bool is_func_unit( const char* token );

/*! \brief Extracts filename from file pathname. */
const char* get_basename( /*@returned@*/ const char* str );

/*! \brief Extracts directory path from file pathname. */
char* get_dirname( char* str );

/*! \brief Returns TRUE if the specified directory exists. */
bool directory_exists( const char* dir );

/*! \brief Loads contents of specified directory to file list if extension is part of list. */
void directory_load( const char* dir, const str_link* ext_head, str_link** file_head, str_link** file_tail );

/*! \brief Returns TRUE if the specified file exists. */
bool file_exists( const char* file );

/*! \brief Reads line from file and returns it in string form. */
bool util_readline( FILE* file, /*@out@*/char** line );

/*! \brief Searches the specified string for environment variables and substitutes their value if found */
char* substitute_env_vars( const char* value );

/*! \brief Extracts highest level of hierarchy from specified scope. */
void scope_extract_front( const char* scope, /*@out@*/char* front, /*@out@*/char* rest );

/*! \brief Extracts lowest level of hierarchy from specified scope. */
void scope_extract_back( const char* scope, /*@out@*/char* back, /*@out@*/char* rest );

/*! \brief Extracts rest of scope not included in front. */
void scope_extract_scope( const char* scope, const char* front, /*@out@*/char* back );

/*! \brief Generates printable version of given signal/instance string */
/*@only@*/ char* scope_gen_printable( const char* str );

/*! \brief Compares two signal names or two instance names. */
bool scope_compare( const char* str1, const char* str2 );

/*! \brief Returns TRUE if specified scope is local (contains no periods). */
bool scope_local( const char* scope );

/*! \brief Returns next Verilog file to parse. */
str_link* get_next_vfile( str_link* curr, const char* mod );

/*! \brief Performs safe malloc call. */
/*@only@*/ void* malloc_safe1( size_t size, const char* file, int line, unsigned int profile_index );

/*! \brief Performs safe malloc call without upper bound on byte allocation. */
/*@only@*/ void* malloc_safe_nolimit1( size_t size, const char* file, int line, unsigned int profile_index );

/*! \brief Performs safe deallocation of heap memory. */
void free_safe1( /*@only@*/ /*@out@*/ /*@null@*/ void* ptr, unsigned int profile_index ) /*@releases ptr@*/;

/*! \brief Safely allocates heap memory by performing a call to strdup */
/*@only@*/ char* strdup_safe1( const char* str, const char* file, int line, unsigned int profile_index );

/*! \brief Creates a string containing space characters. */
void gen_space( char* spaces, int num_spaces );

#ifdef HAVE_SYS_TIME_H
/*! \brief Starts timing the specified timer structure. */
void timer_start( timer** tm );

/*! \brief Stops timing the specified timer structure. */
void timer_stop( timer** tm );
#endif

/*! \brief Returns string representation of the specified functional unit type */
const char* get_funit_type( int type );

/*! \brief Calculates miss and percent information from given hit and total information */
void calc_miss_percent( int hits, float total, /*@out@*/ float* misses, /*@out@*/ float* percent );

/*! \brief Sets the given timestep to the correct value from VCD simulation file */
void set_timestep( sim_time* st, char* value );



/*
 $Log$
 Revision 1.36  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.35  2008/01/08 13:27:46  phase1geo
 More splint updates.

 Revision 1.34  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.33  2008/01/07 05:01:58  phase1geo
 Cleaning up more splint errors.

 Revision 1.32  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.31  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.30  2007/12/10 23:16:22  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.29  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.28  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.27  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.26  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.25  2007/03/13 22:12:59  phase1geo
 Merging changes to covered-0_5-branch to fix bug 1678931.

 Revision 1.24.2.1  2007/03/13 22:05:11  phase1geo
 Fixing bug 1678931.  Updated regression.

 Revision 1.24  2006/08/18 22:32:57  phase1geo
 Adding get_dirname routine to util.c for future use.

 Revision 1.23  2006/08/18 04:41:14  phase1geo
 Incorporating bug fixes 1538920 and 1541944.  Updated regressions.  Only
 event1.1 does not currently pass (this does not pass in the stable version
 yet either).

 Revision 1.22  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.21  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.20  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.19  2006/01/14 04:17:23  phase1geo
 Adding is_func_unit function to check to see if a -e value is a valid module, function,
 task or named begin/end block.  Updated regression accordingly.  We are getting closer
 but still have a few diagnostics to figure out yet.

 Revision 1.18  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.

 Revision 1.17  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.16  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.15  2003/10/03 03:08:44  phase1geo
 Modifying filename in summary output to only specify basename of file instead
 of entire path.  The verbose report contains the full pathname still, however.

 Revision 1.14  2003/08/15 20:02:08  phase1geo
 Added check for sys/times.h file for new code additions.

 Revision 1.13  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.12  2003/02/17 22:47:21  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.11  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.10  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.9  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.8  2002/10/31 23:14:31  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.7  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.6  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.5  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.4  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.3  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

