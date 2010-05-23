#ifndef __DEFINES_H__
#define __DEFINES_H__

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
 \file     defines.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
 \brief    Contains definitions/structures used in the Covered utility.
 
 \par
 This file is included by all files within Covered.  All defines, macros, structures, enums,
 and typedefs in the tool are specified in this file.
*/

#include "../config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <float.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cexcept.h"

/*!
 Specifies current version of the Covered utility.
*/
#define COVERED_VERSION    VERSION

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
#define CDD_VERSION        24

/*!
 This contains the header information specified when executing this tool.
*/
#define COVERED_HEADER     "\nCovered %s -- Verilog Code Coverage Utility\nWritten by Trevor Williams  (phase1geo@gmail.com)\nCopyright 2006-2010\nFreely distributable under the GPL license\n", COVERED_VERSION

/*!
 Default database filename if not specified on command-line.
*/
#define DFLT_OUTPUT_CDD    "cov.cdd"

/*!
 Default profiling output filename.
*/
#define PROFILING_OUTPUT_NAME "covered.prof"

/*!
 Default filename that will contain the code necessary to attach Covered as a VPI to the Verilog
 simulator.
*/
#define DFLT_VPI_NAME      "covered_vpi.v"

/*!
 Default filename that will contain the code necessary to dump only the needed signals of the design.
*/
#define DFLT_DUMPVARS_NAME "covered_dump.v"

/*!
 Determine size of integer in bits.
*/
#define INTEGER_WIDTH	   (SIZEOF_INT * 8)

/*!
 Specifies the maximum number of bits that a vector can hold.
*/
#define MAX_BIT_WIDTH      65536

/*!
 Specifies the maximum number of bytes that can be allocated via the safe allocation functions
 in util.c.
*/
#define MAX_MALLOC_SIZE    (MAX_BIT_WIDTH * 2)

/*!
 Length of user_msg global string (used for inputs to snprintf calls).
*/
#define USER_MSG_LENGTH    (MAX_BIT_WIDTH * 2)

/*!
 If -w option is specified to report command, specifies number of characters of width
 we will output.
*/
#define DEFAULT_LINE_WIDTH 105

/*!
 \addtogroup generations Supported Generations

 The following defines are used by Covered to specify the supported Verilog generations

 @{
*/

/*! Verilog-1995 */
#define GENERATION_1995		0

/*! Verilog-2001 */
#define GENERATION_2001 	1

/*! SystemVerilog */
#define GENERATION_SV		2

/*! @} */

/*!
 \addtogroup dumpfile_fmt Dumpfile Format

 The following defines are used by Covered to determine what dumpfile parsing mode we are in

 @{
*/

/*! No dumpfile was specified */
#define DUMP_FMT_NONE      0

/*! VCD dumpfile was specified */
#define DUMP_FMT_VCD       1

/*! LXT dumpfile was specified */
#define DUMP_FMT_LXT       2

/*! FST dumpfile was specified */
#define DUMP_FMT_FST       3

/*! @} */

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

/*!
 Indicates that the specified message is header information that should
 be displayed if the -Q global flag is not specified.
*/
#define HEADER       7

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
#define DB_TYPE_SIGNAL        1

/*!
 Specifies that the current coverage database line describes an expression.
*/
#define DB_TYPE_EXPRESSION    2

/*!
 Specifies that the current coverage database line describes a module.
*/
#define DB_TYPE_FUNIT         3

/*!
 Specifies that the current coverage database line describes a statement.
*/
#define DB_TYPE_STATEMENT     4

/*!
 Specifies that the current coverage database line describes general information.
*/
#define DB_TYPE_INFO          5

/*!
 Specifies that the current coverage database line describes a finite state machine.
*/
#define DB_TYPE_FSM           6

/*!
 Specifies that the current coverage database line describes a race condition block.
*/
#define DB_TYPE_RACE          7

/*!
 Specifies the arguments specified to the score command.
*/
#define DB_TYPE_SCORE_ARGS    8

/*!
 Specifies that the current coverage database line describes a new struct/union
 (all signals and struct/unions below this are members of this struct/union)
*/
#define DB_TYPE_SU_START      9

/*!
 Specifies that the current coverage database line ends the currently populated struct/union.
*/
#define DB_TYPE_SU_END        10

/*!
 Specifies that the current coverage database line contains user-specified information
*/
#define DB_TYPE_MESSAGE       11

/*!
 Specifies that the current line describes a CDD file that was merged into this CDD
*/
#define DB_TYPE_MERGED_CDD    12

/*!
 Specifies that the current line describes an exclusion reason.
*/
#define DB_TYPE_EXCLUDE       13

/*!
 Specifies a version ID for a functional unit (based on the associated file version).
*/
#define DB_TYPE_FUNIT_VERSION 14

/*!
 Specifies that this is a placeholder instance (no functional unit attached).
*/
#define DB_TYPE_INST_ONLY     15

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

/*! Represents a scope that is considered a "no score" functional unit */
#define FUNIT_NO_SCORE       4

/*! Represents a re-entrant Verilog function (syntax "function automatic <name> ... endfunction") */
#define FUNIT_AFUNCTION      5

/*! Represents a re-entrant Verilog task (syntax "task <name> ... endtask") */
#define FUNIT_ATASK          6

/*! Represents a named block inside of a re-entrant Verilog task or function */
#define FUNIT_ANAMED_BLOCK   7

/*! The number of valid functional unit types */
#define FUNIT_TYPES          8

/*! @} */

/*!
 \addtogroup merge_exclusion_resolution_types Exclusion Merge Resolution Types

 The following defines specify the various methods that can be used to globally resolve exclusion reason conflicts.

 @{
*/

/*! Specifies that no -er option was specified on the merge command-line.  Interactively resolve exclusion reason conflicts. */
#define MERGE_ER_NONE    0

/*! Specifies that the first value was specified to the -er option.  Use the first exclusion reason found. */
#define MERGE_ER_FIRST   1

/*! Specifies that the last value was specified to the -er option.  Use the last exclusion reason found. */
#define MERGE_ER_LAST    2

/*! Specifies that the exclusion reasons should be merged. */
#define MERGE_ER_ALL     3

/*! Specified that the newest exclusion reasons should be used. */
#define MERGE_ER_NEW     4

/*! Specified that the oldest exclusion reasons should be used. */
#define MERGE_ER_OLD     5

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
 merge.  See esuppl_u for information on which bits are masked.
*/
#define ESUPPL_MERGE_MASK            0xffffff

/*!
 Specifies the number of bits to store for a given expression for reentrant purposes.
 Contains the following expression supplemental field bits:
 -# left_changed
 -# right_changed
 -# eval_t
 -# eval_f
 -# prev_called
*/
#define ESUPPL_BITS_TO_STORE         5

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

/*!
 Returns a value of 1 if the specified expression is contained in a function.
*/
#define ESUPPL_IS_IN_FUNC(x)         x.part.in_func

/*!
 Returns a value of 1 if the specified expression owns its vector structure or 0 if it
 shares it with someone else.
*/
#define ESUPPL_OWNS_VEC(x)           x.part.owns_vec

/*!
 Returns a value of 1 if the specified expression should be excluded from coverage
 consideration.
*/
#define ESUPPL_EXCLUDED(x)           x.part.excluded

/*!
 Returns the type of expression that this expression should be treated as.  Legal values
 are specified \ref expression_types.
*/
#define ESUPPL_TYPE(x)               x.part.type

/*!
 Returns the base type of the vector value if the expression is an EXP_OP_STATIC.  Legal
 values are SIZED_DECIMAL, DECIMAL, HEXIDECIMAL, OCTAL, BINARY, and QSTRING.
*/
#define ESUPPL_STATIC_BASE(x)        x.part.base

/*! @} */

/*!
 \addtogroup ssuppl_type Signal Supplemental Field Types

 The following defines represent the various types of signals that can be parsed.

 @{
*/

/*! This is an input port net signal */
#define SSUPPL_TYPE_INPUT_NET      0

/*! This is an input port registered signal */
#define SSUPPL_TYPE_INPUT_REG      1

/*! This is an output port net signal */
#define SSUPPL_TYPE_OUTPUT_NET     2

/*! This is an output port registered signal */
#define SSUPPL_TYPE_OUTPUT_REG     3

/*! This is an inout port net signal */
#define SSUPPL_TYPE_INOUT_NET      4

/*! This is an inout port registered signal */
#define SSUPPL_TYPE_INOUT_REG      5

/*! This is a declared net signal (i.e., not a port) */
#define SSUPPL_TYPE_DECL_NET       6

/*! This is a declared registered signal (i.e., not a port) */
#define SSUPPL_TYPE_DECL_REG       7

/*! This is an event */
#define SSUPPL_TYPE_EVENT          8

/*! This signal was implicitly created */
#define SSUPPL_TYPE_IMPLICIT       9

/*! This signal was implicitly created (this signal was created from a positive variable multi-bit expression) */
#define SSUPPL_TYPE_IMPLICIT_POS   10

/*! This signal was implicitly created (this signal was created from a negative variable multi-bit expression) */
#define SSUPPL_TYPE_IMPLICIT_NEG   11

/*! This signal is a parameter */
#define SSUPPL_TYPE_PARAM          12

/*! This signal is a genvar */
#define SSUPPL_TYPE_GENVAR         13

/*! This signal is an enumerated value */
#define SSUPPL_TYPE_ENUM           14

/*! This signal is a memory */
#define SSUPPL_TYPE_MEM            15

/*! This signal is a shortreal */
#define SSUPPL_TYPE_DECL_SREAL     16

/*! This signal is a real */
#define SSUPPL_TYPE_DECL_REAL      17

/*! This signal is a real parameter */
#define SSUPPL_TYPE_PARAM_REAL     18

/*! This signal was implicitly created for a real64 value */
#define SSUPPL_TYPE_IMPLICIT_REAL  19

/*! This signal was implicitly created for a real64 value */
#define SSUPPL_TYPE_IMPLICIT_SREAL 20 

/*! @} */

/*!
 Returns TRUE if the given vsignal is a net type.
*/
#define SIGNAL_IS_NET(x)          ((x->suppl.part.type == SSUPPL_TYPE_INPUT_NET)    || \
                                   (x->suppl.part.type == SSUPPL_TYPE_OUTPUT_NET)   || \
                                   (x->suppl.part.type == SSUPPL_TYPE_INOUT_NET)    || \
                                   (x->suppl.part.type == SSUPPL_TYPE_EVENT)        || \
                                   (x->suppl.part.type == SSUPPL_TYPE_DECL_NET)     || \
                                   (x->suppl.part.type == SSUPPL_TYPE_IMPLICIT)     || \
                                   (x->suppl.part.type == SSUPPL_TYPE_IMPLICIT_POS) || \
                                   (x->suppl.part.type == SSUPPL_TYPE_IMPLICIT_NEG))
     
/*!
 Returns TRUE if the signal must be assigned from the dumpfile.
*/
#define SIGNAL_ASSIGN_FROM_DUMPFILE(x)  (((x->suppl.part.assigned == 0) || info_suppl.part.inlined) && \
                                         (x->suppl.part.type != SSUPPL_TYPE_PARAM)      && \
                                         (x->suppl.part.type != SSUPPL_TYPE_PARAM_REAL) && \
                                         (x->suppl.part.type != SSUPPL_TYPE_ENUM)       && \
                                         (x->suppl.part.type != SSUPPL_TYPE_MEM)        && \
                                         (x->suppl.part.type != SSUPPL_TYPE_GENVAR)     && \
                                         (x->suppl.part.type != SSUPPL_TYPE_EVENT))

/*!
 \addtogroup read_modes Modes for reading database file

 Specify how to integrate read data from database file into memory structures.

 @{
*/

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when reading after parsing source files.
*/
#define READ_MODE_NO_MERGE                0

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when performing a MERGE command on first file.
*/
#define READ_MODE_MERGE_NO_MERGE          1

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when performing a REPORT command.
*/
#define READ_MODE_REPORT_NO_MERGE         2

/*!
 When functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the current instance tree.  If the instance exists, the
 functional unit is merged with the instance's functional unit; otherwise, we are attempting to
 merge two databases that were created from different designs.
*/
#define READ_MODE_MERGE_INST_MERGE        3

/*!
 When functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the functional unit list.  If the functional unit is found,
 it is merged with the existing functional unit; otherwise, it is added to the functional
 unit list.
*/
#define READ_MODE_REPORT_MOD_MERGE        4

/*! @} */

/*!
 \addtogroup param_suppl_types Instance/module parameter supplemental type definitions.

 @{
*/

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
 Specifies that the current module parameter specifies the value for an instance
 instantiation LSB value.
*/
#define PARAM_TYPE_INST_LSB             4

/*!
 Specifies that the current module parameter specifies the value for an instance
 instantiation MSB value.
*/
#define PARAM_TYPE_INST_MSB             5

/*!
 Specifies that the current module parameter is a declared type (belongs to current
 module) and is local (cannot be overridden by defparams and inline parameter overrides).
*/
#define PARAM_TYPE_DECLARED_LOCAL       6

/*! @} */

/*!
 \addtogroup generate_item_types Generate Block Item Types

 The following defines specify the different types of elements that can be stored in
 a generate item structure.

 @{
*/

/*! Holds an expression that is used in either a generate FOR, IF or CASE statement */
#define GI_TYPE_EXPR            0

/*! Holds a signal instantiation */
#define GI_TYPE_SIG             1

/*! Holds a statement block (initial, assign, always) */
#define GI_TYPE_STMT            2

/*! Holds an module instantiation */
#define GI_TYPE_INST            3

/*! Holds a task, function, named begin/end block */
#define GI_TYPE_TFN             4

/*! Holds information for a signal/expression bind */
#define GI_TYPE_BIND            5

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
 Reasons for excluding a logic block from coverage consideration.
*/
typedef enum logic_rm_type_e {
  LOGIC_RM_REAL = 0,     /*!< 0.  Statement block excluded because it contains a real number. */
  LOGIC_RM_SYSFUNC,      /*!< 1.  Statement block excluded because it contains a system function. */
  LOGIC_RM_SYSTASK,      /*!< 2.  Statement block excluded because it contains a system task. */
  LOGIC_RM_NUM           /*!< Number of valid reasons */
} logic_rm_type;

/*!
 Enumeration of the various expression operations that Covered supports.
*/
typedef enum exp_op_type_e {
  EXP_OP_STATIC = 0,      /*!<   0:0x00.  Specifies constant value. */
  EXP_OP_SIG,             /*!<   1:0x01.  Specifies signal value. */
  EXP_OP_XOR,             /*!<   2:0x02.  Specifies '^' operator. */
  EXP_OP_MULTIPLY,        /*!<   3:0x03.  Specifies '*' operator. */
  EXP_OP_DIVIDE,          /*!<   4:0x04.  Specifies '/' operator. */
  EXP_OP_MOD,             /*!<   5:0x05.  Specifies '%' operator. */
  EXP_OP_ADD,             /*!<   6:0x06.  Specifies '+' operator. */
  EXP_OP_SUBTRACT,        /*!<   7:0x07.  Specifies '-' operator. */
  EXP_OP_AND,             /*!<   8:0x08.  Specifies '&' operator. */
  EXP_OP_OR,              /*!<   9:0x09.  Specifies '|' operator. */
  EXP_OP_NAND,            /*!<  10:0x0a.  Specifies '~&' operator. */
  EXP_OP_NOR,             /*!<  11:0x0b.  Specifies '~|' operator. */
  EXP_OP_NXOR,            /*!<  12:0x0c.  Specifies '~^' operator. */
  EXP_OP_LT,              /*!<  13:0x0d.  Specifies '<' operator. */
  EXP_OP_GT,              /*!<  14:0x0e.  Specifies '>' operator. */
  EXP_OP_LSHIFT,          /*!<  15:0x0f.  Specifies '<<' operator. */
  EXP_OP_RSHIFT,          /*!<  16:0x10.  Specifies '>>' operator. */
  EXP_OP_EQ,              /*!<  17:0x11.  Specifies '==' operator. */
  EXP_OP_CEQ,             /*!<  18:0x12.  Specifies '===' operator. */
  EXP_OP_LE,              /*!<  19:0x13.  Specifies '<=' operator. */
  EXP_OP_GE,              /*!<  20:0x14.  Specifies '>=' operator. */
  EXP_OP_NE,              /*!<  21:0x15.  Specifies '!=' operator. */
  EXP_OP_CNE,             /*!<  22:0x16.  Specifies '!==' operator. */
  EXP_OP_LOR,             /*!<  23:0x17.  Specifies '||' operator. */
  EXP_OP_LAND,            /*!<  24:0x18.  Specifies '&&' operator. */
  EXP_OP_COND,            /*!<  25:0x19.  Specifies '?:' conditional operator. */
  EXP_OP_COND_SEL,        /*!<  26:0x1a.  Specifies '?:' conditional select. */
  EXP_OP_UINV,            /*!<  27:0x1b.  Specifies '~' unary operator. */
  EXP_OP_UAND,            /*!<  28:0x1c.  Specifies '&' unary operator. */
  EXP_OP_UNOT,            /*!<  29:0x1d.  Specifies '!' unary operator. */
  EXP_OP_UOR,             /*!<  30:0x1e.  Specifies '|' unary operator. */
  EXP_OP_UXOR,            /*!<  31:0x1f.  Specifies '^' unary operator. */
  EXP_OP_UNAND,           /*!<  32:0x20.  Specifies '~&' unary operator. */
  EXP_OP_UNOR,            /*!<  33:0x21.  Specifies '~|' unary operator. */
  EXP_OP_UNXOR,           /*!<  34:0x22.  Specifies '~^' unary operator. */
  EXP_OP_SBIT_SEL,        /*!<  35:0x23.  Specifies single-bit signal select (i.e., [x]). */
  EXP_OP_MBIT_SEL,        /*!<  36:0x24.  Specifies multi-bit signal select (i.e., [x:y]). */
  EXP_OP_EXPAND,          /*!<  37:0x25.  Specifies bit expansion operator (i.e., {x{y}}). */
  EXP_OP_CONCAT,          /*!<  38:0x26.  Specifies signal concatenation operator (i.e., {x,y}). */
  EXP_OP_PEDGE,           /*!<  39:0x27.  Specifies posedge operator (i.e., \@posedge x). */
  EXP_OP_NEDGE,           /*!<  40:0x28.  Specifies negedge operator (i.e., \@negedge x). */
  EXP_OP_AEDGE,           /*!<  41:0x29.  Specifies anyedge operator (i.e., \@x). */
  EXP_OP_LAST,            /*!<  42:0x2a.  Specifies 1-bit holder for parent. */
  EXP_OP_EOR,             /*!<  43:0x2b.  Specifies 'or' event operator. */
  EXP_OP_DELAY,           /*!<  44:0x2c.  Specifies delay operator (i.e., #(x)). */
  EXP_OP_CASE,            /*!<  45:0x2d.  Specifies case equality expression. */
  EXP_OP_CASEX,           /*!<  46:0x2e.  Specifies casex equality expression. */
  EXP_OP_CASEZ,           /*!<  47:0x2f.  Specifies casez equality expression. */
  EXP_OP_DEFAULT,         /*!<  48:0x30.  Specifies case/casex/casez default expression. */
  EXP_OP_LIST,            /*!<  49:0x31.  Specifies comma separated expression list. */
  EXP_OP_PARAM,           /*!<  50:0x32.  Specifies full parameter. */
  EXP_OP_PARAM_SBIT,      /*!<  51:0x33.  Specifies single-bit select parameter. */
  EXP_OP_PARAM_MBIT,      /*!<  52:0x34.  Specifies multi-bit select parameter. */
  EXP_OP_ASSIGN,          /*!<  53:0x35.  Specifies an assign assignment operator. */
  EXP_OP_DASSIGN,         /*!<  54:0x36.  Specifies a wire declaration assignment operator. */
  EXP_OP_BASSIGN,         /*!<  55:0x37.  Specifies a blocking assignment operator. */
  EXP_OP_NASSIGN,         /*!<  56:0x38.  Specifies a non-blocking assignment operator. */
  EXP_OP_IF,              /*!<  57:0x39.  Specifies an if statement operator. */
  EXP_OP_FUNC_CALL,       /*!<  58:0x3a.  Specifies a function call. */
  EXP_OP_TASK_CALL,       /*!<  59:0x3b.  Specifies a task call (note: this operator MUST be the root of the expression tree) */
  EXP_OP_TRIGGER,         /*!<  60:0x3c.  Specifies an event trigger (->). */
  EXP_OP_NB_CALL,         /*!<  61:0x3d.  Specifies a "call" to a named block */
  EXP_OP_FORK,            /*!<  62:0x3e.  Specifies a fork command */
  EXP_OP_JOIN,            /*!<  63:0x3f.  Specifies a join command */
  EXP_OP_DISABLE,         /*!<  64:0x40.  Specifies a disable command */
  EXP_OP_REPEAT,          /*!<  65:0x41.  Specifies a repeat loop test expression */
  EXP_OP_WHILE,           /*!<  66:0x42.  Specifies a while loop test expression */
  EXP_OP_ALSHIFT,         /*!<  67:0x43.  Specifies arithmetic left shift (<<<) */
  EXP_OP_ARSHIFT,         /*!<  68:0x44.  Specifies arithmetic right shift (>>>) */
  EXP_OP_SLIST,           /*!<  69:0x45.  Specifies sensitivity list (*) */
  EXP_OP_EXPONENT,        /*!<  70:0x46.  Specifies the exponential operator "**" */
  EXP_OP_PASSIGN,         /*!<  71:0x47.  Specifies a port assignment */
  EXP_OP_RASSIGN,         /*!<  72:0x48.  Specifies register assignment (reg a = 1'b0) */
  EXP_OP_MBIT_POS,        /*!<  73:0x49.  Specifies positive variable multi-bit select (a[b+:3]) */
  EXP_OP_MBIT_NEG,        /*!<  74:0x4a.  Specifies negative variable multi-bit select (a[b-:3]) */
  EXP_OP_PARAM_MBIT_POS,  /*!<  75:0x4b.  Specifies positive variable multi-bit parameter select */
  EXP_OP_PARAM_MBIT_NEG,  /*!<  76:0x4c.  Specifies negative variable multi-bit parameter select */
  EXP_OP_NEGATE,          /*!<  77:0x4d.  Specifies the unary negate operator (-) */
  EXP_OP_NOOP,            /*!<  78:0x4e.  Specifies no operation is to be performed (placeholder) */
  EXP_OP_ALWAYS_COMB,     /*!<  79:0x4f.  Specifies an always_comb statement (implicit event expression - similar to SLIST) */
  EXP_OP_ALWAYS_LATCH,    /*!<  80:0x50.  Specifies an always_latch statement (implicit event expression - similar to SLIST) */
  EXP_OP_IINC,            /*!<  81:0x51.  Specifies the immediate increment SystemVerilog operator (++a) */
  EXP_OP_PINC,            /*!<  82:0x52.  Specifies the postponed increment SystemVerilog operator (a++) */
  EXP_OP_IDEC,            /*!<  83:0x53.  Specifies the immediate decrement SystemVerilog operator (--a) */
  EXP_OP_PDEC,            /*!<  84:0x54.  Specifies the postponed decrement SystemVerilog operator (a--) */
  EXP_OP_DLY_ASSIGN,      /*!<  85:0x55.  Specifies a delayed assignment (i.e., a = #5 b; or a = @(c) b;) */
  EXP_OP_DLY_OP,          /*!<  86:0x56.  Child expression of DLY_ASSIGN, points to the delay expr and the op expr */
  EXP_OP_RPT_DLY,         /*!<  87:0x57.  Child expression of DLY_OP, points to the delay expr and the repeat expr */
  EXP_OP_DIM,             /*!<  88:0x58.  Specifies a selection dimension (right expression points to a selection expr) */
  EXP_OP_WAIT,            /*!<  89:0x59.  Specifies a wait statement */
  EXP_OP_SFINISH,         /*!<  90:0x5a.  Specifies a $finish call */
  EXP_OP_SSTOP,           /*!<  91:0x5b.  Specifies a $stop call */
  EXP_OP_ADD_A,           /*!<  92:0x5c.  Specifies the '+=' operator */
  EXP_OP_SUB_A,           /*!<  93:0x5d.  Specifies the '-=' operator */
  EXP_OP_MLT_A,           /*!<  94:0x5e.  Specifies the '*=' operator */
  EXP_OP_DIV_A,           /*!<  95:0x5f.  Specifies the '/=' operator */
  EXP_OP_MOD_A,           /*!<  96:0x60.  Specifies the '%=' operator */
  EXP_OP_AND_A,           /*!<  97:0x61.  Specifies the '&=' operator */
  EXP_OP_OR_A,            /*!<  98:0x62.  Specifies the '|=' operator */
  EXP_OP_XOR_A,           /*!<  99:0x63.  Specifies the '^=' operator */
  EXP_OP_LS_A,            /*!< 100:0x64.  Specifies the '<<=' operator */
  EXP_OP_RS_A,            /*!< 101:0x65.  Specifies the '>>=' operator */
  EXP_OP_ALS_A,           /*!< 102:0x66.  Specifies the '<<<=' operator */
  EXP_OP_ARS_A,           /*!< 103:0x67.  Specifies the '>>>=' operator */
  EXP_OP_FOREVER,         /*!< 104:0x68.  Specifies the 'forever' statement */
  EXP_OP_STIME,           /*!< 105:0x69.  Specifies the $time system call */
  EXP_OP_SRANDOM,         /*!< 106:0x6a.  Specifies the $random system call */
  EXP_OP_PLIST,           /*!< 107:0x6b.  Task/function port list glue */
  EXP_OP_SASSIGN,         /*!< 108:0x6c.  System task port assignment holder */
  EXP_OP_SSRANDOM,        /*!< 109:0x6d.  Specifies the $srandom system call */
  EXP_OP_SURANDOM,        /*!< 110:0x6e.  Specifies the $urandom system call */
  EXP_OP_SURAND_RANGE,    /*!< 111:0x6f.  Specifies the $urandom_range system call */
  EXP_OP_SR2B,            /*!< 112:0x70.  Specifies the $realtobits system call */
  EXP_OP_SB2R,            /*!< 113:0x71.  Specifies the $bitstoreal system call */
  EXP_OP_SSR2B,           /*!< 114:0x72.  Specifies the $shortrealtobits system call */
  EXP_OP_SB2SR,           /*!< 115:0x73.  Specifies the $bitstoshortreal system call */
  EXP_OP_SI2R,            /*!< 116:0x74.  Specifies the $itor system call */
  EXP_OP_SR2I,            /*!< 117:0x75.  Specifies the $rtoi system call */
  EXP_OP_STESTARGS,       /*!< 118:0x76.  Specifies the $test$plusargs system call */
  EXP_OP_SVALARGS,        /*!< 119:0x77.  Specifies the $value$plusargs system call */
  EXP_OP_SSIGNED,         /*!< 120:0x78.  Specifies the $signed system call */
  EXP_OP_SUNSIGNED,       /*!< 121:0x79.  Specifies the $unsigned system call */
  EXP_OP_SCLOG2,          /*!< 122:0x7a.  Specifies the $clog2 system call */
  EXP_OP_NUM              /*!< The total number of defines for expression values */
} exp_op_type;

/*!
 Returns a value of 1 if the specified expression is considered to be measurable.
*/
#define EXPR_IS_MEASURABLE(x)      (((exp_op_info[x->op].suppl.measurable == 1) && \
                                     (ESUPPL_IS_LHS( x->suppl ) == 0) && \
                                     (x->value->suppl.part.data_type != VDATA_R64) && \
                                     (x->value->suppl.part.data_type != VDATA_R32) && \
                                     !((ESUPPL_IS_ROOT( x->suppl ) == 0) && \
                                       ((x->op == EXP_OP_SIG) || \
                                        (x->op == EXP_OP_SBIT_SEL) || \
                                        (x->op == EXP_OP_MBIT_SEL) || \
                                        (x->op == EXP_OP_MBIT_POS) || \
                                        (x->op == EXP_OP_MBIT_NEG)) && \
                                       (x->parent->expr->op != EXP_OP_ASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_DASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_BASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_NASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_RASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_DLY_OP) && \
                                       (x->parent->expr->op != EXP_OP_IF) && \
                                       (x->parent->expr->op != EXP_OP_WHILE) && \
                                       (x->parent->expr->op != EXP_OP_COND)) && \
                                     (x->line != 0)) ? 1 : 0)

/*!
 Returns a value of TRUE if the specified expression is a STATIC, PARAM, PARAM_SBIT, PARAM_MBIT,
 PARAM_MBIT_POS or PARAM_MBIT_NEG operation type.
*/
#define EXPR_IS_STATIC(x)        exp_op_info[x->op].suppl.is_static

/*!
 Returns a value of true if the specified expression is considered a combination expression by
 the combinational logic report generator.
*/
#define EXPR_IS_COMB(x)          ((exp_op_info[x->op].suppl.is_comb > 0) && \
                                  (EXPR_IS_OP_AND_ASSIGN(x) || \
                                  (!expression_is_static_only( x->left ) && \
                                   !expression_is_static_only( x->right))))

/*!
 Returns a value of true if the specified expression is considered to be an event type
 (i.e., it either occurred or did not occur -- WAS_FALSE is not important).
*/
#define EXPR_IS_EVENT(x)         exp_op_info[x->op].suppl.is_event

/*!
 These expressions will only allow a statement block to advance when they evaluate to a value of true (i.e.,
 their statement's false path is NULL).  They allow context switching to occur if they evaluate to false.
*/
#define EXPR_IS_CONTEXT_SWITCH(x)	((exp_op_info[x->op].suppl.is_context_switch == 1) || \
                                         ((x->op == EXP_OP_NB_CALL) && !ESUPPL_IS_IN_FUNC(x->suppl)))

/*!
 These expressions all use someone else's vectors instead of their own.
*/
#define EXPR_OWNS_VEC(o)                ((o != EXP_OP_SIG)            && \
                                         (o != EXP_OP_SBIT_SEL)       && \
                                         (o != EXP_OP_MBIT_SEL)       && \
                                         (o != EXP_OP_MBIT_POS)       && \
                                         (o != EXP_OP_MBIT_NEG)       && \
                                         (o != EXP_OP_TRIGGER)        && \
                                         (o != EXP_OP_PARAM)          && \
                                         (o != EXP_OP_PARAM_SBIT)     && \
                                         (o != EXP_OP_PARAM_MBIT)     && \
                                         (o != EXP_OP_PARAM_MBIT_POS) && \
                                         (o != EXP_OP_PARAM_MBIT_NEG) && \
                                         (o != EXP_OP_ASSIGN)         && \
                                         (o != EXP_OP_DASSIGN)        && \
                                         (o != EXP_OP_BASSIGN)        && \
                                         (o != EXP_OP_NASSIGN)        && \
                                         (o != EXP_OP_RASSIGN)        && \
                                         (o != EXP_OP_IF)             && \
                                         (o != EXP_OP_WHILE)          && \
					 (o != EXP_OP_PASSIGN)        && \
                                         (o != EXP_OP_DLY_ASSIGN)     && \
                                         (o != EXP_OP_DIM))

/*!
 Returns a value of true if the specified expression is considered a unary expression by
 the combinational logic report generator.
*/
#define EXPR_IS_UNARY(x)        exp_op_info[x->op].suppl.is_unary

/*!
 Returns a value of 1 if the specified expression was measurable for combinational 
 coverage but not fully covered during simulation.
*/
#define EXPR_COMB_MISSED(x)     (EXPR_IS_MEASURABLE( x ) && (x->ulid != -1))

/*!
 Returns a value of 1 if the specified expression's left child may be deallocated using the
 expression_dealloc function.  We can deallocate any left expression that doesn't belong to
 a case statement or has the owned bit set to 1.
*/
#define EXPR_LEFT_DEALLOCABLE(x)     (((x->op != EXP_OP_CASE)  && \
                                       (x->op != EXP_OP_CASEX) && \
                                       (x->op != EXP_OP_CASEZ)) || \
                                      (x->suppl.part.owned == 1))

/*!
 Specifies if the right expression can be deallocated (currently there is no reason why a
 right expression can't be deallocated.
*/
#define EXPR_RIGHT_DEALLOCABLE(x)    (TRUE)

/*!
 Specifies if the expression is an op-and-assign operation (i.e., +=, -=, &=, etc.)
*/
#define EXPR_IS_OP_AND_ASSIGN(x)     ((x->op == EXP_OP_ADD_A) || \
                                      (x->op == EXP_OP_SUB_A) || \
                                      (x->op == EXP_OP_MLT_A) || \
                                      (x->op == EXP_OP_DIV_A) || \
                                      (x->op == EXP_OP_MOD_A) || \
                                      (x->op == EXP_OP_AND_A) || \
                                      (x->op == EXP_OP_OR_A)  || \
                                      (x->op == EXP_OP_XOR_A) || \
                                      (x->op == EXP_OP_LS_A)  || \
                                      (x->op == EXP_OP_RS_A)  || \
                                      (x->op == EXP_OP_ALS_A) || \
                                      (x->op == EXP_OP_ARS_A))

/*!
 Specifies the number of temporary vectors required by the given expression operation.
*/
#define EXPR_TMP_VECS(x)             exp_op_info[x].suppl.tmp_vecs

/*!
 Returns TRUE if this expression should have its dim element populated.
*/
#define EXPR_OP_HAS_DIM(x)          ((x == EXP_OP_DIM)            || \
                                     (x == EXP_OP_SBIT_SEL)       || \
                                     (x == EXP_OP_PARAM_SBIT)     || \
                                     (x == EXP_OP_MBIT_SEL)       || \
                                     (x == EXP_OP_PARAM_MBIT)     || \
                                     (x == EXP_OP_MBIT_POS)       || \
                                     (x == EXP_OP_MBIT_NEG)       || \
                                     (x == EXP_OP_PARAM_MBIT_POS) || \
                                     (x == EXP_OP_PARAM_MBIT_NEG))

/*!
 \addtogroup lexer_val_types

 The following defines specify what type of vector value can be extracted from a value
 string.  These defines are used by vector.c and the lexer.
 
 @{
*/

#define DECIMAL			0	/*!< String in format [dD][0-9]+ */
#define BINARY			1	/*!< String in format [bB][01xXzZ_\?]+ */
#define OCTAL			2	/*!< String in format [oO][0-7xXzZ_\?]+ */
#define HEXIDECIMAL		3	/*!< String in format [hH][0-9a-fA-FxXzZ_\?]+ */
#define QSTRING                 4       /*!< Quoted string */
#define SIZED_DECIMAL           5       /*!< String in format [0-9]+'[dD][0-9]+ */

/*! @} */

/*!
 \addtogroup attribute_types Attribute Types

 The following defines specify the attribute types that are parseable by Covered.

 @{
*/

#define ATTRIBUTE_UNKNOWN       0       /*!< This attribute is not recognized by Covered */
#define ATTRIBUTE_FSM           1       /*!< FSM attribute */

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

/*!
 \addtogroup comb_types Combinational Logic Output Types

 The following group of defines specify the combinational logic output types (these are stored in the is_comb
 variable in the exp_info structure.

 @{
*/

/*! Specifies that the expression is not a two variable combinational expression */
#define NOT_COMB	0

/*! Specifies that the expression is an AND type two variable combinational expression */
#define AND_COMB	1

/*! Specifies that the expression is an OR type two variable combinational expression */
#define OR_COMB		2

/*! Specifies that the expression is a two variable combinational expression that is neither an AND or OR type */
#define OTHER_COMB	3

/*! @} */

/*!
 \addtogroup vector_types Vector Types

 The following defines specify the various flavors of vector data that we can store.

 @{
*/

/*! Used for storing 2 or 4-state value-only values (no coverage information stored) */
#define VTYPE_VAL   0

/*! Used for storing 2 or 4-state signal values */
#define VTYPE_SIG   1

/*! Used for storing 2 or 4-state expression values */
#define VTYPE_EXP   2

/*! Used for storing 2 or 4-state memory information */
#define VTYPE_MEM   3

/*! @} */

/*!
 \addtogroup vector_type_sizes Vector Type Data Sizes

 The following defines specify that various types of data types that can be stored in a vector.  This
 value determines which value pointer is used in the vector structure for holding data.

 @{
*/

/*! unsigned long */
#define VDATA_UL      0

/*! 64-bit floating point */
#define VDATA_R64     1

/*! 32-bit floating point */
#define VDATA_R32     2

/*! @} */

/*!
 \addtogroup vector_type_indices Vector Type Information Indices

 The following enumerations describe the indices of the valid vector elements for
 each type of vector.

 \note
 Each vector type has a VALL and VALH.  It is important that the indices of all
 VALL and VALH for each type be the same!
 @{
*/

/*! Provides value indices for VTYPE_VAL vectors */
typedef enum vtype_val_indices_e {
  VTYPE_INDEX_VAL_VALL,
  VTYPE_INDEX_VAL_VALH,
  VTYPE_INDEX_VAL_NUM
} vtype_val_indices;

/*! Provides value indices for VTYPE_SIG vectors */
typedef enum vtype_sig_indices_e {
  VTYPE_INDEX_SIG_VALL,
  VTYPE_INDEX_SIG_VALH,
  VTYPE_INDEX_SIG_XHOLD,
  VTYPE_INDEX_SIG_TOG01,
  VTYPE_INDEX_SIG_TOG10,
  VTYPE_INDEX_SIG_MISC,
  VTYPE_INDEX_SIG_NUM
} vtype_sig_indices;

/*! Provides value indices for VTYPE_MEM vectors */
typedef enum vtype_mem_indices_e {
  VTYPE_INDEX_MEM_VALL,
  VTYPE_INDEX_MEM_VALH,
  VTYPE_INDEX_MEM_XHOLD,
  VTYPE_INDEX_MEM_TOG01,
  VTYPE_INDEX_MEM_TOG10,
  VTYPE_INDEX_MEM_WR,
  VTYPE_INDEX_MEM_RD,
  VTYPE_INDEX_MEM_MISC,
  VTYPE_INDEX_MEM_NUM
} vtype_mem_indices;

/*! Provides value indices for VTYPE_EXP vectors */
typedef enum vtype_exp_indices_e {
  VTYPE_INDEX_EXP_VALL,
  VTYPE_INDEX_EXP_VALH,
  VTYPE_INDEX_EXP_EVAL_A,
  VTYPE_INDEX_EXP_EVAL_B,
  VTYPE_INDEX_EXP_EVAL_C,
  VTYPE_INDEX_EXP_EVAL_D,
  VTYPE_INDEX_EXP_NUM
} vtype_exp_indices;

/*! @} */

/*!
 Enumeration of coverage point types that are stored in the cps array in the comp_cdd_cov structure.
*/
typedef enum cp_indices_e {
  CP_TYPE_LINE,
  CP_TYPE_TOGGLE,
  CP_TYPE_MEM,
  CP_TYPE_LOGIC,
  CP_TYPE_FSM,
  CP_TYPE_ASSERT,
  CP_TYPE_NUM
} cp_indices;

/*!
 Enumeration of report types to output.
*/
typedef enum rpt_type_e {
  RPT_TYPE_HIT,
  RPT_TYPE_MISS,
  RPT_TYPE_EXCL
} rpt_type;

/*!
 Mask for signal supplemental field when writing to CDD file.
*/
#define VSUPPL_MASK               0x7f

/*!
 \addtogroup expression_types Expression Types

 The following defines specify the various flavors of expression types that can be stored in its elem_ptr
 union.

 @{
*/

/*! Specifies that the element pointer is not valid */
#define ETYPE_NONE      0

/*! Specifies that the element pointer points to a functional unit */
#define ETYPE_FUNIT     1

/*! Specifies that the element pointer points to a delay */
#define ETYPE_DELAY     2

/*! Specifies that the element pointer points to a thread */
#define ETYPE_THREAD    3

/*! Specifies that the element pointer points to a single temporary vector */
#define ETYPE_VEC1      4

/*! Specifies that the element pointer points to a vector2 block */
#define ETYPE_VEC2      5

/*! @} */

/*!
 \addtogroup thread_states Thread States

 The following defines specify the various states that a thread can be in.

 @{
*/

/*! Specifies that this thread is current inactive and can be reused */
#define THR_ST_NONE     0

/*! Specifies that this thread is currently in the active queue */
#define THR_ST_ACTIVE   1

/*! Specifies that this thread is currently in the delayed queue */
#define THR_ST_DELAYED  2

/*! Specifies that this thread is currently in the waiting queue */
#define THR_ST_WAITING  3

/*! @} */

/*!
 \addtogroup struct_union_types Struct/Union Types

 The following defines specify the various types of struct/union structures that can exist.

 @{
*/

/*! Specifies a struct */
#define SU_TYPE_STRUCT        0

/*! Specifies a union */
#define SU_TYPE_UNION         1

/*! Specifies a tagged union */
#define SU_TYPE_TAGGED_UNION  2

/*! @} */

/*!
 \addtogroup struct_union_member_types Struct/Union Member Types

 The following defines specify the various types of struct/union member types that can exist.

 @{
*/

/*! Specifies the member is a void type */
#define SU_MEMTYPE_VOID      0

/*! Specifies the member is a signal type */
#define SU_MEMTYPE_SIG       1

/*! Specifies the member is a typedef */
#define SU_MEMTYPE_TYPEDEF   2

/*! Specifies the member is an enumeration */
#define SU_MEMTYPE_ENUM      3

/*! Specifies the member is a struct, union or tagged union */
#define SU_MEMTYPE_SU        4

/*! @} */

/*! Overload for the snprintf function which verifies that we don't overrun character arrays */
//#define snprintf(x,y,...)	assert( snprintf( x, y, __VA_ARGS__ ) < (y) );

/*! Performs time comparison with the sim_time structure */
#define TIME_CMP_LE(x,y)         ((((x).lo <= (y).lo) && ((x).hi <= (y).hi)) || ((x).hi < (y).hi))
#define TIME_CMP_GT(x,y)         (((x).lo > (y).lo) || ((x).hi > (y).hi))
#define TIME_CMP_GE(x,y)         ((((x).lo >= (y).lo) && ((x).hi >= (y).hi)) || ((x).hi > (y).hi))
#define TIME_CMP_NE(x,y)         (((x).lo ^ (y).lo) || ((x).hi ^ (y).hi))

/*! Performs time increment where x is the sim_time structure to increment and y is a 64-bit value to increment to */
#define TIME_INC(x,y)           (x).hi+=((0xffffffff-(x).lo)<(y).lo)?((y).hi+1):(y).hi; (x).lo+=(y).lo; (x).full+=(y).full;

/*! Compares two float values */
#define FEQ(x,y)           (fabs(x-y) < FLT_EPSILON)
      
/*! Compares two double values */
#define DEQ(x,y)           (fabs(x-y) < DBL_EPSILON)

/*!
 Defines boolean variables used in most functions.
*/
typedef enum {
  FALSE,   /*!< Boolean false value */
  TRUE     /*!< Boolean true value */
} bool;

/*!
 Create an 8-bit unsigned value.
*/
#if SIZEOF_CHAR == 1
typedef unsigned char uint8;
#define UINT8(x) x
#define ato8(x)  atoi(x)
#define FMT8    "hh"
#elif SIZEOF_SHORT == 1
typedef unsigned short uint8;
#define UINT8(x) x
#define ato8(x)  atoi(x)
#define FMT8     "h"
#elif SIZEOF_INT == 1
typedef unsigned int uint8;
#define UINT8(x) x
#define ato8(x)  atoi(x)
#define FMT8     ""
#elif SIZEOF_LONG == 1
typedef unsigned long uint8;
#define UINT8(x) x
#define ato8(x)  atol(x)
#define FMT8     "l"
#elif SIZEOF_LONG_LONG == 1
typedef unsigned long long uint8;
#define UINT8(x) x ## LL
#define ato8(x)  atoll(x)
#define FMT8     "ll"
#else
#error "Unable to find an 8-bit data type"
#endif

/*!
 Create an 16-bit unsigned value.
*/
#if SIZEOF_CHAR == 2
typedef unsigned char uint16;
#define UINT16(x) x
#define ato16(x)  atoi(x)
#define FMT16     "hh"
#elif SIZEOF_SHORT == 2
typedef unsigned short uint16;
#define UINT16(x) x
#define ato16(x)  atoi(x)
#define FMT16     "h"
#elif SIZEOF_INT == 2
typedef unsigned int uint16;
#define UINT16(x) x
#define ato16(x)  atoi(x)
#define FMT16     ""
#elif SIZEOF_LONG == 2
typedef unsigned long uint16;
#define UINT16(x) x
#define ato16(x)  atol(x)
#define FMT16     "l"
#elif SIZEOF_LONG_LONG == 2
typedef unsigned long long uint16;
#define UINT16(x) x ## LL
#define ato16(x)  atoll(x)
#define FMT16     "ll"
#else
#error "Unable to find a 16-bit data type"
#endif

/*!
 Create a 32-bit unsigned value.
*/
#if SIZEOF_CHAR == 4
typedef unsigned char uint32;
#define UINT32(x) x
#define ato32(x)  atoi(x)
#define FMT32     "hh"
#elif SIZEOF_SHORT == 4
typedef unsigned short uint32;
#define UINT32(x) x
#define ato32(x)  atoi(x)
#define FMT32     "h"
#elif SIZEOF_INT == 4
typedef unsigned int uint32;
#define UINT32(x) x
#define ato32(x)  atoi(x)
#define FMT32     ""
#elif SIZEOF_LONG == 4
typedef unsigned long uint32;
#define UINT32(x) x
#define ato32(x)  atol(x)
#define FMT32     "l"
#elif SIZEOF_LONG_LONG == 4
typedef unsigned long long uint32;
#define UINT32(x) x ## LL
#define ato32(x)  atoll(x)
#define FMT32     "ll"
#else
#error "Unable to find a 32-bit data type"
#endif

/*!
 Create a 32-bit real value.
*/
#if SIZEOF_FLOAT == 4
typedef float real32;
#elif SIZEOF_DOUBLE == 4
typedef double real32;
#else
#error "Unable to find a 32-bit real data type"
#endif

/*!
 Create a 64-bit unsigned value.
*/
#if SIZEOF_CHAR == 8
typedef char int64;
typedef unsigned char uint64;
#define UINT64(x) x
#define ato64(x)  atoi(x)
#define FMT64     "hh"
#elif SIZEOF_SHORT == 8
typedef short int64;
typedef unsigned short uint64;
#define UINT64(x) x
#define ato64(x)  atoi(x)
#define FMT64     "h"
#elif SIZEOF_INT == 8
typedef int int64;
typedef unsigned int uint64;
#define UINT64(x) x
#define ato64(x)  atoi(x)
#define FMT64     ""
#elif SIZEOF_LONG == 8
typedef long int64;
typedef unsigned long uint64;
#define UINT64(x) x
#define ato64(x)  atol(x)
#define FMT64     "l"
#elif SIZEOF_LONG_LONG == 8
typedef long long int64;
typedef unsigned long long uint64;
#define UINT64(x) x ## LL
#define ato64(x)  atoll(x)
#define FMT64     "ll"
#else
#error "Unable to find a 64-bit data type"
#endif

/*!
 Create a 64-bit real value.
*/
#if SIZEOF_FLOAT == 8
typedef float real64;
#elif SIZEOF_DOUBLE == 8
typedef double real64;
#else
#error "Unable to find a 64-bit real data type"
#endif

/*!
 Machine-dependent value.
*/
typedef unsigned long ulong;

/*!
 Create defines for unsigned long.
*/
#if SIZEOF_LONG == 1
#define UL_SET     (unsigned long)0xff
#define UL_DIV_VAL 3
#define UL_MOD_VAL 0x7
#define UL_BITS    8
#elif SIZEOF_LONG == 2
#define UL_SET     (unsigned long)0xffff
#define UL_DIV_VAL 4
#define UL_MOD_VAL 0xf
#define UL_BITS    16
#elif SIZEOF_LONG == 4
#define UL_SET     (unsigned long)0xffffffff
#define UL_DIV_VAL 5
#define UL_MOD_VAL 0x1f
#define UL_BITS    32
#elif SIZEOF_LONG == 8
#define UL_SET     (unsigned long)0xffffffffffffffffLL
#define UL_DIV_VAL 6
#define UL_MOD_VAL 0x3f
#define UL_BITS    64
#else
#error "Unsigned long is of an unsupported size"
#endif

/*! Divides a bit position by an unsigned long */
#define UL_DIV(x)  (((unsigned int)x) >> UL_DIV_VAL)

/*! Mods a bit position by an unsigned long */
#define UL_MOD(x)  (((unsigned int)x) &  UL_MOD_VAL)

/*------------------------------------------------------------------------------*/

union esuppl_u;

/*!
 Renaming esuppl_u for naming convenience.
*/
typedef union esuppl_u esuppl;

/*!
 A esuppl is a 32-bit value that represents the supplemental field of an expression.
*/
union esuppl_u {
  uint32   all;               /*!< Controls all bits within this union */
  struct {
 
    /* MASKED BITS */
    uint32 swapped        :1;  /*!< Bit 0.  Indicates that the children of this expression have been
                                    swapped.  The swapping of the positions is performed by the score command (for
                                    multi-bit selects) and this bit indicates to the report code to swap them back
                                    when displaying them in. */
    uint32 root           :1;  /*!< Bit 1.  Indicates that this expression is a root expression.
                                    Traversing to the parent pointer will take you to a statement type. */
    uint32 false          :1;  /*!< Bit 2.  Indicates that this expression has evaluated to a value
                                    of FALSE during the lifetime of the simulation. */
    uint32 true           :1;  /*!< Bit 3.  Indicates that this expression has evaluated to a value
                                    of TRUE during the lifetime of the simulation. */
    uint32 left_changed   :1;  /*!< Bit 4.  Indicates that this expression has its left child
                                    expression in a changed state during this timestamp. */
    uint32 right_changed  :1;  /*!< Bit 5.  Indicates that this expression has its right child
                                    expression in a changed state during this timestamp. */
    uint32 eval_00        :1;  /*!< Bit 6.  Indicates that the value of the left child expression
                                    evaluated to FALSE and the right child expression evaluated to FALSE. */
    uint32 eval_01        :1;  /*!< Bit 7.  Indicates that the value of the left child expression
                                    evaluated to FALSE and the right child expression evaluated to TRUE. */
    uint32 eval_10        :1;  /*!< Bit 8.  Indicates that the value of the left child expression
                                    evaluated to TRUE and the right child expression evaluated to FALSE. */
    uint32 eval_11        :1;  /*!< Bit 9.  Indicates that the value of the left child expression
                                    evaluated to TRUE and the right child expression evaluated to TRUE. */
    uint32 lhs            :1;  /*!< Bit 10.  Indicates that this expression exists on the left-hand
                                    side of an assignment operation. */
    uint32 in_func        :1;  /*!< Bit 11.  Indicates that this expression exists in a function */
    uint32 owns_vec       :1;  /*!< Bit 12.  Indicates that this expression either owns its vector
                                    structure or shares it with someone else. */
    uint32 excluded       :1;  /*!< Bit 13.  Indicates that this expression should be excluded from
                                    coverage results.  If a parent expression has been excluded, all children expressions
                                    within its tree are also considered excluded (even if their excluded bits are not
                                    set. */
    uint32 type           :3;  /*!< Bits 16:14.  Indicates how the pointer element should be treated as */
    uint32 base           :3;  /*!< Bits 19:17.  When the expression op is a STATIC, specifies the base
                                    type of the value (DECIMAL, HEXIDECIMAL, OCTAL, BINARY, QSTRING). */
    uint32 clear_changed  :1;  /*!< Bit 20.  Specifies the value of the left/right changed bits after
                                    the left/right subexpression has been performed.  This value should be set to 1 if
                                    a child expression needs to be re-evaluated each time the current expression is
                                    evaluated; otherwise, it should be set to 0. */
    uint32 parenthesis    :1;  /*!< Bit 21.  Specifies that the expression was surrounded by parenthesis
                                    in the original Verilog. */
    uint32 for_cntrl      :2;  /*!< Bit 23:22.  Specifies whether this expression represents a control within a FOR loop
                                    and, if so, what part of the FOR loop this expression represents.  0=Not within a for
                                    loop, 1=FOR loop initializer, 2=FOR loop condition, 3=FOR loop iterator. */
 
    /* UNMASKED BITS */
    uint32 eval_t         :1;  /*!< Bit 24.  Indicates that the value of the current expression is
                                    currently set to TRUE (temporary value). */
    uint32 eval_f         :1;  /*!< Bit 25.  Indicates that the value of the current expression is
                                    currently set to FALSE (temporary value). */
    uint32 comb_cntd      :1;  /*!< Bit 26.  Indicates that the current expression has been previously
                                    counted for combinational coverage.  Only set by report command (therefore this bit
                                    will always be a zero when written to CDD file. */
    uint32 exp_added      :1;  /*!< Bit 27.  Temporary bit value used by the score command but not
                                    displayed to the CDD file.  When this bit is set to a one, it indicates to the
                                    db_add_expression function that this expression and all children expressions have
                                    already been added to the functional unit expression list and should not be added again. */
    uint32 owned          :1;  /*!< Bit 28.  Temporary value used by the score command to indicate
                                    if this expression is already owned by a mod_parm structure. */
    uint32 gen_expr       :1;  /*!< Bit 29.  Temporary value used by the score command to indicate
                                    that this expression is a part of a generate expression. */
    uint32 prev_called    :1;  /*!< Bit 30.  Temporary value used by named block and task expression
                                    functions to indicate if we are in the middle of executing a named block or task
                                    expression (since these cause a context switch to occur. */
    uint32 nba            :1;  /*!< Bit 31.  Specifies that this expression is on the left-hand-side of a non-blocking
                                    assignment. */
  } part;
};

/*------------------------------------------------------------------------------*/

union ssuppl_u;

/*!
 Renaming ssuppl_u field for convenience.
*/
typedef union ssuppl_u ssuppl;

/*!
 Supplemental signal information.
*/
union ssuppl_u {
  uint32 all;
  struct {
    uint32 col            :16; /*!< Specifies the starting column this signal is declared on */
    uint32 type           :5;  /*!< Specifies signal type (see \ref ssuppl_type for legal values) */
    uint32 big_endian     :1;  /*!< Specifies if this signal is in big or little endianness */
    uint32 excluded       :1;  /*!< Specifies if this signal should be excluded for toggle coverage */
    uint32 not_handled    :1;  /*!< Specifies if this signal is handled by Covered or not */
    uint32 assigned       :1;  /*!< Specifies that this signal will be assigned from simulated results (instead of dumpfile) */
    uint32 mba            :1;  /*!< Specifies that this signal MUST be assigned from simulated results because this information
                                     is NOT provided in the dumpfile */
    uint32 implicit_size  :1;  /*!< Specifies that this signal has not been given a specified size by the user at the current time */
  } part;
};

/*------------------------------------------------------------------------------*/

union psuppl_u;

/*!
 Renaming psuppl_u field for convenience.
*/
typedef union psuppl_u psuppl;

/*!
 Supplemental module parameter information.
*/
union psuppl_u {
  uint32 all;
  struct {
    uint32 order    : 16;      /*!< Specifies the parameter order number in its module */
    uint32 type     : 3;       /*!< Specifies the parameter type (see \ref param_suppl_types for legal value) */
    uint32 owns_expr: 1;       /*!< Specifies the parameter is the owner of the associated expression */
    uint32 dimension: 10;      /*!< Specifies the signal dimension that this module parameter resolves */
  } part;
};

/*------------------------------------------------------------------------------*/

union isuppl_u;

/*!
 Renaming isuppl_u field for convenience.
*/
typedef union isuppl_u isuppl;

/*!
 Supplemental field for information line in CDD file.
*/
union isuppl_u {
  uint32 all;
  struct {
    uint32 scored        : 1;  /*!< Bit 0.    Specifies if the design has been scored yet */
    uint32 excl_assign   : 1;  /*!< Bit 1.    Specifies if assign statements are being excluded from coverage */
    uint32 excl_always   : 1;  /*!< Bit 2.    Specifies if always statements are being excluded from coverage */
    uint32 excl_init     : 1;  /*!< Bit 3.    Specifies if initial statements are being excluded from coverage */
    uint32 excl_final    : 1;  /*!< Bit 4.    Specifies if final statements are being excluded from coverage */
    uint32 excl_pragma   : 1;  /*!< Bit 5.    Specifies if code encapsulated in coverage pragmas should be excluded from coverage */
    uint32 assert_ovl    : 1;  /*!< Bit 6.    Specifies that OVL assertions should be included for coverage */
    uint32 vec_ul_size   : 2;  /*!< Bit 8:7.  Specifies the bit size of a vector element (0=8 bits, 1=16-bits, 2=32-bits, 3=64-bits) */
    uint32 inlined       : 1;  /*!< Bit 9.    Specifies if this CDD is used with an inlined code coverage method */
    uint32 scored_line   : 1;  /*!< Bit 10.   Specifies that line coverage was scored and is available for reporting */
    uint32 scored_toggle : 1;  /*!< Bit 11.   Specifies that toggle coverage was scored and is available for reporting */
    uint32 scored_memory : 1;  /*!< Bit 12.   Specifies that memory coverage was scored and is available for reporting */
    uint32 scored_comb   : 1;  /*!< Bit 13.   Specifies that combinational logic coverage was scored and is available for reporting */
    uint32 scored_fsm    : 1;  /*!< Bit 14.   Specifies that FSM coverage was scored and is available for reporting */
    uint32 scored_assert : 1;  /*!< Bit 15.   Specifies that assertion coverage was scored and is available for reporting */
    uint32 scored_events : 1;  /*!< Bit 16.   Specifies that combinational logic event coverage was scored and is available for reporting */
    uint32 verilator     : 1;  /*!< Bit 17.   Specifies that the CDD was generated for use with the Verilator simulator */
  } part;
};

/*------------------------------------------------------------------------------*/

union vsuppl_u;

/*!
 Renaming vsuppl_u field for convenience.
*/
typedef union vsuppl_u vsuppl;

/*!
 Supplemental field for vector structure.
*/
union vsuppl_u {
  uint8   all;                    /*!< Allows us to set all bits in the suppl field */
  struct {
    uint8 type      :2;           /*!< Specifies what type of information is stored in this vector
                                        (see \ref vector_types for legal values) */
    uint8 data_type :2;           /*!< Specifies what the size/type of a single value entry is */
    uint8 owns_data :1;           /*!< Specifies if this vector owns its data array or not */
    uint8 is_signed :1;           /*!< Specifies that this vector should be treated as a signed value */
    uint8 is_2state :1;           /*!< Specifies that this vector should be treated as a 2-state value */
    uint8 set       :1;           /*!< Specifies if this vector's data has been set previously */
  } part;
};

/*------------------------------------------------------------------------------*/

union fsuppl_u;

/*!
 Renaming fsuppl_u field for convenience.
*/
typedef union fsuppl_u fsuppl;

/*!
 Supplemental field for FSM table structure.
*/
union fsuppl_u {
  uint8 all;                      /*!< Allows us to set all bits in the suppl field */
  struct {
    uint8 known : 1;              /*!< Specifies if the possible state transitions are known */
  } part;
};

/*------------------------------------------------------------------------------*/

union asuppl_u;

/*!
 Renaming asuppl_u field for convenience.
*/
typedef union asuppl_u asuppl;

/*!
 Supplemental field for FSM table structure.
*/
union asuppl_u {
  uint8 all;                      /*!< Allows us to set all bits in the suppl field */
  struct {
    uint8 hit      : 1;           /*!< Specifies if from->to arc was hit */
    uint8 excluded : 1;           /*!< Specifies if from->to transition should be excluded from coverage consideration */
  } part;
};

/*------------------------------------------------------------------------------*/

union usuppl_u;

/*!
 Renaming usuppl_u field for convenience.
*/
typedef union usuppl_u usuppl;

/*!
 Supplemental field for a functional unit structure.
*/
union usuppl_u {
  uint8 all;                 /*!< Allows us to set all bits in the suppl field */
  struct {
    uint8 type     : 3;      /*!< Bits 0:2.  Mask = 1.  Specifies the functional unit type (see \ref func_unit_types for legal values) */
    uint8 etype    : 1;      /*!< Bit 3.     Mask = 0.  Set to 0 if elem should be treated as a thread pointer; set to 1 if elem should
                                                        be treated as a thread list pointer. */
    uint8 included : 1;      /*!< Bit 4.     Mask = 1.  Set to 1 if the current functional unit has been included into a file via the
                                                        `include preprocessor command */
    uint8 staticf  : 1;      /*!< Bit 5.     Mask = 1.  Set to 1 if this functional unit is used as a static function */
    uint8 normalf  : 1;      /*!< Bit 6.     Mask = 1.  Set to 1 if this functional unit is used as a normal function */
    uint8 fork     : 1;      /*!< Bit 7.     Mask = 0.  Set to 1 if the functional unit is used for forking */
  } part;
};

/*!
 Mask used for writing the functional unit mask.
*/
#define FUNIT_MASK     0x77

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION DECLARATIONS  */

union  expr_stmt_u;
struct exp_info_s;
struct str_link_s;
struct rv64_s;
struct rv32_s;
struct vector_s;
struct const_value_s;
struct vecblk_s;
struct exp_dim_s;
struct expression_s;
struct vsignal_s;
struct fsm_s;
struct fsm_table_arc_s;
struct fsm_table_s;
struct statement_s;
struct stmt_iter_s;
struct stmt_link_s;
struct stmt_loop_link_s;
struct statistic_s;
struct mod_parm_s;
struct inst_parm_s;
struct fsm_arc_s;
struct race_blk_s;
struct func_unit_s;
struct funit_link_s;
struct inst_link_s;
struct sym_sig_s;
struct sym_exp_s;
struct sym_mem_s;
struct symtable_s;
struct static_expr_s;
struct vector_width_s;
struct exp_bind_s;
struct case_stmt_s;
struct case_gitem_s;
struct funit_inst_s;
struct tnode_s;

#ifdef HAVE_SYS_TIME_H
struct timer_s;
#endif

struct fsm_var_s;
struct fv_bind_s;
struct attr_param_s;
struct stmt_blk_s;
struct thread_s;
struct thr_link_s;
struct thr_list_s;
struct perf_stat_s;
struct port_info_s;
struct param_oride_s;
struct gen_item_s;
struct gitem_link_s;
struct typedef_item_s;
struct enum_item_s;
struct sig_range_s;
struct dim_range_s;
struct reentrant_s;
struct rstack_entry_s;
struct struct_union_s;
struct su_member_s;
struct profiler_s;
struct db_s;
struct sim_time_s;
struct comp_cdd_cov_s;
struct exclude_reason_s;
struct stmt_pair_s;
struct gitem_pair_s;
struct dim_and_nba_s;
struct nonblock_assign_s;
struct str_cov_s;

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION TYPEDEFS  */

/*!
 Renaming expression operation information structure for convenience.
*/
typedef struct exp_info_s exp_info;

/*!
 Renaming string link structure for convenience.
*/
typedef struct str_link_s str_link;

/*!
 Renaming rv64 structure for convenience.
*/
typedef struct rv64_s rv64;

/*!
 Renaming rv32 structure for convenience.
*/
typedef struct rv32_s rv32;

/*!
 Renaming vector structure for convenience.
*/
typedef struct vector_s vector;

/*!
 Renaming vector structure for convenience.
*/
typedef struct const_value_s const_value;

/*!
 Renaming vector structure for convenience.
*/
typedef struct vecblk_s vecblk;

/*!
 Renaming expression dimension structure for convenience.
*/
typedef struct exp_dim_s exp_dim;

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
 Renaming FSM table arc structure for convenience.
*/
typedef struct fsm_table_arc_s fsm_table_arc;

/*!
 Renaming FSM table structure for convenience.
*/
typedef struct fsm_table_s fsm_table;

/*!
 Renaming statement structure for convenience.
*/
typedef struct statement_s statement;

/*!
 Renaming statement iterator structure for convenience.
*/
typedef struct stmt_iter_s stmt_iter;

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
 Renaming functional unit instance link structure for convenience.
*/
typedef struct inst_link_s inst_link;

/*!
 Renaming symbol signal structure for convenience.
*/
typedef struct sym_sig_s sym_sig;

/*!
 Renaming symbol expression structure for convenience.
*/
typedef struct sym_exp_s sym_exp;

/*!
 Renaming symbol memory structure for convenience.
*/
typedef struct sym_mem_s sym_mem;

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
 Renaming case generate item structure for convenience.
*/
typedef struct case_gitem_s case_gitem;

/*!
 Renaming functional unit instance structure for convenience.
*/
typedef struct funit_inst_s funit_inst;

/*!
 Renaming tree node structure for convenience.
*/
typedef struct tnode_s tnode;

#ifdef HAVE_SYS_TIME_H
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

/*!
 Renaming thread structure for convenience.
*/
typedef struct thread_s thread;

/*!
 Renaming thr_link structure for convenience.
*/
typedef struct thr_link_s thr_link;

/*!
 Renaming thr_list structure for convenience.
*/
typedef struct thr_list_s thr_list;

/*!
 Renaming perf_stat structure for convenience.
*/
typedef struct perf_stat_s perf_stat;

/*!
 Renaming port_info_s structure for convenience.
*/
typedef struct port_info_s port_info;

/*!
 Renaming param_orides_s structure for convenience.
*/
typedef struct param_oride_s param_oride;

/*!
 Renaming gen_item_s structure for convenience.
*/
typedef struct gen_item_s gen_item;

/*!
 Renaming gitem_link_s structure for convenience.
*/
typedef struct gitem_link_s gitem_link;

/*!
 Renaming typedef_item_s structure for convenience.
*/
typedef struct typedef_item_s typedef_item;

/*!
 Renaming enum_item_s structure for convenience.
*/
typedef struct enum_item_s enum_item;

/*!
 Renaming sig_range_s structure for convenience.
*/
typedef struct sig_range_s sig_range;

/*!
 Renaming dim_range_s structure for convenience.
*/
typedef struct dim_range_s dim_range;

/*!
 Renaming reentrant_s structure for convenience.
*/
typedef struct reentrant_s reentrant;

/*!
 Renaming rstack_entry_s structure for convenience.
*/
typedef struct rstack_entry_s rstack_entry;

/*!
 Renaming struct_unions_s structure for convenience.
*/
typedef struct struct_union_s struct_union;

/*!
 Renaming su_member_s structure for convenience.
*/
typedef struct su_member_s su_member;

/*!
 Renaming profiler_s structure for convenience.
*/
typedef struct profiler_s profiler;

/*!
 Renaming db_s structure for convenience.
*/
typedef struct db_s db;

/*!
 Renaming sim_time_s structure for convenience.
*/
typedef struct sim_time_s sim_time;

/*!
 Renaming comp_cdd_cov_s structure for convenience.
*/
typedef struct comp_cdd_cov_s comp_cdd_cov;

/*!
 Renaming exclude_reason_s structure for convenience.
*/
typedef struct exclude_reason_s exclude_reason;

/*!
 Renaming stmt_pair_s structure for convenience.
*/
typedef struct stmt_pair_s stmt_pair;

/*!
 Renaming gitem_pair_s structure for convenience.
*/
typedef struct gitem_pair_s gitem_pair;

/*!
 Renaming dim_and_nba_s structure for convenience.
*/
typedef struct dim_and_nba_s dim_and_nba;

/*!
 Renaming nonblock_assign_s structure for convenience.
*/
typedef struct nonblock_assign_s nonblock_assign;

/*!
 Renaming str_cov_s structure for convenience.
*/
typedef struct str_cov_s str_cov;

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION DEFINITIONS  */

/*!
 Representation of simulation time -- used for performance enhancement purposes.
*/
struct sim_time_s {
  unsigned int lo;                   /*!< Lower 32-bits of the current time */
  unsigned int hi;                   /*!< Upper 32-bits of the current time */
  uint64       full;                 /*!< Full 64 bits of the current time - for displaying purposes only */
  bool         final;                /*!< Specifies if this is the final simulation timestep */
};

/*!
 Contains static information about each expression operation type.
*/
struct exp_info_s {
  char* name;                             /*!< Operation name */
  char* op_str;                           /*!< Operation string name for report output purposes */
  bool  (*func)( expression*, thread*, const sim_time* );  /*!< Operation function to call */
  struct {
    uint32 is_event:1;                    /*!< Specifies if operation is an event */
    uint32 is_static:1;                   /*!< Specifies if operation is a static value (does not change during simulation) */
    uint32 is_comb:2;                     /*!< Specifies if operation is combinational (both left/right expressions valid) */
    uint32 is_unary:1;                    /*!< Specifies if operation is unary (left expression valid only) */
    uint32 measurable:1;                  /*!< Specifies if this operation type can be measured */
    uint32 is_context_switch:1;           /*!< Specifies if this operation will cause a context switch */
    uint32 assignable:1;                  /*!< Specifies if this operation can be immediately assigned (i.e., +=) */
    uint32 tmp_vecs:3;                    /*!< Number of temporary vectors used by this expression */
    uint32 real_op:2;                     /*!< Specifies if this operation can have a real value result. 0=no, 1=only right, 2=only left, 3=either */
  } suppl;                                /*!< Supplemental information about this expression */
};

/*!
 Specifies an element in a linked list containing string values.  This data
 structure allows us to add new elements to the list without resizing, this
 optimizes performance with small amount of overhead.
*/
struct str_link_s {
  char*         str;                 /*!< String to store */
  char*         str2;                /*!< Addition string to store */
  uint32        suppl;               /*!< 32-bit additional information */
  uint32        suppl2;              /*!< 32-bit additional information */
  uint8         suppl3;              /*!< 8-bit additional information */
  vector_width* range;               /*!< Pointer to optional range information */
  str_link*     next;                /*!< Pointer to next str_link element */
};

/*!
 This structure contains the contents needed to store and display the floating point value used
 within the design.
*/
struct rv64_s {
  char*  str;                        /*!< Specifies the original static string value */
  real64 val;                        /*!< Specifies 64-bit double-precision floating point value */
};

/*!
 This structure contains the contents needed to store and display the floating point value used
 within the design.
*/
struct rv32_s {
  char*  str;                        /*!< Specifies the original static string value */
  real32 val;                        /*!< Specifies 32-bit double-precision floating point value */
};

/*!
 Contains information for signal value.  This value is represented as
 a generic vector.  The vector.h/.c files contain the functions that
 manipulate this information.
*/
struct vector_s {
  unsigned int width;                /*!< Bit width of this vector */
  vsuppl       suppl;                /*!< Supplemental field */
  union {
    ulong** ul;                      /*!< Machine sized unsigned integer array for value, signal, expression and memory types */
    rv64*   r64;                     /*!< 64-bit floating point value */
    rv32*   r32;                     /*!< 32-bit floating point value (shortreal) */
  } value;
};

/*!
 Contains information about a parsed constant value including its data and base type.
*/
struct const_value_s {
  vector* vec;                       /*!< Pointer to vector containing constant value */
  int     base;                      /*!< Base type of constant value */
};

/*!
 Contains temporary storage vectors used by certain expressions for performance purposes.
*/
struct vecblk_s {
  vector vec[5];                     /*!< Vector array */
  int    index;                      /*!< Specifies to the called function which vector may be accessed */
}; 

/*!
 Specifies the current dimensional LSB of the associated expression.  This is used by the DIM, SBIT, MBIT,
 MBIT_POS and MBIT_NEG expressions types for proper bit selection of given signal.
*/
struct exp_dim_s {
  int  curr_lsb;                     /*!< Calculated LSB of this expression dimension (if -1, LSB was an X) */
  int  dim_lsb;                      /*!< Dimensional LSB (static value) */
  bool dim_be;                       /*!< Dimensional big endianness (static value) */
  int  dim_width;                    /*!< Dimensional width of current expression (static value) */
  bool last;                         /*!< Specifies if this is the dimension that should handle the signal interaction */
};

/*!
 Allows the parent pointer of an expression to point to either another expression
 or a statement.
*/
union expr_stmt_u {
  expression* expr;                  /*!< Pointer to expression */
  statement*  stmt;                  /*!< Pointer to statement */
};

/*!
 An expression is defined to be a logical combination of signals/values.  Expressions may
 contain subexpressions (which are expressions in and of themselves).  An measurable expression
 may only evaluate to TRUE (1) or FALSE (0).  If the parent expression of this expression is
 NULL, then this expression is considered a root expression.  The suppl contains the
 run-time information for its expression.
*/
struct expression_s {
  vector*      value;              /*!< Current value and toggle information of this expression */
  exp_op_type  op;                 /*!< Expression operation type */
  esuppl       suppl;              /*!< Supplemental information for the expression */
  int          id;                 /*!< Specifies unique ID for this expression in the parent */
  int          ulid;               /*!< Specifies underline ID for reporting purposes */
  unsigned int line;               /*!< Specified first line in file that this expression is found on */
  unsigned int ppfline;            /*!< Specifies the first line number in the preprocessed file */
  unsigned int pplline;            /*!< Specifies the last line number in the preprocessed file */
  uint32       exec_num;           /*!< Specifies the number of times this expression was executed during simulation */
  union {
    uint32 all;
    struct {
      uint32 last  : 16;           /*!< Column position of the end of the expression */
      uint32 first : 16;           /*!< Column position of the beginning of the expression */
    } part;
  } col;                           /*!< Specifies column location of beginning/ending of expression */
  vsignal*     sig;                /*!< Pointer to signal.  If NULL then no signal is attached */
  char*        name;               /*!< Name of signal/function/task for output purposes (only valid if we are binding
                                        to a signal, task or function */
  expr_stmt*   parent;             /*!< Parent expression/statement */
  expression*  right;              /*!< Pointer to expression on right */
  expression*  left;               /*!< Pointer to expression on left */
  fsm*         table;              /*!< Pointer to FSM table associated with this expression */
  union {
    func_unit*   funit;            /*!< Pointer to task/function to be called by this expression */
    thread*      thr;              /*!< Pointer to next thread to be called */
    uint64*      scale;            /*!< Pointer to parent functional unit's timescale value */
    vecblk*      tvecs;            /*!< Temporary vectors that are sized to match value */
    exp_dim*     dim;              /*!< Current dimensional LSB of this expression (valid for DIM, SBIT_SEL, MBIT_SEL, MBIT_NEG and MBIT_POS) */
    dim_and_nba* dim_nba;          /*!< Dimension and non-blocking assignment information */
  } elem;
};

/*!
 Stores all information needed to represent a signal.  If value of value element is non-zero at the
 end of the run, this signal has been simulated.
*/
struct vsignal_s {
  int          id;                   /*!< Numerical identifier that is unique from all other signals (used for exclusions) */
  char*        name;                 /*!< Full hierarchical name of signal in design */
  int          line;                 /*!< Specifies line number that this signal was declared on */
  ssuppl       suppl;                /*!< Supplemental information for this signal */
  vector*      value;                /*!< Pointer to vector value of this signal */
  unsigned int pdim_num;             /*!< Number of packed dimensions in pdim array */
  unsigned int udim_num;             /*!< Number of unpacked dimensions in pdim array */
  dim_range*   dim;                  /*!< Unpacked/packed dimension array */
  expression** exps;                 /*!< Expression array */
  unsigned int exp_size;             /*!< Number of elements in the expression array */
};

/*!
 Stores information for an FSM within the design.
*/
struct fsm_s {
  char*       name;                  /*!< User-defined name that this FSM pertains to */
  expression* from_state;            /*!< Pointer to from_state expression */
  expression* to_state;              /*!< Pointer to to_state expression */
  fsm_arc*    arc_head;              /*!< Pointer to head of list of expression pairs that describe the valid FSM arcs */
  fsm_arc*    arc_tail;              /*!< Pointer to tail of list of expression pairs that describe the valid FSM arcs */
  fsm_table*  table;                 /*!< FSM arc traversal table */
  bool        exclude;               /*!< Set to TRUE if the states/transitions of this table should be excluded as determined by pragmas */
};

/*!
 Stores information for a uni-/bi-directional state transition.
*/
struct fsm_table_arc_s {
  asuppl       suppl;                /*!< Supplemental field for this state transition entry */
  unsigned int from;                 /*!< Index to from_state vector value in fsm_table vector array */
  unsigned int to;                   /*!< Index to to_state vector value in fsm_table vector array */
};

/*!
 Stores information for an FSM table (including states and state transitions).
*/
struct fsm_table_s {
  fsuppl          suppl;             /*!< Supplemental field for FSM table */
  unsigned int    id;                /*!< Starting exclusion ID of arc list */
  vector**        fr_states;         /*!< List of FSM from state vectors that are valid for this FSM (VTYPE_VAL) */
  unsigned int    num_fr_states;     /*!< Contains the number of from states stored in this table */
  vector**        to_states;         /*!< List of FSM to state vectors that are valid for this FSM (VTYPE_VAL) */
  unsigned int    num_to_states;     /*!< Contains the number of to states stored in this table */
  fsm_table_arc** arcs;              /*!< List of FSM state transitions */
  unsigned int    num_arcs;          /*!< Contains the number of arcs stored in this table */
};

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
struct statement_s {
  expression* exp;                   /*!< Pointer to associated expression tree */
  statement*  next_true;             /*!< Pointer to next statement to run if expression tree non-zero */
  statement*  next_false;            /*!< Pointer to next statement to run if next_true not picked */
  statement*  head;                  /*!< Pointer to head statement in this block */
  int         conn_id;               /*!< Current connection ID (used to make sure that we do not infinitely loop
                                          in connecting statements together) */
  func_unit*  funit;                 /*!< Pointer to statement's functional unit that it belongs to */
  union {
    uint32  all;
    struct {
      /* Masked bits */
      uint32  head      :1;          /*!< Bit 0.  Mask bit = 1.  Indicates the statement is a head statement */
      uint32  stop_true :1;          /*!< Bit 1.  Mask bit = 1.  Indicates the statement which this expression belongs
                                          should write itself to the CDD and not continue to traverse its next_true pointer. */
      uint32  stop_false:1;          /*!< Bit 2.  Mask bit = 1.  Indicates the statement which this expression belongs
                                          should write itself to the CDD and not continue to traverse its next_false pointer. */
      uint32  cont      :1;          /*!< Bit 3.  Mask bit = 1.  Indicates the statement which this expression belongs is
                                          part of a continuous assignment.  As such, stop simulating this statement tree
                                          after this expression tree is evaluated. */
      uint32  is_called :1;          /*!< Bit 4.  Mask bit = 1.  Indicates that this statement is called by a FUNC_CALL,
                                          TASK_CALL, NB_CALL or FORK statement.  If a statement has this bit set, it will NOT
                                          be automatically placed in the thread queue at time 0. */
      uint32  excluded  :1;          /*!< Bit 5.  Mask bit = 1.  Indicates that this statement (and its associated expression
                                          tree) should be excluded from coverage results. */
      uint32  final     :1;          /*!< Bit 6.  Mask bit = 1.  Indicates that this statement block should only be executed
                                          during the final simulation step. */
      uint32  ignore_rc :1;          /*!< Bit 7.  Mask bit = 1.  Specifies that we should ignore race condition checking for
                                          this statement. */

      /* Unmasked bits */
      uint32  added     :1;          /*!< Bit 8.  Mask bit = 0.  Temporary bit value used by the score command but not
                                          displayed to the CDD file.  When this bit is set to a one, it indicates to the
                                          db_add_statement function that this statement and all children statements have
                                          already been added to the functional unit statement list and should not be added again. */
    } part;
  } suppl;                           /*!< Supplemental bits for statements */
};

/*!
 Statement link iterator.
*/
struct stmt_iter_s {
  stmt_link* curr;                   /*!< Pointer to current statement link */
  stmt_link* last;                   /*!< Two-way pointer to next/previous statement link */
};

/*!
 Statement link element.  Stores pointer to a statement.
*/
struct stmt_link_s {
  statement* stmt;                   /*!< Pointer to statement */
  stmt_link* next;                   /*!< Pointer to next statement element in list */
  bool       rm_stmt;                /*!< Set to TRUE if the associated statement should be removed */
};

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
struct stmt_loop_link_s {
  statement*      stmt;              /*!< Pointer to last statement in loop */
  int             id;                /*!< ID of next statement after last */
  int             type;              /*!< Specifies if the ID is for next_true, next_false or head */
  stmt_loop_link* next;              /*!< Pointer to next statement in stack */
};

/*!
 Contains statistics for coverage results which is stored in a functional unit instance.
*/
struct statistic_s {
  unsigned int line_hit;                    /*!< Number of lines executed during simulation */
  unsigned int line_excluded;               /*!< Number of excluded lines */
  unsigned int line_total;                  /*!< Total number of lines parsed */
  unsigned int tog01_hit;                   /*!< Number of bits toggling from 0 to 1 */
  unsigned int tog10_hit;                   /*!< Number of bits toggling from 1 to 0 */
  unsigned int tog_excluded;                /*!< Number of excluded toggles */
  unsigned int tog_total;                   /*!< Total number of bits to toggle */
  bool         tog_cov_found;               /*!< Specifies if a fully covered signal was found */
  unsigned int comb_hit;                    /*!< Number of logic combinations hit */
  unsigned int comb_excluded;               /*!< Number of excluded logic combinations */
  unsigned int comb_total;                  /*!< Total number of expression combinations */
  int          state_total;                 /*!< Total number of FSM states */
  int          state_hit;                   /*!< Number of FSM states reached */
  int          arc_total;                   /*!< Total number of FSM arcs */
  int          arc_hit;                     /*!< Number of FSM arcs traversed */
  int          arc_excluded;                /*!< Number of excluded arcs */
  unsigned int race_total;                  /*!< Total number of race conditions found */
  unsigned int rtype_total[RACE_TYPE_NUM];  /*!< Total number of each race condition type found */
  unsigned int assert_hit;                  /*!< Number of assertions covered during simulation */
  unsigned int assert_excluded;             /*!< Number of excluded assertions */
  unsigned int assert_total;                /*!< Total number of assertions */
  unsigned int mem_wr_hit;                  /*!< Total number of addressable memory elements written */
  unsigned int mem_rd_hit;                  /*!< Total number of addressable memory elements read */
  unsigned int mem_ae_total;                /*!< Total number of addressable memory elements */
  unsigned int mem_tog01_hit;               /*!< Total number of bits toggling from 0 to 1 in memories */
  unsigned int mem_tog10_hit;               /*!< Total number of bits toggling from 1 to 0 in memories */
  unsigned int mem_tog_total;               /*!< Total number of bits in memories */
  bool         mem_cov_found;               /*!< Specifies if a fully covered memory was found */
  unsigned int mem_excluded;                /*!< Total number of excluded memory coverage points */
  bool         show;                        /*!< Set to TRUE if this module should be output to the report */
};

/*!
 Structure containing parts for a module parameter definition.
*/
struct mod_parm_s {
  char*        name;                 /*!< Name of parameter */
  static_expr* msb;                  /*!< Static expression containing the MSB of the module parameter */
  static_expr* lsb;                  /*!< Static expression containing the LSB of the module parameter */
  bool         is_signed;            /*!< Specifies if the module parameter was labeled as signed */
  expression*  expr;                 /*!< Expression tree containing value of parameter */
  psuppl       suppl;                /*!< Supplemental field */
  expression** exps;                 /*!< Array of expressions for dependents */
  unsigned int exp_size;             /*!< Number of elements in the expression array */
  vsignal*     sig;                  /*!< Pointer to associated signal (if one is available) */
  char*        inst_name;            /*!< Stores name of instance that will have this parameter overriden (only valid for parameter overridding) */
  mod_parm*    next;                 /*!< Pointer to next module parameter in list */
};

/*!
 Structure containing parts for an instance parameter.
*/
struct inst_parm_s {
  vsignal*     sig;                  /*!< Name, MSB, LSB and vector value for this instance parameter */
  char*        inst_name;            /*!< Name of instance to which this structure belongs to */
  mod_parm*    mparm;                /*!< Pointer to base module parameter */
  inst_parm*   next;                 /*!< Pointer to next instance parameter in list */
};

/*!
 Contains information for a single state transition arc in an FSM within the design.
*/
struct fsm_arc_s {
  expression* from_state;            /*!< Pointer to expression that represents the state we are transferring from */
  expression* to_state;              /*!< Pointer to expression that represents the state we are transferring to */
  fsm_arc*    next;                  /*!< Pointer to next fsm_arc in this list */
};

/*!
 Contains information for storing race condition information
*/
struct race_blk_s {
  int       start_line;              /*!< Starting line number of statement block that was found to be a race condition */
  int       end_line;                /*!< Ending line number of statement block that was found to be a race condition */
  int       reason;                  /*!< Numerical reason for why this statement block was found to be a race condition */
  race_blk* next;                    /*!< Pointer to next race block in list */
};

/*!
 Contains information for a functional unit (i.e., module, named block, function or task).
*/
struct func_unit_s {
  usuppl          suppl;             /*!< Supplemental field */
  char*           name;              /*!< Functional unit name */
  int             id;                /*!< Functional unit unique ID */
  char*           orig_fname;        /*!< File name where functional unit exists */
  char*           incl_fname;        /*!< File name where functional unit was included into */
  char*           version;           /*!< Version information for functional unit (if one exists) */
  unsigned int    start_line;        /*!< Starting line number of functional unit in its file */
  unsigned int    end_line;          /*!< Ending line number of functional unit in its file */
  unsigned int    start_col;         /*!< Starting column number of function unit (column of first character of module, task, function or begin keyword) */
  int             ts_unit;           /*!< Timescale unit value */
  uint64          timescale;         /*!< Timescale for this functional unit contents */
  statistic*      stat;              /*!< Pointer to functional unit coverage statistics structure */
  vsignal**       sigs;              /*!< Array of signal pointers that belong to this functional unit */
  unsigned int    sig_size;          /*!< Number of elements in the sigs array */
  unsigned int    sig_no_rm_index;   /*!< Index in sigs array that begins the list of signals that should not be deallocated */
  expression**    exps;              /*!< Array of expression pointers that belong to this functional unit */
  unsigned int    exp_size;          /*!< Number of elements in the exps array */
  statement*      first_stmt;        /*!< Pointer to first head statement in this functional unit (for tasks/functions only) */
  stmt_link*      stmt_head;         /*!< Head pointer to list of statements in this functional unit */
  stmt_link*      stmt_tail;         /*!< Tail pointer to list of statements in this functional unit */
  fsm**           fsms;              /*!< Array of FSM pointers that belong to this functional unit */
  unsigned int    fsm_size;          /*!< Number of elements in the fsms array */
  race_blk*       race_head;         /*!< Head pointer to list of race condition blocks in this functional unit if we are a module */
  race_blk*       race_tail;         /*!< Tail pointer to list of race condition blocks in this functional unit if we are a module */
  mod_parm*       param_head;        /*!< Head pointer to list of parameters in this functional unit if we are a module */
  mod_parm*       param_tail;        /*!< Tail pointer to list of parameters in this functional unit if we are a module */
  gitem_link*     gitem_head;        /*!< Head pointer to list of generate items within this module */
  gitem_link*     gitem_tail;        /*!< Tail pointer to list of generate items within this module */
  func_unit*      parent;            /*!< Pointer to parent functional unit (only valid for functions, tasks and named blocks */
  funit_link*     tf_head;           /*!< Head pointer to list of task/function functional units if we are a module */
  funit_link*     tf_tail;           /*!< Tail pointer to list of task/function functional units if we are a module */
  typedef_item*   tdi_head;          /*!< Head pointer to list of typedef types for this functional unit */
  typedef_item*   tdi_tail;          /*!< Tail pointer to list of typedef types for this functional unit */
  enum_item*      ei_head;           /*!< Head pointer to list of enumerated values for this functional unit */
  enum_item*      ei_tail;           /*!< Tail pointer to list of enumerated values for this functional unit */
  struct_union*   su_head;           /*!< Head pointer to list of struct/unions for this functional unit */
  struct_union*   su_tail;           /*!< Tail pointer to list of struct/unions for this functional unit */
  exclude_reason* er_head;           /*!< Head pointer to list of exclusion reason structures for this functional unit */
  exclude_reason* er_tail;           /*!< Tail pointer to list of exclusion reason structures for this functional unit */
  union {
    thread*   thr;                   /*!< Pointer to a single thread that this statement is associated with */
    thr_list* tlist;                 /*!< Pointer to a list of threads that this statement is currently associated with */
  } elem;                            /*!< Pointer element */
};

/*!
 Linked list element that stores a functional unit (no scope).
*/
struct funit_link_s {
  func_unit*  funit;                 /*!< Pointer to functional unit in list */
  funit_link* next;                  /*!< Next functional unit link in list */
};

/*!
 Linked list element that stores a functional unit instance.
*/
struct inst_link_s {
  funit_inst* inst;                  /*!< Pointer to functional unit instance in list */
  bool        ignore;                /*!< If set to TRUE, causes this instance tree to be ignored from being written/used */
  bool        base;                  /*!< If set to TRUE, this instance tree is a registered base tree */
  inst_link*  next;                  /*!< Next functional unit instance link in list */
};

/*!
 For each signal within a symtable entry, an independent MSB and LSB needs to be
 stored along with the signal pointer that it references to properly assign the
 VCD signal value to the appropriate signal.  This structure is setup to hold these
 three key pieces of information in a list-style data structure.
*/
struct sym_sig_s {
  vsignal* sig;                      /*!< Pointer to signal */
  int      msb;                      /*!< Most significant bit of value to set */
  int      lsb;                      /*!< Least significant bit of value to set */
  sym_sig* next;                     /*!< Pointer to next signal */
};

/*!
 For an expression, the type of coverage action to perform must accompany the pointer to the expression.
*/
struct sym_exp_s {
  expression* exp;                   /*!< Pointer to expression pertaining to this entry */
  char        action;                /*!< Specifies the action to perform to the expression (line=0, logic=1) */
  sym_exp*    next;                  /*!< Pointer to next expression */
};

/*!
 Stores symbol name of signal along with pointer to signal itself into a lookup table
*/
struct symtable_s {
  union {
    sym_sig*   sig;                  /*!< Pointer to signal symtable entry stack */
    sym_exp*   exp;                  /*!< Pointer to expression symtable entry */
    fsm*       table;                /*!< Pointer to FSM table symtable entry */
  } entry;
  char         entry_type;           /*!< Specifies if this entry represents a signal (1), expression (2) or fsm (3) */
  char*        value;                /*!< String representation of last current value */
  unsigned int size;                 /*!< Number of bytes allowed storage for value */
  symtable*    table[94];            /*!< Array of symbol tables for next level (only enough for printable characters) */
};

/*!
 Specifies possible values for a static expression (constant value).
*/
struct static_expr_s {
  expression* exp;                   /*!< Specifies if static value is an expression */
  int         num;                   /*!< Specifies if static value is a numeric value */
};

/*!
 Specifies bit range of a signal or expression.
*/
struct vector_width_s {
  static_expr*  left;                /*!< Specifies left bit value of bit range */
  static_expr*  right;               /*!< Specifies right bit value of bit range */
  bool          implicit;            /*!< Specifies if vector width was explicitly set by user or implicitly set by parser */
};

/*!
 Binds a signal to an expression.
*/
struct exp_bind_s {
  int              type;             /*!< Specifies if name refers to a signal (0), function (FUNIT_FUNCTION) or task (FUNIT_TASK) */
  char*            name;             /*!< Name of Verilog scoped signal/functional unit to bind */
  int              clear_assigned;   /*!< Expression ID of the cleared signal's expression */
  int              line;             /*!< Specifies line of expression -- used when expression is deallocated and we are clearing */
  expression*      exp;              /*!< Expression to bind. */
  expression*      fsm;              /*!< FSM expression to create value for when this expression is bound */
  func_unit*       funit;            /*!< Pointer to functional unit containing expression */
  bool             staticf;          /*!< Set to TRUE if the functional unit being bound to is a result of a static function */
  exp_bind*        next;             /*!< Pointer to next binding in list */
};

/*!
 Binds an expression to a statement.  This is used when constructing a case
 structure.
*/
struct case_stmt_s {
  expression*     expr;              /*!< Pointer to case equality expression */
  statement*      stmt;              /*!< Pointer to first statement in case statement */
  unsigned int    line;              /*!< First line number of case statement */
  unsigned int    ppfline;           /*!< First line number from preprocessed file of case statement */
  unsigned int    pplline;           /*!< Last line number from preprocessed file of case statement */
  int             fcol;              /*!< First column of case statement */
  int             lcol;              /*!< Last column of case statement */
  case_statement* prev;              /*!< Pointer to previous case statement in list */
};

/*!
 Binds an expression to a generate item.  This is used when constructing a case structure
 in a generate block.
*/
struct case_gitem_s {
  expression*     expr;              /*!< Pointer to case equality expression */
  gen_item*       gi;                /*!< Pointer to first generate item in case generate item */
  unsigned int    line;              /*!< Line number of case generate item */
  unsigned int    ppline;            /*!< Line number from preprocessed file of case generate item */
  int             fcol;              /*!< First column of case generate item */
  int             lcol;              /*!< Last column of case generate item */
  case_gitem*     prev;              /*!< Pointer to previous case generate item in list */
};

/*!
 A functional unit instance element in the functional unit instance tree.
*/
struct funit_inst_s {
  char*         name;                /*!< Instance name of this functional unit instance */
  int           id;                  /*!< Unique ID for this instance */
  unsigned int  ppfline;             /*!< First line of instantiation from preprocessor */
  int           fcol;                /*!< First column of instantiation */
  struct {
    uint8 name_diff : 1;             /*!< If set to TRUE, means that this instance name is not accurate due to merging */
    uint8 ignore    : 1;             /*!< If set to TRUE, causes this instance to not be written to the CDD file (used
                                          as a placeholder in the instance tree for functional unit that will be generated
                                          at a later time). */
    uint8 gend_scope: 1;             /*!< Set to 1 if this instance is a generated scope */
  } suppl;                           /*!< Supplemental field for the instance */
  func_unit*    funit;               /*!< Pointer to functional unit this instance represents */
  statistic*    stat;                /*!< Pointer to statistic holder */
  vector_width* range;               /*!< Used to create an array of instances */
  inst_parm*    param_head;          /*!< Head pointer to list of parameter overrides in this functional unit if it is a module */
  inst_parm*    param_tail;          /*!< Tail pointer to list of parameter overrides in this functional unit if it is a module */
  gitem_link*   gitem_head;          /*!< Head pointer to list of generate items used for this instance */
  gitem_link*   gitem_tail;          /*!< Tail pointer to list of generate items used for this instance */
  funit_inst*   parent;              /*!< Pointer to parent instance -- used for convenience only */
  funit_inst*   child_head;          /*!< Pointer to head of child list */
  funit_inst*   child_tail;          /*!< Pointer to tail of child list */
  funit_inst*   next;                /*!< Pointer to next child in parents list */
};

/*!
 Node for a tree that carries two strings:  a key and a value.  The tree is a binary
 tree that is sorted by key.
*/
struct tnode_s {
  char*  name;                       /*!< Key value for tree node */
  char*  value;                      /*!< Value of node */
  tnode* left;                       /*!< Pointer to left child node */
  tnode* right;                      /*!< Pointer to right child node */
  tnode* up;                         /*!< Pointer to parent node */
};

#ifdef HAVE_SYS_TIME_H
/*!
 Structure for holding code timing data.  This information can be useful for optimizing
 code segments.  To use a timer, create a pointer to a timer structure in global
 memory and assign the pointer to the value NULL.  Then simply call timer_start to
 start timing and timer_stop to stop timing.  You may call as many timer_start/timer_stop
 pairs as needed and the total timed time will be kept in the structure's total variable.
 To clear the total for a timer, call timer_clear.
*/
struct timer_s {
  struct timeval start;              /*!< Contains start time of a particular timer */
  uint64         total;              /*!< Contains the total amount of user time accrued for this timer */
};
#endif

/*!
 Contains information for an FSM state variable.
*/
struct fsm_var_s {
  char*       funit;                 /*!< Name of functional unit containing FSM variable */
  char*       name;                  /*!< Name associated with this FSM variable */
  expression* ivar;                  /*!< Pointer to input state expression */
  expression* ovar;                  /*!< Pointer to output state expression */
  vsignal*    iexp;                  /*!< Pointer to input signal matching ovar name */
  fsm*        table;                 /*!< Pointer to FSM containing signal from ovar */
  bool        exclude;               /*!< Set to TRUE if the associated FSM needs to be excluded from coverage consideration */
  fsm_var*    next;                  /*!< Pointer to next fsm_var element in list */
};

/*!
 Binding structure for FSM variables to their expressions.
*/
struct fv_bind_s {
  char*       sig_name;              /*!< Name of signal to bind to expression */
  expression* expr;                  /*!< Pointer to expression to bind to signal */
  char*       funit_name;            /*!< Name of functional unit to find sig_name and expression */
  statement*  stmt;                  /*!< Pointer to statement which contains root of expr */
  fv_bind*    next;                  /*!< Pointer to next FSM variable bind element in list */
};

/*!
 Container for a Verilog-2001 attribute.
*/
struct attr_param_s {
  char*       name;                  /*!< Name of attribute parameter identifier */
  expression* expr;                  /*!< Pointer to expression assigned to the attribute parameter identifier */
  int         index;                 /*!< Index position in the array that this parameter is located at */
  attr_param* next;                  /*!< Pointer to next attribute parameter in list */
  attr_param* prev;                  /*!< Pointer to previous attribute parameter in list */
};

/*!
 The stmt_blk structure contains extra information for a given signal that is only used during
 the parsing stage (therefore, the information does not need to be specified in the vsignal
 structure itself) and is used to keep track of information about that signal's use in the current
 functional unit.  It is used for race condition checking.
*/
struct stmt_blk_s {
  statement* stmt;                   /*!< Pointer to top-level statement in statement tree that this signal is first found in */
  bool       remove;                 /*!< Specifies if this statement block should be removed after checking is complete */
  bool       seq;                    /*!< If true, this statement block is considered to include sequential logic */
  bool       cmb;                    /*!< If true, this statement block is considered to include combinational logic */
  bool       bassign;                /*!< If true, at least one expression in statement block is a blocking assignment */
  bool       nassign;                /*!< If true, at least one expression in statement block is a non-blocking assignment */
};

/*!
 Simulator feature that keeps track of head pointer for a given thread along with pointers to the
 parent and children thread of this thread.  Threads allow us to handle task calls and fork/join
 statements.
*/
struct thread_s {
  func_unit* funit;                  /*!< Pointer to functional unit that this thread is running for */
  thread*    parent;                 /*!< Pointer to parent thread that spawned this thread */
  statement* curr;                   /*!< Pointer to current statement running in this thread */
  reentrant* ren;                    /*!< Pointer to re-entrant structure to use for this thread */
  union {
    uint8 all;
    struct {
      uint8 state      : 2;          /*!< Set to current state of this thread (see \ref thread_states for legal values) */
      uint8 kill       : 1;          /*!< Set to TRUE if this thread should be killed */
      uint8 exec_first : 1;          /*!< Set to TRUE when the first statement is being executed */
    } part;
  } suppl;
  unsigned   active_children;        /*!< Set to the number of children threads in active thread queue */
  thread*    queue_prev;             /*!< Pointer to previous thread in active/delayed queue */
  thread*    queue_next;             /*!< Pointer to next thread in active/delayed queue */
  thread*    all_prev;               /*!< Pointer to previous thread in all pool */
  thread*    all_next;               /*!< Pointer to next thread in all pool */
  sim_time   curr_time;              /*!< Set to the current simulation time for this thread */
};

/*!
 Linked list structure for a thread list.
*/
struct thr_link_s {
  thread*   thr;                     /*!< Pointer to thread */
  thr_link* next;                    /*!< Pointer to next thread link in list */
};

/*!
 Linked list structure for a thread list.
*/
struct thr_list_s {
  thr_link* head;                    /*!< Pointer to the head link of a thread list */
  thr_link* tail;                    /*!< Pointer to the tail link of a thread list */
  thr_link* next;                    /*!< Pointer to next thread link in list to use for newly added threads */
};

/*!
 Performance statistic container used for simulation-time performance characteristics.
*/
struct perf_stat_s {
  uint32  op_exec_cnt[EXP_OP_NUM];   /*!< Specifies the number of times that the associated operation was executed */
  float   op_cnt[EXP_OP_NUM];        /*!< Specifies the number of expressions containing the associated operation */
};

/*!
 Container for port-specific information.
*/
struct port_info_s {
  int           type;                /*!< Type of port (see \ref ssuppl_type for legal values) */
  bool          is_signed;           /*!< Set to TRUE if this port is signed */
  sig_range*    prange;              /*!< Contains packed range information for this port */
  sig_range*    urange;              /*!< Contains unpacked range information for this port */
};

/*!
 Container for holding information pertinent for parameter overriding.
*/
struct param_oride_s {
  char*        name;                 /*!< Specifies name of parameter being overridden (only valid for by_name syntax) */
  expression*  expr;                 /*!< Expression to override parameter with */
  param_oride* next;                 /*!< Pointer to next parameter override structure in list */
};

/*!
 Container holding a generate block item.
*/
struct gen_item_s {
  union {
    expression* expr;                /*!< Pointer to an expression */
    vsignal*    sig;                 /*!< Pointer to signal */
    statement*  stmt;                /*!< Pointer to statement */
    funit_inst* inst;                /*!< Pointer to instance */
  } elem;                            /*!< Union of various pointers this generate item is pointing at */
  union {
    uint32      all;                 /*!< Specifies the entire supplemental field */
    struct {
      uint32    type       : 3;      /*!< Specifies which element pointer is valid */
      uint32    conn_id    : 16;     /*!< Connection ID (used for connecting) */
      uint32    stop_true  : 1;      /*!< Specifies that we should stop traversing the true path */
      uint32    stop_false : 1;      /*!< Specifies that we should stop traversing the false path */
      uint32    resolved   : 1;      /*!< Specifies if this generate item has been resolved */
      uint32    removed    : 1;      /*!< Specifies if this generate item has been "removed" from the design */
    } part;
  } suppl;
  char*         varname;             /*!< Specifies genvar name (for TFN) or signal/TFN name to bind to (BIND) */
  gen_item*     next_true;           /*!< Pointer to the next generate item if expr is true */
  gen_item*     next_false;          /*!< Pointer to the next generate item if expr is false */
};

/*!
 Generate item link element.  Stores pointer to a generate item.
*/
struct gitem_link_s {
  gen_item*   gi;                    /*!< Pointer to a generate item */
  gitem_link* next;                  /*!< Pointer to next generate item element in list */
};

/*!
 Structure to hold information for a typedef'ed type.  This information is stored within a module but is
 not written to the CDD file.
*/
struct typedef_item_s {
  char*         name;                /*!< Name of this typedef */
  bool          is_signed;           /*!< Specifies if this typedef type is signed */
  bool          is_handled;          /*!< Specifies if this typedef type is handled by Covered */
  bool          is_sizeable;         /*!< Specifies if a range can be later placed on this typedef */
  sig_range*    prange;              /*!< Dimensional packed range information for this type */
  sig_range*    urange;              /*!< Dimensional unpacked range information for this type */
  typedef_item* next;                /*!< Pointer to next item in typedef list */
};

/*!
 Binds a signal to a static expression for an enumerated value.
*/
struct enum_item_s {
  vsignal*      sig;                 /*!< Enumerated value in the form of a signal */
  static_expr*  value;               /*!< Value to assign to enumerated value */
  bool          last;                /*!< Set to TRUE if this is the last enumerated value in the list */
  enum_item*    next;                /*!< Pointer to next enumeration item in the list */
};

/*!
 Represents a multi-dimensional memory structure in the design.
*/
struct sig_range_s {
  int           dim_num;             /*!< Specifies the number of dimensions in dim array */
  vector_width* dim;                 /*!< Pointer to array of unpacked or packed dimensions */
  bool          clear;               /*!< Set to TRUE when this range has been fully consumed */
  bool          exp_dealloc;         /*!< Set to TRUE if expressions should be deallocated from the database because they are not in use */
};

/*!
 Represents a dimension in a multi-dimensional array/signal.
*/
struct dim_range_s {
  int        msb;                    /*!< MSB of range */
  int        lsb;                    /*!< LSB of range */
};

/*!
 Represents a reentrant stack and control information.
*/
struct reentrant_s {
  uint8*        data;                /*!< Packed bit data stored from signals */
  int           data_size;           /*!< Number of data elements stored in a single rstack_entry data */
};

/*!
 Represents a SystemVerilog structure/union.
*/
struct struct_union_s {
  char*         name;                /*!< Name of this struct or union */
  int           type;                /*!< Specifies whether this is a struct, union or tagged union */
  bool          packed;              /*!< Specifies if the data in this struct/union should be handled in a packed or unpacked manner */
  bool          is_signed;           /*!< Specifies if the data in the struct/union should be handled as a signed value or not */
  bool          owns_data;           /*!< Specifies if this struct/union owns its vector data */
  int           tag_pos;             /*!< Specifies the current tag position */
  vector*       data;                /*!< Pointer to all data needed for this structure */
  su_member*    mem_head;            /*!< Pointer to head of struct/union member list */
  su_member*    mem_tail;            /*!< Pointer to tail of struct/union member list */
  struct_union* next;                /*!< Pointer to next struct/union in list */
};

/*!
 Represents a single structure/union member variable.
*/
struct su_member_s {
  int             type;              /*!< Type of struct/union member */
  int             pos;               /*!< Position of member in the list */
  union {
    vsignal*      sig;               /*!< Points to a signal */
    struct_union* su;                /*!< Points to a struct/union */
    enum_item*    ei;                /*!< Points to an enumerated item */
    typedef_item* tdi;               /*!< Points to a typedef'ed item */
  } elem;                            /*!< Member element pointer */
  su_member*      parent;            /*!< Pointer to parent struct/union member */
  su_member*      next;              /*!< Pointer to next struct/union member */
};

/*!
 Represents a profiling entry for a given function.
*/
struct profiler_s {
  char*  func_name;                  /*!< Name of function that this profiler is associated with */
#ifdef HAVE_SYS_TIME_H
  /*@null@*/ timer* time_in;         /*!< Time spent running this function */
#endif
  int    calls;                      /*!< Number of times this function has been called */
  int    mallocs;                    /*!< Number of malloc calls made in this function */
  int    frees;                      /*!< Number of free calls made in this function */
  bool   timed;                      /*!< Specifies if the function should be timed or not */
};

/*!
 Represents a single design database.
*/
struct db_s {
  char*        top_module;            /*!< Name of top-most module in the design */
  char**       leading_hierarchies;   /*!< Specifies the Verilog hierarchy leading up to the DUT (taken from the -i value) */
  int          leading_hier_num;      /*!< Specifies the number of hierarchies stored in the leading_hierarchies array */
  bool         leading_hiers_differ;  /*!< Set to TRUE if more than one leading hierarchy exists and it differs with the first leading hierarchy */
  inst_link*   inst_head;             /*!< Pointer to head of instance tree list */
  inst_link*   inst_tail;             /*!< Pointer to tail of instance tree list */
  funit_inst** insts;                 /*!< Array of instances (for inlined coverage) */
  unsigned int inst_num;              /*!< Number of elements in the insts array */
  funit_link*  funit_head;            /*!< Pointer to head of functional unit list */
  funit_link*  funit_tail;            /*!< Pointer to tail of functional unit list */
  str_link*    fver_head;             /*!< Pointer to head of file version list */
  str_link*    fver_tail;             /*!< Pointer to head of file version list */
};

/*!
 Compressed version of coverage information for a given CDD.  This structure is used by the rank command
 for comparing coverages for ranking purposes.
*/
struct comp_cdd_cov_s {
  char*        cdd_name;                /*!< Name of CDD that this coverage information is for */
  uint64       timesteps;               /*!< Number of simulations timesteps stored in the CDD file */
  uint64       total_cps;               /*!< Number of total coverage points this CDD file represents */
  uint64       unique_cps;              /*!< Number of unique coverage points this CDD file represents */
  uint64       score;                   /*!< Storage for current score */
  bool         required;                /*!< Set to TRUE if this CDD is required to be in the ranked list by the user */
  ulong*       cps[CP_TYPE_NUM];        /*!< Compressed coverage points for each coverage metric */
  unsigned int cps_index[CP_TYPE_NUM];  /*!< Contains index of current bit to populate */
};

/*!
 Structure that holds the information for an exclusion reason.  This structure is stored in the functional
 unit that contains the signal, expression or FSM table.
*/
struct exclude_reason_s {
  char            type;                 /*!< Specifies the type of exclusion (L=line, T=toggle, M=memory, E=expression, F=FSM, A=assertion) */
  int             id;                   /*!< Specifies the numerical ID of the exclusion (type and id together make a unique identifier) */
  time_t          timestamp;            /*!< Time that the reason was added to the CDD */
  char*           reason;               /*!< String containing reason for exclusion */
  exclude_reason* next;                 /*!< Pointer to the next exclusion reason structure */
};

/*!
 Structure that contains a pair of statements (used for IF statement handling in parser).
*/
struct stmt_pair_s {
  statement* stmt1;                     /*!< Pointer to first statement */
  statement* stmt2;                     /*!< Pointer to second statement */
};

/*!
 Structure that contains a pair of generate items (used for generate IF statement handling in parser).
*/
struct gitem_pair_s {
  gen_item* gitem1;                     /*!< Pointer to first generate item */
  gen_item* gitem2;                     /*!< Pointer to second generate item */
};

/*!
 Dimension and non-blocking assignment structure that exists as an element within an expression that is on the LHS of a non-blocking
 assignment.
*/
struct dim_and_nba_s {
  exp_dim*         dim;                 /*!< Pointer to current LHS dimension */
  nonblock_assign* nba;                 /*!< Pointer to non-blocking assignment */
};

/*!
 Structure that holds information for performing non-blocking assignments.
*/
struct nonblock_assign_s {
  vsignal*        lhs_sig;              /*!< Pointer to left-hand-side signal to assign */
  int             lhs_lsb;              /*!< Left-hand-side LSB to assign */
  int             lhs_msb;              /*!< Left-hand-side MSB to assign */
  vector*         rhs_vec;              /*!< Pointer to right-hand-side vector containing value to assign */
  int             rhs_lsb;              /*!< Right-hand-side LSB to assign */
  int             rhs_msb;              /*!< Right-hand-side MSB to assign */
  struct {
    uint8         is_signed : 1;        /*!< Specifies if value is signed */
    uint8         added     : 1;        /*!< Set to 1 if this nonblocking assignment has been added to the simulation queue */
  } suppl;                              /*!< Supplemental field */
};

/*!
 Structure that is used to hold inlined code coverage information including the original code and the coverage code.
*/
struct str_cov_s {
  char*           cov;                  /*!< Coverage string */
  char*           str;                  /*!< Code string */
};

/*!
 This will define the exception type that gets thrown (Covered does not care about this value)
*/
define_exception_type(int);

extern struct exception_context the_exception_context[1];

#endif

