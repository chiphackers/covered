#ifndef __DEFINES_H__
#define __DEFINES_H__

/*!
 \file     defines.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
 \brief    Contains definitions/structures used in the Covered utility.
 
 \par
 This file is included by all files within Covered.  All defines, macros, structures, enums,
 and typedefs in the tool are specified in this file.
*/

#include "../config.h"

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*!
 Specifies current version of the Covered utility.
*/
#define COVERED_VERSION    VERSION

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
#define CDD_VERSION        5

/*!
 This contains the header information specified when executing this tool.
*/
#define COVERED_HEADER     "\nCovered %s -- Verilog Code Coverage Utility\nWritten by Trevor Williams  (trevorw@charter.net)\nFreely distributable under the GPL license\n", COVERED_VERSION

/*!
 Default database filename if not specified on command-line.
*/
#define DFLT_OUTPUT_CDD    "cov.cdd"

/*!
 Default filename that will contain the code necessary to attach Covered as a VPI to the Verilog
 simulator.
*/
#define DFLT_VPI_NAME      "covered_vpi.v"

/*!
 Determine size of integer in bits.
*/
#define INTEGER_WIDTH	   (SIZEOF_INT * 8)

/*!
 Length of user_msg global string (used for inputs to snprintf calls).
*/
#define USER_MSG_LENGTH    4096

/*!
 If -w option is specified to report command, specifies number of characters of width
 we will output.
*/
#define DEFAULT_LINE_WIDTH 105

/*!
 \addtogroup output_type Output type

 The following defines are used by the print_output function to
 determine what type of output to generate and whether the output
 should or should not be suppressed.

 @{
*/

/*!
 Indicates that the specified error is fatal and will cause the
 program to immediately stop.  Cannot be suppressed.
*/
#define FATAL        1

/*!
 Indicates that the specified error is fatal but that the ERROR prefix
 to the error message should not be output.  Used for an error that needs
 multiple lines of output.
*/
#define FATAL_WRAP   2

/*!
 Indicate that the specified error is only a warning that something
 may be wrong but will not cause the program to immediately terminate.
 Suppressed if output_suppressed boolean variable in util.c is set to TRUE.
*/
#define WARNING      3

/*!
 Indicates that the specified error is only a warning but that the
 WARNING prefix should not be output.  Used for warning messages that
 need multiple lines of output.
*/
#define WARNING_WRAP 4

/*!
 Indicate that the specified message is not an error but some type of
 necessary output.  Suppressed if output_suppressed boolean variable in
 util.c is set to TRUE.
*/
#define NORMAL       5

/*!
 Indicates that the specified message is debug information that should
 only be displayed to the screen when the -D flag is specified.
*/
#define DEBUG        6

/*! @} */

/*!
 \addtogroup info_merged_types Merge codes on INFO line.

 The following defines specify various merge types for CDD files.

 @{
*/

#define INFO_NOT_MERGED      0

#define INFO_ONE_MERGED      1

#define INFO_TWO_MERGED      2

/*! @} */

/*!
 \addtogroup db_types Database line types

 The following defines are used in the coverage database to indicate
 the type for parsing purposes.

 @{
*/

/*!
 Specifies that the current coverage database line describes a signal.
*/
#define DB_TYPE_SIGNAL       1

/*!
 Specifies that the current coverage database line describes an expression.
*/
#define DB_TYPE_EXPRESSION   2

/*!
 Specifies that the current coverage database line describes a module.
*/
#define DB_TYPE_FUNIT        3

/*!
 Specifies that the current coverage database line describes a statement.
*/
#define DB_TYPE_STATEMENT    4

/*!
 Specifies that the current coverage database line describes general information.
*/
#define DB_TYPE_INFO         5

/*!
 Specifies that the current coverage database line describes a finite state machine.
*/
#define DB_TYPE_FSM          6

/*!
 Specifies that the current coverage database line describes a race condition block.
*/
#define DB_TYPE_RACE         7

/*! @} */

/*!
 \addtogroup func_unit_types Functional Unit Types

 The following defines specify the type of functional unit being represented in the \ref func_unit
 structure.  A functional unit is defined to be any Verilog structure that contains scope.
 
 @{
*/

/*! Represents a Verilog module (syntax "module <name> ... endmodule") */
#define FUNIT_MODULE         0

/*! Represents a Verilog named block (syntax "begin : <name> ... end") */
#define FUNIT_NAMED_BLOCK    1

/*! Represents a Verilog function (syntax "function <name> ... endfunction") */
#define FUNIT_FUNCTION       2

/*! Represents a Verilog task (syntax "task <name> ... endtask") */
#define FUNIT_TASK           3

/*! The number of valid functional unit types */
#define FUNIT_TYPES          4

/*! @} */

/*!
 \addtogroup report_detail Detailedness of reports.

 The following defines specify the amount of detail to include in a generated report.

 @{
*/

/*!
 Specifies that only summary information should be included in report.
*/
#define REPORT_SUMMARY       0x0

/*!
 Includes summary information as well as verbose output for line and toggle coverage.
 Provides high-level view of combinational logic holes.  This information is useful
 for understanding where uncovered logic exists but not necessarily why it was not
 covered.
*/
#define REPORT_DETAILED      0x2

/*!
 Includes summary information and all verbose information for line, toggle and
 combinational logic.  Shows why combinational logic was not considered covered by
 exposing all missed expressions in each expression tree.  This information is most
 useful in debugging but may be a bit to much to easily discern where logic is not
 covered.
*/
#define REPORT_VERBOSE       0xffffffff

/*! @} */

/*!
 Used for merging two vector nibbles from two vectors.  Both vector nibble
 fields are ANDed with this mask and ORed together to perform the merge.  
 Fields that are merged are:
 - TOGGLE0->1
 - TOGGLE1->0
 - FALSE
 - TRUE
*/
#define VECTOR_MERGE_MASK    0x6c

/*!
 \addtogroup expr_suppl Expression supplemental field defines and macros.

 @{
*/

/*!
 Used for merging two supplemental fields from two expressions.  Both expression
 supplemental fields are ANDed with this mask and ORed together to perform the
 merge.  Fields that are merged are:
 - SWAPPED
 - ROOT
 - EXECUTED
 - TRUE
 - FALSE
 - STMT_STOP
 - STMT_HEAD
 - STMT_CONTINUOUS
 - EVAL 00, 01, 10, 11
*/
#define ESUPPL_MERGE_MASK            0x3f93fff

/*!
 Returns a value of 1 if the specified supplemental value has the SWAPPED
 bit set indicating that the children of the current expression were
 swapped positions during the scoring phase.
*/
#define ESUPPL_WAS_SWAPPED(x)        x.part.swapped

/*!
 Returns a value of 1 if the specified supplemental value has the ROOT bit
 set indicating that the current expression is the root expression of its
 expression tree.
*/
#define ESUPPL_IS_ROOT(x)            x.part.root

/*!
 Returns a value of 1 if the specified supplemental value has the executed
 bit set; otherwise, returns a value of 0 to indicate whether the
 corresponding expression was executed during simulation or not.
*/
#define ESUPPL_WAS_EXECUTED(x)       x.part.executed

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 whose associated statement is a head statement.
*/
#define ESUPPL_IS_STMT_HEAD(x)       x.part.stmt_head

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 whose associated statement is a stop (for writing purposes).
*/
#define ESUPPL_IS_STMT_STOP(x)       x.part.stmt_stop

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 whose associated statement is a continous assignment.
*/
#define ESUPPL_IS_STMT_CONTINUOUS(x) x.part.stmt_cont

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 who was just evaluated to TRUE.
*/
#define ESUPPL_IS_TRUE(x)            x.part.eval_t

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 who was just evaluated to FALSE.
*/
#define ESUPPL_IS_FALSE(x)           x.part.eval_f

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 that has evaluated to a value of TRUE (1) during simulation.
*/
#define ESUPPL_WAS_TRUE(x)           x.part.true

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 that has evaluated to a value of FALSE (0) during simulation.
*/
#define ESUPPL_WAS_FALSE(x)          x.part.false

/*!
 Returns a value of 1 if the left child expression was changed during this
 timestamp.
*/
#define ESUPPL_IS_LEFT_CHANGED(x)    x.part.left_changed

/*!
 Returns a value of 1 if the right child expression was changed during this
 timestamp.
*/
#define ESUPPL_IS_RIGHT_CHANGED(x)   x.part.right_changed

/*!
 Returns a value of 1 if the specified expression has already been counted
 for combinational coverage.
*/
#define ESUPPL_WAS_COMB_COUNTED(x)   x.part.comb_cntd

/*!
 Returns a value of 1 if the specified expression exists on the left-hand side
 of an assignment operation.
*/
#define ESUPPL_IS_LHS(x)             x.part.lhs

/*! @} */
     
/*!
 \addtogroup read_modes Modes for reading database file

 Specify how to integrate read data from database file into memory structures.

 @{
*/

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when performing a MERGE command or reading after parsing source files.
*/
#define READ_MODE_MERGE_NO_MERGE          0

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when performing a REPORT command.
*/
#define READ_MODE_REPORT_NO_MERGE         1

/*!
 When functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the current instance tree.  If the instance exists, the
 functional unit is merged with the instance's functional unit; otherwise, we are attempting to
 merge two databases that were created from different designs.
*/
#define READ_MODE_MERGE_INST_MERGE        2

/*!
 When functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the functional unit list.  If the functional unit is found,
 it is merged with the existing functional unit; otherwise, it is added to the functional
 unit list.
*/
#define READ_MODE_REPORT_MOD_MERGE        3

/*!
 When a functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the functional unit list.  If the functional unit is found,
 it is replaced (unless the functional unit has already been replaced, in which case it is merged) with
 the existing functional unit.
*/
#define READ_MODE_REPORT_MOD_REPLACE      4
/*! @} */

/*!
 \addtogroup param_suppl Instance/module parameter supplemental field definitions.

 @{
*/

/*!
 Specifies the least significant bit of the parameter order for current parameter
 in module.
*/
#define PARAM_LSB_ORDER                 0

/*!
 Specifies the least significant bit of the parameter type (declared/override).
*/
#define PARAM_LSB_TYPE                  16

/*!
 Specifies that the current module parameter is a declared type (belongs to current 
 module).
*/
#define PARAM_TYPE_DECLARED             0

/*!
 Specifies that the current module parameter is an override type (overrides the value
 of an instantiated modules parameter declaration).
*/
#define PARAM_TYPE_OVERRIDE             1

/*!
 Specifies that the current module parameter specifies the value for a signal LSB
 value.
*/
#define PARAM_TYPE_SIG_LSB              2

/*!
 Specifies that the current module parameter specifies the value for a signal MSB
 value.
*/
#define PARAM_TYPE_SIG_MSB              3

/*!
 Specifies that the current module parameter specifies the value for an expression
 LSB value.
*/
#define PARAM_TYPE_EXP_LSB              4

/*!
 Specifies that the current module parameter specifies the value for an expression
 MSB value.
*/
#define PARAM_TYPE_EXP_MSB              5

/*!
 Returns the order for the specified modparm type.
*/
#define PARAM_ORDER(x)                  ((x->suppl >> PARAM_LSB_ORDER) & 0xffff)

/*!
 Returns the type (declared/override) of the specified modparm type.
*/
#define PARAM_TYPE(x)                   ((x->suppl >> PARAM_LSB_TYPE) & 0x7)

/*! @} */

/*!
 \addtogroup delay_expr_types  Delay expression types.
 
 The following defines are used select a delay expression type in the form min:typ:max
 when encountered during parsing.
 
 @{
*/

/*!
 Default value of delay expression selector to indicate that the user has not
 specified a value to use.
*/
#define DELAY_EXPR_DEFAULT      0

/*!
 Causes parser to always use min value in min:typ:max delay expressions.
*/
#define DELAY_EXPR_MIN          1

/*!
 Causes parser to always use typ value in min:typ:max delay expressions.
*/
#define DELAY_EXPR_TYP          2

/*!
 Causes parser to always use max value in min:typ:max delay expressions.
*/
#define DELAY_EXPR_MAX          3

/*! @} */

/*!
 \addtogroup expr_ops Expression operations

 The following defines are used for identifying expression operations.

 @{
*/
/*! Decimal value = 0.  Specifies constant value. */
#define EXP_OP_STATIC     0x00
/*! Decimal value = 1.  Specifies signal value. */
#define EXP_OP_SIG        0x01
/*! Decimal value = 2.  Specifies '^' operator. */
#define EXP_OP_XOR        0x02
/*! Decimal value = 3.  Specifies '*' operator. */
#define EXP_OP_MULTIPLY   0x03
/*! Decimal value = 4.  Specifies '/' operator. */
#define EXP_OP_DIVIDE     0x04
/*! Decimal value = 5.  Specifies '%' operator. */
#define EXP_OP_MOD        0x05
/*! Decimal value = 6.  Specifies '+' operator. */
#define EXP_OP_ADD        0x06
/*! Decimal value = 7.  Specifies '-' operator. */
#define EXP_OP_SUBTRACT   0x07
/*! Decimal value = 8.  Specifies '&' operator. */
#define EXP_OP_AND        0x08
/*! Decimal value = 9.  Specifies '|' operator. */
#define EXP_OP_OR         0x09
/*! Decimal value = 10.  Specifies '~&' operator. */
#define EXP_OP_NAND       0x0a
/*! Decimal value = 11.  Specifies '~|' operator. */
#define EXP_OP_NOR        0x0b
/*! Decimal value = 12.  Specifies '~^' operator. */
#define EXP_OP_NXOR       0x0c
/*! Decimal value = 13.  Specifies '<' operator. */
#define EXP_OP_LT         0x0d
/*! Decimal value = 14.  Specifies '>' operator. */
#define EXP_OP_GT         0x0e
/*! Decimal value = 15.  Specifies '<<' operator. */
#define EXP_OP_LSHIFT     0x0f
/*! Decimal value = 16.  Specifies '>>' operator. */
#define EXP_OP_RSHIFT     0x10
/*! Decimal value = 17.  Specifies '==' operator. */
#define EXP_OP_EQ         0x11
/*! Decimal value = 18.  Specifies '===' operator. */
#define EXP_OP_CEQ        0x12
/*! Decimal value = 19.  Specifies '<=' operator. */
#define EXP_OP_LE         0x13
/*! Decimal value = 20.  Specifies '>=' operator. */
#define EXP_OP_GE         0x14
/*! Decimal value = 21.  Specifies '!=' operator. */
#define EXP_OP_NE         0x15
/*! Decimal value = 22.  Specifies '!==' operator. */
#define EXP_OP_CNE        0x16
/*! Decimal value = 23.  Specifies '||' operator. */
#define EXP_OP_LOR        0x17
/*! Decimal value = 24.  Specifies '&&' operator. */
#define EXP_OP_LAND       0x18
/*! Decimal value = 25.  Specifies '?:' conditional operator. */
#define EXP_OP_COND       0x19
/*! Decimal value = 26.  Specifies '?:' conditional select. */
#define EXP_OP_COND_SEL   0x1a
/*! Decimal value = 27.  Specifies '~' unary operator. */
#define EXP_OP_UINV       0x1b
/*! Decimal value = 28.  Specifies '&' unary operator. */
#define EXP_OP_UAND       0x1c
/*! Decimal value = 29.  Specifies '!' unary operator. */
#define EXP_OP_UNOT       0x1d
/*! Decimal value = 30.  Specifies '|' unary operator. */
#define EXP_OP_UOR        0x1e
/*! Decimal value = 31.  Specifies '^' unary operator. */
#define EXP_OP_UXOR       0x1f
/*! Decimal value = 32.  Specifies '~&' unary operator. */
#define EXP_OP_UNAND      0x20
/*! Decimal value = 33.  Specifies '~|' unary operator. */
#define EXP_OP_UNOR       0x21
/*! Decimal value = 34.  Specifies '~^' unary operator. */
#define EXP_OP_UNXOR      0x22
/*! Decimal value = 35.  Specifies single-bit signal select (i.e., [x]). */
#define EXP_OP_SBIT_SEL   0x23
/*! Decimal value = 36.  Specifies multi-bit signal select (i.e., [x:y]). */
#define EXP_OP_MBIT_SEL   0x24
/*! Decimal value = 37.  Specifies bit expansion operator (i.e., {x{y}}). */
#define EXP_OP_EXPAND     0x25
/*! Decimal value = 38.  Specifies signal concatenation operator (i.e., {x,y}). */
#define EXP_OP_CONCAT     0x26
/*! Decimal value = 39.  Specifies posedge operator (i.e., \@posedge x). */
#define EXP_OP_PEDGE      0x27
/*! Decimal value = 40.  Specifies negedge operator (i.e., \@negedge x). */
#define EXP_OP_NEDGE      0x28
/*! Decimal value = 41.  Specifies anyedge operator (i.e., \@x). */
#define EXP_OP_AEDGE      0x29
/*! Decimal value = 42.  Specifies 1-bit holder for parent. */
#define EXP_OP_LAST       0x2a
/*! Decimal value = 43.  Specifies 'or' event operator. */
#define EXP_OP_EOR        0x2b
/*! Decimal value = 44.  Specifies delay operator (i.e., #(x)). */
#define EXP_OP_DELAY      0x2c
/*! Decimal value = 45.  Specifies case equality expression. */
#define EXP_OP_CASE       0x2d
/*! Decimal value = 46.  Specifies casex equality expression. */
#define EXP_OP_CASEX      0x2e
/*! Decimal value = 47.  Specifies casez equality expression. */
#define EXP_OP_CASEZ      0x2f
/*! Decimal value = 48.  Specifies case/casex/casez default expression. */
#define EXP_OP_DEFAULT    0x30
/*! Decimal value = 49.  Specifies comma separated expression list. */
#define EXP_OP_LIST       0x31
/*! Decimal value = 50.  Specifies full parameter. */
#define EXP_OP_PARAM      0x32
/*! Decimal value = 51.  Specifies single-bit select parameter. */
#define EXP_OP_PARAM_SBIT 0x33
/*! Decimal value = 52.  Specifies multi-bit select parameter. */
#define EXP_OP_PARAM_MBIT 0x34
/*! Decimal value = 53.  Specifies an assign assignment operator. */
#define EXP_OP_ASSIGN     0x35
/*! Decimal value = 54.  Specifies a wire declaration assignment operator. */
#define EXP_OP_DASSIGN    0x36
/*! Decimal value = 55.  Specifies a blocking assignment operator. */
#define EXP_OP_BASSIGN    0x37
/*! Decimal value = 56.  Specifies a non-blocking assignment operator. */
#define EXP_OP_NASSIGN    0x38
/*! Decimal value = 57.  Specifies an if statement operator. */
#define EXP_OP_IF         0x39
/*! Decimal value = 58.  Specifies a function call. */
#define EXP_OP_FUNC_CALL  0x3a
/*! Decimal value = 59.  Specifies a task call (note: this operator MUST be the root of the expression tree) */
#define EXP_OP_TASK_CALL  0x3b
/*! Decimal value = 60.  Specifies an event trigger (->). */
#define EXP_OP_TRIGGER    0x3c
/*! The total number of defines for expression values */
#define EXP_OP_NUM        61

/*! @} */

/*!
 Returns a value of 1 if the specified expression is considered to be measurable.
*/
#define EXPR_IS_MEASURABLE(x)      (((x->op != EXP_OP_STATIC) && \
                                     (x->op != EXP_OP_LAST) && \
                                     (x->op != EXP_OP_LIST) && \
                                     (x->op != EXP_OP_COND_SEL) && \
                                     (x->op != EXP_OP_CASE) && \
                                     (x->op != EXP_OP_CASEX) && \
                                     (x->op != EXP_OP_CASEZ) && \
                                     (x->op != EXP_OP_DEFAULT) && \
                                     (x->op != EXP_OP_PARAM) && \
                                     (x->op != EXP_OP_PARAM_SBIT) && \
                                     (x->op != EXP_OP_PARAM_MBIT) && \
                                     (x->op != EXP_OP_DELAY) && \
                                     (x->op != EXP_OP_EOR) && \
                                     (x->op != EXP_OP_ASSIGN) && \
                                     (x->op != EXP_OP_DASSIGN) && \
                                     (x->op != EXP_OP_BASSIGN) && \
                                     (x->op != EXP_OP_NASSIGN) && \
                                     (x->op != EXP_OP_IF) && \
                                     (x->op != EXP_OP_TASK_CALL) && \
				     (x->op != EXP_OP_TRIGGER) && \
                                     (ESUPPL_IS_LHS( x->suppl ) == 0) && \
                                     !((ESUPPL_IS_ROOT( x->suppl ) == 0) && \
                                       ((x->op == EXP_OP_SIG) || \
                                        (x->op == EXP_OP_SBIT_SEL) || \
                                        (x->op == EXP_OP_MBIT_SEL)) && \
                                       (x->parent->expr->op != EXP_OP_ASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_DASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_BASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_NASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_IF) && \
                                       (x->parent->expr->op != EXP_OP_COND)) && \
                                     (x->line != 0)) ? 1 : 0)

/*!
 Returns a value of TRUE if the specified expression is a STATIC, PARAM, PARAM_SBIT or PARAM_MBIT
 operation type.
*/
#define EXPR_IS_STATIC(x)        ((x->op == EXP_OP_STATIC)     || \
                                  (x->op == EXP_OP_PARAM)      || \
                                  (x->op == EXP_OP_PARAM_SBIT) || \
                                  (x->op == EXP_OP_PARAM_MBIT))

/*!
 Returns a value of true if the specified expression is considered a combination expression by
 the combinational logic report generator.
*/
#define EXPR_IS_COMB(x)         ((x->op == EXP_OP_XOR)      || \
		                 (x->op == EXP_OP_ADD)      || \
				 (x->op == EXP_OP_SUBTRACT) || \
				 (x->op == EXP_OP_AND)      || \
				 (x->op == EXP_OP_OR)       || \
				 (x->op == EXP_OP_NAND)     || \
				 (x->op == EXP_OP_NOR)      || \
				 (x->op == EXP_OP_NXOR)     || \
				 (x->op == EXP_OP_LOR)      || \
				 (x->op == EXP_OP_LAND))

/*!
 Returns a value of true if the specified expression is considered to be an event type
 (i.e., it either occurred or did not occur -- WAS_FALSE is not important).
*/
#define EXPR_IS_EVENT(x)        ((x->op == EXP_OP_PEDGE) || \
                                 (x->op == EXP_OP_NEDGE) || \
                                 (x->op == EXP_OP_AEDGE) || \
                                 (x->op == EXP_OP_DELAY))

/*!
 Returns a value of true if the specified expression is considered a unary expression by
 the combinational logic report generator.
*/
#define EXPR_IS_UNARY(x)        !EXPR_IS_COMB(x) && !EXPR_IS_EVENT(x)

/*!
 Returns a value of 1 if the specified expression was measurable for combinational 
 coverage but not fully covered during simulation.
*/
#define EXPR_COMB_MISSED(x)        (EXPR_IS_MEASURABLE( x ) && \
                                    !expression_is_static_only( x ) && \
				    ((EXPR_IS_COMB( x ) && \
				      (!x->suppl.part.eval_00 || \
                                       !x->suppl.part.eval_01 || \
                                       !x->suppl.part.eval_10 || \
                                       !x->suppl.part.eval_11)) || \
				     !ESUPPL_WAS_TRUE( x->suppl ) || \
				     (!ESUPPL_WAS_FALSE( x->suppl ) && \
                                      !EXPR_IS_EVENT( x ))))

/*!
 \addtogroup op_tables

 The following describe the operation table values for AND, OR, XOR, NAND, NOR and
 NXOR operations.

 @{
*/

/*!
 Specifies the number of entries in an optab array.
*/
#define OPTAB_SIZE        17
 
/*                        00  01  02  03  10  11  12  13  20  21  22  23  30  31  32  33  NOT*/
#define AND_OP_TABLE      0,  0,  0,  0,  0,  1,  2,  2,  0,  2,  2,  2,  0,  2,  2,  2,  0
#define OR_OP_TABLE       0,  1,  2,  2,  1,  1,  1,  1,  2,  1,  2,  2,  2,  1,  2,  2,  0
#define XOR_OP_TABLE      0,  1,  2,  2,  1,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  0
#define NAND_OP_TABLE     1,  1,  1,  1,  1,  0,  2,  2,  1,  2,  2,  2,  1,  2,  2,  2,  1
#define NOR_OP_TABLE      1,  0,  2,  2,  0,  0,  0,  0,  2,  0,  2,  2,  2,  0,  2,  2,  1
#define NXOR_OP_TABLE     1,  0,  2,  2,  0,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  1
#define ADD_OP_TABLE      0,  1,  10, 10, 1,  4,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 0

/*! @} */

/*!
 \addtogroup comparison_types

 The following are comparison types used in the vector_op_compare function in vector.c:

 @{
*/

#define COMP_LT         0       /*!< Less than                */
#define COMP_GT         1       /*!< Greater than             */
#define COMP_LE         2       /*!< Less than or equal to    */
#define COMP_GE         3       /*!< Greater than or equal to */
#define COMP_EQ         4       /*!< Equal to                 */
#define COMP_NE         5       /*!< Not equal to             */
#define COMP_CEQ        6       /*!< Case equality            */
#define COMP_CNE        7       /*!< Case inequality          */
#define COMP_CXEQ       8       /*!< Casex equality           */
#define COMP_CZEQ       9       /*!< Casez equality           */

/*! @} */

/*!
 \addtogroup lexer_val_types

 The following defines specify what type of vector value can be extracted from a value
 string.  These defines are used by vector.c and the lexer.
 
 @{
*/

#define MAX_BIT_WIDTH           1024    /*!< Maximum number of bits that a vector can hold */
#define DECIMAL			0	/*!< String in format [dD][0-9]+                   */
#define BINARY			1	/*!< String in format [bB][01xXzZ_\?]+             */
#define OCTAL			2	/*!< String in format [oO][0-7xXzZ_\?]+            */
#define HEXIDECIMAL		3	/*!< String in format [hH][0-9a-fA-FxXzZ_\?]+      */
#define QSTRING                 4       /*!< Quoted string                                 */

/*! @} */

/*!
 \addtogroup arc_fields

 The following defines specify the bit fields in a state transition arc entry.

 @{
*/

#define ARC_HIT_F               0       /*!< From state -> to state hit - forward               */
#define ARC_HIT_R               1       /*!< To state -> from state hit - reverse               */
#define ARC_BIDIR               2       /*!< Entry is bidirectional                             */
#define ARC_NOT_UNIQUE_R        3       /*!< Right state is not unique                          */
#define ARC_NOT_UNIQUE_L        4       /*!< Left state is not unique                           */
#define ARC_ENTRY_SUPPL_SIZE    5       /*!< Number of bits comprising entry supplemental field */

#define ARC_STATUS_SIZE         7       /*!< Number of characters comprising arc status         */

#define ARC_TRANS_KNOWN         0       /*!< Bit position of transitions known field in suppl   */

/*! @} */

/*!
 \addtogroup attribute_types Attribute Types

 The following defines specify the attribute types that are parseable by Covered.

 @{
*/

#define ATTRIBUTE_UNKNOWN       0       /*!< This attribute is not recognized by Covered */
#define ATTRIBUTE_FSM           1       /*!< FSM attribute                               */

/*! @} */

/*!
 \addtogroup race_condition_types Race Condition Violation Types

 The following group of defines specify all of the types of race conditions that Covered is
 currently capable of detecting:

 @{
*/

/*! All sequential logic uses non-blocking assignments */
#define RACE_TYPE_SEQ_USES_NON_BLOCK         0

/*! All combinational logic in an always block uses blocking assignments */
#define RACE_TYPE_CMB_USES_BLOCK             1

/*! All mixed sequential and combinational logic in the same always block uses non-blocking assignments */
#define RACE_TYPE_MIX_USES_NON_BLOCK         2

/*! Blocking and non-blocking assignments should not be used in the same always block */
#define RACE_TYPE_HOMOGENOUS                 3

/*! Assignments made to a variable should only be done within one always block */
#define RACE_TYPE_ASSIGN_IN_ONE_BLOCK1       4

/*! Signal assigned both in statement block and via input/inout port */
#define RACE_TYPE_ASSIGN_IN_ONE_BLOCK2       5

/*! The $strobe system call should only be used to display variables that were assigned using non-blocking assignments */
#define RACE_TYPE_STROBE_DISPLAY_NON_BLOCK   6

/*! No #0 procedural assignments should exist */
#define RACE_TYPE_NO_POUND_0_PROC_ASSIGNS    7

/*! Total number of race condition checks in this list */
#define RACE_TYPE_NUM                        8

/*! @} */

#define snprintf(x,y,...)	{ int svar = snprintf( x, y, __VA_ARGS__ ); assert( svar < (y) ); }

/*!
 Defines boolean variables used in most functions.
*/
typedef enum {
  FALSE,   /*!< Boolean false value */
  TRUE     /*!< Boolean true value  */
} bool;

#if SIZEOF_INT == 4

/*!
 A nibble is a 8-bit value.
*/
typedef unsigned char nibble;

/*!
 A control is a 32-bit value.
*/
typedef unsigned int control;
#else
#if SIZEOF_LONG == 4
typedef unsigned long nibble;
typedef unsigned long control;
#endif
#endif

/*------------------------------------------------------------------------------*/
/*!
 A vec_data is an 8-bit value that represents one bit of data in a signal or expression/subexpression
*/
union vec_data_u;

/*!
 Renaming vec_data_u for naming convenience.
*/
typedef union vec_data_u vec_data;

union vec_data_u {
  nibble all;        /*!< Reference to all bits in this union                         */
  struct {
    nibble value:2;  /*!< 4-state value                                               */
    nibble tog01:1;  /*!< Indicator if bit was toggled from 0->1                      */
    nibble tog10:1;  /*!< Indicator if bit was toggled from 1->0                      */
    nibble set  :1;  /*!< Indicator if bit has been previously assigned this timestep */
    nibble false:1;  /*!< Indicator if bit was set to a value of 0 (FALSE)            */
    nibble true :1;  /*!< Indicator if bit was set to a value of 1 (TRUE)             */
    nibble misc :1;  /*!< Miscellaneous indicator bit                                 */
  } part;
};

/*------------------------------------------------------------------------------*/
/*!
 A esuppl is a 32-bit value that represents the supplemental field of an expression.
*/
union esuppl_u;

/*!
 Renaming esuppl_u for naming convenience.
*/
typedef union esuppl_u esuppl;

union esuppl_u {
  control   all;               /*!< Controls all bits within this union                                             */
  struct {
    control swapped       :1;  /*!< Bit 0.  Indicates that the children of this expression have been swapped.  The swapping
                                    of the positions is performed by the score command (for multi-bit selects) and
                                    this bit indicates to the report code to swap them back when displaying them in
                                    a report.                                                                       */
    control root          :1;  /*!< Bit 1.  Indicates that this expression is a root expression.  Traversing to the parent
                                    pointer will take you to a statement type.                                      */
    control executed      :1;  /*!< Bit 2.  Indicates that this expression has been executed in the queue during the
                                    lifetime of the simulation.                                                     */
    control stmt_head     :1;  /*!< Bit 3.  Indicates the statement which this expression belongs is a head statement (only
                                    valid for root expressions -- parent expression == NULL).                       */
    control stmt_stop     :1;  /*!< Bit 4.  Indicates the statement which this expression belongs should write itself to
                                    the CDD and not continue to traverse its next_true and next_false pointers.     */
    control stmt_cont     :1;  /*!< Bit 5.  Indicates the statement which this expression belongs is part of a continuous
                                    assignment.  As such, stop simulating this statement tree after this expression
                                    tree is evaluated.                                                              */
    control false         :1;  /*!< Bit 6.  Indicates that this expression has evaluated to a value of FALSE during the
                                    lifetime of the simulation.                                                     */
    control true          :1;  /*!< Bit 7.  Indicates that this expression has evaluated to a value of TRUE during the
                                    lifetime of the simulation.                                                     */
    control left_changed  :1;  /*!< Bit 8.  Indicates that this expression has its left child expression in a changed
                                    state during this timestamp.                                                    */
    control right_changed :1;  /*!< Bit 9.  Indicates that this expression has its right child expression in a changed
                                    state during this timestamp.                                                    */
    control eval_00       :1;  /*!< Bit 10.  Indicates that the value of the left child expression evaluated to FALSE
                                    and the right child expression evaluated to FALSE.                              */
    control eval_01       :1;  /*!< Bit 11.  Indicates that the value of the left child expression evaluated to FALSE and
                                    the right child expression evaluated to TRUE.                                   */
    control eval_10       :1;  /*!< Bit 12.  Indicates that the value of the left child expression evaluated to TRUE and the
                                    right child expression evaluated to FALSE.                                      */
    control eval_11       :1;  /*!< Bit 13.  Indicates that the value of the left child expression evaluated to TRUE and the
                                    right child expression evaluated to TRUE.                                       */
    control eval_t        :1;  /*!< Bit 14.  Indicates that the value of the current expression is currently set to TRUE
                                    (temporary value).                                                              */
    control eval_f        :1;  /*!< Bit 15.  Indicates that the value of the current expression is currently set to FALSE
                                    (temporary value).                                                              */
    control comb_cntd     :1;  /*!< Bit 16.  Indicates that the current expression has been previously counted for
                                    combinational coverage.  Only set by report command (therefore this bit will
                                    always be a zero when written to CDD file.                                      */
    control stmt_connected:1;  /*!< Bit 17.  Temporary bit value used by the score command but not displayed to the CDD
                                    file.  When this bit is set to a one, it indicates to the statement_connect
                                    function that this statement and all children statements do not need to be
                                    connected to another statement. */
    control stmt_added    :1;  /*!< Bit 18.  Temporary bit value used by the score command but not displayed to the CDD
                                    file.  When this bit is set to a one, it indicates to the db_add_statement
                                    function that this statement and all children statements have already been
                                    added to the functional unit statement list and should not be added again. */
    control lhs           :1;  /*!< Bit 19.  Indicates that this expression exists on the left-hand side of an assignment
                                    operation. */
  } part;
};


/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION DECLARATIONS  */

/*!
 Allows the parent pointer of an expression to point to either another expression
 or a statement.
*/
union expr_stmt_u;

/*!
 Specifies an element in a linked list containing string values.  This data
 structure allows us to add new elements to the list without resizing, this
 optimizes performance with small amount of overhead.
*/
struct str_link_s;

/*!
 Contains information for signal value.  This value is represented as
 a generic vector.  The vector.h/.c files contain the functions that
 manipulate this information.
*/
struct vector_s;

/*!
 An expression is defined to be a logical combination of signals/values.  Expressions may
 contain subexpressions (which are expressions in and of themselves).  An measurable expression
 may only evaluate to TRUE (1) or FALSE (0).  If the parent expression of this expression is
 NULL, then this expression is considered a root expression.  The nibble suppl contains the
 run-time information for its expression.
*/
struct expression_s;

/*!
 Stores all information needed to represent a signal.  If value of value element is non-zero at the
 end of the run, this signal has been simulated.
*/
struct vsignal_s;

/*!
 TBD
*/
struct fsm_s;

/*!
 A statement is defined to be the structure connected to the root of an expression tree.
 Statements are sequentially run in the run-time engine, starting at the root statement.
 After a statements expression tree has been checked for changes and possibly placed into
 the run-time expression queue, the statements calls the next statement to be run.
 If the value of the root expression of the associated expression tree is a non-zero value,
 the next_true statement will be executed (if next_true is not NULL); otherwise, the
 next_false statement is run.  If next_true and next_false point to the same structure, we
 have hit the end of the statement sequence; executing the next statement will be executing
 the first statement of the statement sequence (next_true and next_false should both point
 to the first statement in the sequence).
*/
struct statement_s;

/*!
 Linked list element that stores a signal.
*/
struct sig_link_s;

/*!
 Statement link iterator.
*/
struct stmt_iter_s;

/*!
 Expression link element.  Stores pointer to an expression.
*/
struct exp_link_s;

/*!
 Statement link element.  Stores pointer to a statement.
*/
struct stmt_link_s;

/*!
 Special statement link that stores the ID of the statement that the specified
 statement pointer needs to traverse when it has completed.  These structure types
 are used by the statement CDD reader.  When a statement is read that points to a
 statement that hasn't been read out of the CDD, the read statement is stored into
 one of these link types that is linked like a stack (pushed/popped at the head).
 The head of this stack is interrogated by future statements being read out.  When
 a statement's ID matches the ID at the head of the stack, the element is popped and
 the two statements are linked accordingly.  This sequence is used to handle statement
 looping.
*/
struct stmt_loop_link_s;

/*!
 Contains statistics for coverage results which is stored in a functional unit instance.
*/
struct statistic_s;

/*!
 Structure containing parts for a module parameter definition.
*/
struct mod_parm_s;

/*!
 Structure containing parts for an instance parameter.
*/
struct inst_parm_s;

/*!
 TBD
*/
struct fsm_arc_s;

/*!
 Linked list element that stores an FSM structure.
*/
struct fsm_link_s;

/*!
 Contains information for storing race condition information
*/
struct race_blk_s;

/*!
 Contains information for a functional unit (i.e., module, named block, function or task).
*/
struct func_unit_s;

/*!
 Linked list element that stores a functional unit (no scope).
*/
struct funit_link_s;

/*!
 For each signal within a symtable entry, an independent MSB and LSB needs to be
 stored along with the signal pointer that it references to properly assign the
 VCD signal value to the appropriate signal.  This structure is setup to hold these
 three key pieces of information in a list-style data structure.
*/
struct sym_sig_s;

/*!
 Stores symbol name of signal along with pointer to signal itself into a lookup table
*/
struct symtable_s;

/*!
 Specifies possible values for a static expression (constant value).
*/
struct static_expr_s;

/*!
 Specifies bit range of a signal or expression.
*/
struct vector_width_s;

/*!
 Binds a signal to an expression.
*/
struct exp_bind_s;

/*!
 Binds an expression to a statement.  This is used when constructing a case
 structure.
*/
struct case_stmt_s;

/*!
 A functional unit instance element in the functional unit instance tree.
*/
struct funit_inst_s;

/*!
 Node for a tree that carries two strings:  a key and a value.  The tree is a binary
 tree that is sorted by key.
*/
struct tnode_s;

#ifdef HAVE_SYS_TIMES_H
/*!
 Structure for holding code timing data.  This information can be useful for optimizing
 code segments.  To use a timer, create a pointer to a timer structure in global
 memory and assign the pointer to the value NULL.  Then simply call timer_start to
 start timing and timer_stop to stop timing.  You may call as many timer_start/timer_stop
 pairs as needed and the total timed time will be kept in the structure's total variable.
 To clear the total for a timer, call timer_clear.
*/
struct timer_s;
#endif

/*!
 TBD
*/
struct fsm_var_s;

/*!
 TBD
*/
struct fv_bind_s;

/*!
 TBD
*/
struct attr_param_s;

/*!
 The stmt_blk structure contains extra information for a given signal that is only used during
 the parsing stage (therefore, the information does not need to be specified in the vsignal
 structure itself) and is used to keep track of information about that signal's use in the current
 functional unit.  It is used for race condition checking.
*/
struct stmt_blk_s;

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION TYPEDEFS  */

/*!
 Renaming string link structure for convenience.
*/
typedef struct str_link_s str_link;

/*!
 Renaming vector structure for convenience.
*/
typedef struct vector_s vector;

/*!
 Renaming expression statement union for convenience.
*/
typedef union expr_stmt_u expr_stmt;

/*!
 Renaming expression structure for convenience.
*/
typedef struct expression_s expression;

/*!
 Renaming signal structure for convenience.
*/
typedef struct vsignal_s     vsignal;

/*!
 Renaming FSM structure for convenience.
*/
typedef struct fsm_s fsm;

/*!
 Renaming statement structure for convenience.
*/
typedef struct statement_s statement;

/*!
 Renaming signal link structure for convenience.
*/
typedef struct sig_link_s sig_link;

/*!
 Renaming statement iterator structure for convenience.
*/
typedef struct stmt_iter_s stmt_iter;

/*!
 Renaming expression link structure for convenience.
*/
typedef struct exp_link_s exp_link;

/*!
 Renaming statement link structure for convenience.
*/
typedef struct stmt_link_s stmt_link;

/*!
 Renaming statement loop link structure for convenience.
*/
typedef struct stmt_loop_link_s stmt_loop_link;

/*!
 Renaming statistic structure for convenience.
*/
typedef struct statistic_s statistic;

/*!
 Renaming module parameter structure for convenience.
*/
typedef struct mod_parm_s mod_parm;

/*!
 Renaming instance parameter structure for convenience.
*/
typedef struct inst_parm_s inst_parm;

/*!
 Renaming FSM arc structure for convenience.
*/
typedef struct fsm_arc_s fsm_arc;

/*!
 Renaming fsm_link structure for convenience.
*/
typedef struct fsm_link_s fsm_link;

/*!
 Renaming race_blk structure for convenience.
*/
typedef struct race_blk_s race_blk;

/*!
 Renaming functional unit structure for convenience.
*/
typedef struct func_unit_s func_unit;

/*!
 Renaming functional unit link structure for convenience.
*/
typedef struct funit_link_s funit_link;

/*!
 Renaming symbol signal structure for convenience.
*/
typedef struct sym_sig_s sym_sig;

/*!
 Renaming symbol table structure for convenience.
*/
typedef struct symtable_s symtable;

/*!
 Renaming static expression structure for convenience.
*/
typedef struct static_expr_s static_expr;

/*!
 Renaming vector width structure for convenience.
*/
typedef struct vector_width_s vector_width;

/*!
 Renaming signal/functional unit to expression binding structure for convenience.
*/
typedef struct exp_bind_s exp_bind;

/*!
 Renaming case statement structure for convenience.
*/
typedef struct case_stmt_s case_statement;

/*!
 Renaming functional unit instance structure for convenience.
*/
typedef struct funit_inst_s funit_inst;

/*!
 Renaming tree node structure for convenience.
*/
typedef struct tnode_s tnode;

#ifdef HAVE_SYS_TIMES_H
/*!
 Renaming timer structure for convenience.
*/
typedef struct timer_s timer;
#endif

/*!
 Renaming FSM variable structure for convenience.
*/
typedef struct fsm_var_s fsm_var;

/*!
 Renaming FSM bind structure for convenience.
*/
typedef struct fv_bind_s fv_bind;

/*!
 Renaming attribute parameter structure for convenience.
*/
typedef struct attr_param_s attr_param;

/*!
 Renaming statement-signal structure for convenience.
*/
typedef struct stmt_blk_s stmt_blk;

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION DEFINITIONS  */

struct str_link_s {
  char*     str;                     /*!< String to store */
  char      suppl;                   /*!< 8-bit additional information */
  str_link* next;                    /*!< Pointer to next str_link element */
};

struct vector_s {
  int        width;                  /*!< Bit width of this vector */
  union {
    nibble   all;                    /*!< Allows us to set all bits in the suppl field */
    struct {
      nibble base    :3;             /*!< Base-type of this data when originally parsed */
      nibble wait    :1;             /*!< Specifies that this signal should be waited for */
      nibble inport  :1;             /*!< Specifies if this vector is part of an input port */
      nibble assigned:1;             /*!< Specifies that this vector will be assigned from simulated results (instead of dumpfile) */
      nibble mba     :1;             /*!< Specifies that this vector MUST be assigned from simulated results because this information
                                          is NOT provided in the dumpfile */
    } part;
  } suppl;                           /*!< Supplemental field */
  vec_data*  value;                  /*!< 4-state current value and toggle history */
};

union expr_stmt_u {
  expression* expr;                  /*!< Pointer to expression */
  statement*  stmt;                  /*!< Pointer to statement */
};

struct expression_s {
  vector*     value;                 /*!< Current value and toggle information of this expression */
  control     op;                    /*!< Expression operation type (see \ref expr_ops for the list of legal values) */
  esuppl      suppl;                 /*!< Supplemental information for the expression */
  int         id;                    /*!< Specifies unique ID for this expression in the parent */
  int         ulid;                  /*!< Specifies underline ID for reporting purposes */
  int         line;                  /*!< Specified line in file that this expression is found on */
  control     col;                   /*!< Specifies column location of beginning/ending of expression */
  vsignal*    sig;                   /*!< Pointer to signal.  If NULL then no signal is attached */
  expr_stmt*  parent;                /*!< Parent expression/statement */
  expression* right;                 /*!< Pointer to expression on right */
  expression* left;                  /*!< Pointer to expression on left */
  fsm*        table;                 /*!< Pointer to FSM table associated with this expression */
  statement*  stmt;                  /*!< Pointer to starting task/function statement to be called by this expression */
};

struct vsignal_s {
  char*      name;                   /*!< Full hierarchical name of signal in design */
  vector*    value;                  /*!< Pointer to vector value of this signal */
  int        lsb;                    /*!< Least-significant bit position of this signal */
  exp_link*  exp_head;               /*!< Head pointer to list of expressions */
  exp_link*  exp_tail;               /*!< Tail pointer to list of expressions */
};

struct fsm_s {
  char*       name;                  /*!< User-defined name that this FSM pertains to */
  expression* from_state;            /*!< Pointer to from_state expression */
  expression* to_state;              /*!< Pointer to to_state expression */
  fsm_arc*    arc_head;              /*!< Pointer to head of list of expression pairs that describe the valid FSM arcs */
  fsm_arc*    arc_tail;              /*!< Pointer to tail of list of expression pairs that describe the valid FSM arcs */
  char*       table;                 /*!< FSM arc traversal table */
};

struct statement_s {
  expression* exp;                   /*!< Pointer to associated expression tree                        */
  sig_link*   wait_sig_head;         /*!< Pointer to head of wait event signal list                    */
  sig_link*   wait_sig_tail;         /*!< Pointer to tail of wait event signal list                    */
  statement*  next_true;             /*!< Pointer to next statement to run if expression tree non-zero */
  statement*  next_false;            /*!< Pointer to next statement to run if next_true not picked     */
};

struct sig_link_s {
  vsignal*  sig;                     /*!< Pointer to signal in list */
  sig_link* next;                    /*!< Pointer to next signal link element in list */
};

struct stmt_iter_s {
  stmt_link* curr;                   /*!< Pointer to current statement link */
  stmt_link* last;                   /*!< Two-way pointer to next/previous statement link */
};

struct exp_link_s {
  expression* exp;                   /*!< Pointer to expression */
  exp_link*   next;                  /*!< Pointer to next expression element in list */
};

struct stmt_link_s {
  statement* stmt;                   /*!< Pointer to statement */
  stmt_link* ptr;                    /*!< Pointer to next statement element in list */
};

struct stmt_loop_link_s {
  statement*      stmt;              /*!< Pointer to last statement in loop */
  int             id;                /*!< ID of next statement after last */
  stmt_loop_link* next;              /*!< Pointer to next statement in stack */
};

struct statistic_s {
  float line_total;                  /*!< Total number of lines parsed */
  int   line_hit;                    /*!< Number of lines executed during simulation */
  float tog_total;                   /*!< Total number of bits to toggle */
  int   tog01_hit;                   /*!< Number of bits toggling from 0 to 1 */
  int   tog10_hit;                   /*!< Number of bits toggling from 1 to 0 */
  float comb_total;                  /*!< Total number of expression combinations */
  int   comb_hit;                    /*!< Number of logic combinations hit */
  float state_total;                 /*!< Total number of FSM states */
  int   state_hit;                   /*!< Number of FSM states reached */
  float arc_total;                   /*!< Total number of FSM arcs */
  int   arc_hit;                     /*!< Number of FSM arcs traversed */
  int   race_total;                  /*!< Total number of race conditions found */
  int   rtype_total[RACE_TYPE_NUM];  /*!< Total number of each race condition type found */
};

struct mod_parm_s {
  char*        name;                 /*!< Full hierarchical name of associated parameter */
  expression*  expr;                 /*!< Expression tree containing value of parameter */
  unsigned int suppl;                /*!< Supplemental field containing type and order number */
  exp_link*    exp_head;             /*!< Pointer to head of expression list for dependents */
  exp_link*    exp_tail;             /*!< Pointer to tail of expression list for dependents */
  vsignal*     sig;                  /*!< Pointer to associated signal (if one is available) */
  mod_parm*    next;                 /*!< Pointer to next module parameter in list */
};

struct inst_parm_s {
  char*        name;                 /*!< Name of associated parameter (no hierarchy) */
  vector*      value;                /*!< Pointer to value of instance parameter */
  mod_parm*    mparm;                /*!< Pointer to base module parameter */
  inst_parm*   next;                 /*!< Pointer to next instance parameter in list */
};

struct fsm_arc_s {
  expression* from_state;            /*!< Pointer to expression that represents the state we are transferring from */
  expression* to_state;              /*!< Pointer to expression that represents the state we are transferring to */
  fsm_arc*    next;                  /*!< Pointer to next fsm_arc in this list */
};

struct fsm_link_s {
  fsm*      table;                   /*!< Pointer to FSM structure to store */
  fsm_link* next;                    /*!< Pointer to next element in fsm_link list */
};

struct race_blk_s {
  int       start_line;              /*!< Starting line number of statement block that was found to be a race condition */
  int       end_line;                /*!< Ending line number of statement block that was found to be a race condition */
  int       reason;                  /*!< Numerical reason for why this statement block was found to be a race condition */
  race_blk* next;                    /*!< Pointer to next race block in list */
};

struct func_unit_s {
  int         type;                  /*!< Specifies the type of functional unit this structure represents.
                                          Legal values are defined in \ref func_unit_types */
  char*       name;                  /*!< Functional unit name */
  char*       filename;              /*!< File name where functional unit exists */
  int         start_line;            /*!< Starting line number of functional unit in its file */
  int         end_line;              /*!< Ending line number of functional unit in its file */
  statistic*  stat;                  /*!< Pointer to functional unit coverage statistics structure */
  sig_link*   sig_head;              /*!< Head pointer to list of signals in this functional unit */
  sig_link*   sig_tail;              /*!< Tail pointer to list of signals in this functional unit */
  exp_link*   exp_head;              /*!< Head pointer to list of expressions in this functional unit */
  exp_link*   exp_tail;              /*!< Tail pointer to list of expressions in this functional unit */
  stmt_link*  stmt_head;             /*!< Head pointer to list of statements in this functional unit */
  stmt_link*  stmt_tail;             /*!< Tail pointer to list of statements in this functional unit */
  fsm_link*   fsm_head;              /*!< Head pointer to list of FSMs in this functional unit */
  fsm_link*   fsm_tail;              /*!< Tail pointer to list of FSMs in this functional unit */
  race_blk*   race_head;             /*!< Head pointer to list of race condition blocks in this functional unit if we are a module */
  race_blk*   race_tail;             /*!< Tail pointer to list of race condition blocks in this functional unit if we are a module */
  mod_parm*   param_head;            /*!< Head pointer to list of parameters in this functional unit if we are a module */
  mod_parm*   param_tail;            /*!< Tail pointer to list of parameters in this functional unit if we are a module */
  funit_link* tf_head;               /*!< Head pointer to list of task/function functional units if we are a module */
  funit_link* tf_tail;               /*!< Tail pointer to list of task/function functional units if we are a module */
 };

struct funit_link_s {
  func_unit*  funit;                 /*!< Pointer to functional unit in list */
  funit_link* next;                  /*!< Next functional unit link in list */
};

struct sym_sig_s {
  vsignal* sig;                      /*!< Pointer to signal that this symtable entry refers to */
  int      msb;                      /*!< Most significant bit of value to set */
  int      lsb;                      /*!< Least significant bit of value to set */
  sym_sig* next;                     /*!< Pointer to next sym_sig link in list */
};

struct symtable_s {
  sym_sig*  sig_head;                /*!< Pointer to head of sym_sig list */
  sym_sig*  sig_tail;                /*!< Pointer to tail of sym_sig list */
  char*     value;                   /*!< String representation of last current value */
  int       size;                    /*!< Number of bytes allowed storage for value */
  symtable* table[256];              /*!< Array of symbol tables for next level */
};

struct static_expr_s {
  expression* exp;                   /*!< Specifies if static value is an expression */
  int         num;                   /*!< Specifies if static value is a numeric value */
};

struct vector_width_s {
  static_expr* left;                 /*!< Specifies left bit value of bit range */
  static_expr* right;                /*!< Specifies right bit value of bit range */
};

struct exp_bind_s {
  int         type;                  /*!< Specifies if name refers to a signal (0), function (FUNIT_FUNCTION) or task (FUNIT_TASK) */
  char*       name;                  /*!< Name of Verilog scoped signal/functional unit to bind */
  int         clear_assigned;        /*!< If >0, clears the signal assigned supplemental field without binding */
  expression* exp;                   /*!< Expression to bind. */
  expression* fsm;                   /*!< FSM expression to create value for when this expression is bound */
  func_unit*  funit;                 /*!< Pointer to functional unit containing expression */
  exp_bind*   next;                  /*!< Pointer to next binding in list */
};

struct case_stmt_s {
  expression*     expr;              /*!< Pointer to case equality expression */
  statement*      stmt;              /*!< Pointer to first statement in case statement */
  int             line;              /*!< Line number of case statement */
  case_statement* prev;              /*!< Pointer to previous case statement in list */
};

struct funit_inst_s {
  char*       name;                  /*!< Instance name of this functional unit instance */
  func_unit*  funit;                 /*!< Pointer to functional unit this instance represents */
  statistic*  stat;                  /*!< Pointer to statistic holder */
  inst_parm*  param_head;            /*!< Head pointer to list of parameter overrides in this functional unit if it is a module */
  inst_parm*  param_tail;            /*!< Tail pointer to list of parameter overrides in this functional unit if it is a module */
  funit_inst* parent;                /*!< Pointer to parent instance -- used for convenience only */
  funit_inst* child_head;            /*!< Pointer to head of child list */
  funit_inst* child_tail;            /*!< Pointer to tail of child list */
  funit_inst* next;                  /*!< Pointer to next child in parents list */
};

struct tnode_s {
  char*  name;                       /*!< Key value for tree node */
  char*  value;                      /*!< Value of node */
  tnode* left;                       /*!< Pointer to left child node */
  tnode* right;                      /*!< Pointer to right child node */
  tnode* up;                         /*!< Pointer to parent node */
};

#ifdef HAVE_SYS_TIMES_H
struct timer_s {
  struct tms start;                  /*!< Contains start time of a particular timer */
  clock_t    total;                  /*!< Contains the total amount of user time accrued for this timer */
};
#endif

struct fsm_var_s {
  char*       funit;                 /*!< Name of functional unit containing FSM variable */
  char*       name;                  /*!< Name associated with this FSM variable */
  expression* ivar;                  /*!< Pointer to input state expression */
  expression* ovar;                  /*!< Pointer to output state expression */
  vsignal*    iexp;                  /*!< Pointer to input signal matching ovar name */
  fsm*        table;                 /*!< Pointer to FSM containing signal from ovar */
  fsm_var*    next;                  /*!< Pointer to next fsm_var element in list */
};

struct fv_bind_s {
  char*       sig_name;              /*!< Name of signal to bind to expression */
  expression* expr;                  /*!< Pointer to expression to bind to signal */
  char*       funit_name;            /*!< Name of functional unit to find sig_name and expression */
  statement*  stmt;                  /*!< Pointer to statement which contains root of expr */
  fv_bind*    next;                  /*!< Pointer to next FSM variable bind element in list */
};

struct attr_param_s {
  char*       name;                  /*!< Name of attribute parameter identifier */
  expression* expr;                  /*!< Pointer to expression assigned to the attribute parameter identifier */
  int         index;                 /*!< Index position in the array that this parameter is located at */
  attr_param* next;                  /*!< Pointer to next attribute parameter in list */
  attr_param* prev;                  /*!< Pointer to previous attribute parameter in list */
};

struct stmt_blk_s {
  statement* stmt;                   /*!< Pointer to top-level statement in statement tree that this signal is first found in */
  bool       remove;                 /*!< Specifies if this statement block should be removed after checking is complete */
  bool       seq;                    /*!< If true, this statement block is considered to include sequential logic */
  bool       cmb;                    /*!< If true, this statement block is considered to include combinational logic */
  bool       bassign;                /*!< If true, at least one expression in statement block is a blocking assignment */
  bool       nassign;                /*!< If true, at least one expression in statement block is a non-blocking assignment */
};


/*
 $Log$
 Revision 1.135  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.134  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.133  2005/11/22 16:46:27  phase1geo
 Fixed bug with clearing the assigned bit in the binding phase.  Full regression
 now runs cleanly.

 Revision 1.132  2005/11/22 05:30:33  phase1geo
 Updates to regression suite for clearing the assigned bit when a statement
 block is removed from coverage consideration and it is assigning that signal.
 This is not fully working at this point.

 Revision 1.131  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.130  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.129  2005/11/17 23:35:16  phase1geo
 Blocking assignment is now working properly along with support for event expressions
 (currently only the original PEDGE, NEDGE, AEDGE and DELAY are supported but more
 can now follow).  Added new race4 diagnostic to verify that a register cannot be
 assigned from more than one location -- this works.  Regression fails at this point.

 Revision 1.128  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.127  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.126  2005/11/11 23:29:12  phase1geo
 Checkpointing some work in progress.  This will cause compile errors.  In
 the process of moving db read expression signal binding from vsignal output to
 expression output so that we can just call the binder in the expression_db_read
 function.

 Revision 1.125  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.

 Revision 1.124  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.123  2005/05/09 03:08:34  phase1geo
 Intermediate checkin for VPI changes.  Also contains parser fix which should
 be branch applied to the latest stable and development versions.

 Revision 1.122  2005/02/08 23:18:23  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.121  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.120  2005/02/04 23:55:48  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.119  2005/02/01 05:11:18  phase1geo
 Updates to race condition checker to find blocking/non-blocking assignments in
 statement block.  Regression still runs clean.

 Revision 1.118  2005/01/27 13:33:49  phase1geo
 Added code to calculate if statement block is sequential, combinational, both
 or none.

 Revision 1.117  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.116  2005/01/10 02:59:29  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.115  2005/01/07 23:00:09  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.114  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.113  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.112  2005/01/06 14:32:24  phase1geo
 Starting to make updates to supplemental fields.  Work is in progress.

 Revision 1.111  2005/01/04 14:37:00  phase1geo
 New changes for race condition checking.  Things are uncompilable at this
 point.

 Revision 1.110  2004/12/17 14:27:46  phase1geo
 More code added to race condition checker.  This is in an unusable state at
 this time.

 Revision 1.109  2004/12/16 13:52:58  phase1geo
 Starting to add support for race-condition detection and handling.

 Revision 1.108  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.107  2004/04/19 04:54:55  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.106  2004/04/17 14:07:55  phase1geo
 Adding replace and merge options to file menu.

 Revision 1.105  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.104  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.103  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.102  2004/01/30 06:04:43  phase1geo
 More report output format tweaks.  Adjusted lines and spaces to make things
 look more organized.  Still some more to go.  Regression will fail at this
 point.

 Revision 1.101  2004/01/25 03:41:48  phase1geo
 Fixes bugs in summary information not matching verbose information.  Also fixes
 bugs where instances were output when no logic was missing, where instance
 children were missing but not output.  Changed code to output summary
 information on a per instance basis (where children instances are not merged
 into parent instance summary information).  Updated regressions as a result.
 Updates to user documentation (though this is not complete at this time).

 Revision 1.100  2004/01/21 22:26:56  phase1geo
 Changed default CDD file name from "cov.db" to "cov.cdd".  Changed instance
 statistic gathering from a child merging algorithm to just calculating
 instance coverage for the individual instances.  Updated full regression for
 this change and updated VCS regression for all past changes of this release.

 Revision 1.99  2004/01/16 23:05:00  phase1geo
 Removing SET bit from being written to CDD files.  This value is meaningless
 after scoring has completed and sometimes causes miscompares when simulators
 change.  Updated regression for this change.  This change should also be made
 to stable release.

 Revision 1.98  2004/01/09 21:57:55  phase1geo
 Fixing bug in combinational logic report generator where partially covered
 expressions were being logged in summary report but not displayed when verbose
 output was needed.  Updated regression for this change.  Also added new multi_exp
 diagnostics to verify multiple expression combination logic expressions in report
 output.  Full regression passes at this point.

 Revision 1.97  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.96  2004/01/02 22:11:03  phase1geo
 Updating regression for latest batch of changes.  Full regression now passes.
 Fixed bug with event or operation in report command.

 Revision 1.95  2003/12/30 23:02:28  phase1geo
 Contains rest of fixes for multi-expression combinational logic report output.
 Full regression fails currently.

 Revision 1.94  2003/12/18 18:40:23  phase1geo
 Increasing detailed depth from 1 to 2 and making detail depth somewhat
 programmable.

 Revision 1.93  2003/12/16 23:22:07  phase1geo
 Adding initial code for line width specification for report output.

 Revision 1.92  2003/12/01 23:27:15  phase1geo
 Adding code for retrieving line summary module coverage information for
 GUI.

 Revision 1.91  2003/11/30 05:46:45  phase1geo
 Adding IF report outputting capability.  Updated always9 diagnostic for these
 changes and updated rest of regression CDD files accordingly.

 Revision 1.90  2003/11/29 06:55:48  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.89  2003/11/15 04:21:57  phase1geo
 Fixing syntax errors found in Doxygen and GCC compiler.

 Revision 1.88  2003/11/05 05:22:56  phase1geo
 Final fix for bug 835366.  Full regression passes once again.

 Revision 1.87  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.86  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.85  2003/10/17 02:12:38  phase1geo
 Adding CDD version information to info line of CDD file.  Updating regression
 for this change.

 Revision 1.84  2003/10/13 03:56:29  phase1geo
 Fixing some problems with new FSM code.  Not quite there yet.

 Revision 1.83  2003/10/11 05:15:07  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.82  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.81  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.80  2003/09/22 19:42:31  phase1geo
 Adding print_output WARNING_WRAP and FATAL_WRAP lines to allow multi-line
 error output to be properly formatted to the output stream.

 Revision 1.79  2003/09/13 19:53:59  phase1geo
 Adding correct way of calculating state and state transition totals.  Modifying
 FSM summary reporting to reflect these changes.  Also added function documentation
 that was missing from last submission.

 Revision 1.78  2003/09/13 02:59:34  phase1geo
 Fixing bugs in arc.c created by extending entry supplemental field to 5 bits
 from 3 bits.  Additional two bits added for calculating unique states.

 Revision 1.77  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.76  2003/08/26 21:53:23  phase1geo
 Added database read/write functions and fixed problems with other arc functions.

 Revision 1.75  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.74  2003/08/22 00:23:59  phase1geo
 Adding development documentation comment to defines.h for new sym_sig structure.

 Revision 1.73  2003/08/21 21:57:30  phase1geo
 Fixing bug with certain flavors of VCD files that alias signals that have differing
 MSBs and LSBs.  This takes care of the rest of the bugs for the 0.2 stable release.

 Revision 1.72  2003/08/15 20:02:08  phase1geo
 Added check for sys/times.h file for new code additions.

 Revision 1.71  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.70  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.69  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.68  2003/02/27 03:41:56  phase1geo
 More development documentation updates.

 Revision 1.67  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.66  2003/02/12 14:56:22  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

 Revision 1.65  2003/02/07 23:12:30  phase1geo
 Optimizing db_add_statement function to avoid memory errors.  Adding check
 for -i option to avoid user error.

 Revision 1.64  2003/01/04 09:25:15  phase1geo
 Fixing file search algorithm to fix bug where unexpected module that was
 ignored cannot be found.  Added instance7.v diagnostic to verify appropriate
 handling of this problem.  Added tree.c and tree.h and removed define_t
 structure in lexer.

 Revision 1.63  2003/01/03 05:52:01  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.62  2002/12/07 17:46:52  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.61  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.60  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.59  2002/11/24 14:38:12  phase1geo
 Updating more regression CDD files for bug fixes.  Fixing bugs where combinational
 expressions were counted more than once.  Adding new diagnostics to regression
 suite.

 Revision 1.58  2002/11/23 21:27:25  phase1geo
 Fixing bug with combinational logic being output when unmeasurable.

 Revision 1.57  2002/11/23 16:10:46  phase1geo
 Updating changelog and development documentation to include FSM description
 (this is a brainstorm on how to handle FSMs when we get to this point).  Fixed
 bug with code underlining function in handling parameter in reports.  Fixing bugs
 with MBIT/SBIT handling (this is not verified to be completely correct yet).

 Revision 1.56  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.55  2002/10/31 23:13:36  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.54  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.53  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.52  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.51  2002/10/23 03:39:06  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.50  2002/10/13 19:20:42  phase1geo
 Added -T option to score command for properly handling min:typ:max delay expressions.
 Updated documentation for -i and -T options to score command and added additional
 diagnostic to test instance handling.

 Revision 1.49  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.48  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.47  2002/09/26 22:58:46  phase1geo
 Fixing syntax error.

 Revision 1.46  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.45  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.44  2002/09/21 04:11:32  phase1geo
 Completed phase 1 for adding in parameter support.  Main code is written
 that will create an instance parameter from a given module parameter in
 its entirety.  The next step will be to complete the module parameter
 creation code all the way to the parser.  Regression still passes and
 everything compiles at this point.

 Revision 1.43  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.42  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.41  2002/09/06 03:05:28  phase1geo
 Some ideas about handling parameters have been added to these files.  Added
 "Special Thanks" section in User's Guide for acknowledgements to people
 helping in project.

 Revision 1.40  2002/08/27 11:53:16  phase1geo
 Adding more code for parameter support.  Moving parameters from being a
 part of modules to being a part of instances and calling the expression
 operation function in the parameter add functions.

 Revision 1.39  2002/08/26 12:57:03  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.38  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.37  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.36  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.35  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.34  2002/07/18 02:33:23  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.33  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.32  2002/07/11 16:59:10  phase1geo
 Added release information to NEWS for upcoming release.

 Revision 1.31  2002/07/10 13:15:57  phase1geo
 Adding case1.1.v Verilog diagnostic to check default case statement.  There
 were reporting problems related to this.  Report problems have been fixed and
 full regression passes.

 Revision 1.30  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.29  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.28  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.27  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.26  2002/07/05 05:00:13  phase1geo
 Removing CASE, CASEX, and CASEZ from line and combinational logic results.

 Revision 1.25  2002/07/05 04:12:46  phase1geo
 Correcting case, casex and casez equality calculation to conform to correct
 equality check for each case type.  Verified that case statements work correctly
 at this point.  Added diagnostics to verify case statements.

 Revision 1.24  2002/07/04 23:10:12  phase1geo
 Added proper support for case, casex, and casez statements in score command.
 Report command still incorrect for these statement types.

 Revision 1.23  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.22  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.21  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.20  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.19  2002/06/30 22:23:20  phase1geo
 Working on fixing looping in parser.  Statement connector needs to be revamped.

 Revision 1.18  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.17  2002/06/27 21:18:48  phase1geo
 Fixing report Verilog output.  simple.v verilog diagnostic now passes.

 Revision 1.16  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.15  2002/06/26 04:59:50  phase1geo
 Adding initial support for delays.  Support is not yet complete and is
 currently untested.

 Revision 1.14  2002/06/23 21:18:21  phase1geo
 Added appropriate statement support in parser.  All parts should be in place
 and ready to start testing.

 Revision 1.13  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.11  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.10  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.9  2002/05/02 03:27:42  phase1geo
 Initial creation of statement structure and manipulation files.  Internals are
 still in a chaotic state.

 Revision 1.8  2002/04/30 05:04:25  phase1geo
 Added initial go-round of adding statement handling to parser.  Added simple
 Verilog test to check correct statement handling.  At this point there is a
 bug in the expression write function (we need to display statement trees in
 the proper order since they are unlike normal expression trees.)
*/

#endif

