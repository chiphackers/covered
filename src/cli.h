#ifndef __CLI_H__
#define __CLI_H__

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
 \file     cli.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     4/11/2007
 \brief    Contains functions for handling the score command CLI debugger.
*/


#include "defines.h"


/*! \brief Performs CLI management. */
void cli_execute(
  const sim_time* time,
  bool            force,
  statement*      stmt
);

/*! \brief Reads in given history file from -cli option */
void cli_read_hist_file(
  const char* fname
);

/*! \brief Signal handler for Ctrl-C event */
void cli_ctrl_c(
  int sig
);

#endif

