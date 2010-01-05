#ifndef __FSM_ARG_H___
#define __FSM_ARG_H__

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
 \file     fsm_arg.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/02/2003
 \brief    Contains functions for handling FSM arguments from the command-line.
*/

#include "defines.h"


/*! \brief Parses specified -F argument for FSM information. */
void fsm_arg_parse(
  const char* arg
);

/*! \brief Parses specified attribute argument for FSM information. */
void fsm_arg_parse_attr(
  attr_param*      ap,
  const func_unit* funit,
  bool             exclude
);

#endif

