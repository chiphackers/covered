#ifndef __DEFINES_H__
#define __DEFINES_H__

/*!
 \file     defines.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/27/2001
 \brief    Contains definitions/structures used in the Covered utility.
*/

#include "../config.h"

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
 Indicate that the specified error is only a warning that something
 may be wrong but will not cause the program to immediately terminate.
 Suppressed if output_suppressed boolean variable in util.c is set to TRUE.
*/
#define WARNING      2

/*!
 Indicate that the specified message is not an error but some type of
 necessary output.  Suppressed if output_suppressed boolean variable in
 util.c is set to TRUE.
*/
#define NORMAL       3

/*!
 Indicates that the specified message is debug information that should
 only be displayed to the screen when the -D flag is specified.
*/
#define DEBUG        4

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

/*! @} */


/*!
 \addtogroup tree_fname_comps Compare types for tree matches.

 The following three defines specify to the tree_find_module function how it should
 deal with the presence/absence of the filename attribute of the module structure.

 @{
*/

/*!
 Specifies that the tree_find_module function should only match if filename is set.
*/
#define FNAME_SET            1

/*!
 Specifies that the tree_find_module function should only match if filename is not set.
*/
#define FNAME_NOTSET         2

/*!
 Specifies that the tree_find_module function should not care about the value of filename.
*/
#define FNAME_DONTCARE       3

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
#define SUPPL_MERGE_MASK            0xff7fffff

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

#define EXP_OP_STATIC     0x00  /*!<  0 Specifies constant value      */
#define EXP_OP_SIG        0x01  /*!<  1 Specifes signal value         */
#define EXP_OP_XOR        0x02  /*!<  2 '^'   operator                */
#define EXP_OP_MULTIPLY   0x03  /*!<  3 '*'   operator                */
#define EXP_OP_DIVIDE     0x04  /*!<  4 '/'   operator                */
#define EXP_OP_MOD        0x05  /*!<  5 '%'   operator                */
#define EXP_OP_ADD        0x06  /*!<  6 '+'   operator                */
#define EXP_OP_SUBTRACT   0x07  /*!<  7 '-'   operator                */
#define EXP_OP_AND        0x08  /*!<  8 '&'   operator                */
#define EXP_OP_OR         0x09  /*!<  9 '|'   operator                */
#define EXP_OP_NAND       0x0a  /*!< 10 '~&'  operator                */
#define EXP_OP_NOR        0x0b  /*!< 11 '~|'  operator                */
#define EXP_OP_NXOR       0x0c  /*!< 12 '~^'  operator                */
#define EXP_OP_LT         0x0d  /*!< 13 '<'   operator                */
#define EXP_OP_GT         0x0e  /*!< 14 '>'   operator                */
#define EXP_OP_LSHIFT     0x0f  /*!< 15 '<<'  operator                */
#define EXP_OP_RSHIFT     0x10  /*!< 16 '>>'  operator                */
#define EXP_OP_EQ         0x11  /*!< 17 '=='  operator                */
#define EXP_OP_CEQ        0x12  /*!< 18 '===' operator                */
#define EXP_OP_LE         0x13  /*!< 19 '<='  operator                */
#define EXP_OP_GE         0x14  /*!< 20 '>='  operator                */
#define EXP_OP_NE         0x15  /*!< 21 '!='  operator                */
#define EXP_OP_CNE        0x16  /*!< 22 '!==' operator                */
#define EXP_OP_LOR        0x17  /*!< 23 '||'  operator                */
#define EXP_OP_LAND       0x18  /*!< 24 '&&'  operator                */
#define EXP_OP_COND       0x19  /*!< 25 '?:' conditional operator     */
#define EXP_OP_COND_SEL   0x1a  /*!< 26 '?:' conditional select       */
#define EXP_OP_UINV       0x1b  /*!< 27 unary '~'  operator           */
#define EXP_OP_UAND       0x1c  /*!< 28 unary '&'  operator           */
#define EXP_OP_UNOT       0x1d  /*!< 29 unary '!'  operator           */
#define EXP_OP_UOR        0x1e  /*!< 30 unary '|'  operator           */
#define EXP_OP_UXOR       0x1f  /*!< 31 unary '^'  operator           */
#define EXP_OP_UNAND      0x20  /*!< 32 unary '~&' operator           */
#define EXP_OP_UNOR       0x21  /*!< 33 unary '~|' operator           */
#define EXP_OP_UNXOR      0x22  /*!< 34 unary '~^' operator           */
#define EXP_OP_SBIT_SEL   0x23  /*!< 35 single-bit signal select      */
#define EXP_OP_MBIT_SEL   0x24  /*!< 36 multi-bit signal select       */
#define EXP_OP_EXPAND     0x25  /*!< 37 bit expansion operator        */
#define EXP_OP_CONCAT     0x26  /*!< 38 signal concatenation operator */
#define EXP_OP_PEDGE      0x27  /*!< 39 posedge operator              */
#define EXP_OP_NEDGE      0x28  /*!< 40 negedge operator              */
#define EXP_OP_AEDGE      0x29  /*!< 41 anyedge operator              */
#define EXP_OP_LAST       0x2a  /*!< 42 1-bit value holder for parent */
#define EXP_OP_EOR        0x2b  /*!< 43 event OR operator             */
#define EXP_OP_DELAY      0x2c  /*!< 44 delay operator                */
#define EXP_OP_CASE       0x2d  /*!< 45 case equality expression      */
#define EXP_OP_CASEX      0x2e  /*!< 46 casex equality expression     */
#define EXP_OP_CASEZ      0x2f  /*!< 47 casez equality expression     */
#define EXP_OP_DEFAULT    0x30  /*!< 48 case default expression       */
#define EXP_OP_LIST       0x31  /*!< 49 comma separated expr list     */
#define EXP_OP_PARAM      0x32  /*!< 50 full parameter                */
#define EXP_OP_PARAM_SBIT 0x33  /*!< 51 single bit select parameter   */
#define EXP_OP_PARAM_MBIT 0x34  /*!< 52 multi-bit select parameter    */

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
 Defines boolean variables used in most functions.
*/
typedef enum {
  FALSE,   /*!< Boolean false value */
  TRUE     /*!< Boolean true value  */
} bool;

/*!
 A nibble is a 32-bit value that is subdivided into three parts:
   Bits  7 -  0 = 4 bits of 4-state value.
   Bits 11 -  8 = Toggle01 value for each of the four 4-state bits.
   Bits 15 - 12 = Toggle10 value for each of the four 4-state bits.
   Bits 19 - 16 = Indicates if this bit has been previously assigned.
   Bits 23 - 20 = Static value indicators for each of the four 4-state bits.
   Bits 27 - 24 = Indicates if this bit was set to a value of 0 (FALSE).
   Bits 31 - 28 = Indicates if this bit was set to a value of 1 (TRUE).
*/
#if SIZEOF_INT == 4
typedef unsigned int nibble;
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

typedef struct vector_s vector;

/*------------------------------------------------------------------------------*/
/*!
 Allows the parent pointer of an expression to point to either another expression
 or a statement.
*/
union expr_stmt_u;

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

typedef struct expression_s expression;
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

typedef struct statement_s statement;

struct statement_s {
  expression* exp;         /*!< Pointer to associated expression tree                        */
  statement*  next_true;   /*!< Pointer to next statement to run if expression tree non-zero */
  statement*  next_false;  /*!< Pointer to next statement to run if next_true not picked     */
};


typedef struct stmt_iter_s stmt_iter;

/*------------------------------------------------------------------------------*/
/*!
 Expression link element.  Stores pointer to an expression.
*/
struct exp_link_s;

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

typedef struct stmt_loop_link_s stmt_loop_link;

struct stmt_loop_link_s {
  statement*      stmt;     /* Pointer to last statement in loop  */
  int             id;       /* ID of next statement after last    */
  stmt_loop_link* next;     /* Pointer to next statement in stack */
};

/*------------------------------------------------------------------------------*/
/*!
 Stores all information needed to represent a signal.  If value of value element is non-zero at the
 end of the run, this signal has been simulated.
*/
struct signal_s {
  char*      name;      /*!< Full hierarchical name of signal in design */
  vector*    value;     /*!< Pointer to vector value of this signal     */
  exp_link*  exp_head;  /*!< Head pointer to list of expressions        */
  exp_link*  exp_tail;  /*!< Tail pointer to list of expressions        */
};

/*------------------------------------------------------------------------------*/
/*!
 Linked list element that stores a signal.
*/
struct sig_link_s;

typedef struct sig_link_s sig_link;

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

typedef struct statistic_s statistic;

struct statistic_s {
  float line_total;    /*!< Total number of lines parsed               */
  int   line_hit;      /*!< Number of lines executed during simulation */
  float tog_total;     /*!< Total number of bits to toggle             */
  int   tog01_hit;     /*!< Number of bits toggling from 0 to 1        */
  int   tog10_hit;     /*!< Number of bits toggling from 1 to 0        */
  float comb_total;    /*!< Total number of expression combinations    */
  int   comb_hit;      /*!< Number of logic combinations hit           */
};

/*------------------------------------------------------------------------------*/
/*!
 Structure containing parts for a module parameter definition.
*/
struct mod_parm_s;

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

typedef struct inst_parm_s inst_parm;

struct inst_parm_s {
  char*        name;     /*!< Name of associated parameter (no hierarchy)         */
  vector*      value;    /*!< Pointer to value of instance parameter              */
  mod_parm*    mparm;    /*!< Pointer to base module parameter                    */
  inst_parm*   next;     /*!< Pointer to next instance parameter in list          */
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
  mod_parm*  param_head; /*!< Head pointer to list of parameters in this module  */
  mod_parm*  param_tail; /*!< Tail pointer to list of parameters in this module  */
};

typedef struct module_s module;

/*------------------------------------------------------------------------------*/
/*!
 Linked list element that stores a module (no scope).
*/
struct mod_link_s;
	
typedef struct mod_link_s mod_link;
	
struct mod_link_s {
  module*    mod;   /*!< Pointer to module in list */
  mod_link* next;   /*!< Next module in list       */
};

/*------------------------------------------------------------------------------*/
/*!
 Stores symbol name of signal along with pointer to signal itself into a lookup table
*/
struct symtable_s;

typedef struct symtable_s symtable;

struct symtable_s {
  char*     sym;          /*!< Name of VCD-specified signal                */
  signal*   sig;          /*!< Pointer to signal for this symbol           */
  char*     value;        /*!< String representation of last current value */
  int       size;         /*!< Number of bytes allowed storage for value   */
  symtable* right;        /*!< Pointer to next symtable entry to the right */
  symtable* left;         /*!< Pointer to next symtable entry to the left  */
};

/*------------------------------------------------------------------------------*/
/*!
 Specifies possible values for a static expression (constant value).
*/
struct static_expr_s {
  expression* exp;        /*!< Specifies if static value is an expression   */
  int         num;        /*!< Specifies if static value is a numeric value */
};

typedef struct static_expr_s static_expr;

/*------------------------------------------------------------------------------*/
/*!
 Specifies bit range of a signal or expression.
*/
struct vector_width_s {
  static_expr* left;      /*!< Specifies left bit value of bit range  */
  static_expr* right;     /*!< Specifies right bit value of bit range */
};

typedef struct vector_width_s vector_width;

/*------------------------------------------------------------------------------*/
/*!
 Binds a signal to an expression.
*/
struct sig_exp_bind_s;

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

union expr_stmt_u {
  expression* expr;         /*!< Pointer to expression */
  statement*  stmt;         /*!< Pointer to statement  */
};


/*
 $Log$
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

