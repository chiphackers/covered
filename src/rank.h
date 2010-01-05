#ifndef __RANK_H__
#define __RANK_H__

/*
 Copyright (c) 2008-2010 Trevor Williams

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

#endif

