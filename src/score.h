#ifndef __SCORE_H__
#define __SCORE_H__

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

#endif

