/*!
 \file     param.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     8/22/2002
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "param.h"
#include "util.h"
#include "link.h"
#include "signal.h"


sig_link* param_head = NULL;
sig_link* param_tail = NULL;


/*!
 \param scope  Full hierarchical reference to specified scope to change value to.
 \param value  Vector value to set parameter to.

 Scans list of all parameters to make sure that specified parameter isn't already
 being set to a new value.  If no match occurs, creates a new parameter link and
 places at the tail of the parameter list.  If match is found, display error
 message to user and exit covered immediately.  This function is called for each
 -P option to the score command and in the defparam parser code.
*/
void param_add_defparam( char* scope, vector* value ) {

  signal    tmp_parm;       /* Temporary parameter holder       */
  sig_link* parm;           /* Holds newly created parameter    */
  signal*   parm_sig;       /* Parameter signal structure       */
  char      err_msg[4096];  /* Error message to display to user */

  /* Setup the temporary parameter */
  tmp_parm.name = scope;

  if( sig_link_find( &tmp_parm, param_head ) == NULL ) {

    assert( value != NULL );
    parm_sig = signal_create( scope, value->width, value->lsb, FALSE );
    sig_link_add( parm_sig, &param_head, &param_tail );

  } else {

    snprintf( err_msg, 4096, "Parameter (%s) value is assigned more than once", scope );
    print_output( err_msg, FATAL );
    exit( 1 );

  }

}


/* $Log$ */

