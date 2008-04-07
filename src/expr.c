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
 \file     expr.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/1/2001
 
 \par Expressions
 The following are special expressions that are handled differently than standard
 unary (i.e., ~a) and dual operators (i.e., a & b).  These expressions are documented
 to help remove confusion (my own) about how they are implemented by Covered and
 handled during the parsing and scoring phases of the tool.
 
 \par EXP_OP_SIG
 A signal expression has no left or right child (they are both NULL).  Its vector
 value is a pointer to the signal vector value to which is belongs.  This allows
 the signal expression value to change automatically when the signal value is
 updated.  No further expression operation is necessary to calculate its value.
 
 \par EXP_OP_SBIT_SEL
 A single-bit signal expression has its left child pointed to the expression tree
 that is required to select the bit from the specified signal value.  The left
 child is allowed to change values during simulation.  To verify that the current
 bit select has not exceeded the ranges of the signal, the signal pointer value
 in the expression structure is used to reference the signal.  The LSB and width
 values from the actual signal can then be used to verify that we are still
 within range.  If we are found to be out of range, a value of X must be assigned
 to the SBIT_SEL expression.  The width of an SBIT_SEL is always constant (1).  The
 LSB of the SBIT_SEL is manipulated by the left expression value.
 
 \par EXP_OP_MBIT_SEL
 A multi-bit signal expression has its left child set to the expression tree on the
 left side of the ':' in the vector and the right child set to the expression tree on
 the right side of the ':' in the vector.  The width of the MBIT_SEL must be constant
 but is related to the difference between the left and right child values; therefore,
 it is required that the left and right child values be constant expressions (consisting
 of only expressions, parameters, and static values).  The width of the MBIT_SEL
 expression is calculated after reading in the MBIT_SEL expression from the CDD file.
 If the left or right child expressions are found to not be constant, an error is
 signaled to the user immediately.  The LSB is also calculated to be the lesser of the
 two child values.  The width and lsb are assigned to the MBIT_SEL expression vector
 immediately.  In the case of MBIT_SEL, the LSB is also constant.  Vector direction
 is currently not considered at this point.

 \par EXP_OP_MBIT_POS
 A variable multi-bit positive select expression operates in much the same way as
 the EXP_OP_MBIT_SEL expression; however, the right expression must be a constant
 expression.  The left expression represents the LSB while the MSB is calculated by
 adding the value of the right expression to the value of the left expression and
 subtracting one.

 \par EXP_OP_MBIT_NEG
 A variable multi-bit negative select expression operates in much the same way as
 the EXP_OP_MBIT_POS expression.  In this case, the left expression represents the
 MSB while the the LSB is calculated by subtracting the value of the right expression
 and adding one.

 \par EXP_OP_ASSIGN, EXP_OP_DASSIGN, EXP_OP_NASSIGN, EXP_OP_IF
 All of these expressions are assignment operators that are in assign statements,
 behavioral non-blocking assignments, and if expressions, respectively.
 These expressions do not have an operation to perform because their vector value pointers
 point to the vector value on the right-hand side of the assignment operator.

 \par EXP_OP_BASSIGN
 The blocking assignment operator differs from the assignment operators mentioned above in that
 Covered will perform the assignment for a blocking assignment expression.  This allows us to
 expand the amount of code that can be covered by allowing several "zero-time" assignments to
 occur while maintaining accurate coverage information.

 \par EXP_OP_RASSIGN
 The register assignment operator is executed in the same manner as the BASSIGN expression operator
 with the exception that the RASSIGN must be executed prior to any simulation.

 \par EXP_OP_PASSIGN
 The port assignment operator is like the blocking assignment operator, in that Covered will perform
 the assignment.  The signal pointer points to the port signal of the function/task, the vector pointer
 is set to point to this signal's vector.

 \par EXP_OP_FUNC_CALL
 A function call expression runs the head statement block of the prescribed function whenever an
 expression changes value in its parameter list.  After the function is simulated the function variable
 value is copied to the expression vector.
 
 \par EXP_OP_TASK_CALL
 A task call expression simply runs the head statement block of the prescribed task immediately
 (the statement block is immediately run using the sim_statement function call).

 \par EXP_OP_NB_CALL
 A named block call expression is not really a legitimate Verilog expression type but is used for
 the purposes of binding an expression to a functional unit (like EXP_OP_FUNC_CALL).  It is not
 measurable and has no report output structure.  It acts much like an EXP_OP_FUNC_CALL expression
 if it is nested in a a function block; otherwise, acts like an EXP_OP_TASK_CALL in simulation but
 does not pass any parameters.

 \par EXP_OP_REPEAT
 The EXP_OP_REPEAT expression takes on the width of its right expression and, starting at a value of 0,
 increments by one until it reaches the value of the right expression (at that time it returns false
 and returns its incrementing value back to 0.
 
 \par EXP_OP_SLIST, EXP_OP_ALWAYS_COMB, EXP_OP_ALWAYS_LATCH
 The EXP_OP_SLIST expression is a 1-bit expression whose value is meaningless.  It indicates the
 Verilog-2001 sensitivity list @* for a statement block and its right expression is attaches to
 an EOR attached list of AEDGE operations.  The SLIST expression works like an EOR but only checks
 the right child.  When outputting an expression tree whose root expression is an SLIST, the rest
 of the expression should be ignored for outputting purposes.

 \par EXP_OP_NOOP
 This expression does nothing.  It is not measurable but is simply a placeholder for a statement that
 Covered will not handle (i.e., standard system calls that only contain inputs) but will not dismiss
 the statement block that the statement exists in.

 \par EXP_OP_DIM
 This expression handles a dimensional selection lookup, allowing us to properly handle multi-dimensional
 array accesses.

 \par EXP_OP_SFINISH, EXP_OP_SSTOP
 These expression types cause the simulator to stop execution immediately.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "binding.h"
#include "defines.h"
#include "expr.h"
#include "fsm.h"
#include "func_unit.h"
#include "instance.h"
#include "link.h"
#include "reentrant.h"
#include "sim.h"
#include "stmt_blk.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern nibble xor_optab[OPTAB_SIZE];
extern nibble and_optab[OPTAB_SIZE];
extern nibble or_optab[OPTAB_SIZE];
extern nibble nand_optab[OPTAB_SIZE];
extern nibble nor_optab[OPTAB_SIZE];
extern nibble nxor_optab[OPTAB_SIZE];

extern char   user_msg[USER_MSG_LENGTH];

extern exp_link* static_expr_head;
extern exp_link* static_expr_tail;
extern inst_link* inst_head;

extern bool debug_mode;
extern int  generate_expr_mode;
extern int  curr_expr_id;
extern bool flag_use_command_line_debug;
extern bool cli_debug_mode;

static bool expression_op_func__xor( expression*, thread*, const sim_time* );
static bool expression_op_func__multiply( expression*, thread*, const sim_time* );
static bool expression_op_func__divide( expression*, thread*, const sim_time* );
static bool expression_op_func__mod( expression*, thread*, const sim_time* );
static bool expression_op_func__add( expression*, thread*, const sim_time* );
static bool expression_op_func__subtract( expression*, thread*, const sim_time* );
static bool expression_op_func__and( expression*, thread*, const sim_time* );
static bool expression_op_func__or( expression*, thread*, const sim_time* );
static bool expression_op_func__nand( expression*, thread*, const sim_time* );
static bool expression_op_func__nor( expression*, thread*, const sim_time* );
static bool expression_op_func__nxor( expression*, thread*, const sim_time* );
static bool expression_op_func__lt( expression*, thread*, const sim_time* );
static bool expression_op_func__gt( expression*, thread*, const sim_time* );
static bool expression_op_func__lshift( expression*, thread*, const sim_time* );
static bool expression_op_func__rshift( expression*, thread*, const sim_time* );
static bool expression_op_func__arshift( expression*, thread*, const sim_time* );
static bool expression_op_func__eq( expression*, thread*, const sim_time* );
static bool expression_op_func__ceq( expression*, thread*, const sim_time* );
static bool expression_op_func__le( expression*, thread*, const sim_time* );
static bool expression_op_func__ge( expression*, thread*, const sim_time* );
static bool expression_op_func__ne( expression*, thread*, const sim_time* );
static bool expression_op_func__cne( expression*, thread*, const sim_time* );
static bool expression_op_func__lor( expression*, thread*, const sim_time* );
static bool expression_op_func__land( expression*, thread*, const sim_time* );
static bool expression_op_func__cond( expression*, thread*, const sim_time* );
static bool expression_op_func__cond_sel( expression*, thread*, const sim_time* );
static bool expression_op_func__uinv( expression*, thread*, const sim_time* );
static bool expression_op_func__uand( expression*, thread*, const sim_time* );
static bool expression_op_func__unot( expression*, thread*, const sim_time* );
static bool expression_op_func__uor( expression*, thread*, const sim_time* );
static bool expression_op_func__uxor( expression*, thread*, const sim_time* );
static bool expression_op_func__unand( expression*, thread*, const sim_time* );
static bool expression_op_func__unor( expression*, thread*, const sim_time* );
static bool expression_op_func__unxor( expression*, thread*, const sim_time* );
static bool expression_op_func__sbit( expression*, thread*, const sim_time* );
static bool expression_op_func__mbit( expression*, thread*, const sim_time* );
static bool expression_op_func__expand( expression*, thread*, const sim_time* );
static bool expression_op_func__concat( expression*, thread*, const sim_time* );
static bool expression_op_func__pedge( expression*, thread*, const sim_time* );
static bool expression_op_func__nedge( expression*, thread*, const sim_time* );
static bool expression_op_func__aedge( expression*, thread*, const sim_time* );
static bool expression_op_func__eor( expression*, thread*, const sim_time* );
static bool expression_op_func__slist( expression*, thread*, const sim_time* );
static bool expression_op_func__delay( expression*, thread*, const sim_time* );
static bool expression_op_func__case( expression*, thread*, const sim_time* );
static bool expression_op_func__casex( expression*, thread*, const sim_time* );
static bool expression_op_func__casez( expression*, thread*, const sim_time* );
static bool expression_op_func__default( expression*, thread*, const sim_time* );
static bool expression_op_func__list( expression*, thread*, const sim_time* );
static bool expression_op_func__bassign( expression*, thread*, const sim_time* );
static bool expression_op_func__func_call( expression*, thread*, const sim_time* );
static bool expression_op_func__task_call( expression*, thread*, const sim_time* );
static bool expression_op_func__trigger( expression*, thread*, const sim_time* );
static bool expression_op_func__nb_call( expression*, thread*, const sim_time* );
static bool expression_op_func__fork( expression*, thread*, const sim_time* );
static bool expression_op_func__join( expression*, thread*, const sim_time* );
static bool expression_op_func__disable( expression*, thread*, const sim_time* );
static bool expression_op_func__repeat( expression*, thread*, const sim_time* );
static bool expression_op_func__null( expression*, thread*, const sim_time* );
static bool expression_op_func__sig( expression*, thread*, const sim_time* );
static bool expression_op_func__exponent( expression*, thread*, const sim_time* );
static bool expression_op_func__passign( expression*, thread*, const sim_time* );
static bool expression_op_func__mbit_pos( expression*, thread*, const sim_time* );
static bool expression_op_func__mbit_neg( expression*, thread*, const sim_time* );
static bool expression_op_func__negate( expression*, thread*, const sim_time* );
static bool expression_op_func__iinc( expression*, thread*, const sim_time* );
static bool expression_op_func__pinc( expression*, thread*, const sim_time* );
static bool expression_op_func__idec( expression*, thread*, const sim_time* );
static bool expression_op_func__pdec( expression*, thread*, const sim_time* );
static bool expression_op_func__dly_assign( expression*, thread*, const sim_time* );
static bool expression_op_func__dly_op( expression*, thread*, const sim_time* );
static bool expression_op_func__repeat_dly( expression*, thread*, const sim_time* );
static bool expression_op_func__wait( expression*, thread*, const sim_time* );
static bool expression_op_func__finish( expression*, thread*, const sim_time* );
static bool expression_op_func__stop( expression*, thread*, const sim_time* );

static void expression_assign( expression*, expression*, int*, thread*, const sim_time* );

/*!
 Array containing static information about expression operation types.  NOTE:  This structure MUST be
 updated if a new expression is added!  The third argument is an initialization to the exp_info_s structure.
*/
const exp_info exp_op_info[EXP_OP_NUM] = { {"STATIC",         "",             expression_op_func__null,       {0, 1, NOT_COMB,   1, 0, 0, 0} },
                                           {"SIG",            "",             expression_op_func__sig,        {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"XOR",            "^",            expression_op_func__xor,        {0, 0, OTHER_COMB, 0, 1, 0, 1} },
                                           {"MULTIPLY",       "*",            expression_op_func__multiply,   {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"DIVIDE",         "/",            expression_op_func__divide,     {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"MOD",            "%",            expression_op_func__mod,        {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"ADD",            "+",            expression_op_func__add,        {0, 0, OTHER_COMB, 0, 1, 0, 1} },
                                           {"SUBTRACT",       "-",            expression_op_func__subtract,   {0, 0, OTHER_COMB, 0, 1, 0, 1} },
                                           {"AND",            "&",            expression_op_func__and,        {0, 0, AND_COMB,   0, 1, 0, 1} },
                                           {"OR",             "|",            expression_op_func__or,         {0, 0, OR_COMB,    0, 1, 0, 1} },
                                           {"NAND",           "~&",           expression_op_func__nand,       {0, 0, AND_COMB,   0, 1, 0, 0} },
                                           {"NOR",            "~|",           expression_op_func__nor,        {0, 0, OR_COMB,    0, 1, 0, 0} },
                                           {"NXOR",           "~^",           expression_op_func__nxor,       {0, 0, OTHER_COMB, 0, 1, 0, 0} },
                                           {"LT",             "<",            expression_op_func__lt,         {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"GT",             ">",            expression_op_func__gt,         {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"LSHIFT",         "<<",           expression_op_func__lshift,     {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"RSHIFT",         ">>",           expression_op_func__rshift,     {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"EQ",             "==",           expression_op_func__eq,         {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"CEQ",            "===",          expression_op_func__ceq,        {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"LE",             "<=",           expression_op_func__le,         {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"GE",             ">=",           expression_op_func__ge,         {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"NE",             "!=",           expression_op_func__ne,         {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"CNE",            "!==",          expression_op_func__cne,        {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"LOR",            "||",           expression_op_func__lor,        {0, 0, OR_COMB,    0, 1, 0, 0} },
                                           {"LAND",           "&&",           expression_op_func__land,       {0, 0, AND_COMB,   0, 1, 0, 0} },
                                           {"COND",           "?:",           expression_op_func__cond,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"COND_SEL",       "",             expression_op_func__cond_sel,   {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"UINV",           "~",            expression_op_func__uinv,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UAND",           "&",            expression_op_func__uand,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UNOT",           "!",            expression_op_func__unot,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UOR",            "|",            expression_op_func__uor,        {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UXOR",           "^",            expression_op_func__uxor,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UNAND",          "~&",           expression_op_func__unand,      {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UNOR",           "~|",           expression_op_func__unor,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"UNXOR",          "~^",           expression_op_func__unxor,      {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"SBIT_SEL",       "[]",           expression_op_func__sbit,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"MBIT_SEL",       "[:]",          expression_op_func__mbit,       {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"EXPAND",         "{{}}",         expression_op_func__expand,     {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"CONCAT",         "{}",           expression_op_func__concat,     {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"PEDGE",          "posedge",      expression_op_func__pedge,      {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"NEDGE",          "negedge",      expression_op_func__nedge,      {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"AEDGE",          "anyedge",      expression_op_func__aedge,      {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"LAST",           "",             expression_op_func__null,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"EOR",            "or",           expression_op_func__eor,        {1, 0, NOT_COMB,   1, 0, 1, 0} },
                                           {"DELAY",          "#",            expression_op_func__delay,      {1, 0, NOT_COMB,   0, 0, 1, 0} },
                                           {"CASE",           "case",         expression_op_func__case,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"CASEX",          "casex",        expression_op_func__casex,      {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"CASEZ",          "casez",        expression_op_func__casez,      {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"DEFAULT",        "",             expression_op_func__default,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"LIST",           "",             expression_op_func__list,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"PARAM",          "",             expression_op_func__sig,        {0, 1, NOT_COMB,   1, 0, 0, 0} },
                                           {"PARAM_SBIT",     "[]",           expression_op_func__sbit,       {0, 1, NOT_COMB,   1, 0, 0, 0} },
                                           {"PARAM_MBIT",     "[:]",          expression_op_func__mbit,       {0, 1, NOT_COMB,   1, 0, 0, 0} },
                                           {"ASSIGN",         "",             expression_op_func__null,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"DASSIGN",        "",             expression_op_func__null,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"BASSIGN",        "",             expression_op_func__bassign,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"NASSIGN",        "",             expression_op_func__null,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"IF",             "",             expression_op_func__null,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"FUNC_CALL",      "",             expression_op_func__func_call,  {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"TASK_CALL",      "",             expression_op_func__task_call,  {0, 0, NOT_COMB,   1, 0, 1, 0} },
                                           {"TRIGGER",        "->",           expression_op_func__trigger,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"NB_CALL",        "",             expression_op_func__nb_call,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"FORK",           "",             expression_op_func__fork,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"JOIN",           "",             expression_op_func__join,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"DISABLE",        "",             expression_op_func__disable,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"REPEAT",         "",             expression_op_func__repeat,     {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"WHILE",          "",             expression_op_func__null,       {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"ALSHIFT",        "<<<",          expression_op_func__lshift,     {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"ARSHIFT",        ">>>",          expression_op_func__arshift,    {0, 0, NOT_COMB,   1, 1, 0, 1} },
                                           {"SLIST",          "@*",           expression_op_func__slist,      {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"EXPONENT",       "**",           expression_op_func__exponent,   {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"PASSIGN",        "",             expression_op_func__passign,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"RASSIGN",        "",             expression_op_func__bassign,    {0, 0, NOT_COMB,   1, 0, 0, 0} },
                                           {"MBIT_POS",       "[+:]",         expression_op_func__mbit_pos,   {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"MBIT_NEG",       "[-:]",         expression_op_func__mbit_neg,   {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"PARAM_MBIT_POS", "[+:]",         expression_op_func__mbit_pos,   {0, 1, NOT_COMB,   1, 0, 0, 0} },
                                           {"PARAM_MBIT_NEG", "[-:]",         expression_op_func__mbit_neg,   {0, 1, NOT_COMB,   1, 0, 0, 0} },
                                           {"NEGATE",         "-",            expression_op_func__negate,     {0, 0, NOT_COMB,   1, 1, 0, 0} },
                                           {"NOOP",           "",             expression_op_func__null,       {0, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"ALWAYS_COMB",    "always_comb",  expression_op_func__slist,      {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"ALWAYS_LATCH",   "always_latch", expression_op_func__slist,      {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"IINC",           "++",           expression_op_func__iinc,       {1, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"PINC",           "++",           expression_op_func__pinc,       {1, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"IDEC",           "--",           expression_op_func__idec,       {1, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"PDEC",           "--",           expression_op_func__pdec,       {1, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"DLY_ASSIGN",     "",             expression_op_func__dly_assign, {1, 0, NOT_COMB,   0, 0, 1, 0} },
                                           {"DLY_OP",         "",             expression_op_func__dly_op,     {1, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"RPT_DLY",        "",             expression_op_func__repeat_dly, {1, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"DIM",            "",             expression_op_func__null,       {0, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"WAIT",           "wait",         expression_op_func__wait,       {1, 0, NOT_COMB,   0, 1, 1, 0} },
                                           {"SFINISH",        "$finish",      expression_op_func__finish,     {0, 0, NOT_COMB,   0, 0, 0, 0} },
                                           {"SSTOP",          "$stop",        expression_op_func__stop,       {0, 0, NOT_COMB,   0, 0, 0, 0} } };

/*! One bit unknown value that can be used by vector selection operators */
static vec_data x_value = {0x2};


/*!
 \param exp    Pointer to expression to add value to.
 \param width  Width of value to create.
 \param data   Specifies if nibble array should be allocated for vector.

 \throws anonymous Throw

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.
*/
static void expression_create_value(
  expression* exp,
  int         width,
  bool        data
) { PROFILE(EXPRESSION_CREATE_VALUE);

  vec_data* value = NULL;  /* Temporary storage of vector nibble array */

  if( (data == TRUE) || ((exp->suppl.part.gen_expr == 1) && (width > 0)) ) {
    if( width > MAX_BIT_WIDTH ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Found an expression width (%d) that exceeds the maximum currently allowed by Covered (%d)",
                                  width, MAX_BIT_WIDTH );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "expr Throw A\n" );
      Throw 0;
    }
    value = (vec_data*)malloc_safe( sizeof( vec_data ) * width );
    assert( exp->value->value == NULL );
  }

  /* Create value */
  vector_init( exp->value, value, (value != NULL), width, VTYPE_EXP );

  PROFILE_END;

}

/*!
 \param right    Pointer to expression on right.
 \param left     Pointer to expression on left.
 \param op       Operation to perform for this expression.
 \param lhs      Specifies this expression is a left-hand-side assignment expression.
 \param id       ID for this expression as determined by the parent.
 \param line     Line number this expression is on.
 \param first    First column index of expression.
 \param last     Last column index of expression.
 \param data     Specifies if we should create a nibble array for the vector value.

 \return Returns pointer to newly created expression.

 \throws anonymous Throw expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create(
  expression*  right,
  expression*  left,
  exp_op_type  op,
  bool         lhs,
  int          id,
  int          line,
  unsigned int first,
  unsigned int last,
  bool         data
) { PROFILE(EXPRESSION_CREATE);

  expression* new_expr;    /* Pointer to newly created expression */
  int         rwidth = 0;  /* Bit width of expression on right */
  int         lwidth = 0;  /* Bit width of expression on left */

  new_expr = (expression*)malloc_safe( sizeof( expression ) );

  new_expr->suppl.all           = 0;
  new_expr->suppl.part.lhs      = (nibble)lhs & 0x1;
  new_expr->suppl.part.gen_expr = (generate_expr_mode > 0) ? 1 : 0;
  new_expr->suppl.part.root     = 1;
  new_expr->op                  = op;
  new_expr->id                  = id;
  new_expr->ulid                = -1;
  new_expr->line                = line;
  new_expr->col                 = ((first & 0xffff) << 16) | (last & 0xffff);
  new_expr->exec_num            = 0;
  new_expr->sig                 = NULL;
  new_expr->parent              = (expr_stmt*)malloc_safe( sizeof( expr_stmt ) );
  new_expr->parent->expr        = NULL;
  new_expr->right               = right;
  new_expr->left                = left;
  new_expr->value               = (vector*)malloc_safe( sizeof( vector ) );
  new_expr->suppl.part.owns_vec = 1;
  new_expr->value->value        = NULL;
  new_expr->table               = NULL;
  new_expr->elem.funit          = NULL;
  new_expr->name                = NULL;

  // printf( "Allocated expression: " );  expression_display( new_expr );

  if( right != NULL ) {

    /* Get information from right */
    assert( right->value != NULL );
    rwidth = right->value->width;

    /* Set right expression parent to this expression */
    assert( right->parent->expr == NULL );
    right->parent->expr = new_expr;

    /* Reset root bit of right expression */
    right->suppl.part.root = 0;

  }

  if( left != NULL ) {

    /* Get information from left */
    assert( left->value != NULL );
    lwidth = left->value->width;

    /* Set left expression parent to this expression (if this is not a case expression) */
    if( (op != EXP_OP_CASE) && (op != EXP_OP_CASEX) && (op != EXP_OP_CASEZ) ) {
      assert( left->parent->expr == NULL );
      left->parent->expr    = new_expr;
      left->suppl.part.root = 0;
    }

  }

  Try {

    /* Create value vector */
    if( ((op == EXP_OP_MULTIPLY) || (op == EXP_OP_LIST)) && (rwidth > 0) && (lwidth > 0) ) {

      /* For multiplication, we need a width the sum of the left and right expressions */
      expression_create_value( new_expr, (lwidth + rwidth), data );

    } else if( (op == EXP_OP_CONCAT) && (rwidth > 0) ) {

      expression_create_value( new_expr, rwidth, data );

    } else if( (op == EXP_OP_EXPAND) && (rwidth > 0) && (lwidth > 0) && (left->value->value != NULL) ) {

      /*
       If the left-hand expression is a known value, go ahead and create the value here; otherwise,
       hold off because our vector value will be coming.
      */
      if( !left->value->suppl.part.unknown ) {
        expression_create_value( new_expr, (vector_to_int( left->value ) * rwidth), data );
      } else {
        expression_create_value( new_expr, 1, data );
      }

    } else if( (op == EXP_OP_LT   )   ||
               (op == EXP_OP_GT   )   ||
               (op == EXP_OP_EQ   )   ||
               (op == EXP_OP_CEQ  )   ||
               (op == EXP_OP_LE   )   ||
               (op == EXP_OP_GE   )   ||
               (op == EXP_OP_NE   )   ||
               (op == EXP_OP_CNE  )   ||
               (op == EXP_OP_LOR  )   ||
               (op == EXP_OP_LAND )   ||
               (op == EXP_OP_UAND )   ||
               (op == EXP_OP_UNOT )   ||
               (op == EXP_OP_UOR  )   ||
               (op == EXP_OP_UXOR )   ||
               (op == EXP_OP_UNAND)   ||
               (op == EXP_OP_UNOR )   ||
               (op == EXP_OP_UNXOR)   ||
               (op == EXP_OP_EOR)     ||
               (op == EXP_OP_NEDGE)   ||
               (op == EXP_OP_PEDGE)   ||
               (op == EXP_OP_AEDGE)   ||
               (op == EXP_OP_CASE)    ||
               (op == EXP_OP_CASEX)   ||
               (op == EXP_OP_CASEZ)   ||
               (op == EXP_OP_DEFAULT) ||
               (op == EXP_OP_REPEAT)  ||
               (op == EXP_OP_RPT_DLY) ||
               (op == EXP_OP_WAIT)    ||
               (op == EXP_OP_SFINISH) ||
               (op == EXP_OP_SSTOP) ) {
  
      /* If this expression will evaluate to a single bit, create vector now */
      expression_create_value( new_expr, 1, data );

    } else {

      /* If both right and left values have their width values set. */
      if( (rwidth > 0) && (lwidth > 0) && 
          (op != EXP_OP_MBIT_SEL)       &&
          (op != EXP_OP_MBIT_POS)       &&
          (op != EXP_OP_MBIT_NEG)       &&
          (op != EXP_OP_PARAM_MBIT)     &&
          (op != EXP_OP_PARAM_MBIT_POS) &&
          (op != EXP_OP_PARAM_MBIT_NEG) ) {

        if( rwidth >= lwidth ) {
          /* Check to make sure that nothing has gone drastically wrong */
          expression_create_value( new_expr, rwidth, data );
        } else {
          /* Check to make sure that nothing has gone drastically wrong */
          expression_create_value( new_expr, lwidth, data );
        }

      } else {
 
        expression_create_value( new_expr, 0, FALSE );
 
      }

    }

  } Catch_anonymous {
    expression_dealloc( new_expr, TRUE );
    printf( "expr Throw B\n" );
    Throw 0;
  }

  if( (data == FALSE) && (generate_expr_mode == 0) ) {
    assert( new_expr->value->value == NULL );
  }

  PROFILE_END;

  return( new_expr );

}

/*!
 \param exp    Pointer to expression to set value to.
 \param sig    Pointer to signal containing vector value to set expression to.
 \param funit  Pointer to functional unit containing expression.

 \throws anonymous expression_operate_recursively expression_operate_recursively expression_operate_recursively
 
 Sets the specified expression (if necessary) to the value of the
 specified signal's vector value.
*/
void expression_set_value( expression* exp, vsignal* sig, func_unit* funit ) { PROFILE(EXPRESSION_SET_VALUE);
  
  int lbit;       /* Bit boundary specified by left child */
  int rbit;       /* Bit boundary specified by right child */
  int exp_width;  /* Dimensional width of the expression */
  int exp_dim;    /* Dimension of the expression */
  
  assert( exp != NULL );
  assert( exp->value != NULL );
  assert( sig != NULL );
  assert( sig->value != NULL );

  /* Regardless of the type, set the supplemental field */
  exp->value->suppl = sig->value->suppl;

  /* Calculate expression dimensional width */
  exp_width = vsignal_calc_width_for_expr( exp, sig );

  /* Calculate the expression dimension */
  exp_dim = expression_get_curr_dimension( exp );
  
  /* Set the expression width */
  switch( exp->op ) {
    case EXP_OP_SIG       :
    case EXP_OP_PARAM     :
    case EXP_OP_TRIGGER   :
      exp->value->width = sig->value->width;
      break;
    case EXP_OP_SBIT_SEL   :
    case EXP_OP_PARAM_SBIT :
      exp->value->width = exp_width;
      break;
    case EXP_OP_MBIT_SEL   :
    case EXP_OP_PARAM_MBIT :
      expression_operate_recursively( exp->left,  funit, TRUE );
      expression_operate_recursively( exp->right, funit, TRUE );
      lbit = vector_to_int( exp->left->value  );
      rbit = vector_to_int( exp->right->value );
      if( lbit <= rbit ) {
        exp->value->width = ((rbit - lbit) + 1) * exp_width;
      } else {
        exp->value->width = ((lbit - rbit) + 1) * exp_width;
      }
      break;
    case EXP_OP_MBIT_POS :
    case EXP_OP_MBIT_NEG :
    case EXP_OP_PARAM_MBIT_POS :
    case EXP_OP_PARAM_MBIT_NEG :
      expression_operate_recursively( exp->right, funit, TRUE );
      exp->value->width = vector_to_int( exp->right->value ) * exp_width;
      break;
    default :  break;
  }

  /* Point expression value to the signal value */
  exp->value->value                = sig->value->value;
  exp->value->suppl.part.owns_data = 0;

  PROFILE_END;

}

/*!
 \param exp  Pointer to current expression

 Recursively sets the signed bit for all parent expressions if both the left and
 right expressions have the signed bit set.  This function is called by the bind()
 function after an expression has been set to a signal.
*/
void expression_set_signed( expression* exp ) { PROFILE(EXPRESSION_SET_SIGNED);

  if( exp != NULL ) {

    /*
     If this expression is attached to a signal that has the signed bit set (which is not a bit select) or
     the valid left and right expressions have the signed bit set, set our is_signed bit
     and continue traversing up expression tree.
    */
    if( ((exp->sig != NULL) && (exp->sig->value->suppl.part.is_signed == 1) &&
         (exp->op != EXP_OP_SBIT_SEL)   &&
         (exp->op != EXP_OP_MBIT_SEL)   &&
         (exp->op != EXP_OP_PARAM_SBIT) &&
         (exp->op != EXP_OP_PARAM_MBIT)) ||
        ((((exp->left  != NULL) && (exp->left->value->suppl.part.is_signed  == 1)) || (exp->left  == NULL)) &&
         (((exp->right != NULL) && (exp->right->value->suppl.part.is_signed == 1)) || (exp->right == NULL)) &&
         ((exp->op == EXP_OP_ADD)      ||
          (exp->op == EXP_OP_SUBTRACT) ||
          (exp->op == EXP_OP_MULTIPLY) ||
          (exp->op == EXP_OP_DIVIDE)   ||
          (exp->op == EXP_OP_MOD)      ||
          (exp->op == EXP_OP_STATIC)   ||
          (exp->op == EXP_OP_LT)       ||
          (exp->op == EXP_OP_GT)       ||
          (exp->op == EXP_OP_LE)       ||
          (exp->op == EXP_OP_GE)       ||
          (exp->op == EXP_OP_EQ)       ||
          (exp->op == EXP_OP_NE))) ||
        (exp->value->suppl.part.is_signed == 1) ) {

      exp->value->suppl.part.is_signed = 1;

      /* If we are not the root expression, traverse up */
      if( ESUPPL_IS_ROOT( exp->suppl ) == 0 ) {
        expression_set_signed( exp->parent->expr );
      }

    }
    
  }

  PROFILE_END;

}

/*!
 \param expr       Pointer to expression to potentially resize.
 \param funit      Pointer to functional unit containing expression.
 \param recursive  Specifies if we should perform a recursive depth-first resize
 \param alloc      If set to TRUE, allocates vector data for all expressions

 \throws anonymous expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value
         expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value
         expression_create_value expression_create_value funit_size_elements expression_resize expression_resize expression_operate_recursively expression_set_value

 Resizes the given expression depending on the expression operation and its
 children's sizes.  If recursive is TRUE, performs the resize in a depth-first
 fashion, resizing the children before resizing the current expression.  If
 recursive is FALSE, only the given expression is evaluated and resized.
*/
void expression_resize(
  expression* expr,
  func_unit*  funit,
  bool        recursive,
  bool        alloc
) { PROFILE(EXPRESSION_RESIZE);

  int         largest_width;  /* Holds larger width of left and right children */
  nibble      old_vec_suppl;  /* Holds original vector supplemental field as this will be erased */
  funit_inst* tmp_inst;       /* Pointer to temporary instance */
  int         ignore = 0;     /* Specifies the number of instances to ignore */

  if( expr != NULL ) {

    nibble new_owns_data;

    if( recursive ) {
      expression_resize( expr->left, funit, recursive, alloc );
      expression_resize( expr->right, funit, recursive, alloc );
    }

    /* Get vector supplemental field */
    old_vec_suppl = expr->value->suppl.all;

    switch( expr->op ) {

      /* Only resize these values if we are recursively resizing */
      case EXP_OP_PARAM          :
      case EXP_OP_PARAM_SBIT     :
      case EXP_OP_PARAM_MBIT     :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
      case EXP_OP_SIG            :
      case EXP_OP_SBIT_SEL       :
      case EXP_OP_MBIT_SEL       :
      case EXP_OP_MBIT_POS       :
      case EXP_OP_MBIT_NEG       :
        if( recursive && (expr->sig != NULL) ) {
          expression_set_value( expr, expr->sig, funit );
          assert( expr->value->value != NULL );
        }
        break;

      /* These operations will already be sized so nothing to do here */
      case EXP_OP_STATIC         :
      case EXP_OP_TRIGGER        :
      case EXP_OP_ASSIGN         :
      case EXP_OP_DASSIGN        :
      case EXP_OP_BASSIGN        :
      case EXP_OP_NASSIGN        :
      case EXP_OP_PASSIGN        :
      case EXP_OP_RASSIGN        :
      case EXP_OP_DLY_ASSIGN     :
      case EXP_OP_IF             :
      case EXP_OP_WHILE          :
      case EXP_OP_LAST           :
      case EXP_OP_DIM            :
        break;

      /* These operations should always be set to a width 1 */
      case EXP_OP_LT      :
      case EXP_OP_GT      :
      case EXP_OP_EQ      :
      case EXP_OP_CEQ     :
      case EXP_OP_LE      :
      case EXP_OP_GE      :
      case EXP_OP_NE      :
      case EXP_OP_CNE     :
      case EXP_OP_LOR     :
      case EXP_OP_LAND    :
      case EXP_OP_UAND    :
      case EXP_OP_UNOT    :
      case EXP_OP_UOR     :
      case EXP_OP_UXOR    :
      case EXP_OP_UNAND   :
      case EXP_OP_UNOR    :
      case EXP_OP_UNXOR   :
      case EXP_OP_EOR     :
      case EXP_OP_CASE    :
      case EXP_OP_CASEX   :
      case EXP_OP_CASEZ   :
      case EXP_OP_DEFAULT :
      case EXP_OP_REPEAT  :
      case EXP_OP_RPT_DLY :
      case EXP_OP_WAIT    :
      case EXP_OP_SFINISH :
      case EXP_OP_SSTOP   :
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, alloc );
        }
        break;

      /* Need to set LAST (left) expression and our width to 1 */
      case EXP_OP_NEDGE   :
      case EXP_OP_PEDGE   :
        if( (expr->left->value->width != 1) || (expr->left->value->value == NULL) ) {
          assert( expr->left->value->value == NULL );
          expression_create_value( expr->left, 1, alloc );
        }
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, alloc );
        }
        break;

      /* Need to set LAST (left) expression to width of right and our width to 1 */
      case EXP_OP_AEDGE   :
        if( (expr->left->value->width != expr->right->value->width) || (expr->left->value->value == NULL) ) {
          assert( expr->left->value->value == NULL );
          expression_create_value( expr->left, expr->right->value->width, alloc );
        }
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, alloc );
        }
        break;

      /*
       In the case of an EXPAND, we need to set the width to be the product of the value of
       the left child and the bit-width of the right child.
      */
      case EXP_OP_EXPAND :
        expression_operate_recursively( expr->left, funit, TRUE );
        if( (expr->value->width != (vector_to_int( expr->left->value ) * expr->right->value->width)) ||
            (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, (vector_to_int( expr->left->value ) * expr->right->value->width), alloc );
        }
        break;

      /* 
       In the case of a MULTIPLY or LIST (for concatenation) operation, its expression width must be the sum of its
       children's width.  Remove the current vector and replace it with the appropriately
       sized vector.
      */
      case EXP_OP_LIST :
        if( (expr->value->width != (expr->left->value->width + expr->right->value->width)) ||
            (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, (expr->left->value->width + expr->right->value->width), alloc );
        }
        if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
          assert( expr->left->value->value == NULL );
          expression_create_value( expr->left, expr->parent->expr->left->value->width, alloc );
        }
        break;

      /*
       A FUNC_CALL expression width is set to the same width as that of the function's return value.
      */
      case EXP_OP_FUNC_CALL :
        if( expr->sig != NULL ) {
          assert( funit != NULL );
          if( (funit->type != FUNIT_AFUNCTION) && (funit->type != FUNIT_ANAMED_BLOCK) ) {
            assert( expr->elem.funit != NULL );
            tmp_inst = inst_link_find_by_funit( expr->elem.funit, inst_head, &ignore );
            funit_size_elements( expr->elem.funit, tmp_inst, FALSE, FALSE );
          }
          if( (expr->value->width != expr->sig->value->width) || (expr->value->value == NULL) ) {
            assert( expr->value->value == NULL );
            expression_create_value( expr, expr->sig->value->width, alloc );
          }
        }
        break;

      default :
        /*
         If this expression is either the root, an LHS expression or a lower-level RHS expression,
         get its size from the largest of its children.
        */
        if( (ESUPPL_IS_ROOT( expr->suppl ) == 1) ||
            (ESUPPL_IS_LHS( expr->suppl ) == 1) ||
            ((expr->parent->expr->op != EXP_OP_ASSIGN) &&
             (expr->parent->expr->op != EXP_OP_DASSIGN) &&
             (expr->parent->expr->op != EXP_OP_BASSIGN) &&
             (expr->parent->expr->op != EXP_OP_NASSIGN) &&
             (expr->parent->expr->op != EXP_OP_RASSIGN) &&
             (expr->parent->expr->op != EXP_OP_DLY_OP)) ) {
          if( (expr->left != NULL) && ((expr->right == NULL) || (expr->left->value->width > expr->right->value->width)) ) {
            largest_width = expr->left->value->width;
          } else if( expr->right != NULL ) {
            largest_width = expr->right->value->width;
          } else {
            largest_width = 1;
          }
          if( (expr->value->width != largest_width) || (expr->value->value == NULL) ) {
            assert( expr->value->value == NULL );
            expression_create_value( expr, largest_width, alloc );
          }

        /* If our parent is a DLY_OP, we need to get our value from the LHS of the DLY_ASSIGN expression */
        } else if( expr->parent->expr->op == EXP_OP_DLY_OP ) {
          if( (expr->parent->expr->parent->expr->left->value->width != expr->value->width) || (expr->value->value == NULL) ) {
            assert( expr->value->value == NULL );
            expression_create_value( expr, expr->parent->expr->parent->expr->left->value->width, alloc );
          }

        /* Otherwise, get our value from the size of the expression on the left-hand-side of the assignment */
        } else {
          if( (expr->parent->expr->left->value->width != expr->value->width) || (expr->value->value == NULL) ) {
            assert( expr->value->value == NULL );
            expression_create_value( expr, expr->parent->expr->left->value->width, alloc );
          }
          if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
            assert( expr->left->value->value == NULL );
            expression_create_value( expr->left, expr->value->width, alloc );
          }
        }
        break;

    }

    /* Reapply original supplemental field (preserving the owns_data bit) now that expression has been resized */
    new_owns_data                     = expr->value->suppl.part.owns_data;
    expr->value->suppl.all            = old_vec_suppl;
    expr->value->suppl.part.owns_data = new_owns_data;

  }

  PROFILE_END;

}

/*!
 \param expr        Pointer to expression to get ID from.
 \param parse_mode  Specifies if ulid (TRUE) or id (FALSE) should be used

 \return Returns expression ID for this expression.

 If specified expression is non-NULL, return expression ID of this
 expression; otherwise, return a value of 0 to indicate that this
 is a leaf node.
*/
int expression_get_id( expression* expr, bool parse_mode ) { PROFILE(EXPRESSION_GET_ID);

  if( expr == NULL ) {
    return( 0 );
  } else {
    return( parse_mode ? expr->ulid : expr->id );
  }

}

/*!
 \param expr  Pointer to root expression to extract first line from

 \return Returns the line number of the first line in this expression.
*/
expression* expression_get_first_line_expr( expression* expr ) { PROFILE(EXPRESSION_GET_FIRST_LINE_EXPR);

  expression* first = NULL;

  if( expr != NULL ) {

    first = expression_get_first_line_expr( expr->left );
    if( (first == NULL) || (first->line > expr->line) ) {
      first = expr;
    }

  }

  PROFILE_END;

  return( first );

}

/*!
 \param expr  Pointer to root expression to extract last line from

 \return Returns the line number of the last line in this expression. 
*/
expression* expression_get_last_line_expr( expression* expr ) { PROFILE(EXPRESSION_GET_LAST_LINE_EXPR);

  expression* last = NULL;

  if( expr != NULL ) {

    last = expression_get_last_line_expr( expr->right );
    if( (last == NULL) || (last->line < expr->line) ) {
      last = expr;
    }
      
  }

  PROFILE_END;

  return( last );

}

/*!
 \param expr  Pointer to expression to get dimension for

 \return Returns the dimension index for the given expression

 Recursively iterates up expression tree, counting the number of dimensions deep that
 the given expression is.
*/
int expression_get_curr_dimension( expression* expr ) { PROFILE(EXPRESSION_GET_CURR_DIMENSION);
  
  int dim;  /* Return value for this function */

  assert( expr != NULL );

  if( expr->op == EXP_OP_DIM ) {

    dim = expression_get_curr_dimension( expr->left ) + 1;

  } else {

    if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) &&
        (expr->parent->expr->op == EXP_OP_DIM) && 
        (expr->parent->expr->right == expr) ) {
      dim = expression_get_curr_dimension( expr->parent->expr );
    } else {
      dim = 0;
    }

  }

  PROFILE_END;

  return( dim );

}

/*!
 \param expr  Pointer to expression tree to parse.
 \param head  Pointer to head of signal name list to populate.
 \param tail  Pointer to tail of signal name list to populate.

 Recursively parses specified expression list in search of RHS signals.
 When a signal name is found, it is added to the signal name list specified
 by head and tail.
*/
void expression_find_rhs_sigs(
  expression* expr,
  str_link**  head,
  str_link**  tail
) { PROFILE(EXPRESSION_FIND_RHS_SIGS);

  char* sig_name;  /* Name of signal found */

  /* Only continue if our expression is valid and it is an RHS */
  if( (expr != NULL) && (ESUPPL_IS_LHS( expr->suppl ) == 0) ) {

    if( (expr->op == EXP_OP_SIG)      ||
        (expr->op == EXP_OP_TRIGGER)  ||
        (expr->op == EXP_OP_SBIT_SEL) ||
        (expr->op == EXP_OP_MBIT_SEL) ||
        (expr->op == EXP_OP_MBIT_POS) ||
        (expr->op == EXP_OP_MBIT_NEG) ) {
 
      /* Get the signal name from the binder */
      sig_name = bind_find_sig_name( expr );
       
      assert( sig_name != NULL );
    
      /* If the signal isn't already in the list, add it */
      if( str_link_find( sig_name, *head ) == NULL ) {
        (void)str_link_add( sig_name, head, tail );
      } else {
        free_safe( sig_name, (strlen( sig_name ) + 1) );
      }

    }

    /* If this expression operation is neither a SIG or TRIGGER, keep searching tree */
    if( (expr->op != EXP_OP_SIG) && (expr->op != EXP_OP_TRIGGER) ) {

      expression_find_rhs_sigs( expr->right, head, tail );
      expression_find_rhs_sigs( expr->left,  head, tail );

    }

  }

  PROFILE_END;

}

/*!
 \param expr  Pointer to expression tree to search
 \param head  Pointer to head of expression list containing expressions that use parameter values
 \param tail  Pointer to tail of expression list containing expressions that use parameter values

 Recursively traverses given expression tree, adding any expressions found that point to parameter
 values.
*/
static void expression_find_params(
    expression* expr,
    exp_link**  head,
    exp_link**  tail
) { PROFILE(EXPRESSION_FIND_PARAMS);

  if( expr != NULL ) {

    /* If we call a parameter, add ourselves to the expression list */
    if( (expr->op == EXP_OP_PARAM)          ||
        (expr->op == EXP_OP_PARAM_SBIT)     ||
        (expr->op == EXP_OP_PARAM_MBIT)     ||
        (expr->op == EXP_OP_PARAM_MBIT_POS) ||
        (expr->op == EXP_OP_PARAM_MBIT_NEG) ) {
      exp_link_add( expr, head, tail );
    }

    /* Search children */
    expression_find_params( expr->left,  head, tail );
    expression_find_params( expr->right, head, tail );

  }

  PROFILE_END;

}

/*!
 \param expr  Pointer to root expression to search under
 \param ulid  Underline ID to search for

 \return Returns a pointer to the found expression; otherwise, returns NULL if the expression
         could not be found.

 Recursively searches the given expression tree for the specified underline ID.  If the
 expression is found, a pointer to it is returned; otherwise, returns NULL.
*/
expression* expression_find_uline_id( expression* expr, int ulid ) { PROFILE(EXPRESSION_FIND_ULINE_ID);

  expression* found_exp = NULL;  /* Pointer to found expression */

  if( expr != NULL ) {

    if( expr->ulid == ulid ) {
      found_exp = expr;
    } else {
      if( (found_exp = expression_find_uline_id( expr->left, ulid )) == NULL ) {
        found_exp = expression_find_uline_id( expr->right, ulid );
      }
    }

  }

  PROFILE_END;

  return( found_exp );

}

/*!
 \param root  Pointer to root of expression tree to search
 \param expr  Pointer to expression to search for

 \return Returns TRUE if the given expression exists within the given expression tree; otherwise,
         returns FALSE
*/
bool expression_find_expr( expression* root, expression* expr ) { PROFILE(EXPRESSION_FIND_EXPR);

  return( (root != NULL) && ((root == expr) || expression_find_expr( root->left, expr ) || expression_find_expr( root->right, expr )) );

}

/*!
 \param expr  Pointer to root of expression tree to search
 \param stmt  Pointer to statement to search for

 \return Returns TRUE if the given expression tree contains an expression that calls the given statement.
*/
bool expression_contains_expr_calling_stmt( expression* expr, statement* stmt ) { PROFILE(EXPRESSION_CONTAINS_EXPR_CALLING_STMT);

  return( (expr != NULL) &&
          (((ESUPPL_TYPE( expr->suppl ) == ETYPE_FUNIT) && (expr->elem.funit->first_stmt == stmt)) ||
           expression_contains_expr_calling_stmt( expr->left, stmt ) ||
           expression_contains_expr_calling_stmt( expr->right, stmt )) );

}

/*!
 \param exp  Pointer to expression to get root statement for.

 \return Returns a pointer to the root statement of the specified expression if one exists;
         otherwise, returns NULL.

 Recursively traverses up expression tree that contains exp until the root expression is found (if
 one exists).  If the root expression is found, return the pointer to the statement pointing to this
 root expression.  If the root expression was not found, return NULL.
*/
statement* expression_get_root_statement(
  expression* exp
) { PROFILE(EXPRESSION_GET_ROOT_STATEMENT);

  if( exp == NULL ) {
    return( NULL );
  } else if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
    return( exp->parent->stmt );
  } else {
    return( expression_get_root_statement( exp->parent->expr ) );
  }

}

/*!
 \param root   Pointer to root of the expression tree to assign unique IDs for
 \param funit  Pointer to functional unit containing this expression tree

 \throws anonymous expression_resize expression_assign_expr_ids expression_assign_expr_ids

 Recursively iterates down the specified expression tree assigning unique IDs to each expression.
*/
void expression_assign_expr_ids(
  expression* root,
  func_unit*  funit
) { PROFILE(EXPRESSION_ASSIGN_EXPR_IDS);

  if( root != NULL ) {

    /* Traverse children first */
    expression_assign_expr_ids( root->left, funit );
    expression_assign_expr_ids( root->right, funit );

    /* Assign our expression a unique ID value */
    root->ulid = curr_expr_id;
    curr_expr_id++;

    /* Resize ourselves */
    expression_resize( root, funit, FALSE, FALSE );

  }

  PROFILE_END;

}

/*!
 \param expr        Pointer to expression to write to database file.
 \param file        Pointer to database file to write to.
 \param parse_mode  Set to TRUE when we are writing after just parsing the design (causes ulid value to be
                    output instead of id)

 This function recursively displays the expression information for the specified
 expression tree to the coverage database specified by file.
*/
void expression_db_write(
  expression* expr,
  FILE*       file,
  bool        parse_mode
) { PROFILE(EXPRESSION_DB_WRITE);

  assert( expr != NULL );

  fprintf( file, "%d %d %d %x %x %x %x %d %d",
    DB_TYPE_EXPRESSION,
    expression_get_id( expr, parse_mode ),
    expr->line,
    expr->col,
    expr->exec_num,
    expr->op,
    (expr->suppl.all & ESUPPL_MERGE_MASK),
    ((expr->op == EXP_OP_STATIC) ? 0 : expression_get_id( expr->right, parse_mode )),
    ((expr->op == EXP_OP_STATIC) ? 0 : expression_get_id( expr->left,  parse_mode ))
  );

  if( ESUPPL_OWNS_VEC( expr->suppl ) ) {
    fprintf( file, " " );
    if( parse_mode && EXPR_OWNS_VEC( expr->op ) && (expr->value->suppl.part.owns_data == 0) ) {
      expr->value->suppl.part.owns_data = 1;
    }
    vector_db_write( expr->value, file, (expr->op == EXP_OP_STATIC) );
  }

  if( expr->name != NULL ) {
    fprintf( file, " %s", expr->name );
  } else if( expr->sig != NULL ) {
    fprintf( file, " %s", expr->sig->name );  /* This will be valid for parameters */
  }

  fprintf( file, "\n" );

  PROFILE_END;

}

/*!
 \param root   Pointer to the root expression to display
 \param ofile  Output file to write expression tree to

 Recursively iterates through the specified expression tree, outputting the expressions
 to the specified file.
*/
void expression_db_write_tree(
  expression* root,
  FILE*       ofile
) { PROFILE(EXPRESSION_DB_WRITE_TREE);

  if( root != NULL ) {

    /* Print children first */
    expression_db_write_tree( root->left, ofile );
    expression_db_write_tree( root->right, ofile );

    /* Now write ourselves */
    expression_db_write( root, ofile, TRUE );

  }

  PROFILE_END;

}

/*!
 \param line        String containing database line to read information from.
 \param curr_funit  Pointer to current functional unit that instantiates this expression.
 \param eval        If TRUE, evaluate expression if children are static.

 \throws anonymous expression_create Throw Throw Throw Throw Throw vector_db_read

 Reads in the specified expression information, creates new expression from
 heap, populates the expression with specified information from file and 
 returns that value in the specified expression pointer.  If all is 
 successful, returns TRUE; otherwise, returns FALSE.
*/
void expression_db_read(
  char**     line,
  func_unit* curr_funit,
  bool       eval
) { PROFILE(EXPRESSION_DB_READ);

  int          id;             /* Holder of expression ID */
  expression*  expr;           /* Pointer to newly created expression */
  int          linenum;        /* Holder of current line for this expression */
  unsigned int column;         /* Holder of column alignment information */
  control      exec_num;       /* Holder of expression's execution number */
  control      op;             /* Holder of expression operation */
  esuppl       suppl;          /* Holder of supplemental value of this expression */
  int          right_id;       /* Holder of expression ID to the right */
  int          left_id;        /* Holder of expression ID to the left */
  expression*  right;          /* Pointer to current expression's right expression */
  expression*  left;           /* Pointer to current expression's left expression */
  int          chars_read;     /* Number of characters scanned in from line */
  vector*      vec;            /* Holders vector value of this expression */
  exp_link*    expl;           /* Pointer to found expression in functional unit */
  int          tmpid;          /* ID of statement that the current expression is bound to */

  if( sscanf( *line, "%d %d %x %x %x %x %d %d%n", &id, &linenum, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 8 ) {

    *line = *line + chars_read;

    /* Find functional unit instance name */
    if( curr_funit == NULL ) {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  expression (%d) in database written before its functional unit", id );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "expr Throw C\n" );
      Throw 0;

    } else {

      /* Find right expression */
      if( right_id == 0 ) {
        right = NULL;
      } else if( (expl = exp_link_find( right_id, curr_funit->exp_head )) == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, right_id );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "expr Throw D\n" );
        Throw 0;
      } else {
        right = expl->exp;
      }

      /* Find left expression */
      if( left_id == 0 ) {
        left = NULL;
      } else if( (expl = exp_link_find( left_id, curr_funit->exp_head )) == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, left_id );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "expr Throw E\n" );
        Throw 0;
      } else {
        left = expl->exp;
      }

      /* Create new expression */
      expr = expression_create( right, left, op, ESUPPL_IS_LHS( suppl ), id, linenum,
                                ((column >> 16) & 0xffff), (column & 0xffff), ESUPPL_OWNS_VEC( suppl ) );

      expr->suppl.all = suppl.all;
      expr->exec_num  = exec_num;

      if( op == EXP_OP_DELAY ) {
        expr->suppl.part.type = ETYPE_DELAY;
        expr->elem.scale = &(curr_funit->timescale);
      }

      if( ESUPPL_OWNS_VEC( suppl ) ) {

        Try {

          /* Read in vector information */
          vector_db_read( &vec, line );

        } Catch_anonymous {
          expression_dealloc( expr, FALSE );
          printf( "expr Throw F\n" );
          Throw 0;
        }

        /* Copy expression value */
        vector_dealloc( expr->value );
        expr->value = vec;

      }

      /* Check to see if we are bound to a signal or functional unit */
      if( ((*line)[0] != '\n') && ((*line)[0] != '\0') ) {
        (*line)++;   /* Remove space */
        switch( op ) {
          case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    *line, expr, curr_funit );  break;
          case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        *line, expr, curr_funit );  break;
          case EXP_OP_FORK      :
          case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, *line, expr, curr_funit );  break;
          case EXP_OP_DISABLE   :  bind_add( 1,                 *line, expr, curr_funit );  break;
          default               :  bind_add( 0,                 *line, expr, curr_funit );  break;
        }
      }

      /* If we are an assignment operator, set our vector value to that of the right child */
      if( (op == EXP_OP_ASSIGN)     ||
          (op == EXP_OP_DASSIGN)    ||
          (op == EXP_OP_BASSIGN)    ||
          (op == EXP_OP_RASSIGN)    ||
          (op == EXP_OP_NASSIGN)    ||
          (op == EXP_OP_DLY_ASSIGN) ||
          (op == EXP_OP_DIM)        ||
          (op == EXP_OP_IF)         ||
          (op == EXP_OP_WHILE) ) {

        vector_dealloc( expr->value );
        expr->value = right->value;

      }

      exp_link_add( expr, &(curr_funit->exp_head), &(curr_funit->exp_tail) );

      /*
       If this expression is a constant expression, force the simulator to evaluate
       this expression and all parent expressions of it.
      */
      if( eval && EXPR_IS_STATIC( expr ) && (ESUPPL_IS_LHS( suppl ) == 0) ) {
        exp_link_add( expr, &static_expr_head, &static_expr_tail );
      }
      
    }

  } else {

    print_output( "Unable to read expression value", FATAL, __FILE__, __LINE__ );
    printf( "expr Throw G\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param base  Expression to merge data into.
 \param line  Pointer to CDD line to parse.
 \param same  Specifies if expression to be merged needs to be exactly the same as the existing expression.

 \throws anonymous Throw vector_db_merge

 Parses specified line for expression information and merges contents into the
 base expression.  If the two expressions given are not the same (IDs, op,
 and/or line position differ) we know that the database files being merged 
 were not created from the same design; therefore, display an error message 
 to the user in this case.  If both expressions are the same, perform the 
 merge.
*/
void expression_db_merge(
  expression* base,
  char**      line,
  bool        same
) { PROFILE(EXPRESSION_DB_MERGE);

  int          id;             /* Expression ID field */
  int          linenum;        /* Expression line number */
  unsigned int column;         /* Column information */
  control      exec_num;       /* Execution number */
  control      op;             /* Expression operation */
  esuppl       suppl;          /* Supplemental field */
  int          right_id;       /* ID of right child */
  int          left_id;        /* ID of left child */
  int          chars_read;     /* Number of characters read */

  assert( base != NULL );

  if( sscanf( *line, "%d %d %x %x %x %x %d %d%n", &id, &linenum, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 8 ) {

    *line = *line + chars_read;

    if( (base->op != op) || (base->line != linenum) || (base->col != column) ) {

      print_output( "Attempting to merge databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      printf( "expr Throw H\n" );
      Throw 0;

    } else {

      /* Merge expression supplemental fields */
      base->suppl.all = (base->suppl.all & ESUPPL_MERGE_MASK) | (suppl.all & ESUPPL_MERGE_MASK);

      /* Merge execution number information */
      if( base->exec_num < exec_num ) {
        base->exec_num = exec_num;
      }

      if( ESUPPL_OWNS_VEC( suppl ) ) {

        /* Merge expression vectors */
        vector_db_merge( base->value, line, same );

      }

    }

  } else {

    print_output( "Unable to parse expression line in database.  Unable to merge.", FATAL, __FILE__, __LINE__ );
    printf( "expr Throw I\n" );
    Throw 0;

  }

  PROFILE_END;

}

/*!
 \param op  Expression operation to get string representation of

 \return Returns a non-writable string that contains the user-readable name of the
         specified expression operation.
*/
const char* expression_string_op(
  int op
) { PROFILE(EXPRESSION_STRING_OP);

  assert( (op >= 0) && (op < EXP_OP_NUM) );

  PROFILE_END;

  return( exp_op_info[op].name );

}

/*!
 \param exp  Pointer to expression to display

 Returns a pointer to user_msg that will contain a user-friendly string version of
 the given expression
*/
char* expression_string(
  expression* exp
) { PROFILE(EXPRESSION_STRING);

  unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "%d (%s line %d)", exp->id, expression_string_op( exp->op ), exp->line );
  assert( rv < USER_MSG_LENGTH );

  PROFILE_END;

  return( user_msg );

}

/*!
 \param expr  Pointer to expression to display.

 Displays contents of the specified expression to standard output.  This function
 is called by the funit_display function.
*/
void expression_display(
  expression* expr
) {

  int right_id;  /* Value of right expression ID */
  int left_id;   /* Value of left expression ID */

  assert( expr != NULL );

  if( expr->left == NULL ) {
    left_id = 0;
  } else {
    left_id = expr->left->id;
  }

  if( expr->right == NULL ) {
    right_id = 0;
  } else {
    right_id = expr->right->id;
  }

  printf( "  Expression (%p) =>  id: %d, op: %s, line: %d, col: %x, suppl: %x, exec_num: %u, left: %d, right: %d, ", 
          expr,
          expr->id,
          expression_string_op( expr->op ),
          expr->line,
	  expr->col,
          expr->suppl.all,
          expr->exec_num,
          left_id, 
          right_id );

  if( expr->value->value == NULL ) {
    printf( "NO DATA VECTOR" );
  } else {
    vector_display_value( expr->value->value, expr->value->width );
  }
  printf( "\n" );

}

/*!
 \param expr  Pointer to expression to set true/false indicators of

 This function sets the true/false indicators in the expression supplemental field as
 needed.  This function should only be called by expression_op_func__* functions that
 are NOT events AND whose return value is TRUE AND whose op type is not STATIC or
 PARAM.
*/
static void expression_set_tf_preclear( expression* expr ) {

  /* Clear current TRUE/FALSE indicators */
  expr->suppl.part.eval_t = 0;
  expr->suppl.part.eval_f = 0;
      
  /* Set TRUE/FALSE bits to indicate value */
  if( !expr->value->suppl.part.unknown ) {
    if( expr->value->suppl.part.not_zero ) {
      expr->suppl.part.true   = 1;
      expr->suppl.part.eval_t = 1;
    } else {
      expr->suppl.part.false  = 1;
      expr->suppl.part.eval_f = 1;
    }
  }

}

/*!
 \param expr  Pointer to expression to set true/false indicators of
  
 This function sets the true/false indicators in the expression supplemental field as
 needed.  This function should only be called by expression_op_func__* functions whose
 return value is TRUE AND whose op type is either STATIC or PARAM.
*/
static void expression_set_tf( expression* expr ) {

  /* Set TRUE/FALSE bits to indicate value */
  if( !expr->value->suppl.part.unknown ) {
    if( expr->value->suppl.part.not_zero ) {
      expr->suppl.part.true   = 1; 
      expr->suppl.part.eval_t = 1;
    } else {
      expr->suppl.part.false  = 1;
      expr->suppl.part.eval_f = 1;
    }
  }

}

/*!
 \param expr  Pointer to expression to set eval_a/b supplemental bits

 Sets the eval_a/b supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are unary operations only and own their
 own vectors.
*/
static void expression_set_unary_evals( expression* expr ) {

  nibble    val;
  int       i;
  int       width = expr->value->width;
  vec_data* value = expr->value->value;

  for( i=0; i<width; i++ ) {
    val = value[i].part.exp.value;
    if( val < 2 ) {
      if( val == 0 ) {
        value[i].part.exp.eval_a = 1;
      } else {
        value[i].part.exp.eval_b = 1;
      }
    }
  }

}

/*!
 \param expr  Pointer to expression to set eval_a/b/c supplemental bits

 Sets the eval_a/b/c supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are AND-type combinational operations only and own their
 own vectors.
*/
static void expression_set_and_comb_evals( expression* expr ) {

  nibble    lval, rval;
  int       i;
  int       width  = expr->value->width;
  int       lwidth = expr->left->value->width;
  int       rwidth = expr->right->value->width;
  vec_data* lvalue = expr->left->value->value;
  vec_data* rvalue = expr->right->value->value;
  vec_data* value  = expr->value->value;

  for( i=0; i<width; i++ ) {
    lval = (i < lwidth) ? lvalue[i].part.exp.value : 0;
    rval = (i < rwidth) ? rvalue[i].part.exp.value : 0;
    if( (lval < 2) || (rval < 2) ) {
      value[i].part.exp.eval_a |= (lval == 0) ? 1 : 0;
      value[i].part.exp.eval_b |= (rval == 0) ? 1 : 0;
      value[i].part.exp.eval_c |= ((lval == 1) && (rval == 1)) ? 1 : 0;
    }
  }

}

/*!             
 \param expr  Pointer to expression to set eval_a/b/c supplemental bits
            
 Sets the eval_a/b/c supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are OR-type combinational operations only and own their
 own vectors.
*/            
static void expression_set_or_comb_evals( expression* expr ) {
              
  nibble    lval, rval;
  int       i;  
  int       width  = expr->value->width;
  int       lwidth = expr->left->value->width;
  int       rwidth = expr->right->value->width;
  vec_data* lvalue = expr->left->value->value;
  vec_data* rvalue = expr->right->value->value;
  vec_data* value  = expr->value->value;

  for( i=0; i<width; i++ ) {
    lval = (i < lwidth) ? lvalue[i].part.exp.value : 0;
    rval = (i < rwidth) ? rvalue[i].part.exp.value : 0;
    if( (lval < 2) || (rval < 2) ) {
      value[i].part.exp.eval_a |= (lval == 1) ? 1 : 0;
      value[i].part.exp.eval_b |= (rval == 1) ? 1 : 0;
      value[i].part.exp.eval_c |= ((lval == 0) && (rval == 0)) ? 1 : 0;
    }
  }

}

/*!
 \param expr  Pointer to expression to set eval_a/b/c/d supplemental bits
            
 Sets the eval_a/b/c/d supplemental bits as necessary.  This function should be called
 by expression_op_func__* functions that are OTHER-type combinational operations only and own their
 own vectors. 
*/            
static void expression_set_other_comb_evals( expression* expr ) { 
                
  nibble    lval, rval;
  int       i; 
  int       width  = expr->value->width;
  int       lwidth = expr->left->value->width;
  int       rwidth = expr->right->value->width;
  vec_data* lvalue = expr->left->value->value;
  vec_data* rvalue = expr->right->value->value;
  vec_data* value  = expr->value->value;

  for( i=0; i<width; i++ ) {
    lval = (i < lwidth) ? lvalue[i].part.exp.value : 0;
    rval = (i < rwidth) ? rvalue[i].part.exp.value : 0;
    if( (lval < 2) && (rval < 2) ) {
      value[i].part.exp.eval_a |= ((lval == 0) && (rval == 0)) ? 1 : 0;
      value[i].part.exp.eval_b |= ((lval == 0) && (rval == 1)) ? 1 : 0;
      value[i].part.exp.eval_c |= ((lval == 1) && (rval == 0)) ? 1 : 0;
      value[i].part.exp.eval_d |= ((lval == 1) && (rval == 1)) ? 1 : 0;
    }
  }

}

/*!
 \param expr  Pointer to expression to set 00/01/10/11 supplemental bits.

 Sets the eval_00/01/10/11 supplemental bits as necessary.  This function should be
 called by expression_op_func__* functions that are NOT events and have both the left
 and right expression children present.
*/
static void expression_set_eval_NN( expression* expr ) {

  control lf = ESUPPL_IS_FALSE( expr->left->suppl  );
  control lt = ESUPPL_IS_TRUE(  expr->left->suppl  );
  control rf = ESUPPL_IS_FALSE( expr->right->suppl );
  control rt = ESUPPL_IS_TRUE(  expr->right->suppl );

  expr->suppl.part.eval_00 |= lf & rf;
  expr->suppl.part.eval_01 |= lf & rt;
  expr->suppl.part.eval_10 |= lt & rf;
  expr->suppl.part.eval_11 |= lt & rt;

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs XOR operation.
*/
bool expression_op_func__xor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__XOR);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform XOR operation and gather statistics */
  if( retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, xor_optab ) ) {
    expression_set_tf_preclear( expr );
  }

  /* Gather other coverage stats */
  expression_set_other_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a multiply operation.
*/
bool expression_op_func__multiply(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__MULTIPLY);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform multiply operation and gather statistics */
  if( retval = vector_op_multiply( expr->value, expr->left->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }

  /* Gather other coverage stats */
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 \throws anonymous Throw

 Performs a 32-bit divide operation.
*/
bool expression_op_func__divide(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__DIVIDE);

  vector   vec1;            /* Temporary vector */
  vec_data bit;             /* Holds the value of a single bit in a vector value */
  vec_data value32[32];     /* Holds the value of a 32-bit vector value */
  int      intval1;         /* Integer holder */
  int      intval2;         /* Integer holder */
  int      i;               /* Loop iterator */
  bool     retval = FALSE;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Clear not_zero and unknown bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  if( expr->left->value->suppl.part.unknown || expr->right->value->suppl.part.unknown ) {

    bit.all            = 0;
    bit.part.exp.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  } else {

    vector_init( &vec1, value32, FALSE, 32, VTYPE_VAL );
    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    if( intval2 == 0 ) {
      print_output( "Division by 0 error", FATAL, __FILE__, __LINE__ );
      printf( "expr Throw J\n" );
      Throw 0;
    }

    intval1 = intval1 / intval2;
    vector_from_int( &vec1, intval1 );
    retval = vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );

  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 \throws anonymous Throw

 Performs a 32-bit modulus operation.
*/
bool expression_op_func__mod(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__MOD);

  vector   vec1;            /* Temporary vector */
  vec_data bit;             /* Holds the value of a single bit in a vector value */
  vec_data value32[32];     /* Holds the value of a 32-bit vector value */
  int      intval1;         /* Integer holder */
  int      intval2;         /* Integer holder */
  int      i;               /* Loop iterator */
  bool     retval = FALSE;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  if( expr->left->value->suppl.part.unknown || expr->right->value->suppl.part.unknown ) {

    bit.all            = 0;
    bit.part.exp.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  } else {

    vector_init( &vec1, value32, FALSE, 32, VTYPE_VAL );
    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    if( intval2 == 0 ) {
      print_output( "Division by 0 error", FATAL, __FILE__, __LINE__ );
      printf( "expr Throw K\n" );
      Throw 0;
    }

    intval1 = intval1 % intval2;
    vector_from_int( &vec1, intval1 );
    retval = vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );

  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an addition operation.
*/
bool expression_op_func__add(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__ADD);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform ADD operation and gather coverage statistics */
  if( retval = vector_op_add( expr->value, expr->left->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_other_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a subtraction operation.
*/
bool expression_op_func__subtract(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__SUBTRACT);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform SUBTRACT operation and gather coverage information */
  if( retval = vector_op_subtract( expr->value, expr->left->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_other_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise AND operation.
*/
bool expression_op_func__and(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__AND);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform bitwise AND operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, and_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_and_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise OR operation.
*/
bool expression_op_func__or(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__OR);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform bitwise OR operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, or_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_or_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NAND operation.
*/
bool expression_op_func__nand(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NAND);

  bool retval;  /* Return value for this function */

  /* Perform bitwise NAND operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nand_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_and_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NOR operation.
*/
bool expression_op_func__nor(
                expression*    expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NOR);

  bool retval;  /* Return value for this function */

  /* Perform bitwise NOR operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nor_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_or_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NXOR operation.
*/
bool expression_op_func__nxor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NXOR);

  bool retval;  /* Return value for this function */

  /* Perform bitwise NXOR operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nxor_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_other_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a less-than comparison operation.
*/
bool expression_op_func__lt(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__LT);

  bool retval;  /* Return value for this function */

  /* Perform less-than operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LT ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a greater-than comparison operation.
*/
bool expression_op_func__gt(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__GT);

  bool retval;  /* Return value for this function */

  /* Perform greater-than operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GT ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a left shift operation.
*/
bool expression_op_func__lshift(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__LSHIFT);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform left shift operation and gather coverage information */
  if( retval = vector_op_lshift( expr->value, expr->left->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a right shift operation.
*/
bool expression_op_func__rshift(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__RSHIFT);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform right shift operation and gather coverage information */
  if( vector_op_rshift( expr->value, expr->left->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an arithmetic right shift operation.
*/
bool expression_op_func__arshift(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__ARSHIFT);

  bool retval;  /* Return value for this function */

  /* If this is an operate and assign, copy the contents of left side of the parent BASSIGN to the LAST value */
  if( EXPR_IS_OP_AND_ASSIGN( expr ) == 1 ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
    (void)vector_set_value( expr->left->value, expr->parent->expr->left->value->value, expr->parent->expr->left->value->width, 0, 0 );
  }

  /* Perform arithmetic right shift operation and gather coverage information */
  if( retval = vector_op_arshift( expr->value, expr->left->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an equality (==) comparison operation.
*/
bool expression_op_func__eq(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__EQ);

  bool retval;  /* Return value for this function */

  /* Perform  equality operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_EQ ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an equality (===) comparison operation.
*/
bool expression_op_func__ceq(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__CEQ);

  bool retval;  /* Return value for this function */

  /* Perform equality operation and gather coverage information */
  if( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a less-than-or-equal comparison operation.
*/
bool expression_op_func__le(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__LE);

  bool retval;  /* Return value for this function */

  /* Perform less-than-or-equal operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LE ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a greater-than-or-equal comparison operation.
*/
bool expression_op_func__ge(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__GE);

  bool retval;  /* Return value for this function */

  /* Perform greater-than-or-equal operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GE ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a not-equal (!=) comparison operation.
*/
bool expression_op_func__ne(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NE);

  bool retval;  /* Return value for this function */

  /* Perform not-equal operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_NE ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a not-equal (!==) comparison operation.
*/
bool expression_op_func__cne(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__CNE);

  bool retval;  /* Return value for this function */

  /* Perform not-equal operation and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CNE ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a logical OR operation.
*/
bool expression_op_func__lor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__LOR);

  vector   vec1;     /* Used for logical reduction */
  vector   vec2;     /* Used for logical reduction */
  vec_data value1a;  /* 1-bit nibble value */
  vec_data value1b;  /* 1-bit nibble value */
  bool     retval;   /* Return value for this function */

  vector_init( &vec1, &value1a, FALSE, 1, VTYPE_VAL );
  vector_init( &vec2, &value1b, FALSE, 1, VTYPE_VAL );

  /* Use the unknown and not_zero supplemental fields to setup the values to LOR */
  value1a.part.val.value = expr->left->value->suppl.part.unknown  ? 2 : expr->left->value->suppl.part.not_zero;
  value1b.part.val.value = expr->right->value->suppl.part.unknown ? 2 : expr->right->value->suppl.part.not_zero;

  /* Perform bitwise OR operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, &vec1, &vec2, or_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_or_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a logical AND operation.
*/
bool expression_op_func__land(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__LAND);

  vector   vec1;     /* Used for logical reduction */
  vector   vec2;     /* Used for logical reduction */
  vec_data value1a;  /* 1-bit nibble value */
  vec_data value1b;  /* 1-bit nibble value */
  bool     retval;   /* Return value for this function */

  vector_init( &vec1, &value1a, FALSE, 1, VTYPE_VAL );
  vector_init( &vec2, &value1b, FALSE, 1, VTYPE_VAL );

  /* Use the unknown and not_zero supplemental fields to setup the values to LOR */
  value1a.part.val.value = expr->left->value->suppl.part.unknown  ? 2 : expr->left->value->suppl.part.not_zero;
  value1b.part.val.value = expr->right->value->suppl.part.unknown ? 2 : expr->right->value->suppl.part.not_zero;

  /* Perform AND operation and gather coverage information */
  if( retval = vector_bitwise_op( expr->value, &vec1, &vec2, and_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_and_comb_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a conditional (?:) operation.
*/
bool expression_op_func__cond(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__COND);

  bool retval;  /* Return value for this function */

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  /* Simple vector copy from right side and gather coverage information */
  if( retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a conditional select (?:) operation.
*/
bool expression_op_func__cond_sel(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__COND_SEL);

  bool retval = FALSE;  /* Return value for this function */

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  if( !expr->parent->expr->left->value->suppl.part.unknown ) {
    if( !expr->parent->expr->left->value->suppl.part.not_zero ) {
      retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
    } else {
      retval = vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );
    }
  } else {
    vec_data bitx;
    int      i;
    bitx.all            = 0; 
    bitx.part.exp.value = 2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bitx, 1, 0, i );
    }
  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary invert operation.
*/
bool expression_op_func__uinv(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UINV);

  bool retval;  /* Return value for this function */

  /* Perform unary inversion operation and gather coverage information */
  if( retval = vector_unary_inv( expr->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary AND operation.
*/
bool expression_op_func__uand(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UAND);

  bool retval;  /* Return value for this function */

  /* Perform unary AND operation and gather coverage information */
  if( retval = vector_unary_op( expr->value, expr->right->value, and_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NOT operation.
*/
bool expression_op_func__unot(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UNOT);

  bool retval;  /* Return value for this function */

  /* Perform unary NOT operation and gather coverage information */
  if( retval = vector_unary_not( expr->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary OR operation.
*/
bool expression_op_func__uor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UOR);

  bool retval;  /* Return value for this function */

  /* Perform unary OR operation and gather coverage information */
  if( retval = vector_unary_op( expr->value, expr->right->value, or_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary XOR operation.
*/
bool expression_op_func__uxor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UXOR);

  bool retval;  /* Return value for this function */

  /* Perform unary XOR operation and gather coverage information */
  if( retval = vector_unary_op( expr->value, expr->right->value, xor_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NAND operation.
*/
bool expression_op_func__unand(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UNAND);

  bool retval;  /* Return value for this function */

  /* Perform unary NAND operation and gather coverage information */
  if( retval = vector_unary_op( expr->value, expr->right->value, nand_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NOR operation.
*/
bool expression_op_func__unor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UNOR);

  bool retval;  /* Return value for this function */

  /* Perform unary NOR operation and gather coverage information */
  if( retval = vector_unary_op( expr->value, expr->right->value, nor_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NXOR operation.
*/
bool expression_op_func__unxor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__UNXOR);

  bool retval;  /* Return value for this function */

  /* Perform unary NXOR operation and gather coverage information */
  if( retval = vector_unary_op( expr->value, expr->right->value, nxor_optab ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 No operation is performed -- expression value is assumed to be changed.
*/
bool expression_op_func__null(
  /*@unused@*/ expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NULL);

  /* Set the true/false indicators as necessary */
  if( expr->op != EXP_OP_STATIC ) {
    expression_set_tf_preclear( expr );
  } else {
    expression_set_tf( expr );
  }

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time
    
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.
    
 Performs a signal operation.  This function should be called by any operation that would otherwise
 call the expression_op_func__null operation but have a valid signal pointer.  We just need to copy
 the unknown and not_zero bits from the signal's vector supplemental field to our own.
*/
bool expression_op_func__sig(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__SIG);

  /* Set the unknown and not_zero bits as necessary */
  expr->value->suppl.part.unknown  = expr->sig->value->suppl.part.unknown;
  expr->value->suppl.part.not_zero = expr->sig->value->suppl.part.not_zero;

  /* Gather coverage information */
  if( expr->op != EXP_OP_PARAM ) {
    expression_set_tf_preclear( expr );
  } else {
    expression_set_tf( expr );
  }

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a single bit select operation.
*/
bool expression_op_func__sbit(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__SBIT);

  int       intval;     /* Integer value */
  vec_data* vstart;     /* Calculated starting vector bit */
  int       vwidth;     /* Width of current vector */
  int       dim_lsb;    /* LSB of current signal dimension */
  bool      dim_be;     /* Big endianness of this dimension */
  int       dim_width;  /* Width of current dimension */
  int       exp_dim;    /* Expression dimension */

  if( !expr->left->value->suppl.part.unknown ) {

    /* Calculate starting bit position and width */
    if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
      vstart = expr->parent->expr->left->value->value;
      vwidth = expr->parent->expr->left->value->width;
    } else {
      vstart = expr->sig->value->value;
      vwidth = expr->sig->value->width;
    }

    /* Calculate dimensional information */
    exp_dim = expression_get_curr_dimension( expr );
    if( expr->sig->dim[exp_dim].lsb < expr->sig->dim[exp_dim].msb ) {
      dim_lsb = expr->sig->dim[exp_dim].lsb;
      dim_be  = FALSE;
    } else {
      dim_lsb = expr->sig->dim[exp_dim].msb;
      dim_be  = TRUE;
    }
    dim_width = vsignal_calc_width_for_expr( expr, expr->sig );

    /* Adjust this expression's vector data pointer to point to correct bits in signal */
    intval = (vector_to_int( expr->left->value ) - dim_lsb) * dim_width;
    if( intval < 0 ) {
      expr->value->value = &x_value;
    } else {
      if( dim_be ) {
        if( intval > vwidth ) {
          expr->value->value = &x_value;
        } else {
          expr->value->value = vstart + (vwidth - (intval + expr->value->width));
        }
      } else {
        if( intval >= vwidth ) {
          expr->value->value = &x_value;
        } else {
          expr->value->value = vstart + intval;
        }
      }
    }

    /* If this expression references a memory and is the last unpacked dimension, set the first read vector data bit to 1 */
    if( (expr->value->suppl.part.type == VTYPE_MEM) && ((exp_dim + 1) == expr->sig->udim_num) ) { 
      expr->value->value[0].part.mem.rd = 1;
    }

    /* Set the unknown and not_zero bits accordingly */
    expr->value->suppl.part.unknown  = ((expr->value->value[0].part.exp.value & 0x2) >> 1);
    expr->value->suppl.part.not_zero = (expr->value->value[0].part.exp.value & 0x1);

  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a multi-bit select operation.
*/
bool expression_op_func__mbit(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__MBIT);

  int       intval;     /* Integer value */
  vec_data* vstart;     /* Calculated starting vector bit */
  int       vwidth;     /* Width of vector to use */
  int       dim_lsb;    /* LSB of current signal dimension */
  int       exp_dim;    /* Expression dimension */
  bool      dim_be;     /* Big endianness of this dimension */
  int       dim_width;  /* Width of the current dimension */
  int       i;          /* Loop iterator */

  /* Calculate starting bit position */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vstart = expr->parent->expr->left->value->value;
    vwidth = expr->parent->expr->left->value->width;
  } else {
    vstart = expr->sig->value->value;
    vwidth = expr->sig->value->width;
  }

  /* Calculate signal LSB and big endianness */
  exp_dim = expression_get_curr_dimension( expr );
  if( expr->sig->dim[exp_dim].lsb < expr->sig->dim[exp_dim].msb ) {
    dim_lsb = expr->sig->dim[exp_dim].lsb;
    dim_be  = FALSE;
  } else {
    dim_lsb = expr->sig->dim[exp_dim].msb;
    dim_be  = TRUE;
  }
  dim_width = vsignal_calc_width_for_expr( expr, expr->sig );

  intval = ((dim_be ? vector_to_int( expr->left->value ) : vector_to_int( expr->right->value )) - dim_lsb) * dim_width;
  assert( intval >= 0 );
  if( dim_be ) {
    assert( intval <= vwidth );
    expr->value->value = vstart + (vwidth - (intval + expr->value->width));
  } else {
    assert( intval < vwidth );
    expr->value->value = vstart + intval;
  }

  /* Calculate unknown and not_zero supplemental fields */
  vector_sync_nz_and_unk( expr->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a expansion ({{}}) operation.
*/
bool expression_op_func__expand(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__EXPAND);

  vec_data bit;             /* 1-bit vector value */
  int      i;               /* Loop iterator */
  int      j;               /* Loop iterator */
  int      width;           /* Width to expand this operation to */
  bool     retval = FALSE;  /* Return value for this function */

  bit.all = 0;

  /* Clear the not_zero and unknown bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  if( !expr->left->value->suppl.part.unknown ) {

    for( j=0; j<expr->right->value->width; j++ ) {
      bit.part.exp.value = expr->right->value->value[j].part.exp.value;
      width              = vector_to_int( expr->left->value );
      for( i=0; i<width; i++ ) {
        retval |= vector_set_value( expr->value, &bit, 1, 0, ((j * expr->right->value->width) + i) );
      }
    }

  } else {

    bit.part.exp.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a list (,) operation.
*/
bool expression_op_func__list(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__LIST);

  bool retval = FALSE;  /* Return value for this function */

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  retval |= vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
  retval |= vector_set_value( expr->value, expr->left->value->value,  expr->left->value->width,  0, expr->right->value->width );

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a concatenation ({}) operation.
*/
bool expression_op_func__concat(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__CONCAT);

  bool retval;  /* Return value for this function */

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  /* Perform concatenation operation and gather coverage information */
  if( retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a positive edge event operation.
*/
bool expression_op_func__pedge(
               expression*     expr,
               thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__PEDGE);

  bool              retval;   /* Return value for this function */
  register vec_data value1a;  /* 1-bit vector value */
  register vec_data value1b;  /* 2-bit vector value */

  value1a.all            = 0;
  value1a.part.exp.value = expr->right->value->value[0].part.exp.value;
  value1b.all            = expr->left->value->value[0].all;

  if( (value1a.part.exp.value != value1b.part.exp.value) &&
      ((value1b.part.exp.value == 0) || (value1a.part.exp.value == 1)) &&
      thr->suppl.part.exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  /* Set left LAST value to current value of right */
  expr->left->value->value[0].all = value1a.all;

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negative-edge event operation.
*/
bool expression_op_func__nedge(
               expression*     expr,
               thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NEDGE);

  bool              retval;   /* Return value for this function */
  register vec_data value1a;  /* 1-bit vector value */
  register vec_data value1b;  /* 2-bit vector value */

  value1a.all            = 0;
  value1a.part.exp.value = expr->right->value->value[0].part.exp.value;
  value1b.all            = expr->left->value->value[0].all;

  if( (value1a.part.exp.value != value1b.part.exp.value) &&
      ((value1b.part.exp.value == 1) || (value1a.part.exp.value == 0)) &&
      thr->suppl.part.exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  /* Set left LAST value to current value of right */
  expr->left->value->value[0].all = value1a.all;

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an any-edge event operation.
*/
bool expression_op_func__aedge(
               expression*     expr,
               thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__AEDGE);

  vector   vec;     /* Temporary vector */
  vec_data bit;     /* 1-bit vector value */
  bool     retval;  /* Return value of this function */

  /* If our signal is an event that has been triggered, automatically set ourselves to true */
  if( (expr->right->sig != NULL) && (expr->right->sig->suppl.part.type == SSUPPL_TYPE_EVENT) ) {

    if( expr->right->suppl.part.eval_t == 1 ) {
      if( thr->suppl.part.exec_first ) {
        expr->suppl.part.true   = 1;
        expr->suppl.part.eval_t = 1;
        retval = TRUE;
      } else {
        expr->suppl.part.eval_t = 0;
        retval = FALSE;
      }
    } else {
      retval = FALSE;
    }

  } else {

    vector_init( &vec, &bit, FALSE, 1, VTYPE_VAL );
    (void)vector_op_compare( &vec, expr->left->value, expr->right->value, COMP_CEQ );

    /* If the last value and the current value are NOT equal, we have a fired event */
    if( (bit.part.exp.value == 0) && thr->suppl.part.exec_first ) {
      expr->suppl.part.true   = 1;
      expr->suppl.part.eval_t = 1;
      retval = TRUE;
      VSUPPL_CLR_NZ_AND_UNK( expr->left->value->suppl );
      (void)vector_set_value( expr->left->value, expr->right->value->value, expr->right->value->width, 0, 0 );
    } else {
      expr->suppl.part.eval_t = 0;
      retval = FALSE;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a event OR operation.
*/
bool expression_op_func__eor(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__EOR);

  bool retval;  /* Return value for this function */

  if( (ESUPPL_IS_TRUE( expr->left->suppl ) == 1) || (ESUPPL_IS_TRUE( expr->right->suppl ) == 1) ) {

    expr->suppl.part.eval_t = 1;
    expr->suppl.part.true   = 1;
    retval = TRUE;

    /* Clear eval_t bits in left and right expressions */
    expr->left->suppl.part.eval_t  = 0;
    expr->right->suppl.part.eval_t = 0;

  } else {

    expr->suppl.part.eval_t = 0;
    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a sensitivity list operation.
*/
bool expression_op_func__slist(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__SLIST);

  bool retval;  /* Return value for this function */

  if( ESUPPL_IS_TRUE( expr->right->suppl ) == 1 ) {

    expr->suppl.part.eval_t = 1;
    expr->suppl.part.true   = 1;
    retval = TRUE;

    /* Clear eval_t bit in right expression */
    expr->right->suppl.part.eval_t = 0;

  } else {

    expr->suppl.part.eval_t = 0;
    retval = FALSE;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a delay operation.
*/
bool expression_op_func__delay(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__DELAY);

  bool retval = FALSE;  /* Return value for this function */

  /* Clear the evaluated TRUE indicator */
  expr->suppl.part.eval_t = 0;

  /* If this is the first statement in the current thread, we are executing for the first time */
  if( thr->suppl.part.exec_first ) {

    if( TIME_CMP_LE( thr->curr_time, *time ) || time->final ) {
      expr->suppl.part.eval_t = 1;
      expr->suppl.part.true   = 1;
      retval = TRUE;
    }

  } else {

    sim_time tmp_time;   /* Contains the time that the delay is set for */
    uint64   intval;     /* 64-bit value */

    /* Get number of clocks to delay */
    intval = vector_to_uint64( expr->right->value ) * *(expr->elem.scale);

    /* Populate the tmp_time structure */
    tmp_time.lo    = intval & UINT64(0xffffffff);
    tmp_time.hi    = (intval >> 32) & UINT64(0xffffffff);
    tmp_time.full  = intval;
    tmp_time.final = FALSE;

    /* Add this delay into the delay queue if this is not the final simulation step */
    if( !time->final ) {
      TIME_INC(tmp_time, thr->curr_time);
      sim_thread_insert_into_delay_queue( thr, &tmp_time );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an event trigger (->) operation.
*/
bool expression_op_func__trigger(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__TRIGGER);

  /* Specify that we have triggered */
  expr->sig->value->suppl.part.unknown  = 0;
  expr->sig->value->suppl.part.not_zero = 1;

  /* Propagate event */
  vsignal_propagate( expr->sig, ((thr == NULL) ? time : &(thr->curr_time)) );

  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a case comparison operation.
*/
bool expression_op_func__case(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__CASE);

  bool retval;  /* Return value for this function */

  /* Perform comparison and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a casex comparison operation.
*/
bool expression_op_func__casex(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__CASEX);

  bool retval;  /* Return value for this function */

  /* Perform comparison and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CXEQ ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a casez comparison operation.
*/
bool expression_op_func__casez(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__CASEZ);

  bool retval;  /* Return value for this function */

  /* Perform comparison and gather coverage information */
  if( retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CZEQ ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );
  expression_set_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a case default comparison operation.
*/
bool expression_op_func__default(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__DEFAULT);

  bool     retval;  /* Return value for this function */
  vec_data bit;     /* 1-bit vector value */

  bit.all            = 0;
  bit.part.exp.value = 1;

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  /* Perform operation and gather coverage information */
  if( retval = vector_set_value( expr->value, &bit, 1, 0, 0 ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a blocking assignment operation.
*/
bool expression_op_func__bassign(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__BASSIGN);

  int intval = 0;  /* Integer value */

  /* Perform assignment */
  expression_assign( expr->left, expr->right, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)) );

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a function call operation.
*/
bool expression_op_func__func_call(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__FUNC_CALL);

  bool retval;  /* Return value for this function */

  /* First, simulate the function */
  sim_thread( sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time ), ((thr == NULL) ? time : &(thr->curr_time)) );

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  /* Then copy the function variable to this expression */
  retval = vector_set_value( expr->value, expr->sig->value->value, expr->value->width, 0, 0 );
  
  /* Deallocate the reentrant structure of the current thread (if it exists) */
  if( (thr != NULL) && (thr->ren != NULL) ) {
    reentrant_dealloc( thr->ren, thr->funit, expr );
    thr->ren = NULL;
  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a task call operation.
*/
bool expression_op_func__task_call(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__TASK_CALL);

  (void)sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time );

  /* Gather coverage information */
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( FALSE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a named block call operation.
*/
bool expression_op_func__nb_call(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NB_CALL);

  bool retval = FALSE;  /* Return value for this function */

  /* Add the thread to the active queue */
  thread* tmp = sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time );

  if( ESUPPL_IS_IN_FUNC( expr->suppl ) ) {

    sim_thread( tmp, &(thr->curr_time) );
    retval = TRUE;

  }

  /* Gather coverage information */
  // expression_set_tf_preclear( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a fork operation.
*/
bool expression_op_func__fork(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__FORK);

  (void)sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time );

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a join operation.
*/
bool expression_op_func__join(
  /*@unused@*/ expression*     expr,
               thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__JOIN);

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( thr->active_children == 0 );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a block disable operation.
*/
bool expression_op_func__disable(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__DISABLE);

  sim_kill_thread_with_funit( expr->elem.funit );

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a repeat loop operation.
*/
bool expression_op_func__repeat(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__REPEAT);

  bool retval;  /* Return value for this function */

  retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LT );

  if( expr->value->value[0].part.exp.value == 0 ) {
    vector_from_int( expr->left->value, 0 );
  } else {
    vector_from_int( expr->left->value, (vector_to_int( expr->left->value ) + 1) );
  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an exponential power operation.
*/
bool expression_op_func__exponent(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__EXPONENT);

  bool     retval  = FALSE;  /* Return value for this function */
  vec_data bit;              /* 1-bit vector value */
  vec_data value32[32];      /* 32-bit vector value */
  vector   vec;              /* Temporary vector value */
  int      intval1;          /* Integer value */
  int      intval2;          /* Integer value */
  int      intval3 = 1;      /* Integer value */
  int      i;                /* Loop iterator */

  /* Clear the unknown and not_zero bits */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );

  if( !expr->left->value->suppl.part.unknown && !expr->right->value->suppl.part.unknown ) {

    vector_init( &vec, value32, FALSE, 32, VTYPE_VAL );
    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    for( i=0; i<intval2; i++ ) {
      intval3 *= intval1;
    }

    vector_from_int( &vec, intval3 );
    retval = vector_set_value( expr->value, vec.value, expr->value->width, 0, 0 );

  } else {

    bit.all            = 0;
    bit.part.exp.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a port assignment operation.
*/
bool expression_op_func__passign(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__PASSIGN);

  bool retval = FALSE;  /* Return value for this function */
  int  intval = 0;      /* Integer value */

  /* If the current thread is running an automatic function, create a reentrant structure for it */
  if( (thr != NULL) && (thr->ren == NULL) &&
      ((thr->funit->type == FUNIT_AFUNCTION) || (thr->funit->type == FUNIT_ATASK) || (thr->funit->type == FUNIT_ANAMED_BLOCK)) ) {
    thr->ren = reentrant_create( thr->funit );
  }

  switch( expr->sig->suppl.part.type ) {

    /* If the connected signal is an input type, copy the parameter expression value to this vector */
    case SSUPPL_TYPE_INPUT :
      VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );
      retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
      vsignal_propagate( expr->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      break;

    /*
     If the connected signal is an output type, do an expression assign from our expression value
     to the right expression.
    */
    case SSUPPL_TYPE_OUTPUT :
      expression_assign( expr->right, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)) );
      retval = TRUE;
      break;

    /* We don't currently support INOUT as these are not used in tasks or functions */
    default :
      assert( (expr->sig->suppl.part.type == SSUPPL_TYPE_INPUT) ||
              (expr->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT) );
      break;

  }

  /* Gather coverage information */
  if( retval ) {
    expression_set_tf_preclear( expr );
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a positive variable multi-bit select operation.
*/
bool expression_op_func__mbit_pos(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__MBIT_POS);

  int       intval;  /* Integer value */
  vec_data* vstart;  /* Starting bit position */

  /* Calculate starting bit position */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vstart = expr->parent->expr->left->value->value;
  } else {
    vstart = expr->sig->value->value;
  }

  if( !expr->left->value->suppl.part.unknown ) {

    int exp_dim = expression_get_curr_dimension( expr );
    int lsb     = (expr->sig->dim[exp_dim].lsb < expr->sig->dim[exp_dim].msb) ? expr->sig->dim[exp_dim].lsb : expr->sig->dim[exp_dim].msb;

    intval = (vector_to_int( expr->left->value ) - lsb) * vsignal_calc_width_for_expr( expr, expr->sig );
    assert( intval >= 0 );
    assert( intval < expr->sig->value->width );
    expr->value->value = vstart + intval;

  }

  /* Calculate unknown and not_zero supplemental fields */
  vector_sync_nz_and_unk( expr->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negative variable multi-bit select operation.
*/
bool expression_op_func__mbit_neg(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__MBIT_NEG);

  int       intval1;  /* Integer value */
  int       intval2;  /* Integer value */
  vec_data* vstart;   /* Starting bit position */

  /* Calculate starting bit position */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vstart = expr->parent->expr->left->value->value;
  } else {
    vstart = expr->sig->value->value;
  }

  if( !expr->left->value->suppl.part.unknown ) {

    int exp_dim = expression_get_curr_dimension( expr );
    int lsb     = (expr->sig->dim[exp_dim].lsb < expr->sig->dim[exp_dim].msb) ? expr->sig->dim[exp_dim].lsb : expr->sig->dim[exp_dim].msb;

    intval1 = vector_to_int( expr->left->value ) - lsb;
    intval2 = vector_to_int( expr->right->value );
    assert( intval1 < expr->sig->value->width );
    assert( ((intval1 - intval2) + 1) >= 0 );
    expr->value->value = vstart + ((intval1 - intval2) + 1);

  }

  /* Synchronize the not_zero and unknown bits */
  vector_sync_nz_and_unk( expr->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negate of the specified expression.
*/
bool expression_op_func__negate(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__NEGATE);

  bool retval;  /* Return value for this function */

  /* Perform negate operation and gather coverage information */
  if( retval = vector_op_negate( expr->value, expr->right->value ) ) {
    expression_set_tf_preclear( expr );
  }
  expression_set_unary_evals( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE.

 Performs an immediate increment operation.
*/
bool expression_op_func__iinc(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__IINC);

  /* Perform increment */
  (void)vector_op_inc( expr->left->value );

  /* Copy the left-hand expression vector supplemental bits back to the signal */
  expr->left->sig->value->suppl.part.unknown  = expr->left->value->suppl.part.unknown;
  expr->left->sig->value->suppl.part.not_zero = expr->left->value->suppl.part.not_zero;

  /* Copy the left-hand value to our expression */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );
  (void)vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE.

 Performs a postponed increment operation.
*/
bool expression_op_func__pinc(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__PINC);

  /* Copy the left-hand value to our expression */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );
  (void)vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );

  /* Perform increment */
  (void)vector_op_inc( expr->left->value );

  /* Copy the left-hand expression vector supplemental bits back to the signal */
  expr->left->sig->value->suppl.part.unknown  = expr->left->value->suppl.part.unknown;
  expr->left->sig->value->suppl.part.not_zero = expr->left->value->suppl.part.not_zero;

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an immediate decrement operation.
*/
bool expression_op_func__idec(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__IDEC);

  /* Perform decrement */
  (void)vector_op_dec( expr->left->value );

  /* Copy the left-hand expression vector supplemental bits back to the signal */
  expr->left->sig->value->suppl.part.unknown  = expr->left->value->suppl.part.unknown;
  expr->left->sig->value->suppl.part.not_zero = expr->left->value->suppl.part.not_zero;

  /* Copy the left-hand value to our expression */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );
  (void)vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a postponed decrement operation.
*/
bool expression_op_func__pdec(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__PDEC);

  /* Copy the left-hand value to our expression */
  VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );
  (void)vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );

  /* Perform decrement */
  (void)vector_op_dec( expr->left->value );

  /* Copy the left-hand expression vector supplemental bits back to the signal */
  expr->left->sig->value->suppl.part.unknown  = expr->left->value->suppl.part.unknown;
  expr->left->sig->value->suppl.part.not_zero = expr->left->value->suppl.part.not_zero;

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
    printf( "        " );  expression_display( expr->left );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );

  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a delayed assignment.
*/
bool expression_op_func__dly_assign(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__DLY_ASSIGN);

  bool retval;      /* Return value for this function */
  int  intval = 0;  /* Integer value */

  /* If we are the first statement in the queue, perform the dly_op manually */
  if( thr->suppl.part.exec_first && (expr->right->left->op == EXP_OP_DELAY) ) {
    (void)expression_op_func__dly_op( expr->right, thr, time );
  }

  /* Check the dly_op expression.  If eval_t is set to 1, perform the assignment */
  if( ESUPPL_IS_TRUE( expr->right->suppl ) == 1 ) {
    expression_assign( expr->left, expr->right, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)) );
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an assignment and delay for a delayed assignment.
*/
bool expression_op_func__dly_op(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__DLY_OP);

  /* If we are not waiting for the delay to occur, copy the contents of the operation */
  if( !thr->suppl.part.exec_first ) {
    VSUPPL_CLR_NZ_AND_UNK( expr->value->suppl );
    (void)vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
  }

  /* Explicitly call the delay/event.  If the delay is complete, set eval_t to TRUE */
  if( expr->left->op == EXP_OP_DELAY ) {
    expr->suppl.part.eval_t = exp_op_info[expr->left->op].func( expr->left, thr, time );
  } else {
    expr->suppl.part.eval_t = expr->left->suppl.part.eval_t;
  }
    
  PROFILE_END;

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a repeated delay for a given assignment.
*/
bool expression_op_func__repeat_dly(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__REPEAT_DLY);

  bool retval = FALSE;  /* Return value for this function */

  /* If the delay condition has been met, call the repeat operation */
  if( exp_op_info[expr->right->op].func( expr->right, thr, time ) ) {

    /* Execute repeat operation */
    (void)expression_op_func__repeat( expr->left, thr, time );

    /* If the repeat operation evaluated to TRUE, perform delay operation */
    if( expr->left->value->value[0].part.exp.value == 1 ) {
      (void)exp_op_info[expr->right->op].func( expr->right, thr, time );
      expr->suppl.part.eval_t = 0;

    /* Otherwise, we are done with the repeat/delay sequence */
    } else {
      expr->suppl.part.eval_t = 1;
      retval = TRUE;
    }

  }
  
  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a wait statement operation.
*/
bool expression_op_func__wait(
               expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__WAIT);

  bool retval = TRUE;  /* Return value for this function */

  /* If the right expression evaluates to TRUE, continue; otherwise, do a context switch */
  if( expr->right->value->suppl.part.not_zero ) {
    expr->suppl.part.eval_t = 1;
    expr->suppl.part.true   = 1;
    retval                  = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval                  = FALSE;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression
 \param time  Pointer to current simulation time

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $finish statement operation.
*/
bool expression_op_func__finish(
  /*@unused@*/ expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__FINISH);

  sim_finish();

  return( FALSE );

}

/*!                                        
 \param expr  Pointer to expression to perform operation on   
 \param thr   Pointer to thread containing this expression    
 \param time  Pointer to current simulation time
                                           
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.  
                                           
 Performs a $stop statement operation.
*/                                         
bool expression_op_func__stop(
  /*@unused@*/ expression*     expr,
  /*@unused@*/ thread*         thr,
  /*@unused@*/ const sim_time* time
) { PROFILE(EXPRESSION_OP_FUNC__STOP);

  sim_stop();

  return( FALSE );

}

/*!
 \param expr  Pointer to expression to set value to.
 \param thr   Pointer to current thread being simulated. 
 \param time  Pointer to current simulation time.

 \return Returns TRUE if the assigned expression value was different than the previously stored value;
         otherwise, returns FALSE.

 \throw anonymous func

 Performs expression operation.  This function must only be run after its
 left and right expressions have been calculated during this clock period.
 Sets the value of the operation in its own vector value and updates the
 suppl nibble as necessary.
*/
bool expression_operate(
  expression*     expr,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_OPERATE);

  bool     retval = TRUE;   /* Return value for this function */
  vector   vec;             /* Used for logical reduction */ 
  vec_data bit;             /* Bit holder for some ops */
  control  lf, lt, rf, rt;  /* Specify left and right WAS_TRUE/WAS_FALSE values */
  int      i;               /* Loop iterator */

  if( expr != NULL ) {

#ifdef DEBUG_MODE
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "      In expression_operate, id: %d, op: %s, line: %d, addr: %p",
                                  expr->id, expression_string_op( expr->op ), expr->line, expr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    assert( expr->value != NULL );

    /* Call expression operation */
    retval = exp_op_info[expr->op].func( expr, thr, time );

    /* If this expression is attached to an FSM, perform the FSM calculation now */
    if( expr->table != NULL ) {
      fsm_table_set( expr->table );
      /* If from_state was not specified, we need to copy the current contents of to_state to from_state */
      if( expr->table->from_state->id == expr->id ) {
        vector_copy( expr->value, expr->table->from_state->value );
      }
    }

    /* Specify that we have executed this expression */
    (expr->exec_num)++;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr    Pointer to top of expression tree to perform recursive operations.
 \param funit   Pointer to functional unit containing this expression.
 \param sizing  Set to TRUE if we are evaluating for purposes of sizing.

 \throws anonymous expression_resize expression_operate_recursively expression_operate_recursively
 
 Recursively performs the proper operations to cause the top-level expression to
 be set to a value.  This function is called during the parse stage to derive 
 pre-CDD widths of multi-bit expressions.  Each MSB/LSB is an expression tree that 
 needs to be evaluated to set the width properly on the MBIT_SEL expression.
*/
void expression_operate_recursively(
  expression* expr,
  func_unit*  funit,
  bool        sizing
) { PROFILE(EXPRESSION_OPERATE_RECURSIVELY);
    
  if( expr != NULL ) {
    
    /* Create dummy time */
    sim_time time = {0,0,0,FALSE};

    /* Evaluate children */
    expression_operate_recursively( expr->left,  funit, sizing );
    expression_operate_recursively( expr->right, funit, sizing );
    
    if( sizing ) {

      /*
       Non-static expression found where static expression required.  Simulator
       should catch this error before us, so no user error (too much work to find
       expression in functional unit expression list for now.
      */
      assert( (expr->op != EXP_OP_SBIT_SEL) &&
              (expr->op != EXP_OP_MBIT_SEL) &&
              (expr->op != EXP_OP_MBIT_POS) &&
              (expr->op != EXP_OP_MBIT_NEG) );

      /* Resize current expression only */
      expression_resize( expr, funit, FALSE, TRUE );
    
    }
    
    /* Perform operation */
    (void)expression_operate( expr, NULL, &time );

    if( sizing ) {

      /* Clear out the execution number value since we aren't really simulating this */
      expr->exec_num = 0;

    }
    
  }
  
  PROFILE_END;

}

/*!
 \param expr  Pointer to expression to evaluate.
 
 \return Returns TRUE if expression contains only static expressions; otherwise, returns FALSE.
 
 Recursively iterates through specified expression tree and returns TRUE if all of
 the leaf expressions are static expressions (STATIC or parameters).
*/
bool expression_is_static_only(
  expression* expr
) { PROFILE(EXPRESSION_IS_STATIC_ONLY);

  if( expr != NULL ) {

    if( (EXPR_IS_STATIC( expr ) == 1) ||
        (ESUPPL_IS_LHS( expr->suppl ) == 1) ||
        ((expr->op == EXP_OP_SIG) && (expr->sig != NULL) &&
         ((expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM) || (expr->sig->suppl.part.type == SSUPPL_TYPE_ENUM))) ) {
      return( TRUE );
    } else {
      return( (expr->op != EXP_OP_MBIT_SEL)           &&
              (expr->op != EXP_OP_SBIT_SEL)           &&
              (expr->op != EXP_OP_SIG)                &&
              (expr->op != EXP_OP_FUNC_CALL)          &&
              (EXPR_IS_OP_AND_ASSIGN( expr ) == 0)    &&
              expression_is_static_only( expr->left ) &&
              expression_is_static_only( expr->right ) );
    }

  } else {

    return( TRUE );

  }

}

/*!
 \param expr  Pointer to expression to check

 \return Returns TRUE if specified expression is on the LHS of a blocking assignment operator.
*/
static bool expression_is_assigned(
  expression* expr
) { PROFILE(EXPRESSION_IS_ASSIGNED);

  bool retval = FALSE;  /* Return value for this function */

  assert( expr != NULL );

  /* If this expression is a trigger then this is an assigned expression */
  if( expr->op == EXP_OP_TRIGGER ) {

    retval = TRUE;

  } else if( (ESUPPL_IS_LHS( expr->suppl ) == 1) &&
             ((expr->op == EXP_OP_SIG)      ||
              (expr->op == EXP_OP_SBIT_SEL) ||
              (expr->op == EXP_OP_MBIT_SEL) ||
              (expr->op == EXP_OP_MBIT_POS) ||
              (expr->op == EXP_OP_MBIT_NEG)) ) {

    while( (expr != NULL) && (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->op != EXP_OP_BASSIGN) && (expr->op != EXP_OP_RASSIGN) ) {
      expr = expr->parent->expr;
    }

    retval = (expr != NULL) && ((expr->op == EXP_OP_BASSIGN) || (expr->op == EXP_OP_RASSIGN)) ;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to check

 \return Returns TRUE if the specifies expression belongs in a single or mult-bit select expression
*/
bool expression_is_bit_select(
  expression* expr
) { PROFILE(EXPRESSION_IS_BIT_SELECT);

  bool retval = FALSE;  /* Return value for this function */

  if( (expr != NULL) && (ESUPPL_IS_ROOT( expr->suppl ) == 0) ) {

    if( (expr->parent->expr->op == EXP_OP_SBIT_SEL) ||
        (expr->parent->expr->op == EXP_OP_MBIT_SEL) ||
        (expr->parent->expr->op == EXP_OP_MBIT_POS) ||
        (expr->parent->expr->op == EXP_OP_MBIT_NEG) ) {
      retval = TRUE;
    } else {
      retval = expression_is_bit_select( expr->parent->expr );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to expression to check for last select

 \return Returns TRUE if the specified expression is the last (most right-hand) part/bit select of a signal;
         otherwise, returns FALSE.
*/
bool expression_is_last_select(
  expression* expr
) { PROFILE(EXPRESSION_IS_LAST_SELECT);

  return( (ESUPPL_IS_ROOT( expr->suppl ) == 0) &&
          ( ((expr->parent->expr->op == EXP_OP_DIM) &&
             (expr->parent->expr->right == expr) &&
             (ESUPPL_IS_ROOT( expr->parent->expr->suppl ) == 0) &&
             (expr->parent->expr->parent->expr->op != EXP_OP_DIM)) ||
            (expr->parent->expr->op != EXP_OP_DIM) ) );

}

/*!
 \param expr  Pointer to expression to examine

 \return Returns TRUE if the specified expression is in an RASSIGN expression tree; otherwise,
         returns FALSE.
*/
bool expression_is_in_rassign(
  expression* expr
) { PROFILE(EXPRESSION_IS_IN_RASSIGN);

  bool retval = FALSE;  /* Return value for this function */

  if( expr != NULL ) {

    if( expr->op == EXP_OP_RASSIGN ) {
      retval = TRUE;
    } else if( ESUPPL_IS_ROOT( expr->suppl ) == 0 ) {
      retval = expression_is_in_rassign( expr->parent->expr );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \param expr  Pointer to current expression to evaluate

 Checks to see if the expression is in the LHS of a BASSIGN expression tree.  If it is, it sets the
 assigned supplemental field of the expression's signal vector to indicate the the value of this
 signal will come from Covered instead of the dumpfile.  This is called in the bind_signal function
 after the expression and signal have been bound (only in parsing stage).
*/
void expression_set_assigned(
  expression* expr
) { PROFILE(EXPRESSION_SET_ASSIGNED);

  expression* curr;  /* Pointer to current expression */

  assert( expr != NULL );

  if( ESUPPL_IS_LHS( expr->suppl ) == 1 ) {

    curr = expr;
    while( (ESUPPL_IS_ROOT( curr->suppl ) == 0) && (curr->op != EXP_OP_BASSIGN) && (curr->op != EXP_OP_RASSIGN) ) {
      curr = curr->parent->expr;
    }

    /*
     If we are on the LHS of a BASSIGN operator, set the assigned bit to indicate that
     this signal will be assigned by Covered and not the dumpfile.
    */
    if( (curr->op == EXP_OP_BASSIGN) || (curr->op == EXP_OP_RASSIGN) ) {
      expr->sig->suppl.part.assigned = 1;
    }

  }

  PROFILE_END;

}

/*!
 \param lhs   Pointer to current expression on left-hand-side of assignment to calculate for.
 \param rhs   Pointer to the right-hand-expression that will be assigned from.
 \param lsb   Current least-significant bit in rhs value to start assigning.
 \param time  Specifies current simulation time when expression assignment occurs.

 Recursively iterates through specified LHS expression, assigning the value from the RHS expression.
 This is called whenever a blocking assignment expression is found during simulation.
*/
void expression_assign(
  expression*     lhs,
  expression*     rhs,
  int*            lsb,
  thread*         thr,
  const sim_time* time
) { PROFILE(EXPRESSION_ASSIGN);

  int       intval1;    /* Integer value to use */
  vec_data* vstart;     /* Starting vector data */
  int       vwidth;     /* Width of vector data to select from */
  bool      assign;     /* Set to TRUE if we should perform assignment */
  int       exp_dim;    /* Current LHS expression dimension */
  int       dim_width;  /* Current LHS dimension width */
  int       dim_lsb;    /* Current LHS dimension LSB */
  bool      dim_be;     /* Current LHS dimension endianness */

  if( lhs != NULL ) {

    /* Calculate starting vector value bit and signal LSB/BE for LHS */
    if( lhs->sig != NULL ) {
      if( (lhs->parent->expr->op == EXP_OP_DIM) && (lhs->parent->expr->right == lhs) ) {
        vstart = lhs->parent->expr->left->value->value;
        vwidth = lhs->parent->expr->left->value->width;
      } else {
        /* Get starting vector bit from signal itself */
        vstart = lhs->sig->value->value;
        vwidth = lhs->sig->value->width;
      }
      exp_dim = expression_get_curr_dimension( lhs );
      if( lhs->sig->dim[exp_dim].lsb < lhs->sig->dim[exp_dim].msb ) {
        dim_lsb = lhs->sig->dim[exp_dim].lsb;
        dim_be  = FALSE;
      } else {
        dim_lsb = lhs->sig->dim[exp_dim].msb;
        dim_be  = TRUE;
      }
      dim_width = vsignal_calc_width_for_expr( lhs, lhs->sig );
    }

    /* Calculate assignment */
    assign = expression_is_last_select( lhs ) && (lhs->op != EXP_OP_DIM);

#ifdef DEBUG_MODE
    if( assign ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "        In expression_assign, lhs_op: %s, rhs_op: %s, lsb: %d, time: %llu",
                                  expression_string_op( lhs->op ), expression_string_op( rhs->op ), *lsb, time->full );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    switch( lhs->op ) {
      case EXP_OP_SIG      :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          if( assign ) {
            VSUPPL_CLR_NZ_AND_UNK( lhs->value->suppl );
            (void)vector_set_value( lhs->value, rhs->value->value, rhs->value->width, *lsb, 0 );
            if( rhs->value->width < lhs->value->width ) {
              (void)vector_bit_fill( lhs->value, lhs->value->width, (rhs->value->width + *lsb) );
            }
            lhs->sig->value->suppl.part.unknown  = lhs->value->suppl.part.unknown;
            lhs->sig->value->suppl.part.not_zero = lhs->value->suppl.part.not_zero;
#ifdef DEBUG_MODE
            if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
              printf( "        " );  vsignal_display( lhs->sig );
            }
#endif
            vsignal_propagate( lhs->sig, time );
          }
        }
        if( assign ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
      case EXP_OP_SBIT_SEL :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          sim_expression( lhs->left, thr, time, TRUE );
          if( !lhs->left->value->suppl.part.unknown ) {
            intval1 = (vector_to_int( lhs->left->value ) - dim_lsb) * dim_width;
            if( intval1 >= 0 ) {           // Only perform assignment if selected bit is within range
              if( dim_be ) {
                if( intval1 <= vwidth ) {  // Only perform assignment if selected bit is within range
                  lhs->value->value = vstart + (vwidth - (intval1 + lhs->value->width));
                } else {
                  assign = FALSE;
                }
              } else {
                if( intval1 < vwidth ) {   // Only perform assignment if selected bit is within range
                  lhs->value->value = vstart + intval1;
                } else {
                  assign = FALSE;
                }
              }
            } else {
              assign = FALSE;
            }
          }
          if( assign ) {
            VSUPPL_CLR_NZ_AND_UNK( lhs->value->suppl );
            (void)vector_set_value( lhs->value, rhs->value->value, rhs->value->width, *lsb, 0 );
            if( rhs->value->width < lhs->value->width ) {
              (void)vector_bit_fill( lhs->value, lhs->value->width, (rhs->value->width + *lsb) );
            }
            vector_sync_nz_and_unk( lhs->sig->value );
#ifdef DEBUG_MODE
            if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
              printf( "        " );  vsignal_display( lhs->sig );
            }
#endif
            vsignal_propagate( lhs->sig, time );
          }
        }
        if( assign ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
      case EXP_OP_MBIT_SEL :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          intval1 = ((dim_be ? vector_to_int( lhs->left->value ) : vector_to_int( lhs->right->value )) - dim_lsb) * dim_width;
          assert( intval1 >= 0 );
          if( dim_be ) {
            assert( intval1 <= vwidth );
            lhs->value->value = vstart + (vwidth - (intval1 + lhs->value->width));
          } else {
            assert( intval1 < vwidth );
            lhs->value->value = vstart + intval1;
          }
          if( assign ) {
            VSUPPL_CLR_NZ_AND_UNK( lhs->value->suppl );
            (void)vector_set_value( lhs->value, rhs->value->value, rhs->value->width, *lsb, 0 );
            if( rhs->value->width < lhs->value->width ) {
              (void)vector_bit_fill( lhs->value, lhs->value->width, (rhs->value->width + *lsb) );
            }
            vector_sync_nz_and_unk( lhs->sig->value );
#ifdef DEBUG_MODE
            if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
              printf( "        " );  vsignal_display( lhs->sig );
            }
#endif
    	    vsignal_propagate( lhs->sig, time );
          }
        }
        if( assign ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
#ifdef NOT_SUPPORTED
      case EXP_OP_MBIT_POS :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          sim_expression( lhs->left, thr, time, TRUE );
          if( !lhs->left->value->suppl.part.unknown ) {
            intval1 = (vector_to_int( lhs->left->value ) - dim_lsb) * lhs->value->width;
            intval2 = vector_to_int( lhs->right->value ) * lhs->value->width;
            assert( intval1 >= 0 );
            assert( ((intval1 + intval2) - 1) < lhs->sig->value->width );
            lhs->value->value = vstart + intval1;
          }
          if( assign ) {
            VSUPPL_CLR_NZ_AND_UNK( lhs->value->suppl );
            (void)vector_set_value( lhs->value, rhs->value->value, intval2, *lsb, 0 );
            vector_sync_nz_and_unk( lhs->sig->value );
#ifdef DEBUG_MODE
            if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
              printf( "        " );  vsignal_display( lhs->sig );
            }
#endif
            vsignal_propagate( lhs->sig, time );
          }
        }
        if( assign ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
      case EXP_OP_MBIT_NEG :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          sim_expression( lhs->left, thr, time, TRUE );
          if( !lhs->left->value->part.unknown ) {
            intval1 = (vector_to_int( lhs->left->value ) - dim_lsb) * lhs->value->width;
            intval2 = vector_to_int( lhs->right->value ) * lhs->value->width;
            assert( intval1 < lhs->sig->value->width );
            assert( ((intval1 - intval2) + 1) >= 0 );
            lhs->value->value = vstart + ((intval1 - intval2) + 1);
          }
          if( assign ) {
            VSUPPL_CLR_NZ_AND_UNK( lhs->value->suppl );
            (void)vector_set_value( lhs->value, rhs->value->value, intval2, *lsb, 0 );
            vector_sync_nz_and_unk( lhs->sig->value );
#ifdef DEBUG_MODE
            if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
              printf( "        " );  vsignal_display( lhs->sig );
            }
#endif
            vsignal_propagate( lhs->sig, time );
          }
        }
        if( assign ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
#endif
      case EXP_OP_CONCAT   :
      case EXP_OP_LIST     :
        expression_assign( lhs->right, rhs, lsb, thr, time );
        expression_assign( lhs->left,  rhs, lsb, thr, time );
        break;
      case EXP_OP_DIM      :
        expression_assign( lhs->left,  rhs, lsb, thr, time );
        expression_assign( lhs->right, rhs, lsb, thr, time );
        break;
      case EXP_OP_STATIC   :
        break;
      default:
        /* This is an illegal expression to have on the left-hand-side of an expression */
#ifdef NOT_SUPPORTED
        assert( (lhs->op == EXP_OP_SIG)      ||
                (lhs->op == EXP_OP_SBIT_SEL) ||
                (lhs->op == EXP_OP_MBIT_SEL) ||
                (lhs->op == EXP_OP_MBIT_POS) ||
                (lhs->op == EXP_OP_MBIT_NEG) ||
                (lhs->op == EXP_OP_CONCAT)   ||
                (lhs->op == EXP_OP_LIST)     ||
                (lhs->op == EXP_OP_DIM) );
#else
	assert( (lhs->op == EXP_OP_SIG)      ||
	        (lhs->op == EXP_OP_SBIT_SEL) ||
		(lhs->op == EXP_OP_MBIT_SEL) ||
		(lhs->op == EXP_OP_CONCAT)   ||
		(lhs->op == EXP_OP_LIST)     ||
                (lhs->op == EXP_OP_DIM) );
#endif
        break;
    }

  }

  PROFILE_END;

}

/*!
 \param expr      Pointer to root expression to deallocate.
 \param exp_only  Removes only the specified expression and not its children.

 Deallocates all heap memory allocated with the malloc routine.
*/
void expression_dealloc(
  expression* expr,
  bool        exp_only
) { PROFILE(EXPRESSION_DEALLOC);

  int        op;        /* Temporary operation holder */
  exp_link*  tmp_expl;  /* Temporary pointer to expression list */
  statement* tmp_stmt;  /* Temporary pointer to statement */

  if( expr != NULL ) {

    op = expr->op;

    if( ESUPPL_OWNS_VEC( expr->suppl ) ) {

      /* Free up memory from vector value storage */
      vector_dealloc( expr->value );
      expr->value = NULL;

      /* If this is a named block call or fork call, remove the statement that this expression points to */
      if( (expr->op == EXP_OP_NB_CALL) || (expr->op == EXP_OP_FORK) ) {

        if( !exp_only ) {
#ifdef DEBUG_MODE
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block starting at line %d because it is a NB_CALL and its calling expression is being removed",
                                      expr->elem.funit->first_stmt->exp->line );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
          stmt_blk_add_to_remove_list( expr->elem.funit->first_stmt );
        } else {
          bind_remove( expr->id, FALSE );
        }

      /* If this is a task call, remove the bind */
      } else if( expr->op == EXP_OP_TASK_CALL ) {
         
        bind_remove( expr->id, FALSE );

      } else if( expr->op == EXP_OP_FUNC_CALL ) {

        /* Remove this expression from the attached signal's expression list (if the signal has not been deallocated yet) */
        if( expr->sig != NULL ) {
          exp_link_remove( expr, &(expr->sig->exp_head), &(expr->sig->exp_tail), FALSE );
        }

        bind_remove( expr->id, FALSE );

      /* Otherwise, we assume (for now) that the expression is a signal */
      } else {

        if( expr->sig == NULL ) {

          /* Remove this expression from the binding list */
          bind_remove( expr->id, expression_is_assigned( expr ) );

        } else {

          /* Remove this expression from the attached signal's expression list */
          exp_link_remove( expr, &(expr->sig->exp_head), &(expr->sig->exp_tail), FALSE );

          /* Clear the assigned bit of the attached signal */
          if( expression_is_assigned( expr ) ) {
  
            expr->sig->suppl.part.assigned = 0;

            /* If this signal must be assigned, remove all statement blocks that reference this signal */
            if( (expr->sig->suppl.part.mba == 1) && !exp_only ) {
              tmp_expl = expr->sig->exp_head;
              while( tmp_expl != NULL ) {
                if( (tmp_stmt = expression_get_root_statement( tmp_expl->exp )) != NULL ) {
#ifdef DEBUG_MODE
                  print_output( "Removing statement block because a statement block is being removed that assigns an MBA", DEBUG, __FILE__, __LINE__ );
#endif
                  stmt_blk_add_to_remove_list( tmp_stmt );
                }
                tmp_expl = tmp_expl->next;
              }
            }

          }
        
        }

      }

    } else {

      /* Remove our expression from its signal, if we have one */
      if( expr->sig != NULL ) {
        exp_link_remove( expr, &(expr->sig->exp_head), &(expr->sig->exp_tail), FALSE );
      }

    }

    /* Deallocate children */
    if( !exp_only ) {

      if( EXPR_RIGHT_DEALLOCABLE( expr ) ) {
        expression_dealloc( expr->right, FALSE );
        expr->right = NULL;
      }

      if( EXPR_LEFT_DEALLOCABLE( expr ) ) {
        expression_dealloc( expr->left, FALSE );
        expr->left = NULL;
      }

    }

    /* Free up memory for the parent pointer */
    free_safe( expr->parent, sizeof( expr_stmt ) );

    /* If name contains data, free it */
    free_safe( expr->name, (strlen( expr->name ) + 1) );

    /* Remove this expression memory */
    free_safe( expr, sizeof( expression ) );

  }

  PROFILE_END;

}


/* 
 $Log$
 Revision 1.319  2008/04/05 19:19:50  phase1geo
 Fixing increment issue and updating regressions.

 Revision 1.318  2008/04/04 20:06:39  phase1geo
 More fixes per regression runs.  Checkpointing.

 Revision 1.317  2008/04/02 05:39:50  phase1geo
 More updates to support error memory deallocation.  Full regression still
 fails at this point.  Checkpointing.

 Revision 1.316  2008/03/31 22:00:38  phase1geo
 Fixing issue with mbit_pos and mbit_neg expressions as found in regression.
 Updated regression files.  Checkpointing.

 Revision 1.315  2008/03/31 21:40:23  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.314  2008/03/30 05:24:01  phase1geo
 Fixing a few more bugs.  Updated regression files per these changes.  Full regression
 still not running cleanly at this point.  Checkpointing.

 Revision 1.313  2008/03/30 05:14:32  phase1geo
 Optimizing sim_expr_changed functionality and fixing bug 1928475.

 Revision 1.312  2008/03/29 18:38:55  phase1geo
 Adding sbit_sel3* diagnostics from stable branch.

 Revision 1.311  2008/03/28 18:28:26  phase1geo
 Fixing bug in trigger expression function due to recent changes.

 Revision 1.310  2008/03/28 17:27:00  phase1geo
 Fixing expression assignment problem due to recent changes.  Updating
 regression files per changes.

 Revision 1.309  2008/03/28 14:03:50  phase1geo
 Removing param check from expression_op_func__null function.

 Revision 1.308  2008/03/28 02:31:57  phase1geo
 Fixing more problems with expression coverage gathering functions.

 Revision 1.307  2008/03/27 22:23:34  phase1geo
 More fixes for regression failures due to recent changes.  Still more work
 to go here, however.

 Revision 1.306  2008/03/27 18:51:46  phase1geo
 Fixing more issues with PASSIGN and BASSIGN operations.

 Revision 1.305  2008/03/27 16:12:52  phase1geo
 Fixing memory write issue.

 Revision 1.304  2008/03/27 06:09:58  phase1geo
 Fixing some regression errors.  Checkpointing.

 Revision 1.303  2008/03/26 22:41:06  phase1geo
 More fixes per latest changes.

 Revision 1.302  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.301  2008/03/26 02:39:05  phase1geo
 Completed initial pass of expression operation function optimizations.  Still work
 to go here.

 Revision 1.300  2008/03/25 23:17:46  phase1geo
 More updates to expression operation functions for performance enhancements.

 Revision 1.299  2008/03/25 13:51:19  phase1geo
 Starting to work on performance upgrad for expression operations.  This
 work is not completed at this point.  Checkpointing.

 Revision 1.298  2008/03/24 13:16:46  phase1geo
 More changes for memory allocation/deallocation issues.  Things are still pretty
 broke at the moment.

 Revision 1.297  2008/03/22 05:23:24  phase1geo
 More attempts to fix memory problems.  Things are still pretty broke at the moment.

 Revision 1.296  2008/03/22 02:21:07  phase1geo
 Checking in start of changes to properly handle the deallocation of vector
 memory from expressions.  Things are pretty broke at this point!

 Revision 1.295  2008/03/21 21:16:38  phase1geo
 Removing UNUSED_* types in lexer due to a bug that was found in the usage of
 ignore_mode in the lexer (a token that should have been ignored was not due to
 the parser's need to examine the created token for branch traversal purposes).
 This cleans up the parser also.

 Revision 1.294  2008/03/21 04:33:45  phase1geo
 Fixing bug 1921909.  Also fixing issue with new check_mem regression script.

 Revision 1.293  2008/03/20 21:52:38  phase1geo
 Updates for memory checking.

 Revision 1.292  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.291  2008/03/14 22:00:18  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.290  2008/03/12 05:09:43  phase1geo
 More exception handling updates.  Added TODO item of creating a graduated test list
 from merged CDD files.

 Revision 1.289  2008/03/11 22:21:01  phase1geo
 Continuing to do next round of exception handling.  Checkpointing.

 Revision 1.288  2008/03/11 22:06:47  phase1geo
 Finishing first round of exception handling code.

 Revision 1.287  2008/03/09 20:45:47  phase1geo
 More exception handling updates.

 Revision 1.286  2008/03/04 22:46:07  phase1geo
 Working on adding check_exceptions.pl script to help me make sure that all
 exceptions being thrown are being caught and handled appropriately.  Other
 code adjustments are made in regards to exception handling.

 Revision 1.285  2008/03/04 06:46:48  phase1geo
 More exception handling updates.  Still work to go.  Checkpointing.

 Revision 1.284  2008/03/04 00:09:20  phase1geo
 More exception handling.  Checkpointing.

 Revision 1.283  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.282  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.281  2008/02/08 23:58:06  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.280  2008/02/01 06:37:07  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.279  2008/01/23 20:48:03  phase1geo
 Fixing bug 1878134 and adding new diagnostics to regression suite to verify
 its behavior.  Full regressions pass.

 Revision 1.278  2008/01/22 03:53:17  phase1geo
 Fixing bug 1876417.  Removing obsolete code in expr.c.

 Revision 1.277  2008/01/19 05:57:14  phase1geo
 Fixing bug 1875180.  The anyedge operator now properly compares the values for
 changes.  Added aedge1 and aedge1.1 diagnostics to regression suite to verify
 this fix.  Updated bad CDD files per this change.

 Revision 1.276  2008/01/16 05:01:22  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.275  2008/01/15 23:01:14  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.274  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.273  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.272  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.271  2008/01/07 05:01:57  phase1geo
 Cleaning up more splint errors.

 Revision 1.270  2008/01/02 06:00:08  phase1geo
 Updating user documentation to include the CLI and profiling documentation.
 (CLI documentation is not complete at this time).  Fixes bug 1861986.

 Revision 1.269  2007/12/31 23:43:36  phase1geo
 Fixing bug 1858408.  Also fixing issues with vector_op_add functionality.

 Revision 1.268  2007/12/20 05:18:30  phase1geo
 Fixing another regression bug with running in --enable-debug mode and removing unnecessary output.

 Revision 1.267  2007/12/20 04:47:50  phase1geo
 Fixing the last of the regression failures from previous changes.  Removing unnecessary
 output used for debugging.

 Revision 1.266  2007/12/19 22:54:35  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.265  2007/12/19 14:37:29  phase1geo
 More compiler fixes (still more to go).  Checkpointing.

 Revision 1.264  2007/12/19 04:27:52  phase1geo
 More fixes for compiler errors (still more to go).  Checkpointing.

 Revision 1.263  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.262  2007/12/17 23:47:48  phase1geo
 Adding more profiling information.

 Revision 1.261  2007/12/12 08:04:15  phase1geo
 Adding more timed functions for profiling purposes.

 Revision 1.260  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.259  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.258  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.257  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.256  2007/09/05 21:07:37  phase1geo
 Fixing bug 1788991.  Full regression passes.  Removed excess output used for
 debugging.  May want to create a new development release with these changes.

 Revision 1.255  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.254  2007/08/31 22:46:36  phase1geo
 Adding diagnostics from stable branch.  Fixing a few minor bugs and in progress
 of working on static_afunc1 failure (still not quite there yet).  Checkpointing.

 Revision 1.253  2007/07/31 22:17:44  phase1geo
 Attempting to debug issue with automatic static functions.  Updated regressions
 per last change.

 Revision 1.252  2007/07/31 20:07:06  phase1geo
 Finished work on automatic function support and added new static_afunc1 diagnostic
 to verify static use of automatic functions (this diagnostic is currently not
 passing).

 Revision 1.251  2007/07/31 03:36:10  phase1geo
 Fixing last known issue with automatic functions.  Also fixing issue with
 toggle report output (still a problem with the toggle calculation for the
 return value of the function).

 Revision 1.250  2007/07/30 22:42:02  phase1geo
 Making some progress on automatic function support.  Things currently don't compile
 but I need to checkpoint for now.

 Revision 1.249  2007/07/30 20:36:14  phase1geo
 Fixing rest of issues pertaining to new implementation of function calls.
 Full regression passes (with the exception of afunc1 which we do not expect
 to pass with these changes as of yet).

 Revision 1.248  2007/07/29 03:32:06  phase1geo
 First attempt to make FUNC_CALL expressions copy the functional return value
 to the expression vector.  Not quite working yet -- checkpointing.

 Revision 1.247  2007/07/27 21:57:08  phase1geo
 Adding afunc1 diagnostic to regression suite (though this diagnostic does not
 currently pass).  Checkpointing.

 Revision 1.246  2007/07/27 19:11:27  phase1geo
 Putting in rest of support for automatic functions/tasks.  Checked in
 atask1 diagnostic files.

 Revision 1.245  2007/07/26 17:05:15  phase1geo
 Fixing problem with static functions (vector data associated with expressions
 were not being allocated).  Regressions have been run.  Only two failures
 in total still to be fixed.

 Revision 1.244  2007/07/26 05:03:42  phase1geo
 Starting to work on fix for static function support.  Fixing issue if
 func_call is called with NULL thr parameter (to avoid segmentation fault).
 IV regression fully passes.

 Revision 1.243  2007/04/18 22:34:58  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.242  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.241  2007/04/11 03:15:20  phase1geo
 Attempting to fix delay expression for new simulation core.  Almost there.

 Revision 1.240  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.239  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.238  2007/03/14 22:26:52  phase1geo
 Fixing bug in nb_call operation (need to set eval_t and eval_f in the case
 where we are considering the named block call to not be changed).

 Revision 1.237  2007/03/08 05:17:30  phase1geo
 Various code fixes.  Full regression does not yet pass.

 Revision 1.236  2006/12/22 23:39:08  phase1geo
 Fixing problem with NB_CALL in infinite loop.

 Revision 1.235  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.234  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.233  2006/12/14 23:46:57  phase1geo
 Fixing remaining compile issues with support for functional unit pointers in
 expressions and unnamed scope handling.  Starting to debug run-time issues now.
 Added atask1 diagnostic to begin this verification process.  Checkpointing.

 Revision 1.232  2006/12/12 06:20:22  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.231  2006/11/30 19:58:11  phase1geo
 Fixing rest of issues so that full regression (IV, Cver and VCS) without VPI
 passes.  Updated regression files.

 Revision 1.230  2006/11/29 23:15:46  phase1geo
 Major overhaul to simulation engine by including an appropriate delay queue
 mechanism to handle simulation timing for delay operations.  Regression not
 fully passing at this moment but enough is working to checkpoint this work.

 Revision 1.229  2006/11/28 16:39:46  phase1geo
 More updates for regressions due to changes in delay handling.  Still work
 to go.

 Revision 1.228  2006/11/27 04:11:41  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.227  2006/11/25 04:24:39  phase1geo
 Adding initial code to fully support the timescale directive and its usage.
 Added -vpi_ts score option to allow the user to specify a top-level timescale
 value for the generated VPI file (this code has not been tested at this point,
 however).  Also updated diagnostic Makefile to get the VPI shared object files
 from the current lib directory (instead of the checked in one).

 Revision 1.226  2006/11/24 05:30:15  phase1geo
 Checking in fix for proper handling of delays.  This does not include the use
 of timescales (which will be fixed next).  Full IV regression now passes.

 Revision 1.225  2006/11/22 20:20:01  phase1geo
 Updates to properly support 64-bit time.  Also starting to make changes to
 simulator to support "back simulation" for when the current simulation time
 has advanced out quite a bit of time and the simulator needs to catch up.
 This last feature is not quite working at the moment and regressions are
 currently broken.  Checkpointing.

 Revision 1.224  2006/10/13 22:46:31  phase1geo
 Things are a bit of a mess at this point.  Adding generate12 diagnostic that
 shows a failure in properly handling generates of instances.

 Revision 1.223  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.222  2006/10/07 02:16:22  phase1geo
 Fixing bug in PEDGE and NEDGE expressions to make them completely compliant
 to the Verilog LRM.

 Revision 1.221  2006/10/07 02:06:57  phase1geo
 Fixing bug in report command in that wait events were not being considered "covered"
 when they successfully passed in simulation.  Added wait1.1 diagnostic which found this
 bug.  Also added feature in VPI mode that will only get signals from simulator that it
 needs (rather than all of the signals).  This should improve performance when running
 in this mode.

 Revision 1.220  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.219  2006/10/05 21:43:17  phase1geo
 Added support for increment and decrement operators in expressions.  Also added
 proper parsing and handling support for immediate and postponed increment/decrement.
 Added inc3, inc3.1, dec3 and dec3.1 diagnostics to verify this new functionality.
 Still need to run regressions.

 Revision 1.218  2006/10/04 22:04:16  phase1geo
 Updating rest of regressions.  Modified the way we are setting the memory rd
 vector data bit (should optimize the score command just a bit).  Also updated
 quite a bit of memory coverage documentation though I still need to finish
 documenting how to understand the report file for this metric.  Cleaning up
 other things and fixing a few more software bugs from regressions.  Added
 marray2* diagnostics to verify endianness in the unpacked dimension list.

 Revision 1.217  2006/10/03 22:47:00  phase1geo
 Adding support for read coverage to memories.  Also added memory coverage as
 a report output for DIAGLIST diagnostics in regressions.  Fixed various bugs
 left in code from array changes and updated regressions for these changes.
 At this point, all IV diagnostics pass regressions.

 Revision 1.216  2006/09/23 19:40:53  phase1geo
 Fixing expression_assign, single and multi-bit select functions in expr.c
 to properly handle multi-dimensional arrays in both LHS and RHS expressions.
 Added marray* diagnostics to verify proper behavior this new functionality
 for packed arrays (all endianness combinations).  Still a lot of testing to
 go, but this should be the majority of the code for now.  Full IV regression
 has been verified to pass.

 Revision 1.215  2006/09/22 22:49:15  phase1geo
 Adding multidimensional array diagnostics to regression suite.  At this point,
 they do not all pass.  Also adding fixes to expression_assign() to get things
 to work.

 Revision 1.214  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.213  2006/09/22 04:23:04  phase1geo
 More fixes to support new signal range structure.  Still don't have full
 regressions passing at the moment.

 Revision 1.212  2006/09/21 04:20:59  phase1geo
 Fixing endianness diagnostics.  Still getting memory error with some diagnostics
 in regressions (ovl1 is one of them).  Updated regression.

 Revision 1.211  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.210  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.209  2006/09/13 23:05:56  phase1geo
 Continuing from last submission.

 Revision 1.208  2006/09/12 22:24:42  phase1geo
 Added first attempt at collecting bitwise coverage information during simulation.
 Updated regressions for this change; however, we currently do not report on this
 information currently.  Also added missing keywords to GUI Verilog highlighter.
 Checkpointing.

 Revision 1.207  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.206  2006/09/08 22:39:50  phase1geo
 Fixes for memory problems.

 Revision 1.205  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.204  2006/09/06 22:09:22  phase1geo
 Fixing bug with multiply-and-op operation.  Also fixing bug in gen_item_resolve
 function where an instance was incorrectly being placed into a new instance tree.
 Full regression passes with these changes.  Also removed verbose output.

 Revision 1.203  2006/09/05 21:00:45  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.202  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.201  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.200  2006/08/24 22:25:11  phase1geo
 Fixing issue with generate expressions within signal hierarchies.  Also added
 ability to parse implicit named and * port lists.  Added diagnostics to regressions
 to verify this new ability.  Full regression passes.

 Revision 1.199  2006/08/24 03:39:02  phase1geo
 Fixing some issues with new static_lexer/parser.  Working on debugging issue
 related to the generate variable mysteriously losing its vector data.

 Revision 1.198  2006/08/21 22:50:00  phase1geo
 Adding more support for delayed assignments.  Added dly_assign1 to testsuite
 to verify the #... type of delayed assignment.  This seems to be working for
 this case but has a potential issue in the report generation.  Checkpointing
 work.

 Revision 1.197  2006/08/20 03:21:00  phase1geo
 Adding support for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, <<<=, >>>=, ++
 and -- operators.  The op-and-assign operators are currently good for
 simulation and code generation purposes but still need work in the comb.c
 file for proper combinational logic underline and reporting support.  The
 increment and decrement operations should be fully supported with the exception
 of their use in FOR loops (I'm not sure if this is supported by SystemVerilog
 or not yet).  Also started adding support for delayed assignments; however, I
 need to rework this completely as it currently causes segfaults.  Added lots of
 new diagnostics to verify this new functionality and updated regression for
 these changes.  Full IV regression now passes.

 Revision 1.196  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.195  2006/08/18 04:41:14  phase1geo
 Incorporating bug fixes 1538920 and 1541944.  Updated regressions.  Only
 event1.1 does not currently pass (this does not pass in the stable version
 yet either).

 Revision 1.194  2006/08/11 18:57:04  phase1geo
 Adding support for always_comb, always_latch and always_ff statement block
 types.  Added several diagnostics to regression suite to verify this new
 behavior.

 Revision 1.193  2006/08/10 22:35:14  phase1geo
 Updating with fixes for upcoming 0.4.7 stable release.  Updated regressions
 for this change.  Full regression still fails due to an unrelated issue.

 Revision 1.192  2006/08/02 22:28:32  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.191  2006/07/31 16:26:53  phase1geo
 Tweaking the is_static_only function to consider expressions using generate
 variables to be static.  Updating regression for changes.  Full regression
 now passes.

 Revision 1.190  2006/07/28 22:42:51  phase1geo
 Updates to support expression/signal binding for expressions within a generate
 block statement block.

 Revision 1.189  2006/07/26 06:22:27  phase1geo
 Fixing rest of issues with generate6 diagnostic.  Still need to know if I
 have broken regressions or not and there are plenty of cases in this area
 to test before I call things good.

 Revision 1.188  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.187  2006/07/21 22:39:00  phase1geo
 Started adding support for generated statements.  Still looks like I have
 some loose ends to tie here before I can call it good.  Added generate5
 diagnostic to regression suite -- this does not quite pass at this point, however.

 Revision 1.186  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.185  2006/07/10 03:05:04  phase1geo
 Contains bug fixes for memory leaks and segmentation faults.  Also contains
 some starting code to support generate blocks.  There is absolutely no
 functionality here, however.

 Revision 1.184  2006/07/09 01:40:39  phase1geo
 Removing the vpi directory (again).  Also fixing a bug in Covered's expression
 deallocator where a case statement contains an unbindable signal.  Previously
 the case test expression was being deallocated twice.  This submission fixes
 this bug (bug was also fixed in the 0.4.5 stable release).  Added new tests
 to verify fix.  Full regression passes.

 Revision 1.183  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.182  2006/05/25 12:11:01  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.181  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.180  2006/04/13 21:04:24  phase1geo
 Adding NOOP expression and allowing $display system calls to not cause its
 statement block to be excluded from coverage.  Updating regressions which fully
 pass.

 Revision 1.179.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.179  2006/04/05 04:20:10  phase1geo
 Fixing bug expression_resize function to properly handle the sizing of a
 concatenation on the left-hand-side of an expression.  Added diagnostic to
 regression suite to duplicate this scenario and verify the fix.

 Revision 1.178  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.177  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.176  2006/03/24 19:01:44  phase1geo
 Changed two variable report output to be as concise as possible.  Updating
 regressions per these changes.

 Revision 1.175  2006/03/23 22:42:54  phase1geo
 Changed two variable combinational expressions that have a constant value in either the
 left or right expression tree to unary expressions.  Removed expressions containing only
 static values from coverage totals.  Fixed bug in stmt_iter_get_next_in_order for looping
 cases (the verbose output was not being emitted for these cases).  Updated regressions for
 these changes -- full regression passes.

 Revision 1.174  2006/03/20 16:43:38  phase1geo
 Fixing code generator to properly display expressions based on lines.  Regression
 still needs to be updated for these changes.

 Revision 1.173  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.172  2006/02/10 16:44:28  phase1geo
 Adding support for register assignment.  Added diagnostic to regression suite
 to verify its implementation.  Updated TODO.  Full regression passes at this
 point.

 Revision 1.171  2006/02/06 22:48:34  phase1geo
 Several enhancements to GUI look and feel.  Fixed error in combinational logic
 window.

 Revision 1.170  2006/02/06 05:07:26  phase1geo
 Fixed expression_set_static_only function to consider static expressions
 properly.  Updated regression as a result of this change.  Added files
 for signed3 diagnostic.  Documentation updates for GUI (these are not quite
 complete at this time yet).

 Revision 1.169  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.168  2006/02/02 22:37:41  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.167  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.166  2006/01/25 16:51:27  phase1geo
 Fixing performance/output issue with hierarchical references.  Added support
 for hierarchical references to parser.  Full regression passes.

 Revision 1.165  2006/01/25 04:32:47  phase1geo
 Fixing bug with latest checkins.  Full regression is now passing for IV simulated
 diagnostics.

 Revision 1.164  2006/01/24 23:33:14  phase1geo
 A few cleanups.

 Revision 1.163  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.162  2006/01/23 22:55:10  phase1geo
 Updates to fix constant function support.  There is some issues to resolve
 here but full regression is passing with the exception of the newly added
 static_func1.1 diagnostic.  Fixed problem where expand and multi-bit expressions
 were getting coverage numbers calculated for them before they were simulated.

 Revision 1.161  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.160  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.159  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.158  2006/01/13 04:01:04  phase1geo
 Adding support for exponential operation.  Added exponent1 diagnostic to verify
 but Icarus does not support this currently.

 Revision 1.157  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.156  2006/01/10 05:56:36  phase1geo
 In the middle of adding support for event sensitivity lists to score command.
 Regressions should pass but this code is not complete at this time.

 Revision 1.155  2006/01/10 05:12:48  phase1geo
 Added arithmetic left and right shift operators.  Added ashift1 diagnostic
 to verify their correct operation.

 Revision 1.154  2006/01/09 04:15:25  phase1geo
 Attempting to fix one last problem with latest changes.  Regression runs are
 currently running.  Checkpointing.

 Revision 1.153  2006/01/08 05:51:03  phase1geo
 Added optimizations to EOR and AEDGE expressions.  In the process of running
 regressions...

 Revision 1.152  2006/01/08 03:05:05  phase1geo
 Checkpointing work on optimized thread handling.  I believe that this is now
 working as wanted; however, regressions will not pass until EOR optimization
 has been completed.  I will be working on this next.

 Revision 1.151  2006/01/06 18:54:03  phase1geo
 Breaking up expression_operate function into individual functions for each
 expression operation.  Also storing additional information in a globally accessible,
 constant structure array to increase performance.  Updating full regression for these
 changes.  Full regression passes.

 Revision 1.150  2006/01/04 22:07:04  phase1geo
 Changing expression execution calculation from sim to expression_operate function.
 Updating all regression files for this change.  Modifications to diagnostic Makefile
 to accommodate environments that do not have valgrind.

 Revision 1.149  2006/01/03 22:59:16  phase1geo
 Fixing bug in expression_assign function -- removed recursive assignment when
 the LHS expression is a signal, single-bit, multi-bit or static value (only
 recurse when the LHS is a CONCAT or LIST).  Fixing bug in db_close function to
 check if the instance tree has been populated before deallocating memory for it.
 Fixing bug in report help information when Tcl/Tk is not available.  Added bassign2
 diagnostic to regression suite to verify first described bug.

 Revision 1.148  2005/12/31 05:00:57  phase1geo
 Updating regression due to recent changes in adding exec_num field in expression
 and removing the executed bit in the expression supplemental field.  This will eventually
 allow us to get information on where the simulator is spending the most time.

 Revision 1.147  2005/12/23 05:41:52  phase1geo
 Fixing several bugs in score command per bug report #1388339.  Fixed problem
 with race condition checker statement iterator to eliminate infinite looping (this
 was the problem in the original bug).  Also fixed expression assigment when static
 expressions are used in the LHS (caused an assertion failure).  Also fixed the race
 condition checker to properly pay attention to task calls, named blocks and fork
 statements to make sure that these are being handled correctly for race condition
 checking.  Fixed bug for signals that are on the LHS side of an assignment expression
 but is not being assigned (bit selects) so that these are NOT considered for race
 conditions.  Full regression is a bit broken now but the opened bug can now be closed.

 Revision 1.146  2005/12/17 05:47:36  phase1geo
 More memory fault fixes.  Regression runs cleanly and we have verified
 no memory faults up to define3.v.  Still have a ways to go.

 Revision 1.145  2005/12/16 23:09:15  phase1geo
 More updates to remove memory leaks.  Full regression passes.

 Revision 1.144  2005/12/15 17:24:46  phase1geo
 More fixes for memory fault clean-up.  At this point all of the always
 diagnostics have been run and come up clean with valgrind.  Full regression
 passes.

 Revision 1.143  2005/12/13 23:15:15  phase1geo
 More fixes for memory leaks.  Regression fully passes at this point.

 Revision 1.142  2005/12/10 06:41:18  phase1geo
 Added support for FOR loops and added diagnostics to regression suite to verify
 functionality.  Fixed statement deallocation function (removed a bunch of code
 there now that statement stopping is working as intended).  Full regression passes.

 Revision 1.141  2005/12/08 22:50:59  phase1geo
 Adding support for while loops.  Added while1 and while1.1 to regression suite.
 Ran VCS on regression suite and updated.  Full regression passes.

 Revision 1.140  2005/12/07 21:50:51  phase1geo
 Added support for repeat blocks.  Added repeat1 to regression and fixed errors.
 Full regression passes.

 Revision 1.139  2005/12/05 23:30:35  phase1geo
 Adding support for disabling tasks.  Full regression passes.

 Revision 1.138  2005/12/05 22:02:24  phase1geo
 Added initial support for disable expression.  Added test to verify functionality.
 Full regression passes.

 Revision 1.137  2005/12/05 20:26:55  phase1geo
 Fixing bugs in code to remove statement blocks that are pointed to by expressions
 in NB_CALL and FORK cases.  Fixed bugs in fork code -- this is now working at the
 moment.  Updated regressions which now fully pass.

 Revision 1.136  2005/12/02 19:58:36  phase1geo
 Added initial support for FORK/JOIN expressions.  Code is not working correctly
 yet as we need to determine if a statement should be done in parallel or not.

 Revision 1.135  2005/12/01 20:49:02  phase1geo
 Adding nested_block3 to verify nested named blocks in tasks.  Fixed named block
 usage to be FUNC_CALL or TASK_CALL -like based on its placement.

 Revision 1.134  2005/12/01 18:35:17  phase1geo
 Fixing bug where functions in continuous assignments could cause the
 assignment to constantly be reevaluated (infinite looping).  Added new nested_block2
 diagnostic to verify nested named blocks in functions.  Also verifies that nested
 named blocks can call functions in the same module.  Also modified NB_CALL expressions
 to act like functions (no context switching involved).  Full regression passes.

 Revision 1.133  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.132  2005/11/29 19:04:47  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.131  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.130  2005/11/28 18:31:02  phase1geo
 Fixing memory fault bug in expression deallocation algorithm.

 Revision 1.129  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

 Revision 1.128  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.127  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.126  2005/11/22 16:46:27  phase1geo
 Fixed bug with clearing the assigned bit in the binding phase.  Full regression
 now runs cleanly.

 Revision 1.125  2005/11/22 05:30:33  phase1geo
 Updates to regression suite for clearing the assigned bit when a statement
 block is removed from coverage consideration and it is assigning that signal.
 This is not fully working at this point.

 Revision 1.124  2005/11/21 22:21:58  phase1geo
 More regression updates.  Also made some updates to debugging output.

 Revision 1.123  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.122  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.121  2005/11/18 05:17:01  phase1geo
 Updating regressions with latest round of changes.  Also added bit-fill capability
 to expression_assign function -- still more changes to come.  We need to fix the
 expression sizing problem for RHS expressions of assignment operators.

 Revision 1.120  2005/11/17 23:35:16  phase1geo
 Blocking assignment is now working properly along with support for event expressions
 (currently only the original PEDGE, NEDGE, AEDGE and DELAY are supported but more
 can now follow).  Added new race4 diagnostic to verify that a register cannot be
 assigned from more than one location -- this works.  Regression fails at this point.

 Revision 1.119  2005/11/17 05:34:44  phase1geo
 Initial work on supporting blocking assignments.  Added new diagnostic to
 check that this initial work is working correctly.  Quite a bit more work to
 do here.

 Revision 1.118  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.117  2005/11/11 23:29:12  phase1geo
 Checkpointing some work in progress.  This will cause compile errors.  In
 the process of moving db read expression signal binding from vsignal output to
 expression output so that we can just call the binder in the expression_db_read
 function.

 Revision 1.116  2005/11/10 19:28:22  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.115  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.114  2005/02/16 13:44:55  phase1geo
 Adding value propagation function to vsignal.c and adding this propagation to
 BASSIGN expression assignment after the assignment occurs.

 Revision 1.113  2005/02/11 22:50:33  phase1geo
 Fixing bug with removing statement blocks that contain statements that cannot
 currently be handled by Covered correctly.  There was a problem when the bad statement
 was the last statement in the statement block.  Updated regression accordingly.
 Added race condition diagnostics that currently are not in regression due to lack
 of code support for them.  Ifdef'ed out the BASSIGN stuff for this checkin.

 Revision 1.112  2005/02/09 14:12:22  phase1geo
 More code for supporting expression assignments.

 Revision 1.111  2005/02/08 23:18:23  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.110  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.109  2005/01/10 02:59:30  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.108  2005/01/07 23:00:09  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.107  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.106  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.105  2004/11/06 14:49:43  phase1geo
 Fixing problem in expression_operate.  This removes some more code from the score command
 to improve run-time efficiency.

 Revision 1.104  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.103  2004/10/22 22:03:31  phase1geo
 More incremental changes to increase score command efficiency.

 Revision 1.102  2004/10/22 21:40:30  phase1geo
 More incremental updates to improve efficiency in score command (though this
 change should not, in and of itself, improve efficiency).

 Revision 1.101  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.100  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.99  2004/07/22 04:43:04  phase1geo
 Finishing code to calculate start and end columns of expressions.  Regression
 has been updated for these changes.  Other various minor changes as well.

 Revision 1.98  2004/04/19 13:42:31  phase1geo
 Forgot to modify replace function for column information.

 Revision 1.97  2004/04/19 04:54:55  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.96  2004/04/05 12:30:52  phase1geo
 Adding *db_replace functions to allow a design to be opened with new CDD
 results (for GUI purposes only).

 Revision 1.95  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.94  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.93  2004/01/25 03:41:48  phase1geo
 Fixes bugs in summary information not matching verbose information.  Also fixes
 bugs where instances were output when no logic was missing, where instance
 children were missing but not output.  Changed code to output summary
 information on a per instance basis (where children instances are not merged
 into parent instance summary information).  Updated regressions as a result.
 Updates to user documentation (though this is not complete at this time).

 Revision 1.92  2004/01/08 23:24:41  phase1geo
 Removing unnecessary scope information from signals, expressions and
 statements to reduce file sizes of CDDs and slightly speeds up fscanf
 function calls.  Updated regression for this fix.

 Revision 1.91  2003/11/30 21:50:45  phase1geo
 Modifying line_collect_uncovered function to create array containing all physical
 lines (rather than just uncovered statement starting line values) for more
 accurate line coverage results for the GUI.  Added new long_exp2 diagnostic that
 is used to test this functionality.

 Revision 1.90  2003/11/30 05:46:45  phase1geo
 Adding IF report outputting capability.  Updated always9 diagnostic for these
 changes and updated rest of regression CDD files accordingly.

 Revision 1.89  2003/11/29 06:55:48  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.88  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.87  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.86  2003/10/31 01:38:13  phase1geo
 Adding new expand diagnostics to verify more situations regarding expansion
 operators.  Fixing expression_create to properly handle all situations of
 this operator's use.

 Revision 1.85  2003/10/30 05:05:11  phase1geo
 Partial fix to bug 832730.  This doesn't seem to completely fix the parameter
 case, however.

 Revision 1.84  2003/10/19 04:23:49  phase1geo
 Fixing bug in VCD parser for new Icarus Verilog VCD dumpfile formatting.
 Fixing bug in signal.c for signal merging.  Updates all CDD files to match
 new format.  Added new diagnostics to test advanced FSM state variable
 features.

 Revision 1.83  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.82  2003/10/16 12:27:19  phase1geo
 Fixing bug in arc.c related to non-zero LSBs.

 Revision 1.81  2003/10/14 04:02:44  phase1geo
 Final fixes for new FSM support.  Full regression now passes.  Need to
 add new diagnostics to verify new functionality, but at least all existing
 cases are supported again.

 Revision 1.80  2003/10/11 05:15:08  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.79  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.78  2003/08/10 00:05:16  phase1geo
 Fixing bug with posedge, negedge and anyedge expressions such that these expressions
 must be armed before they are able to be evaluated.  Fixing bug in vector compare function
 to cause compare to occur on smallest vector size (rather than on largest).  Updated regression
 files and added new diagnostics to test event fix.

 Revision 1.77  2003/08/09 22:16:37  phase1geo
 Updates to development documentation for newly added functions from previous
 checkin.

 Revision 1.76  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.75  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.74  2003/02/26 23:00:46  phase1geo
 Fixing bug with single-bit parameter handling (param4.v diagnostic miscompare
 between Linux and Irix OS's).  Updates to testsuite and new diagnostic added
 for additional testing in this area.

 Revision 1.73  2003/02/07 02:28:23  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.72  2002/12/30 05:31:33  phase1geo
 Fixing bug in module merge for reports when parameterized modules are merged.
 These modules should not output an error to the user when mismatching modules
 are found.

 Revision 1.71  2002/12/07 17:46:53  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.70  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.69  2002/12/03 00:04:56  phase1geo
 Fixing bug uncovered by param6.1.v diagnostic.  Full regression now passes.

 Revision 1.68  2002/12/02 06:14:27  phase1geo
 Fixing bug when an MBIT_SEL expression is used in a module that is instantiated
 more than once (assertion error was firing).  Added diagnostic to test suite to
 test that this case passes.

 Revision 1.67  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.66  2002/11/24 14:38:12  phase1geo
 Updating more regression CDD files for bug fixes.  Fixing bugs where combinational
 expressions were counted more than once.  Adding new diagnostics to regression
 suite.

 Revision 1.65  2002/11/23 16:10:46  phase1geo
 Updating changelog and development documentation to include FSM description
 (this is a brainstorm on how to handle FSMs when we get to this point).  Fixed
 bug with code underlining function in handling parameter in reports.  Fixing bugs
 with MBIT/SBIT handling (this is not verified to be completely correct yet).

 Revision 1.64  2002/11/08 00:58:04  phase1geo
 Attempting to fix problem with parameter handling.  Updated some diagnostics
 in test suite.  Other diagnostics to follow.

 Revision 1.63  2002/11/05 22:27:02  phase1geo
 Adding diagnostic to verify usage of parameters in signal sizing expressions.
 Added diagnostic to regression suite.  Fixed bug with sizing of EXPAND
 expressions in expression creation function.

 Revision 1.62  2002/11/05 16:43:55  phase1geo
 Bug fix for expansion expressions where multiplier wasn't being calculated
 before the expand expression was being sized (leads to a segmentation fault).
 Updated CDD file for expand3.v.  Cleaned up missing CDD file for full
 VCS regression run.

 Revision 1.61  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.60  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.59  2002/10/31 23:13:43  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.58  2002/10/31 05:22:36  phase1geo
 Fixing bug with reading in an integer value from the expression line into
 a short integer.  Needed to use the 'h' value in the sscanf function.  Also
 added VCSONLYDIAGS variable to regression Makefile for diagnostics that can
 only run under VCS (not supported by Icarus Verilog).

 Revision 1.57  2002/10/30 06:07:10  phase1geo
 First attempt to handle expression trees/statement trees that contain
 unsupported code.  These are removed completely and not evaluated (because
 we can't guarantee that our results will match the simulator).  Added real1.1.v
 diagnostic that verifies one case of this scenario.  Full regression passes.

 Revision 1.56  2002/10/29 17:25:56  phase1geo
 Fixing segmentation fault in expression resizer for expressions with NULL
 values in right side child expressions.  Also trying fix for log comments.

 Revision 1.55  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.54  2002/10/23 03:39:07  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.53  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.52  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.51  2002/09/29 02:16:51  phase1geo
 Updates to parameter CDD files for changes affecting these.  Added support
 for bit-selecting parameters.  param4.v diagnostic added to verify proper
 support for this bit-selecting.  Full regression still passes.

 Revision 1.50  2002/09/25 05:36:08  phase1geo
 Initial version of parameter support is now in place.  Parameters work on a
 basic level.  param1.v tests this basic functionality and param1.cdd contains
 the correct CDD output from handling parameters in this file.  Yeah!

 Revision 1.49  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.48  2002/09/23 01:37:44  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.47  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.46  2002/08/19 04:34:07  phase1geo
 Fixing bug in database reading code that dealt with merging modules.  Module
 merging is now performed in a more optimal way.  Full regression passes and
 own examples pass as well.

 Revision 1.45  2002/07/20 13:58:01  phase1geo
 Fixing bug in EXP_OP_LAST for changes in binding.  Adding correct line numbering
 to lexer (tested).  Added '+' to report outputting for signals larger than 1 bit.
 Added mbit_sel1.v diagnostic to verify some multi-bit functionality.  Full
 regression passes.

 Revision 1.43  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.42  2002/07/18 02:33:23  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.41  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.40  2002/07/16 00:05:31  phase1geo
 Adding support for replication operator (EXPAND).  All expressional support
 should now be available.  Added diagnostics to test replication operator.
 Rewrote binding code to be more efficient with memory use.

 Revision 1.39  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.38  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.37  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.36  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.35  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.34  2002/07/09 03:24:48  phase1geo
 Various fixes for module instantiantion handling.  This now works.  Also
 modified report output for toggle, line and combinational information.
 Regression passes.

 Revision 1.33  2002/07/08 19:02:11  phase1geo
 Adding -i option to properly handle modules specified for coverage that
 are instantiated within a design without needing to parse parent modules.

 Revision 1.32  2002/07/08 12:35:31  phase1geo
 Added initial support for library searching.  Code seems to be broken at the
 moment.

 Revision 1.31  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.30  2002/07/05 04:12:46  phase1geo
 Correcting case, casex and casez equality calculation to conform to correct
 equality check for each case type.  Verified that case statements work correctly
 at this point.  Added diagnostics to verify case statements.

 Revision 1.28  2002/07/05 00:10:18  phase1geo
 Adding report support for case statements.  Everything outputs fine; however,
 I want to remove CASE, CASEX and CASEZ expressions from being reported since
 it causes redundant and misleading information to be displayed in the verbose
 reports.  New diagnostics to check CASE expressions have been added and pass.

 Revision 1.27  2002/07/04 23:10:12  phase1geo
 Added proper support for case, casex, and casez statements in score command.
 Report command still incorrect for these statement types.

 Revision 1.26  2002/07/03 21:30:53  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.25  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.24  2002/07/03 03:11:01  phase1geo
 Fixing multiplication handling error.

 Revision 1.23  2002/07/03 00:59:14  phase1geo
 Fixing bug with conditional statements and other "deep" expression trees.

 Revision 1.22  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.21  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.20  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.19  2002/06/29 04:51:31  phase1geo
 Various fixes for bugs found in regression testing.

 Revision 1.18  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.17  2002/06/28 00:40:37  phase1geo
 Cleaning up extraneous output from debugging.

 Revision 1.16  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.15  2002/06/26 22:09:17  phase1geo
 Removing unecessary output and updating regression Makefile.

 Revision 1.14  2002/06/26 04:59:50  phase1geo
 Adding initial support for delays.  Support is not yet complete and is
 currently untested.

 Revision 1.13  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.12  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.11  2002/06/23 21:18:21  phase1geo
 Added appropriate statement support in parser.  All parts should be in place
 and ready to start testing.

 Revision 1.10  2002/06/22 21:08:23  phase1geo
 Added simulation engine and tied it to the db.c file.  Simulation engine is
 currently untested and will remain so until the parser is updated correctly
 for statements.  This will be the next step.

 Revision 1.9  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.8  2002/06/21 05:55:05  phase1geo
 Getting some codes ready for writing simulation engine.  We should be set
 now.

 Revision 1.7  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.6  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

