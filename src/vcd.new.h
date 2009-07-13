#ifndef __VCD_H__
#define __VCD_H__

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
 \file     vcd.h
 \author   Trevor Williams (phase1geo@gmail.com)
 \date     7/21/2002
 \brief    Contains VCD parser functions.
*/

/*!
 List of VCD tokens.
*/
enum vcd_tokens {
  T_VAR,
  T_END,
  T_SCOPE,
  T_UPSCOPE,
  T_COMMENT,
  T_DATE,
  T_DUMPALL,
  T_DUMPOFF,
  T_DUMPON,
  T_DUMPVARS,
  T_ENDDEF,
  T_DUMPPORTS,
  T_DUMPPORTSOFF,
  T_DUMPPORTSON,
  T_DUMPPORTSALL,
  T_TIMESCALE,
  T_VERSION,
  T_VCDCLOSE,
  T_EOF,
  T_STRING,
  T_UNKNOWN
};

/*! \brief Parses specified VCD file, storing information into database. */
void vcd_parse(
  const char* vcd_file
);

#endif

