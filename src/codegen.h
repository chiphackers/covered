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
char* codegen_gen_expr( expression* expr, int line, int parent_op );


/* $Log$
/* Revision 1.2  2002/05/03 03:39:36  phase1geo
/* Removing all syntax errors due to addition of statements.  Added more statement
/* support code.  Still have a ways to go before we can try anything.  Removed lines
/* from expressions though we may want to consider putting these back for reporting
/* purposes.
/* */

#endif
