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
void fsm_report( FILE* ofile, bool verbose, bool instance );

#endif
