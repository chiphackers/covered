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

#ifdef DEBUG_MODE
#ifndef VPI_ONLY
#include "cli.h"
#include "codegen.h"
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
extern uint64                curr_sim_time;
extern /*@null@*/inst_link*  inst_head;
extern bool                  flag_use_command_line_debug;
extern bool                  cli_debug_mode;


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
 Array of all threads created by sim_create_thread().
*/
static thread** all_threads = NULL;

/*!
 Number of elements stored in all_threads array.
*/
static unsigned all_size    = 0;

/*!
 Pointer to head of active thread list.
*/
static thread* active_head  = NULL;

/*!
 Pointer to tail of active thread list.
*/
static thread* active_tail  = NULL;

/*!
 Array of all delayed threads.
*/
static thread** delayed_threads = NULL;

/*!
 Number of delayed threads in delayed_threads array.
*/
static unsigned delayed_size    = 0;


/*!
 Displays the current state of the active queue (for debug purposes only).
*/
void sim_display_active_queue() {

  thread* curr;  /* Pointer to current thread */

  curr = active_head;
  while( curr != NULL ) {
    if( curr->curr == NULL ) {
      printf( "    time %llu, stmt NONE, queued %d, delayed %d (%p, parent=%p, prev=%p, next=%p)  ",
              curr->curr_time,
              curr->suppl.part.queued,
              curr->suppl.part.delayed,
              curr, curr->parent, curr->active_prev, curr->active_next );
    } else {
      printf( "    time %llu, stmt %d, %s, line %d, queued %d, delayed %d (%p, parent=%p, prev=%p, next=%p)  ",
              curr->curr_time,
              curr->curr->exp->id,
              expression_string_op( curr->curr->exp->op ),
              curr->curr->exp->line,
              curr->suppl.part.queued,
              curr->suppl.part.delayed,
              curr, curr->parent, curr->active_prev, curr->active_next );
    }
    if( curr == active_head ) {
      printf( "H" );
    }
    if( curr == active_tail ) {
      printf( "T" );
    }
    printf( "\n" );
    curr = curr->active_next;
  }

}

/*!
 \param tlist  Pointer to thread list to display
 \param size   Number of elements in tlist to display

 Displays the given thread list (array form) to standard output.
*/
void sim_display_thread_list( thread** tlist, unsigned size ) {

  unsigned i;  /* Loop iterator */

  for( i=0; i<size; i++ ) {
    if( tlist[i]->curr == NULL ) {
      printf( "    time %llu, stmt NONE, queued %d, delayed %d (%p, parent=%p, prev=%p, next=%p)  ",
              tlist[i]->curr_time,
              tlist[i]->suppl.part.queued,
              tlist[i]->suppl.part.delayed,
              tlist[i], tlist[i]->parent, tlist[i]->active_prev, tlist[i]->active_next );
    } else {
      printf( "    time %llu, stmt %d, %s, line %d, queued %d, delayed %d (%p, parent=%p, prev=%p, next=%p)  ",
              tlist[i]->curr_time,
              tlist[i]->curr->exp->id,
              expression_string_op( tlist[i]->curr->exp->op ),
              tlist[i]->curr->exp->line,
              tlist[i]->suppl.part.queued,
              tlist[i]->suppl.part.delayed,
              tlist[i], tlist[i]->parent, tlist[i]->active_prev, tlist[i]->active_next );
    }
    if( i == 0 ) {
      printf( "H" );
    }
    if( (i+1) == size ) {
      printf( "T" );
    }
    printf( "\n" );
  }

}

/*!
 Displays the current state of the delay queue (for debug purposes only).
*/
void sim_display_delay_queue() {

  sim_display_thread_list( delayed_threads, delayed_size );

}

/*!
 Displays the current state of the all_threads list (for debug purposes only).
*/
void sim_display_all_list() {

  printf( "ALL THREADS:\n" );
  sim_display_thread_list( all_threads, all_size );  

}

/*!
 Outputs the scope, block name, filename and line number of the current thread in the active queue to standard output.
*/
void sim_display_current() {

  char scope[4096];  /* String containing scope of given functional unit */
  int  ignore = 0;   /* Specifies that we should not ignore a matching functional unit */

  assert( active_head != NULL );
  assert( active_head->funit != NULL );
  assert( active_head->curr != NULL );

  /* Get the scope of the functional unit represented by the current thread */
  scope[0] = '\0';
  instance_gen_scope( scope, inst_link_find_by_funit( active_head->funit, inst_head, &ignore ), TRUE );

  /* Output the given scope */
  printf( "SCOPE: %s, BLOCK: %s, FILE: %s, LINE: %d\n",
          scope, funit_flatten_name( active_head->funit ), active_head->funit->filename, active_head->curr->exp->line );

}

/*!
 Displays the current statement to standard output.
*/
void sim_display_current_stmt() {

  char** code;        /* Pointer to code string from code generator */
  int    code_depth;  /* Depth of code array */
  int    i;           /* Loop iterator */

  assert( active_head != NULL );
  assert( active_head->funit != NULL );
  assert( active_head->curr != NULL );

  /* Generate the logic */
  codegen_gen_expr( active_head->curr->exp, active_head->curr->exp->op, &code, &code_depth, active_head->funit );

  /* Output the full expression */
  for( i=0; i<code_depth; i++ ) {
    printf( "      %7d:    %s\n", active_head->curr->exp->line, code[i] );
    free_safe( code[i] );
  }

  if( code_depth > 0 ) {
    free_safe( code );
  }

}

/*!
 \param thr       Pointer to the thread to add to the delay queue.
 \param sim_time  Time to insert the given thread.

 This function is called by the expression_op_func__delay() function.
*/
void sim_thread_insert_into_delay_queue( thread* thr, uint64 sim_time ) {

  int      i;       /* Loop iterator */
  unsigned parent;  /* Index of parent node in heap structure */
  unsigned child;   /* Index of child node in heap structure */

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before delay thread is inserted for time %llu...\n", sim_time );
  }
#endif

  if( thr != NULL ) {

    assert( thr->suppl.part.delayed == 0 );

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      sim_display_delay_queue();
    }
#endif

    /* Specify that the thread is queued and delayed */
    thr->suppl.part.delayed = 1;
    thr->suppl.part.queued  = 1;

    /* Set the thread simulation time to the given sim_time */
    thr->curr_time = sim_time;

    /* Add the given thread to the delayed queue */
    delayed_threads[delayed_size] = thr;
    delayed_size++;

    /* Now perform a heap sort on the new value */
    for( i=((delayed_size / 2) - 1); i>=0; i-- ) {
      thr    = delayed_threads[i];
      parent = i;
      child  = (i * 2) + 1;
      while( child < delayed_size ) {
        if( ((child + 1) < delayed_size) && (delayed_threads[child+1]->curr_time < delayed_threads[child]->curr_time) ) {
          child++;
        }
        if( delayed_threads[child]->curr_time < thr->curr_time ) {
          delayed_threads[parent] = delayed_threads[child];
          parent = child;
          child  = (parent * 2) + 1;
        } else {
          break;
        }
      }
      delayed_threads[i] = thr;
    }

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      printf( "After delay thread is inserted...\n" );
      sim_display_delay_queue();
      sim_display_all_list();
    }
#endif

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

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before thread is pushed...\n" );
  }
#endif

  /* Only add the thread if it exists and it isn't already in a queue */
  if( (thr != NULL) && (thr->suppl.part.queued == 0) && (ESUPPL_STMT_IS_CALLED( thr->curr->exp->suppl ) == 0) ) {

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      sim_display_active_queue();
    }
#endif

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

    /* Set the active next and prev pointers to NULL */
    thr->active_prev = thr->active_next = NULL;

    /* Add the given thread to the end of the active_threads queue */
    if( active_head == NULL ) {
      active_head = active_tail = thr;
    } else {
      thr->active_prev         = active_tail;
      active_tail->active_next = thr;
      active_tail              = thr;
    }

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      printf( "After thread is pushed...\n" );
      sim_display_active_queue();
      sim_display_all_list();
    }
#endif

  }

}

/*!
 Pops the head thread from the active queue without deallocating the thread.
*/
void sim_thread_pop_head() {

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Before thread is popped from active queue...\n" );
    sim_display_active_queue();
  }
#endif

  /* Advance the curr pointer if we call sim_add_thread */
  if( (active_head->curr->exp->op == EXP_OP_FORK)      ||
      (active_head->curr->exp->op == EXP_OP_FUNC_CALL) ||
      (active_head->curr->exp->op == EXP_OP_TASK_CALL) ||
      (active_head->curr->exp->op == EXP_OP_NB_CALL) ) {
    active_head->curr = active_head->curr->next_true;
  }

  /* If the current thread is not in the delay queue, clear the queued indicator */
  if( active_head->suppl.part.delayed == 0 ) {
    active_head->suppl.part.queued = 0;
  }

  /* If the active_head thread running for an automatic function/task, push the data */
  if( (active_head->funit->type == FUNIT_ATASK) || (active_head->funit->type == FUNIT_AFUNCTION) ) {
    assert( active_head->ren == NULL );
    active_head->ren = reentrant_create( active_head->funit );
  }

  /* Move the head pointer */
  active_head = active_head->active_next;
  if( active_head == NULL ) {
    active_tail = NULL;
  } else {
    active_head->active_prev = NULL;   /* TBD - Placed here for help in debug */
  }

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "After thread is popped from active queue...\n" );
    sim_display_active_queue();
    sim_display_all_list();
  }
#endif

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
     this statement (if it is the head) back onto the active queue.
    */
    } else if( (expr->parent->expr != NULL) && (expr->parent->stmt->thr->curr == expr->parent->stmt) ) {

      sim_thread_push( expr->parent->stmt->thr, sim_time );

    }

    /* Set one of the changed bits to let the simulator know that it needs to evaluate the expression.  */
    if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 0) && (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ) {
      expr->suppl.part.left_changed = 1;
    }

  }

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
thread* sim_create_thread( thread* parent, statement* stmt, func_unit* funit ) {

  thread* thr;  /* Pointer to newly allocated thread */

  /* Allocate the new thread */
  thr            = (thread*)malloc_safe( sizeof( thread ), __FILE__, __LINE__ );
  thr->funit     = funit;
  thr->parent    = parent;
  thr->curr      = stmt;
  thr->ren       = NULL;
  thr->suppl.all = 0;
  thr->curr_time = 0;

  return( thr );

}

/*!
 \param thr     Pointer to thread to add to the active_threads queue
 \param parent  Pointer to parent thread of the new thread to create (set to NULL if there is no parent thread)
 \param stmt    Pointer to head statement to have new thread point to.
 \param funit   Pointer to functional unit that is creating this thread.

 Creates a new thread with the given information and adds the thread to the active queue to run.  Returns a pointer
 to the newly created thread for joining/running purposes.
*/
void sim_add_thread( thread* thr ) {

  assert( thr != NULL );

  /* Only add expression if it is the head statement of its statement block */
  if( ESUPPL_IS_STMT_HEAD( thr->curr->exp->suppl ) == 1 ) {

    /* Initialize thread runtime components */
    thr->suppl.all         = 0;
    thr->suppl.part.queued = 1;    /* We will place the thread immediately into the active queue */
    thr->active_children   = 0;
    thr->curr_time         = curr_sim_time;
    thr->active_prev       = NULL;
    thr->active_next       = NULL;

    /*
     If the parent thread is specified, update our current time, increment the number of active children in the parent
     and add ourselves between the parent thread and its next pointer.
    */
    if( thr->parent != NULL ) {

      thr->curr_time = thr->parent->curr_time;
      thr->parent->active_children++;

      /* Place ourselves between the parent and its active_next pointer */
      thr->active_next = thr->parent->active_next;
      thr->parent->active_next = thr;
      if( thr->active_next == NULL ) {
        active_tail = thr;
      } else {
        thr->active_next->active_prev = thr;
      }
      thr->active_prev = thr->parent;

    } else {

      /*
       If this statement is an always_comb or always_latch, add it to the delay list and change its right
       expression so that it will be executed at time 0 after all initial and always blocks have completed
      */
      if( (thr->curr->exp->op == EXP_OP_ALWAYS_COMB) || (thr->curr->exp->op == EXP_OP_ALWAYS_LATCH) ) {

        /* Add this thread into the delay queue at time 0 */
        sim_thread_insert_into_delay_queue( thr, 0 );

        /* Specify that this block should be evaluated */
        thr->curr->exp->right->suppl.part.eval_t = 1;

      } else {

        /* If the statement block is specified as a final block, add it to the end of the delay queue */
        if( ESUPPL_STMT_FINAL( thr->curr->exp->suppl ) == 1 ) {
          sim_thread_insert_into_delay_queue( thr, (uint64)0xffffffffffffffff );

        /* Otherwise, add it to the active thread list */
        } else {

          if( active_head == NULL ) {
            active_head = active_tail = thr;
          } else {
            thr->active_prev         = active_tail;
            active_tail->active_next = thr;
            active_tail              = thr;
          }
        }

      }
 
    }

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      printf( "After thread is added to active queue...\n" );
      sim_display_active_queue();
      sim_display_all_list();
    }
#endif

  }

}

/*!
 \param thr  Thread to remove from simulation

 Removes the specified thread from its parent and the thread simulation queue and finally deallocates
 the specified thread.
*/
void sim_kill_thread( thread* thr ) {

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
      thr->parent->active_next = thr->active_next;
      if( thr->active_next == NULL ) {
        active_tail = thr->parent;
      } else {
        thr->active_next->active_prev = thr->parent;
      }
      active_head = thr->parent;
      thr->parent->curr_time = thr->curr_time;
      thr->parent->suppl.part.queued = 1;  /* Specify that the parent thread is now back in the active queue */
    } else {
      active_head = active_head->active_next;
      if( active_head == NULL ) {
        active_tail = NULL;
      }
    }

  } else {

    active_head = active_head->active_next;
    if( active_head == NULL ) {
      active_tail = NULL;
    }

  }

#ifdef DEBUG_MODE
  if( debug_mode && !flag_use_command_line_debug ) {
    printf( "Thread queue after thread is killed...\n" );
    sim_display_active_queue();
    sim_display_all_list();
  }
#endif

}

/*!
 \param funit  Pointer to functional unit of thread to kill

 Searches the current state of the active queue for the thread containing the specified head statement.
 If a thread was found to match, kill it.  This function is called whenever the DISABLE statement is
 run.
*/
void sim_kill_thread_with_funit( func_unit* funit ) {

  func_unit*  mod;    /* Pointer to current module */
  funit_link* child;  /* Pointer to current child being examined */

  assert( funit != NULL );

  /* Specify that this thread should be killed */
  funit->first_stmt->thr->suppl.part.kill = 1;

  /* Get parent module */
  mod = funit_get_curr_module( funit );

  /* If this thread is a parent to other threads, kill all children */
  child = mod->tf_head;
  while( child != NULL ) {
    if( child->funit->parent == funit ) {
      child->funit->first_stmt->thr->suppl.part.kill = 1;
    }
    child = child->next;
  }

}

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
  if( ((ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 1) ||
       (expr->op == EXP_OP_CASE)                    ||
       (expr->op == EXP_OP_CASEX)                   ||
       (expr->op == EXP_OP_CASEZ)) &&
      ((expr->op != EXP_OP_DLY_OP) || (expr->left == NULL) || (expr->left->op != EXP_OP_DELAY)) ) {

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
 \param thr       Pointer to current thread to simulate.
 \param sim_time  Current simulation time to simulate.

 Performs statement simulation as described above.  Calls expression simulator if
 the associated root expression is specified that signals have changed value within
 it.  Continues to run for current statement tree until statement tree hits a
 wait-for-event condition (or we reach the end of a simulation tree).
*/
void sim_thread( thread* thr, uint64 sim_time ) {

  statement* stmt;                  /* Pointer to current statement to evaluate */
  bool       expr_changed = FALSE;  /* Specifies if expression tree was modified in any way */

  /* If the thread has a reentrant structure assigned to it, pop it */
  if( thr->ren != NULL ) {
    reentrant_dealloc( thr->ren, thr->funit, sim_time );
    thr->ren = NULL;
  }

  /* Set the value of stmt with the head_stmt */
  stmt = thr->curr;

  /* Clear the delayed supplemental field of the current thread */
  thr->suppl.part.delayed = 0;

  while( (stmt != NULL) && !thr->suppl.part.kill ) {

#ifdef OBSOLETE
    /* Remove the pointer to the current thread from the last statement */
    thr->curr->thr = NULL;

    /* Set current statement thread pointer to current thread */
    stmt->thr = thr;
#endif

#ifdef DEBUG_MODE
    cli_execute();
#endif

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
      (thr->curr == NULL) ||
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

    /* Pop this packet out of the active queue */
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

  unsigned i;          /* Loop iterator */
  unsigned parent;     /* Index of parent node for heapsort */
  unsigned child;      /* Index of child node for heapsort */
  uint64   last_time;  /* Last time placed into the active thread queue */      
  thread*  tmp;        /* Pointer to temporary thread */

  /* Simulate all threads in the active queue */
  while( active_head != NULL ) {
    sim_thread( active_head, sim_time );
  }

  if( delayed_size > 0 ) {

    do {

      /* Now extract items from the delayed queue and put them in the active thread list */
      if( delayed_threads[0]->curr_time <= sim_time ) {

        last_time = delayed_threads[0]->curr_time;
        if( active_head == NULL ) {
          active_head = active_tail = delayed_threads[0]; 
        } else {
          delayed_threads[0]->active_prev = active_tail;
          active_tail->active_next        = delayed_threads[0];
          active_tail                     = delayed_threads[0];
        }
        delayed_size--;

        /* Perform the heap sort sift operation */
        if( delayed_size > 0 ) {
          do {
            tmp    = delayed_threads[delayed_size-1];
            parent = 0;
            child  = 1;
            while( child < (delayed_size-1) ) {
              if( ((child + 1) < (delayed_size-1)) && (delayed_threads[child + 1]->curr_time < delayed_threads[child]->curr_time) ) {
                child++;
              }
              if( delayed_threads[child]->curr_time < tmp->curr_time ) {
                delayed_threads[parent] = delayed_threads[child];
                parent = child;
                child  = (parent * 2) + 1;
              } else {
                break;
              }
            }
            delayed_threads[parent] = tmp;
            if( delayed_threads[0]->curr_time == last_time ) {
              if( active_head == NULL ) {
                active_head = active_tail = delayed_threads[0];
              } else {
                delayed_threads[0]->active_prev = active_tail;
                active_tail->active_next        = delayed_threads[0];
                active_tail                     = delayed_threads[0];
              }
              delayed_size--;
            }
          } while( (delayed_size > 0) && (delayed_threads[0]->curr_time == last_time) );
        }

        /* Simulate all threads in the active queue */
        while( active_head != NULL ) {
          sim_thread( active_head, sim_time );
        }

      }

    } while( (delayed_size > 0) && (delayed_threads[0]->curr_time <= sim_time) );

#ifdef DEBUG_MODE
    if( debug_mode && !flag_use_command_line_debug ) {
      printf( "After delay simulation...\n" );
      sim_display_delay_queue();
    }
#endif

  }

}

/*!
 Allocates thread arrays for simulation and initializes the contents of the active_threads array.
*/
void sim_initialize() {

  thread*  tmp_head = NULL;  /* Pointer to head of temporary thread list */
  thread*  tmp_tail = NULL;  /* Pointer to tail of temporary thread list */
  unsigned size;             /* Stores the size of all the threads */

  /* Iterate through the instances, counting, creating and adding new threads to a temporary list */
  size = inst_link_create_threads( inst_head, &tmp_head, &tmp_tail );

  /* Allocate memory for the all_threads list */
  all_threads = (thread**)malloc_safe( (sizeof( thread* ) * size), __FILE__, __LINE__ );

  /* Copy active threads to the active thread array */
  all_size = 0;
  while( tmp_head != NULL ) {
    assert( all_size < size );
    // printf( "Adding thread %p to all_threads array (pre_all_size=%d)\n", tmp_head, all_size );
    all_threads[all_size] = tmp_head;
    all_size++;
    tmp_tail = tmp_head->active_next;
    if( ESUPPL_STMT_IS_CALLED( tmp_head->curr->exp->suppl ) == 0 ) {
      sim_add_thread( tmp_head );
    }
    tmp_head = tmp_tail;
  }

  /* Allocate delayed thread array */
  delayed_threads = (thread**)malloc_safe( (sizeof( thread* ) * size), __FILE__, __LINE__ );
  delayed_size    = 0;

  /* Add static values */
  sim_add_statics();

  /* Set the CLI debug mode to the value of the general debug mode */
  cli_debug_mode = debug_mode;

}

/*!
 Deallocates all allocated memory for simulation code.
*/
void sim_dealloc() {

  unsigned i;

  /* Deallocate each thread in the all_threads array */
  for( i=0; i<all_size; i++ ) {
    free_safe( all_threads[i] );
  }

  /* Deallocate thread arrays */
  free_safe( all_threads );
  free_safe( delayed_threads );

  /* Clear CLI debug mode */
  cli_debug_mode = FALSE;

}


/*
 $Log$
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

