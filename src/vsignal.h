#ifndef __VSIGNAL_H__
#define __VSIGNAL_H__

/*!
 \file     vsignal.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains functions for handling Verilog signals.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Initializes specified vsignal with specified values. */
void vsignal_init( vsignal* sig, char* name, vector* value, int lsb );

/*! \brief Creates a new vsignal based on the information passed to this function. */
vsignal* vsignal_create( char* name, int width, int lsb );

/*! \brief Outputs this vsignal information to specified file. */
void vsignal_db_write( vsignal* sig, FILE* file );

/*! \brief Reads vsignal information from specified file. */
bool vsignal_db_read( char** line, func_unit* curr_funit, func_unit* last_funit );

/*! \brief Reads and merges two vsignals, placing result into base vsignal. */
bool vsignal_db_merge( vsignal* base, char** line, bool same );

/*! \brief Reads and replaces an original vsignal with a new vsignal. */
bool vsignal_db_replace( vsignal* base, char** line );

/*! \brief Sets value of currently waiting bit of vsignal to specified value. */
void vsignal_set_wait_bit( vsignal* sig, int val );

/*! \brief Gets value of currently waiting bit of vsignal. */
int vsignal_get_wait_bit( vsignal* sig );

/*! \brief Sets vector value assigned bits and returns overlapping indicator. */
bool vsignal_set_assigned( vsignal* sig, int msb, int lsb );

/*! \brief Propagates specified signal information to rest of design. */
void vsignal_propagate( vsignal* sig );

/*! \brief Assigns specified VCD value to specified vsignal. */
void vsignal_vcd_assign( vsignal* sig, char* value, int msb, int lsb );

/*! \brief Adds an expression to the vsignal list. */
void vsignal_add_expression( vsignal* sig, expression* expr );

/*! \brief Displays vsignal contents to standard output. */
void vsignal_display( vsignal* sig );

/*! \brief Converts a string to a vsignal. */
vsignal* vsignal_from_string( char** str );

/*! \brief Deallocates the memory used for this vsignal. */
void vsignal_dealloc( vsignal* sig );


/*
 $Log$
 Revision 1.5  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.4  2005/02/16 13:45:04  phase1geo
 Adding value propagation function to vsignal.c and adding this propagation to
 BASSIGN expression assignment after the assignment occurs.

 Revision 1.3  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.2  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.1  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.16  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.15  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.14  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.13  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.

 Revision 1.12  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.11  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.10  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.9  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.8  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.7  2002/10/31 23:14:24  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.4  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.3  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.
*/

#endif

