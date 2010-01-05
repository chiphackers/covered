#ifndef __INFO_H__
#define __INFO_H__

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
 \file     info.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/12/2003
 \brief    Contains functions for reading/writing info line of CDD file.
*/

#include <stdio.h>


/*! \brief Adds the given argument(s) from the command-line to the score array such that no arguments are duplicated. */
void score_add_args(
             const char* arg1,
  /*@null@*/ const char* arg2
);

/*! \brief Sets the scored bit in the information field. */
void info_set_scored();

/*! \brief Writes info line to specified CDD file. */
void info_db_write(
  FILE* file
);

/*! \brief Reads info line from specified line and stores information. */
bool info_db_read(
  char** line,
  int    read_mode
);

/*! \brief Reads score args line from specified line and stores information. */
void args_db_read(
  char** line
);

/*! \brief Reads user-specified message from specified line and stores information. */
void message_db_read(
  char** line
);

/*! \brief Reads merged CDD information from specified line and stores information. */
void merged_cdd_db_read(
  char** line
);

/*! \brief Deallocates all memory associated with the information section of a database file. */
void info_dealloc();

#endif

