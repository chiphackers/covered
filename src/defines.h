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

/*!
 Specifies current version of the Covered utility.
*/
#define COVERED_VERSION    VERSION

/*!
 This contains the header information specified when executing this tool.
*/
#define COVERED_HEADER     "\nCovered %s -- Verilog Code Coverage Utility\nWritten by Trevor Williams  (trevorw@charter.net)\nFreely distributable under the GPL license\n", COVERED_VERSION

/*!
 Default database filename if not specified on command-line.
*/
#define DFLT_OUTPUT_DB     "cov.db"

/*!
 Determine size of integer in bits.
*/
#define INTEGER_WIDTH	   (SIZEOF_INT * 8)

/*!
 Length of user_msg global string (used for inputs to snprintf calls).
*/
#define USER_MSG_LENGTH  4096

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
#define DB_TYPE_MODULE       3

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
#define REPORT_DETAILED      0x1

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
 \addtogroup vector_defs Vector-specific defines.

 The following defines are used for vector-based operations.

 @{
*/

/*!
 Command bit for knowing if vector is set.
*/
#define VECTOR_SET_BIT	     0x1

/*!
 Command bit for knowing if vector is a static value.
*/
#define VECTOR_STATIC_BIT    0x2

/*!
 Macro for determining if VECTOR_SET_BIT is set.
*/
#define IS_VECTOR_SET(x)     (x & VECTOR_SET_BIT)

/*!
 Macro for determining if VECTOR_STATIC_BIT is set.
*/
#define IS_VECTOR_STATIC(x)  (x & VECTOR_STATIC_BIT)

/*!
 Macro for getting the current nibble size of the specified vector.  x is the
 width of the vector.
*/
#define VECTOR_SIZE(x)       (((x % 4) == 0) ? (x / 4) : ((x / 4) + 1))

/*!
 Used for merging two vector nibbles from two vectors.  Both vector nibble
 fields are ANDed with this mask and ORed together to perform the merge.  
 Fields that are merged are:
 - TOGGLE0->1
 - TOGGLE1->0
 - SET
 - TYPE
 - FALSE
 - TRUE
*/
#define VECTOR_MERGE_MASK    0xff3fff00

/*! @} */

/*!
 \addtogroup expr_suppl Expression supplemental field defines and macros.

 @{
*/

/*!
 Least-significant bit position of expression supplemental field indicating the
 expression's operation type.  The type is 6-bits wide.
*/
#define SUPPL_LSB_OP                0

/*!
 Least-significant bit position of expression supplemental field indicating that the
 children of this expression have been swapped.  The swapping of the positions is
 performed by the score command (for multi-bit selects) and this bit indicates to the
 report code to swap them back when displaying them in a report.
*/
#define SUPPL_LSB_SWAPPED           6

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression is a root expression.  Traversing to the parent pointer will take you to
 a statement type.
*/
#define SUPPL_LSB_ROOT              7

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has been executed in the queue during the lifetime of the simulation.
*/
#define SUPPL_LSB_EXECUTED          8

/*!
 Least-significant bit position of expression supplemental field indicating the
 statement which this expression belongs is a head statement (only valid for root
 expressions -- parent expression == NULL).
*/
#define SUPPL_LSB_STMT_HEAD         9

/*!
 Least-significant bit position of expression supplemental field indicating the
 statement which this expression belongs should write itself to the CDD and not
 continue to traverse its next_true and next_false pointers.
*/
#define SUPPL_LSB_STMT_STOP         10

/*!
 Least-significant bit position of expression supplemental field indicating the
 statement which this expression belongs is part of a continuous assignment.  As such,
 stop simulating this statement tree after this expression tree is evaluated.
*/
#define SUPPL_LSB_STMT_CONTINUOUS   11

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has evaluated to a value of FALSE during the lifetime of the simulation.
*/
#define SUPPL_LSB_FALSE             12

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has evaluated to a value of TRUE during the lifetime of the simulation.
*/
#define SUPPL_LSB_TRUE              13

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has its left child expression in a changed state during this timestamp.
*/
#define SUPPL_LSB_LEFT_CHANGED      14

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has its right child expression in a changed state during this timestamp.
*/
#define SUPPL_LSB_RIGHT_CHANGED     15

/*!
 Least-significant bit position of expression supplemental field indicating that the
 value of the left child expression evaluated to FALSE and the right child expression
 evaluated to FALSE.
*/
#define SUPPL_LSB_EVAL_00           16

/*!
 Least-significant bit position of expression supplemental field indicating that the
 value of the left child expression evaluated to FALSE and the right child expression
 evaluated to TRUE.
*/
#define SUPPL_LSB_EVAL_01           17

/*!
 Least-significant bit position of expression supplemental field indicating that the
 value of the left child expression evaluated to TRUE and the right child expression
 evaluated to FALSE.
*/
#define SUPPL_LSB_EVAL_10           18

/*!
 Least-significant bit position of expression supplemental field indicating that the
 value of the left child expression evaluated to TRUE and the right child expression
 evaluated to TRUE.
*/
#define SUPPL_LSB_EVAL_11           19

/*!
 Least-significant bit position of expression supplemental field indicating that the
 value of the current expression currently set to TRUE (temporary value).
*/
#define SUPPL_LSB_EVAL_T            20

/*!
 Least-significant bit position of expression supplemental field indicating that the
 value of the current expression currently set to FALSE (temporary value).
*/
#define SUPPL_LSB_EVAL_F            21

/*!
 Least-significant bit position of expression supplemental field indicating that the
 current expression has been previously counted for combinational coverage.  Only set
 by report command (therefore this bit will always be a zero when written to CDD file.
*/
#define SUPPL_LSB_COMB_CNTD         22

/*!
 Temporary bit value used by the score command but not displayed to the CDD file.  When
 this bit is set to a one, it indicates to the statement_connect function that this
 statement and all children statements do not need to be connected to another statement.
*/
#define SUPPL_LSB_STMT_CONNECTED    23

/*!
 Temporary bit value used by the score command but not displayed to the CDD file.  When
 this bit is set to a one, it indicates to the db_add_statement function that this
 statement and all children statements have already been added to the module statement
 list and should not be added again.
*/
#define SUPPL_LSB_STMT_ADDED        24

/*!
 Used for merging two supplemental fields from two expressions.  Both expression
 supplemental fields are ANDed with this mask and ORed together to perform the
 merge.  Fields that are merged are:
 - OPERATION
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
#define SUPPL_MERGE_MASK            0xfe7fffff

/*!
 Returns a value of 1 if the specified supplemental value has the SWAPPED
 bit set indicating that the children of the current expression were
 swapped positions during the scoring phase.
*/
#define SUPPL_WAS_SWAPPED(x)        ((x >> SUPPL_LSB_SWAPPED) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental value has the ROOT bit
 set indicating that the current expression is the root expression of its
 expression tree.
*/
#define SUPPL_IS_ROOT(x)            ((x >> SUPPL_LSB_ROOT) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental value has the executed
 bit set; otherwise, returns a value of 0 to indicate whether the
 corresponding expression was executed during simulation or not.
*/
#define SUPPL_WAS_EXECUTED(x)       ((x >> SUPPL_LSB_EXECUTED) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 whose associated statement is a head statement.
*/
#define SUPPL_IS_STMT_HEAD(x)       ((x >> SUPPL_LSB_STMT_HEAD) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 whose associated statement is a stop (for writing purposes).
*/
#define SUPPL_IS_STMT_STOP(x)       ((x >> SUPPL_LSB_STMT_STOP) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 whose associated statement is a continous assignment.
*/
#define SUPPL_IS_STMT_CONTINUOUS(x) ((x >> SUPPL_LSB_STMT_CONTINUOUS) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 who was just evaluated to TRUE.
*/
#define SUPPL_IS_TRUE(x)            ((x >> SUPPL_LSB_EVAL_T) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 who was just evaluated to FALSE.
*/
#define SUPPL_IS_FALSE(x)           ((x >> SUPPL_LSB_EVAL_F) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 that has evaluated to a value of TRUE (1) during simulation.
*/
#define SUPPL_WAS_TRUE(x)           ((x >> SUPPL_LSB_TRUE) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 that has evaluated to a value of FALSE (0) during simulation.
*/
#define SUPPL_WAS_FALSE(x)          ((x >> SUPPL_LSB_FALSE) & 0x1)

/*!
 Returns a value of 1 if the left child expression was changed during this
 timestamp.
*/
#define SUPPL_IS_LEFT_CHANGED(x)    ((x >> SUPPL_LSB_LEFT_CHANGED) & 0x1)

/*!
 Returns a value of 1 if the right child expression was changed during this
 timestamp.
*/
#define SUPPL_IS_RIGHT_CHANGED(x)   ((x >> SUPPL_LSB_RIGHT_CHANGED) & 0x1)

/*!
 Returns a value of 1 if the specified expression has already been counted
 for combinational coverage.
*/
#define SUPPL_WAS_COMB_COUNTED(x)   ((x >> SUPPL_LSB_COMB_CNTD) & 0x1)

/*!
 Returns the specified expression's operation.
*/
#define SUPPL_OP(x)                 ((x >> SUPPL_LSB_OP) & 0x3f)

/*! @} */
     
/*!
 \addtogroup read_modes Modes for reading database file

 Specify how to integrate read data from database file into memory structures.

 @{
*/

/*!
 When new module is read from database file, it is placed in the module list and
 is placed in the correct hierarchical position in the instance tree.  Used when
 performing a MERGE command or reading after parsing source files.
*/
#define READ_MODE_MERGE_NO_MERGE          0

/*!
 When new module is read from database file, it is placed in the module list and
 is placed in the correct hierarchical position in the instance tree.  Used when
 performing a REPORT command.
*/
#define READ_MODE_REPORT_NO_MERGE         1

/*!
 When module is completely read in (including module, signals, expressions), the
 module is looked up in the current instance tree.  If the instance exists, the
 module is merged with the instance's module; otherwise, we are attempting to
 merge two databases that were created from different designs.
*/
#define READ_MODE_MERGE_INST_MERGE        2

/*!
 When module is completely read in (including module, signals, expressions), the
 module is looked up in the module list.  If the module is found, it is merged
 with the existing module; otherwise, it is added to the module list.
*/
#define READ_MODE_REPORT_MOD_MERGE        3

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

/*! @} */

/*!
 Returns a value of 1 if the specified expression is considered to be measurable.
*/
#define EXPR_IS_MEASURABLE(x)      (((SUPPL_OP( x->suppl ) != EXP_OP_STATIC) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_LAST) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_LIST) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_COND_SEL) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_CASE) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_CASEX) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_CASEZ) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_DEFAULT) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_PARAM) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_PARAM_SBIT) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_PARAM_MBIT) && \
                                     (SUPPL_OP( x->suppl ) != EXP_OP_DELAY) && \
                                     !((SUPPL_IS_ROOT( x->suppl ) == 0) && \
                                       ((SUPPL_OP( x->suppl ) == EXP_OP_SIG) || \
                                        (SUPPL_OP( x->suppl ) == EXP_OP_SBIT_SEL) || \
                                        (SUPPL_OP( x->suppl ) == EXP_OP_MBIT_SEL)) && \
                                       (SUPPL_OP( x->parent->expr->suppl ) != EXP_OP_COND))) ? 1 : 0)

/*!
 Returns a value of TRUE if the specified expression is a STATIC, PARAM, PARAM_SBIT or PARAM_MBIT
 operation type.
*/
#define EXPR_IS_STATIC(x)        ((SUPPL_OP( x->suppl ) == EXP_OP_STATIC)     || \
                                  (SUPPL_OP( x->suppl ) == EXP_OP_PARAM)      || \
                                  (SUPPL_OP( x->suppl ) == EXP_OP_PARAM_SBIT) || \
                                  (SUPPL_OP( x->suppl ) == EXP_OP_PARAM_MBIT))

/*!
 Returns a value of 1 if the specified expression was measurable for combinational 
 coverage but not fully covered during simulation.
*/
#define EXPR_COMB_MISSED(x)        (EXPR_IS_MEASURABLE( x ) & \
                                    ~expression_is_static_only( x ) & \
                                    (~SUPPL_WAS_TRUE( x->suppl ) | ~SUPPL_WAS_FALSE( x->suppl )))

/*!
 \addtogroup op_tables

 The following describe the operation table values for AND, OR, XOR, NAND, NOR and
 NXOR operations.

 @{
*/
 
/*                        00  01  02  03  10  11  12  13  20  21  22  23  30  31  32  33 */
#define AND_OP_TABLE      0,  0,  0,  0,  0,  1,  2,  2,  0,  2,  2,  2,  0,  2,  2,  2
#define OR_OP_TABLE       0,  1,  2,  2,  1,  1,  1,  1,  2,  1,  2,  2,  2,  1,  2,  2
#define XOR_OP_TABLE      0,  1,  2,  2,  1,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2
#define NAND_OP_TABLE     1,  1,  1,  1,  1,  0,  2,  2,  1,  2,  2,  2,  1,  2,  2,  2
#define NOR_OP_TABLE      1,  0,  2,  2,  0,  0,  0,  0,  2,  0,  2,  2,  2,  0,  2,  2
#define NXOR_OP_TABLE     1,  0,  2,  2,  0,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2
#define ADD_OP_TABLE      0,  1,  10, 10, 1,  4,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10  

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
 Defines boolean variables used in most functions.
*/
typedef enum {
  FALSE,   /*!< Boolean false value */
  TRUE     /*!< Boolean true value  */
} bool;

#if SIZEOF_INT == 4

/*!
 A nibble is a 32-bit value that is subdivided into the following parts:
 <table>
   <tr> <td> <strong> Bits </strong> </td> <td> <strong> Field Description </strong> </td> </tr>
   <tr> <td> 7:0   </td> <td> Current 4-state value for bits 3-0 </td> </tr>
   <tr> <td> 11:8  </td> <td> Indicator if associated bit was toggled from 0->1 </td> </tr>
   <tr> <td> 15:12 </td> <td> Indicator if associated bit was toggled from 1->0 </td></tr>
   <tr> <td> 19:16 </td> <td> Indicator if associated bit has been previously assigned this timestep </td> </tr>
   <tr> <td> 21:20 </td> <td> Indicates original value type (HEXIDECIMAL, OCTAL, BINARY, DECIMAL) </td> </tr>
   <tr> <td> 23:22 </td> <td> General purpose bits </td> </tr>
   <tr> <td> 27:24 </td> <td> Indicator if associated bit was set to a value of 0 (FALSE) </td> </tr>
   <tr> <td> 31:28 </td> <td> Indicator if associated bit was set to a value of 1 (TRUE) </td> </tr>
 </table>
*/
typedef unsigned int nibble;

/*!
 A control is a 32-bit value that is subdivided into the following parts:
 <table>
   <tr> <td> <strong> Bits </strong> </td> <td> <strong> Field Description </strong> </td> </tr>
   <tr> <td> 5:0   </td> <td> See \ref SUPPL_LSB_OP </td> </tr>
   <tr> <td> 6     </td> <td> See \ref SUPPL_LSB_SWAPPED </td> </tr>
   <tr> <td> 7     </td> <td> See \ref SUPPL_LSB_ROOT </td> </tr>
   <tr> <td> 8     </td> <td> See \ref SUPPL_LSB_EXECUTED </td> </tr>
   <tr> <td> 9     </td> <td> See \ref SUPPL_LSB_STMT_HEAD </td> </tr>
   <tr> <td> 10    </td> <td> See \ref SUPPL_LSB_STMT_STOP </td> </tr>
   <tr> <td> 11    </td> <td> See \ref SUPPL_LSB_STMT_CONTINUOUS </td> </tr>
   <tr> <td> 12    </td> <td> See \ref SUPPL_LSB_FALSE </td> </tr>
   <tr> <td> 13    </td> <td> See \ref SUPPL_LSB_TRUE </td> </tr>
   <tr> <td> 14    </td> <td> See \ref SUPPL_LSB_LEFT_CHANGED </td> </tr>
   <tr> <td> 15    </td> <td> See \ref SUPPL_LSB_RIGHT_CHANGED </td> </tr>
   <tr> <td> 16    </td> <td> See \ref SUPPL_LSB_EVAL_00 </td> </tr>
   <tr> <td> 17    </td> <td> See \ref SUPPL_LSB_EVAL_01 </td> </tr>
   <tr> <td> 18    </td> <td> See \ref SUPPL_LSB_EVAL_10 </td> </tr>
   <tr> <td> 19    </td> <td> See \ref SUPPL_LSB_EVAL_11 </td> </tr>
   <tr> <td> 20    </td> <td> See \ref SUPPL_LSB_EVAL_T </td> </tr>
   <tr> <td> 21    </td> <td> See \ref SUPPL_LSB_EVAL_F </td> </tr>
   <tr> <td> 22    </td> <td> See \ref SUPPL_LSB_COMB_CNTD </td> </tr>
   <tr> <td> 23    </td> <td> See \ref SUPPL_LSB_STMT_CONNECTED </td> </tr>
   <tr> <td> 24    </td> <td> See \ref SUPPL_LSB_STMT_ADDED </td> </tr>
   <tr> <td> 31:25 </td> <td> Reserved </td> </tr>
 </table>
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
 Specifies an element in a linked list containing string values.  This data
 structure allows us to add new elements to the list without resizing, this
 optimizes performance with small amount of overhead.
*/
struct str_link_s;

/*!
 Renaming string link structure for convenience.
*/
typedef struct str_link_s str_link;

struct str_link_s {
  char*     str;     /*!< String to store                  */
  char      suppl;   /*!< 8-bit additional information     */
  str_link* next;    /*!< Pointer to next str_link element */
};

/*------------------------------------------------------------------------------*/
/*!
 Contains information for signal value.  This value is represented as
 a generic vector.  The vector.h/.c files contain the functions that
 manipulate this information.
*/
struct vector_s {
  int     width;     /*!< Bit width of this vector                 */
  int     lsb;       /*!< Least significant bit                    */
  nibble* value;     /*!< 4-state current value and toggle history */
};

/*!
 Renaming vector structure for convenience.
*/
typedef struct vector_s vector;

/*------------------------------------------------------------------------------*/
/*!
 Allows the parent pointer of an expression to point to either another expression
 or a statement.
*/
union expr_stmt_u;

/*!
 Renaming expression statement union for convenience.
*/
typedef union expr_stmt_u expr_stmt;

/*!
 An expression is defined to be a logical combination of signals/values.  Expressions may 
 contain subexpressions (which are expressions in and of themselves).  An measurable expression 
 may only evaluate to TRUE (1) or FALSE (0).  If the parent expression of this expression is 
 NULL, then this expression is considered a root expression.  The nibble suppl contains the
 run-time information for its expression.
*/
struct expression_s;
struct signal_s;

/*!
 Renaming expression structure for convenience.
*/
typedef struct expression_s expression;

/*!
 Renaming signal structure for convenience.
*/
typedef struct signal_s     signal;

struct expression_s {
  vector*     value;       /*!< Current value and toggle information of this expression        */
  control     suppl;       /*!< Vector containing supplemental information for this expression */
  int         id;          /*!< Specifies unique ID for this expression in the parent          */
  int         line;        /*!< Specified line in file that this expression is found on        */
  signal*     sig;         /*!< Pointer to signal.  If NULL then no signal is attached         */
  expr_stmt*  parent;      /*!< Parent expression/statement                                    */
  expression* right;       /*!< Pointer to expression on right                                 */
  expression* left;        /*!< Pointer to expression on left                                  */
};

/*------------------------------------------------------------------------------*/
/*!
 Linked list element that stores a signal.
*/
struct sig_link_s;

/*!
 Renaming signal link structure for convenience.
*/
typedef struct sig_link_s sig_link;

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
 Renaming statement structure for convenience.
*/
typedef struct statement_s statement;

struct statement_s {
  expression* exp;            /*!< Pointer to associated expression tree                        */
  sig_link*   wait_sig_head;  /*!< Pointer to head of wait event signal list                    */
  sig_link*   wait_sig_tail;  /*!< Pointer to tail of wait event signal list                    */
  statement*  next_true;      /*!< Pointer to next statement to run if expression tree non-zero */
  statement*  next_false;     /*!< Pointer to next statement to run if next_true not picked     */
};

/*!
 Renaming statement iterator structure for convenience.
*/
typedef struct stmt_iter_s stmt_iter;

/*------------------------------------------------------------------------------*/
/*!
 Expression link element.  Stores pointer to an expression.
*/
struct exp_link_s;

/*!
 Renaming expression link structure for convenience.
*/
typedef struct exp_link_s exp_link;

struct exp_link_s {
  expression* exp;   /*!< Pointer to expression                      */
  exp_link*   next;  /*!< Pointer to next expression element in list */
};

/*------------------------------------------------------------------------------*/
/*!
 Statement link element.  Stores pointer to a statement.
*/
struct stmt_link_s;

/*!
 Renaming statement link structure for convenience.
*/
typedef struct stmt_link_s stmt_link;

struct stmt_link_s {
  statement* stmt;   /*!< Pointer to statement                       */
  stmt_link* ptr;   /*!< Pointer to next statement element in list  */
};

/*------------------------------------------------------------------------------*/
/*!
 Statement link iterator.
*/
struct stmt_iter_s {
  stmt_link* curr;   /*!< Pointer to current statement link               */
  stmt_link* last;   /*!< Two-way pointer to next/previous statement link */
};

/*------------------------------------------------------------------------------*/
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
 Renaming statement loop link structure for convenience.
*/
typedef struct stmt_loop_link_s stmt_loop_link;

struct stmt_loop_link_s {
  statement*      stmt;     /* Pointer to last statement in loop  */
  int             id;       /* ID of next statement after last    */
  stmt_loop_link* next;     /* Pointer to next statement in stack */
};

/*------------------------------------------------------------------------------*/
struct fsm_s;

typedef struct fsm_s fsm;

/*!
 Stores all information needed to represent a signal.  If value of value element is non-zero at the
 end of the run, this signal has been simulated.
*/
struct signal_s {
  char*      name;      /*!< Full hierarchical name of signal in design       */
  vector*    value;     /*!< Pointer to vector value of this signal           */
  exp_link*  exp_head;  /*!< Head pointer to list of expressions              */
  exp_link*  exp_tail;  /*!< Tail pointer to list of expressions              */
  fsm*       table;     /*!< Pointer to FSM table associated with this signal */
};

/*------------------------------------------------------------------------------*/
struct sig_link_s {
  signal*   sig;   /*!< Pointer to signal in list                   */
  sig_link* next;  /*!< Pointer to next signal link element in list */
};

/*------------------------------------------------------------------------------*/
/*!
 Contains statistics for coverage results which is stored in a module instance.
 NOTE:  FSM WILL BE HANDLED AT A LATER TIME.
*/
struct statistic_s;

/*!
 Renaming statistic structure for convenience.
*/
typedef struct statistic_s statistic;

struct statistic_s {
  float line_total;    /*!< Total number of lines parsed               */
  int   line_hit;      /*!< Number of lines executed during simulation */
  float tog_total;     /*!< Total number of bits to toggle             */
  int   tog01_hit;     /*!< Number of bits toggling from 0 to 1        */
  int   tog10_hit;     /*!< Number of bits toggling from 1 to 0        */
  float comb_total;    /*!< Total number of expression combinations    */
  int   comb_hit;      /*!< Number of logic combinations hit           */
  float state_total;   /*!< Total number of FSM states                 */
  int   state_hit;     /*!< Number of FSM states reached               */
  float arc_total;     /*!< Total number of FSM arcs                   */
  int   arc_hit;       /*!< Number of FSM arcs traversed               */
};

/*------------------------------------------------------------------------------*/
/*!
 Structure containing parts for a module parameter definition.
*/
struct mod_parm_s;

/*!
 Renaming module parameter structure for convenience.
*/
typedef struct mod_parm_s mod_parm;

struct mod_parm_s {
  char*        name;     /*!< Full hierarchical name of associated parameter      */
  expression*  expr;     /*!< Expression tree containing value of parameter       */
  unsigned int suppl;    /*!< Supplemental field containing type and order number */
  exp_link*    exp_head; /*!< Pointer to head of expression list for dependents   */
  exp_link*    exp_tail; /*!< Pointer to tail of expression list for dependents   */
  signal*      sig;      /*!< Pointer to associated signal (if one is available)  */
  mod_parm*    next;     /*!< Pointer to next module parameter in list            */
};

/*------------------------------------------------------------------------------*/
/*!
 Structure containing parts for an instance parameter.
*/
struct inst_parm_s;

/*!
 Renaming instance parameter structure for convenience.
*/
typedef struct inst_parm_s inst_parm;

struct inst_parm_s {
  char*        name;     /*!< Name of associated parameter (no hierarchy)         */
  vector*      value;    /*!< Pointer to value of instance parameter              */
  mod_parm*    mparm;    /*!< Pointer to base module parameter                    */
  inst_parm*   next;     /*!< Pointer to next instance parameter in list          */
};

/*-------------------------------------------------------------------------------*/
struct fsm_arc_s;

typedef struct fsm_arc_s fsm_arc;

struct fsm_arc_s {
  expression* from_state;  /*!< Pointer to expression that represents the state we are transferring from */
  expression* to_state;    /*!< Pointer to expression that represents the state we are transferring to   */
  fsm_arc*    next;        /*!< Pointer to next fsm_arc in this list                                     */
};

/*-------------------------------------------------------------------------------*/
struct fsm_s {
  signal*  from_sig;  /*!< Pointer to from_state signal                                                 */
  signal*  to_sig;    /*!< Pointer to to_state signal                                                   */
  fsm_arc* arc_head;  /*!< Pointer to head of list of expression pairs that describe the valid FSM arcs */
  fsm_arc* arc_tail;  /*!< Pointer to tail of list of expression pairs that describe the valid FSM arcs */
  char*    table;     /*!< FSM arc traversal table                                                      */
};

/*-------------------------------------------------------------------------------*/
/*!
 Linked list element that stores an FSM structure.
*/
struct fsm_link_s;

/*!
 Renaming fsm_link structure for convenience.
*/
typedef struct fsm_link_s fsm_link;

struct fsm_link_s {
  fsm*      table;  /*!< Pointer to FSM structure to store        */
  fsm_link* next;   /*!< Pointer to next element in fsm_link list */
};

/*------------------------------------------------------------------------------*/
/*!
 Contains information for a Verilog module.  A module contains a list of signals within the
 module.
*/
struct module_s {
  char*      name;       /*!< Module name                                        */
  char*      filename;   /*!< File name where module exists                      */
  statistic* stat;       /*!< Pointer to module coverage statistics structure    */
  sig_link*  sig_head;   /*!< Head pointer to list of signals in this module     */
  sig_link*  sig_tail;   /*!< Tail pointer to list of signals in this module     */
  exp_link*  exp_head;   /*!< Head pointer to list of expressions in this module */
  exp_link*  exp_tail;   /*!< Tail pointer to list of expressions in this module */
  stmt_link* stmt_head;  /*!< Head pointer to list of statements in this module  */
  stmt_link* stmt_tail;  /*!< Tail pointer to list of statements in this module  */
  fsm_link*  fsm_head;   /*!< Head pointer to list of FSMs in this module        */
  fsm_link*  fsm_tail;   /*!< Tail pointer to list of FSMs in this module        */
  mod_parm*  param_head; /*!< Head pointer to list of parameters in this module  */
  mod_parm*  param_tail; /*!< Tail pointer to list of parameters in this module  */
};

/*!
 Renaming module structure for convenience.
*/
typedef struct module_s module;

/*------------------------------------------------------------------------------*/
/*!
 Linked list element that stores a module (no scope).
*/
struct mod_link_s;

/*!
 Renaming module link structure for convenience.
*/
typedef struct mod_link_s mod_link;
	
struct mod_link_s {
  module*    mod;   /*!< Pointer to module in list */
  mod_link* next;   /*!< Next module in list       */
};

/*------------------------------------------------------------------------------*/
/*!
 For each signal within a symtable entry, an independent MSB and LSB needs to be
 stored along with the signal pointer that it references to properly assign the
 VCD signal value to the appropriate signal.  This structure is setup to hold these
 three key pieces of information in a list-style data structure.
*/
struct sym_sig_s;

/*!
 Renaming symbol signal structure for convenience.
*/
typedef struct sym_sig_s sym_sig;

struct sym_sig_s {
  signal*  sig;   /*!< Pointer to signal that this symtable entry refers to */
  int      msb;   /*!< Most significant bit of value to set                 */
  int      lsb;   /*!< Least significant bit of value to set                */
  sym_sig* next;  /*!< Pointer to next sym_sig link in list                 */
};

/*------------------------------------------------------------------------------*/
/*!
 Stores symbol name of signal along with pointer to signal itself into a lookup table
*/
struct symtable_s;

/*!
 Renaming symbol table structure for convenience.
*/
typedef struct symtable_s symtable;

struct symtable_s {
  sym_sig*  sig_head;     /*!< Pointer to head of sym_sig list             */
  sym_sig*  sig_tail;     /*!< Pointer to tail of sym_sig list             */
  char*     value;        /*!< String representation of last current value */
  int       size;         /*!< Number of bytes allowed storage for value   */
  symtable* table[256];   /*!< Array of symbol tables for next level       */
};

/*------------------------------------------------------------------------------*/
/*!
 Specifies possible values for a static expression (constant value).
*/
struct static_expr_s {
  expression* exp;        /*!< Specifies if static value is an expression   */
  int         num;        /*!< Specifies if static value is a numeric value */
};

/*!
 Renaming static expression structure for convenience.
*/
typedef struct static_expr_s static_expr;

/*------------------------------------------------------------------------------*/
/*!
 Specifies bit range of a signal or expression.
*/
struct vector_width_s {
  static_expr* left;      /*!< Specifies left bit value of bit range  */
  static_expr* right;     /*!< Specifies right bit value of bit range */
};

/*!
 Renaming vector width structure for convenience.
*/
typedef struct vector_width_s vector_width;

/*------------------------------------------------------------------------------*/
/*!
 Binds a signal to an expression.
*/
struct sig_exp_bind_s;

/*!
 Renaming signal/expression binding structure for convenience.
*/
typedef struct sig_exp_bind_s sig_exp_bind;

struct sig_exp_bind_s {
  char*         sig_name;  /*!< Name of Verilog scoped signal           */
  expression*   exp;       /*!< Expression to bind.                     */
  module*       mod;       /*!< Pointer to module containing expression */
  sig_exp_bind* next;      /*!< Pointer to next binding in list         */
};

/*------------------------------------------------------------------------------*/
/*!
 Binds an expression to a statement.  This is used when constructing a case
 structure.
*/
struct case_stmt_s;

/*!
 Renaming case statement structure for convenience.
*/
typedef struct case_stmt_s case_statement;

struct case_stmt_s {
  expression*     expr;    /*!< Pointer to case equality expression          */
  statement*      stmt;    /*!< Pointer to first statement in case statement */
  int             line;    /*!< Line number of case statement                */
  case_statement* prev;    /*!< Pointer to previous case statement in list   */
};

/*------------------------------------------------------------------------------*/
/*!
 A module instance element in the module instance tree.
*/
struct mod_inst_s;

/*!
 Renaming module instance structure for convenience.
*/
typedef struct mod_inst_s mod_inst;

struct mod_inst_s {
  char*      name;          /*!< Instance name of this module instance                      */
  module*    mod;           /*!< Pointer to module this instance represents                 */
  statistic* stat;          /*!< Pointer to statistic holder                                */
  inst_parm* param_head;    /*!< Head pointer to list of parameter overrides in this module */
  inst_parm* param_tail;    /*!< Tail pointer to list of parameter overrides in this module */
  mod_inst*  parent;        /*!< Pointer to parent instance -- used for convenience only    */
  mod_inst*  child_head;    /*!< Pointer to head of child list                              */
  mod_inst*  child_tail;    /*!< Pointer to tail of child list                              */
  mod_inst*  next;          /*!< Pointer to next child in parents list                      */
};

/*-------------------------------------------------------------------------------*/
/*!
 Node for a tree that carries two strings:  a key and a value.  The tree is a binary
 tree that is sorted by key.
*/
struct tnode_s;

/*!
 Renaming tree node structure for convenience.
*/
typedef struct tnode_s tnode;

struct tnode_s {
  char*  name;     /*!< Key value for tree node     */
  char*  value;    /*!< Value of node               */
  tnode* left;     /*!< Pointer to left child node  */
  tnode* right;    /*!< Pointer to right child node */
  tnode* up;       /*!< Pointer to parent node      */
};

/*-------------------------------------------------------------------------------*/
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
 
/*!
 Renaming timer structure for convenience.
*/
typedef struct timer_s timer;

struct timer_s {
  struct tms start;  /*!< Contains start time of a particular timer                     */
  clock_t    total;  /*!< Contains the total amount of user time accrued for this timer */
};
#endif

/*-------------------------------------------------------------------------------*/
struct fsm_sig_s;

typedef struct fsm_sig_s fsm_sig;

struct fsm_sig_s {
  char*    name;   /*!< Name of signal used in FSM                            */
  int      width;  /*!< Bit select width of signal used for FSM               */
  int      lsb;    /*!< Least significant bit position of signal used for FSM */
  fsm_sig* next;   /*!< Pointer to next FSM signal in list                    */
};

/*-------------------------------------------------------------------------------*/
struct fsm_var_s;

typedef struct fsm_var_s fsm_var;

struct fsm_var_s {
  char*    mod;        /*!< Name of module to containing FSM variable                    */
  fsm_sig* ivar_head;  /*!< Pointer to head of input variable signal list within module  */
  fsm_sig* ivar_tail;  /*!< Pointer to tail of input variable signal list within module  */
  fsm_sig* ovar_head;  /*!< Pointer to head of output variable signal list within module */
  fsm_sig* ovar_tail;  /*!< Pointer to tail of output variable signal list within module */
  signal*  isig;       /*!< Pointer to input signal matching ovar name                   */
  fsm*     table;      /*!< Pointer to FSM containing signal from ovar                   */
  fsm_var* next;       /*!< Pointer to next fsm_var element in list                      */
};

/*-------------------------------------------------------------------------------*/

union expr_stmt_u {
  expression* expr;         /*!< Pointer to expression */
  statement*  stmt;         /*!< Pointer to statement  */
};


/*
 $Log$
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

