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
 \file     sim.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     6/20/2002

 \par
 The simulation engine is made up of three parts:
 -# pre-simulation statement queue
 -# statement simulation engine
 -# expression simulation engine

 \par
 The operation of the simulation engine is as follows.  When a signal is found
 in the VCD file, the expressions to which it is a part of the RHS are looked up
 in the design tree.  The expression tree is then parsed from the expression to
 the root, setting the #ESUPPL_IS_LEFT_CHANGED or #ESUPPL_IS_RIGHT_CHANGED as it 
 makes its way to the root.  When at the root expression, the #ESUPPL_IS_STMT_HEAD 
 bit is interrogated.  If this bit is a 1, the expression's statement is loaded 
 into the pre-simulation statement queue.  If the bit is a 0, no further action is 
 taken.

 \par
 Once the timestep marker has been set, the simulate function is called.  The 
 statement located at the head of the queue is placed into the statement simulation 
 engine and the head pointer is set to point to the next statement in the queue.  
 The head statement is continually taken until the pre-simulation statement queue 
 is empty.  This signifies that the timestep has been completed.

 \par
 When a statement is placed into the statement simulation engine, the #ESUPPL_IS_STMT_HEAD 
 bit is cleared in the root expression.  Additionally, the root expression pointed to by 
 the statement is interrogated to see if the #ESUPPL_IS_LEFT_CHANGED or #ESUPPL_IS_RIGHT_CHANGED 
 bits are set.  If one or both of the bits are found to be set, the root expression 
 is placed into the expression simulation engine for further processing.  When the 
 statement's expression has completed its simulation, the value of the root expression 
 is used to determine if the next_true or next_false path will be taken.  If the value 
 of the root expression is true, the next_true statement is loaded into the statement 
 simulation engine.  If the value of the root expression is false and the next_false 
 pointer is NULL, this signifies that the current statement tree has completed for this 
 timestep.  At this point, the current statement will set the #ESUPPL_IS_STMT_HEAD bit in 
 its root expression and is removed from the statement simulation engine.  The next statement 
 at the head of the pre-simulation statement queue is then loaded into the statement 
 simulation engine.  If next_false statement is not NULL, it is loaded into the statement 
 simulation engine and work is done on that statement.

 \par
 When a root expression is placed into the expression simulation engine, the tree is
 traversed, following the paths that have set #ESUPPL_IS_LEFT_CHANGED or 
 #ESUPPL_IS_RIGHT_CHANGED bits set.  Each expression tree is traversed depth first.  When an 
 expression is reached that does not have either of these bits set, we have reached the expression
 whose value has changed.  When this expression is found, it is evaluated and the
 resulting value is stored into its value vector.  Once this has occurred, the parent
 expression checks to see if the other child expression has changed value.  If so,
 that child expression's tree is traversed.  Once both child expressions contain the
 current value for the current timestep, the parent expression evaluates its
 expression with the values of its children and clears both the #ESUPPL_IS_LEFT_CHANGED and
 #ESUPPL_IS_RIGHT_CHANGED bits to indicate that both children were evaluated.  The resulting
 value is stored into the current expression's value vector and the parent expression
 of the current expression is worked on.  This evaluation process continues until the
 root expression of the tree has been evaluated.  At this point the expression tree
 is removed from the expression simulation engine and the associated statement worked
 on by the statement simulation engine as specified above.
*/

#include <assert.h>

#include "defines.h"
#include "expr.h"
#include "iter.h"
#include "link.h"
#include "sim.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern nibble   or_optab[OPTAB_SIZE];
extern char     user_msg[USER_MSG_LENGTH];
extern exp_info exp_op_info[EXP_OP_NUM];
extern uint64   curr_sim_time;

/*!
 Pointer to head of expression list that contains all expressions that contain static (non-changing)
 values.  These expressions will be forced to be simulated, making sure that correct coverage numbers
 for expressions containing static values is maintained.
*/
exp_link*  static_expr_head = NULL;

/*!
 Pointer to tail of expression list that contains all expressions that contain static (non-changing)
 values.  These expressions will be forced to be simulated, making sure that correct coverage numbers
 for expressions containing static values is maintained.
*/
exp_link*  static_expr_tail = NULL;

/*!
 Pointer to head of thread list that contains all threads that will initially be simulated
 for the current timestep.
*/
thread* thread_head = NULL;

/*!
 Pointer to tail of thread list that contains all threads that will initially be simulated
 for the current timestep.
*/
thread* thread_tail = NULL;

/*!
 Pointer to head of thread list containing threads that are currently waiting for a delay period to be
 completed.
*/
thread* delay_head  = NULL;


/*!
 Displays the thread contents of the given queue.
*/
void sim_display_threads( thread* head, thread* tail ) {

  thread* curr;          /* Pointer to current thread */
  bool    done = FALSE;  /* Specifies when we are done displaying the current list */

  curr = head;
  while( (curr != NULL) && !done ) {
    printf( "    stmt %d, %s, line %d, queued %d, delayed %d (%p)  ",
            curr->curr->exp->id,
            expression_string_op( curr->curr->exp->op ),
            curr->curr->exp->line,
            curr->suppl.part.queued,
            curr->suppl.part.delayed, curr );
    if( curr == head ) {
      printf( "H" );
    }
    if( curr == tail ) {
      printf( "T" );
      done = TRUE;
    }
    printf( "\n" );
    curr = curr->next;
  }

}

/*!
 Displays the current state of the thread queue (for debug purposes only).
*/
void sim_display_thread_queue() {

  sim_display_threads( thread_head, thread_tail );

}

/*!
 Displays the current state of the delay queue (for debug purposes only).
*/
void sim_display_delay_queue() {

  thread* curr_time;  /* Pointer to current time slot */

  curr_time = delay_head;
  while( curr_time != NULL ) {
    printf( "  Time %llu\n", curr_time->curr_time );
    sim_display_threads( curr_time->child_head, curr_time->child_tail );
    curr_time = curr_time->next;
  }
  
}

/*!
 \param thr       Pointer to the thread to add to the delay queue.
 \param sim_time  Time to insert the given thread.

 This function is called by the expression_op_func__delay() function.
*/
void sim_thread_insert_into_delay_queue( thread* thr, uint64 sim_time ) {

  thread* curr_time;         /* Pointer to current thread in delay queue */
  thread* last_time = NULL;  /* Pointer to the last time slot */
  thread* new_time;          /* Pointer to newly created time thread */

  // printf( "Before delay thread is inserted for time %llu...\n", sim_time );

  if( thr != NULL ) {

    assert( thr->suppl.part.delayed == 0 );

    // sim_display_delay_queue();

    /* Search through thread queue searching for a simulation time that matches the given thread */
    curr_time = delay_head;
    while( (curr_time != NULL) && (thr->suppl.part.delayed == 0) ) {

      assert( curr_time->suppl.part.time_thread == 1 );

      /* Add this thread to the current time thread list */
      if( curr_time->curr_time == sim_time ) {
        thr->prev = curr_time->child_tail;
        curr_time->child_tail->next = thr;
        curr_time->child_tail       = thr;
        if( curr_time->child_head == NULL ) {
          curr_time->child_head = thr;
        }
        thr->suppl.part.delayed = 1;
        thr->suppl.part.queued  = 1;

      /* Advance to the next time thread */
      } else if( curr_time->curr_time < sim_time ) {
        last_time = curr_time;
        curr_time = curr_time->next;

      /* Create a new time thread and add this thread */
      } else {
        new_time = (thread*)malloc_safe( sizeof( thread ), __FILE__, __LINE__ );
        new_time->curr_time = sim_time;
        new_time->suppl.all = 0;
        new_time->suppl.part.time_thread = 1;
        new_time->prev = curr_time->prev;
        new_time->next = curr_time;
        new_time->child_head = thr;
        new_time->child_tail = thr;
        if( new_time->prev != NULL ) {
          new_time->prev->next = new_time;
        } else {
          delay_head = new_time;
        }
        curr_time->prev = new_time;
        thr->prev = NULL;
        thr->suppl.part.delayed = 1;
        thr->suppl.part.queued  = 1;
      }
    }

    /* If we did not find a time slot, create one and append it to the end of the delay queue */
    if( curr_time == NULL ) {
      new_time = (thread*)malloc_safe( sizeof( thread ), __FILE__, __LINE__ );
      new_time->curr_time = sim_time;
      new_time->suppl.all = 0;
      new_time->suppl.part.time_thread = 1;
      new_time->prev = last_time;
      new_time->next = NULL;
      new_time->child_head = thr;
      new_time->child_tail = thr;
      if( delay_head == NULL ) {
        delay_head = new_time;
      } else {
        last_time->next = new_time;
      }
      thr->prev = NULL;
      thr->suppl.part.delayed = 1;
      thr->suppl.part.queued  = 1;
    }

    // printf( "After delay thread is inserted...\n" );
    // sim_display_delay_queue();

  }

}

/*!
 \param thr       Pointer to thread to add to the tail of the simulation queue.
 \param sim_time  Current simulation time of thread to push

 Adds the specified thread to the end of the current simulation queue.  This function gets
 called whenever a head statement has a signal change or the head statement is a delay operation
 and
*/
void sim_thread_push( thread* thr, uint64 sim_time ) {

  exp_op_type op;  /* Operation type of current expression in given thread */

  // printf( "Before thread is pushed...\n" );

  /* Only add the thread if it exists and it isn't already in a queue */
  if( (thr != NULL) && (thr->suppl.part.queued == 0) ) {

    // sim_display_thread_queue();

    /* Add thread to tail-end of queue */
    if( thread_tail == NULL ) {
      thr->next = NULL;
      thr->prev = NULL;
      thread_head = thread_tail = thr;
    } else {
      thr->next         = NULL;
      thr->prev         = thread_tail;
      thread_tail->next = thr;
      thread_tail       = thr;
    }

    /* Set the queue indicator to TRUE */
    thr->suppl.part.queued = 1;

    /* Set the current time of the thread to the given value */
    op = thr->curr->exp->op;
    if( (op == EXP_OP_PEDGE)       ||
        (op == EXP_OP_NEDGE)       ||
        (op == EXP_OP_AEDGE)       ||
        (op == EXP_OP_EOR)         ||
        (op == EXP_OP_WAIT)        ||
        (op == EXP_OP_SLIST)       ||
        (op == EXP_OP_ALWAYS_COMB) ||
        (op == EXP_OP_ALWAYS_LATCH) ) {
      thr->curr_time = sim_time;
    }

    // printf( "After thread is pushed...\n" );
    // sim_display_thread_queue();

  }

}

/*!
 Pops the head thread from the thread queue without deallocating the thread.
*/
void sim_thread_pop_head() {

  thread* tmp_head = thread_head;  /* Pointer to head of thread queue */

  // printf( "Before thread is popped from thread queue...\n" );
  // sim_display_thread_queue();

  if( thread_head != NULL ) {

    /* Move the head pointer */
    thread_head = thread_head->next;

    /* If the current thread is in the delay queue, set the next pointer to NULL */
    if( tmp_head->suppl.part.delayed == 1 ) {
      tmp_head->next = NULL;

    /* Reset previous and next pointers */
    } else {
      tmp_head->next = tmp_head->prev = NULL;
      tmp_head->suppl.part.queued = 0;
    }

    /* If the queue is now empty, set tail to NULL as well */
    if( thread_head == NULL ) {
      thread_tail = NULL;
    }

  }

  // printf( "After thread is popped from thread queue...\n" );
  // sim_display_thread_queue();

}

/*!
 \param expr      Pointer to expression that contains a changed signal value.
 \param sim_time  Specifies current simulation time for the thread to push.

 Traverses up expression tree pointed to by leaf node expr, setting the
 CHANGED bits as it reaches the root expression.  When the root expression is
 found, the statement pointed to by the root's parent pointer is added to the
 pre-simulation statement queue for simulation at the end of the timestep.  If,
 upon traversing the tree, an expression is found to already be have a CHANGED
 bit set, we know that the statement has already been added, so stop here and
 do not add the statement again.
*/
void sim_expr_changed( expression* expr, uint64 sim_time ) {

  assert( expr != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In sim_expr_changed, expr %d, op %s, line %d, left_changed: %d, right_changed: %d, time: %llu",
            expr->id, expression_string_op( expr->op ), expr->line,
            ESUPPL_IS_LEFT_CHANGED( expr->suppl ),
            ESUPPL_IS_RIGHT_CHANGED( expr->suppl ),
            sim_time );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* No need to continue to traverse up tree if both CHANGED bits are set */
  if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 0) ||
      (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ||
      (expr->op == EXP_OP_COND) ) {

    /* If we are not the root expression, do the following */
    if( ESUPPL_IS_ROOT( expr->suppl ) == 0 ) {

      /* Set the appropriate CHANGED bit of the parent expression */
      if( (expr->parent->expr->left != NULL) && (expr->parent->expr->left->id == expr->id) ) {

        expr->parent->expr->suppl.part.left_changed = 1;

        /* If the parent of this expression is a CONDITIONAL, set the RIGHT_CHANGED bit of the parent too */
        if( expr->parent->expr->op == EXP_OP_COND ) {

          expr->parent->expr->suppl.part.right_changed = 1;

        }


      } else if( (expr->parent->expr->right != NULL) && (expr->parent->expr->right->id == expr->id) ) {
        
        expr->parent->expr->suppl.part.right_changed = 1;

      }

      /* Continue up the tree */
      sim_expr_changed( expr->parent->expr, sim_time );

    /*
     Otherwise, if we have hit the root expression and the parent pointer is valid, add 
     this statement (if it is the head) back onto the thread queue.
    */
    } else if( expr->parent->expr != NULL ) {

      sim_thread_push( expr->parent->stmt->thr, sim_time );

    }

    /* Set one of the changed bits to let the simulator know that it needs to evaluate the expression.  */
    if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 0) && (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ) {
      expr->suppl.part.left_changed = 1;
    }

  }

}

/*!
 \param parent  Pointer to parent thread of the new thread to create (set to NULL if there is no parent thread)
 \param stmt    Pointer to head statement to have new thread point to.

 \return Returns a pointer to the newly created thread if created; otherwise, returns NULL.

 Creates a new thread with the given information and adds the thread to the thread queue to run.  Returns a pointer
 to the newly created thread for joining/running purposes.
*/
thread* sim_add_thread( thread* parent, statement* stmt ) {

  thread* thr         = NULL;   /* Pointer to new thread to create */
  bool    first_child = FALSE;  /* Specifies if this is the first child to be added to the parent */

  assert( stmt != NULL );

  /* Only add expression if it is the head statement of its statement block */
  if( ESUPPL_IS_STMT_HEAD( stmt->exp->suppl ) == 1 ) {

    /* Create and initialize thread */
    thr                    = (thread*)malloc_safe( sizeof( thread ), __FILE__, __LINE__ );
    thr->parent            = parent;
    thr->head              = stmt;
    thr->curr              = stmt;
    thr->suppl.all         = 0;
    thr->suppl.part.queued = 1;    /* We will place the thread immediately into the thread queue */
    thr->curr_time         = curr_sim_time;
    thr->child_head        = NULL;
    thr->child_tail        = NULL;
    thr->prev_sib          = NULL;
    thr->next_sib          = NULL;
    thr->prev              = NULL;
    thr->next              = NULL;

    /* Set statement pointer to this thread */
    stmt->thr        = thr;
    stmt->static_thr = thr;

    /* If the parent thread is specified, add this thread to its list of children */
    if( parent != NULL ) {
      if( parent->child_head == NULL ) {
        parent->child_head = parent->child_tail = thr;
        first_child = TRUE;
      } else {
        thr->prev_sib                = parent->child_tail;
        parent->child_tail->next_sib = thr;
        parent->child_tail           = thr;
      }
      thr->curr_time    = parent->curr_time;
    }

    /* Add this thread to the simulation thread queue */
    if( parent != NULL ) {

      /* We are not the first statement since we are a child */
      thr->suppl.part.exec_first = 0;

      /* Insert this child between the parent and its next thread */
      thr->prev    = parent;
      thr->next    = parent->next;
      parent->next = thr;

      /* Fix the next thread to point to us */
      if( thr->next == NULL ) {
        thread_tail = thr;
      } else {
        thr->next->prev = thr;
      }

    } else {

      /* We are the first statement */
      thr->suppl.part.exec_first = 1;

      /*
       If this statement is an always_comb or always_latch, add it to the delay list and change its right
       expression so that it will be executed at time 0 after all initial and always blocks have completed
      */
      if( (stmt->exp->op == EXP_OP_ALWAYS_COMB) || (stmt->exp->op == EXP_OP_ALWAYS_LATCH) ) {

        /* Add this thread into the delay queue at time 0 */
        sim_thread_insert_into_delay_queue( thr, 0 );

        /* Specify that this block should be evaluated */
        stmt->exp->right->suppl.part.eval_t = 1;

      } else {

        /* If the statement block is specified as a final block, add it to the end of the delay queue */
        if( ESUPPL_STMT_FINAL( stmt->exp->suppl ) == 1 ) {
          sim_thread_insert_into_delay_queue( thr, 0xffffffffffffffff );

        /* Otherwise, add it to the normal thread list */
        } else {
          if( thread_head == NULL ) {
            thread_head = thread_tail = thr;
          } else {
            thr->prev         = thread_tail;
            thread_tail->next = thr;
            thread_tail       = thr;
          }
        }

      }
 
    }

    // printf( "After thread is added to thread queue...\n" );
    // sim_display_thread_queue();

  }

  return( thr );

}

/*!
 \param thr  Thread to remove from simulation

 Removes the specified thread from its parent and the thread simulation queue and finally deallocates
 the specified thread.
*/
void sim_kill_thread( thread* thr ) {

  bool last_child = FALSE;  /* Specifies if we are the last child being removed from parent thread */

  assert( thr != NULL );

  /* Remove this thread from its parent, if it has a parent */
  if( thr->parent != NULL ) {

    /* If this thread is the only child of its parent, make the parent child empty */
    if( (thr == thr->parent->child_head) && (thr == thr->parent->child_tail) ) {

      thr->parent->child_head = thr->parent->child_tail = NULL;
      last_child = TRUE;

    /* If this thread is the head, we need to bump up the head pointer */
    } else if( thr == thr->parent->child_head ) {
 
      thr->parent->child_head           = thr->next_sib;
      thr->parent->child_head->prev_sib = NULL;

    /* If this thread is the tail, we need to adjust the tail pointer */
    } else if( thr == thr->parent->child_tail ) {
  
      thr->parent->child_tail           = thr->prev_sib;
      thr->parent->child_tail->next_sib = NULL;

    /* Otherwise, we need to to adjust the pointers within the list */
    } else {

      thr->prev_sib->next_sib = thr->next_sib;
      thr->next_sib->prev_sib = thr->prev_sib;

    }

  }

  /* If we are the last child, re-insert the parent in our place (setting thread_head to the parent) */
  if( last_child ) {
    thr->parent->next = thr->next;
    if( thr->parent->next == NULL ) {
      thread_tail = thr->parent;
    } else {
      thr->parent->next->prev = thr->parent;
    }
    thread_head = thr->parent;
    thr->parent->curr_time = thr->curr_time;
  } else {
    thread_head = thread_head->next;
    if( thread_head == NULL ) {
      thread_tail = NULL;
    }
  }

  /* Set the statement thread pointer to NULL */
  thr->curr->thr = NULL;

  /* Now we can deallocate the thread */
  free_safe( thr );

}

/*!
 \param stmt  Pointer to head statement of thread to kill

 Searches the current state of the thread queue for the thread containing the specified head statement.
 If a thread was found to match, kill it.  This function is called whenever the DISABLE statement is
 run.
*/
void sim_kill_thread_with_stmt( statement* stmt ) {

  thread* child;  /* Pointer to current child being examined */

  assert( stmt != NULL );
  assert( stmt->static_thr != NULL );

  /* Specify that this thread should be killed */
  stmt->static_thr->suppl.part.kill = 1;

  /* If this thread is a parent to other threads, kill all children */
  child = stmt->static_thr->child_head;
  while( child != NULL ) {
    child->suppl.part.kill = 1;
    child                  = child->next_sib;
  }

}

#ifdef OBSOLETE
/*!
 Kills all threads in the simulator.  This is used at the end of a simulation run.
*/
void sim_kill_all_threads() {

  while( thread_head != NULL ) {
    sim_kill_thread( thread_head );
  }
    
}
#endif

/*!
 Iterates through static expression list and causes the simulator to
 evaluate these expressions at simulation time.
*/
void sim_add_statics() {
  
  exp_link* curr;   /* Pointer to current expression link */
  
  curr = static_expr_head;
  while( curr != NULL ) {
    sim_expr_changed( curr->exp, 0 );
    curr = curr->next;
  }
  
  exp_link_delete_list( static_expr_head, FALSE );
  
}

/*!
 \param expr  Pointer to expression to simulate.
 \param thr   Pointer to current thread that is being simulated.

 \return Returns TRUE if this expression has changed value from previous sim; otherwise,
         returns FALSE.

 Recursively traverses specified expression tree, following the #ESUPPL_IS_LEFT_CHANGED 
 and #ESUPPL_IS_RIGHT_CHANGED bits in the supplemental field.  Once an expression is
 found that has neither bit set, perform the expression operation and move back up
 the tree.  Once both left and right children have calculated values, perform the
 expression operation for the current expression, clear both changed bits and
 return.
*/
bool sim_expression( expression* expr, thread* thr ) {

  bool retval        = FALSE;  /* Return value for this function */
  bool left_changed  = FALSE;  /* Signifies if left expression tree has changed value */
  bool right_changed = FALSE;  /* Signifies if right expression tree has changed value */

  assert( expr != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "    In sim_expression %d, left_changed %d, right_changed %d",
            expr->id, ESUPPL_IS_LEFT_CHANGED( expr->suppl ), ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Traverse left child expression if it has changed */
  if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 1) ||
      (expr->op == EXP_OP_CASE)                    ||
      (expr->op == EXP_OP_CASEX)                   ||
      (expr->op == EXP_OP_CASEZ) ) {

    /* Simulate the left expression if it has changed */
    if( expr->left != NULL ) {
      if( expr->left->suppl.part.lhs == 0 ) {
        left_changed = sim_expression( expr->left, thr );
      }
    } else {
      left_changed = TRUE;
    }

    /* Clear LEFT CHANGED bit */
    expr->suppl.part.left_changed = 0;

  }

  /* Traverse right child expression if it has changed */
  if( (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 1) &&
      ((expr->op != EXP_OP_DLY_OP) || !thr->suppl.part.exec_first) ) {

    /* Simulate the right expression if it has changed */
    if( expr->right != NULL ) {
      if( expr->right->suppl.part.lhs == 0 ) {
        right_changed = sim_expression( expr->right, thr );
      }
    } else {
      right_changed = TRUE;
    } 

    /* Clear RIGHT CHANGED bit */
    expr->suppl.part.right_changed = 0;

  }

  /*
   Now perform expression operation for this expression if left or right
   expressions trees have changed.
  */
  if( (ESUPPL_IS_STMT_CONTINUOUS( expr->suppl ) == 0) || left_changed || right_changed ) {
    retval = expression_operate( expr, thr );
  }

  return( retval );

}

/*!
 \param thr  Pointer to current thread to simulate.

 Performs statement simulation as described above.  Calls expression simulator if
 the associated root expression is specified that signals have changed value within
 it.  Continues to run for current statement tree until statement tree hits a
 wait-for-event condition (or we reach the end of a simulation tree).
*/
void sim_thread( thread* thr ) {

  statement* stmt;                  /* Pointer to current statement to evaluate */
  bool       expr_changed = FALSE;  /* Specifies if expression tree was modified in any way */

  /* Set the value of stmt with the head_stmt */
  stmt = thr->curr;

  /* Clear the delayed supplemental field of the current thread */
  thr->suppl.part.delayed = 0;

  while( (stmt != NULL) && !thr->suppl.part.kill ) {

    /* Remove the pointer to the current thread from the last statement */
    thr->curr->thr = NULL;

    /* Set current statement thread pointer to current thread */
    stmt->thr = thr;

    /* Place expression in expression simulator and run */
    expr_changed = sim_expression( stmt->exp, thr );

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "  Executed statement %d, expr changed %d", stmt->exp->id, expr_changed );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
      
    thr->curr = stmt;

    /* Set exec_first to FALSE */
    thr->suppl.part.exec_first = 0;

    if( ESUPPL_IS_STMT_CONTINUOUS( stmt->exp->suppl ) == 1 ) {
       /* If this is a continuous assignment, don't traverse next pointers. */
       stmt = NULL;
    } else {
      if( ESUPPL_IS_TRUE( stmt->exp->suppl ) == 1 ) {
        stmt = stmt->next_true;
      } else {
        stmt = stmt->next_false;
      }
    }

  }

  /* If this is the last statement in the tree with no loopback, kill the current thread */
  if( (expr_changed && 
      (((thr->curr->next_true == NULL) && (thr->curr->next_false == NULL)) ||
       (!EXPR_IS_CONTEXT_SWITCH( thr->curr->exp ) && !ESUPPL_IS_STMT_CONTINUOUS( thr->curr->exp->suppl )))) ||
      thr->suppl.part.kill ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Completed thread %p, killing...\n", thr );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
 
    /* Destroy the thread */
    sim_kill_thread( thr );

  /* Otherwise, we are switching contexts */
  } else {

    /* Set exec_first to TRUE for next run */
    thr->suppl.part.exec_first = 1;

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Switching context of thread %p...\n", thr );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    /* Pop this packet out of the thread queue */
    sim_thread_pop_head();

  }

}

/*!
 \param sim_time  Current simulation time from dumpfile or simulator.

 This function is the heart of the simulation engine.  It is called by the
 db_do_timestep() function in db.c  and moves the statements and expressions into
 the appropriate simulation functions.  See above explanation on this procedure.
*/
void sim_simulate( uint64 sim_time ) {

  thread* tmp;  /* Temporary pointer to the current thread */

  /* Simulate all threads in the thread queue */
  while( thread_head != NULL ) {
    sim_thread( thread_head );
  }

  /* Now simulate all threads in the delay queue up to the current simulation time */
  while( (delay_head != NULL) && (delay_head->curr_time <= sim_time) ) {

    assert( delay_head->suppl.part.time_thread == 1 );

    /* Place the current time slot queue into thread queue */
    while( delay_head->child_head != NULL ) {
      thread_head = delay_head->child_head;
      thread_tail = delay_head->child_tail;
      delay_head->child_head = NULL;
      delay_head->child_tail = NULL;
      while( thread_head != NULL ) {
        sim_thread( thread_head );
      }
    }

    /* Now deallocate the current time slot */
    tmp = delay_head;
    delay_head = delay_head->next;
    if( delay_head != NULL ) {
      delay_head->prev = NULL;
    }
    free_safe( tmp );

    // printf( "After delay simulation...\n" );
    // sim_display_delay_queue();

  }
    
}


/*
 $Log$
 Revision 1.78  2006/11/28 16:39:46  phase1geo
 More updates for regressions due to changes in delay handling.  Still work
 to go.

 Revision 1.77  2006/11/27 04:11:42  phase1geo
 Adding more changes to properly support thread time.  This is a work in progress
 and regression is currently broken for the moment.  Checkpointing.

 Revision 1.76  2006/11/24 05:30:15  phase1geo
 Checking in fix for proper handling of delays.  This does not include the use
 of timescales (which will be fixed next).  Full IV regression now passes.

 Revision 1.75  2006/11/22 20:20:01  phase1geo
 Updates to properly support 64-bit time.  Also starting to make changes to
 simulator to support "back simulation" for when the current simulation time
 has advanced out quite a bit of time and the simulator needs to catch up.
 This last feature is not quite working at the moment and regressions are
 currently broken.  Checkpointing.

 Revision 1.74  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.73  2006/10/06 17:18:13  phase1geo
 Adding support for the final block type.  Added final1 diagnostic to regression
 suite.  Full regression passes.

 Revision 1.72  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.71  2006/08/21 22:50:01  phase1geo
 Adding more support for delayed assignments.  Added dly_assign1 to testsuite
 to verify the #... type of delayed assignment.  This seems to be working for
 this case but has a potential issue in the report generation.  Checkpointing
 work.

 Revision 1.70  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.69  2006/08/11 18:57:04  phase1geo
 Adding support for always_comb, always_latch and always_ff statement block
 types.  Added several diagnostics to regression suite to verify this new
 behavior.

 Revision 1.68  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.67  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.66  2006/01/23 17:23:28  phase1geo
 Fixing scope issues that came up when port assignment was added.  Full regression
 now passes.

 Revision 1.65  2006/01/09 04:15:25  phase1geo
 Attempting to fix one last problem with latest changes.  Regression runs are
 currently running.  Checkpointing.

 Revision 1.64  2006/01/08 05:51:03  phase1geo
 Added optimizations to EOR and AEDGE expressions.  In the process of running
 regressions...

 Revision 1.63  2006/01/08 03:05:06  phase1geo
 Checkpointing work on optimized thread handling.  I believe that this is now
 working as wanted; however, regressions will not pass until EOR optimization
 has been completed.  I will be working on this next.

 Revision 1.62  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.61  2006/01/06 18:54:03  phase1geo
 Breaking up expression_operate function into individual functions for each
 expression operation.  Also storing additional information in a globally accessible,
 constant structure array to increase performance.  Updating full regression for these
 changes.  Full regression passes.

 Revision 1.60  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.59  2006/01/04 22:07:04  phase1geo
 Changing expression execution calculation from sim to expression_operate function.
 Updating all regression files for this change.  Modifications to diagnostic Makefile
 to accommodate environments that do not have valgrind.

 Revision 1.58  2006/01/03 23:00:18  phase1geo
 Removing debugging output from last checkin.

 Revision 1.57  2006/01/03 22:59:16  phase1geo
 Fixing bug in expression_assign function -- removed recursive assignment when
 the LHS expression is a signal, single-bit, multi-bit or static value (only
 recurse when the LHS is a CONCAT or LIST).  Fixing bug in db_close function to
 check if the instance tree has been populated before deallocating memory for it.
 Fixing bug in report help information when Tcl/Tk is not available.  Added bassign2
 diagnostic to regression suite to verify first described bug.

 Revision 1.56  2005/12/31 05:00:57  phase1geo
 Updating regression due to recent changes in adding exec_num field in expression
 and removing the executed bit in the expression supplemental field.  This will eventually
 allow us to get information on where the simulator is spending the most time.

 Revision 1.55  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.54  2005/12/07 21:50:51  phase1geo
 Added support for repeat blocks.  Added repeat1 to regression and fixed errors.
 Full regression passes.

 Revision 1.53  2005/12/05 23:30:35  phase1geo
 Adding support for disabling tasks.  Full regression passes.

 Revision 1.52  2005/12/05 22:45:39  phase1geo
 Bug fixes to disable code when disabling ourselves -- we move thread killing
 to be done by the sim_thread routine only.

 Revision 1.51  2005/12/05 22:02:24  phase1geo
 Added initial support for disable expression.  Added test to verify functionality.
 Full regression passes.

 Revision 1.50  2005/11/30 18:25:56  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.49  2005/11/29 19:04:48  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.48  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.47  2005/11/22 05:30:33  phase1geo
 Updates to regression suite for clearing the assigned bit when a statement
 block is removed from coverage consideration and it is assigning that signal.
 This is not fully working at this point.

 Revision 1.46  2005/11/21 23:31:06  phase1geo
 Adding support for initial statement blocks.  Added first initial1 diagnostic
 to regression suite.  This support seems to be working for the moment -- still
 need to run regressions.

 Revision 1.45  2005/11/21 22:21:58  phase1geo
 More regression updates.  Also made some updates to debugging output.

 Revision 1.44  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.43  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.42  2005/11/18 05:17:01  phase1geo
 Updating regressions with latest round of changes.  Also added bit-fill capability
 to expression_assign function -- still more changes to come.  We need to fix the
 expression sizing problem for RHS expressions of assignment operators.

 Revision 1.41  2005/11/17 23:35:16  phase1geo
 Blocking assignment is now working properly along with support for event expressions
 (currently only the original PEDGE, NEDGE, AEDGE and DELAY are supported but more
 can now follow).  Added new race4 diagnostic to verify that a register cannot be
 assigned from more than one location -- this works.  Regression fails at this point.

 Revision 1.40  2005/11/17 05:34:44  phase1geo
 Initial work on supporting blocking assignments.  Added new diagnostic to
 check that this initial work is working correctly.  Quite a bit more work to
 do here.

 Revision 1.39  2005/02/08 23:18:23  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.38  2005/01/07 17:59:52  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.37  2004/11/08 12:35:34  phase1geo
 More updates to improve efficiency.

 Revision 1.36  2004/03/30 15:42:15  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.35  2003/11/29 06:55:49  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.34  2003/11/05 05:22:56  phase1geo
 Final fix for bug 835366.  Full regression passes once again.

 Revision 1.33  2003/10/14 04:02:44  phase1geo
 Final fixes for new FSM support.  Full regression now passes.  Need to
 add new diagnostics to verify new functionality, but at least all existing
 cases are supported again.

 Revision 1.32  2003/10/13 22:10:07  phase1geo
 More changes for FSM support.  Still not quite there.

 Revision 1.31  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.30  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.29  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.28  2002/11/27 06:03:35  phase1geo
 Adding diagnostics to verify selectable delay.  Removing selectable delay
 warning from being output constantly to only outputting when selectable delay
 found in design and -T option not specified.  Full regression passes.

 Revision 1.27  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.26  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.25  2002/10/31 23:14:24  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.24  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.23  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.22  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.21  2002/09/19 05:25:20  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.20  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.19  2002/07/10 03:01:50  phase1geo
 Added define1.v and define2.v diagnostics to regression suite.  Both diagnostics
 now pass.  Fixed cases where constants were not causing proper TRUE/FALSE values
 to be calculated.

 Revision 1.18  2002/07/05 00:37:37  phase1geo
 Small update to CASE handling in scope to avoid future errors.

 Revision 1.17  2002/07/03 21:30:53  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.16  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.15  2002/07/03 00:59:14  phase1geo
 Fixing bug with conditional statements and other "deep" expression trees.

 Revision 1.14  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.13  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.12  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.11  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.10  2002/06/28 00:40:37  phase1geo
 Cleaning up extraneous output from debugging.

 Revision 1.9  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.8  2002/06/26 04:59:50  phase1geo
 Adding initial support for delays.  Support is not yet complete and is
 currently untested.

 Revision 1.7  2002/06/26 03:45:48  phase1geo
 Fixing more bugs in simulator and report functions.  About to add support
 for delay statements.

 Revision 1.6  2002/06/25 21:46:10  phase1geo
 Fixes to simulator and reporting.  Still some bugs here.

 Revision 1.5  2002/06/25 03:39:03  phase1geo
 Fixed initial scoring bugs.  We now generate a legal CDD file for reporting.
 Fixed some report bugs though there are still some remaining.

 Revision 1.4  2002/06/23 21:18:22  phase1geo
 Added appropriate statement support in parser.  All parts should be in place
 and ready to start testing.

 Revision 1.3  2002/06/22 21:08:23  phase1geo
 Added simulation engine and tied it to the db.c file.  Simulation engine is
 currently untested and will remain so until the parser is updated correctly
 for statements.  This will be the next step.
*/

