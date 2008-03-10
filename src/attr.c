/*
 Copyright (c) 2006 Trevor Williams

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
 \file     attr.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/25/2003

 \par What is an attribute?
 An attribute is a Verilog-2001 syntatical feature that allows meta-comment-like information to
 be stored in a Verilog file which can be used by any tool that is capable of implementing its functionality.
 It uses the syntax (* ... *) which may be placed before just about any Verilog construct within a design.
 Covered is able to parse attributes, implementing its functionality for all Covered-defined attribute commands.
 Currently covered can parse the following attribute commands:

 \par
 -# \b covered_fsm - Allows FSM-specific coverage information to be embedded within a design file
 -# \b covered_assert - Allows assertion coverage information to be embedded within a design file
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "assertion.h"
#include "attr.h"
#include "defines.h"
#include "expr.h"
#include "fsm_arg.h"
#include "func_unit.h"
#include "profiler.h"
#include "util.h"


/*!
 \param name  String identifier of this attribute parameter.
 \param expr  Pointer to the expression assigned to the parameter identifier.

 \return Returns a pointer to the newly allocated/initialized attribute parameter.

 Allocates and initializes an attribute parameter entry.
*/
attr_param* attribute_create( const char* name, expression* expr ) { PROFILE(ATTRIBUTE_CREATE);

  attr_param* ap;  /* Pointer to newly allocated attribute parameter */

  ap        = (attr_param*)malloc_safe( sizeof( attr_param ) );
  ap->name  = strdup_safe( name );
  ap->expr  = expr;
  ap->index = 0;
  ap->next  = NULL;
  ap->prev  = NULL;

  return( ap );

}

/*!
 \param ap       Pointer to current element of attribute parameter list to parse.
 \param funit    Pointer to current functional unit containing this attribute.
 \param exclude  If set to 1, sets the exclude bits (if they exist) in the structure created by the attribute.

 \throws anonymous fsm_arg_parse_attr attribute_parse

 Parses the attribute parameter list in a recursive fashion.  First,
 we go for the last entry and see if it refers to an attribute that covered
 should parse.  If this attribute is identified by Covered as one of its own, it
 calls the appropriate function to handle the entire attribute parameter list.
*/
void attribute_parse(
  attr_param*      ap,
  const func_unit* funit,
  bool             exclude
) { PROFILE(ATTRIBUTE_PARSE);

  if( ap != NULL ) {

    if( ap->next != NULL ) {
      attribute_parse( ap->next, funit, exclude );
    } else {
      if( strcmp( ap->name, "covered_fsm" ) == 0 ) {
        fsm_arg_parse_attr( ap->prev, funit, exclude );
      } else if( strcmp( ap->name, "covered_assert" ) == 0 ) {
        assertion_parse_attr( ap->prev, funit, exclude );
      }
    }

  }

}

/*!
 \param ap  Pointer to the attribute parameter list to remove.

 Deallocates all memory for the entire attribute parameter list.
*/
void attribute_dealloc( attr_param* ap ) { PROFILE(ATTRIBUTE_DEALLOC);

  if( ap != NULL ) {

    /* Deallocate the next attribute */
    attribute_dealloc( ap->next );

    /* Deallocate the name string */
    free_safe( ap->name );

    /* Deallocate the expression tree */
    expression_dealloc( ap->expr, FALSE );

    /* Finally, deallocate myself */
    free_safe( ap );

  }

}

/*
 $Log$
 Revision 1.11  2008/03/04 06:46:47  phase1geo
 More exception handling updates.  Still work to go.  Checkpointing.

 Revision 1.10  2008/02/01 06:37:07  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.9  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.8  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.7  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.6  2006/04/05 15:19:18  phase1geo
 Adding support for FSM coverage output in the GUI.  Started adding components
 for assertion coverage to GUI and report functions though there is no functional
 support for this at this time.

 Revision 1.5  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.4  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.3  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.2  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.1  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

*/

