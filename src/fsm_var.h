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
fsm_var* fsm_var_add( char* mod );

/*! \brief Adds specified signal information to FSM variable input list. */
void fsm_var_add_in_sig( fsm_var* fv, char* name, int width, int lsb );

/*! \brief Adds specified signal information to FSM variable output list. */
void fsm_var_add_out_sig( fsm_var* fv, char* name, int width, int lsb );

/*! \brief Finds specified variable in input FSM signal list. */
fsm_var* fsm_var_find_in_var( char* mod, char* var );

/*! \brief Finds specified variable in output FSM signal list. */
fsm_var* fsm_var_find_out_var( char* mod, char* var );

/*! \brief Outputs all FSM variables that were unused during parsing. */
void fsm_var_check_for_unused();

/*! \brief Removes specified FSM variable from global FSM variable list. */
void fsm_var_remove( fsm_var* fv );


/*
 $Log$
*/

#endif

