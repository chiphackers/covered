/*!
 \file     fsm_arg.c
 \author   Trevor Williams  (trevorw@sgi.com)
 \date     10/02/2003
*/

#include "defines.h"
#include "fsm_arg.h"
#include "util.h"
#include "fsm.h"
#include "fsm_var.h"
#include "fsm_sig.h"


/*!
 \param arg  Command-line argument following -F specifier.

 \return Returns TRUE if argument is considered legal for the -F specifier;
         otherwise, returns FALSE.

 Parses specified argument string for FSM information.  If the FSM information
 is considered legal, returns TRUE; otherwise, returns FALSE.
*/
bool fsm_arg_parse( char* arg ) {

  bool     retval = TRUE;  /* Return value for this function        */
  char*    ptr1   = arg;   /* Pointer to current character in arg   */
  char*    ptr2;           /* Pointer to current character in arc   */
  fsm_var* fv;             /* Pointer to newly created FSM variable */

  while( (*ptr1 != '\0') && (*ptr1 != '=') ) {
    ptr1++;
  }

  if( *ptr1 == '\0' ) {

    print_output( "Option -F must specify a module and one or two variables.  See \"covered score -h\" for more information.", FATAL );
    retval = FALSE;

  } else {

    *ptr1 = '\0';

    /* Add new FSM variable to list */
    fv = fsm_var_add( arg );

    ptr1++;
    ptr2 = ptr1;

    while( (*ptr2 != '\0') && (*ptr2 != ',') ) {
      ptr2++;
    }

    if( *ptr2 == '\0' ) {
      fsm_var_add_in_sig(  fv, ptr1, -1, 0 );
      fsm_var_add_out_sig( fv, ptr1, -1, 0 );
    } else {
      *ptr2 = '\0';
      fsm_var_add_in_sig(  fv, ptr1, -1, 0 );
      ptr2++;
      fsm_var_add_out_sig( fv, ptr2, -1, 0 );
    }

  }

  return( retval );

}

/*
 $Log$
 Revision 1.1  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.
*/

