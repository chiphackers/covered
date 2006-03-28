#ifndef __LXT_H__
#define __LXT_H__

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
 \file     lxt.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     1/25/2006
 \brief    Contains functions for LXT parsing/simulation
*/


/*! \brief  Parses and scores LXT-style dumpfile */
void lxt_parse( char* lxt_file );


/*
 $Log$
 Revision 1.1  2006/01/26 17:34:04  phase1geo
 Finished work on lxt2_read.c and fixed bug in LXT-based simulator.  Also adding
 missing lxt.h header file.

*/

#endif

