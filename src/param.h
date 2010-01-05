#ifndef __PARAM_H__
#define __PARAM_H__

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
 \file     param.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/22/2002
 \brief    Contains functions and structures necessary to handle parameters.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Searches specified module parameter list for matching parameter. */
mod_parm* mod_parm_find(
  const char* name,
  mod_parm* parm
);

/*! \brief Generates a string version of given signal's width (size) if it exists */
char* mod_parm_gen_size_code(
            vsignal*     sig,
            unsigned int dimension,
            func_unit*   mod,
  /*@out@*/ int*         number
);

/*! \brief Generates a string version of given signal's LSB if it exists */
char* mod_parm_gen_lsb_code(
            vsignal*     sig,
            unsigned int dimension,
            func_unit*   mod,
  /*@out@*/ int*         number
);

/*! \brief Creates new module parameter and adds it to the specified list. */
mod_parm* mod_parm_add(
  char*        scope,
  static_expr* msb,
  static_expr* lsb,
  bool         is_signed,
  expression*  expr,
  int          type,
  func_unit*   funit,
  char*        inst_name
);

/*! \brief Outputs contents of module parameter list to standard output. */
int mod_parm_display(
  mod_parm* mparm
);

/*! \brief Creates a new instance parameter for a generate variable */
void inst_parm_add_genvar(
  vsignal*    sig,
  funit_inst* inst
);

/*! \brief Performs bind of signal and expressions for the given instance parameter. */
void inst_parm_bind(
  inst_parm* iparm
);

/*! \brief Adds parameter override to defparam list. */
void defparam_add(
  const char* scope,
  vector*     expr
);

/*! \brief Deallocates all memory associated with defparam storage from command-line */
void defparam_dealloc();

/*! \brief Sets the specified signal size according to the specified instance parameter */
void param_set_sig_size(
  vsignal*   sig,
  inst_parm* icurr
);

/*! \brief Evaluates parameter expression for the given instance. */
void param_expr_eval(
  expression* expr,
  funit_inst* inst
);

/*! \brief Resolves all parameters for the specified instance. */
void param_resolve_inst(
  funit_inst* inst
);

/*! \brief Resolves all parameters for the specified instance tree. */
void param_resolve(
  funit_inst* inst
);

/*! \brief Outputs specified instance parameter to specified output stream. */
void param_db_write(
  inst_parm* iparm,
  FILE*      file
);

/*! \brief Deallocates specified module parameter and possibly entire module parameter list. */
void mod_parm_dealloc(
  mod_parm* parm,
  bool      recursive
);

/*! \brief Deallocates specified instance parameter and possibly entire instance parameter list. */
void inst_parm_dealloc(
  inst_parm* parm,
  bool       recursive
);

#endif

