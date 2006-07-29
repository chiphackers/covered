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
 \file     link.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     11/28/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "link.h"
#include "defines.h"
#include "vsignal.h"
#include "expr.h"
#include "func_unit.h"
#include "util.h"
#include "statement.h"
#include "iter.h"
#include "fsm.h"
#include "gen_item.h"


/*!
 \param str   String to add to specified list.
 \param head  Pointer to head str_link element of list.
 \param tail  Pointer to tail str_link element of list.

 \return Returns a pointer to newly created string link.

 Creates a new str_link element with the value specified for str.  Sets
 next pointer of element to NULL, sets the tail element to point to the
 new element and sets the tail value to the new element.
*/
str_link* str_link_add( char* str, str_link** head, str_link** tail ) {

  str_link* tmp;    /* Temporary pointer to newly created str_link element */

  tmp = (str_link*)malloc_safe( sizeof( str_link ), __FILE__, __LINE__ );

  tmp->str    = str;
  tmp->suppl  = 0x0;
  tmp->suppl2 = 0x0;
  tmp->suppl3 = 0x0;
  tmp->range  = NULL;
  tmp->next   = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  return( tmp );

}

/*!
 \param stmt  Pointer to statement to add to specified statement list.
 \param head  Pointer to head str_link element of list.
 \param tail  Pointer to tail str_link element of list.

 Creates a new stmt_link element with the value specified for stmt.  Sets
 next pointer of element to head, sets the head element to point to the
 new element and (possibly) sets the tail value to the new element.
*/
void stmt_link_add_head( statement* stmt, stmt_link** head, stmt_link** tail ) {

  stmt_link* tmp;    /* Temporary pointer to newly created stmt_link element */

  tmp = (stmt_link*)malloc_safe( sizeof( stmt_link ), __FILE__, __LINE__ );

  tmp->stmt = stmt;

  if( *head == NULL ) {
    *head = *tail = tmp;
    tmp->ptr = NULL;
  } else {
    tmp->ptr     = (stmt_link*)((long int)(*head) ^ (long int)NULL);
    (*head)->ptr = (stmt_link*)((long int)((*head)->ptr) ^ (long int)tmp);
    *head        = tmp;
  }

}

/*!
 \param stmt  Pointer to statement to add to specified statement list.
 \param head  Pointer to head str_link element of list.
 \param tail  Pointer to tail str_link element of list.

 Creates a new stmt_link element with the value specified for stmt.  Sets
 next pointer of element to NULL and sets the tail value to the new element.
*/
void stmt_link_add_tail( statement* stmt, stmt_link** head, stmt_link** tail ) {

  stmt_link* tmp;    /* Temporary pointer to newly created stmt_link element */

  tmp = (stmt_link*)malloc_safe( sizeof( stmt_link ), __FILE__, __LINE__ );

  tmp->stmt = stmt;

  if( *head == NULL ) {
    *head = *tail = tmp;
    tmp->ptr = NULL;
  } else {
    tmp->ptr     = (stmt_link*)((long int)(*tail) ^ (long int)NULL);
    (*tail)->ptr = (stmt_link*)((long int)((*tail)->ptr) ^ (long int)tmp);
    *tail        = tmp;
  }

}

/*!
 \param expr  Expression to add to specified expression list.
 \param head  Pointer to head exp_link element of list.
 \param tail  Pointer to tail exp_link element of list.

 Creates a new exp_link element with the value specified for expr.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void exp_link_add( expression* expr, exp_link** head, exp_link** tail ) {

  exp_link* tmp;   /* Temporary pointer to newly created exp_link element */

  tmp = (exp_link*)malloc_safe( sizeof( exp_link ), __FILE__, __LINE__ );

  tmp->exp  = expr;
  tmp->next = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

}

/*!
 \param sig   Signal to add to specified signal list.
 \param head  Pointer to head sig_link element of list.
 \param tail  Pointer to tail sig_link element of list.

 Creates a new sig_link element with the value specified for sig.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void sig_link_add( vsignal* sig, sig_link** head, sig_link** tail ) {

  sig_link* tmp;   /* Temporary pointer to newly created sig_link element */

  tmp = (sig_link*)malloc_safe( sizeof( sig_link ), __FILE__, __LINE__ );

  tmp->sig  = sig;
  tmp->next = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

}

/*!
 \param table  Pointer to FSM structure to store.
 \param head   Pointer to head of FSM list.
 \param tail   Pointer to tail of FSM list.

 Creates a new fsm_link element with the value specified for table.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void fsm_link_add( fsm* table, fsm_link** head, fsm_link** tail ) {

  fsm_link* tmp;  /* Temporary pointer to newly created fsm_link element */

  tmp = (fsm_link*)malloc_safe( sizeof( fsm_link ), __FILE__, __LINE__ );

  tmp->table = table;
  tmp->next  = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

}

/*!
 \param funit  Functional unit to add to specified functional unit list.
 \param head   Pointer to head funit_link element of list.
 \param tail   Pointer to tail funit_link element of list.

 Creates a new funit_link element with the value specified for functional unit.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void funit_link_add( func_unit* funit, funit_link** head, funit_link** tail ) {
	
  funit_link* tmp;   /* Temporary pointer to newly created funit_link element */
	
  tmp = (funit_link*)malloc_safe( sizeof( funit_link ), __FILE__, __LINE__ );
	
  tmp->funit = funit;
  tmp->next  = NULL;
	
  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }
  
}

/*!
 \param gi     Generate item to add to specified gitem_link list.
 \param head   Pointer to head gitem_link element of list.
 \param tail   Pointer to tail gitem_link element of list.

 Creates a new gitem_link element with the value specified for generate item.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void gitem_link_add( gen_item* gi, gitem_link** head, gitem_link** tail ) {

  gitem_link* tmp;  /* Temporary pointer to newly created gitem_link element */

  tmp = (gitem_link*)malloc_safe( sizeof( gitem_link ), __FILE__, __LINE__ );

  tmp->gi   = gi;
  tmp->next = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

}

/*!
 \param head  Pointer to head of str_link list.

 Displays the string contents of the str_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void str_link_display( str_link* head ) {

  str_link* curr;    /* Pointer to current str_link link to display */

  printf( "String list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  str: %s\n", curr->str );
    curr = curr->next;
  }

}

/*!
 \param head  Pointer to head of stmt_link list.

 Displays the string contents of the stmt_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void stmt_link_display( stmt_link* head ) {

  stmt_iter curr;   /* Statement list iterator */

  printf( "Statement list:\n" );

  stmt_iter_reset( &curr, head );
  while( curr.curr != NULL ) {
    assert( curr.curr->stmt != NULL );
    assert( curr.curr->stmt->exp != NULL );
    printf( "  id: %d, line: %d\n", curr.curr->stmt->exp->id, curr.curr->stmt->exp->line );
    stmt_iter_next( &curr );
  }

}

/*!
 \param head  Pointer to head of exp_link list.

 Displays the string contents of the exp_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void exp_link_display( exp_link* head ) {

  exp_link* curr;    /* Pointer to current expression link */

  printf( "Expression list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  id: %d, op: %s, line: %d\n", curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line );
    curr = curr->next;
  }

}

/*!
 \param head  Pointer to head of sig_link list.

 Displays the string contents of the sig_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void sig_link_display( sig_link* head ) {

  sig_link* curr;    /* Pointer to current sig_link link to display */

  printf( "Signal list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  name: %s\n", curr->sig->name );
    curr = curr->next;
  }

}

/*!
 \param head  Pointer to head of funit_link list.

 Displays the string contents of the funit_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void funit_link_display( funit_link* head ) {

  funit_link* curr;    /* Pointer to current funit_link link to display */

  printf( "Functional unit list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  name: %s, type: %d\n", curr->funit->name, curr->funit->type );
    curr = curr->next;
  }

}

/*!
 \param head  Pointer to head of funit_link list.

 Displays the string contents of the funit_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void gitem_link_display( gitem_link* head ) {

  gitem_link* curr;  /* Pointer to current gitem_link to display */

  printf( "Generate item list:\n" );

  curr = head;
  while( curr != NULL ) {
    gen_item_display( curr->gi );
    curr = curr->next;
  }

}

/*!
 \param value  String to find in str_link list.
 \param head   Pointer to head link in str_link list to search.
 \return Returns the pointer to the found str_link or NULL if the search was unsuccessful.

 Iteratively searches the str_link list specifed by the head str_link element.  If
 a matching string is found, the pointer to this element is returned.  If the specified
 string could not be matched, the value of NULL is returned.
*/
str_link* str_link_find( char* value, str_link* head ) {

  str_link* curr;    /* Pointer to current str_link link */
  
  curr = head;
  while( (curr != NULL) && (strcmp( curr->str, value ) != 0) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param id    ID of statement to find.
 \param head  Pointer to head of stmt_link list to search.

 \return Returns the pointer to the found stmt_link or NULL if the search was unsuccessful.

 Iteratively searches the stmt_link list specified by the head stmt_link element.  If
 a matching statement is found, the pointer to this element is returned.  If the specified
 statement could not be matched, the value of NULL is returned.
*/
stmt_link* stmt_link_find( int id, stmt_link* head ) {

  stmt_iter curr;   /* Statement list iterator */

  stmt_iter_reset( &curr, head );
  while( (curr.curr != NULL) && (curr.curr->stmt->exp->id != id) ) {
    stmt_iter_next( &curr );
  }

  return( curr.curr );

}

/*!
 \param exp   Pointer to expression to find.
 \param head  Pointer to head of exp_link list to search.

 \return Returns the pointer to the found exp_link or NULL if the search was unsuccessful.

 Iteratively searches the exp_link list specified by the head exp_link element.  If
 a matching expression is found, the pointer to this element is returned.  If the specified
 expression could not be matched, the value of NULL is returned.
*/
exp_link* exp_link_find( expression* exp, exp_link* head ) {

  exp_link* curr;   /* Expression list iterator */

  curr = head;
  while( (curr != NULL) && (curr->exp->id != exp->id) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param sig   Pointer to signal to find.
 \param head  Pointer to head of sig_link list to search.
 \return Returns the pointer to the found sig_link or NULL if the search was unsuccessful.

 Iteratively searches the sig_link list specified by the head sig_link element.  If
 a matching signal is found, the pointer to this element is returned.  If the specified
 signal could not be matched, the value of NULL is returned.
*/
sig_link* sig_link_find( vsignal* sig, sig_link* head ) {

  sig_link* curr;    /* Pointer to current sig_link link */

  curr = head;
  while( (curr != NULL) && !scope_compare( curr->sig->name, sig->name ) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param table  Pointer to FSM structure to find.
 \param head   Pointer to head of fsm_link list to search.

 \return Returns the pointer to the found fsm_link, or NULL if the search was unsuccessful.

 Iteratively searches the fsm_link list specified by the head fsm_link element.  If
 a matching FSM is found, the pointer to this element is returned.  If the specified
 FSM structure could not be matched, the value of NULL is returned.
*/
fsm_link* fsm_link_find( fsm* table, fsm_link* head ) {

  fsm_link* curr;  /* Pointer to current fsm_link element */

  curr = head;
  while( (curr != NULL) && (strcmp( curr->table->name, table->name ) != 0) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param funit  Pointer to functional unit to find.
 \param head   Pointer to head of funit_link list to search.
 
 \return Returns the pointer to the found funit_link or NULL if the search was unsuccessful.

 Iteratively searches the funit_link list specified by the head funit_link element.  If
 a matching functional unit is found, the pointer to this element is returned.  If the specified
 functional unit could not be matched, the value of NULL is returned.
*/
funit_link* funit_link_find( func_unit* funit, funit_link* head ) {

  funit_link* curr;    /* Pointer to current funit_link link */

  curr = head;
  while( (curr != NULL) && (!scope_compare( curr->funit->name, funit->name ) || (curr->funit->type != funit->type)) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param gi    Pointer to generate item to find.
 \param head  Pointer to head of gitem_link list to search.

 \return Returns the pointer to the found gitem_link or NULL if the search was unsuccessful.

 Iteratively searches the gitem_link list specified by the head gitem_link element.  If
 a matching generate item is found, the pointer to this element is returned.  If the specified
 generate item could not be matched, the value of NULL is returned.
*/
gitem_link* gitem_link_find( gen_item* gi, gitem_link* head ) {

  gitem_link* curr;  /* Pointer to current gitem_link */

  curr = head;
  while( (curr != NULL) && (gen_item_find( curr->gi, gi ) == NULL) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param str   Pointer to string to find and remove.
 \param head  Pointer to head of string list.
 \param tail  Pointer to tail of string list.

 Searches specified list for string that matches the specified string.  If
 a match is found, remove it from the list and deallocate the link memory.
*/
void str_link_remove( char* str, str_link** head, str_link** tail ) {

  str_link* curr;  /* Pointer to current string link */
  str_link* last;  /* Pointer to last string link */

  curr = *head;
  last = NULL;
  while( (curr != NULL) && (strcmp( str, curr->str ) != 0) ) {
    last = curr;
    curr = curr->next;
    assert( (curr == NULL) || (curr->str != NULL) );
  }

  if( curr != NULL ) {

    if( curr == *head ) {
      *head = curr->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = curr->next;
    }

    /* Deallocate associated string */
    free_safe( curr->str );

#ifdef OBSOLETE
    /* Deallocate associated range, if present */
    if( curr->range != NULL ) {
      static_expr_dealloc( curr->range->left, FALSE );
      static_expr_dealloc( curr->range->right, FALSE );
      free_safe( curr->range );
    }
#endif

    /* Now deallocate this link itself */
    free_safe( curr );

  }

}

/*!
 \param exp        Pointer to expression to find and remove.
 \param head       Pointer to head of expression list.
 \param tail       Pointer to tail of expression list.
 \param recursive  If TRUE, recursively removes expression tree and expressions.

 Searches specified list for expression that matches the specified expression.  If
 a match is found, remove it from the list and deallocate the link memory.
*/
void exp_link_remove( expression* exp, exp_link** head, exp_link** tail, bool recursive ) {

  exp_link* curr;  /* Pointer to current expression link */
  exp_link* last;  /* Pointer to last expression link */

  assert( exp != NULL );

  /* If recursive mode is set, remove children first */
  if( recursive ) {
    if( (exp->left != NULL) && EXPR_LEFT_DEALLOCABLE( exp ) ) {
      exp_link_remove( exp->left, head, tail, recursive );
    }
    if( (exp->right != NULL) && EXPR_RIGHT_DEALLOCABLE( exp ) ) {
      exp_link_remove( exp->right, head, tail, recursive );
    }
  }

  curr = *head;
  last = NULL;
  while( (curr != NULL) && (curr->exp->id != exp->id) ) {
    last = curr;
    curr = curr->next;
    if( curr != NULL ) {
      assert( curr->exp != NULL );
    }
  }

  if( curr != NULL ) {

    if( curr == *head ) {
      *head = curr->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = curr->next;
    }

    free_safe( curr );

  }

  /* If recursive flag set, remove expression as well */
  if( recursive ) {
    expression_dealloc( exp, TRUE );
  }

}

/*!
 \param gi    Pointer to specified generate item to remove
 \param head  Pointer to head of generate item list
 \param tail  Pointer to tail of generate item list

 Deletes specified generate item from the given list, adjusting the head and
 tail pointers accordingly.
*/
void gitem_link_remove( gen_item* gi, gitem_link** head, gitem_link** tail ) {

  gitem_link* gil;   /* Pointer to current generate item link */
  gitem_link* last;  /* Pointer to last generate item link traversed */

  gil = *head;
  while( (gil != NULL) && (gil->gi != gi) ) {
    last = gil;
    gil  = gil->next;
  }

  if( gil != NULL ) {

    if( gil == *head ) {
      *head = gil->next;
    } else if( gil == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = gil->next;
    }

    free_safe( gil );

  }

}

/*!
 \param head  Pointer to head str_link element of list.

 Deletes each element of the specified list.
*/
void str_link_delete_list( str_link* head ) {

  str_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate memory for stored string */
    if( tmp->str != NULL ) {
      free_safe( tmp->str );
      tmp->str = NULL;
    }

    /* Deallocate memory for range information, if specified */
#ifdef OBSOLETE
    if( tmp->range != NULL ) {
      static_expr_dealloc( tmp->range->left, FALSE );
      static_expr_dealloc( tmp->range->right, FALSE );
      free_safe( tmp->range );
    }
#endif

    /* Deallocate str_link element itself */
    free_safe( tmp );

  }

}

/*!
 \param stmt  Pointer to the statement to unlink from the given statement list
 \param head  Pointer to the head of a statement list
 \param tail  Pointer to the tail of a statement list

 Iterates through given statement list searching for the given statement.  When
 the statement link is found that matches, removes that link from the list and repairs
 the list.
*/
void stmt_link_unlink( statement* stmt, stmt_link** head, stmt_link** tail ) {

  stmt_iter  curr;   /* Statement list iterator */
  stmt_link* next;   /* Pointer to next stmt_link in list */
  stmt_link* next2;  /* Pointer to next after next stmt_link in list */
  stmt_link* last2;  /* Pointer to last before last stmt_link in list */

  stmt_iter_reset( &curr, *head );

  while( (curr.curr != NULL) && (curr.curr->stmt != stmt) ) {
    stmt_iter_next( &curr );
  }

  if( curr.curr != NULL ) {

    if( (curr.curr == *head) && (curr.curr == *tail) ) {
      *head = *tail = NULL;
    } else if( curr.curr == *head ) {
      next           = (stmt_link*)((long int)curr.curr->ptr ^ (long int)curr.last);
      next2          = (stmt_link*)((long int)next->ptr ^ (long int)curr.curr);
      next->ptr      = next2;
      *head          = next;
    } else if( curr.curr == *tail ) {
      last2          = (stmt_link*)((long int)curr.last->ptr ^ (long int)curr.curr);
      curr.last->ptr = last2;
      *tail          = curr.last;
    } else {
      next           = (stmt_link*)((long int)curr.curr->ptr ^ (long int)curr.last);
      next2          = (stmt_link*)((long int)next->ptr ^ (long int)curr.curr);
      last2          = (stmt_link*)((long int)curr.last->ptr ^ (long int)curr.curr);
      next->ptr      = (stmt_link*)((long int)curr.last ^ (long int)next2);
      curr.last->ptr = (stmt_link*)((long int)last2 ^ (long int)next);
    }

    /* Deallocate the stmt_link */
    free_safe( curr.curr );

  }

}

/*!
 \param head  Pointer to head stmt_link element of list.

 Deletes each element of the specified list.
*/
void stmt_link_delete_list( stmt_link* head ) {

  stmt_iter curr;  /* Statement list iterator */

  stmt_iter_reset( &curr, head );
  
  while( curr.curr != NULL ) {

    /* Deallocate statement */
    statement_dealloc( curr.curr->stmt );
    curr.curr->stmt = NULL;

    head      = (stmt_link*)((long int)(curr.curr->ptr) ^ (long int)(curr.last));
    if( head != NULL ) {
      head->ptr = (stmt_link*)((long int)(curr.curr) ^ (long int)(head->ptr));
    }

    /* Deallocate stmt_link element itself */
    free_safe( curr.curr );

    stmt_iter_reset( &curr, head );
    
  }

}

/*!
 \param head  Pointer to head exp_link element of list.
 \param del_exp  If set to TRUE, deallocates the expression; otherwise, leaves expression alone.

 Deletes each element of the specified list.
*/
void exp_link_delete_list( exp_link* head, bool del_exp ) {

  exp_link* tmp;  /* Pointer to current expression link to remove */
  
  while( head != NULL ) {

    tmp  = head;
    head = head->next;
    
    /* Deallocate expression */
    if( del_exp ) {
      expression_dealloc( tmp->exp, TRUE );
      tmp->exp = NULL;
    }
    
    /* Deallocate exp_link element itself */
    free_safe( tmp );
    
  }

}

/*!
 \param head     Pointer to head sig_link element of list.
 \param del_sig  If set to TRUE, deallocates the signal; otherwise, leaves signal alone.

 Deletes each element of the specified list.
*/
void sig_link_delete_list( sig_link* head, bool del_sig ) {

  sig_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate signal */
    if( del_sig ) {
      vsignal_dealloc( tmp->sig );
      tmp->sig = NULL;
    }

    /* Deallocate sig_link element itself */
    free_safe( tmp );

  }

}

/*!
 \param head  Pointer to head fsm_link element of list.

 Deletes each element of the specified list.
*/
void fsm_link_delete_list( fsm_link* head ) {

  fsm_link* tmp;  /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate FSM structure */
    fsm_dealloc( tmp->table );
    tmp->table = NULL;

    /* Deallocate fsm_link element itself */
    free_safe( tmp );

  }

}

/*!
 \param head      Pointer to head funit_link element of list.
 \param rm_funit  If TRUE, deallocates specified functional unit; otherwise, just deallocates the links

 Deletes each element of the specified list.
*/
void funit_link_delete_list( funit_link* head, bool rm_funit ) {

  funit_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate signal */
    if( rm_funit ) {
      funit_dealloc( tmp->funit );
      tmp->funit = NULL;
    }

    /* Deallocate funit_link element itself */
    free_safe( tmp );

  }

}

/*!
 \param head      Pointer to head gitem_link element of list.
 \param rm_elems  If TRUE, deallocates specified generate item.

 Deletes each element of the specified list.
*/
void gitem_link_delete_list( gitem_link* head, bool rm_elems ) {

  gitem_link* tmp;  /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate generate item */
    gen_item_dealloc( tmp->gi, rm_elems );

    /* Deallocate gitem_link element itself */
    free_safe( tmp );

  }

}


/*
 $Log$
 Revision 1.50  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.49  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.48  2006/07/14 18:53:32  phase1geo
 Fixing -g option for keywords.  This seems to be working and I believe that
 regressions are passing here as well.

 Revision 1.47  2006/07/13 22:24:57  phase1geo
 We are really broke at this time; however, more code has been added to support
 the -g score option.

 Revision 1.46  2006/07/09 01:40:39  phase1geo
 Removing the vpi directory (again).  Also fixing a bug in Covered's expression
 deallocator where a case statement contains an unbindable signal.  Previously
 the case test expression was being deallocated twice.  This submission fixes
 this bug (bug was also fixed in the 0.4.5 stable release).  Added new tests
 to verify fix.  Full regression passes.

 Revision 1.45  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.44  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.43  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.42  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.41  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.40  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.39  2005/12/19 05:18:24  phase1geo
 Fixing memory leak problems with instance1.1.  Full regression has some segfaults
 that need to be looked at now.

 Revision 1.38  2005/12/14 23:03:24  phase1geo
 More updates to remove memory faults.  Still a work in progress but full
 regression passes.

 Revision 1.37  2005/11/22 16:46:27  phase1geo
 Fixed bug with clearing the assigned bit in the binding phase.  Full regression
 now runs cleanly.

 Revision 1.36  2005/11/22 05:30:33  phase1geo
 Updates to regression suite for clearing the assigned bit when a statement
 block is removed from coverage consideration and it is assigning that signal.
 This is not fully working at this point.

 Revision 1.35  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.34  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.33  2005/01/25 13:42:27  phase1geo
 Fixing segmentation fault problem with race condition checking.  Added race1.1
 to regression.  Removed unnecessary output statements from previous debugging
 checkins.

 Revision 1.32  2005/01/24 13:21:45  phase1geo
 Modifying unlinking algorithm for statement links.  Still getting
 segmentation fault at this time.

 Revision 1.31  2005/01/11 14:24:16  phase1geo
 Intermediate checkin.

 Revision 1.30  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.29  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.28  2004/07/22 04:43:06  phase1geo
 Finishing code to calculate start and end columns of expressions.  Regression
 has been updated for these changes.  Other various minor changes as well.

 Revision 1.27  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.26  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.25  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.24  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.23  2003/10/13 03:56:29  phase1geo
 Fixing some problems with new FSM code.  Not quite there yet.

 Revision 1.22  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.21  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.20  2003/02/08 21:54:06  phase1geo
 Fixing memory problems with db_remove_statement function.  Updating comments
 in statement.c to explain some of the changes necessary to properly remove
 a statement tree.

 Revision 1.19  2003/02/07 02:28:23  phase1geo
 Fixing bug with statement removal.  Expressions were being deallocated but not properly
 removed from module parameter expression lists and module expression lists.  Regression
 now passes again.

 Revision 1.18  2003/02/05 22:50:56  phase1geo
 Some minor tweaks to debug output and some minor bug "fixes".  At this point
 regression isn't stable yet.

 Revision 1.17  2003/01/04 09:33:28  phase1geo
 Updating documentation to match recent code fixes/changes.

 Revision 1.16  2003/01/04 09:25:15  phase1geo
 Fixing file search algorithm to fix bug where unexpected module that was
 ignored cannot be found.  Added instance7.v diagnostic to verify appropriate
 handling of this problem.  Added tree.c and tree.h and removed define_t
 structure in lexer.

 Revision 1.15  2003/01/03 02:07:43  phase1geo
 Fixing segmentation fault in lexer caused by not closing the temporary
 input file before unlinking it.  Fixed case where module was parsed but not
 at the head of the module list.

 Revision 1.14  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.13  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.12  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.11  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.10  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.9  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.8  2002/07/18 22:02:35  phase1geo
 In the middle of making improvements/fixes to the expression/signal
 binding phase.

 Revision 1.7  2002/06/26 22:09:17  phase1geo
 Removing unecessary output and updating regression Makefile.

 Revision 1.6  2002/06/26 03:45:48  phase1geo
 Fixing more bugs in simulator and report functions.  About to add support
 for delay statements.

 Revision 1.5  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.4  2002/06/25 02:02:04  phase1geo
 Fixing bugs with writing/reading statements and with parsing design with
 statements.  We now get to the scoring section.  Some problems here at
 the moment with the simulator.

 Revision 1.3  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

