#ifndef __LINK_H__
#define __LINK_H__

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
 \file     link.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/28/2001
 \brief    Contains functions to manipulate a linked lists.
*/

#include "defines.h"


/*! \brief Adds specified string to str_link element at the end of the list. */
str_link* str_link_add( char* str, str_link** head, str_link** tail );

/*! \brief Adds specified statement to stmt_link element at the beginning of the list. */
void stmt_link_add_head( statement* stmt, stmt_link** head, stmt_link** tail );

/*! \brief Adds specified statement to stmt_link element at the end of the list. */
void stmt_link_add_tail( statement* stmt, stmt_link** head, stmt_link** tail );

/*! \brief Adds specified expression to exp_link element at the end of the list. */
void exp_link_add( expression* expr, exp_link** head, exp_link** tail );

/*! \brief Adds specified signal to sig_link element at the end of the list. */
void sig_link_add( vsignal* sig, sig_link** head, sig_link** tail );

/*! \brief Adds specified FSM to fsm_link element at the end of the list. */
void fsm_link_add( fsm* table, fsm_link** head, fsm_link** tail );

/*! \brief Adds specified functional unit to funit_link element at the end of the list. */
void funit_link_add( func_unit* funit, funit_link** head, funit_link** tail );

/*! \brief Adds specified generate item to the end of specified gitem list. */
void gitem_link_add( gen_item* gi, gitem_link** head, gitem_link** tail );


/*! \brief Displays specified string list to standard output. */
void str_link_display( str_link* head );

/*! \brief Displays specified statement list to standard output. */
void stmt_link_display( stmt_link* head );

/*! \brief Displays specified expression list to standard output. */
void exp_link_display( exp_link* head );

/*! \brief Displays specified signal list to standard output. */
void sig_link_display( sig_link* head );

/*! \brief Displays specified functional unit list to standard output. */
void funit_link_display( funit_link* head );

/*! \brief Displays specified generate item list to standard output. */
void gitem_link_display( gitem_link* head );


/*! \brief Finds specified string in the given str_link list. */
str_link* str_link_find( char* value, str_link* head );

/*! \brief Finds specified statement in the given stmt_link list. */
stmt_link* stmt_link_find( int id, stmt_link* head );

/*! \brief Finds specified expression in the given exp_link list. */
exp_link* exp_link_find( expression* exp, exp_link* head );

/*! \brief Finds specified signal in given sig_link list. */
sig_link* sig_link_find( vsignal* sig, sig_link* head );

/*! \brief Finds specified FSM structure in fsm_link list. */
fsm_link* fsm_link_find( fsm* table, fsm_link* head );

/*! \brief Finds specified functional unit in given funit_link list. */
funit_link* funit_link_find( func_unit* funit, funit_link* head );

/*! \brief Finds specified generate item in given gitem_link list. */
gitem_link* gitem_link_find( gen_item* gi, gitem_link* head );


/*! \brief Searches for and removes specified string link from list. */
void str_link_remove( char* str, str_link** head, str_link** tail );

/*! \brief Searches for and removes specified expression link from list. */
void exp_link_remove( expression* exp, exp_link** head, exp_link** tail, bool recursive );

/*! \brief Searches for and removes specified generate item link from list. */
void gitem_link_remove( gen_item* gi, gitem_link** head, gitem_link** tail );


/*! \brief Deletes entire list specified by head pointer. */
void str_link_delete_list( str_link* head );

/*! \brief Unlinks the stmt_link specified by the specified statement */
void stmt_link_unlink( statement* stmt, stmt_link** head, stmt_link** tail );

/*! \brief Deletes entire list specified by head pointer. */
void stmt_link_delete_list( stmt_link* head );

/*! \brief Deletes entire list specified by head pointer. */
void exp_link_delete_list( exp_link* head, bool del_exp );

/*! \brief Deletes entire list specified by head pointer. */
void sig_link_delete_list( sig_link* head, bool del_sig );

/*! \brief Deletes entire list specified by head pointer. */
void fsm_link_delete_list( fsm_link* head );

/*! \brief Deletes entire list specified by head pointer. */
void funit_link_delete_list( funit_link* head, bool rm_funit );

/*! \brief Deletes entire list specified by head pointer. */
void gitem_link_delete_list( gitem_link* head, bool rm_elems );


/*
 $Log$
 Revision 1.19  2006/07/17 22:12:43  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.18  2006/07/13 22:24:57  phase1geo
 We are really broke at this time; however, more code has been added to support
 the -g score option.

 Revision 1.17  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.16  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.15  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.14  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.13  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.12  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.11  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.10  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.9  2003/02/07 02:28:23  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.8  2003/01/03 02:07:43  phase1geo
 Fixing segmentation fault in lexer caused by not closing the temporary
 input file before unlinking it.  Fixed case where module was parsed but not
 at the head of the module list.

 Revision 1.7  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.6  2002/10/31 23:13:56  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.5  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.4  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.3  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.2  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

#endif

