#ifndef __FSM_H__
#define __FSM_H__

/*!
 \file     fsm.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting FSM coverage.
*/

#include <stdio.h>

#include "defines.h"

/*! \brief Adds specified module and variable to the FSM variable list. */
void fsm_add_fsm_variable( char* mod, char* var );

/*! \brief Returns TRUE if specified module and variable are a user-specified FSM */
bool fsm_is_fsm_variable( char* mod, char* var );

/*! \brief Creates and initializes new FSM structure. */
fsm* fsm_create( signal* sig );

/*! \brief Adds new FSM arc structure to specified FSMs arc list. */
void fsm_add_arc( fsm* table, expression* from_state, expression* to_state );

/*! \brief Sets sizes of tables in specified FSM structure. */
void fsm_create_tables( fsm* table );

/*! \brief Outputs contents of specified FSM to CDD file. */
bool fsm_db_write( fsm* table, FILE* file );

/*! \brief Reads in contents of specified FSM. */
bool fsm_db_read( char** line, module* mod );

/*! \brief Reads and merges two FSMs, placing result into base FSM. */
bool fsm_db_merge( fsm* base, char** line, bool same );

/*! \brief Sets the bit in set table based on the values of last and curr. */
void fsm_table_set( fsm* table, vector* last, vector* curr );

/*! \brief Gathers statistics about the current FSM */
void fsm_get_stats( fsm_link* table, float* state_total, int* state_hit, float* arc_total, int* arc_hit );

/*! \brief Generates report output for FSM coverage. */
void fsm_report( FILE* ofile, bool verbose );

/*! \brief Deallocates specified FSM structure. */
void fsm_dealloc( fsm* table );

/*
 $Log$
 Revision 1.6  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:13:50  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

