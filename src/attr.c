/*
 Copyright (c) 2006-2010 Trevor Williams

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
 \return Returns a pointer to the newly allocated/initialized attribute parameter.

 Allocates and initializes an attribute parameter entry.
*/
attr_param* attribute_create(
  const char* name,  /*!< String identifier of this attribute parameter */
  expression* expr   /*!< Pointer to the expression assigned to the parameter identifier */
) { PROFILE(ATTRIBUTE_CREATE);

  attr_param* ap;  /* Pointer to newly allocated attribute parameter */

  ap        = (attr_param*)malloc_safe( sizeof( attr_param ) );
  ap->name  = strdup_safe( name );
  ap->expr  = expr;
  ap->index = 0;
  ap->next  = NULL;
  ap->prev  = NULL;

  PROFILE_END;

  return( ap );

}

/*!
 \throws anonymous fsm_arg_parse_attr attribute_parse

 Parses the attribute parameter list in a recursive fashion.  First,
 we go for the last entry and see if it refers to an attribute that covered
 should parse.  If this attribute is identified by Covered as one of its own, it
 calls the appropriate function to handle the entire attribute parameter list.
*/
void attribute_parse(
  attr_param*      ap,      /*!< Pointer to current element of attribute parameter list to parse */
  const func_unit* funit,   /*!< Pointer to current functional unit containing this attribute */
  bool             exclude  /*!< If set to 1, sets the exclude bits (if they exist) in the structure created by the attribute */
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

  PROFILE_END;

}

/*!
 Deallocates all memory for the entire attribute parameter list.
*/
void attribute_dealloc(
  attr_param* ap  /*!< Pointer to the attribute parameter list to remove */
) { PROFILE(ATTRIBUTE_DEALLOC);

  if( ap != NULL ) {

    /* Deallocate the next attribute */
    attribute_dealloc( ap->next );

    /* Deallocate the name string */
    free_safe( ap->name, (strlen( ap->name ) + 1) );

    /* Deallocate the expression tree */
    expression_dealloc( ap->expr, FALSE );

    /* Finally, deallocate myself */
    free_safe( ap, sizeof( attr_param ) );

  }

  PROFILE_END;

}

