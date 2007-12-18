#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

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
 \file     symtable.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     1/3/2002
 \brief    Contains functions for manipulating a symtable structure.
*/

#include "defines.h"


/*! \brief Creates a new symtable structure. */
symtable* symtable_create();

/*! \brief Creates a new symtable entry and adds it to the specified symbol table. */
void symtable_add( char* sym, vsignal* sig, int msb, int lsb );

/*! \brief Sets all matching symtable entries to specified value */
void symtable_set_value( char* sym, char* value );

/*! \brief Assigns stored values to all associated signals stored in specified symbol table. */
void symtable_assign( const sim_time* time );

/*! \brief Deallocates all symtable entries for specified symbol table. */
void symtable_dealloc( symtable* symtab );


/*
 $Log$
 Revision 1.17  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.16  2006/11/27 04:11:42  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.15  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.14  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.13  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.12  2003/08/21 21:57:30  phase1geo
 Fixing bug with certain flavors of VCD files that alias signals that have differing
 MSBs and LSBs.  This takes care of the rest of the bugs for the 0.2 stable release.

 Revision 1.11  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.10  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.9  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.8  2003/01/03 05:52:01  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.7  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.6  2002/10/31 23:14:30  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.5  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.4  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.3  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

