#ifndef __FSM_VAR_H__
#define __FSM_VAR_H__

/*!
 \file     fsm_var.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/3/2003
 \brief    Contains functions for handling FSM variable structure.
*/

#include "defines.h"


/*! \brief Allocates, initializes and adds FSM variable to global list. */
fsm_var* fsm_var_add( char* mod, expression* in_state, expression* out_state );

/*! \brief Adds specified signal and expression to binding list. */
void fsm_var_bind_add( char* sig_name, expression* expr, char* mod_name );

/*! \brief Add specified module and statement to binding list. */
void fsm_var_stmt_add( statement* stmt, char* mod_name );

/*! \brief Performs FSM signal/expression binding process. */
void fsm_var_bind( mod_link* mod_head );

/*! \brief Removes specified FSM variable from global FSM variable list. */
void fsm_var_remove( fsm_var* fv );


/*
 $Log$
 Revision 1.1  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

*/

#endif

