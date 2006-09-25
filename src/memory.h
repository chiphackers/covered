#ifndef __MEMORY_H__
#define __MEMORY_H__

/*!
 \file     memory.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     9/24/2006
 \brief    Contains functions for generating memory coverage reports
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Calculates line coverage numbers for the specified expression list. */
void memory_get_stats( sig_link* sigl, float* ae_total, int* write_hit, int* read_hit, float* tog_total, int* tog01_hit, int* tog10_hit );

/*! \brief Generates report output for line coverage. */
void memory_report( FILE* ofile, bool verbose );


/*
 $Log$
*/

#endif

