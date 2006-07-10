#ifndef __GEN_ITEM_H__
#define __GEN_ITEM_H__

/*!
 \file     gen_item.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     7/10/2006
 \brief    Contains functions for handling generate items.
*/


#include "defines.h"

#define GI_TYPE_EXPR	0
#define GI_TYPE_SIG     1
#define GI_TYPE_STMT    2
#define GI_TYPE_INST    3

struct gen_item_s;

typedef struct gen_item_s gen_item;

struct gen_item_s {
  union {
    expression* expr;        /*!< Pointer to an expression */
    vsignal*    sig;         /*!< Pointer to signal */
    statement*  stmt;        /*!< Pointer to statement */
    funit_inst* inst;        /*!< Pointer to instance */
  } elem;                    /*!< Union of various pointers this generate item is pointing at */
  int           type;        /*!< Specifies which element pointer is valid */
  gen_item*     next_true;   /*!< Pointer to the next generate item if expr is true */
  gen_item*     next_false;  /*!< Pointer to the next generate item if expr is false */
};



/*
 $Log$
*/

#endif
