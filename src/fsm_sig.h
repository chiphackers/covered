#ifndef __FSM_SIG_H__
#define __FSM_SIG_H__

/*!
 \file     fsm_sig.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/3/2003
 \brief    Contains functions for handling FSM signal structure.
*/

#include "defines.h"


/*! \brief Allocates, initializes and adds new FSM signal to specified list. */
void fsm_sig_add( fsm_sig** fs_head, fsm_sig** fs_tail, char* name, int width, int lsb );

/*! \brief Finds specified signal name in specified FSM signal list. */
fsm_sig* fsm_sig_find( fsm_sig* fs_head, char* name );

/*! \brief Deallocates all memory associated with the specified FSM signal. */
void fsm_sig_dealloc( fsm_sig* fs );

/*! \brief Deallocates entire FSM signal list associated with the specified argument. */
void fsm_sig_delete_list( fsm_sig* fs );


/*
 $Log$
*/

#endif

