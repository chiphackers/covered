#ifndef __CODEGEN_H__
#define __CODEGEN_H__

/*!
 \file     codegen.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     4/11/2002
 \brief    Contains functions for generating Verilog code from expression trees.
*/

#include "defines.h"


//! Creates Verilog code string from specified expression tree.
char* codegen_gen_expr( expression* expr, int line );

#endif
