#ifndef __LINK_H__
#define __LINK_H__

/*!
 \file     link.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/28/2001
 \brief    Contains functions to manipulate a linked lists.
*/

#include "defines.h"


//! Adds specified string to str_link element at the end of the list.
void str_link_add( char* str, str_link** head, str_link** tail );

//! Adds specified string to exp_link element at the end of the list.
void exp_link_add( expression* expr, exp_link** head, exp_link** tail );

//! Adds specified string to sig_link element at the end of the list.
void sig_link_add( signal* sig, sig_link** head, sig_link** tail );

//! Adds specified module to mod_link element at the end of the list.
void mod_link_add( module* mod, mod_link** head, mod_link** tail );


//! Displays specified string list to standard output.
void str_link_display( str_link* head );

//! Displays specified expression list to standard output.
void exp_link_display( exp_link* head );

//! Displays specified signal list to standard output.
void sig_link_display( sig_link* head );

//! Displays specified module list to standard output.
void mod_link_display( mod_link* head );


//! Finds specified string in the given str_link list.
str_link* str_link_find( char* value, str_link* head );

//! Finds specified expression in the given exp_link list.
exp_link* exp_link_find( expression* exp, exp_link* head );

//! Finds specified signal in given sig_link list.
sig_link* sig_link_find( signal* sig, sig_link* head );

//! Finds specified module in given mod_link list.
mod_link* mod_link_find( module* mod, mod_link* head );


//! Deletes entire list specified by head pointer.
void str_link_delete_list( str_link* head );

//! Deletes entire list specified by head pointer.
void exp_link_delete_list( exp_link* head, bool del_exp );

//! Deletes entire list specified by head pointer.
void sig_link_delete_list( sig_link* head );

//! Deletes entire list specified by head pointer.
void mod_link_delete_list( mod_link* head );

#endif

