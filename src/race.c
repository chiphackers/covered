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

