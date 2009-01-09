#ifndef __SCORE_H__
#define __SCORE_H__

/*
 Copyright (c) 2006-2009 Trevor Williams

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
 \file     score.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/29/2001
 \brief    Contains functions for score command.
*/


/*! \brief Creates a module that contains all of the signals to dump from the design for coverage purposes. */
void score_generate_top_dumpvars_module( const char* dumpvars_file );

/*! \brief Parses the specified define from the command-line */
void score_parse_define( const char* def );

/*! \brief Parses score command-line and performs score. */
void command_score( int argc, int last_arg, const char** argv );


/*
 $Log$
 Revision 1.15  2008/11/13 05:08:36  phase1geo
 Fixing bug found with merge8.5 diagnostic and fixing issues with VPI.  Full
 regressions now pass.

 Revision 1.14  2008/11/12 07:04:01  phase1geo
 Fixing argument merging and updating regressions.  Checkpointing.

 Revision 1.13  2008/10/07 05:24:18  phase1geo
 Adding -dumpvars option.  Need to resolve a few issues before this work is considered
 complete.

 Revision 1.12  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.11  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.10  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.9  2006/05/03 22:49:42  phase1geo
 Causing all files to be preprocessed when written to the file viewer.  I'm sure that
 I am breaking all kinds of things at this point, but things do work properly on a few
 select test cases so I'm checkpointing here.

 Revision 1.8  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.7  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.6  2002/10/31 23:14:19  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.5  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.4  2002/07/20 21:34:58  phase1geo
 Separating ability to parse design and score dumpfile.  Now both or either
 can be done (allowing one to parse once and score multiple times).
*/

#endif

