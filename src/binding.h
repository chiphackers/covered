#ifndef __BINDING_H__
#define __BINDING_H__

/*!
 \file     binding.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/4/2002
 \brief    Contains all functions for vsignal/expression binding.
*/

#include "defines.h"


/*! \brief Adds vsignal and expression to binding list. */
void bind_add( const char* sig_name, expression* exp, module* mod );

/*! \brief Removes the expression with ID of id from binding list. */
void bind_remove( int id );

/*! \brief Finds vsignal in module and bind the expression to this vsignal. */
bool bind_perform( char* sig_name, expression* exp, module* mod_sig, module* mod_exp, bool implicit_allowed, bool fsm_bind );

/*! \brief Performs vsignal/expression bind (performed after parse completed). */
void bind();


/* 
 $Log$
 Revision 1.11  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.10  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.9  2003/01/05 22:25:23  phase1geo
 Fixing bug with declared integers, time, real, realtime and memory types where
 they are confused with implicitly declared signals and given 1-bit value types.
 Updating regression for changes.

 Revision 1.8  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.7  2002/10/31 23:13:19  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.6  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.5  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.4  2002/07/20 22:22:52  phase1geo
 Added ability to create implicit signals for local signals.  Added implicit1.v
 diagnostic to test for correctness.  Full regression passes.  Other tweaks to
 output information.

 Revision 1.3  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.
*/

#endif

