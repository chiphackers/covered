/*
 Copyright (c) 2006-2009 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     enumerate.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     8/29/2006
*/

#include <stdlib.h>

#include "defines.h"
#include "enumerate.h"
#include "obfuscate.h"
#include "param.h"
#include "static.h"
#include "util.h"
#include "vector.h"


extern char user_msg[USER_MSG_LENGTH];


/*!
 Allocates, initializes and adds a new enumerated item to the given functional unit.
*/
void enumerate_add_item(
  vsignal*     enum_sig,  /*!< Pointer to signal in database that represents an enumeration value */
  static_expr* value,     /*!< Pointer to static expression that contains the value to be assigned to the signal during elaboration */
  func_unit*   funit      /*!< Pointer to functional unit that this enumeration value will be added to */
) { PROFILE(ENUMERATE_ADD_ITEM);

  enum_item* ei;  /* Pointer to newly allocated enumeration item */

  /* Allocate and initialize the enumeration item */
  ei = (enum_item*)malloc_safe( sizeof( enum_item ) );
  ei->sig   = enum_sig;
  ei->value = value;
  ei->last  = FALSE;
  ei->next  = NULL;

  /* Add it to the current functional unit's enumeration list */
  if( funit->ei_head == NULL ) {
    funit->ei_head = funit->ei_tail = ei;
  } else {
    funit->ei_tail->next = ei;
    funit->ei_tail       = ei;
  }

  PROFILE_END;

}

/*!
 Called after all enumerations have been parsed for this list.
*/
void enumerate_end_list(
  func_unit* funit  /*!< Pointer to functional unit to close out enumerated list */
) { PROFILE(ENUMERATE_END_LIST);

  /* Make sure that we aren't calling this function when there is no existing enumerated list */
  assert( funit->ei_tail != NULL );

  /* Set the last bit of the tail enumerated item */
  funit->ei_tail->last = TRUE;

  PROFILE_END;

}

/*!
 \throws anonymous Throw param_expr_eval

 Resolves all enumerated values for their value for the given instance.  This needs
 to be called during elaboration after all signals have been sized and parameters have
 been resolved.
*/
void enumerate_resolve(
  funit_inst* inst  /*!< Pointer to functional unit instance to resolve all enumerated values */
) { PROFILE(ENUMERATE_RESOLVE);

  enum_item* ei;                 /* Pointer to current enumeration item in the given functional unit */
  int        last_value = 0;     /* Value of last value for this enumeration */
  bool       first      = TRUE;  /* Specifies if the current enumeration is the first */
  bool       is_signed;          /* Contains original value of signedness of signal */

  assert( inst != NULL );

  ei = inst->funit->ei_head;
  while( ei != NULL ) {

    assert( ei->sig->value != NULL );

    /* Store signedness */
    is_signed = ei->sig->value->suppl.part.is_signed;

    /* If no value was assigned, we need to assign one now */
    if( ei->value == NULL ) {

      if( first ) {
        (void)vector_from_int( ei->sig->value, 0 );
      } else if( last_value == -1 ) {
        unsigned int rv;
        print_output( "Implicit enumerate assignment cannot follow an X or Z value", FATAL, __FILE__, __LINE__ );
        rv = snprintf( user_msg, USER_MSG_LENGTH, "File: %s, Line: %d", obf_file( inst->funit->filename ), ei->sig->line );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL_WRAP, __FILE__, __LINE__ );
        Throw 0;
      } else {
        (void)vector_from_int( ei->sig->value, (last_value + 1) );
      }

    /* Otherwise, reduce the static_expr value to a number and assign it */
    } else {

      if( ei->value->exp == NULL ) {
        (void)vector_from_int( ei->sig->value, ei->value->num );
      } else {
        param_expr_eval( ei->value->exp, inst );
        (void)vector_set_value_ulong( ei->sig->value, ei->value->exp->value->value.ul, ei->sig->value->width );
      }

    }

    /* Put back the original signedness */
    ei->sig->value->suppl.part.is_signed = is_signed;
        
    /* Set the first value to indicate if the next value is the first in the list */
    first = ei->last;

    /* Set last_value to that of this this signal value */
    if( !vector_is_unknown( ei->sig->value ) ) {
      last_value = vector_to_int( ei->sig->value );
    } else {
      last_value = -1;
    }

    ei = ei->next;

  }

  PROFILE_END;

}

/*!
 Deallocates all memory associated with the given enumeration.
*/
void enumerate_dealloc(
  enum_item* ei  /*!< Pointer to enumeration to deallocate */
) { PROFILE(ENUMERATE_DEALLOC);

  if( ei != NULL ) {

    /* Deallocate static expression, if necessary */
    if( ei->value != NULL ) {
      static_expr_dealloc( ei->value, TRUE );
    }

    /* Deallocate ourself */
    free_safe( ei, sizeof( enum_item ) );

  }

  PROFILE_END;

}

/*!
 Deallocates all memory associated with the enumeration list in the given functional unit
*/
void enumerate_dealloc_list(
  func_unit* funit  /*!< Pointer to functional unit to remove enumeration list for */
) { PROFILE(ENUMERATE_DEALLOC_LIST);

  enum_item* tmp;  /* Temporary pointer to current link in list */

  while( funit->ei_head != NULL ) {
    tmp  = funit->ei_head;
    funit->ei_head = tmp->next;
    enumerate_dealloc( tmp );
  }

  /* Set the tail pointer to NULL as well */
  funit->ei_tail = NULL;

  PROFILE_END;

}


/*
 $Log$
 Revision 1.23  2008/10/21 05:38:41  phase1geo
 More updates to support real values.  Added vector_from_real64 functionality.
 Checkpointing.

 Revision 1.22  2008/10/20 23:20:02  phase1geo
 Adding support for vector_from_int coverage accumulation (untested at this point).
 Updating Cver regressions.  Checkpointing.

 Revision 1.21  2008/08/18 23:07:26  phase1geo
 Integrating changes from development release branch to main development trunk.
 Regression passes.  Still need to update documentation directories and verify
 that the GUI stuff works properly.

 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.19  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.18  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.17.2.3  2008/05/28 05:57:10  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.17.2.2  2008/04/23 05:20:44  phase1geo
 Completed initial pass of code updates.  I can now begin testing...  Checkpointing.

 Revision 1.17.2.1  2008/04/21 23:13:04  phase1geo
 More work to update other files per vector changes.  Currently in the middle
 of updating expr.c.  Checkpointing.

 Revision 1.17  2008/04/04 21:23:29  phase1geo
 Last set of fixes to get IV regression to fully pass!

 Revision 1.16  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.15  2008/03/14 22:00:18  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.14  2008/03/09 20:45:47  phase1geo
 More exception handling updates.

 Revision 1.13  2008/03/04 00:09:20  phase1geo
 More exception handling.  Checkpointing.

 Revision 1.12  2008/01/15 23:01:14  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.11  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.10  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.9  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.8  2007/09/14 06:22:12  phase1geo
 Filling in existing functions in struct_union.  Completed parser code for handling
 struct/union declarations.  Code compiles thus far.

 Revision 1.7  2006/10/16 21:34:46  phase1geo
 Increased max bit width from 1024 to 65536 to allow for more room for memories.
 Fixed issue with enumerated values being explicitly assigned unknown values and
 created error output message when an implicitly assigned enum followed an explicitly
 assign unknown enum value.  Fixed problem with generate blocks in different
 instantiations of the same module.  Fixed bug in parser related to setting the
 curr_packed global variable correctly.  Added enum2 and enum2.1 diagnostics to test
 suite to verify correct enumerate behavior for the changes made in this checkin.
 Full regression now passes.

 Revision 1.6  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.5  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.4  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.3  2006/09/21 22:44:20  phase1geo
 More updates to regressions for latest changes to support memories/multi-dimensional
 arrays.  We still have a handful of VCS diagnostics that are failing.  Checkpointing
 for now.

 Revision 1.2  2006/08/30 12:02:48  phase1geo
 Changing assertion in vcd.c that fails when the VCD file is improperly formatted
 to a user error message with a bit more meaning.  Fixing problem with signedness
 of enumeration resolution.  Added enum1.1 diagnostic to testsuite.

 Revision 1.1  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

*/

