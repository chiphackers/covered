#ifndef __DB_H__
#define __DB_H__

/*!
 \file     db.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
 \brief    Contains functions for writing and reading contents of
           covered database file.
*/

#include "defines.h"

/*! \brief Writes contents of expressions, modules and signals to database file. */
bool db_write( char* file, bool parse_mode );

/*! \brief Reads contents of database file and stores into internal lists. */
bool db_read( char* file, int read_mode );

/*! \brief Adds specified module node to module tree.  Called by parser. */
void db_add_instance( char* scope, char* modname );

/*! \brief Adds specified module to module list.  Called by parser. */
void db_add_module( char* name, char* file, int start_line );

/*! \brief Adds specified declared parameter to parameter list.  Called by parser. */
void db_add_declared_param( char* name, expression* expr );

/*! \brief Adds specified override parameter to parameter list.  Called by parser. */
void db_add_override_param( char* inst_name, expression* expr );

/*! \brief Adds signal/expression vector parameter to parameter list. */
void db_add_vector_param( signal* sig, expression* parm_exp, int type );

/*! \brief Adds specified defparam to parameter override list.  Called by parser. */
void db_add_defparam( char* name, expression* expr );

/*! \brief Adds specified signal to signal list.  Called by parser. */
void db_add_signal( char* name, static_expr* left, static_expr* right );

/*! \brief Called when the endmodule keyword is parsed. */
void db_end_module( int end_line );

/*! \brief Finds specified signal in module and returns pointer to the signal structure.  Called by parser. */
signal* db_find_signal( char* name );

/*! \brief Creates new expression from specified information.  Called by parser and db_add_expression. */
expression* db_create_expression( expression* right, expression* left, int op, bool lhs, int line, char* sig_name );

/*! \brief Adds specified expression to expression list.  Called by parser. */
void db_add_expression( expression* root );

/*! \brief Creates new statement expression from specified information.  Called by parser. */
statement* db_create_statement( expression* exp );

/*! \brief Adds specified statement to current module's statement list.  Called by parser. */
void db_add_statement( statement* stmt, statement* start );

/*! \brief Removes specified statement from current module. */
void db_remove_statement_from_current_module( statement* stmt );

/*! \brief Removes specified statement and associated expression from list and memory. */
void db_remove_statement( statement* stmt );

/*! \brief Connects one statement block to another. */
void db_statement_connect( statement* curr_stmt, statement* next_stmt );

/*! \brief Sets STMT_STOP bit in the appropriate statements. */
void db_statement_set_stop( statement* stmt, statement* post, bool both );

/*! \brief Connects true statement to specified statement. */
void db_connect_statement_true( statement* stmt, statement* exp_true );

/*! \brief Connects false statement to specified statement. */
void db_connect_statement_false( statement* stmt, statement* exp_false );

/*! \brief Allocates and initializes an attribute parameter. */
attr_param* db_create_attr_param( char* name, expression* expr );

/*! \brief Parses the specified attribute parameter list for Covered attributes */
void db_parse_attribute( attr_param* ap );

/*! \brief Sets current VCD scope to specified scope. */
void db_set_vcd_scope( char* scope );

/*! \brief Moves current VCD hierarchy up one level */
void db_vcd_upscope();

/*! \brief Adds symbol to signal specified by name. */
void db_assign_symbol( char* name, char* symbol, int msb, int lsb );

/*! \brief Sets the found symbol value to specified character value.  Called by VCD lexer. */
void db_set_symbol_char( char* sym, char value );

/*! \brief Sets the found symbol value to specified string value.  Called by VCD lexer. */
void db_set_symbol_string( char* sym, char* value );

/*! \brief Performs a timestep for all signal changes during this timestep. */
void db_do_timestep( int time ); 

/*! \brief Deallocates all memory associated with a particular design. */
void db_dealloc_design();

/*
 $Log$
 Revision 1.33  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.32  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.31  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.30  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.29  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.28  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.27  2003/01/25 22:39:02  phase1geo
 Fixing case where statement is found to be unsupported in middle of statement
 tree.  The entire statement tree is removed from consideration for simulation.

 Revision 1.26  2002/12/11 14:51:57  phase1geo
 Fixes compiler errors from last checkin.

 Revision 1.25  2002/12/03 06:01:16  phase1geo
 Fixing bug where delay statement is the last statement in a statement list.
 Adding diagnostics to verify this functionality.

 Revision 1.24  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.23  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.22  2002/10/31 23:13:30  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.21  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.20  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.19  2002/10/23 03:39:06  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.17  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.16  2002/09/23 01:37:44  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.15  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.14  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.13  2002/08/26 12:57:03  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.12  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.11  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.10  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.9  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.8  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.7  2002/06/27 12:36:47  phase1geo
 Fixing bugs with scoring.  I think I got it this time.

 Revision 1.6  2002/06/24 12:34:56  phase1geo
 Fixing the set of the STMT_HEAD and STMT_STOP bits.  We are getting close.

 Revision 1.5  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.4  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.3  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.2  2002/04/30 05:04:25  phase1geo
 Added initial go-round of adding statement handling to parser.  Added simple
 Verilog test to check correct statement handling.  At this point there is a
 bug in the expression write function (we need to display statement trees in
 the proper order since they are unlike normal expression trees.)
*/

#endif

