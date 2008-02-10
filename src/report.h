#ifndef __REPORT_H__
#define __REPORT_H__

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
 \file     report.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/29/2001
 \brief    Contains functions for report command.
*/


/*! \brief Parses command-line for report command and performs report functionality. */
void command_report( int argc, int last_arg, const char** argv );

/*! \brief Prints header of report. */
void report_print_header( FILE* ofile );

/*! \brief Parses arguments on report command command-line. */
void report_parse_args( int argc, int last_arg, const char** argv );

/*! \brief Reads the CDD and readies the design for reporting */
void report_read_cdd_and_ready( char* ifile, int read_mode );

/*! \brief Closes the currently loaded CDD */
void report_close_cdd();

/*! \brief Saves the currently loaded CDD as the specified filename */
void report_save_cdd( char* filename );


/*
 $Log$
 Revision 1.14  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.13  2008/02/08 23:58:07  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.12  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.11  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.10  2006/06/20 22:14:32  phase1geo
 Adding support for saving CDD files (needed for file merging and saving exclusion
 information for a CDD file) in the GUI.  Still have a bit to go as I am getting core
 dumps to occur.

 Revision 1.9  2006/06/16 22:44:19  phase1geo
 Beginning to add ability to open/close CDD files without needing to close Covered's
 GUI.  This seems to work but does cause some segfaults yet.

 Revision 1.8  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.7  2005/02/05 04:13:30  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.6  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:14:18  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

