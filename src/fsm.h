#ifndef __FSM_H__
#define __FSM_H__

/*!
 \file     fsm.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting FSM coverage.
*/

#include <stdio.h>

#include "defines.h"

//! Generates report output for FSM coverage.
void fsm_report( FILE* ofile, bool verbose );

/* $Log$
/* Revision 1.2  2002/07/03 03:31:11  phase1geo
/* Adding RCS Log strings in files that were missing them so that file version
/* information is contained in every source and header file.  Reordering src
/* Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
/* */

#endif
