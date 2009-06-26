/*
 Copyright (c) 2006-2009 Trevor Williams

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
 \author   Trevor Williams  (phase1geo@gmail.com)
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
#include "fsm.h"
#include "gen_item.h"
#include "obfuscate.h"
#include "instance.h"


/*!
 \return Returns a pointer to newly created string link.

 Creates a new str_link element with the value specified for str.  Sets
 next pointer of element to NULL, sets the tail element to point to the
 new element and sets the tail value to the new element.
*/
str_link* str_link_add(
            char*      str,   /*!< String to add to specified list */
  /*@out@*/ str_link** head,  /*!< Pointer to head str_link element of list */
  /*@out@*/ str_link** tail   /*!< Pointer to tail str_link element of list */
) { PROFILE(STR_LINK_ADD);

  str_link* tmp;  /* Temporary pointer to newly created str_link element */

  tmp = (str_link*)malloc_safe( sizeof( str_link ) );

  tmp->str    = str;
  tmp->str2   = NULL;
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

  PROFILE_END;

  return( tmp );

}

/*!
 Creates a new stmt_link element with the value specified for stmt.  Sets
 next pointer of element to head, sets the head element to point to the
 new element and (possibly) sets the tail value to the new element.
*/
void stmt_link_add(
            statement*  stmt,     /*!< Pointer to statement to add to specified statement list */
            bool        rm_stmt,  /*!< Value to specify if statement should be removed when the statement link is deleted */
  /*@out@*/ stmt_link** head,     /*!< Pointer to head str_link element of list */
  /*@out@*/ stmt_link** tail      /*!< Pointer to tail str_link element of list */
) { PROFILE(STMT_LINK_ADD_HEAD);

  stmt_link* tmp;  /* Temporary pointer to newly created stmt_link element */
  stmt_link* curr;

  tmp = (stmt_link*)malloc_safe( sizeof( stmt_link ) );

  tmp->stmt    = stmt;
  tmp->rm_stmt = rm_stmt;
  tmp->next    = NULL;

  if( *head == NULL ) {

    *head = *tail = tmp;

  } else {

    stmt_link* curr = *head;
    stmt_link* last = NULL;

    /* Insert the new statement in order (based on ppline) - start at tail the tail and work to the head */
    while( (curr != NULL) &&
           ((curr->stmt->exp->ppline < stmt->exp->ppline) ||
            ((curr->stmt->exp->ppline == stmt->exp->ppline) &&
             (((curr->stmt->exp->col >> 16) & 0xffff) < (stmt->exp->col >> 16 & 0xffff)))) ) {
      last = curr;
      curr = curr->next;
    }

    if( curr == *head ) {
      tmp->next = *head;
      *head     = tmp;
    } else if( curr == NULL ) {
      (*tail)->next = tmp;
      *tail         = tmp;
    } else {
      tmp->next  = curr;
      last->next = tmp;
    }

  }

  PROFILE_END;

}

/*!
 Creates a new exp_link element with the value specified for expr.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void exp_link_add(
            expression* expr,  /*!< Expression to add to specified expression list */
  /*@out@*/ exp_link**  head,  /*!< Pointer to head exp_link element of list */
  /*@out@*/ exp_link**  tail   /*!< Pointer to tail exp_link element of list */
) { PROFILE(EXP_LINK_ADD);

  exp_link* tmp;  /* Temporary pointer to newly created exp_link element */

  tmp = (exp_link*)malloc_safe( sizeof( exp_link ) );

  tmp->exp  = expr;
  tmp->next = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  PROFILE_END;

}

/*!
 Creates a new sig_link element with the value specified for sig.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void sig_link_add(
            vsignal*   sig,     /*!< Signal to add to specified signal list */
            bool       rm_sig,  /*!< Set to TRUE if signal should be deallocated when the sig_link is deallocated */
  /*@out@*/ sig_link** head,    /*!< Pointer to head sig_link element of list */
  /*@out@*/ sig_link** tail     /*!< Pointer to tail sig_link element of list */
) { PROFILE(SIG_LINK_ADD);

  sig_link* tmp;   /* Temporary pointer to newly created sig_link element */

  tmp = (sig_link*)malloc_safe( sizeof( sig_link ) );

  tmp->sig    = sig;
  tmp->rm_sig = rm_sig;
  tmp->next   = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  PROFILE_END;

}

/*!
 Creates a new fsm_link element with the value specified for table.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void fsm_link_add(
            fsm*       table,  /*!< Pointer to FSM structure to store */
  /*@out@*/ fsm_link** head,   /*!< Pointer to head of FSM list */
  /*@out@*/ fsm_link** tail    /*!< Pointer to tail of FSM list */
) { PROFILE(FSM_LINK_ADD);

  fsm_link* tmp;  /* Temporary pointer to newly created fsm_link element */

  tmp = (fsm_link*)malloc_safe( sizeof( fsm_link ) );

  tmp->table = table;
  tmp->next  = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  PROFILE_END;

}

/*!
 Creates a new funit_link element with the value specified for functional unit.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void funit_link_add(
            func_unit*   funit,  /*!< Functional unit to add to specified functional unit list */
  /*@out@*/ funit_link** head,   /*!< Pointer to head funit_link element of list */
  /*@out@*/ funit_link** tail    /*!< Pointer to tail funit_link element of list */
) { PROFILE(FUNIT_LINK_ADD);
	
  funit_link* tmp;   /* Temporary pointer to newly created funit_link element */
	
  tmp = (funit_link*)malloc_safe( sizeof( funit_link ) );
	
  tmp->funit = funit;
  tmp->next  = NULL;
	
  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  PROFILE_END;
  
}

#ifndef VPI_ONLY
/*!
 Creates a new gitem_link element with the value specified for generate item.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
void gitem_link_add(
            gen_item*    gi,    /*!< Generate item to add to specified gitem_link list */
  /*@out@*/ gitem_link** head,  /*!< Pointer to head gitem_link element of list */
  /*@out@*/ gitem_link** tail   /*!< Pointer to tail gitem_link element of list */
) { PROFILE(GITEM_LINK_ADD);

  gitem_link* tmp;  /* Temporary pointer to newly created gitem_link element */

  tmp = (gitem_link*)malloc_safe( sizeof( gitem_link ) );

  tmp->gi   = gi;
  tmp->next = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  PROFILE_END;

}
#endif /* VPI_ONLY */

/*!
 \return Returns pointer to newly allocated instance link.

 Creates a new inst_link element with the value specified for functional unit instance.
 Sets next pointer of element to NULL, sets the tail element to point
 to the new element and sets the tail value to the new element.
*/
inst_link* inst_link_add(
            funit_inst* inst,  /*!< Functional unit instance root to add */
  /*@out@*/ inst_link** head,  /*!< Pointer to head inst_link element of list */
  /*@out@*/ inst_link** tail   /*!< Pointer to tail inst_link element of list */
) { PROFILE(INST_LINK_ADD);

  inst_link* tmp;  /* Temporary pointer to newly created inst_link element */

  tmp = (inst_link*)malloc_safe( sizeof( inst_link ) );

  tmp->inst   = inst;
  tmp->ignore = FALSE;
  tmp->base   = FALSE;
  tmp->next   = NULL;

  if( *head == NULL ) {
    *head = *tail = tmp;
  } else {
    (*tail)->next = tmp;
    *tail         = tmp;
  }

  PROFILE_END;

  return( tmp );

}

/**************************************************************************************/

/*!
 Displays the string contents of the str_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void str_link_display(
  str_link* head  /*!< Pointer to head of str_link list */
) {

  str_link* curr;    /* Pointer to current str_link link to display */

  printf( "String list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  str: %s\n", curr->str );
    curr = curr->next;
  }

}

/*!
 Displays the string contents of the stmt_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void stmt_link_display(
  stmt_link* head  /*!< Pointer to head of stmt_link list */
) {

  stmt_link* curr = head;

  printf( "Statement list:\n" );

  while( curr != NULL ) {
    assert( curr->stmt != NULL );
    assert( curr->stmt->exp != NULL );
    printf( "  %s, ppline: %u, col: %u, added: %u, stmt_head: %u\n",
            expression_string( curr->stmt->exp ), curr->stmt->exp->ppline, ((curr->stmt->exp->col >> 16) & 0xffff),
            curr->stmt->suppl.part.added, curr->stmt->suppl.part.head );
    curr = curr->next;
  }

}

/*!
 Displays the string contents of the exp_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void exp_link_display(
  exp_link* head  /*!< Pointer to head of exp_link list */
) {

  exp_link* curr;    /* Pointer to current expression link */

  printf( "Expression list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  id: %d, op: %s, line: %u\n", curr->exp->id, expression_string_op( curr->exp->op ), curr->exp->line );
    curr = curr->next;
  }

}

/*!
 Displays the string contents of the sig_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void sig_link_display(
  sig_link* head  /*!< Pointer to head of sig_link list */
) {

  sig_link* curr;    /* Pointer to current sig_link link to display */

  printf( "Signal list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  name: %s\n", obf_sig( curr->sig->name ) );
    curr = curr->next;
  }

}

/*!
 Displays the string contents of the funit_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void funit_link_display(
  funit_link* head  /*!< Pointer to head of funit_link list */
) {

  funit_link* curr;    /* Pointer to current funit_link link to display */

  printf( "Functional unit list:\n" );

  curr = head;
  while( curr != NULL ) {
    printf( "  name: %s, type: %s\n", obf_funit( curr->funit->name ), get_funit_type( curr->funit->suppl.part.type ) );
    curr = curr->next;
  }

}

#ifndef VPI_ONLY
/*!
 Displays the contents of the gitem_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void gitem_link_display(
  gitem_link* head  /*!< Pointer to head of gitem_link list */
) {

  gitem_link* curr;  /* Pointer to current gitem_link to display */

  printf( "Generate item list:\n" );

  curr = head;
  while( curr != NULL ) {
    gen_item_display_block( curr->gi );
    curr = curr->next;
  }

}
#endif /* VPI_ONLY */

/*!
 Displays the contents of the inst_link list pointed to by head
 to standard output.  This function is mainly used for debugging purposes.
*/
void inst_link_display( 
  inst_link* head  /*!< Pointer to head of inst_link list */
) {

  inst_link* curr;  /* Pointer to current inst_link to display */

  printf( "Instance list:\n" );

  curr = head;
  while( curr != NULL ) {
    instance_display_tree( curr->inst );
    curr = curr->next;
  }

}

/**************************************************************************************/

/*!
 \return Returns the pointer to the found str_link or NULL if the search was unsuccessful.

 Iteratively searches the str_link list specifed by the head str_link element.  If
 a matching string is found, the pointer to this element is returned.  If the specified
 string could not be matched, the value of NULL is returned.
*/
str_link* str_link_find(
  const char* value,  /*!< String to find in str_link list */
  str_link*   head    /*!< Pointer to head link in str_link list to search */
) { PROFILE(STR_LINK_FIND);

  str_link* curr;    /* Pointer to current str_link link */
  
  curr = head;
  while( (curr != NULL) && (strcmp( curr->str, value ) != 0) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

/*!
 \return Returns the pointer to the found stmt_link or NULL if the search was unsuccessful.

 Iteratively searches the stmt_link list specified by the head stmt_link element.  If
 a matching statement is found, the pointer to this element is returned.  If the specified
 statement could not be matched, the value of NULL is returned.
*/
stmt_link* stmt_link_find(
  int        id,   /*!< ID of statement to find */
  stmt_link* head  /*!< Pointer to head of stmt_link list to search */
) { PROFILE(STMT_LINK_FIND);

  stmt_link* curr = head;

  while( (curr != NULL) && (curr->stmt->exp->id != id) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

/*!
 \return Returns the pointer to the found exp_link or NULL if the search was unsuccessful.

 Iteratively searches the exp_link list specified by the head exp_link element.  If
 a matching expression is found, the pointer to this element is returned.  If the specified
 expression could not be matched, the value of NULL is returned.
*/
exp_link* exp_link_find(
  int       id,   /*!< Expression ID to find */
  exp_link* head  /*!< Pointer to head of exp_link list to search */
) { PROFILE(EXP_LINK_FIND);

  exp_link* curr;   /* Expression list iterator */

  curr = head;
  while( (curr != NULL) && (curr->exp->id != id) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

/*!
 \return Returns the pointer to the found sig_link or NULL if the search was unsuccessful.

 Iteratively searches the sig_link list specified by the head sig_link element.  If
 a matching signal is found, the pointer to this element is returned.  If the specified
 signal could not be matched, the value of NULL is returned.
*/
sig_link* sig_link_find(
  const char* name,  /*!< Name of signal to find */
  sig_link*   head   /*!< Pointer to head of sig_link list to search */
) { PROFILE(SIG_LINK_FIND);

  sig_link* curr;    /* Pointer to current sig_link link */

  curr = head;
  while( (curr != NULL) && !scope_compare( curr->sig->name, name ) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

/*!
 \return Returns the pointer to the found fsm_link, or NULL if the search was unsuccessful.

 Iteratively searches the fsm_link list specified by the head fsm_link element.  If
 a matching FSM is found, the pointer to this element is returned.  If the specified
 FSM structure could not be matched, the value of NULL is returned.
*/
fsm_link* fsm_link_find(
  const char* name,  /*!< Name of FSM structure to find */
  fsm_link*   head   /*!< Pointer to head of fsm_link list to search */
) { PROFILE(FSM_LINK_FIND);

  fsm_link* curr;  /* Pointer to current fsm_link element */

  curr = head;
  while( (curr != NULL) && (strcmp( curr->table->name, name ) != 0) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

/*!
 \return Returns the pointer to the found funit_link or NULL if the search was unsuccessful.

 Iteratively searches the funit_link list specified by the head funit_link element.  If
 a matching functional unit is found, the pointer to this element is returned.  If the specified
 functional unit could not be matched, the value of NULL is returned.
*/
funit_link* funit_link_find(
  const char* name,  /*!< Name of functional unit to find */
  int         type,  /*!< Type of functional unit to find */
  funit_link* head   /*!< Pointer to head of funit_link list to search */
) { PROFILE(FUNIT_LINK_FIND);

  funit_link* curr;    /* Pointer to current funit_link link */

  curr = head;
  while( (curr != NULL) && (!scope_compare( curr->funit->name, name ) || (curr->funit->suppl.part.type != type)) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}

#ifndef VPI_ONLY
/*!
 \return Returns the pointer to the found gitem_link or NULL if the search was unsuccessful.

 Iteratively searches the gitem_link list specified by the head gitem_link element.  If
 a matching generate item is found, the pointer to this element is returned.  If the specified
 generate item could not be matched, the value of NULL is returned.
*/
gitem_link* gitem_link_find(
  gen_item*   gi,   /*!< Pointer to generate item to find */
  gitem_link* head  /*!< Pointer to head of gitem_link list to search */
) { PROFILE(GITEM_LINK_FIND);

  gitem_link* curr;  /* Pointer to current gitem_link */

  curr = head;
  while( (curr != NULL) && (gen_item_find( curr->gi, gi ) == NULL) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( curr );

}
#endif /* VPI_ONLY */

/*!
 \return Returns the pointer to the found funit_inst or NULL if the search was unsuccessful.

 Iteratively searches the inst_link list specified by the head inst_link element.  If
 a matching instance is found, the pointer to this element is returned.  If the specified
 generate item could not be matched, the value of NULL is returned.
*/
funit_inst* inst_link_find_by_scope(
  char*      scope,      /*!< Hierarchical scope to search for */
  inst_link* head,       /*!< Pointer to head of inst_link list to search */
  bool       rm_unnamed  /*!< If TRUE, removes any unnamed scopes in the design from being compared */
) { PROFILE(INST_LINK_FIND_BY_SCOPE);

  inst_link*  curr;         /* Pointer to current inst_link */
  funit_inst* inst = NULL;  /* Pointer to found instance */

  curr = head;
  while( (curr != NULL) && ((inst = instance_find_scope( curr->inst, scope, rm_unnamed )) == NULL) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( inst );

}

/*!
 \return Returns the pointer to the found funit_inst or NULL if the search was unsuccessful.

 Iteratively searches the inst_link list specified by the head inst_link element.  If
 a matching instance is found, the pointer to this element is returned.  If the specified
 generate item could not be matched, the value of NULL is returned.
*/
funit_inst* inst_link_find_by_funit(
  const func_unit* funit,  /*!< Functional unit to search for */
  inst_link*       head,   /*!< Pointer to head of inst_link list to search */
  int*             ignore  /*!< Pointer to integer specifying the number of instances to ignore that match the given functional unit */
) { PROFILE(INST_LINK_FIND_BY_FUNIT);

  inst_link*  curr;         /* Pointer to current inst_link */
  funit_inst* inst = NULL;  /* Pointer to found instance */

  curr = head;
  while( (curr != NULL) && ((inst = instance_find_by_funit( curr->inst, funit, ignore )) == NULL) ) {
    curr = curr->next;
  }

  PROFILE_END;

  return( inst );

}

/**************************************************************************************/

/*!
 Searches specified list for string that matches the specified string.  If
 a match is found, remove it from the list and deallocate the link memory.
*/
void str_link_remove(
            char*      str,   /*!< Pointer to string to find and remove */
  /*@out@*/ str_link** head,  /*!< Pointer to head of string list */
  /*@out@*/ str_link** tail   /*!< Pointer to tail of string list */
) { PROFILE(STR_LINK_REMOVE);

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

    if( (curr == *head) && (curr == *tail) ) {
      *head = *tail = NULL;
    } else if( curr == *head ) {
      *head = curr->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = curr->next;
    }

    /* Deallocate associated string */
    free_safe( curr->str, (strlen( curr->str ) + 1) );

    /* Now deallocate this link itself */
    free_safe( curr, sizeof( str_link ) );

  }

  PROFILE_END;

}

/*!
 Searches specified list for expression that matches the specified expression.  If
 a match is found, remove it from the list and deallocate the link memory.
*/
void exp_link_remove(
            expression* exp,       /*!< Pointer to expression to find and remove */
  /*@out@*/ exp_link**  head,      /*!< Pointer to head of expression list */
  /*@out@*/ exp_link**  tail,      /*!< Pointer to tail of expression list */
            bool        recursive  /*!< If TRUE, recursively removes expression tree and expressions */
) { PROFILE(EXP_LINK_REMOVE);

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

    if( (curr == *head) && (curr == *tail) ) {
      *head = *tail = NULL;
    } else if( curr == *head ) {
      *head = curr->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = curr->next;
    }

    free_safe( curr, sizeof( exp_link ) );

  }

  /* If recursive flag set, remove expression as well */
  if( recursive ) {
    expression_dealloc( exp, TRUE );
  }

  PROFILE_END;

}

#ifndef VPI_ONLY
/*!
 Deletes specified generate item from the given list, adjusting the head and
 tail pointers accordingly.
*/
void gitem_link_remove(
            gen_item*    gi,    /*!< Pointer to specified generate item to remove */
  /*@out@*/ gitem_link** head,  /*!< Pointer to head of generate item list */
  /*@out@*/ gitem_link** tail   /*!< Pointer to tail of generate item list */
) { PROFILE(GITEM_LINK_REMOVE);

  gitem_link* gil;   /* Pointer to current generate item link */
  gitem_link* last;  /* Pointer to last generate item link traversed */

  gil = *head;
  while( (gil != NULL) && (gil->gi != gi) ) {
    last = gil;
    gil  = gil->next;
  }

  if( gil != NULL ) {

    if( (gil == *head) && (gil == *tail) ) {
      *head = *tail = NULL;
    } else if( gil == *head ) {
      *head = gil->next;
    } else if( gil == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = gil->next;
    }

    free_safe( gil, sizeof( gitem_link ) );

  }

  PROFILE_END;

}
#endif /* VPI_ONLY */

/*!
 Searches for and removes the given functional unit from the given list and adjusts list as
 necessary.
*/
void funit_link_remove(
            func_unit*   funit,    /*!< Pointer to functional unit to find and remove */
  /*@out@*/ funit_link** head,     /*!< Pointer to head of functional unit list to remove functional unit from */
  /*@out@*/ funit_link** tail,     /*!< Pointer to tail of functional unit list to remove functional unit from */
            bool         rm_funit  /*!< If set to TRUE, deallocates functional unit as well */
) { PROFILE(FUNIT_LINK_REMOVE);

  funit_link* curr = *head;  /* Pointer to current functional unit link */
  funit_link* last = NULL;   /* Pointer to last functional unit link traversed */

  assert( funit != NULL );

  /* Search for matching functional unit */
  while( (curr != NULL) && (curr->funit != funit) ) {
    last = curr;
    curr = curr->next;
  }

  if( curr != NULL ) {

    /* Remove the functional unit from the list */
    if( (curr == *head) && (curr == *tail) ) {
      *head = *tail = NULL;
    } else if( curr == *head ) {
      *head = curr->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = curr->next;
    }

    /* Remove the functional unit, if necessary */
    if( rm_funit ) {
      funit_dealloc( curr->funit );
    }

    /* Deallocate the link */
    free_safe( curr, sizeof( funit_link ) );

  }

  PROFILE_END;

}

/**************************************************************************************/

/*!
 Deletes each element of the specified list.
*/
void str_link_delete_list(
  str_link* head  /*!< Pointer to head str_link element of list */
) { PROFILE(STR_LINK_DELETE_LIST);

  str_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate memory for stored string(s) */
    free_safe( tmp->str,  (strlen( tmp->str )  + 1) );
    free_safe( tmp->str2, (strlen( tmp->str2 ) + 1) );

    tmp->str  = NULL;
    tmp->str2 = NULL;

    /* Deallocate str_link element itself */
    free_safe( tmp, sizeof( str_link ) );

  }

  PROFILE_END;

}

/*!
 Iterates through given statement list searching for the given statement.  When
 the statement link is found that matches, removes that link from the list and repairs
 the list.
*/
void stmt_link_unlink(
            statement*  stmt,  /*!< Pointer to the statement to unlink from the given statement list */
  /*@out@*/ stmt_link** head,  /*!< Pointer to the head of a statement list */
  /*@out@*/ stmt_link** tail   /*!< Pointer to the tail of a statement list */
) { PROFILE(STMT_LINK_UNLINK);

  stmt_link* curr = *head;
  stmt_link* last = NULL;

  while( (curr != NULL) && (curr->stmt != stmt) ) {
    last = curr;
    curr = curr->next;
  }

  if( curr != NULL ) {

    if( (curr == *head) && (curr == *tail) ) {
      *head = *tail = NULL;
    } else if( curr == *head ) {
      *head = (*head)->next;
    } else if( curr == *tail ) {
      last->next = NULL;
      *tail      = last;
    } else {
      last->next = curr->next;
    }

    /* Deallocate the stmt_link */
    free_safe( curr, sizeof( stmt_link ) );

  }

  PROFILE_END;

}

/*!
 Deletes each element of the specified list.
*/
void stmt_link_delete_list(
  stmt_link* head  /*!< Pointer to head stmt_link element of list */
) { PROFILE(STMT_LINK_DELETE_LIST);

  stmt_link* curr = head;
  stmt_link* tmp;

  while( curr != NULL ) {

    tmp  = curr;
    curr = curr->next;

    /* Deallocate statement */
    if( tmp->rm_stmt ) {
      statement_dealloc( tmp->stmt );
    }
    tmp->stmt = NULL;

    /* Deallocate stmt_link element itself */
    free_safe( tmp, sizeof( stmt_link ) );

  }

  PROFILE_END;

}

/*!
 Deletes each element of the specified list.
*/
void exp_link_delete_list(
  exp_link* head,    /*!< Pointer to head exp_link element of list */
  bool      del_exp  /*!< If set to TRUE, deallocates the expression; otherwise, leaves expression alone */
) { PROFILE(EXP_LINK_DELETE_LIST);

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
    free_safe( tmp, sizeof( exp_link ) );
    
  }

  PROFILE_END;

}

/*!
 Deletes each element of the specified list.
*/
void sig_link_delete_list(
  sig_link* head,    /*!< Pointer to head sig_link element of list */
  bool      del_sig  /*!< If set to TRUE, deallocates the signal; otherwise, leaves signal alone */
) { PROFILE(SIG_LINK_DELETE_LIST);

  sig_link* tmp;   /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate signal */
    if( del_sig && tmp->rm_sig ) {
      vsignal_dealloc( tmp->sig );
      tmp->sig = NULL;
    }

    /* Deallocate sig_link element itself */
    free_safe( tmp, sizeof( sig_link ) );

  }

  PROFILE_END;

}

/*!
 Deletes each element of the specified list.
*/
void fsm_link_delete_list(
  fsm_link* head  /*!< Pointer to head fsm_link element of list */
) { PROFILE(FSM_LINK_DELETE_LIST);

  fsm_link* tmp;  /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate FSM structure */
    fsm_dealloc( tmp->table );
    tmp->table = NULL;

    /* Deallocate fsm_link element itself */
    free_safe( tmp, sizeof( fsm_link ) );

  }

  PROFILE_END;

}

/*!
 Deletes each element of the specified list.
*/
void funit_link_delete_list(
  /*@out@*/ funit_link** head,     /*!< Pointer to head funit_link element of list */
  /*@out@*/ funit_link** tail,     /*!< Pointer to tail funit_link element of list */
            bool         rm_funit  /*!< If TRUE, deallocates specified functional unit; otherwise, just deallocates the links */
) { PROFILE(FUNIT_LINK_DELETE_LIST);

  funit_link* tmp;   /* Temporary pointer to current link in list */

  while( *head != NULL ) {

    tmp   = *head;
    *head = tmp->next;

    /* Deallocate signal */
    if( rm_funit ) {
      funit_dealloc( tmp->funit );
      tmp->funit = NULL;
    }

    /* Deallocate funit_link element itself */
    free_safe( tmp, sizeof( funit_link ) );

  }

  *tail = NULL;

  PROFILE_END;

}

#ifndef VPI_ONLY
/*!
 Deletes each element of the specified list.
*/
void gitem_link_delete_list(
  gitem_link* head,     /*!< Pointer to head gitem_link element of list */
  bool        rm_elems  /*!< If TRUE, deallocates specified generate item */
) { PROFILE(GITEM_LINK_DELETE_LIST);

  gitem_link* tmp;  /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate generate item */
    gen_item_dealloc( tmp->gi, rm_elems );

    /* Deallocate gitem_link element itself */
    free_safe( tmp, sizeof( gitem_link ) );

  }

  PROFILE_END;

}
#endif /* VPI_ONLY */

/*!
 Deletes each element of the specified list.
*/
void inst_link_delete_list(
  inst_link* head  /*!< Pointer to head inst_link element of list */
) { PROFILE(INST_LINK_DELETE_LIST);

  inst_link* tmp;  /* Temporary pointer to current link in list */

  while( head != NULL ) {

    tmp  = head;
    head = tmp->next;

    /* Deallocate instance item */
    instance_dealloc( tmp->inst, tmp->inst->name );

    /* Deallocate inst_link element itself */
    free_safe( tmp, sizeof( inst_link ) );

  }

  PROFILE_END;

}

