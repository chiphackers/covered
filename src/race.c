/*!
 \file     race.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/15/2004

 \par
 The contents of this file contain functions to handle race condition checking and information
 handling regarding race conditions.  Since race conditions can cause the Covered simulator to not
 provide the same run-time order as the primary Verilog simulator, we need to be able to detect when
 race conditions are occurring within the design.  The following conditions are checked within the design
 during the parsing stage.

 \par
 -# All sequential logic uses non-blocking assignments.
 -# All latches use non-blocking assignments.
 -# All combinational logic in an always block uses blocking assignments.
 -# All mixed sequential and combinational logic in the same always block uses non-blocking assignments.
 -# Blocking and non-blocking assignments should not be used in the same always block.
 -# Assignments made to a variable should only be done within one always block.
 -# The $strobe system call should only be used to display variables that were assigned using non-blocking
    assignments.
 -# No #0 procedural assignments should exist.

 \par
 The failure of any one of these rules will cause Covered to either display warning type messages to the user when
 the race condition checking flag has not been set or error messages when the race condition checking flag has
 been set by the user.
*/


#include "defines.h"
#include "race.h"


extern bool race_flag = false;


/*!
 \param stmt  Pointer to current statement to find signals for.
 \param root  Pointer to root statement in statement tree.

 Finds all signals within the given statement tree that are assigned within this tree.
 When this list of signals is found, new stmt_sig links are added to the stmt_sig linked
 list, setting the signal and top-level statement values into these links.  If a signal
 is found within this statement tree that is also in the current stmt_sig list and the
 top-level statements are found to be different, an error message is displayed to the user
 (depending on the race condition flag set in the score command).
*/
void race_find_and_add_stmt_sigs( statement* stmt, statement* root ) {

}


/*
 $Log$
*/

