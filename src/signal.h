#ifndef __SIGNAL_H__
#define __SIGNAL_H__

/*!
 \file     signal.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
 \brief    Contains functions for handling signals.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Initializes specified signal with specified values. */
void signal_init( signal* sig, char* name, vector* value );

/*! \brief Creates a new signal based on the information passed to this function. */
signal* signal_create( char* name, int width, int lsb );

/*! \brief Outputs this signal information to specified file. */
void signal_db_write( signal* sig, FILE* file, char* modname );

/*! \brief Reads signal information from specified file. */
bool signal_db_read( char** line, module* curr_mod );

/*! \brief Reads and merges two signals, placing result into base signal. */
bool signal_db_merge( signal* base, char** line, bool same );

/*! \brief Sets value of currently waiting bit of signal to specified value. */
void signal_set_wait_bit( signal* sig, int val );

/*! \brief Gets value of currently waiting bit of signal. */
int signal_get_wait_bit( signal* sig );

/*! \brief Assigns specified VCD value to specified signal. */
void signal_vcd_assign( signal* sig, char* value, int msb, int lsb );

/*! \brief Adds an expression to the signal list. */
void signal_add_expression( signal* sig, expression* expr );

/*! \brief Displays signal contents to standard output. */
void signal_display( signal* sig );

/*! \brief Converts a string to a signal. */
signal* signal_from_string( char** str );

/*! \brief Deallocates the memory used for this signal. */
void signal_dealloc( signal* sig );


/*
 $Log$
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

