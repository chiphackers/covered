#ifndef __FSM_ARG_H___
#define __FSM_ARG_H__

/*!
 \file     fsm_arg.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/02/2003
 \brief    Contains functions for handling FSM arguments from the command-line.
*/

#include "defines.h"


/*! \brief Parses specified -F argument for FSM information. */
bool fsm_arg_parse( char* arg );

/*! \brief Parses specified attribute argument for FSM information. */
void fsm_arg_parse_attr( attr_param* ap, func_unit* funit );


/*
 $Log$
 Revision 1.2  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.1  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.

*/

#endif

