#ifndef __VSIGNAL_H__
#define __VSIGNAL_H__

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
 \file     vsignal.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/1/2001
 \brief    Contains functions for handling Verilog signals.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Creates a new vsignal based on the information passed to this function. */
vsignal* vsignal_create(
  const char*  name,
  unsigned int type,
  unsigned int width,
  unsigned int line,
  unsigned int col
);

/*! \brief Creates the vector for a given signal based on the values of its dimension information */
void vsignal_create_vec(
  vsignal* sig
);

/*! \brief Duplicates the given signal and returns a newly allocated signal */
vsignal* vsignal_duplicate(
  vsignal* sig
);

/*! \brief Outputs this vsignal information to specified file. */
void vsignal_db_write(
  vsignal* sig,
  FILE*    file
);

/*! \brief Reads vsignal information from specified file. */
void vsignal_db_read(
             char** line,
  /*@null@*/ func_unit* curr_funit
);

/*! \brief Reads and merges two vsignals, placing result into base vsignal. */
void vsignal_db_merge(
  vsignal* base,
  char**   line,
  bool     same
);

/*! \brief Merges two vsignals, placing the result into the base vsignal. */
void vsignal_merge(
  vsignal* base,
  vsignal* other
);

/*! \brief Propagates specified signal information to rest of design. */
void vsignal_propagate(
  vsignal*        sig,
  const sim_time* time
);

/*! \brief Assigns specified VCD value to specified vsignal. */
void vsignal_vcd_assign(
  vsignal*        sig,
  const char*     value,
  unsigned int    msb,
  unsigned int    lsb,
  const sim_time* time
);

/*! \brief Adds an expression to the vsignal list. */
void vsignal_add_expression(
  vsignal*    sig,
  expression* expr
);

/*! \brief Displays vsignal contents to standard output. */
void vsignal_display(
  vsignal* sig
);

/*! \brief Converts a string to a vsignal. */
vsignal* vsignal_from_string(
  char** str
);

/*! \brief Calculates width of the specified signal's vector value based on the given expression */
int vsignal_calc_width_for_expr(
  expression* expr,
  vsignal*    sig
);

/*! \brief Calculates LSB of the specified signal's vector value based on the given expression */
int vsignal_calc_lsb_for_expr(
  expression* expr,
  vsignal*    sig,
  int         lsb_val
);

/*! \brief Deallocates the memory used for this vsignal. */
void vsignal_dealloc(
  vsignal* sig
);

#endif

