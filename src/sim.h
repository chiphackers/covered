#ifndef __SIM_H__
#define __SIM_H__

/*!
 \file     sim.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/20/2002
 \brief    Contains functions for simulation engine.
*/


#include "defines.h"

//! Adds specified expression's statement to pre-simulation statement queue.
void sim_add_to_queue( expression* expr );

//! Simulates current timestep.
void sim_simulate();

/* $Log$
/* Revision 1.1  2002/06/21 05:55:05  phase1geo
/* Getting some codes ready for writing simulation engine.  We should be set
/* now.
/* */

#endif
