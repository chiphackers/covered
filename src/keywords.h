#ifndef __KEYWORDS_H__
#define __KEYWORDS_H__

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
 \file     keywords.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/2/2001
 \brief    Contains functions for checking Verilog keywords.
*/

/*! \brief Returns the defined value of the keyword or IDENTIFIER if this is not a Verilog keyword. */
extern int lexer_keyword_1995_code( const char* str, int length );

/*! \brief Returns the defined value of the keyword or IDENTIFIER if this is not a Verilog keyword. */
extern int lexer_keyword_2001_code( const char* str, int length );

/*! \brief Returns the defined value of the keyword or IDENTIFIER if this is not a Verilog keyword. */
extern int lexer_keyword_sv_code( const char* str, int length );

#endif

