#ifndef __COVERED_VERILATOR_H__
#define __COVERED_VERILATOR_H__

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

#include "db.h"

/*!
 Called from Verilog $c call which gathers line coverage for the given expression in
 the given instance.
*/
inline void covered_line(
  unsigned inst_index,
  unsigned expr_index
) {

  db_add_line_coverage( inst_index, expr_index );

}

#endif

