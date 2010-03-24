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
#include <math.h>

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
#include "sys_tasks.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern char         user_msg[USER_MSG_LENGTH];
extern expression** static_exprs;
extern unsigned int static_expr_size;
extern db**         db_list;
extern unsigned int curr_db;
extern bool         debug_mode;
extern int          generate_expr_mode;
extern int          curr_expr_id;
extern bool         flag_use_command_line_debug;
extern bool         cli_debug_mode;
extern int          nba_queue_size;

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
static bool expression_op_func__assign( expression*, thread*, const sim_time* );
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
static bool expression_op_func__dim( expression*, thread*, const sim_time* );
static bool expression_op_func__wait( expression*, thread*, const sim_time* );
static bool expression_op_func__finish( expression*, thread*, const sim_time* );
static bool expression_op_func__stop( expression*, thread*, const sim_time* );
static bool expression_op_func__add_a( expression*, thread*, const sim_time* );
static bool expression_op_func__sub_a( expression*, thread*, const sim_time* );
static bool expression_op_func__multiply_a( expression*, thread*, const sim_time* );
static bool expression_op_func__divide_a( expression*, thread*, const sim_time* );
static bool expression_op_func__mod_a( expression*, thread*, const sim_time* );
static bool expression_op_func__and_a( expression*, thread*, const sim_time* );
static bool expression_op_func__or_a( expression*, thread*, const sim_time* );
static bool expression_op_func__xor_a( expression*, thread*, const sim_time* );
static bool expression_op_func__lshift_a( expression*, thread*, const sim_time* );
static bool expression_op_func__rshift_a( expression*, thread*, const sim_time* );
static bool expression_op_func__arshift_a( expression*, thread*, const sim_time* );
static bool expression_op_func__time( expression*, thread*, const sim_time* );
static bool expression_op_func__random( expression*, thread*, const sim_time* );
static bool expression_op_func__sassign( expression*, thread*, const sim_time* );
static bool expression_op_func__srandom( expression*, thread*, const sim_time* );
static bool expression_op_func__urandom( expression*, thread*, const sim_time* );
static bool expression_op_func__urandom_range( expression*, thread*, const sim_time* );
static bool expression_op_func__realtobits( expression*, thread*, const sim_time* );
static bool expression_op_func__bitstoreal( expression*, thread*, const sim_time* );
static bool expression_op_func__shortrealtobits( expression*, thread*, const sim_time* );
static bool expression_op_func__bitstoshortreal( expression*, thread*, const sim_time* );
static bool expression_op_func__itor( expression*, thread*, const sim_time* );
static bool expression_op_func__rtoi( expression*, thread*, const sim_time* );
static bool expression_op_func__test_plusargs( expression*, thread*, const sim_time* );
static bool expression_op_func__value_plusargs( expression*, thread*, const sim_time* );
static bool expression_op_func__signed( expression*, thread*, const sim_time* );
static bool expression_op_func__unsigned( expression*, thread*, const sim_time* );
static bool expression_op_func__clog2( expression*, thread*, const sim_time* );

#ifndef RUNLIB
static void expression_assign( expression*, expression*, int*, thread*, const sim_time*, bool eval_lhs, bool nb );
#endif /* RUNLIB */

/*!
 Array containing static information about expression operation types.  NOTE:  This structure MUST be
 updated if a new expression is added!  The third argument is an initialization to the exp_info_s structure.
*/
const exp_info exp_op_info[EXP_OP_NUM] = { {"STATIC",         "",                 expression_op_func__null,            {0, 1, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"SIG",            "",                 expression_op_func__sig,             {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"XOR",            "^",                expression_op_func__xor,             {0, 0, OTHER_COMB, 0, 1, 0, 1, 0, 0} },
                                           {"MULTIPLY",       "*",                expression_op_func__multiply,        {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 3} },
                                           {"DIVIDE",         "/",                expression_op_func__divide,          {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 3} },
                                           {"MOD",            "%",                expression_op_func__mod,             {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 0} },
                                           {"ADD",            "+",                expression_op_func__add,             {0, 0, OTHER_COMB, 0, 1, 0, 1, 0, 3} },
                                           {"SUBTRACT",       "-",                expression_op_func__subtract,        {0, 0, OTHER_COMB, 0, 1, 0, 1, 3, 3} },
                                           {"AND",            "&",                expression_op_func__and,             {0, 0, AND_COMB,   0, 1, 0, 1, 0, 0} },
                                           {"OR",             "|",                expression_op_func__or,              {0, 0, OR_COMB,    0, 1, 0, 1, 0, 0} },
                                           {"NAND",           "~&",               expression_op_func__nand,            {0, 0, AND_COMB,   0, 1, 0, 0, 0, 0} },
                                           {"NOR",            "~|",               expression_op_func__nor,             {0, 0, OR_COMB,    0, 1, 0, 0, 0, 0} },
                                           {"NXOR",           "~^",               expression_op_func__nxor,            {0, 0, OTHER_COMB, 0, 1, 0, 0, 0, 0} },
                                           {"LT",             "<",                expression_op_func__lt,              {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"GT",             ">",                expression_op_func__gt,              {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"LSHIFT",         "<<",               expression_op_func__lshift,          {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 0} },
                                           {"RSHIFT",         ">>",               expression_op_func__rshift,          {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 0} },
                                           {"EQ",             "==",               expression_op_func__eq,              {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"CEQ",            "===",              expression_op_func__ceq,             {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"LE",             "<=",               expression_op_func__le,              {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"GE",             ">=",               expression_op_func__ge,              {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"NE",             "!=",               expression_op_func__ne,              {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"CNE",            "!==",              expression_op_func__cne,             {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"LOR",            "||",               expression_op_func__lor,             {0, 0, OR_COMB,    0, 1, 0, 0, 0, 0} },
                                           {"LAND",           "&&",               expression_op_func__land,            {0, 0, AND_COMB,   0, 1, 0, 0, 0, 0} },
                                           {"COND",           "?:",               expression_op_func__cond,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 1} },
                                           {"COND_SEL",       "",                 expression_op_func__cond_sel,        {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 3} },
                                           {"UINV",           "~",                expression_op_func__uinv,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"UAND",           "&",                expression_op_func__uand,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"UNOT",           "!",                expression_op_func__unot,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 1} },
                                           {"UOR",            "|",                expression_op_func__uor,             {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"UXOR",           "^",                expression_op_func__uxor,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"UNAND",          "~&",               expression_op_func__unand,           {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"UNOR",           "~|",               expression_op_func__unor,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"UNXOR",          "~^",               expression_op_func__unxor,           {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"SBIT_SEL",       "[]",               expression_op_func__sbit,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"MBIT_SEL",       "[:]",              expression_op_func__mbit,            {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"EXPAND",         "{{}}",             expression_op_func__expand,          {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"CONCAT",         "{}",               expression_op_func__concat,          {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"PEDGE",          "posedge",          expression_op_func__pedge,           {1, 0, NOT_COMB,   0, 1, 1, 0, 1, 0} },
                                           {"NEDGE",          "negedge",          expression_op_func__nedge,           {1, 0, NOT_COMB,   0, 1, 1, 0, 1, 0} },
                                           {"AEDGE",          "anyedge",          expression_op_func__aedge,           {1, 0, NOT_COMB,   0, 1, 1, 0, 1, 1} },
                                           {"LAST",           "",                 expression_op_func__null,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"EOR",            "or",               expression_op_func__eor,             {1, 0, NOT_COMB,   1, 0, 1, 0, 0, 0} },
                                           {"DELAY",          "#",                expression_op_func__delay,           {1, 0, NOT_COMB,   0, 0, 1, 0, 0, 0} },
                                           {"CASE",           "case",             expression_op_func__case,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"CASEX",          "casex",            expression_op_func__casex,           {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"CASEZ",          "casez",            expression_op_func__casez,           {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"DEFAULT",        "",                 expression_op_func__default,         {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"LIST",           "",                 expression_op_func__list,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"PARAM",          "",                 expression_op_func__sig,             {0, 1, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"PARAM_SBIT",     "[]",               expression_op_func__sbit,            {0, 1, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"PARAM_MBIT",     "[:]",              expression_op_func__mbit,            {0, 1, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"ASSIGN",         "",                 expression_op_func__null,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"DASSIGN",        "",                 expression_op_func__null,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"BASSIGN",        "",                 expression_op_func__assign,          {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"NASSIGN",        "",                 expression_op_func__assign,          {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"IF",             "",                 expression_op_func__null,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"FUNC_CALL",      "",                 expression_op_func__func_call,       {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"TASK_CALL",      "",                 expression_op_func__task_call,       {0, 0, NOT_COMB,   1, 0, 1, 0, 0, 0} },
                                           {"TRIGGER",        "->",               expression_op_func__trigger,         {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"NB_CALL",        "",                 expression_op_func__nb_call,         {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"FORK",           "",                 expression_op_func__fork,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"JOIN",           "",                 expression_op_func__join,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"DISABLE",        "",                 expression_op_func__disable,         {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"REPEAT",         "",                 expression_op_func__repeat,          {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"WHILE",          "",                 expression_op_func__null,            {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"ALSHIFT",        "<<<",              expression_op_func__lshift,          {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 0} },
                                           {"ARSHIFT",        ">>>",              expression_op_func__arshift,         {0, 0, NOT_COMB,   1, 1, 0, 1, 0, 0} },
                                           {"SLIST",          "@*",               expression_op_func__slist,           {1, 0, NOT_COMB,   0, 1, 1, 0, 0, 0} },
                                           {"EXPONENT",       "**",               expression_op_func__exponent,        {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"PASSIGN",        "",                 expression_op_func__passign,         {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"RASSIGN",        "",                 expression_op_func__assign,          {0, 0, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"MBIT_POS",       "[+:]",             expression_op_func__mbit_pos,        {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"MBIT_NEG",       "[-:]",             expression_op_func__mbit_neg,        {0, 0, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"PARAM_MBIT_POS", "[+:]",             expression_op_func__mbit_pos,        {0, 1, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"PARAM_MBIT_NEG", "[-:]",             expression_op_func__mbit_neg,        {0, 1, NOT_COMB,   1, 0, 0, 0, 0, 0} },
                                           {"NEGATE",         "-",                expression_op_func__negate,          {0, 0, NOT_COMB,   1, 1, 0, 0, 2, 1} },
                                           {"NOOP",           "",                 expression_op_func__null,            {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"ALWAYS_COMB",    "always_comb",      expression_op_func__slist,           {1, 0, NOT_COMB,   0, 1, 1, 0, 0, 0} },
                                           {"ALWAYS_LATCH",   "always_latch",     expression_op_func__slist,           {1, 0, NOT_COMB,   0, 1, 1, 0, 0, 0} },
                                           {"IINC",           "++",               expression_op_func__iinc,            {1, 0, NOT_COMB,   0, 0, 0, 0, 2, 2} },
                                           {"PINC",           "++",               expression_op_func__pinc,            {1, 0, NOT_COMB,   0, 0, 0, 0, 2, 2} },
                                           {"IDEC",           "--",               expression_op_func__idec,            {1, 0, NOT_COMB,   0, 0, 0, 0, 5, 2} },
                                           {"PDEC",           "--",               expression_op_func__pdec,            {1, 0, NOT_COMB,   0, 0, 0, 0, 5, 2} },
                                           {"DLY_ASSIGN",     "",                 expression_op_func__dly_assign,      {1, 0, NOT_COMB,   0, 0, 1, 0, 0, 0} },
                                           {"DLY_OP",         "",                 expression_op_func__dly_op,          {1, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"RPT_DLY",        "",                 expression_op_func__repeat_dly,      {1, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"DIM",            "",                 expression_op_func__dim,             {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"WAIT",           "wait",             expression_op_func__wait,            {1, 0, NOT_COMB,   0, 1, 1, 0, 0, 0} },
                                           {"SFINISH",        "$finish",          expression_op_func__finish,          {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SSTOP",          "$stop",            expression_op_func__stop,            {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"ADD_A",          "+=",               expression_op_func__add_a,           {0, 0, OTHER_COMB, 0, 1, 0, 1, 1, 3} },
                                           {"SUB_A",          "-=",               expression_op_func__sub_a,           {0, 0, OTHER_COMB, 0, 1, 0, 1, 4, 3} },
                                           {"MLT_A",          "*=",               expression_op_func__multiply_a,      {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 3} },
                                           {"DIV_A",          "/=",               expression_op_func__divide_a,        {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 3} },
                                           {"MOD_A",          "%=",               expression_op_func__mod_a,           {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 0} },
                                           {"AND_A",          "&=",               expression_op_func__and_a,           {0, 0, AND_COMB,   0, 1, 0, 1, 1, 0} },
                                           {"OR_A",           "|=",               expression_op_func__or_a,            {0, 0, OR_COMB,    0, 1, 0, 1, 1, 0} },
                                           {"XOR_A",          "^=",               expression_op_func__xor_a,           {0, 0, OTHER_COMB, 0, 1, 0, 1, 1, 0} },
                                           {"LSHIFT_A",       "<<=",              expression_op_func__lshift_a,        {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 0} },
                                           {"RSHIFT_A",       ">>=",              expression_op_func__rshift_a,        {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 0} },
                                           {"ALSHIFT_A",      "<<<=",             expression_op_func__lshift_a,        {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 0} },
                                           {"ARSHIFT_A",      ">>>=",             expression_op_func__arshift_a,       {0, 0, NOT_COMB,   1, 1, 0, 1, 1, 0} },
                                           {"FOREVER",        "",                 expression_op_func__null,            {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"STIME",          "$time",            expression_op_func__time,            {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SRANDOM",        "$random",          expression_op_func__random,          {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"PLIST",          "",                 expression_op_func__null,            {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SASSIGN",        "",                 expression_op_func__sassign,         {0, 0, NOT_COMB,   0, 0, 0, 0, 0, 1} },
                                           {"SSRANDOM",       "$srandom",         expression_op_func__srandom,         {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"URANDOM",        "$urandom",         expression_op_func__urandom,         {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"URAND_RANGE",    "$urandom_range",   expression_op_func__urandom_range,   {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SR2B",           "$realtobits",      expression_op_func__realtobits,      {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SB2R",           "$bitstoreal",      expression_op_func__bitstoreal,      {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SSR2B",          "$shortrealtobits", expression_op_func__shortrealtobits, {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SB2SR",          "$bitstoshortreal", expression_op_func__bitstoshortreal, {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SI2R",           "$itor",            expression_op_func__itor,            {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SR2I",           "$rtoi",            expression_op_func__rtoi,            {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"STESTARGS",      "$test$plusargs",   expression_op_func__test_plusargs,   {0, 1, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"SVALARGS",       "$value$plusargs",  expression_op_func__value_plusargs,  {0, 1, NOT_COMB,   1, 1, 0, 0, 0, 0} },
                                           {"SSIGNED",        "$signed",          expression_op_func__signed,          {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SUNSIGNED",      "$unsigned",        expression_op_func__unsigned,        {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
                                           {"SCLOG2",         "$clog2",           expression_op_func__clog2,           {0, 1, NOT_COMB,   0, 0, 0, 0, 0, 0} },
 };


/*!
 Allocates the appropriate amount of memory for the temporary vectors for the
 given expression.  This function should only be called when memory is required
 to be allocated and when ourselves and our children expressions within the same
 tree have been sized.
*/
static void expression_create_tmp_vecs(
  expression* exp,   /*!< Pointer to expression to allocate temporary vectors for */
  int         width  /*!< Number of bits wide the given expression will contain */
) { PROFILE(EXPRESSION_CREATE_TMP_VECS);

  /*
   Only create temporary vectors for expressions that require them and who have not already have had
   temporary vectors created.
  */
  if( (EXPR_TMP_VECS( exp->op ) > 0) && (exp->elem.tvecs == NULL) ) {
 
    switch( exp->value->suppl.part.data_type ) {
      case VDATA_UL :
        {
          ulong    hdata;
          unsigned i;

          /* Calculate the width that we need to allocate */
          switch( exp->op ) {
            case EXP_OP_PEDGE :
            case EXP_OP_NEDGE :  hdata = UL_SET;  width = 1;                         break;
            case EXP_OP_AEDGE :  hdata = UL_SET;  width = exp->right->value->width;  break;
            case EXP_OP_ADD_A :
            case EXP_OP_SUB_A :
            case EXP_OP_MLT_A :
            case EXP_OP_DIV_A :
            case EXP_OP_MOD_A :
            case EXP_OP_AND_A :
            case EXP_OP_OR_A  :
            case EXP_OP_XOR_A :
            case EXP_OP_LS_A  :
            case EXP_OP_RS_A  :
            case EXP_OP_ALS_A :
            case EXP_OP_ARS_A :  hdata = 0x0;  width = exp->left->value->width;   break;
            default           :  hdata = 0x0;                                     break;
          }

          /* Allocate the memory */
          exp->elem.tvecs = (vecblk*)malloc_safe( sizeof( vecblk ) );
          for( i=0; i<EXPR_TMP_VECS( exp->op ); i++ ) {
            vector* vec = vector_create( width, VTYPE_VAL, VDATA_UL, TRUE );
            vector_init_ulong( &(exp->elem.tvecs->vec[i]), vec->value.ul, 0, hdata, TRUE, width, VTYPE_VAL );
            free_safe( vec, sizeof( vector ) );
          }
        }
        break;
      case VDATA_R64 :
        {
          unsigned int i;
          exp->elem.tvecs = (vecblk*)malloc_safe( sizeof( vecblk ) );
          for( i=0; i<EXPR_TMP_VECS( exp->op ); i++ ) {
            vector* vec = vector_create( 64, VTYPE_VAL, VDATA_R64, TRUE );
            vector_init_r64( &(exp->elem.tvecs->vec[i]), vec->value.r64, 0.0, NULL, TRUE, VTYPE_VAL );
            free_safe( vec, sizeof( vector ) );
          }
        }
        break;
      case VDATA_R32 :
        {
          unsigned int i;
          exp->elem.tvecs = (vecblk*)malloc_safe( sizeof( vecblk ) );
          for( i=0; i<EXPR_TMP_VECS( exp->op ); i++ ) {
            vector* vec = vector_create( 32, VTYPE_VAL, VDATA_R32, TRUE );
            vector_init_r32( &(exp->elem.tvecs->vec[i]), vec->value.r32, 0.0, NULL, TRUE, VTYPE_VAL );
            free_safe( vec, sizeof( vector ) );
          }
        }
        break;
      default :  assert( 0 );  break;
    }

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Allocates a non-blocking assignment structure to the given expression and initializes it.
*/
void expression_create_nba(
  expression* expr,     /*!< Pointer to expression to add non-blocking assignment structure to */
  vsignal*    lhs_sig,  /*!< Pointer to left-hand-side signal */
  vector*     rhs_vec   /*!< Pointer to right-hand-side vector */
) { PROFILE(EXPRESSION_CREATE_NBA);

  exp_dim* dim = expr->elem.dim;

  /* Allocate memory */
  nonblock_assign* nba = (nonblock_assign*)malloc_safe( sizeof( nonblock_assign ) );

  /* Initialize the structure */
  nba->lhs_sig         = lhs_sig;
  nba->rhs_vec         = rhs_vec;
  nba->suppl.is_signed = (expr->op == EXP_OP_SIG) ? rhs_vec->suppl.part.is_signed : FALSE;
  nba->suppl.added     = 0;

  /* Now change the elem pointer from a dim to a dim_nba */
  expr->elem.dim_nba      = (dim_and_nba*)malloc_safe( sizeof( dim_and_nba ) );
  expr->elem.dim_nba->dim = dim;
  expr->elem.dim_nba->nba = nba;

  /* Set the nba supplemental bit */
  expr->suppl.part.nba = 1;

  /* Increment the number of nba structures */
  nba_queue_size++;

  PROFILE_END;

}

/*!
 \return Returns a pointer to the non-blocking assignment if this expression is a child of the LHS
         that will be assigned via a non-blocking assignment; otherwise, returns a value of NULL.
*/
expression* expression_is_nba_lhs(
  expression* exp  /*!< Pointer to child expression to check */
) { PROFILE(EXPRESSION_IS_NBA_LHS);

  while( (exp->op != EXP_OP_NASSIGN)                &&
         (ESUPPL_IS_ROOT( exp->suppl ) == 0)        &&
         (exp->parent->expr->op != EXP_OP_SBIT_SEL) &&
         (exp->parent->expr->op != EXP_OP_MBIT_SEL) &&
         (exp->parent->expr->op != EXP_OP_MBIT_POS) &&
         (exp->parent->expr->op != EXP_OP_MBIT_NEG) ) {
    exp = exp->parent->expr;
  }

  PROFILE_END;

  return( (exp->op == EXP_OP_NASSIGN) ? exp : NULL );

}
#endif /* RUNLIB */

/*!
 \throws anonymous Throw

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.
*/
static void expression_create_value(
  expression* exp,    /*!< Pointer to expression to add value to */
  int         width,  /*!< Width of value to create */
  bool        data    /*!< Specifies if uint8 array should be allocated for vector */
) { PROFILE(EXPRESSION_CREATE_VALUE);

  /* If the left or right expressions are storing real numbers, create real number storage for this expression */
  if( ((exp_op_info[exp->op].suppl.real_op & 0x2) && (exp->left->value->suppl.part.data_type  == VDATA_R64)) ||
      ((exp_op_info[exp->op].suppl.real_op & 0x1) && (exp->right->value->suppl.part.data_type == VDATA_R64)) ||
      (exp->value->suppl.part.data_type == VDATA_R64) ) {

    if( (data == TRUE) || ((exp->suppl.part.gen_expr == 1) && (width > 0)) ) {
      vector_init_r64( exp->value, (rv64*)malloc_safe( sizeof( rv64 ) ), 0.0, NULL, TRUE, VTYPE_EXP );
      expression_create_tmp_vecs( exp, 64 );
    } else {
      vector_init_r64( exp->value, NULL, 0.0, NULL, FALSE, VTYPE_EXP );
    }

  /* If the left or right expressions are storing shortreal numbers, create shortreal number storage for this expression */
  } else if( ((exp_op_info[exp->op].suppl.real_op & 0x2) && (exp->left->value->suppl.part.data_type  == VDATA_R32)) ||
             ((exp_op_info[exp->op].suppl.real_op & 0x1) && (exp->right->value->suppl.part.data_type == VDATA_R32)) ||
             (exp->value->suppl.part.data_type == VDATA_R32) ) {

    if( (data == TRUE) || ((exp->suppl.part.gen_expr == 1) && (width > 0)) ) {
      vector_init_r32( exp->value, (rv32*)malloc_safe( sizeof( rv32 ) ), 0.0, NULL, TRUE, VTYPE_EXP );
      expression_create_tmp_vecs( exp, 32 );
    } else {
      vector_init_r32( exp->value, NULL, 0.0, NULL, FALSE, VTYPE_EXP );
    }

  /* Otherwise, create a ulong vector */
  } else {

    if( ((data == TRUE) || (exp->suppl.part.gen_expr == 1)) && (width > 0) ) {

      vector* vec = NULL;

      if( width > MAX_BIT_WIDTH ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Found an expression width (%d) that exceeds the maximum currently allowed by Covered (%d)",
                                    width, MAX_BIT_WIDTH );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

      vec = vector_create( width, VTYPE_EXP, VDATA_UL, TRUE );
      assert( exp->value->value.ul == NULL );
      vector_init_ulong( exp->value, vec->value.ul, 0x0, 0x0, TRUE, width, vec->suppl.part.type );
      free_safe( vec, sizeof( vector ) );

      /* Create the temporary vectors now, if needed */
      expression_create_tmp_vecs( exp, width );

    } else {

     vector_init_ulong( exp->value, NULL, 0x0, 0x0, FALSE, width, VTYPE_EXP );

    }

  }

  PROFILE_END;

}

/*!
 \return Returns pointer to newly created expression.

 \throws anonymous Throw expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create(
  expression*  right,    /*!< Pointer to expression on right */
  expression*  left,     /*!< Pointer to expression on left */
  exp_op_type  op,       /*!< Operation to perform for this expression */
  bool         lhs,      /*!< Specifies this expression is a left-hand-side assignment expression */
  int          id,       /*!< ID for this expression as determined by the parent */
  unsigned int line,     /*!< Line number this expression is on */
  unsigned int ppfline,  /*!< First line number from preprocessed file */
  unsigned int pplline,  /*!< Last line number from preprocessed file */
  unsigned int first,    /*!< First column index of expression */
  unsigned int last,     /*!< Last column index of expression */
  bool         data      /*!< Specifies if we should create a uint8 array for the vector value */
) { PROFILE(EXPRESSION_CREATE);

  expression* new_expr;    /* Pointer to newly created expression */
  int         rwidth = 0;  /* Bit width of expression on right */
  int         lwidth = 0;  /* Bit width of expression on left */

  new_expr = (expression*)malloc_safe( sizeof( expression ) );

  new_expr->suppl.all           = 0;
  new_expr->suppl.part.lhs      = (uint8)lhs & 0x1;
  new_expr->suppl.part.gen_expr = (generate_expr_mode > 0) ? 1 : 0;
  new_expr->suppl.part.root     = 1;
  new_expr->op                  = op;
  new_expr->id                  = id;
  new_expr->ulid                = -1;
  new_expr->line                = line;
  new_expr->ppfline             = ppfline;
  new_expr->pplline             = pplline;
  new_expr->col.part.first      = first;
  new_expr->col.part.last       = last;
  new_expr->exec_num            = 0;
  new_expr->sig                 = NULL;
  new_expr->parent              = (expr_stmt*)malloc_safe( sizeof( expr_stmt ) );
  new_expr->parent->expr        = NULL;
  new_expr->right               = right;
  new_expr->left                = left;
  new_expr->value               = (vector*)malloc_safe( sizeof( vector ) );
  new_expr->suppl.part.owns_vec = 1;
  new_expr->value->value.ul     = NULL;
  new_expr->value->suppl.all    = 0;
  new_expr->table               = NULL;
  new_expr->elem.funit          = NULL;
  new_expr->name                = NULL;

  if( EXPR_OP_HAS_DIM( op ) ) {
    new_expr->elem.dim           = (exp_dim*)malloc_safe( sizeof( exp_dim ) );
    new_expr->elem.dim->curr_lsb = -1;
  }

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

    } else if( (op == EXP_OP_EXPAND) && (rwidth > 0) && (lwidth > 0) && (left->value->value.ul != NULL) ) {

      /*
       If the left-hand expression is a known value, go ahead and create the value here; otherwise,
       hold off because our vector value will be coming.
      */
      if( !vector_is_unknown( left->value ) ) {
        expression_create_value( new_expr, (vector_to_int( left->value ) * rwidth), data );
      } else {
        expression_create_value( new_expr, 1, data );
      }

    /* $time, $realtobits, $bitstoreal, $itor and $rtoi expressions are always 64-bits wide */
    } else if( (op == EXP_OP_STIME) || (op == EXP_OP_SR2B) || (op == EXP_OP_SB2R) || (op == EXP_OP_SI2R) || (op == EXP_OP_SR2I) ) {

      expression_create_value( new_expr, 64, data );

    /* $random, $urandom, $urandom_range, $shortrealtobits and $bitstoshortreal expressions are always 32-bits wide */
    } else if( (op == EXP_OP_SRANDOM) || (op == EXP_OP_SURANDOM) || (op == EXP_OP_SURAND_RANGE) || (op == EXP_OP_SSR2B) || (op == EXP_OP_SB2SR) || (op == EXP_OP_SCLOG2) ) {

      expression_create_value( new_expr, 32, data );

    } else if( (op == EXP_OP_LT)        ||
               (op == EXP_OP_GT)        ||
               (op == EXP_OP_EQ)        ||
               (op == EXP_OP_CEQ)       ||
               (op == EXP_OP_LE)        ||
               (op == EXP_OP_GE)        ||
               (op == EXP_OP_NE)        ||
               (op == EXP_OP_CNE)       ||
               (op == EXP_OP_LOR)       ||
               (op == EXP_OP_LAND)      ||
               (op == EXP_OP_UAND)      ||
               (op == EXP_OP_UNOT)      ||
               (op == EXP_OP_UOR)       ||
               (op == EXP_OP_UXOR)      ||
               (op == EXP_OP_UNAND)     ||
               (op == EXP_OP_UNOR)      ||
               (op == EXP_OP_UNXOR)     ||
               (op == EXP_OP_EOR)       ||
               (op == EXP_OP_NEDGE)     ||
               (op == EXP_OP_PEDGE)     ||
               (op == EXP_OP_AEDGE)     ||
               (op == EXP_OP_CASE)      ||
               (op == EXP_OP_CASEX)     ||
               (op == EXP_OP_CASEZ)     ||
               (op == EXP_OP_DEFAULT)   ||
               (op == EXP_OP_REPEAT)    ||
               (op == EXP_OP_RPT_DLY)   ||
               (op == EXP_OP_WAIT)      ||
               (op == EXP_OP_SFINISH)   ||
               (op == EXP_OP_SSTOP)     ||
               (op == EXP_OP_SSRANDOM)  ||
               (op == EXP_OP_STESTARGS) ||
               (op == EXP_OP_SVALARGS) ) {
  
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
    Throw 0;
  }

/*
  if( (data == FALSE) && (generate_expr_mode == 0) ) {
    assert( new_expr->value->value.ul == NULL );
  }
*/

  PROFILE_END;

  return( new_expr );

}

/*!
 \throws anonymous expression_operate_recursively expression_operate_recursively expression_operate_recursively
 
 Sets the specified expression (if necessary) to the value of the
 specified signal's vector value.
*/
void expression_set_value(
  expression* exp,   /*!< Pointer to expression to set value to */
  vsignal*    sig,   /*!< Pointer to signal containing vector value to set expression to */
  func_unit*  funit  /*!< Pointer to functional unit containing expression */
) { PROFILE(EXPRESSION_SET_VALUE);
  
  assert( exp != NULL );
  assert( exp->value != NULL );
  assert( sig != NULL );
  assert( sig->value != NULL );

  /* Set our vector type to match the signal type */
  exp->value->suppl.part.data_type = sig->value->suppl.part.data_type;

  /* If we are a SIG, PARAM or TRIGGER type, set our value to the signal's value */
  if( (exp->op == EXP_OP_SIG) || (exp->op == EXP_OP_PARAM) || (exp->op == EXP_OP_TRIGGER) ) {

    exp->value->suppl                = sig->value->suppl;
    exp->value->width                = sig->value->width;
    exp->value->value.ul             = sig->value->value.ul;
    exp->value->suppl.part.owns_data = 0;

  /* Otherwise, create our own vector to store the part select */
  } else {

    unsigned int edim      = expression_get_curr_dimension( exp );
    int          exp_width = vsignal_calc_width_for_expr( exp, sig );
    exp_dim*     dim;

    /* Allocate dimensional structure (if needed) and populate it with static information */
    if( exp->elem.dim == NULL ) {
      exp->elem.dim = dim = (exp_dim*)malloc_safe( sizeof( exp_dim ) );
    } else if( exp->suppl.part.nba == 1 ) {
      dim = exp->elem.dim_nba->dim;
    } else {
      dim = exp->elem.dim;
    }
    dim->curr_lsb = -1;
    if( sig->dim[edim].lsb < sig->dim[edim].msb ) {
      dim->dim_lsb = sig->dim[edim].lsb;
      dim->dim_be  = FALSE;
    } else {
      dim->dim_lsb = sig->dim[edim].msb;
      dim->dim_be  = TRUE;
    }
    dim->dim_width = exp_width;
    dim->last      = expression_is_last_select( exp );

    /* Set the expression width */
    switch( exp->op ) {
      case EXP_OP_SBIT_SEL   :
      case EXP_OP_PARAM_SBIT :
        break;
      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
        {
          int lbit, rbit;
          expression_operate_recursively( exp->left,  funit, TRUE );
          expression_operate_recursively( exp->right, funit, TRUE );
          lbit = vector_to_int( exp->left->value  );
          rbit = vector_to_int( exp->right->value );
          if( lbit <= rbit ) {
            exp_width = ((rbit - lbit) + 1) * exp_width;
          } else {
            exp_width = ((lbit - rbit) + 1) * exp_width;
          }
        }
        break;
      case EXP_OP_MBIT_POS :
      case EXP_OP_MBIT_NEG :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        expression_operate_recursively( exp->right, funit, TRUE );
        exp_width = vector_to_int( exp->right->value ) * exp_width;
        break;
      default :  break;
    }

    /* Allocate a vector for this expression */
    if( exp->value->value.ul != NULL ) {
      vector_dealloc_value( exp->value );
    }
    expression_create_value( exp, exp_width, TRUE );

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Recursively sets the signed bit for all parent expressions if both the left and
 right expressions have the signed bit set.  This function is called by the bind()
 function after an expression has been set to a signal.
*/
void expression_set_signed(
  expression* exp  /*!< Pointer to current expression */
) { PROFILE(EXPRESSION_SET_SIGNED);

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
          (exp->op == EXP_OP_STATIC))) ||
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
 \throws anonymous expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value
         expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value expression_create_value
         expression_create_value expression_create_value funit_size_elements expression_resize expression_resize expression_operate_recursively expression_set_value

 Resizes the given expression depending on the expression operation and its
 children's sizes.  If recursive is TRUE, performs the resize in a depth-first
 fashion, resizing the children before resizing the current expression.  If
 recursive is FALSE, only the given expression is evaluated and resized.
*/
void expression_resize(
  expression* expr,       /*!< Pointer to expression to potentially resize */
  func_unit*  funit,      /*!< Pointer to functional unit containing expression */
  bool        recursive,  /*!< Specifies if we should perform a recursive depth-first resize */
  bool        alloc       /*!< If set to TRUE, allocates vector data for all expressions */
) { PROFILE(EXPRESSION_RESIZE);

  unsigned int largest_width;  /* Holds larger width of left and right children */
  uint8        old_vec_suppl;  /* Holds original vector supplemental field as this will be erased */
  funit_inst*  tmp_inst;       /* Pointer to temporary instance */
  int          ignore = 0;     /* Specifies the number of instances to ignore */

  if( expr != NULL ) {

    uint8 new_owns_data;
    uint8 new_data_type;

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
          assert( expr->value->value.ul != NULL );
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
      case EXP_OP_STIME          :
      case EXP_OP_SRANDOM        :
      case EXP_OP_SURANDOM       :
      case EXP_OP_SURAND_RANGE   :
        break;

      /* These operations should always be set to a width 1 */
      case EXP_OP_LT        :
      case EXP_OP_GT        :
      case EXP_OP_EQ        :
      case EXP_OP_CEQ       :
      case EXP_OP_LE        :
      case EXP_OP_GE        :
      case EXP_OP_NE        :
      case EXP_OP_CNE       :
      case EXP_OP_LOR       :
      case EXP_OP_LAND      :
      case EXP_OP_UAND      :
      case EXP_OP_UNOT      :
      case EXP_OP_UOR       :
      case EXP_OP_UXOR      :
      case EXP_OP_UNAND     :
      case EXP_OP_UNOR      :
      case EXP_OP_UNXOR     :
      case EXP_OP_EOR       :
      case EXP_OP_CASE      :
      case EXP_OP_CASEX     :
      case EXP_OP_CASEZ     :
      case EXP_OP_DEFAULT   :
      case EXP_OP_REPEAT    :
      case EXP_OP_RPT_DLY   :
      case EXP_OP_WAIT      :
      case EXP_OP_SFINISH   :
      case EXP_OP_SSTOP     :
      case EXP_OP_NEDGE     :
      case EXP_OP_PEDGE     :
      case EXP_OP_AEDGE     :
      case EXP_OP_PLIST     :
      case EXP_OP_SSRANDOM  :
      case EXP_OP_STESTARGS :
      case EXP_OP_SVALARGS  : 
        if( (expr->value->width != 1) || (expr->value->value.ul == NULL) ) {
          assert( expr->value->value.ul == NULL );
          expression_create_value( expr, 1, alloc );
        }
        break;

      /* These operations should always be set to a width of 32 */
      case EXP_OP_SCLOG2 :
        if( (expr->value->width != 32) || (expr->value->value.ul == NULL) ) {
          assert( expr->value->value.ul == NULL );
          expression_create_value( expr, 32, alloc );
        }
        break;

      /*
       In the case of an EXPAND, we need to set the width to be the product of the value of
       the left child and the bit-width of the right child.
      */
      case EXP_OP_EXPAND :
        expression_operate_recursively( expr->left, funit, TRUE );
        if( vector_is_unknown( expr->left->value ) ) {
          unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Unknown value used for concatenation multiplier, file: %s, line: %u", funit->orig_fname, expr->line );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, FATAL, __FILE__, __LINE__ );
          Throw 0;
        }
        if( (expr->value->width != (vector_to_int( expr->left->value ) * expr->right->value->width)) ||
            (expr->value->value.ul == NULL) ) {
          assert( expr->value->value.ul == NULL );
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
            (expr->value->value.ul == NULL) ) {
          assert( expr->value->value.ul == NULL );
          expression_create_value( expr, (expr->left->value->width + expr->right->value->width), alloc );
        }
        break;

      /*
       A FUNC_CALL expression width is set to the same width as that of the function's return value.
      */
      case EXP_OP_FUNC_CALL :
        if( expr->sig != NULL ) {
          assert( funit != NULL );
          if( (funit->suppl.part.type != FUNIT_AFUNCTION) && (funit->suppl.part.type != FUNIT_ANAMED_BLOCK) ) {
            assert( expr->elem.funit != NULL );
            tmp_inst = inst_link_find_by_funit( expr->elem.funit, db_list[curr_db]->inst_head, &ignore );
            funit_size_elements( expr->elem.funit, tmp_inst, FALSE, FALSE );
          }
          if( (expr->value->width != expr->sig->value->width) || (expr->value->value.ul == NULL) ) {
            assert( expr->value->value.ul == NULL );
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
          if( (expr->value->width != largest_width) || (expr->value->value.ul == NULL) ) {
            assert( expr->value->value.ul == NULL );
            expression_create_value( expr, largest_width, alloc );
          }

        /* If our parent is a DLY_OP, we need to get our value from the LHS of the DLY_ASSIGN expression */
        } else if( expr->parent->expr->op == EXP_OP_DLY_OP ) {
          if( (expr->parent->expr->parent->expr->left->value->width != expr->value->width) || (expr->value->value.ul == NULL) ) {
            assert( expr->value->value.ul == NULL );
            expression_create_value( expr, expr->parent->expr->parent->expr->left->value->width, alloc );
          }

        /* Otherwise, get our value from the size of the expression on the left-hand-side of the assignment */
        } else {
          if( (expr->parent->expr->left->value->width != expr->value->width) || (expr->value->value.ul == NULL) ) {
            assert( expr->value->value.ul == NULL );
            expression_create_value( expr, expr->parent->expr->left->value->width, alloc );
          }
        }
        break;

    }

    /* Reapply original supplemental field (preserving the owns_data bit) now that expression has been resized */
    new_owns_data                     = expr->value->suppl.part.owns_data;
    new_data_type                     = expr->value->suppl.part.data_type;
    expr->value->suppl.all            = old_vec_suppl;
    expr->value->suppl.part.owns_data = new_owns_data;
    expr->value->suppl.part.data_type = new_data_type;

  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \return Returns expression ID for this expression.

 If specified expression is non-NULL, return expression ID of this
 expression; otherwise, return a value of 0 to indicate that this
 is a leaf node.
*/
int expression_get_id(
  expression* expr,       /*!< Pointer to expression to get ID from */
  bool        parse_mode  /*!< Specifies if ulid (TRUE) or id (FALSE) should be used */
) { PROFILE(EXPRESSION_GET_ID);

  int id;

  if( expr == NULL ) {
    id = 0;
  } else {
    id = parse_mode ? expr->ulid : expr->id;
  }

  PROFILE_END;

  return( id );

}

#ifndef RUNLIB
/*!
 \return Returns the line number of the first line in this expression.
*/
expression* expression_get_first_line_expr(
  expression* expr  /*!< Pointer to root expression to extract first line from */
) { PROFILE(EXPRESSION_GET_FIRST_LINE_EXPR);

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
 \return Returns the line number of the last line in this expression. 
*/
expression* expression_get_last_line_expr(
  expression* expr  /*!< Pointer to root expression to extract last line from */
) { PROFILE(EXPRESSION_GET_LAST_LINE_EXPR);

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
#endif /* RUNLIB */

/*!
 \return Returns the dimension index for the given expression

 Recursively iterates up expression tree, counting the number of dimensions deep that
 the given expression is.
*/
unsigned int expression_get_curr_dimension(
  expression* expr  /*!< Pointer to expression to get dimension for */
) { PROFILE(EXPRESSION_GET_CURR_DIMENSION);
  
  unsigned int dim;  /* Return value for this function */

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

#ifndef RUNLIB
/*!
 Recursively parses specified expression list in search of RHS signals.
 When a signal name is found, it is added to the signal name list specified
 by head and tail.
*/
void expression_find_rhs_sigs(
            expression* expr,  /*!< Pointer to expression tree to parse */
  /*@out@*/ str_link**  head,  /*!< Pointer to head of signal name list to populate */
  /*@out@*/ str_link**  tail   /*!< Pointer to tail of signal name list to populate */
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
 Recursively traverses given expression tree, adding any expressions found that point to parameter
 values.
*/
static void expression_find_params(
            expression*   expr,     /*!< Pointer to expression tree to search */
  /*@out@*/ expression*** exps,     /*!< Pointer to expression array containing expressions that use parameter values */
  /*@out@*/ unsigned int* exp_size  /*!< Pointer to number of elements in the exps array */
) { PROFILE(EXPRESSION_FIND_PARAMS);

  if( expr != NULL ) {

    /* If we call a parameter, add ourselves to the expression list */
    if( (expr->op == EXP_OP_PARAM)          ||
        (expr->op == EXP_OP_PARAM_SBIT)     ||
        (expr->op == EXP_OP_PARAM_MBIT)     ||
        (expr->op == EXP_OP_PARAM_MBIT_POS) ||
        (expr->op == EXP_OP_PARAM_MBIT_NEG) ) {
      exp_link_add( expr, exps, exp_size );
    }

    /* Search children */
    expression_find_params( expr->left,  exps, exp_size );
    expression_find_params( expr->right, exps, exp_size );

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the found expression; otherwise, returns NULL if the expression
         could not be found.

 Recursively searches the given expression tree for the specified underline ID.  If the
 expression is found, a pointer to it is returned; otherwise, returns NULL.
*/
expression* expression_find_uline_id(
  expression* expr,  /*!< Pointer to root expression to search under */
  int         ulid   /*!< Underline ID to search for */
) { PROFILE(EXPRESSION_FIND_ULINE_ID);

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
 \return Returns TRUE if the given expression exists within the given expression tree; otherwise,
         returns FALSE
*/
bool expression_find_expr(
  expression* root,  /*!< Pointer to root of expression tree to search */
  expression* expr   /*!< Pointer to expression to search for */
) { PROFILE(EXPRESSION_FIND_EXPR);

  bool retval = (root != NULL) && ((root == expr) || expression_find_expr( root->left, expr ) || expression_find_expr( root->right, expr ));

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the given expression tree contains an expression that calls the given statement.
*/
bool expression_contains_expr_calling_stmt(
  expression* expr,  /*!< Pointer to root of expression tree to search */
  statement*  stmt   /*!< Pointer to statement to search for */
) { PROFILE(EXPRESSION_CONTAINS_EXPR_CALLING_STMT);

  bool retval = (expr != NULL) &&
                (((ESUPPL_TYPE( expr->suppl ) == ETYPE_FUNIT) && (expr->elem.funit->first_stmt == stmt)) ||
                 expression_contains_expr_calling_stmt( expr->left, stmt ) ||
                 expression_contains_expr_calling_stmt( expr->right, stmt ));

  PROFILE_END;

  return( retval );

}
#endif /* RUNLIB */

/*!
 \return Returns a pointer to the root statement of the specified expression if one exists;
         otherwise, returns NULL.

 Recursively traverses up expression tree that contains exp until the root expression is found (if
 one exists).  If the root expression is found, return the pointer to the statement pointing to this
 root expression.  If the root expression was not found, return NULL.
*/
statement* expression_get_root_statement(
  expression* exp  /*!< Pointer to expression to get root statement for */
) { PROFILE(EXPRESSION_GET_ROOT_STATEMENT);

  statement* stmt;  /* Pointer to root statement */

  if( exp == NULL ) {
    stmt = NULL;
  } else if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
    stmt = exp->parent->stmt;
  } else {
    stmt = expression_get_root_statement( exp->parent->expr );
  }

  PROFILE_END;

  return( stmt );

}

#ifndef RUNLIB
/*!
 \throws anonymous expression_resize expression_assign_expr_ids expression_assign_expr_ids

 Recursively iterates down the specified expression tree assigning unique IDs to each expression.
*/
void expression_assign_expr_ids(
  expression* root,  /*!< Pointer to root of the expression tree to assign unique IDs for */
  func_unit*  funit  /*!< Pointer to functional unit containing this expression tree */
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
#endif /* RUNLIB */

/*!
 This function recursively displays the expression information for the specified
 expression tree to the coverage database specified by file.
*/
void expression_db_write(
  expression* expr,        /*!< Pointer to expression to write to database file */
  FILE*       file,        /*!< Pointer to database file to write to */
  bool        parse_mode,  /*!< Set to TRUE when we are writing after just parsing the design (causes ulid value to be
                                output instead of id) */
  bool        ids_issued   /*!< Set to TRUE if IDs were issued prior to calling this function */
) { PROFILE(EXPRESSION_DB_WRITE);

  assert( expr != NULL );

  fprintf( file, "%d %d %u %u %u %x %x %x %x %d %d",
    DB_TYPE_EXPRESSION,
    expression_get_id( expr, ids_issued ),
    expr->line,
    expr->ppfline,
    expr->pplline,
    expr->col.all,
    ((((expr->op == EXP_OP_DASSIGN) || (expr->op == EXP_OP_ASSIGN)) && (expr->exec_num == 0)) ? (uint32)1 : expr->exec_num),
    expr->op,
    (expr->suppl.all & ESUPPL_MERGE_MASK),
    ((expr->op == EXP_OP_STATIC) ? 0 : expression_get_id( expr->right, ids_issued )),
    ((expr->op == EXP_OP_STATIC) ? 0 : expression_get_id( expr->left,  ids_issued ))
  );

  if( ESUPPL_OWNS_VEC( expr->suppl ) ) {
    fprintf( file, " " );
    if( parse_mode && EXPR_OWNS_VEC( expr->op ) && (expr->value->suppl.part.owns_data == 0) && (expr->value->width > 0) ) {
      expr->value->suppl.part.owns_data = 1;
    }
    vector_db_write( expr->value, file, (expr->op == EXP_OP_STATIC), FALSE );
  }

  if( expr->name != NULL ) {
    fprintf( file, " %s", expr->name );
  } else if( expr->sig != NULL ) {
    fprintf( file, " %s", expr->sig->name );  /* This will be valid for parameters */
  }

  fprintf( file, "\n" );

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Recursively iterates through the specified expression tree, outputting the expressions
 to the specified file.
*/
void expression_db_write_tree(
  expression* root,  /*!< Pointer to the root expression to display */
  FILE*       ofile  /*!< Output file to write expression tree to */
) { PROFILE(EXPRESSION_DB_WRITE_TREE);

  if( root != NULL ) {

    /* Print children first */
    if( EXPR_LEFT_DEALLOCABLE( root ) ) {
      expression_db_write_tree( root->left, ofile );
    }
    expression_db_write_tree( root->right, ofile );

    /* Now write ourselves */
    expression_db_write( root, ofile, TRUE, TRUE );

  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \throws anonymous expression_create Throw Throw Throw Throw Throw vector_db_read

 Reads in the specified expression information, creates new expression from
 heap, populates the expression with specified information from file and 
 returns that value in the specified expression pointer.  If all is 
 successful, returns TRUE; otherwise, returns FALSE.
*/
void expression_db_read(
  char**     line,        /*!< String containing database line to read information from */
  func_unit* curr_funit,  /*!< Pointer to current functional unit that instantiates this expression */
  bool       eval         /*!< If TRUE, evaluate expression if children are static */
) { PROFILE(EXPRESSION_DB_READ);

  expression*  expr;        /* Pointer to newly created expression */
  unsigned int linenum;     /* Holder of current line for this expression */
  unsigned int ppfline;
  unsigned int pplline;
  unsigned int column;      /* Holder of column alignment information */
  uint32       exec_num;    /* Holder of expression's execution number */
  uint32       op;          /* Holder of expression operation */
  esuppl       suppl;       /* Holder of supplemental value of this expression */
  int          right_id;    /* Holder of expression ID to the right */
  int          left_id;     /* Holder of expression ID to the left */
  expression*  right;       /* Pointer to current expression's right expression */
  expression*  left;        /* Pointer to current expression's left expression */
  int          chars_read;  /* Number of characters scanned in from line */
  vector*      vec;         /* Holders vector value of this expression */

  if( sscanf( *line, "%d %u %u %u %x %x %x %x %d %d%n", &curr_expr_id, &linenum, &ppfline, &pplline, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 10 ) {

    *line = *line + chars_read;

    /* Find functional unit instance name */
    if( curr_funit == NULL ) {

      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  expression (%d) in database written before its functional unit", curr_expr_id );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;

    } else {

      /* Find right expression */
      if( right_id == 0 ) {
        right = NULL;
      } else if( (right = exp_link_find( right_id, curr_funit->exps, curr_funit->exp_size )) == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", curr_expr_id, right_id );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

      /* Find left expression */
      if( left_id == 0 ) {
        left = NULL;
      } else if( (left = exp_link_find( left_id, curr_funit->exps, curr_funit->exp_size )) == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", curr_expr_id, left_id );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        Throw 0;
      }

      /* Create new expression */
      expr = expression_create( right, left, op, ESUPPL_IS_LHS( suppl ), curr_expr_id, linenum, ppfline, pplline,
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
          expression_dealloc( expr, TRUE );
          Throw 0;
        }

        /* Copy expression value */
        vector_dealloc( expr->value );
        expr->value = vec;

      }

      /* Create temporary vectors if necessary */
      expression_create_tmp_vecs( expr, expr->value->width );

      /* Check to see if we are bound to a signal or functional unit */
      if( ((*line)[0] != '\n') && ((*line)[0] != '\0') ) {
        (*line)++;   /* Remove space */
        switch( op ) {
          case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    *line, expr, curr_funit, FALSE );  break;
          case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        *line, expr, curr_funit, FALSE );  break;
          case EXP_OP_FORK      :
          case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, *line, expr, curr_funit, FALSE );  break;
          case EXP_OP_DISABLE   :  bind_add( 1,                 *line, expr, curr_funit, FALSE );  break;
          default               :  bind_add( 0,                 *line, expr, curr_funit, FALSE );  break;
        }
      }

      /* If we are an assignment operator, set our vector value to that of the right child */
      if( (op == EXP_OP_ASSIGN)     ||
          (op == EXP_OP_DASSIGN)    ||
          (op == EXP_OP_BASSIGN)    ||
          (op == EXP_OP_RASSIGN)    ||
          (op == EXP_OP_NASSIGN)    ||
          (op == EXP_OP_DLY_ASSIGN) ||
          (op == EXP_OP_IF)         ||
          (op == EXP_OP_WHILE)      ||
          (op == EXP_OP_DIM) ) {

        vector_dealloc( expr->value );
        expr->value = right->value;

      }

      exp_link_add( expr, &(curr_funit->exps), &(curr_funit->exp_size) );

#ifndef RUNLIB
      /*
       If this expression is a constant expression, force the simulator to evaluate
       this expression and all parent expressions of it.
      */
      if( eval && EXPR_IS_STATIC( expr ) && (ESUPPL_IS_LHS( suppl ) == 0) ) {
        exp_link_add( expr, &static_exprs, &static_expr_size );
      }
#endif /* RUNLIB */
      
    }

  } else {

    print_output( "Unable to read expression value", FATAL, __FILE__, __LINE__ );
    Throw 0;

  }

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 \throws anonymous Throw vector_db_merge

 Parses specified line for expression information and merges contents into the
 base expression.  If the two expressions given are not the same (IDs, op,
 and/or line position differ) we know that the database files being merged 
 were not created from the same design; therefore, display an error message 
 to the user in this case.  If both expressions are the same, perform the 
 merge.
*/
void expression_db_merge(
  expression* base,  /*!< Expression to merge data into */
  char**      line,  /*!< Pointer to CDD line to parse */
  bool        same   /*!< Specifies if expression to be merged needs to be exactly the same as the existing expression */
) { PROFILE(EXPRESSION_DB_MERGE);

  int          id;             /* Expression ID field */
  unsigned int linenum;        /* Expression line number */
  unsigned int ppfline;
  unsigned int pplline;
  unsigned int column;         /* Column information */
  uint32       exec_num;       /* Execution number */
  uint32       op;             /* Expression operation */
  esuppl       suppl;          /* Supplemental field */
  int          right_id;       /* ID of right child */
  int          left_id;        /* ID of left child */
  int          chars_read;     /* Number of characters read */

  assert( base != NULL );

  if( sscanf( *line, "%d %u %u %u %x %x %x %x %d %d%n", &id, &linenum, &ppfline, &pplline, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 10 ) {

    *line = *line + chars_read;

    if( (base->op != op) || (base->line != linenum) || (base->ppfline != ppfline) || (base->pplline != pplline) || (base->col.all != column) ) {

      print_output( "Attempting to merge databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
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
    Throw 0;

  }

  PROFILE_END;

}

/*!
 Performs an expression merge of two expressions, storing the result into the base expression.  This
 function is used by the GUI for calculating module coverage.
*/
void expression_merge(
  expression* base,  /*!< Base expression that will contain merged results */
  expression* other  /*!< Other expression that will merge its results with the base results */
) { PROFILE(EXPRESSION_MERGE);

  assert( base != NULL );
  assert( base->op      == other->op );
  assert( base->line    == other->line );
  assert( base->col.all == other->col.all );

  /* Merge expression supplemental fields */
  base->suppl.all = (base->suppl.all & ESUPPL_MERGE_MASK) | (other->suppl.all & ESUPPL_MERGE_MASK);

  /* Merge execution number information */
  if( base->exec_num < other->exec_num ) {
    base->exec_num = other->exec_num;
  }

  if( ESUPPL_OWNS_VEC( base->suppl ) ) {
    vector_merge( base->value, other->value );
  }

  PROFILE_END;

}
#endif /* RUNLIB */

/*!
 \return Returns a non-writable string that contains the user-readable name of the
         specified expression operation.
*/
const char* expression_string_op(
  int op  /*!< Expression operation to get string representation of */
) { PROFILE(EXPRESSION_STRING_OP);

  assert( (op >= 0) && (op < EXP_OP_NUM) );

  PROFILE_END;

  return( exp_op_info[op].name );

}

/*!
 Returns a pointer to user_msg that will contain a user-friendly string version of
 the given expression
*/
char* expression_string(
  expression* exp  /*!< Pointer to expression to display */
) { PROFILE(EXPRESSION_STRING);

  unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "%d (%s line %u, %u)", exp->id, expression_string_op( exp->op ), exp->line, exp->ppfline );
  assert( rv < USER_MSG_LENGTH );

  PROFILE_END;

  return( user_msg );

}

#ifndef RUNLIB
/*!
 Displays contents of the specified expression to standard output.  This function
 is called by the funit_display function.
*/
void expression_display(
  expression* expr  /*!< Pointer to expression to display */
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

  printf( "  Expression (%p) =>  id: %d, op: %s, line: %u, ppfline: %u, pplline: %u, col: %x, suppl: %x, exec_num: %u, left: %d, right: %d, ", 
          expr,
          expr->id,
          expression_string_op( expr->op ),
          expr->line,
          expr->ppfline,
          expr->pplline,
	  expr->col.all,
          expr->suppl.all,
          expr->exec_num,
          left_id, 
          right_id );

  if( expr->value->value.ul == NULL ) {
    printf( "NO DATA VECTOR" );
  } else {
    switch( expr->value->suppl.part.data_type ) {
      case VDATA_UL  :  vector_display_value_ulong( expr->value->value.ul, expr->value->width );  break;
      case VDATA_R64 :
        if( expr->value->value.r64->str != NULL ) {
          printf( "%s", expr->value->value.r64->str );
        } else {
          printf( "%.16lf", expr->value->value.r64->val );
        }
        break;
      case VDATA_R32 :
        if( expr->value->value.r32->str != NULL ) {
          printf( "%s", expr->value->value.r32->str );
        } else {
          printf( "%.16f", expr->value->value.r32->val );
        }
        break;
      default :  assert( 0 );  break;
    }
  }
  printf( "\n" );

}
#endif /* RUNLIB */

/*!
 This function sets the true/false indicators in the expression supplemental field as
 needed.  This function should only be called by expression_op_func__* functions that
 are NOT events AND whose return value is TRUE AND whose op type is not STATIC or
 PARAM.
*/
inline static void expression_set_tf_preclear(
  expression* expr,    /*!< Pointer to expression to set true/false indicators of */
  bool        changed  /*!< Set to TRUE if the expression changed value */
) {

  /* If the expression changed value or it has never been set, calculate coverage information */
  if( changed || (expr->value->suppl.part.set == 0) ) {

    /* Clear current TRUE/FALSE indicators */
    expr->suppl.part.eval_t = 0;
    expr->suppl.part.eval_f = 0;
      
    /* Set TRUE/FALSE bits to indicate value */
    if( !vector_is_unknown( expr->value ) ) {
      if( vector_is_not_zero( expr->value ) ) {
        expr->suppl.part.true   = 1;
        expr->suppl.part.eval_t = 1;
      } else {
        expr->suppl.part.false  = 1;
        expr->suppl.part.eval_f = 1;
      }
    }

    /* Indicate that the vector has been evaluated */
    expr->value->suppl.part.set = 1;

  }

}

/*!
 This function sets the true/false indicators in the expression supplemental field as
 needed.  This function should only be called by expression_op_func__* functions whose
 return value is TRUE AND whose op type is either STATIC or PARAM.
*/
inline static void expression_set_tf(
  expression* expr,    /*!< Pointer to expression to set true/false indicators of */
  bool        changed  /*!< Set to TRUE if the expression changed value */
) {

  /* If the expression changed value or it has never been set, calculate coverage information */
  if( changed || (expr->value->suppl.part.set == 0) ) {

    /* Set TRUE/FALSE bits to indicate value */
    if( !vector_is_unknown( expr->value ) ) {
      if( vector_is_not_zero( expr->value ) ) {
        expr->suppl.part.true   = 1; 
        expr->suppl.part.eval_t = 1;
      } else {
        expr->suppl.part.false  = 1;
        expr->suppl.part.eval_f = 1;
      }
    }

    /* Indicate that the vector has been evaluated */
    expr->value->suppl.part.set = 1;

  }

}

/*!
 Sets the eval_0x/x0/11 supplemental bits as necessary.  This function should be
 called by expression_op_func__* functions that are NOT events and have both the left
 and right expression children present.
*/
inline static void expression_set_and_eval_NN(
  expression* expr  /*!< Pointer to expression to set 00/01/10/11 supplemental bits */
) {

  uint32 lf = ESUPPL_IS_FALSE( expr->left->suppl  );
  uint32 lt = ESUPPL_IS_TRUE(  expr->left->suppl  );
  uint32 rf = ESUPPL_IS_FALSE( expr->right->suppl );
  uint32 rt = ESUPPL_IS_TRUE(  expr->right->suppl );

  expr->suppl.part.eval_01 |= lf;
  expr->suppl.part.eval_10 |= rf;
  expr->suppl.part.eval_11 |= lt & rt;

}

/*!
 Sets the eval_00/x1/1x supplemental bits as necessary.  This function should be
 called by expression_op_func__* functions that are NOT events and have both the left
 and right expression children present.
*/
inline static void expression_set_or_eval_NN(
  expression* expr  /*!< Pointer to expression to set 00/01/10/11 supplemental bits */
) {

  uint32 lf = ESUPPL_IS_FALSE( expr->left->suppl  );
  uint32 lt = ESUPPL_IS_TRUE(  expr->left->suppl  );
  uint32 rf = ESUPPL_IS_FALSE( expr->right->suppl );
  uint32 rt = ESUPPL_IS_TRUE(  expr->right->suppl );

  expr->suppl.part.eval_00 |= lf & rf;
  expr->suppl.part.eval_01 |= rt;
  expr->suppl.part.eval_10 |= lt;

}

/*!
 Sets the eval_00/01/10/11 supplemental bits as necessary.  This function should be
 called by expression_op_func__* functions that are NOT events and have both the left
 and right expression children present.
*/
inline static void expression_set_other_eval_NN(
  expression* expr  /*!< Pointer to expression to set 00/01/10/11 supplemental bits */
) {

  uint32 lf = ESUPPL_IS_FALSE( expr->left->suppl  );
  uint32 lt = ESUPPL_IS_TRUE(  expr->left->suppl  );
  uint32 rf = ESUPPL_IS_FALSE( expr->right->suppl );
  uint32 rt = ESUPPL_IS_TRUE(  expr->right->suppl );

  expr->suppl.part.eval_00 |= lf & rf;
  expr->suppl.part.eval_01 |= lf & rt;
  expr->suppl.part.eval_10 |= lt & rf;
  expr->suppl.part.eval_11 |= lt & rt;

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs XOR operation.
*/
bool expression_op_func__xor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__XOR);

  /* Perform XOR operation */
  bool retval = vector_bitwise_xor_op( expr->value, expr->left->value, expr->right->value );

  /* Gather other coverage stats */
  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an XOR-and-assign operation.
*/
bool expression_op_func__xor_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__XOR_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform XOR and gather coverage information */
  retval = vector_bitwise_xor_op( expr->value, tmp, expr->right->value );

  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  /* Fourth, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a multiply operation.
*/
bool expression_op_func__multiply(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MULTIPLY);

  /* Perform multiply operation */
  bool retval = vector_op_multiply( expr->value, expr->left->value, expr->right->value );

  /* Gather other coverage stats */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an multiply-and-assign operation.
*/
bool expression_op_func__multiply_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MULTIPLY_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform multiply */
  retval = vector_op_multiply( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  /* Fourth, assign the new value to the left expression */
  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL :
      expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
      break;
    case VDATA_R64 :
      if( vector_from_real64( expr->left->sig->value, expr->value->value.r64->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    case VDATA_R32 :
      if( vector_from_real64( expr->left->sig->value, (double)expr->value->value.r32->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    default :  assert( 0 );  break;
  }
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 \throws anonymous Throw

 Performs a 32-bit divide operation.
*/
bool expression_op_func__divide(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DIVIDE);

  /* Perform divide operation */
  bool retval = vector_op_divide( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an divide-and-assign operation.
*/
bool expression_op_func__divide_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DIVIDE_A);

  bool    retval = FALSE;                        /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */
  
  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );
  
  /* Perform division operation */
  retval = vector_op_divide( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  /* Finally, assign the new value to the left expression */
  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL :
      expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
      break;
    case VDATA_R64 :
      if( vector_from_real64( expr->left->sig->value, expr->value->value.r64->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    case VDATA_R32 :
      if( vector_from_real64( expr->left->sig->value, (double)expr->value->value.r32->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    default :  assert( 0 );  break;
  }
#else
  assert( 0 );
#endif /* RUNLIB */
  
  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 \throws anonymous Throw

 Performs a 32-bit modulus operation.
*/
bool expression_op_func__mod(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MOD);

  /* Perform mod operation */
  bool retval = vector_op_modulus( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an modulus-and-assign operation.
*/
bool expression_op_func__mod_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MOD_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Perform mod operation and ather coverage information */
  retval = vector_op_modulus( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  /* Finally, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an addition operation.
*/
bool expression_op_func__add(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__ADD);

  /* Perform add operation */
  bool retval = vector_op_add( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an add-and-assign operation.
*/
bool expression_op_func__add_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__ADD_A);
  
  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* Evaluate the left expression */
  (void)sim_expression( expr->left, thr, time, TRUE );
  
  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform addition */
  retval = vector_op_add( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  /* Finally, assign the new value to the left expression */
  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL :
      expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
      break;
    case VDATA_R64 :
      if( vector_from_real64( expr->left->sig->value, expr->value->value.r64->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    case VDATA_R32 :
      if( vector_from_real64( expr->left->sig->value, (double)expr->value->value.r32->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    default :  assert( 0 );  break;
  }
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a subtraction operation.
*/
bool expression_op_func__subtract(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SUBTRACT);

  bool retval;  /* Return value for this function */

  /* Perform SUBTRACT operation */
  expr->elem.tvecs->index = 0;
  retval = vector_op_subtract( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a subtract-and-assign operation.
*/
bool expression_op_func__sub_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SUB_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );
  
  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform subtraction */
  expr->elem.tvecs->index = 1;
  retval = vector_op_subtract( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  /* Finally, assign the new value to the left expression */
  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL :
      expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
      break;
    case VDATA_R64 :
      if( vector_from_real64( expr->left->sig->value, expr->value->value.r64->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    case VDATA_R32 :
      if( vector_from_real64( expr->left->sig->value, (double)expr->value->value.r32->val ) ) {
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    default :  assert( 0 );  break;
  }
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise AND operation.
*/
bool expression_op_func__and(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__AND);

  /* Perform bitwise AND operation */
  bool retval = vector_bitwise_and_op( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_and_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_and_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an AND-and-assign operation.
*/
bool expression_op_func__and_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__AND_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform AND operation */
  retval = vector_bitwise_and_op( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_and_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_and_eval_NN( expr );

  /* Finally, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise OR operation.
*/
bool expression_op_func__or(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__OR);

  /* Perform bitwise OR operation */
  bool retval = vector_bitwise_or_op( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_or_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_or_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an OR-and-assign operation.
*/
bool expression_op_func__or_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__OR_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform OR operation */
  retval = vector_bitwise_or_op( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_or_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_or_eval_NN( expr );

  /* Finally, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NAND operation.
*/
bool expression_op_func__nand(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NAND);

  /* Perform bitwise NAND operation */
  bool retval = vector_bitwise_nand_op( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_and_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_and_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NOR operation.
*/
bool expression_op_func__nor(
                expression*    expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NOR);

  /* Perform bitwise NOR operation */
  bool retval = vector_bitwise_nor_op( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_or_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_or_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NXOR operation.
*/
bool expression_op_func__nxor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NXOR);

  /* Perform bitwise NXOR operation */
  bool retval = vector_bitwise_nxor_op( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_other_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_other_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a less-than comparison operation.
*/
bool expression_op_func__lt(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LT);

  /* Perform less-than operation */
  bool retval = vector_op_lt( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a greater-than comparison operation.
*/
bool expression_op_func__gt(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__GT);

  /* Perform greater-than operation */
  bool retval = vector_op_gt( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a left shift operation.
*/
bool expression_op_func__lshift(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LSHIFT);

  /* Perform left-shift operation */
  bool retval = vector_op_lshift( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an left-shift-and-assign operation.
*/
bool expression_op_func__lshift_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LSHIFT_A);
  
  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */
  
  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform left-shift operation */
  retval = vector_op_lshift( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  /* Finally, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */
  
  PROFILE_END;
  
  return( retval );
  
}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a right shift operation.
*/
bool expression_op_func__rshift(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__RSHIFT);

  /* Perform right-shift operation */
  bool retval = vector_op_rshift( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an right-shift-and-assign operation.
*/
bool expression_op_func__rshift_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__RSHIFT_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform right-shift and collect coverage information */
  retval = vector_op_rshift( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  /* Finally, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an arithmetic right shift operation.
*/
bool expression_op_func__arshift(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__ARSHIFT);

  /* Perform arithmetic right-shift operation */
  bool retval = vector_op_arshift( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an arithmetic right-shift-and-assign operation.
*/
bool expression_op_func__arshift_a(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__ARSHIFT_A);

  bool    retval;                                /* Return value for this function */
#ifndef RUNLIB
  vector* tmp    = &(expr->elem.tvecs->vec[0]);  /* Temporary pointer to temporary vector */
  int     intval = 0;                            /* Integer value */

  /* First, evaluate the left-hand expression */
  (void)sim_expression( expr->left, thr, time, TRUE );

  /* Second, copy the value of the left expression into a temporary vector */
  vector_copy( expr->left->value, tmp );

  /* Third, perform arithmetic right-shift and collect coverage information */
  retval = vector_op_arshift( expr->value, tmp, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  /* Finally, assign the new value to the left expression */
  expression_assign( expr->left, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), FALSE, FALSE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs the $time system call.
*/
bool expression_op_func__time(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__TIME);

  uint64 curr_time = (thr == NULL) ? time->full : thr->curr_time.full;
  bool   retval    = vector_from_uint64( expr->value, (curr_time / thr->funit->timescale) );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs the $random system call.
*/
bool expression_op_func__random(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__RANDOM);

#ifndef RUNLIB
  long rand;

  /* If $random contains a seed parameter, get it */
  if( (expr->left != NULL) && (expr->left->op == EXP_OP_SASSIGN) ) {

    int         intval = 0;
    exp_op_type op     = expr->left->right->op;
    long        seed   = (long)vector_to_int( expr->left->value );

    /* Get random number from seed */
    rand = sys_task_random( &seed );

    /* Store seed value */
    if( (op == EXP_OP_SIG) || (op == EXP_OP_SBIT_SEL) || (op == EXP_OP_MBIT_SEL) || (op == EXP_OP_DIM) ) {
      (void)vector_from_int( expr->left->value, seed );
      expression_assign( expr->left->right, expr->left, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), TRUE, FALSE );
    }

  } else {

    /* Get random value using existing seed value */
    rand = sys_task_random( NULL );

  }
  
  /* Convert it to a vector and store it */
  (void)vector_from_int( expr->value, (int)rand ); 
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a system task port assignment.
*/
bool expression_op_func__sassign(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SASSIGN);

  bool retval;

  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL  :  retval = vector_set_value_ulong( expr->value, expr->right->value->value.ul, expr->right->value->width );  break;
    case VDATA_R64 :
      {
        double real = expr->right->value->value.r64->val;
        retval = !DEQ( expr->value->value.r64->val, real );
        expr->value->value.r64->val = real;
      }
      break;
    case VDATA_R32 :
      {
        float real = expr->right->value->value.r32->val;
        retval = !FEQ( expr->value->value.r32->val, real );
        expr->value->value.r32->val = real;
      }
      break;
    default :  assert( 0 );  break;
  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $srandom system task call.
*/
bool expression_op_func__srandom(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SRANDOM);

#ifndef RUNLIB
  assert( (expr->left != NULL) && (expr->left->op == EXP_OP_SASSIGN) );

  /* Get the seed value and set it */
  sys_task_srandom( (long)vector_to_int( expr->left->value ) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $urandom system task call.
*/
bool expression_op_func__urandom(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__URANDOM);

  unsigned long rand;
  bool          retval;

#ifndef RUNLIB
  /* If $random contains a seed parameter, get it */
  if( (expr->left != NULL) && (expr->left->op == EXP_OP_SASSIGN) ) {

    int         intval = 0;
    exp_op_type op     = expr->left->right->op;
    long        seed   = (long)vector_to_int( expr->left->value );

    /* Get random number from seed */
    rand = sys_task_urandom( &seed );

    /* Store seed value */
    if( (op == EXP_OP_SIG) || (op == EXP_OP_SBIT_SEL) || (op == EXP_OP_MBIT_SEL) || (op == EXP_OP_DIM) ) {
      (void)vector_from_int( expr->left->value, seed );
      expression_assign( expr->left->right, expr->left, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), TRUE, FALSE );
    }

  } else {

    /* Get random value using existing seed value */
    rand = sys_task_urandom( NULL );

  }

  /* Convert it to a vector and store it */
  retval = vector_from_uint64( expr->value, (uint64)rand );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $urandom_range system task call.
*/
bool expression_op_func__urandom_range(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__URANDOM_RANGE);

  expression*   plist = expr->left;
  unsigned long max;
  unsigned long min   = 0;
  unsigned long rand;
  bool          retval;

#ifndef RUNLIB
  if( (plist == NULL) || ((plist->op != EXP_OP_PLIST) && (plist->op != EXP_OP_SASSIGN)) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$urandom_range called without any parameters specified (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  if( plist->op == EXP_OP_SASSIGN ) {

    /* This value should be max value */
    max = (long)vector_to_uint64( plist->value );

  } else {

    assert( (plist->left != NULL) && (plist->left->op == EXP_OP_SASSIGN) );

    /* Get max value */
    max = (long)vector_to_uint64( plist->left->value );

    /* Get min value if it has been specified */
    if( (plist->right != NULL) && (plist->right->op == EXP_OP_SASSIGN) ) {
      min = (long)vector_to_uint64( plist->right->value );
    }

  }

  /* Get random number from seed */
  rand = sys_task_urandom_range( max, min );

  /* Convert it to a vector and store it */
  retval = vector_from_uint64( expr->value, (uint64)rand );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $realtobits system task call.
*/
bool expression_op_func__realtobits(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__REALTOBITS);

  expression* left = expr->left;
  uint64      u64;
  bool        retval;

#ifndef RUNLIB
  /* Check to make sure that there is exactly one parameter */ 
  if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$realtobits called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to make sure that the parameter is a real */
  if( left->value->suppl.part.data_type != VDATA_R64 ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$realtobits called without real parameter (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Make sure that the storage vector is a bits type */
  assert( expr->value->suppl.part.data_type == VDATA_UL );

  /* Convert and store the data */
  retval = vector_from_uint64( expr->value, sys_task_realtobits( left->value->value.r64->val ) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $bitstoreal system task call.
*/
bool expression_op_func__bitstoreal(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__BITSTOREAL);

  expression* left = expr->left;
  uint64      u64;
  bool        retval;

#ifndef RUNLIB
  /* Check to make sure that there is exactly one parameter */
  if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$bitstoreal called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to make sure that the parameter is a real */
  if( left->value->suppl.part.data_type != VDATA_UL ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$bitstoreal called without non-real parameter (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Convert and store the data */
  retval = vector_from_real64( expr->value, sys_task_bitstoreal( vector_to_uint64( left->value ) ) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;
 
  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $shortrealtobits system task call.
*/
bool expression_op_func__shortrealtobits(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SHORTREALTOBITS);

  expression* left = expr->left;
  uint64      u64;
  bool        retval;

#ifndef RUNLIB
  /* Check to make sure that there is exactly one parameter */
  if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$shortrealtobits called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to make sure that the parameter is a real */
  if( left->value->suppl.part.data_type != VDATA_R64 ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$shortrealtobits called without real parameter (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Make sure that the storage vector is a bits type */
  assert( expr->value->suppl.part.data_type == VDATA_UL );

  /* Convert and store the data */
  retval = vector_from_uint64( expr->value, sys_task_shortrealtobits( left->value->value.r64->val ) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $bitstoshortreal system task call.
*/
bool expression_op_func__bitstoshortreal(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__BITSTOSHORTREAL);

  expression* left = expr->left;
  uint64      u64;
  bool        retval;

#ifndef RUNLIB
  /* Check to make sure that there is exactly one parameter */
  if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$bitstoshortreal called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to make sure that the parameter is a real */
  if( left->value->suppl.part.data_type != VDATA_UL ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$bitstoshortreal called without non-real parameter (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Convert and store the data */
  retval = vector_from_real64( expr->value, sys_task_bitstoshortreal( vector_to_uint64( left->value ) ) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $itor system task call.
*/
bool expression_op_func__itor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__ITOR);

  expression* left = expr->left;
  uint64      u64;
  bool        retval;

#ifndef RUNLIB
  /* Check to make sure that there is exactly one parameter */
  if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$itor called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to make sure that the parameter is a real */
  if( left->value->suppl.part.data_type != VDATA_UL ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$itor called without non-real parameter (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Convert and store the data */
  retval = vector_from_real64( expr->value, sys_task_itor( vector_to_int( left->value ) ) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $rtoi system task call.
*/
bool expression_op_func__rtoi(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__RTOI);

  expression* left = expr->left;
  uint64      u64;
  bool        retval;

#ifndef RUNLIB
  /* Check to make sure that there is exactly one parameter */
  if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$rtoi called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Check to make sure that the parameter is a real */
  if( left->value->suppl.part.data_type != VDATA_R64 ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$rtoi called without real parameter (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Make sure that the storage vector is a bits type */
  assert( expr->value->suppl.part.data_type == VDATA_UL );

  /* Convert and store the data */
  retval = vector_from_int( expr->value, sys_task_rtoi( left->value->value.r64->val ) );
#else
  assert( 0 );
#endif

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $test$plusargs system task call.
*/
bool expression_op_func__test_plusargs(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__TEST_PLUSARGS);

  bool retval = FALSE;

#ifndef RUNLIB
  /* Only evaluate this expression if it has not been evaluated yet */
  if( expr->exec_num == 0 ) {

    expression* left     = expr->left;
    uint64      u64;
    char*       arg;
    ulong       scratchl;
    ulong       scratchh = 0;

    /* Check to make sure that there is exactly one parameter */
    if( (left == NULL) || (left->op != EXP_OP_SASSIGN) ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$test$plusargs called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

    /* Get argument to search for */
    arg = vector_to_string( left->value, QSTRING, TRUE, 0 );

    /* Scan the simulation argument list for matching values */
    scratchl = sys_task_test_plusargs( arg );

    /* Perform coverage and assignment */
    retval = vector_set_coverage_and_assign_ulong( expr->value, &scratchl, &scratchh, 0, 0 );

    /* Deallocate memory */
    free_safe( arg, (strlen( arg ) + 1) );

  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $value$plusargs system task call.
*/
bool expression_op_func__value_plusargs(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__VALUE_PLUSARGS);

  bool retval = FALSE;

#ifndef RUNLIB
  /* Only evaluate this expression if it has not been evaluated yet */
  if( expr->exec_num == 0 ) {

    expression* left     = expr->left;
    uint64      u64;
    char*       arg;
    ulong       scratchl;
    ulong       scratchh = 0;
    int         intval   = 0;

    /* Check to make sure that there is exactly two parameters */
    if( (left == NULL) || (left->op != EXP_OP_PLIST) || (left->left->op != EXP_OP_SASSIGN) || (left->right->op != EXP_OP_SASSIGN) ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "$value$plusargs called with incorrect number of parameters (file: %s, line: %u)", thr->funit->orig_fname, expr->line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      Throw 0;
    }

    /* Get the first argument string value */
    arg = vector_to_string( left->left->value, QSTRING, TRUE, 0 );

    /* Scan the simulation argument list for matching values */
    if( (scratchl = sys_task_value_plusargs( arg, left->right->value )) == 1 ) {

      /* Assign the value to the proper signal and propagate the signal change, if an option match occurred */
      switch( left->right->value->suppl.part.data_type ) {
        case VDATA_UL :
          expression_assign( left->right->right, left->right, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), TRUE, FALSE );
          break;
        case VDATA_R64 :
          if( vector_from_real64( left->right->right->sig->value, left->right->value->value.r64->val ) ) {
            vsignal_propagate( left->right->right->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
          }
          break;
        case VDATA_R32 :
          if( vector_from_real64( left->right->right->sig->value, (double)left->right->value->value.r32->val ) ) {
            vsignal_propagate( left->right->right->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
          }
          break;
        default :  assert( 0 );  break;
      }

    }

    /* Perform coverage and assignment */
    retval = vector_set_coverage_and_assign_ulong( expr->value, &scratchl, &scratchh, 0, 0 );

    /* Deallocate memory */
    free_safe( arg, (strlen( arg ) + 1) );

  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $signed system task call.
*/
bool expression_op_func__signed(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SIGNED);

  vector_copy( expr->left->value, expr->value );

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $unsigned system task call.
*/
bool expression_op_func__unsigned(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UNSIGNED);

  vector_copy( expr->left->value, expr->value );

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $clog2 system task call.
*/
bool expression_op_func__clog2(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CLOG2);

  /* Perform the operation */
  bool retval = vector_op_clog2( expr->value, expr->left->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an equality (==) comparison operation.
*/
bool expression_op_func__eq(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__EQ);

  /* Perform equality operation */
  bool retval = vector_op_eq( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an equality (===) comparison operation.
*/
bool expression_op_func__ceq(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CEQ);

  /* Perform case equality operation */
  bool retval = vector_op_ceq( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a less-than-or-equal comparison operation.
*/
bool expression_op_func__le(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LE);

  /* Perform less-than-or-equal operation */
  bool retval = vector_op_le( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a greater-than-or-equal comparison operation.
*/
bool expression_op_func__ge(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__GE);

  /* Perform greater-than-or-equal operation */
  bool retval = vector_op_ge( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a not-equal (!=) comparison operation.
*/
bool expression_op_func__ne(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NE);

  /* Perform not-equal operation */
  bool retval = vector_op_ne( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a not-equal (!==) comparison operation.
*/
bool expression_op_func__cne(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CNE);

  /* Perform case not-equal operation */
  bool retval = vector_op_cne( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a logical OR operation.
*/
bool expression_op_func__lor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LOR);

  /* Perform logical OR operation */
  bool retval = vector_op_lor( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_or_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_or_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a logical AND operation.
*/
bool expression_op_func__land(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LAND);

  /* Perform logical AND operation */
  bool retval = vector_op_land( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_and_comb_evals( expr->value, expr->left->value, expr->right->value );
  expression_set_and_eval_NN( expr );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a conditional (?:) operation.
*/
bool expression_op_func__cond(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__COND);

  bool retval;

  /* Simple vector copy from right side and gather coverage information */
  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL :
      retval = vector_set_value_ulong( expr->value, expr->right->value->value.ul, expr->right->value->width );
      break;
    case VDATA_R64 :
      retval = !DEQ( expr->value->value.r64->val, expr->right->value->value.r64->val );
      expr->value->value.r64->val = expr->right->value->value.r64->val;
      break;
    case VDATA_R32 :
      retval = !FEQ( expr->value->value.r32->val, expr->right->value->value.r32->val );
      expr->value->value.r32->val = expr->right->value->value.r32->val;
      break;
    default :  assert( 0 );  break;
  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a conditional select (?:) operation.
*/
bool expression_op_func__cond_sel(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__COND_SEL);

  bool retval;  /* Return value for this function */

  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL :
      if( !vector_is_unknown( expr->parent->expr->left->value ) ) {
        if( !vector_is_not_zero( expr->parent->expr->left->value ) ) {
          retval = vector_set_value_ulong( expr->value, expr->right->value->value.ul, expr->right->value->width );
        } else {
          retval = vector_set_value_ulong( expr->value, expr->left->value->value.ul, expr->left->value->width );
        }
      } else {
        retval = vector_set_to_x( expr->value );
      }
      break;
    case VDATA_R64 :
      if( !vector_is_unknown( expr->parent->expr->left->value ) ) {
        if( !vector_is_not_zero( expr->parent->expr->left->value ) ) {
          real64 rval = (expr->right->value->suppl.part.data_type == VDATA_UL) ? (double)vector_to_uint64( expr->right->value ) : expr->right->value->value.r64->val;
          retval      = !DEQ( expr->value->value.r64->val, rval );
          expr->value->value.r64->val = rval;
        } else {
          real64 lval = (expr->left->value->suppl.part.data_type == VDATA_UL) ? (double)vector_to_uint64( expr->left->value ) : expr->left->value->value.r64->val;
          retval      = !DEQ( expr->value->value.r64->val, lval );
          expr->value->value.r64->val = lval;
        }
      } else {
        retval = !DEQ( expr->value->value.r64->val, 0.0 );
        expr->value->value.r64->val = 0.0;
      }
      break;
    case VDATA_R32 :
      if( !vector_is_unknown( expr->parent->expr->left->value ) ) {
        if( !vector_is_not_zero( expr->parent->expr->left->value ) ) {
          real32 rval = (expr->right->value->suppl.part.data_type == VDATA_UL) ? (float)vector_to_uint64( expr->right->value ) : expr->right->value->value.r32->val;
          retval      = !DEQ( expr->value->value.r32->val, rval );
          expr->value->value.r64->val = rval;
        } else {
          real32 lval = (expr->left->value->suppl.part.data_type == VDATA_UL) ? (float)vector_to_uint64( expr->left->value ) : expr->left->value->value.r32->val;
          retval      = !DEQ( expr->value->value.r32->val, lval );
          expr->value->value.r32->val = lval;
        }
      } else {
        retval = !DEQ( expr->value->value.r32->val, 0.0 );
        expr->value->value.r32->val = 0.0;
      }
      break;
    default :
      assert( 0 );
      break;
  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary invert operation.
*/
bool expression_op_func__uinv(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UINV);

  /* Perform unary inversion operation */
  bool retval = vector_unary_inv( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary AND operation.
*/
bool expression_op_func__uand(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UAND);

  /* Perform unary AND operation */
  bool retval = vector_unary_and( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NOT operation.
*/
bool expression_op_func__unot(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UNOT);

  /* Perform unary NOT operation */
  bool retval = vector_unary_not( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary OR operation.
*/
bool expression_op_func__uor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UOR);

  /* Perform unary OR operation */
  bool retval = vector_unary_or( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary XOR operation.
*/
bool expression_op_func__uxor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UXOR);

  /* Perform unary XOR operation */
  bool retval = vector_unary_xor( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NAND operation.
*/
bool expression_op_func__unand(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UNAND);

  /* Perform unary NAND operation */
  bool retval = vector_unary_nand( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NOR operation.
*/
bool expression_op_func__unor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UNOR);

  /* Perform unary NOR operation */
  bool retval = vector_unary_nor( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NXOR operation.
*/
bool expression_op_func__unxor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__UNXOR);

  /* Perform unary NXOR operation */
  bool retval = vector_unary_nxor( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 No operation is performed -- expression value is assumed to be changed.
*/
bool expression_op_func__null(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NULL);

  /* Set the true/false indicators as necessary */
  if( expr->op != EXP_OP_STATIC ) {
    expression_set_tf_preclear( expr, TRUE );
  } else {
    expression_set_tf( expr, TRUE );
  }

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.
    
 Performs a signal operation.  This function should be called by any operation that would otherwise
 call the expression_op_func__null operation but have a valid signal pointer.  We just need to copy
 the unknown and not_zero bits from the signal's vector supplemental field to our own.
*/
bool expression_op_func__sig(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SIG);

  /* Gather coverage information */
  if( expr->op != EXP_OP_PARAM ) {
    expression_set_tf_preclear( expr, TRUE );
  } else {
    expression_set_tf( expr, TRUE );
  }

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a single bit select operation.
*/
bool expression_op_func__sbit(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__SBIT);

  bool     retval;
  exp_dim* dim    = (expr->suppl.part.nba == 0) ? expr->elem.dim : expr->elem.dim_nba->dim;
  int      curr_lsb;
  int      intval = (vector_to_int( expr->left->value ) - dim->dim_lsb) * dim->dim_width;
  int      prev_lsb;
  int      vwidth;

  /* Calculate starting bit position and width */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vwidth   = expr->parent->expr->left->value->width;
    prev_lsb = expr->parent->expr->left->elem.dim->curr_lsb;
  } else {
    vwidth   = expr->sig->value->width;
    prev_lsb = 0;
  }

  if( dim->dim_be ) {
    curr_lsb = (prev_lsb + (vwidth - (intval + (int)expr->value->width)));
  } else {
    curr_lsb = (prev_lsb + intval);
  }

  /* If we are the last dimension to be calculated, perform the bit pull */
  if( dim->last ) {
    retval = vector_part_select_pull( expr->value, expr->sig->value, curr_lsb, ((curr_lsb + expr->value->width) - 1), TRUE );
  } else {
    retval = (dim->curr_lsb != curr_lsb);
  }

  /* Set current LSB dimensional information */
  dim->curr_lsb = curr_lsb;

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a multi-bit select operation.
*/
bool expression_op_func__mbit(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MBIT);

  bool     retval = FALSE;  /* Return value for this function */
  int      intval;          /* LSB of range to copy */
  int      vwidth;          /* Width of vector to use */
  int      prev_lsb;
  int      curr_lsb;
  exp_dim* dim    = (expr->suppl.part.nba == 0) ? expr->elem.dim : expr->elem.dim_nba->dim;

  /* Calculate starting bit position */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vwidth   = expr->parent->expr->left->value->width;
    prev_lsb = expr->parent->expr->left->elem.dim->curr_lsb;
  } else {
    vwidth   = expr->sig->value->width;
    prev_lsb = 0;
  }

  /* Calculate the LSB and MSB and copy the bit range */
  intval = ((dim->dim_be ? vector_to_int( expr->left->value ) : vector_to_int( expr->right->value )) - dim->dim_lsb) * dim->dim_width;
  if( dim->dim_be ) {
    curr_lsb = (prev_lsb + (vwidth - (intval + (int)expr->value->width)));
  } else {
    curr_lsb = (prev_lsb + intval);
  }

  if( dim->last ) {
    retval = vector_part_select_pull( expr->value, expr->sig->value, curr_lsb, ((curr_lsb + expr->value->width) - 1), TRUE );
  } else {
    retval = (curr_lsb != dim->curr_lsb);
  }

  /* Set current LSB dimensional information */
  dim->curr_lsb = curr_lsb;

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a expansion ({{}}) operation.
*/
bool expression_op_func__expand(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__EXPAND);

  bool retval = FALSE;

  if( expr->value->width > 0 ) {

    /* Perform expansion operation */
    retval = vector_op_expand( expr->value, expr->left->value, expr->right->value );

    /* Gather coverage information */
    expression_set_tf_preclear( expr, retval );
    vector_set_unary_evals( expr->value );

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a list (,) operation.
*/
bool expression_op_func__list(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__LIST);

  bool retval = FALSE;

  if( expr->value->width > 0 ) {

    /* Perform list operation */
    retval = vector_op_list( expr->value, expr->left->value, expr->right->value );

    /* Gather coverage information */
    expression_set_tf_preclear( expr, retval );
    vector_set_unary_evals( expr->value );

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a concatenation ({}) operation.
*/
bool expression_op_func__concat(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CONCAT);

  /* Perform concatenation operation */
  bool retval = vector_set_value_ulong( expr->value, expr->right->value->value.ul, expr->right->value->width );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a positive edge event operation.
*/
bool expression_op_func__pedge(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__PEDGE);

  bool   retval;   /* Return value for this function */
  ulong  nvall = expr->right->value->value.ul[0][VTYPE_INDEX_EXP_VALL];
  ulong  nvalh = expr->right->value->value.ul[0][VTYPE_INDEX_EXP_VALH];
  ulong* ovall = &(expr->elem.tvecs->vec[0].value.ul[0][VTYPE_INDEX_EXP_VALL]);
  ulong* ovalh = &(expr->elem.tvecs->vec[0].value.ul[0][VTYPE_INDEX_EXP_VALH]);

  if( ((*ovalh | ~(*ovall)) & (~nvalh & nvall)) && thr->suppl.part.exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  /* Set left LAST value to current value of right */
  *ovalh = nvalh;
  *ovall = nvall;

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negative-edge event operation.
*/
bool expression_op_func__nedge(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NEDGE);

  bool   retval;   /* Return value for this function */
  ulong  nvall = expr->right->value->value.ul[0][VTYPE_INDEX_EXP_VALL];
  ulong  nvalh = expr->right->value->value.ul[0][VTYPE_INDEX_EXP_VALH];
  ulong* ovall = &(expr->elem.tvecs->vec[0].value.ul[0][VTYPE_INDEX_EXP_VALL]);
  ulong* ovalh = &(expr->elem.tvecs->vec[0].value.ul[0][VTYPE_INDEX_EXP_VALH]);

  if( ((*ovalh | *ovall) & (~nvalh & ~nvall)) && thr->suppl.part.exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  /* Set left LAST value to current value of right */
  *ovalh = nvalh;
  *ovall = nvall;

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an any-edge event operation.
*/
bool expression_op_func__aedge(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__AEDGE);

  bool retval;  /* Return value of this function */

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

    /* We only need to do full value comparison if the right expression is something other than a signal */ 
    if( thr->suppl.part.exec_first && ((expr->right->op == EXP_OP_SIG) || !vector_ceq_ulong( &(expr->elem.tvecs->vec[0]), expr->right->value )) ) {
      expr->suppl.part.true   = 1;
      expr->suppl.part.eval_t = 1;
      vector_copy( expr->right->value, &(expr->elem.tvecs->vec[0]) );
      retval = TRUE;
    } else {
      expr->suppl.part.eval_t = 0;
      retval = FALSE;
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a event OR operation.
*/
bool expression_op_func__eor(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
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
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a sensitivity list operation.
*/
bool expression_op_func__slist(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
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
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a delay operation.
*/
bool expression_op_func__delay(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DELAY);

  bool retval = FALSE;  /* Return value for this function */

#ifndef RUNLIB
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
    vector_to_sim_time( expr->right->value, *(expr->elem.scale), &tmp_time );

    /* Make sure that we set the final value to FALSE */
    tmp_time.final = FALSE;

    /* Add this delay into the delay queue if this is not the final simulation step */
    if( !time->final ) {
      TIME_INC(tmp_time, thr->curr_time);
      sim_thread_insert_into_delay_queue( thr, &tmp_time );
    }

  }
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an event trigger (->) operation.
*/
bool expression_op_func__trigger(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__TRIGGER);

#ifndef RUNLIB
  /* Indicate that we have triggered */
  expr->sig->value->value.ul[0][VTYPE_INDEX_SIG_VALL] = 1;
  expr->sig->value->value.ul[0][VTYPE_INDEX_SIG_VALH] = 0;

  /* Propagate event */
  vsignal_propagate( expr->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a case comparison operation.
*/
bool expression_op_func__case(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CASE);

  /* Perform case comparison operation */
  bool retval = vector_op_ceq( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a casex comparison operation.
*/
bool expression_op_func__casex(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CASEX);

  /* Perform casex comparison operation */
  bool retval = vector_op_cxeq( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage inforamtion */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a casez comparison operation.
*/
bool expression_op_func__casez(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__CASEZ);

  /* Perform casez comparison operation */
  bool retval = vector_op_czeq( expr->value, expr->left->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a case default comparison operation.
*/
bool expression_op_func__default(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DEFAULT);

  bool  retval;    /* Return value for this function */
  ulong valh = 0;  /* High value to store */
  ulong vall = 1;  /* Low value to store */

  /* Perform default case operation */
  retval = vector_set_coverage_and_assign_ulong( expr->value, &vall, &valh, 0, 0 );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a blocking assignment operation.
*/
bool expression_op_func__assign(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__BASSIGN);

#ifndef RUNLIB
  bool nb = (expr->op == EXP_OP_NASSIGN);

  /* Perform assignment */
  switch( expr->right->value->suppl.part.data_type ) {
    case VDATA_UL :
      switch( expr->left->value->suppl.part.data_type ) {
        case VDATA_UL :
          {
            int intval = 0;
            expression_assign( expr->left, expr->right, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), TRUE, nb );
          }
          break;
        case VDATA_R64 :
          if( !nb ) {
            expr->left->value->value.r64->val = vector_to_real64( expr->right->value );
            vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
          }
          break;
        case VDATA_R32 :
          if( !nb ) {
            expr->left->value->value.r32->val = (float)vector_to_real64( expr->right->value );
            vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
          }
          break;
        default :  assert( 0 );  break;
      }
      break;
    case VDATA_R64 :
      if( !nb ) {
        assert( expr->left->sig != NULL );
        (void)vector_from_real64( expr->left->sig->value, (real64)expr->right->value->value.r64->val );
        expr->left->sig->value->suppl.part.set = 1;
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    case VDATA_R32 :
      if( !nb ) {
        assert( expr->left->sig != NULL );
        (void)vector_from_real64( expr->left->sig->value, (real64)expr->right->value->value.r32->val );
        expr->left->sig->value->suppl.part.set = 1;
        vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      }
      break;
    default :  assert( 0 );  break;
  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, TRUE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a function call operation.
*/
bool expression_op_func__func_call(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__FUNC_CALL);

  bool retval;  /* Return value for this function */

#ifndef RUNLIB
  /* First, simulate the function */
  sim_thread( sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time ), ((thr == NULL) ? time : &(thr->curr_time)) );

  /* Then copy the function variable to this expression */
  switch( expr->value->suppl.part.data_type ) {
    case VDATA_UL  :  retval = vector_set_value_ulong( expr->value, expr->sig->value->value.ul, expr->value->width );  break;
    case VDATA_R64 :  retval = vector_from_real64( expr->value, expr->sig->value->value.r64->val );  break;
    case VDATA_R32 :  retval = vector_from_real64( expr->value, (double)expr->sig->value->value.r32->val );  break;
    default        :  assert( 0 );  break;
  }
  
  /* Deallocate the reentrant structure of the current thread (if it exists) */
  if( (thr != NULL) && (thr->ren != NULL) ) {
    reentrant_dealloc( thr->ren, thr->funit, expr );
    thr->ren = NULL;
  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a task call operation.
*/
bool expression_op_func__task_call(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__TASK_CALL);

#ifndef RUNLIB
  (void)sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time );

  /* Gather coverage information */
  vector_set_unary_evals( expr->value );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( FALSE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a named block call operation.
*/
bool expression_op_func__nb_call(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NB_CALL);

  bool retval = FALSE;  /* Return value for this function */

#ifndef RUNLIB
  /* Add the thread to the active queue */
  thread* tmp = sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time );

  if( ESUPPL_IS_IN_FUNC( expr->suppl ) ) {

    sim_thread( tmp, &(thr->curr_time) );
    retval = TRUE;

  }
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a fork operation.
*/
bool expression_op_func__fork(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__FORK);

#ifndef RUNLIB
  (void)sim_add_thread( thr, expr->elem.funit->first_stmt, expr->elem.funit, time );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, TRUE );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a join operation.
*/
bool expression_op_func__join(
  /*@unused@*/ expression*     expr,  /*!< Pointer to expression to perform operation on */
               thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__JOIN);

  /* Gather coverage information */
  expression_set_tf_preclear( expr, (thr->active_children == 0) );

  PROFILE_END;

  return( thr->active_children == 0 );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a block disable operation.
*/
bool expression_op_func__disable(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DISABLE);

#ifndef RUNLIB
  sim_kill_thread_with_funit( expr->elem.funit );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, TRUE );

  PROFILE_END;
#else
  assert( 0 );
#endif /* RUNLIB */

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a repeat loop operation.
*/
bool expression_op_func__repeat(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__REPEAT);

  bool retval;  /* Return value for this function */

  retval = vector_op_lt( expr->value, expr->left->value, expr->right->value );

  if( expr->value->value.ul[0][VTYPE_INDEX_VAL_VALL] == 0 ) {
    (void)vector_from_int( expr->left->value, 0 );
  } else {
    (void)vector_from_int( expr->left->value, (vector_to_int( expr->left->value ) + 1) );
  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an exponential power operation.
*/
bool expression_op_func__exponent(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__EXPONENT);

  bool retval;  /* Return value for this function */

  /* If the left and right expression values are not unknown, calculate exponent */
  if( !vector_is_unknown( expr->left->value ) && !vector_is_unknown( expr->right->value ) ) {

    int   intval1;
    int   intval2;
    ulong vall = 1;
    ulong valh = 0;
    int   i;

    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    for( i=0; i<intval2; i++ ) {
      vall *= intval1;
    }

    /* Assign value and calculate coverage */
    retval = vector_set_coverage_and_assign_ulong( expr->value, &vall, &valh, 0, 31 );

  /* Otherwise, the entire value should be X */
  } else {

    retval = vector_set_to_x( expr->value );

  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a port assignment operation.
*/
bool expression_op_func__passign(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__PASSIGN);

  bool retval = FALSE;  /* Return value for this function */
  int  intval = 0;      /* Integer value */

#ifndef RUNLIB
  /* If the current thread is running an automatic function, create a reentrant structure for it */
  if( (thr != NULL) && (thr->ren == NULL) &&
      ((thr->funit->suppl.part.type == FUNIT_AFUNCTION) ||
       (thr->funit->suppl.part.type == FUNIT_ATASK)     ||
       (thr->funit->suppl.part.type == FUNIT_ANAMED_BLOCK)) ) {
    thr->ren = reentrant_create( thr->funit );
  }

  switch( expr->sig->suppl.part.type ) {

    /* If the connected signal is an input type, copy the parameter expression value to this vector */
    case SSUPPL_TYPE_INPUT_NET :
    case SSUPPL_TYPE_INPUT_REG :
      retval = vector_set_value_ulong( expr->value, expr->right->value->value.ul, expr->right->value->width );
      vsignal_propagate( expr->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
      break;

    /*
     If the connected signal is an output type, do an expression assign from our expression value
     to the right expression.
    */
    case SSUPPL_TYPE_OUTPUT_NET :
    case SSUPPL_TYPE_OUTPUT_REG :
      expression_assign( expr->right, expr, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), TRUE, FALSE );
      retval = TRUE;
      break;

    /* We don't currently support INOUT as these are not used in tasks or functions */
    default :
      assert( (expr->sig->suppl.part.type == SSUPPL_TYPE_INPUT_NET) ||
              (expr->sig->suppl.part.type == SSUPPL_TYPE_INPUT_REG) ||
              (expr->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT_NET) ||
              (expr->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT_REG) );
      break;

  }

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a positive variable multi-bit select operation.
*/
bool expression_op_func__mbit_pos(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MBIT_POS);

  bool     retval   = FALSE;  /* Return value for this function */
  exp_dim* dim      = (expr->suppl.part.nba == 0) ? expr->elem.dim : expr->elem.dim_nba->dim;
  int      curr_lsb = 0;
  int      vwidth;
  int      intval   = (vector_to_int( expr->left->value ) - dim->dim_lsb) * dim->dim_width;
  int      prev_lsb = ((ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr)) ? expr->parent->expr->left->elem.dim->curr_lsb : 0;

  /* Calculate starting bit position */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vwidth = expr->parent->expr->left->value->width;
  } else {
    vwidth = expr->sig->value->width;
  }

  curr_lsb = (prev_lsb + intval);

  /* If this is the last dimension, perform the assignment */
  if( dim->last ) {
    retval = vector_part_select_pull( expr->value, expr->sig->value, curr_lsb, ((curr_lsb + vector_to_int( expr->right->value )) - 1), TRUE );
  } else {
    retval = (dim->curr_lsb != curr_lsb);
  }

  /* Set current LSB dimensional information */
  dim->curr_lsb = curr_lsb;

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negative variable multi-bit select operation.
*/
bool expression_op_func__mbit_neg(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__MBIT_NEG);

  bool     retval   = FALSE;  /* Return value for this function */
  exp_dim* dim      = (expr->suppl.part.nba == 0) ? expr->elem.dim : expr->elem.dim_nba->dim;
  int      curr_lsb;
  int      vwidth;
  int      intval1  = vector_to_int( expr->left->value ) - dim->dim_lsb;
  int      intval2  = vector_to_int( expr->right->value );
  int      prev_lsb = ((ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr)) ? expr->parent->expr->left->elem.dim->curr_lsb : 0;

  /* Calculate starting bit position */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) && (expr->parent->expr->right == expr) ) {
    vwidth = expr->parent->expr->left->value->width;
  } else {
    vwidth = expr->sig->value->width;
  }

  intval1 = vector_to_int( expr->left->value ) - dim->dim_lsb;
  intval2 = vector_to_int( expr->right->value );

  curr_lsb = (prev_lsb + ((intval1 - intval2) + 1));

  /* If this is the last dimension, perform the assignment */
  if( dim->last ) { 
    retval = vector_part_select_pull( expr->value, expr->sig->value, curr_lsb, ((curr_lsb + vector_to_int( expr->right->value )) - 1), TRUE );
  } else {
    retval = (dim->curr_lsb != curr_lsb);
  }

  /* Set the dimensional current LSB with the calculated value */
  dim->curr_lsb = curr_lsb;

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negate of the specified expression.
*/
bool expression_op_func__negate(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__NEGATE);

  bool retval;  /* Return value for this function */

  /* Perform negate operation and gather coverage information */
  expr->elem.tvecs->index = 0;
  retval = vector_op_negate( expr->value, expr->right->value );

  /* Gather coverage information */
  expression_set_tf_preclear( expr, retval );
  vector_set_unary_evals( expr->value );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE.

 Performs an immediate increment operation.
*/
bool expression_op_func__iinc(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__IINC);

#ifndef RUNLIB
  /* Perform increment */
  expr->elem.tvecs->index = 0;
  if( expr->left->sig != NULL ) {
    (void)vector_op_inc( expr->left->sig->value, expr->elem.tvecs );
    expr->left->sig->value->suppl.part.set = 1;
  } else {
    (void)vector_op_inc( expr->left->value, expr->elem.tvecs );
  }

  /* Assign the left value to our value */
  switch( expr->left->value->suppl.part.data_type ) {
    case VDATA_UL  :  (void)vector_set_value_ulong( expr->value, expr->left->value->value.ul, expr->left->value->width );  break;
    case VDATA_R64 :  expr->value->value.r64->val = expr->left->value->value.r64->val;  break;
    case VDATA_R32 :  expr->value->value.r32->val = expr->left->value->value.r32->val;  break;
    default        :  assert( 0 );  break;
  }

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE.

 Performs a postponed increment operation.
*/
bool expression_op_func__pinc(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__PINC);

#ifndef RUNLIB
  /* Copy the left-hand value to our expression */
  switch( expr->left->value->suppl.part.data_type ) {
    case VDATA_UL  :  (void)vector_set_value_ulong( expr->value, expr->left->value->value.ul, expr->left->value->width );  break;
    case VDATA_R64 :  expr->value->value.r64->val = expr->left->value->value.r64->val;  break;
    case VDATA_R32 :  expr->value->value.r32->val = expr->left->value->value.r32->val;  break;
    default        :  assert( 0 );  break;
  }

  /* Perform increment */
  expr->elem.tvecs->index = 0;
  if( expr->left->sig != NULL ) {
    (void)vector_op_inc( expr->left->sig->value, expr->elem.tvecs );
    expr->left->sig->value->suppl.part.set = 1;
  } else {
    (void)vector_op_inc( expr->left->value, expr->elem.tvecs );
  }

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an immediate decrement operation.
*/
bool expression_op_func__idec(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__IDEC);

#ifndef RUNLIB
  /* Perform decrement */
  expr->elem.tvecs->index = 0;
  if( expr->left->sig != NULL ) {
    (void)vector_op_dec( expr->left->sig->value, expr->elem.tvecs );
    expr->left->sig->value->suppl.part.set = 1;
  } else {
    (void)vector_op_dec( expr->left->value, expr->elem.tvecs );
  }

  /* Copy the left-hand value to our expression */
  switch( expr->left->value->suppl.part.data_type ) {
    case VDATA_UL  :  (void)vector_set_value_ulong( expr->value, expr->left->value->value.ul, expr->left->value->width );  break;
    case VDATA_R64 :  expr->value->value.r64->val = expr->left->value->value.r64->val;  break;
    case VDATA_R32 :  expr->value->value.r32->val = expr->left->value->value.r32->val;  break;
    default        :  assert( 0 );  break;
  }

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a postponed decrement operation.
*/
bool expression_op_func__pdec(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__PDEC);

#ifndef RUNLIB
  /* Copy the left-hand value to our expression */
  switch( expr->left->value->suppl.part.data_type ) {
    case VDATA_UL  :  (void)vector_set_value_ulong( expr->value, expr->left->value->value.ul, expr->left->value->width );  break;
    case VDATA_R64 :  expr->value->value.r64->val = expr->left->value->value.r64->val;  break;
    case VDATA_R32 :  expr->value->value.r32->val = expr->left->value->value.r32->val;  break;
    default        :  assert( 0 );  break;
  }

  /* Perform decrement */
  expr->elem.tvecs->index = 0;
  if( expr->left->sig != NULL ) {
    (void)vector_op_dec( expr->left->sig->value, expr->elem.tvecs );
    expr->left->sig->value->suppl.part.set = 1;
  } else {
    (void)vector_op_dec( expr->left->value, expr->elem.tvecs );
  }

#ifdef DEBUG_MODE
  if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
    printf( "        " );  vsignal_display( expr->left->sig );
    printf( "        " );  expression_display( expr->left );
  }
#endif

  /* Propagate value change */
  vsignal_propagate( expr->left->sig, ((thr == NULL) ? time : &(thr->curr_time)) );
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( TRUE );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a delayed assignment.
*/
bool expression_op_func__dly_assign(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DLY_ASSIGN);

  bool retval;      /* Return value for this function */
  int  intval = 0;  /* Integer value */

#ifndef RUNLIB
  /* If we are the first statement in the queue, perform the dly_op manually */
  if( thr->suppl.part.exec_first && (expr->right->left->op == EXP_OP_DELAY) ) {
    (void)expression_op_func__dly_op( expr->right, thr, time );
  }

  /* Check the dly_op expression.  If eval_t is set to 1, perform the assignment */
  if( ESUPPL_IS_TRUE( expr->right->suppl ) == 1 ) {
    expression_assign( expr->left, expr->right, &intval, thr, ((thr == NULL) ? time : &(thr->curr_time)), TRUE, FALSE );
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an assignment and delay for a delayed assignment.
*/
bool expression_op_func__dly_op(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DLY_OP);

  /* If we are not waiting for the delay to occur, copy the contents of the operation */
  if( !thr->suppl.part.exec_first ) {
    (void)vector_set_value_ulong( expr->value, expr->right->value->value.ul, expr->right->value->width );
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
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a repeated delay for a given assignment.
*/
bool expression_op_func__repeat_dly(
  expression*     expr,  /*!< Pointer to expression to perform operation on */
  thread*         thr,   /*!< Pointer to thread containing this expression */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__REPEAT_DLY);

  bool retval = FALSE;  /* Return value for this function */

  /* If the delay condition has been met, call the repeat operation */
  if( exp_op_info[expr->right->op].func( expr->right, thr, time ) ) {

    /* Execute repeat operation */
    (void)expression_op_func__repeat( expr->left, thr, time );

    /* If the repeat operation evaluated to TRUE, perform delay operation */
    if( expr->left->value->value.ul[0][VTYPE_INDEX_VAL_VALL] == 1 ) {
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
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a multi-array dimension operation.
*/
bool expression_op_func__dim(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__DIM);

  bool retval;                                  /* Return value for this function */
  int  rlsb = expr->right->elem.dim->curr_lsb;  /* Right dimensional LSB value */

  /* If the right-hand dimensional LSB value is different than our own, set it to the new value and return TRUE */
  retval = (rlsb != expr->elem.dim->curr_lsb);
  expr->elem.dim->curr_lsb = rlsb;

  /* Get coverage information */
  expression_set_tf_preclear( expr, retval );
 
  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a wait statement operation.
*/
bool expression_op_func__wait(
               expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__WAIT);

  bool retval;  /* Return value for this function */

  /* If the right expression evaluates to TRUE, continue; otherwise, do a context switch */
  if( vector_is_not_zero( expr->right->value ) ) {
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
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a $finish statement operation.
*/
bool expression_op_func__finish(
  /*@unused@*/ expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__FINISH);

#ifndef RUNLIB
  sim_finish();
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( FALSE );

}

/*!                                        
 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.  
                                           
 Performs a $stop statement operation.
*/                                         
bool expression_op_func__stop(
  /*@unused@*/ expression*     expr,  /*!< Pointer to expression to perform operation on */
  /*@unused@*/ thread*         thr,   /*!< Pointer to thread containing this expression */
  /*@unused@*/ const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OP_FUNC__STOP);

#ifndef RUNLIB
  sim_stop();
#else
  assert( 0 );
#endif /* RUNLIB */

  PROFILE_END;

  return( FALSE );

}

/*!
 \return Returns TRUE if the assigned expression value was different than the previously stored value;
         otherwise, returns FALSE.

 \throw anonymous func

 Performs expression operation.  This function must only be run after its
 left and right expressions have been calculated during this clock period.
 Sets the value of the operation in its own vector value and updates the
 suppl uint8 as necessary.
*/
bool expression_operate(
  expression*     expr,  /*!< Pointer to expression to set value to */
  thread*         thr,   /*!< Pointer to current thread being simulated */
  const sim_time* time   /*!< Pointer to current simulation time */
) { PROFILE(EXPRESSION_OPERATE);

  bool retval = TRUE;  /* Return value for this function */

  if( expr != NULL ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "      In expression_operate, id: %d, op: %s, line: %u, addr: %p",
                                  expr->id, expression_string_op( expr->op ), expr->line, expr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    assert( expr->value != NULL );

    /* Call expression operation */
    retval = exp_op_info[expr->op].func( expr, thr, time );

#ifndef RUNLIB
    /* If this expression is attached to an FSM, perform the FSM calculation now */
    if( expr->table != NULL ) {
      fsm_table_set( expr, time );
    }
#endif /* RUNLIB */

    /* Specify that we have executed this expression */
    (expr->exec_num)++;

  }

  PROFILE_END;

  return( retval );

}

/*!
 \throws anonymous expression_resize expression_operate_recursively expression_operate_recursively
 
 Recursively performs the proper operations to cause the top-level expression to
 be set to a value.  This function is called during the parse stage to derive 
 pre-CDD widths of multi-bit expressions.  Each MSB/LSB is an expression tree that 
 needs to be evaluated to set the width properly on the MBIT_SEL expression.
*/
void expression_operate_recursively(
  expression* expr,   /*!< Pointer to top of expression tree to perform recursive operations */
  func_unit*  funit,  /*!< Pointer to functional unit containing this expression */
  bool        sizing  /*!< Set to TRUE if we are evaluating for purposes of sizing */
) { PROFILE(EXPRESSION_OPERATE_RECURSIVELY);
    
  if( expr != NULL ) {
    
    /* Create dummy time */
    sim_time time = {0,0,0,FALSE};

    /* Evaluate children */
    expression_operate_recursively( expr->left,  funit, sizing );
    expression_operate_recursively( expr->right, funit, sizing );
    
#ifndef RUNLIB
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
#endif /* RUNLIB */
    
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
 Sets line coverage for the given expression.
*/
void expression_set_line_coverage(
  expression* exp
) { PROFILE(EXPRESSION_SET_LINE_COVERAGE);

  exp->exec_num++;

  PROFILE_END;

}

#ifndef RUNLIB
/*!
 Assigns data from a dumpfile to the given expression's coverage bits according to the
 given action and the expression itself.
*/
void expression_vcd_assign(
  expression* expr,    /*!< Pointer to expression to assign */
  char        action,  /*!< Specifies action to perform for an expression */
  const char* value    /*!< Coverage data from dumpfile to assign */
) { PROFILE(EXPRESSION_VCD_ASSIGN);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Assigning expression (line: %u, op: %s, id: %d, action=%c) to value %s",
                                expr->line, expression_string_op( expr->op ), expr->id, action, value );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  if( action == 'L' ) {

    /* If we have seen a value of 1, increment the exec_num to indicate that the line has been hit */
    if( value[0] == '1' ) {
      expr->exec_num++;
    }

  } else if( (action == 'e') || (action == 'E') ) {
    expr->suppl.part.true |= (value[0] == '1');

  } else if( (action == 'u') || (action == 'U') ) {
    expr->suppl.part.true  |= (value[0] == '1');
    expr->suppl.part.false |= (value[0] == '0');
             
  } else if( (action == 'c') || (action == 'C') ) {

    /* Since we need to sign-extend values, calculate the lt, lf, rt and rf values */
    uint32 lt = (value[1] != '\0') ? (value[0] == '1') : 0;
    uint32 lf = (value[1] != '\0') ? (value[0] == '0') : ((value[0] == '0') || (value[0] == '1'));
    uint32 rt = (value[1] != '\0') ? (value[1] == '1') : (value[0] == '1');
    uint32 rf = (value[1] != '\0') ? (value[1] == '0') : (value[0] == '0');

    if( exp_op_info[expr->op].suppl.is_comb == AND_COMB ) {
      expr->suppl.part.eval_10 |= rf;
      expr->suppl.part.eval_01 |= lf;
      expr->suppl.part.eval_11 |= (lt & rt);

    } else if( exp_op_info[expr->op].suppl.is_comb == OR_COMB ) {
      expr->suppl.part.eval_01 |= rt;
      expr->suppl.part.eval_10 |= lt;
      expr->suppl.part.eval_00 |= (lf & rf);

    } else if( exp_op_info[expr->op].suppl.is_comb == OTHER_COMB ) {
      expr->suppl.part.eval_00 |= (lf & rf);
      expr->suppl.part.eval_01 |= (lf & rt);
      expr->suppl.part.eval_10 |= (lt & rf);
      expr->suppl.part.eval_11 |= (lt & rt);

    }

  } else if( (action == 'r') || (action == 'R') ) {

    uint64 intval;

    if( convert_str_to_uint64( value, 31, 0, &intval ) ) {
      vector_set_mem_rd_ulong( expr->sig->value, ((expr->value->width - 1) + intval), intval );
    }

  } else if( (action == 'w') || (action == 'W') ) {

    uint64 intval;

    if( convert_str_to_uint64( value, 31, 0, &intval ) ) {
      if( strlen( value ) > 32 ) {
        char* tmpval = strdup_safe( value );
        tmpval[strlen( value ) - 32] = '\0';
        (void)vector_vcd_assign( expr->sig->value, tmpval, ((expr->value->width - 1) + intval), intval );
        free_safe( tmpval, (strlen( value ) + 1) );
      } else {
        (void)vector_vcd_assign( expr->sig->value, "0", ((expr->value->width - 1) + intval), intval );
      }
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if expression contains only static expressions; otherwise, returns FALSE.
 
 Recursively iterates through specified expression tree and returns TRUE if all of
 the leaf expressions are static expressions (STATIC or parameters).
*/
static bool expression_is_static_only_helper(
            expression* expr,  /*!< Pointer to expression to evaluate */
  /*@out@*/ bool*       one    /*!< Pointer to value that holds whether a static value of zero was found */
) { PROFILE(EXPRESSION_IS_STATIC_ONLY_HELPER);

  bool retval;  /* Return value for this function */

  if( expr != NULL ) {

    if( (EXPR_IS_STATIC( expr ) == 1) ||
        (ESUPPL_IS_LHS( expr->suppl ) == 1) ||
        ((expr->op == EXP_OP_SIG) && (expr->sig != NULL) &&
         ((expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM) ||
          (expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM_REAL) ||
          (expr->sig->suppl.part.type == SSUPPL_TYPE_ENUM))) ) {
      retval = TRUE;
      if( one != NULL ) {
        *one |= (expr->value->value.ul != NULL) ? vector_is_not_zero( expr->value ) : TRUE;
      }
    } else if( expr->op == EXP_OP_CONCAT ) {
      bool curr_one   = FALSE;
      bool all_static = expression_is_static_only_helper( expr->right, &curr_one );
      retval          = (!curr_one && all_static) || curr_one;
    } else if( expr->op == EXP_OP_COND ) {
      retval = expression_is_static_only_helper( expr->right, one );
    } else {
      if( (expr->op != EXP_OP_MBIT_SEL)  &&
          (expr->op != EXP_OP_SBIT_SEL)  &&
          (expr->op != EXP_OP_SIG)       &&
          (expr->op != EXP_OP_FUNC_CALL) &&
          (EXPR_IS_OP_AND_ASSIGN( expr ) == 0) ) {
        bool lstatic = expression_is_static_only_helper( expr->left,  one );
        bool rstatic = expression_is_static_only_helper( expr->right, one );
        retval       = lstatic && rstatic;
      } else {
        retval = FALSE;
      }
    }

  } else {

    retval = TRUE;

  }

  PROFILE_END;

  return( retval );

}

bool expression_is_static_only(
  expression* expr  /*!< Pointer to expression to evaluate */
) {

  return( expression_is_static_only_helper( expr, NULL ) ); 

}
#endif /* RUNLIB */

/*!
 \return Returns TRUE if specified expression is on the LHS of a blocking assignment operator.
*/
static bool expression_is_assigned(
  expression* expr  /*!< Pointer to expression to check */
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

    while( (expr != NULL) &&
           (ESUPPL_IS_ROOT( expr->suppl ) == 0)        &&
           (expr->op != EXP_OP_BASSIGN)                &&
           (expr->op != EXP_OP_RASSIGN)                &&
           (expr->parent->expr->op != EXP_OP_SBIT_SEL) &&
           (expr->parent->expr->op != EXP_OP_MBIT_SEL) &&
           (expr->parent->expr->op != EXP_OP_MBIT_POS) &&
           (expr->parent->expr->op != EXP_OP_MBIT_NEG) ) {
      expr = expr->parent->expr;
    }

    retval = (expr != NULL) && ((expr->op == EXP_OP_BASSIGN) || (expr->op == EXP_OP_RASSIGN)) ;

  }

  PROFILE_END;

  return( retval );

}

#ifndef RUNLIB
/*!
 \return Returns TRUE if the specifies expression belongs in a single or mult-bit select expression
*/
bool expression_is_bit_select(
  expression* expr  /*!< Pointer to expression to check */
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
#endif /* RUNLIB */

/*!
 \return Returns TRUE if the specified expression is the last (most right-hand) part/bit select of a signal;
         otherwise, returns FALSE.
*/
bool expression_is_last_select(
  expression* expr  /*!< Pointer to expression to check for last select */
) { PROFILE(EXPRESSION_IS_LAST_SELECT);

  bool retval = (ESUPPL_IS_ROOT( expr->suppl ) == 1) ||
                ( ((expr->parent->expr->op == EXP_OP_DIM) &&
                   (expr->parent->expr->right == expr) &&
                   (ESUPPL_IS_ROOT( expr->parent->expr->suppl ) == 0) &&
                   (expr->parent->expr->parent->expr->op != EXP_OP_DIM)) ||
                  (expr->parent->expr->op != EXP_OP_DIM) );

  PROFILE_END;

  return( retval );

}

#ifndef RUNLIB
/*!
 \return Returns a pointer to the first dimensional select of a memory.
*/
expression* expression_get_first_select(
  expression* expr  /*!< Pointer to last dimension expression */
) { PROFILE(EXPRESSION_GET_FIRST_SELECT);

  while( (ESUPPL_IS_ROOT( expr->suppl ) == 0) && (expr->parent->expr->op == EXP_OP_DIM) ) {
    expr = expr->parent->expr;
  }

  PROFILE_END;

  return( expr );

}


/*!
 \return Returns TRUE if the specified expression is in an RASSIGN expression tree; otherwise,
         returns FALSE.
*/
bool expression_is_in_rassign(
  expression* expr  /*!< Pointer to expression to examine */
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
 Checks to see if the expression is in the LHS of a BASSIGN expression tree.  If it is, it sets the
 assigned supplemental field of the expression's signal vector to indicate the the value of this
 signal will come from Covered instead of the dumpfile.  This is called in the bind_signal function
 after the expression and signal have been bound (only in parsing stage).
*/
void expression_set_assigned(
  expression* expr  /*!< Pointer to current expression to evaluate */
) { PROFILE(EXPRESSION_SET_ASSIGNED);

  expression* curr;  /* Pointer to current expression */

  assert( expr != NULL );

  if( ESUPPL_IS_LHS( expr->suppl ) == 1 ) {

    curr = expr;
    while( (ESUPPL_IS_ROOT( curr->suppl ) == 0)        &&
           (curr->op != EXP_OP_BASSIGN)                &&
           (curr->op != EXP_OP_RASSIGN)                &&
           (curr->parent->expr->op != EXP_OP_SBIT_SEL) &&
           (curr->parent->expr->op != EXP_OP_MBIT_SEL) &&
           (curr->parent->expr->op != EXP_OP_MBIT_POS) &&
           (curr->parent->expr->op != EXP_OP_MBIT_NEG) ) {
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
 Recursively iterates down expression tree, setting the left/right changed bits in each expression.  This
 is necessary to get continuous assignments and FSM statements to simulate (even if their driving signals
 have not changed).
*/
void expression_set_changed(
  expression* expr  /*!<  Pointer to expression to set changed bits for */
) { PROFILE(EXPRESSION_SET_CHANGED);

  if( expr != NULL ) {

    /* Set the children expressions */
    expression_set_changed( expr->left );
    expression_set_changed( expr->right );

    /* Now set our bits */
    expr->suppl.part.left_changed  = 1;
    expr->suppl.part.right_changed = 1;

  }

  PROFILE_END;

}

/*!
 Recursively iterates through specified LHS expression, assigning the value from the RHS expression.
 This is called whenever a blocking assignment expression is found during simulation.
*/
void expression_assign(
  expression*     lhs,       /*!< Pointer to current expression on left-hand-side of assignment to calculate for */
  expression*     rhs,       /*!< Pointer to the right-hand-expression that will be assigned from */
  int*            lsb,       /*!< Current least-significant bit in rhs value to start assigning */
  thread*         thr,       /*!< Pointer to thread for the given expressions */
  const sim_time* time,      /*!< Specifies current simulation time when expression assignment occurs */
  bool            eval_lhs,  /*!< If TRUE, allows the left-hand expression to be evaluated, if necessary (should be
                                  set to TRUE unless for specific cases where it is not necessary and would be
                                  redundant to do so -- i.e., op-and-assign cases) */
  bool            nb         /*!< If TRUE, this assignment should be a non-blocking assignment */
) { PROFILE(EXPRESSION_ASSIGN);

  if( lhs != NULL ) {

    exp_dim* dim      = (lhs->suppl.part.nba == 0) ? lhs->elem.dim : lhs->elem.dim_nba->dim;
    int      vwidth   = 0;
    int      prev_lsb = 0;

    /* Calculate starting vector value bit and signal LSB/BE for LHS */
    if( lhs->sig != NULL ) {
      if( (lhs->parent->expr->op == EXP_OP_DIM) && (lhs->parent->expr->right == lhs) ) {
        vwidth   = lhs->parent->expr->left->value->width;
        prev_lsb = lhs->parent->expr->left->elem.dim->curr_lsb;
      } else {
        vwidth   = lhs->sig->value->width;
        prev_lsb = 0;
      }
    }

#ifdef DEBUG_MODE
    if( debug_mode ) {
      if( ((dim != NULL) && dim->last) || (lhs->op == EXP_OP_SIG) ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "        In expression_assign, lhs_op: %s, rhs_op: %s, lsb: %d, time: %" FMT64 "u, nb: %d",
                                    expression_string_op( lhs->op ), expression_string_op( rhs->op ), *lsb, time->full, nb );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, DEBUG, __FILE__, __LINE__ );
      }
    }
#endif

    switch( lhs->op ) {
      case EXP_OP_SIG      :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          if( nb ) {
            if( lhs->suppl.part.nba == 1 ) {
              sim_add_nonblock_assign( lhs->elem.dim_nba->nba, 0, (lhs->value->width - 1), *lsb, (rhs->value->width - 1) );
            }
          } else {
            bool changed = vector_part_select_push( lhs->sig->value, 0, (lhs->value->width - 1), rhs->value, *lsb, (rhs->value->width - 1), rhs->value->suppl.part.is_signed );
            lhs->sig->value->suppl.part.set = 1;
#ifdef DEBUG_MODE
            if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
              printf( "        " );  vsignal_display( lhs->sig );
            }
#endif
            if( changed ) {
              vsignal_propagate( lhs->sig, time );
            }
          }
        }
        *lsb += lhs->value->width;
        break;
      case EXP_OP_SBIT_SEL :
        if( eval_lhs && (ESUPPL_IS_LEFT_CHANGED( lhs->suppl ) == 1) ) {
          (void)sim_expression( lhs->left, thr, time, TRUE );
        }
        if( lhs->sig->suppl.part.assigned == 1 ) {
          bool changed = FALSE;
          if( !vector_is_unknown( lhs->left->value ) ) {
            int intval = (vector_to_int( lhs->left->value ) - dim->dim_lsb) * dim->dim_width;
            if( intval >= 0 ) {           // Only perform assignment if selected bit is within range
              if( dim->dim_be ) {
                dim->curr_lsb = ((intval > vwidth) || (prev_lsb == -1)) ? -1 : (prev_lsb + (vwidth - (intval + (int)lhs->value->width)));
              } else {
                dim->curr_lsb = ((intval >= vwidth) || (prev_lsb == -1)) ? -1 : (prev_lsb + intval);
              }
            } else {
              dim->curr_lsb = -1;
            }
          }
          if( dim->last && (dim->curr_lsb != -1) ) {
            if( nb ) {
              if( lhs->suppl.part.nba == 1 ) {
                sim_add_nonblock_assign( lhs->elem.dim_nba->nba, dim->curr_lsb, ((dim->curr_lsb + lhs->value->width) - 1), *lsb, (rhs->value->width - 1) );
              }
            } else {
              changed = vector_part_select_push( lhs->sig->value, dim->curr_lsb, ((dim->curr_lsb + lhs->value->width) - 1), rhs->value, *lsb, (rhs->value->width - 1), FALSE );
              lhs->sig->value->suppl.part.set = 1;
#ifdef DEBUG_MODE
              if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
                printf( "        " );  vsignal_display( lhs->sig );
              }
#endif
              if( changed ) {
                vsignal_propagate( lhs->sig, time );
              }
            }
          }
        }
        if( dim->last && (dim->curr_lsb != -1) ) {
          *lsb += lhs->value->width;
        }
        break;
      case EXP_OP_MBIT_SEL :
        if( lhs->sig->suppl.part.assigned == 1 ) {
          bool changed = FALSE;
          int  intval  = ((dim->dim_be ? vector_to_int( lhs->left->value ) : vector_to_int( lhs->right->value )) - dim->dim_lsb) * dim->dim_width;
          assert( intval >= 0 );
          if( dim->dim_be ) {
            assert( intval <= vwidth );
            dim->curr_lsb = (prev_lsb == -1) ? -1 : (prev_lsb + (vwidth - (intval + (int)lhs->value->width)));
          } else {
            assert( intval < vwidth );
            dim->curr_lsb = (prev_lsb == -1) ? -1 : (prev_lsb + intval);
          }
          if( dim->last && (dim->curr_lsb != -1) ) {
            if( nb ) {
              if( lhs->suppl.part.nba == 1 ) {
                sim_add_nonblock_assign( lhs->elem.dim_nba->nba, dim->curr_lsb, ((dim->curr_lsb + lhs->value->width) - 1), *lsb, (rhs->value->width - 1) );
              }
            } else {
              changed = vector_part_select_push( lhs->sig->value, dim->curr_lsb, ((dim->curr_lsb + lhs->value->width) - 1), rhs->value, *lsb, (rhs->value->width - 1), FALSE );
              lhs->sig->value->suppl.part.set = 1;
#ifdef DEBUG_MODE
              if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
                printf( "        " );  vsignal_display( lhs->sig );
              }
#endif
              if( changed ) {
                vsignal_propagate( lhs->sig, time );
              }
            }
          }
        }
        if( dim->last && (dim->curr_lsb != -1) ) {
          *lsb += lhs->value->width;
        }
        break;
#ifdef NOT_SUPPORTED
      case EXP_OP_MBIT_POS :
        if( eval_lhs && (ESUPPL_IS_LEFT_CHANGED( lhs->suppl ) == 1) ) {
          (void)sim_expression( lhs->left, thr, time, TRUE );
        }
        if( lhs->sig->suppl.part.assigned == 1 ) {
          if( !lhs->left->value->suppl.part.unknown ) {
            intval1 = (vector_to_int( lhs->left->value ) - dim_lsb) * lhs->value->width;
            intval2 = vector_to_int( lhs->right->value ) * lhs->value->width;
            assert( intval1 >= 0 );
            assert( ((intval1 + intval2) - 1) < lhs->sig->value->width );
            lhs->value->value.ul = vstart + intval1;
          }
          if( assign ) {
            if( nb ) {
              if( lhs->suppl.part.nba == 1 ) {
                sim_add_nonblock_assign( lhs->elem.dim_nba->nba, dim->curr_lsb, ((dim->curr_lsb + lhs->value->width) - 1), *lsb, (rhs->value->width - 1) );
              }
            } else {
              bool changed = vector_set_value( lhs->value, rhs->value->value.ul, intval2, *lsb, 0 );
              lhs->sig->value->suppl.part.set = 1;
#ifdef DEBUG_MODE
              if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
                printf( "        " );  vsignal_display( lhs->sig );
              }
#endif
              if( changed ) {
                vsignal_propagate( lhs->sig, time );
              }
            }
          }
        }
        if( (dim != NULL) && dim->last ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
      case EXP_OP_MBIT_NEG :
        if( eval_lhs && (ESUPPL_IS_LEFT_CHANGED( lhs->suppl ) == 1) ) {
          (void)sim_expression( lhs->left, thr, time, TRUE );
        }
        if( lhs->sig->suppl.part.assigned == 1 ) {
          if( !lhs->left->value->part.unknown ) {
            intval1 = (vector_to_int( lhs->left->value ) - dim_lsb) * lhs->value->width;
            intval2 = vector_to_int( lhs->right->value ) * lhs->value->width;
            assert( intval1 < lhs->sig->value->width );
            assert( ((intval1 - intval2) + 1) >= 0 );
            lhs->value->value.ul = vstart + ((intval1 - intval2) + 1);
          }
          if( assign ) {
            if( nb ) {
              if( lhs->suppl.part.nba == 1 ) {
                sim_add_nonblock_assign( lhs->elem.dim_nba->nba, dim->curr_lsb, ((dim->curr_lsb + lhs->value->width) - 1), *lsb, (rhs->value->width - 1) );
              }
            } else {
              bool changed = vector_set_value( lhs->value, rhs->value->value.ul, intval2, *lsb, 0 );
              lhs->sig->value->suppl.part.set = 1;
#ifdef DEBUG_MODE
              if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
                printf( "        " );  vsignal_display( lhs->sig );
              }
#endif
              if( changed ) {
                vsignal_propagate( lhs->sig, time );
              }
            }
          }
        }
        if( assign ) {
          *lsb = *lsb + lhs->value->width;
        }
        break;
#endif
      case EXP_OP_CONCAT   :
      case EXP_OP_LIST     :
        expression_assign( lhs->right, rhs, lsb, thr, time, eval_lhs, nb );
        expression_assign( lhs->left,  rhs, lsb, thr, time, eval_lhs, nb );
        break;
      case EXP_OP_DIM      :
        expression_assign( lhs->left,  rhs, lsb, thr, time, eval_lhs, nb );
        expression_assign( lhs->right, rhs, lsb, thr, time, eval_lhs, nb );
        lhs->elem.dim->curr_lsb = lhs->right->elem.dim->curr_lsb;
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
#endif /* RUNLIB */

/*!
 Deallocates all heap memory allocated with the malloc routine.
*/
void expression_dealloc(
  expression* expr,     /*!< Pointer to root expression to deallocate */
  bool        exp_only  /*!< Removes only the specified expression and not its children */
) { PROFILE(EXPRESSION_DEALLOC);

  int        op;        /* Temporary operation holder */
  statement* tmp_stmt;  /* Temporary pointer to statement */

  if( expr != NULL ) {

    op = expr->op;

    if( ESUPPL_OWNS_VEC( expr->suppl ) ) {

      /* Free up memory from vector value storage */
      vector_dealloc( expr->value );
      expr->value = NULL;

      /* If this is a named block call or fork call, remove the statement that this expression points to */
      if( (expr->op == EXP_OP_NB_CALL) || (expr->op == EXP_OP_FORK) ) {

        if( !exp_only && (expr->elem.funit != NULL) ) {
#ifdef DEBUG_MODE
          if( debug_mode ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block starting at line %d because it is a NB_CALL and its calling expression is being removed",
                                        expr->elem.funit->first_stmt->exp->line );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, DEBUG, __FILE__, __LINE__ );
          }
#endif
#ifndef RUNLIB
          stmt_blk_add_to_remove_list( expr->elem.funit->first_stmt );
#endif /* RUNLIB */
        } else {
          bind_remove( expr->id, FALSE );
        }

      /* If this is a task call, remove the bind */
      } else if( expr->op == EXP_OP_TASK_CALL ) {
         
        bind_remove( expr->id, FALSE );

      } else if( expr->op == EXP_OP_FUNC_CALL ) {

        /* Remove this expression from the attached signal's expression list (if the signal has not been deallocated yet) */
        if( expr->sig != NULL ) {
          exp_link_remove( expr, &(expr->sig->exps), &(expr->sig->exp_size), FALSE );
        }

        bind_remove( expr->id, FALSE );

      /* Otherwise, we assume (for now) that the expression is a signal */
      } else {

        if( expr->sig == NULL ) {

          /* Remove this expression from the binding list */
          bind_remove( expr->id, expression_is_assigned( expr ) );

        } else {

          /* Remove this expression from the attached signal's expression list */
          exp_link_remove( expr, &(expr->sig->exps), &(expr->sig->exp_size), FALSE );

          /* Clear the assigned bit of the attached signal */
          if( expression_is_assigned( expr ) ) {
  
            expr->sig->suppl.part.assigned = 0;

            /* If this signal must be assigned, remove all statement blocks that reference this signal */
            if( (expr->sig->suppl.part.mba == 1) && !exp_only ) {
              unsigned int i;
              for( i=0; i<expr->sig->exp_size; i++ ) {
                if( (tmp_stmt = expression_get_root_statement( expr->sig->exps[i] )) != NULL ) {
#ifdef DEBUG_MODE
                  if( debug_mode ) {
                    print_output( "Removing statement block because a statement block is being removed that assigns an MBA", DEBUG, __FILE__, __LINE__ );
                  }
#endif
#ifndef RUNLIB
                  stmt_blk_add_to_remove_list( tmp_stmt );
#endif /* RUNLIB */
                }
              }
            }

          }
        
        }

      }

    } else {

      /* Remove our expression from its signal, if we have one */
      if( expr->sig != NULL ) {
        exp_link_remove( expr, &(expr->sig->exps), &(expr->sig->exp_size), FALSE );
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

    /* If we have temporary vectors to deallocate, do so now */
    if( (expr->elem.tvecs != NULL) && (EXPR_TMP_VECS( expr->op ) > 0) ) {
      unsigned int i;
      for( i=0; i<EXPR_TMP_VECS( expr->op ); i++ ) {
        vector_dealloc_value( &(expr->elem.tvecs->vec[i]) );
      }
      free_safe( expr->elem.tvecs, sizeof( vecblk ) );
    }

    /* If we have expression dimensional information, deallocate it now */
    if( (expr->elem.dim != NULL) && EXPR_OP_HAS_DIM( expr->op ) ) {
      if( expr->suppl.part.nba == 1 ) {
        free_safe( expr->elem.dim_nba->dim, sizeof( exp_dim ) );
        free_safe( expr->elem.dim_nba->nba, sizeof( nonblock_assign ) );
        free_safe( expr->elem.dim_nba, sizeof( dim_and_nba ) );
      } else {
        free_safe( expr->elem.dim, sizeof( exp_dim ) );
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
