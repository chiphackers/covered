#ifndef __REPORT_H__
#define __REPORT_H__

/*!
 \file     report.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/29/2001
 \brief    Contains functions for report command.
*/


/*! \brief Parses command-line for report command and performs report functionality. */
int command_report( int argc, int last_arg, char** argv );

/*! \brief Reads the CDD and readies the design for reporting */
bool report_read_cdd_and_ready( char* ifile, int read_mode );


/*
 $Log$
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

