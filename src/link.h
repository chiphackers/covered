#ifndef __LINK_H__
#define __LINK_H__

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
 \file     link.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/28/2001
 \brief    Contains functions to manipulate a linked lists.
*/

#include "defines.h"


/*! \brief Adds specified string to str_link element at the end of the list. */
str_link* str_link_add(
  char*      str,
  str_link** head,
  str_link** tail
);

/*! \brief Adds specified statement to stmt_link element at the beginning of the list. */
void stmt_link_add_head(
  statement*  stmt,
  bool        rm_stmt,
  stmt_link** head,
  stmt_link** tail
);

/*! \brief Adds specified statement to stmt_link element at the end of the list. */
void stmt_link_add_tail(
  statement*  stmt,
  bool        rm_stmt, 
  stmt_link** head,
  stmt_link** tail
);

/*! \brief Adds specified expression to exp_link element at the end of the list. */
void exp_link_add(
  expression*   expr,
  expression*** exps,
  unsigned int* exp_size
);

/*! \brief Adds specified signal to sig_link element at the end of the list. */
void sig_link_add(
  vsignal*      sig,
  bool          rm_sig,
  vsignal***    sigs,
  unsigned int* sig_size,
  unsigned int* sig_no_rm_index
);

/*! \brief Adds specified FSM to fsm_link element at the end of the list. */
void fsm_link_add(
  fsm*          table,
  fsm***        fsms,
  unsigned int* fsm_size
);

/*! \brief Adds specified functional unit to funit_link element at the end of the list. */
void funit_link_add(
  func_unit*   funit,
  funit_link** head,
  funit_link** tail
);

#ifndef VPI_ONLY
/*! \brief Adds specified generate item to the end of specified gitem list. */
void gitem_link_add(
  gen_item*    gi,
  gitem_link** head,
  gitem_link** tail
);
#endif

/*! \brief Adds specified functional unit instance to inst_link element at the end of the list. */
inst_link* inst_link_add(
  funit_inst* inst,
  inst_link** head,
  inst_link** tail
);

/*********************************************************************************/

/*! \brief Displays specified string list to standard output. */
void str_link_display(
  str_link* head
);

/*! \brief Displays specified statement list to standard output. */
void stmt_link_display(
  stmt_link* head
);

/*! \brief Displays specified expression list to standard output. */
void exp_link_display(
  expression** exps,
  unsigned int exp_size
);

/*! \brief Displays specified signal list to standard output. */
void sig_link_display(
  vsignal**    sigs,
  unsigned int sig_size
);

/*! \brief Displays specified functional unit list to standard output. */
void funit_link_display(
  funit_link* head
);

#ifndef VPI_ONLY
/*! \brief Displays specified generate item list to standard output. */
void gitem_link_display(
  gitem_link* head
);
#endif

/*! \brief Displays specified instance list to standard output. */
void inst_link_display(
  inst_link* head
);

/*********************************************************************************/

/*! \brief Finds specified string in the given str_link list. */
/*@null@*/ str_link* str_link_find(
  const char* value,
  str_link*   head
);

/*! \brief Finds specified statement in the given stmt_link list. */
/*@null@*/ stmt_link* stmt_link_find(
  int        id,
  stmt_link* head
);

/*! \brief Finds specified statement in the given stmt_link list. */
stmt_link* stmt_link_find_by_position(
  unsigned int first_line,
  unsigned int first_column,
  stmt_link*   head
);

/*! \brief Finds specified expression in the given exp_link list. */
/*@null@*/ expression* exp_link_find(
  int          id,
  expression** exps,
  unsigned int exp_size
);

/*! \brief Finds specified signal in given sig_link list. */
/*@null@*/ vsignal* sig_link_find(
  const char*  name,
  vsignal**    sigs,
  unsigned int sig_size
);

/*! \brief Finds specified FSM structure in fsm_link list. */
/*@null@*/ fsm* fsm_link_find(
  const char*  name,
  fsm**        fsms,
  unsigned int fsm_size
);

/*! \brief Finds specified functional unit in given funit_link list. */
/*@null@*/ /*@shared@*/ funit_link* funit_link_find(
  const char* name,
  int         type,
  funit_link* head
);

#ifndef VPI_ONLY
/*! \brief Finds specified generate item in given gitem_link list. */
/*@null@*/ gitem_link* gitem_link_find(
  gen_item*   gi,
  gitem_link* head
);
#endif

/*! \brief Finds specified functional unit instance in given inst_link list by scope. */
/*@null@*/ funit_inst* inst_link_find_by_scope(
  char*      scope,
  inst_link* head,
  bool       rm_unnamed
);

/*! \brief Finds specified functional unit instance in given inst_link list by functional unit. */
/*@null@*/ funit_inst* inst_link_find_by_funit(
  const func_unit* funit,
  inst_link*       head,
  int*             ignore
);

/*********************************************************************************/

/*! \brief Searches for and removes specified string link from list. */
void str_link_remove(
  char*      str,
  str_link** head,
  str_link** tail
);

/*! \brief Searches for and removes specified expression link from list. */
void exp_link_remove(
  expression*   exp,
  expression*** exps,
  unsigned int* exp_size,
  bool          recursive
);

#ifndef VPI_ONLY
/*! \brief Searches for and removes specified generate item link from list. */
void gitem_link_remove(
  gen_item*    gi,
  gitem_link** head,
  gitem_link** tail
);
#endif

/*! \brief Searches for and removes specified functional unit link from list. */
void funit_link_remove(
  func_unit*   funit,
  funit_link** head,
  funit_link** tail,
  bool         rm_funit
);

/*********************************************************************************/

/*! \brief Deletes entire list specified by head pointer. */
void str_link_delete_list(
  str_link* head
);

/*! \brief Unlinks the stmt_link specified by the specified statement */
void stmt_link_unlink(
  statement*  stmt,
  stmt_link** head,
  stmt_link** tail
);

/*! \brief Deletes entire list specified by head pointer. */
void stmt_link_delete_list(
  stmt_link* head
);

/*! \brief Deletes entire list specified by head pointer. */
void exp_link_delete_list(
  expression** exps,
  unsigned int exp_size,
  bool         del_exp
);

/*! \brief Deletes entire list specified by head pointer. */
void sig_link_delete_list(
  vsignal**    sigs,
  unsigned int sig_size,
  unsigned int sig_no_rm_index,
  bool         del_sig
);

/*! \brief Deletes entire list specified by head pointer. */
void fsm_link_delete_list(
  fsm**        fsms,
  unsigned int fsm_size
);

/*! \brief Deletes entire list specified by head pointer. */
void funit_link_delete_list(
  funit_link** head,
  funit_link** tail,
  bool         rm_funit
);

#ifndef VPI_ONLY
/*! \brief Deletes entire list specified by head pointer. */
void gitem_link_delete_list(
  gitem_link* head,
  bool        rm_elems
);
#endif

/*! \brief Deletes entire list specified by head pointer. */
void inst_link_delete_list(
  inst_link* head
);

#endif

