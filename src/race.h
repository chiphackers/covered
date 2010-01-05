#ifndef __RACE_H__
#define __RACE_H__

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
 \file     race.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/15/2004
 \brief    Contains functions used to check for race conditions and proper signal use
           within the specified design.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Checks the current module for race conditions */
void race_check_modules();

/*! \brief Writes contents of specified race condition block to specified file output */
void race_db_write(
  race_blk* head,
  FILE*     file
);

/*! \brief Reads contents from specified line for a race condition block and assigns the new block to the curr_mod */
void race_db_read(
             char**     line,
  /*@null@*/ func_unit* curr_mod
);

/*! \brief Get statistic information for the specified race condition block list */
void race_get_stats(
            race_blk*     curr,
  /*@out@*/ unsigned int* race_total,
  /*@out@*/ unsigned int  type_total[][RACE_TYPE_NUM]
);

/*! \brief Displays report information for race condition blocks in design */
void race_report(
  FILE* ofile,
  bool  verbose
);

/*! \brief Collects all of the lines in the specified module that were not verified due to race condition breach */
void race_collect_lines(
            func_unit* funit,
  /*@out@*/ int**      slines,
  /*@out@*/ int**      elines,
  /*@out@*/ int**      reasons,
  /*@out@*/ int*       line_cnt
);

/*! \brief Deallocates the specified race condition block from memory */
void race_blk_delete_list(
  race_blk* rb
);

#endif

