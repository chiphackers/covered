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
void sim_expr_changed( expression* expr );

//! Adds specified statement to pre-simulation statement queue.
void sim_add_stmt_to_queue( statement* stmt );

//! Simulates current timestep.
void sim_simulate();

/* $Log$
/* Revision 1.2  2002/06/22 21:08:23  phase1geo
/* Added simulation engine and tied it to the db.c file.  Simulation engine is
/* currently untested and will remain so until the parser is updated correctly
/* for statements.  This will be the next step.
/*
/* Revision 1.1  2002/06/21 05:55:05  phase1geo
/* Getting some codes ready for writing simulation engine.  We should be set
/* now.
/* */

#endif
