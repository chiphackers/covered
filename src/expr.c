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
 \author   Trevor Williams  (trevorw@charter.net)
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
 expression changes value in its parameter list.  The expression's value vector pointer is
 automatically set to the return value signal in the function by the binding process.  As such,
 a function call expression should never deallocate its vector.
 
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
 
 \par EXP_OP_SLIST
 The EXP_OP_SLIST expression is a 1-bit expression whose value is meaningless.  It indicates the
 Verilog-2001 sensitivity list @* for a statement block and its right expression is attaches to
 an EOR attached list of AEDGE operations.  The SLIST expression works like an EOR but only checks
 the right child.  When outputting an expression tree whose root expression is an SLIST, the rest
 of the expression should be ignored for outputting purposes.

 \par EXP_OP_NOOP
 This expression does nothing.  It is not measurable but is simply a placeholder for a statement that
 Covered will not handle (i.e., standard system calls that only contain inputs) but will not dismiss
 the statement block that the statement exists in.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "link.h"
#include "vector.h"
#include "binding.h"
#include "util.h"
#include "sim.h"
#include "fsm.h"
#include "func_unit.h"
#include "vsignal.h"


extern nibble xor_optab[OPTAB_SIZE];
extern nibble and_optab[OPTAB_SIZE];
extern nibble or_optab[OPTAB_SIZE];
extern nibble nand_optab[OPTAB_SIZE];
extern nibble nor_optab[OPTAB_SIZE];
extern nibble nxor_optab[OPTAB_SIZE];

extern int  curr_sim_time;
extern char user_msg[USER_MSG_LENGTH];

extern exp_link* static_expr_head;
extern exp_link* static_expr_tail;

extern bool debug_mode;
extern int  generate_expr_mode;
extern int  curr_expr_id;

static bool expression_op_func__xor( expression*, thread* );
static bool expression_op_func__multiply( expression*, thread* );
static bool expression_op_func__divide( expression*, thread* );
static bool expression_op_func__mod( expression*, thread* );
static bool expression_op_func__add( expression*, thread* );
static bool expression_op_func__subtract( expression*, thread* );
static bool expression_op_func__and( expression*, thread* );
static bool expression_op_func__or( expression*, thread* );
static bool expression_op_func__nand( expression*, thread* );
static bool expression_op_func__nor( expression*, thread* );
static bool expression_op_func__nxor( expression*, thread* );
static bool expression_op_func__lt( expression*, thread* );
static bool expression_op_func__gt( expression*, thread* );
static bool expression_op_func__lshift( expression*, thread* );
static bool expression_op_func__rshift( expression*, thread* );
static bool expression_op_func__arshift( expression*, thread* );
static bool expression_op_func__eq( expression*, thread* );
static bool expression_op_func__ceq( expression*, thread* );
static bool expression_op_func__le( expression*, thread* );
static bool expression_op_func__ge( expression*, thread* );
static bool expression_op_func__ne( expression*, thread* );
static bool expression_op_func__cne( expression*, thread* );
static bool expression_op_func__lor( expression*, thread* );
static bool expression_op_func__land( expression*, thread* );
static bool expression_op_func__cond( expression*, thread* );
static bool expression_op_func__cond_sel( expression*, thread* );
static bool expression_op_func__uinv( expression*, thread* );
static bool expression_op_func__uand( expression*, thread* );
static bool expression_op_func__unot( expression*, thread* );
static bool expression_op_func__uor( expression*, thread* );
static bool expression_op_func__uxor( expression*, thread* );
static bool expression_op_func__unand( expression*, thread* );
static bool expression_op_func__unor( expression*, thread* );
static bool expression_op_func__unxor( expression*, thread* );
static bool expression_op_func__sbit( expression*, thread* );
static bool expression_op_func__expand( expression*, thread* );
static bool expression_op_func__concat( expression*, thread* );
static bool expression_op_func__pedge( expression*, thread* );
static bool expression_op_func__nedge( expression*, thread* );
static bool expression_op_func__aedge( expression*, thread* );
static bool expression_op_func__eor( expression*, thread* );
static bool expression_op_func__slist( expression*, thread* );
static bool expression_op_func__delay( expression*, thread* );
static bool expression_op_func__case( expression*, thread* );
static bool expression_op_func__casex( expression*, thread* );
static bool expression_op_func__casez( expression*, thread* );
static bool expression_op_func__default( expression*, thread* );
static bool expression_op_func__list( expression*, thread* );
static bool expression_op_func__bassign( expression*, thread* );
static bool expression_op_func__func_call( expression*, thread* );
static bool expression_op_func__task_call( expression*, thread* );
static bool expression_op_func__trigger( expression*, thread* );
static bool expression_op_func__nb_call( expression*, thread* );
static bool expression_op_func__fork( expression*, thread* );
static bool expression_op_func__join( expression*, thread* );
static bool expression_op_func__disable( expression*, thread* );
static bool expression_op_func__repeat( expression*, thread* );
static bool expression_op_func__null( expression*, thread* );
static bool expression_op_func__exponent( expression*, thread* );
static bool expression_op_func__passign( expression*, thread* );
static bool expression_op_func__mbit_pos( expression*, thread* );
static bool expression_op_func__mbit_neg( expression*, thread* );
static bool expression_op_func__negate( expression*, thread* );
static bool expression_op_func__noop( expression*, thread* );


/*!
 Array containing static information about expression operation types.  NOTE:  This structure MUST be
 updated if a new expression is added!  The third argument is an initialization to the exp_info_s structure.
*/
const exp_info exp_op_info[EXP_OP_NUM] = { {"STATIC",         "",        expression_op_func__null,      {0, 1, NOT_COMB,   1, 0, 0} },
                                           {"SIG",            "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"XOR",            "^",       expression_op_func__xor,       {0, 0, OTHER_COMB, 0, 1, 0} },
                                           {"MULTIPLY",       "*",       expression_op_func__multiply,  {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"DIVIDE",         "/",       expression_op_func__divide,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"MOD",            "%",       expression_op_func__mod,       {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"ADD",            "+",       expression_op_func__add,       {0, 0, OTHER_COMB, 0, 1, 0} },
                                           {"SUBTRACT",       "-",       expression_op_func__subtract,  {0, 0, OTHER_COMB, 0, 1, 0} },
                                           {"AND",            "&",       expression_op_func__and,       {0, 0, AND_COMB,   0, 1, 0} },
                                           {"OR",             "|",       expression_op_func__or,        {0, 0, OR_COMB,    0, 1, 0} },
                                           {"NAND",           "~&",      expression_op_func__nand,      {0, 0, AND_COMB,   0, 1, 0} },
                                           {"NOR",            "~|",      expression_op_func__nor,       {0, 0, OR_COMB,    0, 1, 0} },
                                           {"NXOR",           "~^",      expression_op_func__nxor,      {0, 0, OTHER_COMB, 0, 1, 0} },
                                           {"LT",             "<",       expression_op_func__lt,        {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"GT",             ">",       expression_op_func__gt,        {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"LSHIFT",         "<<",      expression_op_func__lshift,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"RSHIFT",         ">>",      expression_op_func__rshift,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"EQ",             "==",      expression_op_func__eq,        {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"CEQ",            "===",     expression_op_func__ceq,       {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"LE",             "<=",      expression_op_func__le,        {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"GE",             ">=",      expression_op_func__ge,        {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"NE",             "!=",      expression_op_func__ne,        {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"CNE",            "!==",     expression_op_func__cne,       {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"LOR",            "||",      expression_op_func__lor,       {0, 0, OR_COMB,    0, 1, 0} },
                                           {"LAND",           "&&",      expression_op_func__land,      {0, 0, AND_COMB,   0, 1, 0} },
                                           {"COND",           "?:",      expression_op_func__cond,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"COND_SEL",       "",        expression_op_func__cond_sel,  {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"UINV",           "~",       expression_op_func__uinv,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UAND",           "&",       expression_op_func__uand,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UNOT",           "!",       expression_op_func__unot,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UOR",            "|",       expression_op_func__uor,       {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UXOR",           "^",       expression_op_func__uxor,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UNAND",          "~&",      expression_op_func__unand,     {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UNOR",           "~|",      expression_op_func__unor,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"UNXOR",          "~^",      expression_op_func__unxor,     {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"SBIT_SEL",       "[]",      expression_op_func__sbit,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"MBIT_SEL",       "[:]",     expression_op_func__null,      {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"EXPAND",         "{{}}",    expression_op_func__expand,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"CONCAT",         "{}",      expression_op_func__concat,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"PEDGE",          "posedge", expression_op_func__pedge,     {1, 0, NOT_COMB,   0, 1, 1} },
                                           {"NEDGE",          "negedge", expression_op_func__nedge,     {1, 0, NOT_COMB,   0, 1, 1} },
                                           {"AEDGE",          "anyedge", expression_op_func__aedge,     {1, 0, NOT_COMB,   0, 1, 1} },
                                           {"LAST",           "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"EOR",            "or",      expression_op_func__eor,       {1, 0, NOT_COMB,   1, 0, 1} },
                                           {"DELAY",          "#",       expression_op_func__delay,     {1, 0, NOT_COMB,   0, 0, 1} },
                                           {"CASE",           "case",    expression_op_func__case,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"CASEX",          "casex",   expression_op_func__casex,     {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"CASEZ",          "casez",   expression_op_func__casez,     {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"DEFAULT",        "",        expression_op_func__default,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"LIST",           "",        expression_op_func__list,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"PARAM",          "",        expression_op_func__null,      {0, 1, NOT_COMB,   1, 0, 0} },
                                           {"PARAM_SBIT",     "[]",      expression_op_func__sbit,      {0, 1, NOT_COMB,   1, 0, 0} },
                                           {"PARAM_MBIT",     "[:]",     expression_op_func__null,      {0, 1, NOT_COMB,   1, 0, 0} },
                                           {"ASSIGN",         "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"DASSIGN",        "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"BASSIGN",        "",        expression_op_func__bassign,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"NASSIGN",        "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"IF",             "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"FUNC_CALL",      "",        expression_op_func__func_call, {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"TASK_CALL",      "",        expression_op_func__task_call, {0, 0, NOT_COMB,   1, 0, 1} },
                                           {"TRIGGER",        "->",      expression_op_func__trigger,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"NB_CALL",        "",        expression_op_func__nb_call,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"FORK",           "",        expression_op_func__fork,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"JOIN",           "",        expression_op_func__join,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"DISABLE",        "",        expression_op_func__disable,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"REPEAT",         "",        expression_op_func__repeat,    {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"WHILE",          "",        expression_op_func__null,      {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"ALSHIFT",        "<<<",     expression_op_func__lshift,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"ARSHIFT",        ">>>",     expression_op_func__arshift,   {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"SLIST",          "@*",      expression_op_func__slist,     {1, 0, NOT_COMB,   0, 1, 1} },
                                           {"EXPONENT",       "**",      expression_op_func__exponent,  {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"PASSIGN",        "",        expression_op_func__passign,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"RASSIGN",        "",        expression_op_func__bassign,   {0, 0, NOT_COMB,   1, 0, 0} },
                                           {"MBIT_POS",       "[+:]",    expression_op_func__mbit_pos,  {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"MBIT_NEG",       "[-:]",    expression_op_func__mbit_neg,  {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"PARAM_MBIT_POS", "[+:]",    expression_op_func__mbit_pos,  {0, 1, NOT_COMB,   1, 0, 0} },
                                           {"PARAM_MBIT_NEG", "[-:]",    expression_op_func__mbit_neg,  {0, 1, NOT_COMB,   1, 0, 0} },
                                           {"NEGATE",         "-",       expression_op_func__negate,    {0, 0, NOT_COMB,   1, 1, 0} },
                                           {"NOOP",           "",        expression_op_func__null,      {0, 0, NOT_COMB,   0, 0, 0} } };

/*!
 \param exp    Pointer to expression to add value to.
 \param width  Width of value to create.
 \param data   Specifies if nibble array should be allocated for vector.

 Creates a value vector that is large enough to store width number of
 bits in value and sets the specified expression value to this value.  This
 function should be called by either the expression_create function, the bind
 function, or the signal db_read function.
*/
void expression_create_value( expression* exp, int width, bool data ) {

  vec_data* value = NULL;  /* Temporary storage of vector nibble array */

  if( (data == TRUE) || ((exp->suppl.part.gen_expr == 1) && (width > 0)) ) {
    value = (vec_data*)malloc_safe( (sizeof( vec_data ) * width), __FILE__, __LINE__ );
  }

  /* Create value */
  vector_init( exp->value, value, width );

}

/*!
 \param right  Pointer to expression on right.
 \param left   Pointer to expression on left.
 \param op     Operation to perform for this expression.
 \param lhs    Specifies this expression is a left-hand-side assignment expression.
 \param id     ID for this expression as determined by the parent.
 \param line   Line number this expression is on.
 \param first  First column index of expression.
 \param last   Last column index of expression.
 \param data   Specifies if we should create a nibble array for the vector value.

 \return Returns pointer to newly created expression.

 Creates a new expression from heap memory and initializes its values for
 usage.  Right and left expressions need to be created before this function is called.
*/
expression* expression_create( expression* right, expression* left, exp_op_type op, bool lhs, int id,
                               int line, int first, int last, bool data ) {

  expression* new_expr;    /* Pointer to newly created expression */
  int         rwidth = 0;  /* Bit width of expression on right */
  int         lwidth = 0;  /* Bit width of expression on left */

  new_expr = (expression*)malloc_safe( sizeof( expression ), __FILE__, __LINE__ );

  new_expr->suppl.all           = 0;
  new_expr->suppl.part.lhs      = (nibble)lhs & 0x1;
  new_expr->suppl.part.owns_vec = EXPR_OWNS_VEC( op, new_expr->suppl );
  new_expr->suppl.part.gen_expr = (generate_expr_mode > 0) ? 1 : 0;
  new_expr->op                  = op;
  new_expr->id                  = id;
  new_expr->ulid                = -1;
  new_expr->line                = line;
  new_expr->col                 = ((first & 0xffff) << 16) | (last & 0xffff);
  new_expr->exec_num            = 0;
  new_expr->sig                 = NULL;
  new_expr->parent              = (expr_stmt*)malloc_safe( sizeof( expr_stmt ), __FILE__, __LINE__ );
  new_expr->parent->expr        = NULL;
  new_expr->right               = right;
  new_expr->left                = left;
  new_expr->value               = (vector*)malloc_safe( sizeof( vector ), __FILE__, __LINE__ );
  new_expr->table               = NULL;
  new_expr->stmt                = NULL;
  new_expr->name                = NULL;

  if( right != NULL ) {

    /* Get information from right */
    assert( right->value != NULL );
    rwidth = right->value->width;

  }

  if( left != NULL ) {

    /* Get information from left */
    assert( left->value != NULL );
    lwidth = left->value->width;

  }

  /* Create value vector */
  if( ((op == EXP_OP_MULTIPLY) || (op == EXP_OP_LIST)) && (rwidth > 0) && (lwidth > 0) ) {

    /* For multiplication, we need a width the sum of the left and right expressions */
    assert( rwidth < 1024 );
    assert( lwidth < 1024 );
    expression_create_value( new_expr, (lwidth + rwidth), data );

  } else if( (op == EXP_OP_CONCAT) && (rwidth > 0) ) {

    assert( rwidth < 1024 );
    expression_create_value( new_expr, rwidth, data );

  } else if( (op == EXP_OP_EXPAND) && (rwidth > 0) && (lwidth > 0) && (left->value->value != NULL) ) {

    assert( rwidth < 1024 );
    assert( lwidth < 1024 );

    /*
     If the left-hand expression is a known value, go ahead and create the value here; otherwise,
     hold off because our vector value will be coming.
    */
    if( !vector_is_unknown( left->value ) ) {
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
             (op == EXP_OP_REPEAT) ) {

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
        assert( rwidth < 1024 );
        expression_create_value( new_expr, rwidth, data );
      } else {
        /* Check to make sure that nothing has gone drastically wrong */
        assert( lwidth < 1024 );
        expression_create_value( new_expr, lwidth, data );
      }

    } else {
 
      expression_create_value( new_expr, 0, FALSE );
 
    }

  }

  if( (data == FALSE) && (generate_expr_mode == 0) ) {
    assert( new_expr->value->value == NULL );
  }

  return( new_expr );

}

/*!
 \param exp  Pointer to expression to set value to.
 \param vec  Pointer to vector value to set expression to.
 
 Sets the specified expression (if necessary) to the value of the
 specified vector value.
*/
void expression_set_value( expression* exp, vector* vec ) {
  
  int lbit;  /* Bit boundary specified by left child */
  int rbit;  /* Bit boundary specified by right child */
  
  assert( exp != NULL );
  assert( exp->value != NULL );
  assert( vec != NULL );

  /* Regardless of the type, set the supplemental field */
  exp->value->suppl = vec->suppl;
  
  switch( exp->op ) {
    case EXP_OP_SIG       :
    case EXP_OP_PARAM     :
    case EXP_OP_FUNC_CALL :
    case EXP_OP_TRIGGER   :
      exp->value->value = vec->value;
      exp->value->width = vec->width;
      break;
    case EXP_OP_SBIT_SEL   :
    case EXP_OP_PARAM_SBIT :
      exp->value->value = vec->value;
      exp->value->width = 1;
      break;
    case EXP_OP_MBIT_SEL   :
    case EXP_OP_PARAM_MBIT :
      expression_operate_recursively( exp->left,  TRUE );
      expression_operate_recursively( exp->right, TRUE );
      lbit = vector_to_int( exp->left->value  );
      rbit = vector_to_int( exp->right->value );
      if( lbit <= rbit ) {
        exp->value->width = ((rbit - lbit) + 1);
        if( exp->op == EXP_OP_PARAM_MBIT ) {
          exp->value->value = vec->value + ((vec->width - exp->value->width) - lbit);
        } else {
          exp->value->value = vec->value + (((vec->width - exp->value->width) - lbit) - exp->sig->lsb);
        }
      } else {
        exp->value->width = ((lbit - rbit) + 1);
        if( exp->op == EXP_OP_PARAM_MBIT ) {
          exp->value->value = vec->value + rbit;
        } else {
          exp->value->value = vec->value + (rbit - exp->sig->lsb);
        }
      }
      break;
    case EXP_OP_MBIT_POS :
    case EXP_OP_MBIT_NEG :
    case EXP_OP_PARAM_MBIT_POS :
    case EXP_OP_PARAM_MBIT_NEG :
      expression_operate_recursively( exp->right, TRUE );
      exp->value->value = vec->value;
      exp->value->width = vector_to_int( exp->right->value );
      break;
    default :  break;
  }

}

/*!
 \param exp  Pointer to current expression

 Recursively sets the signed bit for all parent expressions if both the left and
 right expressions have the signed bit set.  This function is called by the bind()
 function after an expression has been set to a signal.
*/
void expression_set_signed( expression* exp ) {

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

}

/*!
 \param expr       Pointer to expression to potentially resize.
 \param recursive  Specifies if we should perform a recursive depth-first resize

 Resizes the given expression depending on the expression operation and its
 children's sizes.  If recursive is TRUE, performs the resize in a depth-first
 fashion, resizing the children before resizing the current expression.  If
 recursive is FALSE, only the given expression is evaluated and resized.
*/
void expression_resize( expression* expr, bool recursive ) {

  int    largest_width;  /* Holds larger width of left and right children */
  nibble old_vec_suppl;  /* Holds original vector supplemental field as this will be erased */

  if( expr != NULL ) {

    if( recursive ) {
      expression_resize( expr->left, recursive );
      expression_resize( expr->right, recursive );
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
          expression_set_value( expr, expr->sig->value );
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
      case EXP_OP_IF             :
      case EXP_OP_FUNC_CALL      :
      case EXP_OP_WHILE          :
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
      case EXP_OP_NEDGE   :
      case EXP_OP_PEDGE   :
      case EXP_OP_CASE    :
      case EXP_OP_CASEX   :
      case EXP_OP_CASEZ   :
      case EXP_OP_DEFAULT :
      case EXP_OP_REPEAT  :
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, FALSE );
        }
        break;

      case EXP_OP_LAST    :
        if( (expr->value->width != 2) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 2, FALSE );
        }
        break;

      /*
       In the case of an AEDGE expression, it needs to have the size of its LAST child expression
       to be the width of its right child + one bit for an armed value.
      */
      case EXP_OP_AEDGE :
        if( (expr->left->value->width != expr->right->value->width) || (expr->left->value->value == NULL) ) {
          assert( expr->left->value->value == NULL );
          expression_create_value( expr->left, (expr->right->value->width + 1), FALSE );
        }
        if( (expr->value->width != 1) || (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, 1, FALSE );
        }
        break;

      /*
       In the case of an EXPAND, we need to set the width to be the product of the value of
       the left child and the bit-width of the right child.
      */
      case EXP_OP_EXPAND :
        expression_operate_recursively( expr->left, TRUE );
        if( (expr->value->width != (vector_to_int( expr->left->value ) * expr->right->value->width)) ||
            (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, (vector_to_int( expr->left->value ) * expr->right->value->width), FALSE );
        }
        break;

      /* 
       In the case of a MULTIPLY or LIST (for concatenation) operation, its expression width must be the sum of its
       children's width.  Remove the current vector and replace it with the appropriately
       sized vector.
      */
      case EXP_OP_MULTIPLY :
      case EXP_OP_LIST :
        if( (expr->value->width != (expr->left->value->width + expr->right->value->width)) ||
            (expr->value->value == NULL) ) {
          assert( expr->value->value == NULL );
          expression_create_value( expr, (expr->left->value->width + expr->right->value->width), FALSE );
        }
        break;

      default :
        if( (ESUPPL_IS_ROOT( expr->suppl ) == 1) ||
            (ESUPPL_IS_LHS( expr->suppl ) == 1) ||
            ((expr->parent->expr->op != EXP_OP_ASSIGN) &&
             (expr->parent->expr->op != EXP_OP_DASSIGN) &&
             (expr->parent->expr->op != EXP_OP_BASSIGN) &&
             (expr->parent->expr->op != EXP_OP_NASSIGN) &&
             (expr->parent->expr->op != EXP_OP_RASSIGN)) ) {
          if( (expr->left != NULL) && ((expr->right == NULL) || (expr->left->value->width > expr->right->value->width)) ) {
            largest_width = expr->left->value->width;
          } else if( expr->right != NULL ) {
            largest_width = expr->right->value->width;
          } else {
            largest_width = 1;
          }
          if( (expr->value->width != largest_width) || (expr->value->value == NULL) ) {
            assert( expr->value->value == NULL );
            expression_create_value( expr, largest_width, FALSE );
          }
        } else {
          if( (expr->parent->expr->left->value->width != expr->value->width) || (expr->value->value == NULL) ) {
            assert( expr->value->value == NULL );
            expression_create_value( expr, expr->parent->expr->left->value->width, FALSE );
          }
        }
        break;

    }

    /* Reapply original supplemental field now that expression has been resized */
    expr->value->suppl.all = old_vec_suppl;

  }

}

/*!
 \param expr        Pointer to expression to get ID from.
 \param parse_mode  Specifies if ulid (TRUE) or id (FALSE) should be used

 \return Returns expression ID for this expression.

 If specified expression is non-NULL, return expression ID of this
 expression; otherwise, return a value of 0 to indicate that this
 is a leaf node.
*/
int expression_get_id( expression* expr, bool parse_mode ) {

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
expression* expression_get_first_line_expr( expression* expr ) {

  expression* first = NULL;

  if( expr != NULL ) {

    first = expression_get_first_line_expr( expr->left );
    if( (first == NULL) || (first->line > expr->line) ) {
      first = expr;
    }

  }

  return( first );

}

/*!
 \param expr  Pointer to root expression to extract last line from

 \return Returns the line number of the last line in this expression. 
*/
expression* expression_get_last_line_expr( expression* expr ) {

  expression* last = NULL;

  if( expr != NULL ) {

    last = expression_get_last_line_expr( expr->right );
    if( (last == NULL) || (last->line < expr->line) ) {
      last = expr;
    }
      
  }

  return( last );

}

/*!
 \param expr  Pointer to expression tree to parse.
 \param head  Pointer to head of signal name list to populate.
 \param tail  Pointer to tail of signal name list to populate.

 Recursively parses specified expression list in search of RHS signals.
 When a signal name is found, it is added to the signal name list specified
 by head and tail.
*/
void expression_find_rhs_sigs( expression* expr, str_link** head, str_link** tail ) {

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
        str_link_add( sig_name, head, tail );
      } else {
        free_safe( sig_name );
      }

    }

    /* If this expression operation is neither a SIG or TRIGGER, keep searching tree */
    if( (expr->op != EXP_OP_SIG) && (expr->op != EXP_OP_TRIGGER) ) {

      expression_find_rhs_sigs( expr->right, head, tail );
      expression_find_rhs_sigs( expr->left,  head, tail );

    }

  }

}

/*!
 \param expr  Pointer to root expression to search under
 \param ulid  Underline ID to search for

 \return Returns a pointer to the found expression; otherwise, returns NULL if the expression
         could not be found.

 Recursively searches the given expression tree for the specified underline ID.  If the
 expression is found, a pointer to it is returned; otherwise, returns NULL.
*/
expression* expression_find_uline_id( expression* expr, int ulid ) {

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

  return( found_exp );

}

/*!
 \param exp  Pointer to expression to get root statement for.

 \return Returns a pointer to the root statement of the specified expression if one exists;
         otherwise, returns NULL.

 Recursively traverses up expression tree that contains exp until the root expression is found (if
 one exists).  If the root expression is found, return the pointer to the statement pointing to this
 root expression.  If the root expression was not found, return NULL.
*/
statement* expression_get_root_statement( expression* exp ) {

  if( exp == NULL ) {
    return( NULL );
  } else if( ESUPPL_IS_ROOT( exp->suppl ) == 1 ) {
    return( exp->parent->stmt );
  } else {
    return( expression_get_root_statement( exp->parent->expr ) );
  }

}

/*!
 \param root  Pointer to root of the expression tree to assign unique IDs for

 Recursively iterates down the specified expression tree assigning unique IDs to each expression.
*/
void expression_assign_expr_ids( expression* root ) {

  if( root != NULL ) {

    /* Traverse children first */
    expression_assign_expr_ids( root->left );
    expression_assign_expr_ids( root->right );

    /* Assign our expression a unique ID value */
    root->ulid = curr_expr_id;
    curr_expr_id++;

    /* Resize ourselves */
    expression_resize( root, FALSE );

  }

}

/*!
 \param expr        Pointer to expression to write to database file.
 \param file        Pointer to database file to write to.
 \param parse_mode  Set to TRUE when we are writing after just parsing the design (causes ulid value to be
                    output instead of id)

 This function recursively displays the expression information for the specified
 expression tree to the coverage database specified by file.
*/
void expression_db_write( expression* expr, FILE* file, bool parse_mode ) {

  func_unit* funit;  /* Pointer to functional unit containing the statement attached to this expression */

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
    vector_db_write( expr->value, file, (expr->op == EXP_OP_STATIC) );
  }

  if( expr->name != NULL ) {
    fprintf( file, " %s", expr->name );
  } else if( expr->sig != NULL ) {
    fprintf( file, " %s", expr->sig->name );  /* This will be valid for parameters */
  } else if( expr->stmt != NULL ) {
    fprintf( file, " %d", expression_get_id( expr->stmt->exp, parse_mode ) );
  }

  fprintf( file, "\n" );

}

/*!
 \param root   Pointer to the root expression to display
 \param ofile  Output file to write expression tree to

 Recursively iterates through the specified expression tree, outputting the expressions
 to the specified file.
*/
void expression_db_write_tree( expression* root, FILE* ofile ) {

  if( root != NULL ) {

    /* Print children first */
    expression_db_write_tree( root->left, ofile );
    expression_db_write_tree( root->right, ofile );

    /* Now write ourselves */
    expression_db_write( root, ofile, TRUE );

  }

}

/*!
 \param line        String containing database line to read information from.
 \param curr_funit  Pointer to current functional unit that instantiates this expression.
 \param eval        If TRUE, evaluate expression if children are static.

 \return Returns TRUE if parsing successful; otherwise, returns FALSE.

 Reads in the specified expression information, creates new expression from
 heap, populates the expression with specified information from file and 
 returns that value in the specified expression pointer.  If all is 
 successful, returns TRUE; otherwise, returns FALSE.
*/
bool expression_db_read( char** line, func_unit* curr_funit, bool eval ) {

  bool        retval = TRUE;  /* Return value for this function */
  int         id;             /* Holder of expression ID */
  expression* expr;           /* Pointer to newly created expression */
  int         linenum;        /* Holder of current line for this expression */
  int         column;         /* Holder of column alignment information */
  control     exec_num;       /* Holder of expression's execution number */
  control     op;             /* Holder of expression operation */
  esuppl      suppl;          /* Holder of supplemental value of this expression */
  int         right_id;       /* Holder of expression ID to the right */
  int         left_id;        /* Holder of expression ID to the left */
  expression* right;          /* Pointer to current expression's right expression */
  expression* left;           /* Pointer to current expression's left expression */
  int         chars_read;     /* Number of characters scanned in from line */
  vector*     vec;            /* Holders vector value of this expression */
  expression  texp;           /* Temporary expression link holder for searching */
  exp_link*   expl;           /* Pointer to found expression in functional unit */
  char        tmpname[1024];  /* Name of signal/functional unit that the current expression is bound to */
  int         tmpid;          /* ID of statement that the current expression is bound to */

  if( sscanf( *line, "%d %d %x %x %x %x %d %d%n", &id, &linenum, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 8 ) {

    *line = *line + chars_read;

    /* Find functional unit instance name */
    if( curr_funit == NULL ) {

      snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  expression (%d) in database written before its functional unit", id );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      retval = FALSE;

    } else {

      /* Find right expression */
      texp.id = right_id;
      if( right_id == 0 ) {
        right = NULL;
      } else if( (expl = exp_link_find( &texp, curr_funit->exp_head )) == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, right_id );
  	    print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      } else {
        right = expl->exp;
      }

      /* Find left expression */
      texp.id = left_id;
      if( left_id == 0 ) {
        left = NULL;
      } else if( (expl = exp_link_find( &texp, curr_funit->exp_head )) == NULL ) {
        snprintf( user_msg, USER_MSG_LENGTH, "Internal error:  root expression (%d) found before leaf expression (%d) in database file", id, left_id );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        exit( 1 );
      } else {
        left = expl->exp;
      }

      /* Create new expression */
      expr = expression_create( right, left, op, ESUPPL_IS_LHS( suppl ), id, linenum,
                                ((column >> 16) & 0xffff), (column & 0xffff), ESUPPL_OWNS_VEC( suppl ) );

      expr->suppl.all = suppl.all;
      expr->exec_num  = exec_num;

      if( right != NULL ) {
        right->parent->expr = expr;
      }

      /* Don't set left child's parent if the parent is a CASE, CASEX, or CASEZ type expression */
      if( (left != NULL) && 
          (op != EXP_OP_CASE)  &&
          (op != EXP_OP_CASEX) &&
          (op != EXP_OP_CASEZ) ) {
        left->parent->expr = expr;
      }

      if( ESUPPL_OWNS_VEC( suppl ) ) {

        /* Read in vector information */
        if( vector_db_read( &vec, line ) ) {

          /* Copy expression value */
          vector_dealloc( expr->value );
          expr->value = vec;

        } else {

          print_output( "Unable to read vector value for expression", FATAL, __FILE__, __LINE__ );
          retval = FALSE;
 
        }

      }

      /* Check to see if we are bound to a statement */
      if( sscanf( *line, "%d%n", &tmpid, &chars_read ) == 1 ) {
        *line = *line + chars_read;
        bind_add_stmt( tmpid, expr, curr_funit );

      /* Check to see if we are bound to a signal or functional unit */
      // } else if( sscanf( *line, "%s%n", tmpname, &chars_read ) == 1 ) {
      } else if( ((*line)[0] != '\n') && ((*line)[0] != '\0') ) {
        (*line)++;   /* Remove space */
        switch( op ) {
          case EXP_OP_FUNC_CALL :  bind_add( FUNIT_FUNCTION,    *line, expr, curr_funit );  break;
          case EXP_OP_TASK_CALL :  bind_add( FUNIT_TASK,        *line, expr, curr_funit );  break;
          case EXP_OP_NB_CALL   :  bind_add( FUNIT_NAMED_BLOCK, *line, expr, curr_funit );  break;
          case EXP_OP_DISABLE   :  bind_add( 1,                 *line, expr, curr_funit );  break;
          default               :  bind_add( 0,                 *line, expr, curr_funit );  break;
        }
      }

      /* If we are an assignment operator, set our vector value to that of the right child */
      if( (op == EXP_OP_ASSIGN)  ||
          (op == EXP_OP_DASSIGN) ||
          (op == EXP_OP_BASSIGN) ||
          (op == EXP_OP_RASSIGN) ||
          (op == EXP_OP_NASSIGN) ||
          (op == EXP_OP_IF)      ||
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
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Expression to merge data into.
 \param line  Pointer to CDD line to parse.
 \param same  Specifies if expression to be merged needs to be exactly the same as the existing expression.

 \return Returns TRUE if parse and merge was sucessful; otherwise, returns FALSE.

 Parses specified line for expression information and merges contents into the
 base expression.  If the two expressions given are not the same (IDs, op,
 and/or line position differ) we know that the database files being merged 
 were not created from the same design; therefore, display an error message 
 to the user in this case.  If both expressions are the same, perform the 
 merge.
*/
bool expression_db_merge( expression* base, char** line, bool same ) {

  bool    retval = TRUE;  /* Return value for this function */
  int     id;             /* Expression ID field */
  int     linenum;        /* Expression line number */
  int     column;         /* Column information */
  control exec_num;       /* Execution number */
  control op;             /* Expression operation */
  esuppl  suppl;          /* Supplemental field */
  int     right_id;       /* ID of right child */
  int     left_id;        /* ID of left child */
  int     chars_read;     /* Number of characters read */

  assert( base != NULL );

  if( sscanf( *line, "%d %d %x %x %x %x %d %d%n", &id, &linenum, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 8 ) {

    *line = *line + chars_read;

    if( (base->op != op) || (base->line != linenum) || (base->col != column) ) {

      print_output( "Attempting to merge databases derived from different designs.  Unable to merge",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      /* Merge expression supplemental fields */
      base->suppl.all = (base->suppl.all & ESUPPL_MERGE_MASK) | (suppl.all & ESUPPL_MERGE_MASK);

      /* Merge execution number information */
      if( base->exec_num < exec_num ) {
        base->exec_num = exec_num;
      }

      if( ESUPPL_OWNS_VEC( suppl ) ) {

        /* Merge expression vectors */
        retval = vector_db_merge( base->value, line, same );

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param base  Expression to replace data with.
 \param line  Pointer to CDD line to parse.

 \return Returns TRUE if parse and merge was sucessful; otherwise, returns FALSE.

 Parses specified line for expression information and replaces contents of the
 original expression with the contents of the newly read expression.  If the two
 expressions given are not the same (IDs, op, and/or line position differ) we
 know that the database files being merged were not created from the same design;
 therefore, display an error message to the user in this case.  If both expressions
 are the same, perform the replacement.
*/
bool expression_db_replace( expression* base, char** line ) {

  bool    retval = TRUE;  /* Return value for this function */
  int     id;             /* Expression ID field */
  int     linenum;        /* Expression line number */
  int     column;         /* Column location information */
  control exec_num;       /* Execution count */
  control op;             /* Expression operation */
  esuppl  suppl;          /* Supplemental field */
  int     right_id;       /* ID of right child */
  int     left_id;        /* ID of left child */
  int     chars_read;     /* Number of characters read */

  assert( base != NULL );

  if( sscanf( *line, "%d %d %x %x %x %x %d %d%n", &id, &linenum, &column, &exec_num, &op, &(suppl.all), &right_id, &left_id, &chars_read ) == 8 ) {

    *line = *line + chars_read;

    if( (base->op != op) || (base->line != linenum) || (base->col != column) ) {

      print_output( "Attempting to replace a database derived from a different design.  Unable to replace",
                    FATAL, __FILE__, __LINE__ );
      exit( 1 );

    } else {

      /* Replace expression supplemental fields */
      base->suppl.all = suppl.all;

      /* Replace execution count value */
      base->exec_num  = exec_num;

      if( ESUPPL_OWNS_VEC( suppl ) ) {

        /* Merge expression vectors */
        retval = vector_db_replace( base->value, line );

      }

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param op  Expression operation to get string representation of

 \return Returns a non-writable string that contains the user-readable name of the
         specified expression operation.
*/
const char* expression_string_op( int op ) {

  assert( (op >= 0) && (op < EXP_OP_NUM) );

  return( exp_op_info[op].name );

}

/*!
 \param exp  Pointer to expression to display

 Returns a pointer to user_msg that will contain a user-friendly string version of
 the given expression
*/
char* expression_string( expression* exp ) {

  snprintf( user_msg, USER_MSG_LENGTH, "%d (%s line %d)", exp->id, expression_string_op( exp->op ), exp->line );

  return( user_msg );

}

/*!
 \param expr  Pointer to expression to display.

 Displays contents of the specified expression to standard output.  This function
 is called by the funit_display function.
*/
void expression_display( expression* expr ) {

  int right_id;  /* Value of right expression ID */
  int left_id;   /* Value of left expression ID  */
  char op[20];   /* String representation of expression operation */    

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

  printf( "  Expression =>  id: %d, op: %s, line: %d, col: %x, suppl: %x, exec_num: %d, left: %d, right: %d, ", 
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
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs XOR operation.
*/
bool expression_op_func__xor( expression* expr, thread* thr ) {

  return( vector_bitwise_op( expr->value, expr->left->value, expr->right->value, xor_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a multiply operation.
*/
bool expression_op_func__multiply( expression* expr, thread* thr ) {

  return( vector_op_multiply( expr->value, expr->left->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a 32-bit divide operation.
*/
bool expression_op_func__divide( expression* expr, thread* thr ) {

  vector   vec1;            /* Temporary vector */
  vec_data bit;             /* Holds the value of a single bit in a vector value */
  vec_data value32[32];     /* Holds the value of a 32-bit vector value */
  int      intval1;         /* Integer holder */
  int      intval2;         /* Integer holder */
  int      i;               /* Loop iterator */
  bool     retval = FALSE;  /* Return value for this function */

  if( vector_is_unknown( expr->left->value ) || vector_is_unknown( expr->right->value ) ) {

    bit.all        = 0;
    bit.part.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  } else {

    vector_init( &vec1, value32, 32 );
    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    if( intval2 == 0 ) {
      print_output( "Division by 0 error", FATAL, __FILE__, __LINE__ );
      exit( 1 );
    }

    intval1 = intval1 / intval2;
    vector_from_int( &vec1, intval1 );
    retval = vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );

  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a 32-bit modulus operation.
*/
bool expression_op_func__mod( expression* expr, thread* thr ) {

  vector   vec1;            /* Temporary vector */
  vec_data bit;             /* Holds the value of a single bit in a vector value */
  vec_data value32[32];     /* Holds the value of a 32-bit vector value */
  int      intval1;         /* Integer holder */
  int      intval2;         /* Integer holder */
  int      i;               /* Loop iterator */
  bool     retval = FALSE;  /* Return value for this function */

  if( vector_is_unknown( expr->left->value ) || vector_is_unknown( expr->right->value ) ) {

    bit.all        = 0;
    bit.part.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  } else {

    vector_init( &vec1, value32, 32 );
    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    if( intval2 == 0 ) {
      print_output( "Division by 0 error", FATAL, __FILE__, __LINE__ );
      exit( 1 );
    }

    intval1 = intval1 % intval2;
    vector_from_int( &vec1, intval1 );
    retval = vector_set_value( expr->value, vec1.value, expr->value->width, 0, 0 );

  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an addition operation.
*/
bool expression_op_func__add( expression* expr, thread* thr ) {

  return( vector_op_add( expr->value, expr->left->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a subtraction operation.
*/
bool expression_op_func__subtract( expression* expr, thread* thr ) {

  return( vector_op_subtract( expr->value, expr->left->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise AND operation.
*/
bool expression_op_func__and( expression* expr, thread* thr ) {

  return( vector_bitwise_op( expr->value, expr->left->value, expr->right->value, and_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise OR operation.
*/
bool expression_op_func__or( expression* expr, thread* thr ) {

  return( vector_bitwise_op( expr->value, expr->left->value, expr->right->value, or_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NAND operation.
*/
bool expression_op_func__nand( expression* expr, thread* thr ) {

  return( vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nand_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NOR operation.
*/
bool expression_op_func__nor( expression* expr, thread* thr ) {

  return( vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nor_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a bitwise NXOR operation.
*/
bool expression_op_func__nxor( expression* expr, thread* thr ) {

  return( vector_bitwise_op( expr->value, expr->left->value, expr->right->value, nxor_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a less-than comparison operation.
*/
bool expression_op_func__lt( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LT ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a greater-than comparison operation.
*/
bool expression_op_func__gt( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GT ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a left shift operation.
*/
bool expression_op_func__lshift( expression* expr, thread* thr ) {

  return( vector_op_lshift( expr->value, expr->left->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a right shift operation.
*/
bool expression_op_func__rshift( expression* expr, thread* thr ) {

  return( vector_op_rshift( expr->value, expr->left->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an arithmetic right shift operation.
*/
bool expression_op_func__arshift( expression* expr, thread* thr ) {

  return( vector_op_arshift( expr->value, expr->left->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an equality (==) comparison operation.
*/
bool expression_op_func__eq( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_EQ ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an equality (===) comparison operation.
*/
bool expression_op_func__ceq( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a less-than-or-equal comparison operation.
*/
bool expression_op_func__le( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LE ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a greater-than-or-equal comparison operation.
*/
bool expression_op_func__ge( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_GE ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a not-equal (!=) comparison operation.
*/
bool expression_op_func__ne( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_NE ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a not-equal (!==) comparison operation.
*/
bool expression_op_func__cne( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CNE ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a logical OR operation.
*/
bool expression_op_func__lor( expression* expr, thread* thr ) {

  vector   vec1;     /* Used for logical reduction */
  vector   vec2;     /* Used for logical reduction */
  vec_data value1a;  /* 1-bit nibble value */
  vec_data value1b;  /* 1-bit nibble value */

  vector_init( &vec1, &value1a, 1 );
  vector_init( &vec2, &value1b, 1 );

  vector_unary_op( &vec1, expr->left->value,  or_optab );
  vector_unary_op( &vec2, expr->right->value, or_optab );

  return( vector_bitwise_op( expr->value, &vec1, &vec2, or_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a logical AND operation.
*/
bool expression_op_func__land( expression* expr, thread* thr ) {

  vector   vec1;     /* Used for logical reduction */
  vector   vec2;     /* Used for logical reduction */
  vec_data value1a;  /* 1-bit nibble value */
  vec_data value1b;  /* 1-bit nibble value */

  vector_init( &vec1, &value1a, 1 );
  vector_init( &vec2, &value1b, 1 );

  vector_unary_op( &vec1, expr->left->value,  or_optab );
  vector_unary_op( &vec2, expr->right->value, or_optab );

  return( vector_bitwise_op( expr->value, &vec1, &vec2, and_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a conditional (?:) operation.
*/
bool expression_op_func__cond( expression* expr, thread* thr ) {

  /* Simple vector copy from right side */
  return( vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a conditional select (?:) operation.
*/
bool expression_op_func__cond_sel( expression* expr, thread* thr ) {

  vector   vec1;            /* Used for logical reduction */
  vec_data bit1;            /* 1-bit vector value */
  vec_data bitx;            /* 1-bit X vector value */
  int      i;               /* Loop iterator */
  bool     retval = FALSE;  /* Return value for this function */

  vector_init( &vec1, &bit1, 1 );
  vector_unary_op( &vec1, expr->parent->expr->left->value, or_optab );

  if( vec1.value[0].part.value == 0 ) {
    retval = vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
  } else if( vec1.value[0].part.value == 1 ) {
    retval = vector_set_value( expr->value, expr->left->value->value, expr->left->value->width, 0, 0 );
  } else {
    bitx.all        = 0; 
    bitx.part.value = 2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bitx, 1, 0, i );
    }
  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary invert operation.
*/
bool expression_op_func__uinv( expression* expr, thread* thr ) {

  return( vector_unary_inv( expr->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary AND operation.
*/
bool expression_op_func__uand( expression* expr, thread* thr ) {

  return( vector_unary_op( expr->value, expr->right->value, and_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NOT operation.
*/
bool expression_op_func__unot( expression* expr, thread* thr ) {

  return( vector_unary_not( expr->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary OR operation.
*/
bool expression_op_func__uor( expression* expr, thread* thr ) {

  return( vector_unary_op( expr->value, expr->right->value, or_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary XOR operation.
*/
bool expression_op_func__uxor( expression* expr, thread* thr ) {

  return( vector_unary_op( expr->value, expr->right->value, xor_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NAND operation.
*/
bool expression_op_func__unand( expression* expr, thread* thr ) {

  return( vector_unary_op( expr->value, expr->right->value, nand_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NOR operation.
*/
bool expression_op_func__unor( expression* expr, thread* thr ) {

  return( vector_unary_op( expr->value, expr->right->value, nor_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a unary NXOR operation.
*/
bool expression_op_func__unxor( expression* expr, thread* thr ) {

  return( vector_unary_op( expr->value, expr->right->value, nxor_optab ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 No operation is performed -- expression value is assumed to be changed.
*/
bool expression_op_func__null( expression* expr, thread* thr ) {

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a single bit select operation.
*/
bool expression_op_func__sbit( expression* expr, thread* thr ) {

  int intval;  /* Integer value */

  if( !vector_is_unknown( expr->left->value ) ) {

    intval = vector_to_int( expr->left->value ) - expr->sig->lsb;
    assert( intval >= 0 );
    assert( intval < expr->sig->value->width );

    if( expr->sig->suppl.part.big_endian == 1 ) {
      expr->value->value = expr->sig->value->value + ((expr->sig->value->width - intval) - 1);
    } else {
      expr->value->value = expr->sig->value->value + intval;
    }

  }

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a expansion ({{}}) operation.
*/
bool expression_op_func__expand( expression* expr, thread* thr ) {

  vec_data bit;             /* 1-bit vector value */
  int      i;               /* Loop iterator */
  int      j;               /* Loop iterator */
  int      width;           /* Width to expand this operation to */
  bool     retval = FALSE;  /* Return value for this function */

  bit.all = 0;

  if( vector_is_unknown( expr->left->value ) ) {

    bit.part.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  } else {

    for( j=0; j<expr->right->value->width; j++ ) {
      bit.part.value = expr->right->value->value[j].part.value;
      width          = vector_to_int( expr->left->value );
      for( i=0; i<width; i++ ) {
        retval |= vector_set_value( expr->value, &bit, 1, 0, ((j * expr->right->value->width) + i) );
      }
    }

  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a list (,) operation.
*/
bool expression_op_func__list( expression* expr, thread* thr ) {

  bool retval = FALSE;  /* Return value for this function */

  retval |= vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
  retval |= vector_set_value( expr->value, expr->left->value->value,  expr->left->value->width,  0, expr->right->value->width );

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a concatenation ({}) operation.
*/
bool expression_op_func__concat( expression* expr, thread* thr ) {

  return( vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a positive edge event operation.
*/
bool expression_op_func__pedge( expression* expr, thread* thr ) {

  bool              retval;   /* Return value for this function */
  register vec_data value1a;  /* 1-bit vector value */
  register vec_data value1b;  /* 2-bit vector value */

  value1a.all        = 0;
  value1a.part.value = expr->right->value->value[0].part.value;
  value1b.all        = expr->left->value->value[0].all;

  if( (value1a.part.value != value1b.part.value) && (value1a.part.value == 1) && thr->exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  /* Set left LAST value to current value of right */
  expr->left->value->value[0].all = value1a.all;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negative-edge event operation.
*/
bool expression_op_func__nedge( expression* expr, thread* thr ) {

  bool              retval;   /* Return value for this function */
  register vec_data value1a;  /* 1-bit vector value */
  register vec_data value1b;  /* 2-bit vector value */

  value1a.all        = 0;
  value1a.part.value = expr->right->value->value[0].part.value;
  value1b.all        = expr->left->value->value[0].all;

  if( (value1a.part.value != value1b.part.value) && (value1a.part.value == 0) && thr->exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  /* Set left LAST value to current value of right */
  expr->left->value->value[0].all = value1a.all;

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an any-edge event operation.
*/
bool expression_op_func__aedge( expression* expr, thread* thr ) {

  vector   vec;     /* Temporary vector */
  vec_data bit;     /* 1-bit vector value */
  bool     retval;  /* Return value of this function */

  vector_init( &vec, &bit, 1 );
  vector_op_compare( &vec, expr->left->value, expr->right->value, COMP_CEQ );

  /* If the last value and the current value are NOT equal, we have a fired event */
  if( (bit.part.value == 0) && thr->exec_first ) {
    expr->suppl.part.true   = 1;
    expr->suppl.part.eval_t = 1;
    retval = TRUE;
    vector_set_value_only( expr->left->value, expr->right->value->value, expr->right->value->width, 0, 0 );
  } else {
    expr->suppl.part.eval_t = 0;
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a event OR operation.
*/
bool expression_op_func__eor( expression* expr, thread* thr ) {

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

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a sensitivity list operation.
*/
bool expression_op_func__slist( expression* expr, thread* thr ) {

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

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a delay operation.
*/
bool expression_op_func__delay( expression* expr, thread* thr ) {

  bool retval;   /* Return value for this function */
  int  intval1;  /* Integer value holder */
  int  intval2;  /* Integer value holder */

  expr->suppl.part.eval_t = 0;

  /* If this expression is not currently waiting, set the start time of delay */
  if( vector_to_int( expr->left->value ) == 0xffffffff ) {
    vector_from_int( expr->left->value, curr_sim_time );
  }

  intval1 = vector_to_int( expr->left->value );           /* Start time of delay */
  intval2 = vector_to_int( expr->right->value );          /* Number of clocks to delay */

  if( ((intval1 + intval2) <= curr_sim_time) || ((curr_sim_time == -1) && (intval1 != 0xffffffff)) ) {
    expr->suppl.part.eval_t = 1;
    expr->suppl.part.true   = 1;
    vector_from_int( expr->left->value, 0xffffffff );
    retval = TRUE;
  } else {
    retval = FALSE;
  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an event trigger (->) operation.
*/
bool expression_op_func__trigger( expression* expr, thread* thr ) {

  if( expr->value->value[0].part.value == 1 ) {
    expr->value->value[0].part.value = 0;
  } else {
    expr->value->value[0].part.value = 1;
  }

  /* Propagate event */
  vsignal_propagate( expr->sig );

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a case comparison operation.
*/
bool expression_op_func__case( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CEQ ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a casex comparison operation.
*/
bool expression_op_func__casex( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CXEQ ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a casez comparison operation.
*/
bool expression_op_func__casez( expression* expr, thread* thr ) {

  return( vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_CZEQ ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a case default comparison operation.
*/
bool expression_op_func__default( expression* expr, thread* thr ) {

  vec_data bit;  /* 1-bit vector value */

  bit.all        = 0;
  bit.part.value = 1;

  return( vector_set_value( expr->value, &bit, 1, 0, 0 ) );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a blocking assignment operation.
*/
bool expression_op_func__bassign( expression* expr, thread* thr ) {

  int intval = 0;  /* Integer value */

  expression_assign( expr->left, expr->right, &intval );

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a function call operation.
*/
bool expression_op_func__func_call( expression* expr, thread* thr ) {

  sim_thread( sim_add_thread( thr, expr->stmt ) );

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a task call operation.
*/
bool expression_op_func__task_call( expression* expr, thread* thr ) {

  bool retval = FALSE;  /* Return value for this function */

  if( expr->value->value[0].part.misc == 0 ) {

    sim_add_thread( thr, expr->stmt );
    expr->value->value[0].part.misc  = 1;
    expr->value->value[0].part.value = 0;

  } else if( thr->child_head == NULL ) {

    expr->value->value[0].part.misc  = 0;
    expr->value->value[0].part.value = 1;
    retval = TRUE;

  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a named block call operation.
*/
bool expression_op_func__nb_call( expression* expr, thread* thr ) {

  bool retval = FALSE;  /* Return value for this function */

  if( ESUPPL_IS_IN_FUNC( expr->suppl ) ) {

    sim_thread( sim_add_thread( thr, expr->stmt ) );
    retval = TRUE;

  } else {

    if( expr->value->value[0].part.misc == 0 ) {
      sim_add_thread( thr, expr->stmt );
      expr->value->value[0].part.misc  = 1;
      expr->value->value[0].part.value = 0;
    } else if( thr->child_head == NULL ) {
      expr->value->value[0].part.misc  = 0;
      expr->value->value[0].part.value = 1;
      retval = TRUE;
    }

  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a fork operation.
*/
bool expression_op_func__fork( expression* expr, thread* thr ) {

  sim_add_thread( thr, expr->stmt );

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a join operation.
*/
bool expression_op_func__join( expression* expr, thread* thr ) {

  return( thr->child_head == NULL );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a block disable operation.
*/
bool expression_op_func__disable( expression* expr, thread* thr ) {

  sim_kill_thread_with_stmt( expr->stmt );

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a repeat loop operation.
*/
bool expression_op_func__repeat( expression* expr, thread* thr ) {

  bool retval;  /* Return value for this function */

  retval = vector_op_compare( expr->value, expr->left->value, expr->right->value, COMP_LT );

  if( expr->value->value[0].part.value == 0 ) {
    vector_from_int( expr->left->value, 0 );
  } else {
    vector_from_int( expr->left->value, (vector_to_int( expr->left->value ) + 1) );
  }

  return( retval );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs an exponential power operation.
*/
bool expression_op_func__exponent( expression* expr, thread* thr ) {

  bool     retval  = FALSE;  /* Return value for this function */
  vec_data bit;              /* 1-bit vector value */
  vec_data value32[32];      /* 32-bit vector value */
  vector   vec;              /* Temporary vector value */
  int      intval1;          /* Integer value */
  int      intval2;          /* Integer value */
  int      intval3 = 1;      /* Integer value */
  int      i;                /* Loop iterator */

  if( vector_is_unknown( expr->left->value ) || vector_is_unknown( expr->right->value ) ) {

    bit.all        = 0;
    bit.part.value = 0x2;
    for( i=0; i<expr->value->width; i++ ) {
      retval |= vector_set_value( expr->value, &bit, 1, 0, i );
    }

  } else {

    vector_init( &vec, value32, 32 );
    intval1 = vector_to_int( expr->left->value );
    intval2 = vector_to_int( expr->right->value );

    for( i=0; i<intval2; i++ ) {
      intval3 *= intval1;
    }

    vector_from_int( &vec, intval3 );
    retval = vector_set_value( expr->value, vec.value, expr->value->width, 0, 0 );

  }

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a port assignment operation.
*/
bool expression_op_func__passign( expression* expr, thread* thr ) {

  int intval = 0;  /* Integer value */

  switch( expr->sig->suppl.part.type ) {

    /* If the connected signal is an input type, copy the parameter expression value to this vector */
    case SSUPPL_TYPE_INPUT :
      vector_set_value( expr->value, expr->right->value->value, expr->right->value->width, 0, 0 );
      vsignal_propagate( expr->sig );
      break;

    /*
     If the connected signal is an output type, do an expression assign from our expression value
     to the right expression.
    */
    case SSUPPL_TYPE_OUTPUT :
      expression_assign( expr->right, expr, &intval );
      break;

    /* We don't currently support INOUT as these are not used in tasks or functions */
    default :
      assert( (expr->sig->suppl.part.type == SSUPPL_TYPE_INPUT) ||
              (expr->sig->suppl.part.type == SSUPPL_TYPE_OUTPUT) );
      break;

  }

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a positive variable multi-bit select operation.
*/
bool expression_op_func__mbit_pos( expression* expr, thread* thr ) {

  int intval;  /* Integer value */

  if( !vector_is_unknown( expr->left->value ) ) {

    intval = vector_to_int( expr->left->value ) - expr->sig->lsb;
    assert( intval >= 0 );
    assert( intval < expr->sig->value->width );
    expr->value->value = expr->sig->value->value + intval;

  }

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negative variable multi-bit select operation.
*/
bool expression_op_func__mbit_neg( expression* expr, thread* thr ) {

  int intval1;  /* Integer value */
  int intval2;  /* Integer value */

  if( !vector_is_unknown( expr->left->value ) ) {

    intval1 = vector_to_int( expr->left->value ) - expr->sig->lsb;
    intval2 = vector_to_int( expr->right->value );
    assert( intval1 < expr->sig->value->width );
    assert( ((intval1 - intval2) + 1) >= 0 );
    expr->value->value = expr->sig->value->value + ((intval1 - intval2) + 1);

  }

  return( TRUE );

}

/*!
 \param expr  Pointer to expression to perform operation on
 \param thr   Pointer to thread containing this expression

 \return Returns TRUE if the expression has changed value from its previous value; otherwise, returns FALSE.

 Performs a negate of the specified expression.
*/
bool expression_op_func__negate( expression* expr, thread* thr ) {

  return( vector_op_negate( expr->value, expr->right->value ) );

}

/*!
 \param expr  Pointer to expression to set value to.
 \param thr   Pointer to current thread being simulated. 

 \return Returns TRUE if the assigned expression value was different than the previously stored value;
         otherwise, returns FALSE.

 Performs expression operation.  This function must only be run after its
 left and right expressions have been calculated during this clock period.
 Sets the value of the operation in its own vector value and updates the
 suppl nibble as necessary.
*/
bool expression_operate( expression* expr, thread* thr ) {

  bool     retval = TRUE;   /* Return value for this function */
  vector   vec;             /* Used for logical reduction */ 
  vec_data bit;             /* Bit holder for some ops */
  control  lf, lt, rf, rt;  /* Specify left and right WAS_TRUE/WAS_FALSE values */

  if( expr != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "      In expression_operate, id: %d, op: %s, line: %d, addr: %p",
              expr->id, expression_string_op( expr->op ), expr->line, expr );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    assert( expr->value != NULL );

    /* Call expression operation */
    retval = exp_op_info[expr->op].func( expr, thr );

    /* If we have a new value, recalculate TRUE/FALSE indicators */
    if( EXPR_IS_EVENT( expr ) == 0 ) {

      if( retval ) {

        /* Clear current TRUE/FALSE indicators */
        if( (expr->op != EXP_OP_STATIC) && (expr->op != EXP_OP_PARAM ) ) {
          expr->suppl.part.eval_t = 0;
          expr->suppl.part.eval_f = 0;
        }
      
        /* Set TRUE/FALSE bits to indicate value */
        vector_init( &vec, &bit, 1 );
        vector_unary_op( &vec, expr->value, or_optab );
        switch( vec.value[0].part.value ) {
          case 0 :  expr->suppl.part.false = 1;  expr->suppl.part.eval_f = 1;  break;
          case 1 :  expr->suppl.part.true  = 1;  expr->suppl.part.eval_t = 1;  break;
          default:  break;
        }

      }

      /* Set EVAL00, EVAL01, EVAL10 or EVAL11 bits based on current value of children */
      if( (expr->left != NULL) && (expr->right != NULL) ) {
        lf = ESUPPL_IS_FALSE( expr->left->suppl  );
        lt = ESUPPL_IS_TRUE(  expr->left->suppl  );
        rf = ESUPPL_IS_FALSE( expr->right->suppl );
        rt = ESUPPL_IS_TRUE(  expr->right->suppl );
        expr->suppl.part.eval_00 |= lf & rf;
        expr->suppl.part.eval_01 |= lf & rt;
        expr->suppl.part.eval_10 |= lt & rf;
        expr->suppl.part.eval_11 |= lt & rt;
      }

      /* If this expression is attached to an FSM, perform the FSM calculation now */
      if( expr->table != NULL ) {
        fsm_table_set( expr->table );
        /* If from_state was not specified, we need to copy the current contents of to_state to from_state */
        if( expr->table->from_state->id == expr->id ) {
          vector_dealloc( expr->table->from_state->value );
          vector_copy( expr->value, &(expr->table->from_state->value) );
        }
      }

    }

  }

  /* Specify that we have executed this expression */
  (expr->exec_num)++;

  return( retval );

}

/*!
 \param expr    Pointer to top of expression tree to perform recursive operations.
 \param sizing  Set to TRUE if we are evaluating for purposes of sizing.
 
 Recursively performs the proper operations to cause the top-level expression to
 be set to a value.  This function is called during the parse stage to derive 
 pre-CDD widths of multi-bit expressions.  Each MSB/LSB is an expression tree that 
 needs to be evaluated to set the width properly on the MBIT_SEL expression.
*/
void expression_operate_recursively( expression* expr, bool sizing ) {
    
  if( expr != NULL ) {
    
    /* Evaluate children */
    expression_operate_recursively( expr->left,  sizing );
    expression_operate_recursively( expr->right, sizing );
    
    if( sizing ) {

      /*
       Non-static expression found where static expression required.  Simulator
       should catch this error before us, so no user error (too much work to find
       expression in functional unit expression list for now.
      */
      assert( (expr->op != EXP_OP_SIG)      &&
              (expr->op != EXP_OP_SBIT_SEL) &&
              (expr->op != EXP_OP_MBIT_SEL) &&
              (expr->op != EXP_OP_MBIT_POS) &&
              (expr->op != EXP_OP_MBIT_NEG) );

      /* Resize current expression only */
      expression_resize( expr, FALSE );
    
      /* Create vector value to store operation information */
      if( expr->value->value == NULL ) {
        expression_create_value( expr, expr->value->width, TRUE );
      }

    }
    
    /* Perform operation */
    expression_operate( expr, NULL );

    if( sizing ) {

      /* Clear out the execution number value since we aren't really simulating this */
      expr->exec_num = 0;

    }
    
  }
  
}

/*!
 \param expr  Pointer to expression to evaluate.
 
 \return Returns TRUE if expression contains only static expressions; otherwise, returns FALSE.
 
 Recursively iterates through specified expression tree and returns TRUE if all of
 the leaf expressions are static expressions (STATIC or parameters).
*/
bool expression_is_static_only( expression* expr ) {

  if( expr != NULL ) {

    if( (EXPR_IS_STATIC( expr ) == 1) ||
        (ESUPPL_IS_LHS( expr->suppl ) == 1) ||
        ((expr->op == EXP_OP_SIG) && (expr->sig != NULL) && (expr->sig->suppl.part.type == SSUPPL_TYPE_PARAM)) ) {
      return( TRUE );
    } else {
      return( (expr->op != EXP_OP_MBIT_SEL)           &&
              (expr->op != EXP_OP_SBIT_SEL)           &&
              (expr->op != EXP_OP_SIG)                &&
              (expr->op != EXP_OP_FUNC_CALL)          &&
              expression_is_static_only( expr->left ) &&
              expression_is_static_only( expr->right ) );
    }

  } else {

    return( TRUE );

  }

}

/*! \brief Returns TRUE if specified expression is on the LHS of a blocking assignment operator. */
bool expression_is_assigned( expression* expr ) {

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

  return( retval );

}

/*!
 \param expr  Pointer to expression to check

 \return Returns TRUE if the specifies expression belongs in a single or mult-bit select expression
*/
bool expression_is_bit_select( expression* expr ) {

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

  return( retval );

}

/*!
 \param expr  Pointer to current expression to evaluate

 Checks to see if the expression is in the LHS of a BASSIGN expression tree.  If it is, it sets the
 assigned supplemental field of the expression's signal vector to indicate the the value of this
 signal will come from Covered instead of the dumpfile.  This is called in the bind_signal function
 after the expression and signal have been bound (only in parsing stage).
*/
void expression_set_assigned( expression* expr ) {

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
      expr->sig->value->suppl.part.assigned = 1;
    }

  }

}

/*!
 \param lhs  Pointer to current expression on left-hand-side of assignment to calculate for.
 \param rhs  Pointer to the right-hand-expression that will be assigned from.
 \param lsb  Current least-significant bit in rhs value to start assigning.

 Recursively iterates through specified LHS expression, assigning the value from the RHS expression.
 This is called whenever a blocking assignment expression is found during simulation.
*/
void expression_assign( expression* lhs, expression* rhs, int* lsb ) {

  int intval1;  /* Integer value to use */
  int intval2;  /* Integer value to use */

  if( lhs != NULL ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "        In expression_assign, lhs_op: %s, rhs_op: %s, lsb: %d",
              expression_string_op( lhs->op ), expression_string_op( rhs->op ), *lsb );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    switch( lhs->op ) {
      case EXP_OP_SIG      :
        if( lhs->sig->value->suppl.part.assigned == 1 ) {
          vector_set_value( lhs->value, rhs->value->value, rhs->value->width, *lsb, 0 );
          if( rhs->value->width < lhs->value->width ) {
            vector_bit_fill( lhs->value, lhs->value->width, (rhs->value->width + *lsb) );
          }
  	  vsignal_propagate( lhs->sig );
#ifdef DEBUG_MODE
  	  if( debug_mode ) {
            printf( "        " );  vsignal_display( lhs->sig );
          }
#endif
        }
        *lsb = *lsb + lhs->value->width;
        break;
      case EXP_OP_SBIT_SEL :
        if( lhs->sig->value->suppl.part.assigned == 1 ) {
          if( !vector_is_unknown( lhs->left->value ) ) {
            intval1 = vector_to_int( lhs->left->value ) - lhs->sig->lsb;
            assert( intval1 >= 0 );
            assert( intval1 < lhs->sig->value->width );
            if( lhs->sig->suppl.part.big_endian == 1 ) {
              lhs->value->value = lhs->sig->value->value + ((lhs->sig->value->width - intval1) - 1);
            } else {
              lhs->value->value = lhs->sig->value->value + intval1;
            }
          }
          vector_set_value( lhs->value, rhs->value->value, 1, *lsb, 0 );
	  vsignal_propagate( lhs->sig );
#ifdef DEBUG_MODE
          if( debug_mode ) {
            printf( "        " );  vsignal_display( lhs->sig );
          }
#endif
        }
        *lsb = *lsb + lhs->value->width;
        break;
      case EXP_OP_MBIT_SEL :
        if( lhs->sig->value->suppl.part.assigned == 1 ) {
          vector_set_value( lhs->value, rhs->value->value, rhs->value->width, *lsb, 0 );
          if( rhs->value->width < lhs->value->width ) {
            vector_bit_fill( lhs->value, lhs->value->width, (rhs->value->width + *lsb) );
          }
  	  vsignal_propagate( lhs->sig );
#ifdef DEBUG_MODE
          if( debug_mode ) {
            printf( "        " );  vsignal_display( lhs->sig );
          }
#endif
        }
        *lsb = *lsb + lhs->value->width;
        break;
#ifdef NOT_SUPPORTED
      case EXP_OP_MBIT_POS :
        if( lhs->sig->value->suppl.part.assigned == 1 ) {
          if( !vector_is_unknown( lhs->left->value ) ) {
            intval1 = vector_to_int( lhs->left->value ) - lhs->sig->lsb;
            intval2 = vector_to_int( lhs->right->value );
            assert( intval1 >= 0 );
            assert( ((intval1 + intval2) - 1) < lhs->sig->value->width );
            lhs->value->value = lhs->sig->value->value + intval1;
          }
          vector_set_value( lhs->value, rhs->value->value, intval2, *lsb, 0 );
          vsignal_propagate( lhs->sig );
#ifdef DEBUG_MODE
          if( debug_mode ) {
            printf( "        " );  vsignal_display( lhs->sig );
          }
#endif
        }
        *lsb = *lsb + lhs->value->width;
        break;
      case EXP_OP_MBIT_NEG :
        if( lhs->sig->value->suppl.part.assigned == 1 ) {
          if( !vector_is_unknown( lhs->left->value ) ) {
            intval1 = vector_to_int( lhs->left->value ) - lhs->sig->lsb;
            intval2 = vector_to_int( lhs->right->value );
            assert( intval1 < lhs->sig->value->width );
            assert( ((intval1 - intval2) + 1) >= 0 );
            lhs->value->value = lhs->sig->value->value + ((intval1 - intval2) + 1);
          }
          vector_set_value( lhs->value, rhs->value->value, intval2, *lsb, 0 );
          vsignal_propagate( lhs->sig );
#ifdef DEBUG_MODE
          if( debug_mode ) {
            printf( "        " );  vsignal_display( lhs->sig );
          }
#endif
        }
        *lsb = *lsb + lhs->value->width;
        break;
#endif
      case EXP_OP_CONCAT   :
      case EXP_OP_LIST     :
        expression_assign( lhs->right, rhs, lsb );
        expression_assign( lhs->left,  rhs, lsb );
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
                (lhs->op == EXP_OP_LIST) );
#else
	assert( (lhs->op == EXP_OP_SIG)      ||
	        (lhs->op == EXP_OP_SBIT_SEL) ||
		(lhs->op == EXP_OP_MBIT_SEL) ||
		(lhs->op == EXP_OP_CONCAT)   ||
		(lhs->op == EXP_OP_LIST) );
#endif
        break;
    }

  }

}

/*!
 \param expr      Pointer to root expression to deallocate.
 \param exp_only  Removes only the specified expression and not its children.

 Deallocates all heap memory allocated with the malloc routine.
*/
void expression_dealloc( expression* expr, bool exp_only ) {

  int        op;        /* Temporary operation holder */
  exp_link*  tmp_expl;  /* Temporary pointer to expression list */
  statement* tmp_stmt;  /* Temporary pointer to statement */

  if( expr != NULL ) {

    op = expr->op;

    if( ESUPPL_OWNS_VEC( expr->suppl ) ) {

      /* Free up memory from vector value storage */
      vector_dealloc( expr->value );
      expr->value = NULL;

      /* If this is a named block call or fork statement, remove the statement that this expression points to */
      if( (expr->op == EXP_OP_NB_CALL) || (expr->op == EXP_OP_FORK) ) {

        if( !exp_only ) {
#ifdef DEBUG_MODE
          snprintf( user_msg, USER_MSG_LENGTH, "Removing statement block starting at line %d because it is a NB_CALL or FORK and its calling expression is being removed", expr->stmt->exp->line );
          print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
          stmt_blk_add_to_remove_list( expr->stmt );
        } else {
          bind_rm_stmt( expr->id );
        }

      }

    } else {

      /* Deallocate vector memory but not vector itself */
      if( (op != EXP_OP_ASSIGN)  &&
          (op != EXP_OP_DASSIGN) &&
          (op != EXP_OP_BASSIGN) &&
          (op != EXP_OP_RASSIGN) &&
          (op != EXP_OP_NASSIGN) &&
          (op != EXP_OP_IF)      &&
          (op != EXP_OP_WHILE)   &&
          (op != EXP_OP_PASSIGN) ) {
        free_safe( expr->value );
      }

      if( expr->sig == NULL ) {

        /* Remove this expression from the binding list */
        bind_remove( expr->id, expression_is_assigned( expr ) );

      } else {

        /* Remove this expression from the attached signal's expression list */
        exp_link_remove( expr, &(expr->sig->exp_head), &(expr->sig->exp_tail), FALSE );

        /* Clear the assigned bit of the attached signal */
        if( expression_is_assigned( expr ) ) {

          expr->sig->value->suppl.part.assigned = 0;

          /* If this signal must be assigned, remove all statement blocks that reference this signal */
          if( (expr->sig->value->suppl.part.mba == 1) && !exp_only ) {
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

    free_safe( expr->parent );

    /* If name contains data, free it */
    if( expr->name != NULL ) {
      free_safe( expr->name );
    }

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

    /* Remove this expression memory */
    free_safe( expr );

  }

}


/* 
 $Log$
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

