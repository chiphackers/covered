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
#include "signal.h"
#include "expr.h"
#include "module.h"
#include "util.h"
#include "statement.h"
#include "iter.h"


/*!
 \param str   String to add to specified list.
 \param head  Pointer to head str_link element of list.
 \param tail  Pointer to tail str_link element of list.

 Creates a new str_link element with the value specified for str.  Sets
 next pointer of element to NULL, sets the tail element to point to the
 new element and sets the tail value to the new element.
*/
void str_link_add( char* str, str_link** head, str_link** tail ) {

  str_link* tmp;    /* Temporary pointer to newly created str_link element */

  tmp = (str_link*)malloc_safe( sizeof( str_link ) );

  tmp->str   = str;
  tmp->suppl = '_';
  tmp->next  = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

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

  tmp = (stmt_link*)malloc_safe( sizeof( stmt_link ) );

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

  tmp = (stmt_link*)malloc_safe( sizeof( stmt_link ) );

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

  tmp = (exp_link*)malloc_safe( sizeof( exp_link ) );

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
void sig_link_add( signal* sig, sig_link** head, sig_link** tail ) {

  sig_link* tmp;   /* Temporary pointer to newly created sig_link element */

  tmp = (sig_link*)malloc_safe( sizeof( sig_link ) );

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
 \param mod   Module to add to specified module list.
 \param head  Pointer to head mod_link element of list.
 \param tail  Pointer to tail mod_link element of list.

 Creates a new mod_link element with the value specified for mod.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void mod_link_add( module* mod, mod_link** head, mod_link** tail ) {
	
  mod_link* tmp;   /* Temporary pointer to newly created mod_link element */
	
  tmp = (mod_link*)malloc_safe( sizeof( mod_link ) );
	
  tmp->mod  = mod;
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
    printf( "  id: %d\n", curr.curr->stmt->exp->id );
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
    printf( "  id: %d, op: %d\n", curr->exp->id, SUPPL_OP( curr->exp->suppl ) );
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
 \param head  Pointer to head of mod_link list.

 Displays the string contents of the mod_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void mod_link_display( mod_link* head ) {

  mod_link* curr;    /* Pointer to current mod_link link to display */

  printf( "Module list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  name: %s\n", curr->mod->name );
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
sig_link* sig_link_find( signal* sig, sig_link* head ) {

  sig_link* curr;    /* Pointer to current sig_link link */

  curr = head;
  while( (curr != NULL) && (strcmp( curr->sig->name, sig->name ) != 0) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param mod    Pointer to module to find.
 \param head  Pointer to head of mod_link list to search.
 
 \return Returns the pointer to the found mod_link or NULL if the search was unsuccessful.

 Iteratively searches the mod_link list specified by the head mod_link element.  If
 a matching module is found, the pointer to this element is returned.  If the specified
 module could not be matched, the value of NULL is returned.
*/
mod_link* mod_link_find( module* mod, mod_link* head ) {

  mod_link* curr;    /* Pointer to current mod_link link */

  curr = head;
  while( (curr != NULL) && (strcmp( curr->mod->name, mod->name ) != 0) ) {
    curr = curr->next;
  }

  return( curr );

}

/*!
 \param exp   Pointer to expression to find and remove.
 \param head  Pointer to head of expression list.
 \param tail  Pointer to tail of expression list.

 Searches specified list for expression that matches the specified expression.  If
 a match is found, remove it from the list and deallocate the link memory.
*/
void exp_link_remove( expression* exp, exp_link** head, exp_link** tail ) {

  exp_link* curr;  /* Pointer to current expression link */
  exp_link* last;  /* Pointer to last expression link    */

  curr = *head;
  last = NULL;
  while( (curr != NULL) && (curr->exp->id != exp->id) ) {
    last = curr;
    curr = curr->next;
    assert( curr->exp != NULL );
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

    /* Deallocate str_link element itself */
    free_safe( tmp );

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
 \param head  Pointer to head sig_link element of list.

 Deletes each element of the specified list.
*/
void sig_link_delete_list( sig_link* head ) {

  sig_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate signal */
    signal_dealloc( tmp->sig );
    tmp->sig = NULL;

    /* Deallocate sig_link element itself */
    free_safe( tmp );

  }

}

/*!
 \param head  Pointer to head mod_link element of list.

 Deletes each element of the specified list.
*/
void mod_link_delete_list( mod_link* head ) {

  mod_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate signal */
    module_dealloc( tmp->mod );
    tmp->mod = NULL;

    /* Deallocate sig_link element itself */
    free_safe( tmp );

  }

}


/*
 $Log$
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

