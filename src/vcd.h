#ifndef __VCD_H__
#define __VCD_H__

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
 \file     vcd.h
 \author   Trevor Williams (phase1geo@gmail.com)
 \date     7/21/2002
 \brief    Contains VCD parser functions.
*/

/*! \brief Parses specified VCD file, storing information into database. */
void vcd_parse(
  const char* vcd_file
);

#endif

