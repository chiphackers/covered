#ifndef __GUI_H__
#define __GUI_H__

/*!
 \file     gui.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/24/2003
 \brief    Contains functions that are used by the GUI report viewer.
*/

#include "defines.h"

/*! \brief Collects array of uncovered lines from given module. */
bool line_collect_uncovered( char* mod_name, int** lines, int* line_cnt );

/*! \brief Collects array of module names from the design. */
bool module_get_list( char*** mod_list, int* mod_size );

/*! \brief Retrieves filename of given module. */
char* module_get_filename( char* mod_name );

/*
 $Log$
*/

#endif

