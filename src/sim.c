/*!
 \file     sim.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/20/2002
*/

#include "sim.h"

/*!
 \par
 This function is the heart of the simulation engine.  It is called by the
 db_do_timestep() function in db.c  The simulation engine is made up of
 four parts:
 -# pre-simulation statement queue
 -# statement simulation engine
 -# expression simulation engine
 -# post-simulation statement queue

 \par
 The operation of the simulation engine is as follows.  When a signal is found
 in the VCD file, the expressions to which it is a part of the RHS are looked up
 in the design tree.  The expression tree is then parsed from the expression to
 the root, setting the LEFT_SIDE_CHANGED or RIGHT_SIDE_CHANGED as it makes its
 way to the root.  When at the root expression, the STMT_HEAD bit is interrogated.
 If this bit is a 1, the expression's statement is loaded into the pre-simulation
 statement queue.  If the bit is a 0, no further action is taken.

 \par
 Once the timestep marker has been set, the simulate function is called.  All
 statements for the DUT are initially stored in the pre-simulation statement
 queue.  The statement located at the head of the queue is placed into the
 statement simulation engine and the head pointer is set to point to the next
 statement in the queue.  The head statement is continually taken until the
 pre-simulation statement queue is empty.  This signifies that the timestep has
 been completed.

 \par
 When a statement is placed into the statement simulation engine, the root
 expression pointed to by the statement is interrogated to see if the 
 LEFT_SIDE_CHANGED or RIGHT_SIDE_CHANGED bits are set.  If one or both of the
 bits are found to be set, the root expression is placed into the expression
 simulation engine for further processing.  When the statement's expression has
 completed its simulation, the value of the root expression is used to determine
 if the next_true or next_false path will be taken.  If the value of the root
 expression is true, the next_true statement is loaded into the statement simulation
 engine.  If the value of the root expression is false, the next_false statement is
 loaded into the statement simulation engine.  
*/
void simulate() {

}

/* $Log$ */
