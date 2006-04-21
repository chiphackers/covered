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
 \file     vsignal.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/1/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "vsignal.h"
#include "expr.h"
#include "link.h"
#include "vector.h"
#include "func_unit.h"
#include "util.h"
#include "sim.h"


extern nibble or_optab[OPTAB_SIZE];
extern char   user_msg[USER_MSG_LENGTH];


/*!
 \param sig         Pointer to vsignal to initialize.
 \param name        Pointer to vsignal name string.
 \param type        Type of signal to create
 \param value       Pointer to vsignal value.
 \param lsb         Least-significant bit position in this string.
 \param line        Line number that this signal is declared on.
 \param col         Starting column that this signal is declared on.
 \param big_endian  Set to 1 if the MSB is less than the LSB.
 
 Initializes the specified vsignal with the values of name, value and lsb.  This
 function is called by the vsignal_create routine and is also useful for
 creating temporary vsignals (reduces the need for dynamic memory allocation).
 for performance enhancing purposes.
*/
void vsignal_init( vsignal* sig, char* name, int type, vector* value, int lsb, int line, int col, int big_endian ) {

  sig->name                  = name;
  sig->suppl.all             = 0;
  sig->suppl.part.type       = type;
  sig->suppl.part.col        = col;
  sig->suppl.part.big_endian = big_endian;
  sig->value                 = value;
  sig->lsb                   = lsb;
  sig->line                  = line;
  sig->exp_head              = NULL;
  sig->exp_tail              = NULL;

}

/*!
 \param name        Full hierarchical name of this vsignal.
 \param type        Type of signal to create
 \param width       Bit width of this vsignal.
 \param lsb         Bit position of least significant bit.
 \param line        Line number that this signal is declared on.
 \param col         Starting column that this signal is declared on.
 \param big_endian  Set to 1 if the MSB is less than the LSB.

 \returns Pointer to newly created vsignal.

 This function should be called by the Verilog parser or the
 database reading function.  It initializes all of the necessary
 values for a vsignal and returns a pointer to this newly created
 vsignal.
*/
vsignal* vsignal_create( char* name, int type, int width, int lsb, int line, int col, int big_endian ) {

  vsignal* new_sig;  /* Pointer to newly created vsignal */

  new_sig = (vsignal*)malloc_safe( sizeof( vsignal ), __FILE__, __LINE__ );

  vsignal_init( new_sig, ((name != NULL) ? strdup_safe( name, __FILE__, __LINE__ ) : NULL),
                type, vector_create( width, TRUE ), lsb, line, col, big_endian );

  return( new_sig );

}

/*!
 \param sig      Signal to write to file.
 \param file     Name of file to display vsignal contents to.

 Prints the vsignal information for the specified vsignal to the
 specified file.  This file will be the database coverage file
 for this design.
*/
void vsignal_db_write( vsignal* sig, FILE* file ) {

  exp_link* curr;  /* Pointer to current expression link element */

  /* Don't write this vsignal if it isn't usable by Covered */
  if( (sig->name[0] != '!') && (sig->value->width != -1) ) {

    /* Display identification and value information first */
    fprintf( file, "%d %s %d %d %x ",
      DB_TYPE_SIGNAL,
      sig->name,
      sig->lsb,
      sig->line,
      sig->suppl.all
    );

    vector_db_write( sig->value, file, (sig->name[0] == '#') );

    fprintf( file, "\n" );

  }

}

/*!
 \param line        Pointer to current line from database file to parse.
 \param curr_funit  Pointer to current functional unit instantiating this vsignal.

 \return Returns TRUE if vsignal information read successfully; otherwise,
         returns FALSE.

 Creates a new vsignal structure, parses current file line for vsignal
 information and stores it to the specified vsignal.  If there are any problems
 in reading in the current line, returns FALSE; otherwise, returns TRUE.
*/
bool vsignal_db_read( char** line, func_unit* curr_funit ) {

  bool       retval = TRUE;  /* Return value for this function */
  char       name[256];      /* Name of current vsignal */
  vsignal*   sig;            /* Pointer to the newly created vsignal */
  vector*    vec;            /* Vector value for this vsignal */
  int        lsb;            /* Least-significant bit of this vsignal */
  int        sline;          /* Declared line number */
  ssuppl     suppl;          /* Supplemental field */
  int        exp_id;         /* Expression ID */
  int        chars_read;     /* Number of characters read from line */
  expression texp;           /* Temporary expression link for searching purposes */
  exp_link*  expl;           /* Temporary expression link for storage */
  func_unit* parent_mod;     /* Pointer to parent module */

  /* Get name values. */
  if( sscanf( *line, "%s %d %d %x %n", name, &lsb, &sline, &(suppl.all), &chars_read ) == 4 ) {

    *line = *line + chars_read;

    /* Read in vector information */
    if( vector_db_read( &vec, line ) ) {

      /* Create new vsignal */
      sig = vsignal_create( name, suppl.part.type, vec->width, lsb, sline, suppl.part.col, suppl.part.big_endian );

      /* Copy over vector value */
      vector_dealloc( sig->value );
      sig->value = vec;

      /* Add vsignal to vsignal list */
      if( curr_funit == NULL ) {
        print_output( "Internal error:  vsignal in database written before its functional unit", FATAL, __FILE__, __LINE__ );
        retval = FALSE;
      } else {
        sig_link_add( sig, &(curr_funit->sig_head), &(curr_funit->sig_tail) );
      }

      /* Read in expressions */
      while( sscanf( *line, "%d%n", &exp_id, &chars_read ) == 1 ) {

        *line = *line + chars_read;

        /* Find expression in current (or last if funit is a function) functional unit and add it to vsignal list */
        texp.id = exp_id;
        expl    = NULL;

        if( (curr_funit->type == FUNIT_FUNCTION) && scope_compare( curr_funit->name, sig->name ) ) {
          parent_mod = funit_get_curr_module( curr_funit );
          expl       = exp_link_find( &texp, parent_mod->exp_head );
        }

        if( expl == NULL ) {
          expl = exp_link_find( &texp, curr_funit->exp_head );
        }

        if( expl != NULL ) {

          exp_link_add( expl->exp, &(sig->exp_head), &(sig->exp_tail) );
          
          expl->exp->sig = sig;
          
          /*
           If expression is a vsignal holder, we need to set the expression's vector to point
           to our vector and set its vsignal pointer to point to us.
          */
          if( (expl->exp->op == EXP_OP_SIG)            ||
              (expl->exp->op == EXP_OP_SBIT_SEL)       ||
              (expl->exp->op == EXP_OP_MBIT_SEL)       ||
              (expl->exp->op == EXP_OP_MBIT_POS)       ||
              (expl->exp->op == EXP_OP_MBIT_NEG)       ||
              (expl->exp->op == EXP_OP_PARAM)          ||
              (expl->exp->op == EXP_OP_PARAM_SBIT)     ||
              (expl->exp->op == EXP_OP_PARAM_MBIT)     ||
              (expl->exp->op == EXP_OP_PARAM_MBIT_POS) ||
              (expl->exp->op == EXP_OP_PARAM_MBIT_NEG) ||
              (expl->exp->op == EXP_OP_FUNC_CALL) ) {
            expression_set_value( expl->exp, sig->value );
          }

        } else {

          if( name[0] != '#' ) {
            snprintf( user_msg, USER_MSG_LENGTH, "Expression %d not found for vsignal %s", texp.id, sig->name );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            retval = FALSE;
            exit( 1 );
          }

        }

      }

    } else {

      retval = FALSE;

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Signal to store result of merge into.
 \param line  Pointer to line of CDD file to parse.
 \param same  Specifies if vsignal to merge needs to be exactly the same as the existing vsignal.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Parses specified line for vsignal information and performs merge 
 of the base and in vsignals, placing the resulting merged vsignal 
 into the base vsignal.  If the vsignals are found to be unalike 
 (names are different), an error message is displayed to the user.  
 If both vsignals are the same, perform the merge on the vsignal's 
 vectors.
*/
bool vsignal_db_merge( vsignal* base, char** line, bool same ) {
 
  bool    retval;      /* Return value of this function */
  char    name[256];   /* Name of current vsignal */
  int     lsb;         /* Least-significant bit of this vsignal */
  int     sline;       /* Declared line number */
  ssuppl  suppl;       /* Supplemental signal information */
  int     chars_read;  /* Number of characters read from line */

  assert( base != NULL );
  assert( base->name != NULL );

  if( sscanf( *line, "%s %d %d %x %n", name, &lsb, &sline, &(suppl.all), &chars_read ) == 4 ) {

    *line = *line + chars_read;

    if( !scope_compare( base->name, name ) || (base->lsb != lsb) ) {

      print_output( "Attempting to merge two databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      /* Read in vector information */
      retval = vector_db_merge( base->value, line, same );

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Signal to store replaced data.
 \param line  Pointer to line of CDD file to parse.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Parses specified line for vsignal information and performs a replacement
 of the base with the new vsignal.  If the vsignals are found to be unalike
 (names are different), an error message is displayed to the user.
 If both vsignals are the same, perform the replacement.
*/
bool vsignal_db_replace( vsignal* base, char** line ) {

  bool   retval;      /* Return value of this function */
  char   name[256];   /* Name of current vsignal */
  int    lsb;         /* Least-significant bit of this vsignal */
  int    sline;       /* Declared line number */
  ssuppl suppl;       /* Supplemental signal information */
  int    chars_read;  /* Number of characters read from line */

  assert( base != NULL );
  assert( base->name != NULL );

  if( sscanf( *line, "%s %d %d %x %n", name, &lsb, &sline, &(suppl.all), &chars_read ) == 4 ) {

    *line = *line + chars_read;

    if( !scope_compare( base->name, name ) || (base->lsb != lsb) ) {

      print_output( "Attempting to replace a database derived from a different design.  Unable to replace",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      /* Read in vector information */
      retval = vector_db_replace( base->value, line );

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param sig  Pointer to signal to set assigned bits to
 \param msb  Most-significant bit to set in signal vector
 \param lsb  Least-significant bit to set in signal vector

 \return Returns TRUE if any of the set bits were previously set by earlier calls to this 
         function for this signal.

 Sets all assigned bits in the signal vector value bit array for the range specified.  If
 any of these set bits were previously set, return TRUE to the calling function to indicate
 that this occurred.  This function is used by the race condition checker.
*/
bool vsignal_set_assigned( vsignal* sig, int msb, int lsb ) {

  return( vector_set_assigned( sig->value, (msb - sig->lsb), (lsb - sig->lsb) ) );

}

/*!
 \param sig  Pointer to signal to propagate change information from.

  When the specified signal in the parameter list has changed values, this function
  is called to propagate the value change to the simulator to cause any statements
  waiting on this value change to be resimulated.
*/
void vsignal_propagate( vsignal* sig ) {

  exp_link* curr_expr;  /* Pointer to current expression in signal list */

  /* Iterate through vsignal's expression list */
  curr_expr = sig->exp_head;
  while( curr_expr != NULL ) {

    /* Add to simulation queue if expression is a RHS, not a function call and not a port assignment */
    if( (ESUPPL_IS_LHS( curr_expr->exp->suppl ) == 0) &&
        (curr_expr->exp->op != EXP_OP_FUNC_CALL) &&
        (curr_expr->exp->op != EXP_OP_PASSIGN) ) {
      sim_expr_changed( curr_expr->exp );
    }

    curr_expr = curr_expr->next;

  }

}

/*!
 \param sig    Pointer to vsignal to assign VCD value to.
 \param value  String version of VCD value.
 \param msb    Most significant bit to assign to.
 \param lsb    Least significant bit to assign to.

 Assigns the associated value to the specified vsignal's vector.  After this, it
 iterates through its expression list, setting the TRUE and FALSE bits accordingly.
 Finally, calls the simulator expr_changed function for each expression.
*/
void vsignal_vcd_assign( vsignal* sig, char* value, int msb, int lsb ) {

  bool      vec_changed;  /* Specifies if assigned value differed from original value */
  exp_link* curr_expr;    /* Pointer to current expression link under evaluation */

  assert( sig->value != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "Assigning vsignal %s[%d:%d] (lsb=%d) to value %s", sig->name, msb, lsb, sig->lsb, value );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Set vsignal value to specified value */
  if( lsb > 0 ) {
    vec_changed = vector_vcd_assign( sig->value, value, (msb - sig->lsb), (lsb - sig->lsb) );
  } else {
    vec_changed = vector_vcd_assign( sig->value, value, msb, lsb );
  }

  /* Don't go through the hassle of updating expressions if value hasn't changed */
  if( vec_changed ) {

    /* Propagate signal changes to rest of design */
    vsignal_propagate( sig );

  } 

}

/*!
 \param sig   Pointer to vsignal to add expression to.
 \param expr  Expression to add to list.

 Adds the specified expression to the end of this vsignal's expression
 list.
*/
void vsignal_add_expression( vsignal* sig, expression* expr ) {

  exp_link_add( expr, &(sig->exp_head), &(sig->exp_tail) );

}

/*!
 \param sig  Pointer to vsignal to display to standard output.

 Displays vsignal's name, width, lsb and value vector to the standard output.
*/
void vsignal_display( vsignal* sig ) {

  assert( sig != NULL );

  printf( "  Signal =>  name: %s, lsb: %d, ", sig->name, sig->lsb );
  vector_display_value( sig->value->value, sig->value->width );
  printf( "\n" );

}

/*!
 \param str  String version of vsignal.

 \return Returns pointer to newly created vsignal structure, or returns
         NULL is specified string does not properly describe a vsignal.

 Converts the specified string describing a Verilog design vsignal.  The
 vsignal may be a standard vsignal name, a single bit select vsignal or a
 multi-bit select vsignal.
*/
vsignal* vsignal_from_string( char** str ) {

  vsignal* sig;         /* Pointer to newly created vsignal */
  char     name[4096];  /* Signal name */
  int      left;        /* Left selection value of the signal */
  int      right;       /* Right selection value of the signal */
  int      width;       /* Width of the signal */
  int      lsb;         /* LSB of the signal */
  int      big_endian;  /* Endianness of the signal */
  int      chars_read;  /* Number of characters read from string */

  if( sscanf( *str, "%[a-zA-Z0-9_]\[%d:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    if( right > left ) {
      width      = (right - left) + 1;
      lsb        = left;
      big_endian = 1;
    } else {
      width      = (left - right) + 1;
      lsb        = right;
      big_endian = 0;
    }
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, width, lsb, 0, 0, big_endian );
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d+:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT_POS, right, left, 0, 0, 0 );
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d-:%d]%n", name, &left, &right, &chars_read ) == 3 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT_NEG, right, ((left - right) + 1), 0, 0, 0 );
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d]%n", name, &right, &chars_read ) == 2 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, right, 0, 0, 0 );
    *str += chars_read;
  } else if( sscanf( *str, "%[a-zA-Z0-9_]%n", name, &chars_read ) == 1 ) {
    sig = vsignal_create( name, SSUPPL_TYPE_IMPLICIT, 1, 0, 0, 0, 0 );
    /* Specify that this width is unknown */
    sig->value->width = 0;
    *str += chars_read;
  } else {
    sig = NULL;
  }

  return( sig );

}

/*!
 \param sig  Pointer to vsignal to deallocate.

 Deallocates all malloc'ed memory back to the heap for the specified
 vsignal.
*/
void vsignal_dealloc( vsignal* sig ) {

  exp_link* curr_expl;  /* Pointer to current expression link to set to NULL */

  if( sig != NULL ) {

    if( sig->name != NULL ) {
      free_safe( sig->name );
      sig->name = NULL;
    }

    /* Free up memory for value */
    vector_dealloc( sig->value );
    sig->value = NULL;

    /* Free up memory for expression list */
    curr_expl = sig->exp_head;
    while( curr_expl != NULL ) {
      curr_expl->exp->sig = NULL;
      curr_expl = curr_expl->next;
    }

    exp_link_delete_list( sig->exp_head, FALSE );
    sig->exp_head = NULL;

    /* Finally free up the memory for this vsignal */
    free_safe( sig );

  }

}

/*
 $Log$
 Revision 1.22.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.22  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.21  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.20  2006/02/01 15:13:11  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.19  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.18  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.17  2006/01/23 03:53:30  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.16  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.15  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.14  2005/12/01 18:35:17  phase1geo
 Fixing bug where functions in continuous assignments could cause the
 assignment to constantly be reevaluated (infinite looping).  Added new nested_block2
 diagnostic to verify nested named blocks in functions.  Also verifies that nested
 named blocks can call functions in the same module.  Also modified NB_CALL expressions
 to act like functions (no context switching involved).  Full regression passes.

 Revision 1.13  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.12  2005/11/21 22:21:58  phase1geo
 More regression updates.  Also made some updates to debugging output.

 Revision 1.11  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.10  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.9  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.8  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.7  2005/02/16 13:45:04  phase1geo
 Adding value propagation function to vsignal.c and adding this propagation to
 BASSIGN expression assignment after the assignment occurs.

 Revision 1.6  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.5  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.4  2004/11/07 05:51:24  phase1geo
 If assigned signal value did not change, do not cause associated expression tree(s)
 to be re-evaluated.

 Revision 1.3  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.2  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.1  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.48  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.47  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.46  2003/11/29 06:55:49  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.45  2003/11/12 17:34:03  phase1geo
 Fixing bug where signals are longer than allowable bit width.

 Revision 1.44  2003/11/05 05:22:56  phase1geo
 Final fix for bug 835366.  Full regression passes once again.

 Revision 1.43  2003/10/20 16:05:06  phase1geo
 Fixing bug for older versions of Icarus that does not output the correct
 bit select in the VCD dumpfile.  Covered will automatically adjust the bit-select
 range if this occurrence is found in the dumpfile.

 Revision 1.42  2003/10/19 04:23:49  phase1geo
 Fixing bug in VCD parser for new Icarus Verilog VCD dumpfile formatting.
 Fixing bug in signal.c for signal merging.  Updates all CDD files to match
 new format.  Added new diagnostics to test advanced FSM state variable
 features.

 Revision 1.41  2003/10/17 21:59:07  phase1geo
 Fixing signal_vcd_assign function to properly adjust msb and lsb based on
 the lsb of the signal that is being assigned to.

 Revision 1.40  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.39  2003/10/16 04:26:01  phase1geo
 Adding new fsm5 diagnostic to testsuite and regression.  Added proper support
 for FSM variables that are not able to be bound correctly.  Fixing bug in
 signal_from_string function.

 Revision 1.38  2003/10/13 22:10:07  phase1geo
 More changes for FSM support.  Still not quite there.

 Revision 1.37  2003/10/11 05:15:08  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.36  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.35  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.

 Revision 1.34  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.33  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.32  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.31  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.30  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.29  2003/02/26 23:00:50  phase1geo
 Fixing bug with single-bit parameter handling (param4.v diagnostic miscompare
 between Linux and Irix OS's).  Updates to testsuite and new diagnostic added
 for additional testing in this area.

 Revision 1.28  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.27  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.26  2002/12/29 06:09:32  phase1geo
 Fixing bug where output was not squelched in report command when -Q option
 is specified.  Fixed bug in preprocessor where spaces where added in when newlines
 found in C-style comment blocks.  Modified regression run to check CDD file and
 generated module and instance reports.  Started to add code to handle signals that
 are specified in design but unused in Covered.

 Revision 1.25  2002/11/05 00:20:08  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.24  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.23  2002/10/31 23:14:23  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.22  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.21  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.20  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.19  2002/10/11 04:24:02  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.18  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.17  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.16  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.15  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.14  2002/08/14 04:52:48  phase1geo
 Removing unnecessary calls to signal_dealloc function and fixing bug
 with signal_dealloc function.

 Revision 1.13  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.12  2002/07/19 13:10:07  phase1geo
 Various fixes to binding scheme.

 Revision 1.11  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.10  2002/07/18 02:33:24  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.9  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.8  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.7  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.6  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

