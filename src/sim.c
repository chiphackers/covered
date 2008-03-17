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

#include <stdio.h>
#include <assert.h>

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
#include "cli.h"
#endif
#endif
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "instance.h"
#include "iter.h"
#include "link.h"
#include "reentrant.h"
#include "sim.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"


extern nibble                or_optab[OPTAB_SIZE];
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
 \param thr         Pointer to thread to display to standard output
 \param show_queue  If set to TRUE, displays queue_prev/queue_next; otherwise, displays all_prev/all_next
 \param endl        If set to TRUE, prints a newline character

 Displays the contents of the given thread to standard output.
*/
void sim_display_thread(
  const thread* thr,
  bool          show_queue,
  bool          endl
) {

  if( !endl ) {
    printf( "    " );
  }

  /*@-duplicatequals -formattype@*/
  printf( "time %llu, ", thr->curr_time.full );
  /*@=duplicatequals =formattype@*/

  if( thr->curr == NULL ) {
    printf( "stmt NONE, " );
  } else {
    printf( "stmt %d, ", thr->curr->exp->id );
    printf( "%s, ", expression_string_op( thr->curr->exp->op ) );
    printf( "line %d, ", thr->curr->exp->line );
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
 \param queue_head  Pointer to head of queue to display
 \param queue_tail  Pointer to tail of queue to display

 Displays the current state of the active queue (for debug purposes only).
*/
static void sim_display_queue(
  thread* queue_head,
  thread* queue_tail
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
 \param thr   Pointer to the thread to add to the delay queue.
 \param time  Pointer to time to insert the given thread.

 This function is called by the expression_op_func__delay() function.
*/
void sim_thread_insert_into_delay_queue(
  thread*         thr,
  const sim_time* time
) { PROFILE(SIM_THREAD_INSERT_INTO_DELAY_QUEUE);

  thread* curr;  /* Pointer to current thread in delayed queue to compare against */

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before delay thread is inserted for time %llu...\n", time->full );
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
 \param thr   Pointer to thread to add to the tail of the simulation queue.
 \param time  Current simulation time of thread to push

 Adds the specified thread to the end of the current simulation queue.  This function gets
 called whenever a head statement has a signal change or the head statement is a delay operation
 and
*/
void sim_thread_push(
  thread*         thr,
  const sim_time* time
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
 \param expr  Pointer to expression that contains a changed signal value.
 \param time  Specifies current simulation time for the thread to push.

 Traverses up expression tree pointed to by leaf node expr, setting the
 CHANGED bits as it reaches the root expression.  When the root expression is
 found, the statement pointed to by the root's parent pointer is added to the
 pre-simulation statement queue for simulation at the end of the timestep.  If,
 upon traversing the tree, an expression is found to already be have a CHANGED
 bit set, we know that the statement has already been added, so stop here and
 do not add the statement again.
*/
void sim_expr_changed( expression* expr, const sim_time* time ) { PROFILE(SIM_EXPR_CHANGED);

  assert( expr != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In sim_expr_changed, expr %d, op %s, line %d, left_changed: %d, right_changed: %d, time: %llu",
            expr->id, expression_string_op( expr->op ), expr->line,
            ESUPPL_IS_LEFT_CHANGED( expr->suppl ),
            ESUPPL_IS_RIGHT_CHANGED( expr->suppl ),
            time->full );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* No need to continue to traverse up tree if both CHANGED bits are set */
  if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 0) ||
      (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ||
      (expr->op == EXP_OP_COND) ) {

    /* If we are not the root expression, do the following */
    if( ESUPPL_IS_ROOT( expr->suppl ) == 0 ) { PROFILE(SIM_EXPR_CHANGED_A);

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
      sim_expr_changed( expr->parent->expr, time );

      PROFILE_END;

    /*
     Otherwise, if we have hit the root expression and the parent pointer is valid, add 
     this statement (if it is the head) back onto the active queue.
    */
    } else if( expr->parent->expr != NULL ) {

      funit_push_threads( expr->parent->stmt->funit, expr->parent->stmt, time );

    }

    /* Set one of the changed bits to let the simulator know that it needs to evaluate the expression.  */
    if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 0) && (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ) {
      expr->suppl.part.left_changed = 1;
    }

  }

  PROFILE_END;

}

/*!
 \param parent  Pointer to parent thread (if one exists) of the newly created thread
 \param stmt    Pointer to the statement that is the head statement of the thread's block
 \param funit   Pointer to functional unit containing the new thread

 \return Returns a pointer to the newly allocated and initialized thread

 Allocates a new thread for simulation purposes and initializes the thread structure with
 everything that can be done at time 0.  This function does not place the thread into any
 queues (this is left to the sim_add_thread function).
*/
static thread* sim_create_thread( thread* parent, statement* stmt, func_unit* funit ) { PROFILE(SIM_CREATE_THREAD);

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
 \param parent  Pointer to parent thread of the new thread to create (set to NULL if there is no parent thread)
 \param stmt    Pointer to head statement to have new thread point to.
 \param funit   Pointer to functional unit that is creating this thread.
 \param time    Pointer to current simulation time.

 \return Returns the pointer to the thread that was added to the active queue (if one was added).

 Creates a new thread with the given information and adds the thread to the active queue to run.  Returns a pointer
 to the newly created thread for joining/running purposes.
*/
thread* sim_add_thread( thread* parent, statement* stmt, func_unit* funit, const sim_time* time ) { PROFILE(SIM_ADD_THREAD);

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
 \param thr  Thread to remove from simulation

 Removes the specified thread from its parent and the thread simulation queue and finally deallocates
 the specified thread.
*/
static void sim_kill_thread( thread* thr ) { PROFILE(SIM_KILL_THREAD);

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
 \param funit  Pointer to functional unit of thread to kill

 Searches the current state of the active queue for the thread containing the specified head statement.
 If a thread was found to match, kill it.  This function is called whenever the DISABLE statement is
 run.
*/
void sim_kill_thread_with_funit( func_unit* funit ) { PROFILE(SIM_KILL_THREAD_WITH_FUNIT);

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
  
  exp_link* curr;   /* Pointer to current expression link */
  sim_time  time;   /* Current simulation time */

  /* Initialize the time to 0 */
  time.lo    = 0;
  time.hi    = 0;
  time.full  = 0;
  time.final = FALSE;
  
  curr = static_expr_head;
  while( curr != NULL ) {
    sim_expr_changed( curr->exp, &time );
    curr = curr->next;
  }
  
  exp_link_delete_list( static_expr_head, FALSE );

  PROFILE_END;
  
}

/*!
 \param expr  Pointer to expression to simulate.
 \param thr   Pointer to current thread that is being simulated.
 \param time  Pointer to current simulation time.

 \return Returns TRUE if this expression has changed value from previous sim; otherwise,
         returns FALSE.

 Recursively traverses specified expression tree, following the #ESUPPL_IS_LEFT_CHANGED 
 and #ESUPPL_IS_RIGHT_CHANGED bits in the supplemental field.  Once an expression is
 found that has neither bit set, perform the expression operation and move back up
 the tree.  Once both left and right children have calculated values, perform the
 expression operation for the current expression, clear both changed bits and
 return.
*/
static bool sim_expression( expression* expr, thread* thr, const sim_time* time ) { PROFILE(SIM_EXPRESSION);

  bool retval        = FALSE;  /* Return value for this function */
  bool left_changed  = FALSE;  /* Signifies if left expression tree has changed value */
  bool right_changed = FALSE;  /* Signifies if right expression tree has changed value */

  assert( expr != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "    In sim_expression %d, left_changed %d, right_changed %d, thread %p",
            expr->id, ESUPPL_IS_LEFT_CHANGED( expr->suppl ), ESUPPL_IS_RIGHT_CHANGED( expr->suppl ), thr );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Traverse left child expression if it has changed */
  if( ((ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 1) ||
       (expr->op == EXP_OP_CASE)                    ||
       (expr->op == EXP_OP_CASEX)                   ||
       (expr->op == EXP_OP_CASEZ)) &&
      ((expr->op != EXP_OP_DLY_OP) || (expr->left == NULL) || (expr->left->op != EXP_OP_DELAY)) ) {

    /* Clear LEFT CHANGED bit */
    expr->suppl.part.left_changed = 0;

    /* Simulate the left expression if it has changed */
    if( expr->left != NULL ) {
      if( expr->left->suppl.part.lhs == 0 ) {
        left_changed = sim_expression( expr->left, thr, time );
      }
    } else {
      left_changed = TRUE;
    }

  }

  /* Traverse right child expression if it has changed */
  if( (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 1) &&
      ((expr->op != EXP_OP_DLY_OP) || !thr->suppl.part.exec_first) ) {

    /* Clear RIGHT CHANGED bit */
    expr->suppl.part.right_changed = 0;

    /* Simulate the right expression if it has changed */
    if( expr->right != NULL ) {
      if( expr->right->suppl.part.lhs == 0 ) {
        right_changed = sim_expression( expr->right, thr, time );
      }
    } else {
      right_changed = TRUE;
    } 

  }

  /*
   Now perform expression operation for this expression if left or right
   expressions trees have changed.
  */
  if( (ESUPPL_IS_ROOT( expr->suppl ) == 0) || (expr->parent->stmt == NULL) || (expr->parent->stmt->suppl.part.cont == 0) || left_changed || right_changed ) {
    retval = expression_operate( expr, thr, time );
  }

  PROFILE_END;

  return( retval );

}

/*!
 \param thr   Pointer to current thread to simulate.
 \param time  Current simulation time to simulate.

 Performs statement simulation as described above.  Calls expression simulator if
 the associated root expression is specified that signals have changed value within
 it.  Continues to run for current statement tree until statement tree hits a
 wait-for-event condition (or we reach the end of a simulation tree).
*/
void sim_thread(
  thread*         thr,
  const sim_time* time
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
    cli_execute( time, force_stop );
    force_stop = FALSE;
#endif
#endif

    /* Place expression in expression simulator and run */
    expr_changed = sim_expression( stmt->exp, thr, time );

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "  Executed statement %d, expr changed %d, thread %p", stmt->exp->id, expr_changed, thr );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
    snprintf( user_msg, USER_MSG_LENGTH, "Completed thread %p, killing...\n", thr );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
 
    /* Destroy the thread */
    sim_kill_thread( thr );

  /* Otherwise, we are switching contexts */
  } else {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Switching context of thread %p...\n", thr );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
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
 \param time  Current simulation time from dumpfile or simulator.

 \return Returns TRUE if simulation should continue; otherwise, returns FALSE to indicate
         that simulation should no longer continue.

 This function is the heart of the simulation engine.  It is called by the
 db_do_timestep() function in db.c  and moves the statements and expressions into
 the appropriate simulation functions.  See above explanation on this procedure.
*/
bool sim_simulate(
  const sim_time* time
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

  /* Add static values */
  sim_add_statics();

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
  /* Set the CLI debug mode to the value of the general debug mode */
  cli_debug_mode = debug_mode;
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

  all_head = all_tail = all_next = NULL;

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
  /* Clear CLI debug mode */
  cli_debug_mode = FALSE;
#endif
#endif

  PROFILE_END;

}


/*
 $Log$
 Revision 1.123  2008/02/29 00:08:31  phase1geo
 Completed optimization code in simulator.  Still need to verify that code
 changes enhanced performances as desired.  Checkpointing.

 Revision 1.122  2008/02/28 07:54:09  phase1geo
 Starting to add functionality for simulation optimization in the sim_expr_changed
 function (feature request 1897410).

 Revision 1.121  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.120  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.119  2008/02/23 00:26:02  phase1geo
 Fixing bug 1899768 and adding extra debug information.

 Revision 1.118  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.117  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.116  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.115  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.114  2007/12/20 04:47:50  phase1geo
 Fixing the last of the regression failures from previous changes.  Removing unnecessary
 output used for debugging.

 Revision 1.113  2007/12/19 22:54:35  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.112  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.111  2007/12/14 23:38:37  phase1geo
 More performance enhancements.  Checkpointing.

 Revision 1.110  2007/12/13 14:25:12  phase1geo
 Attempting to enhance performance of the sim_simulation function (I have a few
 different code segments here to do the task at the moment).

 Revision 1.109  2007/12/13 02:32:39  phase1geo
 Fixing segmentation fault.

 Revision 1.108  2007/12/12 23:36:57  phase1geo
 Optimized vector_op_add function significantly.  Other improvements made to
 profiler output.  Attempted to optimize the sim_simulation function although
 it hasn't had the intended effect and delay1.3 is currently failing.  Checkpointing
 for now.

 Revision 1.107  2007/12/12 14:17:44  phase1geo
 Enhancing the profiling report.

 Revision 1.106  2007/12/12 08:04:15  phase1geo
 Adding more timed functions for profiling purposes.

 Revision 1.105  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.104  2007/12/11 05:48:26  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.103  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.102  2007/09/04 22:50:50  phase1geo
 Fixed static_afunc1 issues.  Reran regressions and updated necessary files.
 Also working on debugging one remaining issue with mem1.v (not solved yet).

 Revision 1.101  2007/07/30 22:42:02  phase1geo
 Making some progress on automatic function support.  Things currently don't compile
 but I need to checkpoint for now.

 Revision 1.100  2007/07/27 21:57:08  phase1geo
 Adding afunc1 diagnostic to regression suite (though this diagnostic does not
 currently pass).  Checkpointing.

 Revision 1.99  2007/07/27 19:11:27  phase1geo
 Putting in rest of support for automatic functions/tasks.  Checked in
 atask1 diagnostic files.

 Revision 1.98  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.97  2007/07/24 22:52:26  phase1geo
 More clean-up for VCS regressions.  Still not fully passing yet.

 Revision 1.96  2007/07/23 12:32:58  phase1geo
 Updating ChangeLog and fixing some compile issues with the VPI library.

 Revision 1.95  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.94  2007/04/20 22:56:46  phase1geo
 More regression updates and simulator core fixes.  Still a ways to go.

 Revision 1.93  2007/04/18 22:35:02  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.92  2007/04/13 21:47:12  phase1geo
 More simulation debugging.  Added 'display all_list' command to CLI to output
 the list of all threads.  Updated regressions though we are not fully passing
 at the moment.  Checkpointing.

 Revision 1.91  2007/04/12 21:35:20  phase1geo
 More bug fixes to simulation core.  Checkpointing.

 Revision 1.90  2007/04/12 20:54:55  phase1geo
 Adding cli > output when replaying and adding back all of the functions (since
 the cli > prompt helps give it context.  Fixing bugs in simulation core.
 Checkpointing.

 Revision 1.89  2007/04/12 04:15:40  phase1geo
 Adding history all command, added list command and updated the display current
 command to include statement output.

 Revision 1.88  2007/04/11 22:29:48  phase1geo
 Adding support for CLI to score command.  Still some work to go to get history
 stuff right.  Otherwise, it seems to be working.

 Revision 1.87  2007/04/10 22:10:11  phase1geo
 Fixing some more simulation issues.

 Revision 1.86  2007/04/10 03:56:18  phase1geo
 Completing majority of code to support new simulation core.  Starting to debug
 this though we still have quite a ways to go here.  Checkpointing.

 Revision 1.85  2007/04/09 22:47:53  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.84  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.83  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.82  2006/12/12 06:20:23  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.81  2006/11/30 19:58:11  phase1geo
 Fixing rest of issues so that full regression (IV, Cver and VCS) without VPI
 passes.  Updated regression files.

 Revision 1.80  2006/11/30 05:04:23  phase1geo
 More fixes to new simulation algorithm.  Still have a couple of failures that
 need to be looked at.  Checkpointing.

 Revision 1.79  2006/11/29 23:15:46  phase1geo
 Major overhaul to simulation engine by including an appropriate delay queue
 mechanism to handle simulation timing for delay operations.  Regression not
 fully passing at this moment but enough is working to checkpoint this work.

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

