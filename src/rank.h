#ifndef __RANK_H__
#define __RANK_H__

/*
 Copyright (c) 2008-2009 Trevor Williams

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
 \file     rank.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/28/2008
 \brief    Contains functions for rank command.
*/


/*! \brief Parses command-line for rank options and performs rank command. */
void command_rank( int argc, int last_arg, const char** argv );


/*
 $Log$
 Revision 1.2  2008/07/29 06:34:22  phase1geo
 Merging in contents of development branch to the main development trunk.

 Revision 1.1.4.1  2008/07/10 22:43:54  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.1.2.1  2008/06/30 13:14:22  phase1geo
 Starting to work on new 'rank' command.  Checkpointing.

*/

#endif

