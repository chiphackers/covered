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
 \file     globals.c
 \author   Trevor Williams  (phase1geo@gmail.com
 \date     12/2/2009
 \brief    Contains global variables needed in the libraries.
*/

#include "defines.h"

bool                     report_gui                  = FALSE;
int                      timestep_update             = 0;
bool                     report_covered              = FALSE;
bool                     flag_use_line_width         = FALSE;
int                      line_width                  = DEFAULT_LINE_WIDTH;
bool                     report_instance             = FALSE;
int                      flag_race_check             = WARNING;
tnode*                   def_table                   = NULL;
int                      fork_depth                  = -1;
int*                     fork_block_depth;
int                      block_depth                 = 0;
int                      merge_in_num;
char**                   merge_in                    = NULL;
char*                    top_module                  = NULL;
char*                    top_instance                = NULL;
unsigned int             flag_global_generation      = GENERATION_SV;
int                      generate_expr_mode          = 0;
bool                     cli_debug_mode              = FALSE;
bool                     flag_use_command_line_debug = FALSE;
struct exception_context the_exception_context[1];
str_link*                merge_in_head               = NULL;
str_link*                merge_in_tail               = NULL;
char*                    cdd_message                 = NULL;
char*                    merged_file                 = NULL;
bool                     flag_output_exclusion_ids   = FALSE;
bool                     report_exclusions           = FALSE;
str_link*                race_ignore_mod_head        = NULL;
str_link*                race_ignore_mod_tail        = NULL;
bool                     instance_specified          = FALSE;
unsigned int             inline_comb_depth           = 0xffffffff;
int                      merge_er_value              = MERGE_ER_NONE;
str_link*                gen_mod_head                = NULL;
