#ifndef __STATIC_H__
#define __STATIC_H__

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
 \file     static.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/02/2002
 \brief    Contains functions for handling creation of static expressions.
*/

#include "defines.h"


/*! \brief Calculates new values for unary static expressions and returns the new static expression. */
static_expr* static_expr_gen_unary(
  static_expr* stexp,
  exp_op_type  op,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  int          first,
  int          last
);

/*! \brief Calculates new values for static expression and returns the new static expression. */
static_expr* static_expr_gen(
  static_expr* right,
  static_expr* left,
  int          op,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  int          first,
  int          last,
  char*        func_name
);

/*! \brief Calculates new values for ternary static expressions and returns the new static expression. */
static_expr* static_expr_gen_ternary( 
  static_expr* sel,
  static_expr* right,
  static_expr* left,
  unsigned int line,
  unsigned int ppfline,
  unsigned int pplline,
  int          first,
  int          last
);

/*! \brief Calculates LSB, width and endianness for specified left/right pair for vector (used before parameter resolve). */
void static_expr_calc_lsb_and_width_pre(
            static_expr*  left,
            static_expr*  right,
  /*@out@*/ unsigned int* width, 
  /*@out@*/ int*          lsb,
  /*@out@*/ int*          big_endian
);

/*! \brief Calculates LSB, width and endianness for specified left/right pair for vector (used after parameter resolve). */
void static_expr_calc_lsb_and_width_post(
            static_expr*  left,
            static_expr*  right,
  /*@out@*/ unsigned int* width,
  /*@out@*/ int*          lsb,
  /*@out@*/ int*          big_endian
);

/*! \brief Deallocates static_expr memory from heap. */
void static_expr_dealloc(
  static_expr* stexp,
  bool         rm_exp
);

#endif

