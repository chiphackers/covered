#ifndef __DEFINES_H__
#define __DEFINES_H__

/*!
 \file    defines.h
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
#define COVERED_HEADER     "\nCovered %s -- Verilog Code Coverage Utility\nWritten by Trevor Williams  (trevorw@charter.net)\nFreely distributable under the GPL license\n\n", COVERED_VERSION

/*!
 Default database filename if not specified on command-line.
*/
#define DFLT_OUTPUT_DB     "cov.db"

/*!
 Determine size of integer in bits.
*/
#define INTEGER_WIDTH	   (SIZEOF_INT * 8)

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

/*! @} */


/*!
 \addtogroup tree_fname_comps Compare types for tree matches.

 The following three defines specify to the tree_find_module function how it should
 deal with the presence/absence of the filename attribute of the module structure.

 @{
*/

/*!9
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
 - FALSE
 - TRUE
*/
#define VECTOR_MERGE_MASK    0xff0fff00

/*! @} */

/*!
 \addtogroup expr_suppl Expression supplemental field defines and macros.

 @{
*/

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression is currently in the run-time queue.
*/
#define SUPPL_LSB_IN_QUEUE          0

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has been executed in the queue during the lifetime of the simulation.
*/
#define SUPPL_LSB_EXECUTED          1

/*!
 Least-significant bit position of expression supplemental field indicating this expression's
 value has changed in this timestamp.
*/
#define SUPPL_LSB_CHANGED           2

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression evaluates to a value of 0 or 1 and is therefore measurable for coverage.
*/
#define SUPPL_LSB_MEASURABLE        3

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has evaluated to a value of FALSE during the lifetime of the simulation.
*/
#define SUPPL_LSB_FALSE             4

/*!
 Least-significant bit position of expression supplemental field indicating that this
 expression has evaluated to a value of TRUE during the lifetime of the simulation.
*/
#define SUPPL_LSB_TRUE              5

/*!
 Least-significant bit position of expression supplemental field indicating the
 expression's operation type.
*/
#define SUPPL_LSB_OP                8

/*!
 Least-significant bit position of expression supplemental field for the LSB of the
 signal that this expression is encompassing.  Only valid if the expression operation
 type is SBIT_SEL or MBIT_SEL.
*/
#define SUPPL_LSB_SIG_LSB           16

/*!
 Used for merging two supplemental fields from two expressions.  Both expression
 supplemental fields are ANDed with this mask and ORed together to perform the
 merge.  Fields that are merged are:
 - EXECUTED
 - TRUE
 - FALSE
*/
#define SUPPL_MERGE_MASK            0x302

/*!
 Returns a value of 1 if the specified supplemental value has the executed
 bit set; otherwise, returns a value of 0 to indicate whether the
 corresponding expression was executed during simulation or not.
*/
#define SUPPL_WAS_EXECUTED(x)       ((x >> SUPPL_LSB_EXECUTED) & 0x1)

/*!
 Returns a value of 1 if the specified supplemental value is considered
 to be measurable.
*/
#define SUPPL_IS_MEASURABLE(x)      ((x >> SUPPL_LSB_MEASURABLE) & 0x1)

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
 Returns a value of 1 if the specified supplemental belongs to an expression
 that was measurable for combinational coverage but not fully covered during
 simulation.
*/
#define SUPPL_COMB_MISSED(x)        (SUPPL_IS_MEASURABLE(x) & (~SUPPL_WAS_TRUE(x) | ~SUPPL_WAS_FALSE(x)))

/*!
 Returns the specified expression's operation.
*/
#define SUPPL_OP(x)                 ((x >> SUPPL_LSB_OP) & 0xff)

/*! @} */
     
/*!
 \addtogroup read_modes Modes for reading database file

 Specify how to integrate read data from database file into memory structures.

 @{
*/

/*!
 When new module is read from database file, it is placed in the module list and
 is placed in the correct hierarchical position in the instance tree.
*/
#define READ_MODE_NO_MERGE          0

/*!
 When module is completely read in (including module, signals, expressions), the
 module is looked up in the current instance tree.  If the instance exists, the
 module is merged with the instance's module; otherwise, we are attempting to
 merge two databases that were created from differe9nt designs.
*/
#define READ_MODE_INST_MERGE        1

/*!
 When module is completely read in (including module, signals, expressions), the
 module is looked up in the module list.  If the module is found, it is merged
 with the existing module; otherwise, it is added to the module list.
*/
#define READ_MODE_MOD_MERGE         2

/*! @} */

/*!
 \addtogroup expr_ops Expression operations

 The following defines are used for identifying expression operations.

 @{
*/

#define EXP_OP_NONE	0x0	/*!<  0 Specifies no expression (leaf node in expression tree) */
#define EXP_OP_SIG	0x1	/*!<  1 Specifes that this expression contains signal value    */
#define EXP_OP_XOR	0x2	/*!<  2 '^'   operator */
#define EXP_OP_MULTIPLY	0x3    	/*!<  3 '*'   operator */
#define EXP_OP_DIVIDE	0x4	/*!<  4 '/'   operator */
#define EXP_OP_MOD	0x5	/*!<  5 '%'   operator */
#define EXP_OP_ADD	0x6	/*!<  6 '+'   operator */
#define EXP_OP_SUBTRACT	0x7	/*!<  7 '-'   operator */
#define EXP_OP_AND	0x8	/*!<  8 '&'   operator */
#define EXP_OP_OR	0x9	/*!<  9 '|'   operator */
#define EXP_OP_NAND     0xa     /*!< 10 '~&'  operator */
#define EXP_OP_NOR	0xb	/*!< 11 '~|'  operator */
#define EXP_OP_NXOR	0xc	/*!< 12 '~^'  operator */
#define EXP_OP_LT	0xd	/*!< 13 '<'   operator */
#define EXP_OP_GT	0xe	/*!< 14 '>'   operator */
#define EXP_OP_LSHIFT	0xf	/*!< 15 '<<'  operator */
#define EXP_OP_RSHIFT	0x10	/*!< 16 '>>'  operator */
#define EXP_OP_EQ	0x11	/*!< 17 '=='  operator */
#define EXP_OP_CEQ	0x12	/*!< 18 '===' operator */
#define EXP_OP_LE	0x13	/*!< 19 '<='  operator */
#define EXP_OP_GE	0x14	/*!< 20 '>='  operator */
#define EXP_OP_NE	0x15	/*!< 21 '!='  operator */
#define EXP_OP_CNE	0x16	/*!< 22 '!==' operator */
#define EXP_OP_LOR	0x17	/*!< 23 '||'  operator */
#define EXP_OP_LAND	0x18	/*!< 24 '&&'  operator */
#define EXP_OP_COND_T	0x19	/*!< 25 '?:' true condition operator  */
#define EXP_OP_COND_F   0x1a    /*!< 26 '?:' false condition operator */
#define EXP_OP_UINV	0x1b	/*!< 27 unary '~'  operator           */
#define EXP_OP_UAND	0x1c	/*!< 28 unary '&'  operator           */
#define EXP_OP_UNOT	0x1d	/*!< 29 unary '!'  operator           */
#define EXP_OP_UOR	0x1e	/*!< 30 unary '|'  operator           */
#define EXP_OP_UXOR	0x1f 	/*!< 31 unary '^'  operator           */
#define EXP_OP_UNAND	0x20	/*!< 32 unary '~&' operator           */
#define EXP_OP_UNOR	0x21	/*!< 33 unary '~|' operator           */
#define EXP_OP_UNXOR	0x22	/*!< 34 unary '~^' operator           */
#define EXP_OP_SBIT_SEL	0x23	/*!< 35 single-bit signal select      */
#define EXP_OP_MBIT_SEL	0x24	/*!< 36 multi-bit signal select       */
#define EXP_OP_EXPAND	0x25	/*!< 37 bit expansion operator        */
#define EXP_OP_CONCAT	0x26	/*!< 38 signal concatenation operator */
#define EXP_OP_PEDGE	0x27	/*!< 39 posedge operator              */
#define EXP_OP_NEDGE	0x28	/*!< 40 negedge operator              */
#define EXP_OP_AEDGE	0x29	/*!< 41 anyedge operator              */
#define EXP_OP_STMT     0x2a    /*!< 42 statement operator            */

/*! @} */

/*!
 \addtogroup op_tables

 The following describe the operation table values for AND, OR, XOR, NAND, NOR and
 NXOR operations.

 @{
*/
 
//                        00  01  02  03  10  11  12  13  20  21  22  23  30  31  32  33
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
#define COMP_CEQ        6       /*!< Absolute equal to        */
#define COMP_CNE        7       /*!< Absolute not equal to    */

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
#else
#if SIZEOF_LONG == 4
typedef unsigned long nibble;
#endif
#endif

//------------------------------------------------------------------------------
/*!
 Specifies an element in a linked list containing string values.  This data
 structure allows us to add new elements to the list without resizing, this
 optimizes performance with small amount of overhead.
*/
struct str_link_s;

typedef struct str_link_s str_link;

struct str_link_s {
  char*     str;     /*!< String to store                  */
  char*     suppl;   /*!< Pointer to additional string     */
  str_link* next;    /*!< Pointer to next str_link element */
};

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/*!
 An expression is defined to be a logical combination of signals/values.  Expressions may 
 contain subexpressions (which are expressions in and of themselves).  An measurable expression 
 may only evaluate to TRUE (1) or FALSE (0).  If the parent expression of this expression is 
 NULL, then this expression is considered a root expression.  The nibble cntrl contains the
 run-time information for its expression.  Its bits are specified as the following:
 Bit  0        = Set to 1 if this expression is currently in the run-time queue.
 Bit  1        = Set to 1 if this expression has been executed in the run-time queue.
                 Indicates to report tool that this expression has been covered under line
                 coverage.
 Bit  2        = Indicates that the value of this expression has changed during this timestamp
 Bit  3        = Indicates that this expression evaluates to 0 or 1 and can be considered
                 measurable.
 Bit  8        = Indicates that the value of this expression has been evaluated to FALSE (0)
 Bit  9        = Indicates that the value of this expression has been evaluated to TRUE (1)
 Bits 16 - 32  = Used if this expression has an operation type of EXPR_OP_SBIT_SEL or
                 EXPR_OP_MBIT_SEL.  Stores the LSB value of the signal that this expression
                 is harboring.
*/
struct expression_s;
struct signal_s;

typedef struct expression_s expression;
typedef struct signal_s     signal;

struct expression_s {
  vector*     value;       /*!< Current value and toggle information of this expression        */
  nibble      suppl;       /*!< Vector containing supplemental information for this expression */
  int         line;        /*!< Starting line number of root expression (only valid if root)   */
  int         id;          /*!< Specifies unique ID for this expression in the parent          */
  signal*     sig;         /*!< Pointer to signal.  If NULL then no signal is attached         */
  expression* parent;      /*!< Parent expression.  If NULL then this is the root expression   */
  expression* right;       /*!< Pointer to expression on right                                 */
  expression* left;        /*!< Pointer to expression on left                                  */
};

//------------------------------------------------------------------------------
/*!
 Expression link element.  Stores pointer to an expression.
*/
struct exp_link_s;

typedef struct exp_link_s exp_link;

struct exp_link_s {
  expression* exp;   /*!< Pointer to expression                      */
  exp_link*   next;  /*!< Pointer to next expression element in list */
};

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/*!
 Linked list element that stores a signal.
*/
struct sig_link_s;

typedef struct sig_link_s sig_link;

struct sig_link_s {
  signal*   sig;   /*!< Pointer to signal in list                   */
  sig_link* next;  /*!< Pointer to next signal link element in list */
};

//------------------------------------------------------------------------------
/*!
 Contains information for a Verilog module.  A module contains a list of signals within the
 module.
*/
struct module_s {
  char*     name;      /*!< Module name                                        */
  char*     filename;  /*!< File name where module exists                      */
  char*     scope;     /*!< Verilog hierarchical scope of this module          */
  sig_link* sig_head;  /*!< Head pointer to list of signals in this module     */
  sig_link* sig_tail;  /*!< Tail pointer to list of signals in this module     */
  exp_link* exp_head;  /*!< Head pointer to list of expressions in this module */
  exp_link* exp_tail;  /*!< Tail pointer to list of expressions in this module */
};

typedef struct module_s module;

//------------------------------------------------------------------------------
/*!
 Linked list element that stores a module (no scope).
*/
struct mod_link_s;
	
typedef struct mod_link_s mod_link;
	
struct mod_link_s {
  module*    mod;   /*!< Pointer to module in list */
  mod_link* next;   /*!< Next module in list       */
};

//------------------------------------------------------------------------------
/*!
 Stores symbol name of signal along with pointer to signal itself into a lookup table
*/
struct symtable_s;

typedef struct symtable_s symtable;

struct symtable_s {
  char*     sym;          /*!< Name of VCD-specified signal                */
  signal*   sig;          /*!< Pointer to signal for this symbol           */
  symtable* right;        /*!< Pointer to next symtable entry to the right */
  symtable* left;         /*!< Pointer to next symtable entry to the left  */
};

//------------------------------------------------------------------------------
/*!
 Specifies bit range of this signal
*/
struct signal_width_s {
  int width;
  int lsb;
};

typedef struct signal_width_s signal_width;

//------------------------------------------------------------------------------
/*!
 Binds a signal to an expression.
*/
struct sig_exp_bind_s;

typedef struct sig_exp_bind_s sig_exp_bind;

struct sig_exp_bind_s {
  char*         sig_name;  /*!< Name of Verilog scoped signal                */
  expression*   exp;       /*!< Expression to bind.                          */
  char*         mod_name;  /*!< Name of Verilog module containing expression */
  sig_exp_bind* next;      /*!< Pointer to next binding in list              */
};

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/*!
 A module instance element in the module instance tree.
*/
struct mod_inst_s;

typedef struct mod_inst_s mod_inst;

struct mod_inst_s {
  char*      name;          /*!< Instance name of this module instance      */
  module*    mod;           /*!< Pointer to module this instance represents */
  statistic* stat;          /*!< Pointer to statistic holder                */
  mod_inst*  child_head;    /*!< Pointer to head of child list              */
  mod_inst*  child_tail;    /*!< Pointer to tail of child list              */
  mod_inst*  next;          /*!< Pointer to next child in parents list      */
};


#endif

