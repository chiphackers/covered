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
 \file     sim.c
 \author   Trevor Williams  (phase1geo@gmail.com)
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
 makes its way to the root.  When at the root expression, the statement's head
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
 When a statement is placed into the statement simulation engine, the head
 bit is cleared in the root expression.  Additionally, the root expression pointed to by 
 the statement is interrogated to see if the #ESUPPL_IS_LEFT_CHANGED or #ESUPPL_IS_RIGHT_CHANGED 
 bits are set.  If one or both of the bits are found to be set, the root expression 
 is placed into the expression simulation engine for further processing.  When the 
 statement's expression has completed its simulation, the value of the root expression 
 is used to determine if the next_true or next_false path will be taken.  If the value 
 of the root expression is true, the next_true statement is loaded into the statement 
 simulation engine.  If the value of the root expression is false and the next_false 
 pointer is NULL, this signifies that the current statement tree has completed for this 
 timestep.  At this point, the current statement will set the head bit in 
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

#include <stdio.h>
#include <assert.h>
#include <signal.h>

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
#include "cli.h"
#endif
#endif
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "instance.h"
#include "link.h"
#include "reentrant.h"
#include "sim.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern char                  user_msg[USER_MSG_LENGTH];
extern bool                  debug_mode;
extern exp_info              exp_op_info[EXP_OP_NUM];
extern /*@null@*/inst_link*  inst_head;
extern bool                  flag_use_command_line_debug;
#ifdef DEBUG_MODE
#ifndef VPI_ONLY
extern bool                  cli_debug_mode;
#endif
#endif


/*!
 Pointer to expression array that contains all expressions that contain static (non-changing)
 values.  These expressions will be forced to be simulated, making sure that correct coverage numbers
 for expressions containing static values is maintained.
*/
expression** static_exprs = NULL;

/*!
 Contains the number of elements in the static_exprs array.
*/
unsigned int static_expr_size = 0;

/*!
 Head of list of all allocated threads.
*/
static thread* all_head = NULL;

/*!
 Tail of list of all allocated threads.
*/
static thread* all_tail = NULL;

/*!
 Pointer to next thread to allocate.
*/
static thread* all_next = NULL;

/*!
 Pointer to head of active thread list.
*/
static thread* active_head  = NULL;

/*!
 Pointer to tail of active thread list.
*/
static thread* active_tail  = NULL;

/*!
 Pointer to head of delayed thread list.
*/
static thread* delayed_head = NULL;

/*!
 Pointer to tail of delayed thread list.
*/
static thread* delayed_tail = NULL;

/*!
 List of thread state string names.
*/
static const char* thread_state_str[4] = {"NONE", "ACTIVE", "DELAYED", "WAITING"};

/*!
 Global variable used to cause simulator to stop simulation.  Do not directly modify this variable!
*/
static bool simulate = TRUE;

/*!
 Causes simulation to stop and invoke the CLI prompt, if possible.
*/
static bool force_stop = FALSE;

/*!
 Non-blocking assignment queue.
*/
static nonblock_assign** nba_queue = NULL;

/*!
 The allocated size of the non-blocking assignment queue.
*/
int nba_queue_size = 0;

/*!
 The current number of nba structures in the nba_queue.
*/
static int nba_queue_curr_size = 0;


/*!
 Displays the contents of the given thread to standard output.
*/
void sim_display_thread(
  const thread* thr,         /*!< Pointer to thread to display to standard output */
  bool          show_queue,  /*!< If set to TRUE, displays queue_prev/queue_next; otherwise, displays all_prev/all_next */
  bool          endl         /*!< If set to TRUE, prints a newline character */
) {

  if( !endl ) {
    printf( "    " );
  }

  /*@-duplicatequals -formattype -formatcode@*/
  printf( "time %" FMT64 "u, ", thr->curr_time.full );
  /*@=duplicatequals =formattype =formatcode@*/

  if( thr->curr == NULL ) {
    printf( "stmt NONE, " );
  } else {
    printf( "stmt %d, ", thr->curr->exp->id );
    printf( "%s, ", expression_string_op( thr->curr->exp->op ) );
    printf( "line %u, ", thr->curr->exp->line );
  }

  printf( "state %s ", thread_state_str[thr->suppl.part.state] );
  printf( "(%p, ", thr );
  printf( "parent=%p, ", thr->parent );
  printf( "prev=%p, ", (show_queue ? thr->queue_prev : thr->all_prev) );
  printf( "next=%p)  ", (show_queue ? thr->queue_next : thr->all_next) );

  if( endl ) {
    printf( "\n" );
  }

}

/*!
 Displays the current state of the active queue (for debug purposes only).
*/
static void sim_display_queue(
  thread* queue_head,  /*!< Pointer to head of queue to display */
  thread* queue_tail   /*!< Pointer to tail of queue to display */
) {

  thread* thr;  /* Pointer to current thread */

  thr = queue_head;
  while( thr != NULL ) {
    sim_display_thread( thr, TRUE, FALSE );
    if( thr == queue_head ) {
      printf( "H" );
    }
    if( thr == queue_tail ) {
      printf( "T" );
    }
    printf( "\n" );
    thr = thr->queue_next;
  }

}

/*!
 Displays the current state of the active queue (for debug purposes only).
*/
void sim_display_active_queue() {

  sim_display_queue( active_head, active_tail );

}

/*!
 Displays the current state of the delay queue (for debug purposes only).
*/
void sim_display_delay_queue() {

  sim_display_queue( delayed_head, delayed_tail );

}

/*!
 Displays the current state of the all_threads list (for debug purposes only).
*/
void sim_display_all_list() {

  thread* thr;  /* Pointer to current thread */

  printf( "ALL THREADS:\n" );

  thr = all_head;
  while( thr != NULL ) {
    sim_display_thread( thr, FALSE, FALSE );
    if( thr == all_head ) {
      printf( "H" );
    }
    if( thr == all_tail ) {
      printf( "T" );
    }
    if( thr == all_next ) {
      printf( "N" );
    }
    printf( "\n" );
    thr = thr->all_next;
  }

}

/*!
 \return Returns a pointer to the current head of the active thread queue.
*/
thread* sim_current_thread() { PROFILE(SIM_CURRENT_THREAD);

  return( active_head );

}

/*!
 Pops the head thread from the active queue without deallocating the thread.
*/
static void sim_thread_pop_head() { PROFILE(SIM_THREAD_POP_HEAD);

  thread* thr = active_head;  /* Pointer to head of active queue */

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before thread is popped from active queue...\n" );
    sim_display_active_queue();
  }
#endif

  /* Move the head pointer */
  active_head = active_head->queue_next;
  if( active_head == NULL ) {
    active_tail = NULL;
  } else {
    active_head->queue_prev = NULL;   /* TBD - Placed here for help in debug */
  }

  /* Advance the curr pointer if we call sim_add_thread */
  if( (thr->curr->exp->op == EXP_OP_FORK)      ||
      (thr->curr->exp->op == EXP_OP_JOIN)      ||  /* TBD */
      (thr->curr->exp->op == EXP_OP_FUNC_CALL) ||
      (thr->curr->exp->op == EXP_OP_TASK_CALL) ||
      (thr->curr->exp->op == EXP_OP_NB_CALL) ) {
    thr->curr = thr->curr->next_true;
    thr->suppl.part.state = THR_ST_NONE;
  } else {
    thr->suppl.part.state      = THR_ST_WAITING;
    thr->suppl.part.exec_first = 1; 
  }

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "After thread is popped from active queue...\n" );
    sim_display_active_queue();
  }
#endif

  PROFILE_END;

}

/*!
 This function is called by the expression_op_func__delay() function.
*/
void sim_thread_insert_into_delay_queue(
  thread*         thr,  /*!< Pointer to the thread to add to the delay queue */
  const sim_time* time  /*!< Pointer to time to insert the given thread */
) { PROFILE(SIM_THREAD_INSERT_INTO_DELAY_QUEUE);

  thread* curr;  /* Pointer to current thread in delayed queue to compare against */

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before delay thread is inserted for time %" FMT64 "u...\n", time->full );
  }
#endif

  if( thr != NULL ) {

    assert( thr->suppl.part.state != THR_ST_DELAYED );

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      sim_display_delay_queue();
    }
#endif

    /* If the thread is currently in the active state, remove it from the active queue now */
    if( thr->suppl.part.state == THR_ST_ACTIVE ) {
 
      /* Move the head pointer */
      active_head = active_head->queue_next;
      if( active_head == NULL ) {
        active_tail = NULL;
      } else {
        active_head->queue_prev = NULL;   /* TBD - Placed here for help in debug */
      }

    }

    /* Specify that the thread is queued and delayed */
    thr->suppl.part.state = THR_ST_DELAYED;

    /* Set the thread simulation time to the given time */
    thr->curr_time = *time;

    /* Add the given thread to the delayed queue in simulation time order */
    if( delayed_head == NULL ) {
      delayed_head = delayed_tail = thr;
      thr->queue_prev = NULL;
      thr->queue_next = NULL;
    } else {
      curr = delayed_tail;
      while( (curr != NULL) && TIME_CMP_GT(curr->curr_time, *time) ) {
        curr = curr->queue_prev;
      }
      if( curr == NULL ) {
        thr->queue_prev          = NULL;
        thr->queue_next          = delayed_head;
        delayed_head->queue_prev = thr;
        delayed_head             = thr;
      } else if( curr == delayed_tail ) {
        thr->queue_prev          = delayed_tail;
        thr->queue_next          = NULL;
        delayed_tail->queue_next = thr;
        delayed_tail             = thr;
      } else {
        thr->queue_prev             = curr;
        thr->queue_next             = curr->queue_next;
        thr->queue_next->queue_prev = thr;
        curr->queue_next            = thr;
      }
    }
    
#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      printf( "After delay thread is inserted...\n" );
      sim_display_delay_queue();
      sim_display_all_list();
    }
#endif

  }

  PROFILE_END;

}

/*!
 Adds the specified thread to the end of the current simulation queue.  This function gets
 called whenever a head statement has a signal change or the head statement is a delay operation
 and
*/
void sim_thread_push(
  thread*         thr,  /*!< Pointer to thread to add to the tail of the simulation queue */
  const sim_time* time  /*!< Current simulation time of thread to push */
) { PROFILE(SIM_THREAD_PUSH);

  exp_op_type op;  /* Operation type of current expression in given thread */

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before thread is pushed...\n" );
    sim_display_active_queue();
  }
#endif

  /* Set the state to ACTIVE */
  thr->suppl.part.state = THR_ST_ACTIVE;

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
    thr->curr_time = *time;
  }

  /* Set the active next and prev pointers to NULL */
  thr->queue_prev = thr->queue_next = NULL;

  /* Add the given thread to the end of the active_threads queue */
  if( active_head == NULL ) {
    active_head = active_tail = thr;
  } else {
    thr->queue_prev         = active_tail;
    active_tail->queue_next = thr;
    active_tail             = thr;
  }

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "After thread is pushed...\n" );
    sim_display_active_queue();
    sim_display_all_list();
  }
#endif

  PROFILE_END;

}

/*!
 Traverses up expression tree pointed to by leaf node expr, setting the
 CHANGED bits as it reaches the root expression.  When the root expression is
 found, the statement pointed to by the root's parent pointer is added to the
 pre-simulation statement queue for simulation at the end of the timestep.  If,
 upon traversing the tree, an expression is found to already be have a CHANGED
 bit set, we know that the statement has already been added, so stop here and
 do not add the statement again.
*/
void sim_expr_changed(
  expression*     expr,  /*!< Pointer to expression that contains a changed signal value */
  const sim_time* time   /*!< Specifies current simulation time for the thread to push */
) { PROFILE(SIM_EXPR_CHANGED);

  assert( expr != NULL );

  /* Set my left_changed bit to indicate to sim_expression that it should evaluate me */
  expr->suppl.part.left_changed = 1;

  while( ESUPPL_IS_ROOT( expr->suppl ) == 0 ) {

    expression* parent = expr->parent->expr;

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In sim_expr_changed, expr %d, op %s, line %d, left_changed: %d, right_changed: %d, time: %" FMT64 "u",
                                  expr->id, expression_string_op( expr->op ), expr->line,
                                  ESUPPL_IS_LEFT_CHANGED( expr->suppl ),
                                  ESUPPL_IS_RIGHT_CHANGED( expr->suppl ),
                                  time->full );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* If our expression is on the left of the parent, set the left_changed as needed */
    if( (parent->left != NULL) && (parent->left->id == expr->id) ) {
      
      /* If the bit we need to set is already set, stop iterating up tree */
      if( ESUPPL_IS_LEFT_CHANGED( parent->suppl ) == 1 ) {
        break;
      } else {
        parent->suppl.part.left_changed = 1;
        if( parent->op == EXP_OP_COND ) {
          parent->suppl.part.right_changed = 1;
        }
      }

    /* Otherwise, we assume that we match the right side */
    } else {

      /* If the bit we need to set is already set, stop iterating up tree */
      if( ESUPPL_IS_RIGHT_CHANGED( parent->suppl ) == 1 ) {
        break;
      } else {
        parent->suppl.part.right_changed = 1;
      }

    }

    expr = parent;

  }

  /* If we reached the root expression, push our thread onto the active queue */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 1) && (expr->parent->stmt != NULL) ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "In sim_expr_changed, expr %d, op %s, line %d, left_changed: %d, right_changed: %d, time: %" FMT64 "u",
                                  expr->id, expression_string_op( expr->op ), expr->line,
                                  ESUPPL_IS_LEFT_CHANGED( expr->suppl ),
                                  ESUPPL_IS_RIGHT_CHANGED( expr->suppl ),
                                  time->full );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    funit_push_threads( expr->parent->stmt->funit, expr->parent->stmt, time );

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the newly allocated and initialized thread

 Allocates a new thread for simulation purposes and initializes the thread structure with
 everything that can be done at time 0.  This function does not place the thread into any
 queues (this is left to the sim_add_thread function).
*/
static thread* sim_create_thread(
  thread*    parent,  /*!< Pointer to parent thread (if one exists) of the newly created thread */
  statement* stmt,    /*!< Pointer to the statement that is the head statement of the thread's block */
  func_unit* funit    /*!< Pointer to functional unit containing the new thread */
) { PROFILE(SIM_CREATE_THREAD);

  thread* thr;  /* Pointer to newly allocated thread */

  /* If the next thread to use is empty, create a new one and add it to the end of the all pool */
  if( all_next == NULL ) {

    /* Allocate the new thread */
    thr           = (thread*)malloc_safe( sizeof( thread ) );
    thr->all_prev = NULL;
    thr->all_next = NULL;

    /* Place newly allocated thread in the all_threads pool */
    if( all_head == NULL ) {
      all_head = all_tail = thr;
    } else {
      thr->all_prev      = all_tail;
      all_tail->all_next = thr;
      all_tail           = thr;
    }

  /* Otherwise, select the next thread and advance the all_next pointer */
  } else {

    thr = all_next; 
    all_next = all_next->all_next;

  }

  /* Initialize the contents of the thread */
  thr->funit           = funit;
  thr->parent          = parent;
  thr->curr            = stmt;
  thr->ren             = NULL;
  thr->suppl.all       = 0;  /* Sets the current state of the thread to NONE */
  thr->curr_time.lo    = 0;
  thr->curr_time.hi    = 0;
  thr->curr_time.full  = 0LL;
  thr->curr_time.final = FALSE;
  thr->queue_prev      = NULL;
  thr->queue_next      = NULL;

  /* Add this thread to the given functional unit */
  funit_add_thread( funit, thr );

  PROFILE_END;

  return( thr );

}

/*!
 \return Returns the pointer to the thread that was added to the active queue (if one was added).

 Creates a new thread with the given information and adds the thread to the active queue to run.  Returns a pointer
 to the newly created thread for joining/running purposes.
*/
thread* sim_add_thread(
  thread*         parent,  /*!< Pointer to parent thread of the new thread to create (set to NULL if there is no parent thread) */
  statement*      stmt,    /*!< Pointer to head statement to have new thread point to */
  func_unit*      funit,   /*!< Pointer to functional unit that is creating this thread */
  const sim_time* time     /*!< Pointer to current simulation time */
) { PROFILE(SIM_ADD_THREAD);

  thread* thr = NULL;  /* Pointer to added thread */

  /* Only add expression if it is the head statement of its statement block */
  if( stmt->suppl.part.head == 1 ) {

    /* Create thread, if needed */
    thr = sim_create_thread( parent, stmt, funit );

    /* Initialize thread runtime components */
    thr->suppl.all       = 0;
    thr->active_children = 0;
    thr->queue_prev      = NULL;
    thr->queue_next      = NULL;

    /*
     If the parent thread is specified, update our current time, increment the number of active children in the parent
     and add ourselves between the parent thread and its next pointer.
    */
    if( thr->parent != NULL ) {

      thr->curr_time = thr->parent->curr_time;
      thr->parent->active_children++;

      /* Place ourselves between the parent and its queue_next pointer */
      thr->queue_next = thr->parent->queue_next;
      thr->parent->queue_next = thr;
      if( thr->queue_next == NULL ) {
        active_tail = thr;
      } else {
        thr->queue_next->queue_prev = thr;
      }
      thr->queue_prev = thr->parent;
      thr->suppl.part.state = THR_ST_ACTIVE;    /* We will place the thread immediately into the active queue */

    } else {

      thr->curr_time = *time;

      /*
       If this statement is an always_comb or always_latch, add it to the delay list and change its right
       expression so that it will be executed at time 0 after all initial and always blocks have completed
      */
      if( (thr->curr->exp->op == EXP_OP_ALWAYS_COMB) || (thr->curr->exp->op == EXP_OP_ALWAYS_LATCH) ) {

        sim_time tmp_time;

        /* Add this thread into the delay queue at time 0 */
        tmp_time.lo    = 0;
        tmp_time.hi    = 0;
        tmp_time.full  = 0LL;
        tmp_time.final = FALSE;
        sim_thread_insert_into_delay_queue( thr, &tmp_time );

        /* Specify that this block should be evaluated */
        thr->curr->exp->right->suppl.part.eval_t = 1;

      } else {

        /* If the statement block is specified as a final block, add it to the end of the delay queue */
        if( thr->curr->suppl.part.final == 1 ) {

          sim_time tmp_time;

          tmp_time.lo    = 0xffffffff;
          tmp_time.hi    = 0xffffffff;
          tmp_time.full  = UINT64(0xffffffffffffffff);
          tmp_time.final = TRUE;
          sim_thread_insert_into_delay_queue( thr, &tmp_time );

        /* Otherwise, add it to the active thread list */
        } else {

          if( active_head == NULL ) {
            active_head = active_tail = thr;
          } else {
            thr->queue_prev         = active_tail;
            active_tail->queue_next = thr;
            active_tail             = thr;
          }
          thr->suppl.part.state = THR_ST_ACTIVE;

        }

      }
 
    }

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      printf( "Adding thread: " );
      sim_display_thread( thr, FALSE, TRUE );
      printf( "After thread is added to active queue...\n" );
      sim_display_active_queue();
      sim_display_all_list();
    }
#endif

  }

  PROFILE_END;

  return( thr );

}

/*!
 Removes the specified thread from its parent and the thread simulation queue and finally deallocates
 the specified thread.
*/
static void sim_kill_thread(
  thread* thr  /*!< Thread to remove from simulation */
) { PROFILE(SIM_KILL_THREAD);

  assert( thr != NULL );

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Thread queue before thread is killed...\n" );
    sim_display_active_queue();
  }
#endif

  if( thr->parent != NULL ) {

    /* Decrement the active children by one */
    thr->parent->active_children--;

    /* If we are the last child, re-insert the parent in our place (setting active_head to the parent) */
    if( thr->parent->active_children == 0 ) {
      thr->parent->queue_next = thr->queue_next;
      if( thr->queue_next == NULL ) {
        active_tail = thr->parent;
      } else {
        thr->queue_next->queue_prev = thr->parent;
      }
      active_head = thr->parent;
      thr->parent->curr_time = thr->curr_time;
      thr->parent->suppl.part.state = THR_ST_ACTIVE;  /* Specify that the parent thread is now back in the active queue */
    } else {
      active_head = active_head->queue_next;
      if( active_head == NULL ) {
        active_tail = NULL;
      }
    }

  } else {

    active_head = active_head->queue_next;
    if( active_head == NULL ) {
      active_tail = NULL;
    } else {
      active_head->queue_prev = NULL;  /* Here for debug purposes - TBD */
    }

  }

  /* Check to make sure that the thread is not in the waiting state */
  assert( thr->suppl.part.state != THR_ST_WAITING );

  /* Remove this thread from its functional unit */
  funit_delete_thread( thr->funit, thr );

  /* Finally, park this thread at the end of the all_queue (if its not already there) */
  if( thr != all_tail ) {
    if( thr == all_head ) {
      all_head           = thr->all_next;
      all_head->all_prev = NULL;
    } else {
      thr->all_prev->all_next = thr->all_next;
      thr->all_next->all_prev = thr->all_prev;
    }
    thr->all_prev      = all_tail;
    thr->all_next      = NULL;
    all_tail->all_next = thr;
    all_tail           = thr;
  }

  /* If the all_next pointer is NULL, point it to the moved thread */
  if( all_next == NULL ) {
    all_next = all_tail;
  }

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Thread queue after thread is killed...\n" );
    sim_display_active_queue();
    sim_display_all_list();
  }
#endif

  PROFILE_END;

}

/*!
 Searches the current state of the active queue for the thread containing the specified head statement.
 If a thread was found to match, kill it.  This function is called whenever the DISABLE statement is
 run.
*/
void sim_kill_thread_with_funit(
  func_unit* funit  /*!< Pointer to functional unit of thread to kill */
) { PROFILE(SIM_KILL_THREAD_WITH_FUNIT);

  thread* thr;  /* Pointer to current thread */

  assert( funit != NULL );

  /* Kill any threads that match the given functional unit or are children of it */
  thr = all_head;
  while( thr != NULL ) {
    if( (thr->funit == funit) || (funit_is_child_of( funit, thr->funit )) ) {
      thr->suppl.part.kill = 1;
    }
    thr = thr->all_next;
  }

  PROFILE_END;

}

/*!
 Iterates through static expression list and causes the simulator to
 evaluate these expressions at simulation time.
*/
static void sim_add_statics() { PROFILE(SIM_ADD_STATICS);
  
  unsigned int i;
  sim_time     time;   /* Current simulation time */

  /* Initialize the time to 0 */
  time.lo    = 0;
  time.hi    = 0;
  time.full  = 0;
  time.final = FALSE;
  
  for( i=0; i<static_expr_size; i++ ) {
    sim_expr_changed( static_exprs[i], &time );
  }
  
  exp_link_delete_list( static_exprs, static_expr_size, FALSE );
  static_exprs     = NULL;
  static_expr_size = 0;

  PROFILE_END;
  
}

/*!
 \return Returns TRUE if this expression has changed value from previous sim; otherwise,
         returns FALSE.

 Recursively traverses specified expression tree, following the #ESUPPL_IS_LEFT_CHANGED 
 and #ESUPPL_IS_RIGHT_CHANGED bits in the supplemental field.  Once an expression is
 found that has neither bit set, perform the expression operation and move back up
 the tree.  Once both left and right children have calculated values, perform the
 expression operation for the current expression, clear both changed bits and
 return.
*/
bool sim_expression(
  expression*     expr,  /*!< Pointer to expression to simulate */
  thread*         thr,   /*!< Pointer to current thread that is being simulated */
  const sim_time* time,  /*!< Pointer to current simulation time */
  bool            lhs    /*!< Specifies if we should only traverse LHS expressions or RHS expressions */
) { PROFILE(SIM_EXPRESSION);

  bool retval        = FALSE;  /* Return value for this function */
  bool left_changed  = FALSE;  /* Signifies if left expression tree has changed value */
  bool right_changed = FALSE;  /* Signifies if right expression tree has changed value */

  assert( expr != NULL );

  /* If our LHS mode matches the needed LHS mode, continue */
  if( ESUPPL_IS_LHS( expr->suppl ) == lhs ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "    In sim_expression %d, left_changed %d, right_changed %d, thread %p",
                                  expr->id, ESUPPL_IS_LEFT_CHANGED( expr->suppl ), ESUPPL_IS_RIGHT_CHANGED( expr->suppl ), thr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Traverse left child expression if it has changed */
    if( ((ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 1) ||
         (expr->op == EXP_OP_CASE)                    ||
         (expr->op == EXP_OP_CASEX)                   ||
         (expr->op == EXP_OP_CASEZ)) &&
        ((expr->op != EXP_OP_DLY_OP) || (expr->left == NULL) || (expr->left->op != EXP_OP_DELAY)) ) {

      /* Simulate the left expression if it has changed */
      if( expr->left != NULL ) {
        expr->suppl.part.left_changed = expr->suppl.part.clear_changed;
        left_changed = sim_expression( expr->left, thr, time, lhs );
      } else {
        expr->suppl.part.left_changed = 0;
        left_changed                  = TRUE;
      }

    }

    /* Traverse right child expression if it has changed */
    if( (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 1) &&
        ((expr->op != EXP_OP_DLY_OP) || !thr->suppl.part.exec_first) ) {
  
      /* Simulate the right expression if it has changed */
      if( expr->right != NULL ) {
        expr->suppl.part.right_changed = expr->suppl.part.clear_changed;
        right_changed = sim_expression( expr->right, thr, time, lhs );
      } else {
        expr->suppl.part.right_changed = 0;
        right_changed                  = TRUE;
      }

    }

    /*
     Now perform expression operation for this expression if left or right
     expressions trees have changed.
    */
    if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) || (expr->parent->stmt == NULL) || (expr->parent->stmt->suppl.part.cont == 0) || left_changed || right_changed || (expr->table != NULL) ) {
      retval = expression_operate( expr, thr, time );
    }

  }

  PROFILE_END;

  return( retval );

}

/*!
 Performs statement simulation as described above.  Calls expression simulator if
 the associated root expression is specified that signals have changed value within
 it.  Continues to run for current statement tree until statement tree hits a
 wait-for-event condition (or we reach the end of a simulation tree).
*/
void sim_thread(
  thread*         thr,  /*!< Pointer to current thread to simulate */
  const sim_time* time  /*!< Current simulation time to simulate */
) { PROFILE(SIM_THREAD);

  statement* stmt;                  /* Pointer to current statement to evaluate */
  bool       expr_changed = FALSE;  /* Specifies if expression tree was modified in any way */

  /* If the thread has a reentrant structure assigned to it, pop it */
  if( thr->ren != NULL ) {
    reentrant_dealloc( thr->ren, thr->funit, FALSE );
    thr->ren = NULL;
  }

  /* Set the value of stmt with the head_stmt */
  stmt = thr->curr;


  while( (stmt != NULL) && !thr->suppl.part.kill && simulate ) {

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
    cli_execute( time, force_stop, stmt );
    force_stop = FALSE;
#endif
#endif

    /* Place expression in expression simulator and run */
    expr_changed = sim_expression( stmt->exp, thr, time, FALSE );

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "  Executed statement %d, expr changed %d, thread %p", stmt->exp->id, expr_changed, thr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif
      
    thr->curr = stmt;

    /* Set exec_first to FALSE */
    thr->suppl.part.exec_first = 0;

    if( stmt->suppl.part.cont == 1 ) {
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
       (!EXPR_IS_CONTEXT_SWITCH( thr->curr->exp ) && !thr->curr->suppl.part.cont))) ||
      (thr->curr == NULL) ||
      (!expr_changed && (stmt == NULL) &&
       ((thr->curr->exp->op == EXP_OP_CASE)  ||
        (thr->curr->exp->op == EXP_OP_CASEX) ||
        (thr->curr->exp->op == EXP_OP_CASEZ) ||
        (thr->curr->exp->op == EXP_OP_DEFAULT))) ||
      thr->suppl.part.kill ||
      !simulate ) {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Completed thread %p, killing...\n", thr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif
 
    /* Destroy the thread */
    sim_kill_thread( thr );

  /* Otherwise, we are switching contexts */
  } else {

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Switching context of thread %p...\n", thr );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
    }
#endif

    /* Pop this packet out of the active queue */
    if( ((thr->curr->exp->op != EXP_OP_DELAY) && 
         ((thr->curr->exp->op != EXP_OP_DLY_ASSIGN) || (thr->curr->exp->right->left->op != EXP_OP_DELAY))) ||
        time->final ) {
      sim_thread_pop_head();
    } else {
      thr->suppl.part.exec_first = 1;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns TRUE if simulation should continue; otherwise, returns FALSE to indicate
         that simulation should no longer continue.

 This function is the heart of the simulation engine.  It is called by the
 db_do_timestep() function in db.c  and moves the statements and expressions into
 the appropriate simulation functions.  See above explanation on this procedure.
*/
bool sim_simulate(
  const sim_time* time  /*!< Current simulation time from dumpfile or simulator */
) { PROFILE(SIM_SIMULATE);

  /* Simulate all threads in the active queue */
  while( active_head != NULL ) {
    sim_thread( active_head, time );
  }

  while( (delayed_head != NULL) && TIME_CMP_LE(delayed_head->curr_time, *time) ) {

    active_head  = active_tail = delayed_head;
    delayed_head = delayed_head->queue_next;
    active_head->queue_prev = active_head->queue_next = NULL;
    if( delayed_head != NULL ) {
      delayed_head->queue_prev = NULL;
    } else {
      delayed_tail = NULL;
    }
    active_head->suppl.part.state = THR_ST_ACTIVE;

    while( active_head != NULL ) {
      sim_thread( active_head, time );
    }

  }

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "After delay simulation...\n" );
    sim_display_delay_queue();
  }
#endif

  PROFILE_END;

  return( simulate );

}

/*!
 Allocates thread arrays for simulation and initializes the contents of the active_threads array.
*/
void sim_initialize() { PROFILE(SIM_INITIALIZE);

  /* Create non-blocking assignment queue */
  if( nba_queue_size > 0 ) {
    nba_queue           = (nonblock_assign**)malloc_safe( sizeof( nonblock_assign ) * nba_queue_size );
    nba_queue_curr_size = 0;
  }

  /* Add static values */
  sim_add_statics();

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
  /* Set the CLI debug mode to the value of the general debug mode */
  cli_debug_mode = debug_mode;

  /* Add a signal handler for Ctrl-C if we are running in CLI mode */
  if( flag_use_command_line_debug ) {
    signal( SIGINT, cli_ctrl_c );
  }
#endif
#endif

  PROFILE_END;

}

/*!
 Stops the simulation and gets the user to a CLI prompt, if possible.
*/
void sim_stop() { PROFILE(SIM_STOP);

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
  force_stop = TRUE;
#else
  simulate = FALSE;
#endif
#else
  simulate = FALSE;
#endif

  PROFILE_END;

}

/*!
 Causes the simulator to finish gracefully.
*/
void sim_finish() { PROFILE(SIM_FINISH);

  simulate = FALSE;

  PROFILE_END;

}

/*!
 Updates and adds the given non-blocking assignment structure to the simulation
 queue.
*/
void sim_add_nonblock_assign(
  nonblock_assign* nba,      /*!< Pointer to non-blocking assignment to updated and add */
  int              lhs_lsb,  /*!< LSB of left-hand-side vector to assign */
  int              lhs_msb,  /*!< MSB of left-hand-side vector to assign */
  int              rhs_lsb,  /*!< LSB of right-hand-side vector to assign from */
  int              rhs_msb   /*!< MSB of right-hand-side vector to assign from */
) { PROFILE(SIM_ADD_NONBLOCK_ASSIGN);

  /* Update the non-blocking assignment structure */
  nba->lhs_lsb = lhs_lsb;
  nba->lhs_msb = lhs_msb;
  nba->rhs_lsb = rhs_lsb;
  nba->rhs_msb = rhs_msb;

  /* Add it to the simulation queue (if it has not been already) */
  if( nba->suppl.added == 0 ) {
    nba_queue[nba_queue_curr_size++] = nba;
    nba->suppl.added = 1;
  }

  PROFILE_END;

}

/*!
 Performs non-blocking assignment for the nba elements in the current nba simulation queue.
*/
void sim_perform_nba(
  const sim_time* time  /*!< Current simulation time */
) { PROFILE(SIM_PERFORM_NBA);

  int              i;
  bool             changed;
  nonblock_assign* nba;

  for( i=0; i<nba_queue_curr_size; i++ ) {
    nba     = nba_queue[i];
    changed = vector_part_select_push( nba->lhs_sig->value, nba->lhs_lsb, nba->lhs_msb, nba->rhs_vec, nba->rhs_lsb, nba->rhs_msb, nba->suppl.is_signed );
    nba->lhs_sig->value->suppl.part.set = 1;
#ifdef DEBUG_MODE
#ifndef VPI_ONLY
    if( debug_mode && (!flag_use_command_line_debug || cli_debug_mode) ) {
      if( i == 0 ) {
        printf( "Non-blocking assignments:\n" );
      }
      printf( "    " );  vsignal_display( nba->lhs_sig );
    }
#endif
#endif
    if( changed ) {
      vsignal_propagate( nba->lhs_sig, time );
    }
    nba->suppl.added = 0;
  }

  /* Clear the nba queue */
  nba_queue_curr_size = 0;

  PROFILE_END;

}

/*!
 Deallocates all allocated memory for simulation code.
*/
void sim_dealloc() { PROFILE(SIM_DEALLOC);

  thread* tmp;  /* Temporary thread pointer */

  /* Deallocate each thread in the all_threads array */
  while( all_head != NULL ) {
    tmp = all_head;
    all_head = all_head->all_next;
    free_safe( tmp, sizeof( thread ) );
  }

  all_head     = all_tail     = all_next = NULL;
  active_head  = active_tail  = NULL;
  delayed_head = delayed_tail = NULL;

  /* Deallocate all static expressions, if there are any */
  exp_link_delete_list( static_exprs, static_expr_size, FALSE );

  /* Deallocate the non-blocking assignment queue */
  free_safe( nba_queue, (sizeof( nonblock_assign ) * nba_queue_size) );

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
  /* Clear CLI debug mode */
  cli_debug_mode = FALSE;
#endif
#endif

  PROFILE_END;

}

