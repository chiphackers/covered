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
 the root, setting the #ESUPPL_LSB_LEFT_CHANGED or #ESUPPL_LSB_RIGHT_CHANGED as it 
 makes its way to the root.  When at the root expression, the #ESUPPL_LSB_STMT_HEAD 
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
 When a statement is placed into the statement simulation engine, the #ESUPPL_LSB_STMT_HEAD 
 bit is cleared in the root expression.  Additionally, the root expression pointed to by 
 the statement is interrogated to see if the #ESUPPL_LSB_LEFT_CHANGED or #ESUPPL_LSB_RIGHT_CHANGED 
 bits are set.  If one or both of the bits are found to be set, the root expression 
 is placed into the expression simulation engine for further processing.  When the 
 statement's expression has completed its simulation, the value of the root expression 
 is used to determine if the next_true or next_false path will be taken.  If the value 
 of the root expression is true, the next_true statement is loaded into the statement 
 simulation engine.  If the value of the root expression is false and the next_false 
 pointer is NULL, this signifies that the current statement tree has completed for this 
 timestep.  At this point, the current statement will set the #ESUPPL_LSB_STMT_HEAD bit in 
 its root expression and is removed from the statement simulation engine.  The next statement 
 at the head of the pre-simulation statement queue is then loaded into the statement 
 simulation engine.  If next_false statement is not NULL, it is loaded into the statement 
 simulation engine and work is done on that statement.

 \par
 When a root expression is placed into the expression simulation engine, the tree is
 traversed, following the paths that have set #ESUPPL_LSB_LEFT_CHANGED or 
 #ESUPPL_LSB_RIGHT_CHANGED bits set.  Each expression tree is traversed depth first.  When an 
 expression is reached that does not have either of these bits set, we have reached the expression
 whose value has changed.  When this expression is found, it is evaluated and the
 resulting value is stored into its value vector.  Once this has occurred, the parent
 expression checks to see if the other child expression has changed value.  If so,
 that child expression's tree is traversed.  Once both child expressions contain the
 current value for the current timestep, the parent expression evaluates its
 expression with the values of its children and clears both the #ESUPPL_LSB_LEFT_CHANGED and
 #ESUPPL_LSB_RIGHT_CHANGED bits to indicate that both children were evaluated.  The resulting
 value is stored into the current expression's value vector and the parent expression
 of the current expression is worked on.  This evaluation process continues until the
 root expression of the tree has been evaluated.  At this point the expression tree
 is removed from the expression simulation engine and the associated statement worked
 on by the statement simulation engine as specified above.
*/

#include <assert.h>

#include "defines.h"
#include "sim.h"
#include "expr.h"
#include "vector.h"
#include "iter.h"
#include "link.h"
#include "vsignal.h"


extern nibble or_optab[OPTAB_SIZE];
extern char   user_msg[USER_MSG_LENGTH];

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
 Pointer to current thread being simulated
*/
thread* curr_thread = NULL;


/*!
 \param expr  Pointer to expression that contains a changed signal value.

 Traverses up expression tree pointed to by leaf node expr, setting the
 CHANGED bits as it reaches the root expression.  When the root expression is
 found, the statement pointed to by the root's parent pointer is added to the
 pre-simulation statement queue for simulation at the end of the timestep.  If,
 upon traversing the tree, an expression is found to already be have a CHANGED
 bit set, we know that the statement has already been added, so stop here and
 do not add the statement again.
*/
void sim_expr_changed( expression* expr ) {

  assert( expr != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "In sim_expr_changed, expr %d, op %s, line %d, left_changed: %d, right_changed: %d",
            expr->id, expression_string_op( expr->op ), expr->line,
            ESUPPL_IS_LEFT_CHANGED( expr->suppl ),
            ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) );
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
      sim_expr_changed( expr->parent->expr );

    }

    /* Set one of the changed bits to let the simulator know that it needs to evaluate the expression.  */
    if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 0) && (ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ) {
      expr->suppl.part.left_changed = 1;
    }

  }

}

/*!
 Displays the current state of the thread queue
*/
void sim_display_thread_queue() {

  thread* curr;  /* Pointer to current thread */
  
  printf( "Current thread queue...\n" );

  curr = thread_head;
  while( curr != NULL ) {
    if( curr == curr_thread ) {
      printf( "  -> stmt %d, %s, line %d  ", curr->curr->exp->id, expression_string_op( curr->curr->exp->op ), curr->curr->exp->line );
    } else {
      printf( "     stmt %d, %s, line %d  ", curr->curr->exp->id, expression_string_op( curr->curr->exp->op ), curr->curr->exp->line );
    }
    if( curr == thread_head ) {
      printf( "H" );
    }
    if( curr == thread_tail ) {
      printf( "T" );
    }
    printf( "\n" );
    curr = curr->next;
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

  sig_link* sigl;                 /* Pointer to current signal in signal list */
  thread*   thr         = NULL;   /* Pointer to new thread to create */
  bool      first_child = FALSE;  /* Specifies if this is the first child to be added to the parent */

  assert( stmt != NULL );

  /* Only add expression if it is the head statement of its statement block */
  if( ESUPPL_IS_STMT_HEAD( stmt->exp->suppl ) == 1 ) {

    /* Create and initialize thread */
    thr             = (thread*)malloc_safe( sizeof( thread ), __FILE__, __LINE__ );
    thr->parent     = parent;
    thr->head       = stmt;
    thr->curr       = stmt;
    thr->kill       = FALSE;
    thr->child_head = NULL;
    thr->child_tail = NULL;
    thr->prev_sib   = NULL;
    thr->next_sib   = NULL;
    thr->prev       = NULL;
    thr->next       = NULL;

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
    }

    /* Arm all events in current statement expression */
    expression_arm_events( stmt->exp );

    /* Set wait signals */
    sigl = stmt->wait_sig_head;
    while( sigl != NULL ) {
      vsignal_set_wait_bit( sigl->sig, 1 );
      sigl = sigl->next;
    }

    /* Add this thread to the simulation thread queue */
    if( parent != NULL ) {

      /* If this is the first child to be added to the parent, remove the parent from the thread queue */
      if( first_child ) {
        thr->prev = parent->prev;
        thr->next = parent->next;

      /* Otherwise, just insert this child between the parent's previous and the other child */
      } else {
        thr->prev = parent->next->prev;
        thr->next = parent->next; 
      }

      /* Set the parent to point its next pointer to us */
      parent->next = thr;

      /* Fix the previous thread to point to us */
      if( thr->prev == NULL ) {
        thread_head = thr;
      } else {
        thr->prev->next = thr;
      }

      /* Fix the next thread to point to us */
      if( thr->next == NULL ) {
        thread_tail = thr;
      } else {
        thr->next->prev = thr;
      }

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

#ifdef OBSOLETE
  /* Remove all children (if any) */
  while( thr->child_head != NULL ) {
    sim_kill_thread( thr->child_head );
  }
#endif

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

  /* If this thread is the current thread, retreat the current thread pointer */
  if( thr == curr_thread ) {
    curr_thread = thr->prev;
  }

  /* If we are the last child, re-insert the parent thread */
  if( last_child ) {
    if( thr->prev == NULL ) {
      thread_head = thr->parent;
    } else {
      thr->prev->next = thr->parent;
    }
    thr->parent->prev = thr->prev;
    thr->parent->next = thr;
    thr->prev         = thr->parent;
  }    

  /* Now remove the thread from the thread list */
  if( (thr == thread_head) && (thr == thread_tail) ) {
    thread_head = thread_tail = NULL;
  } else if( thr == thread_head ) {
    thread_head       = thr->next;
    thread_head->prev = NULL;
  } else if( thr == thread_tail ) {
    thread_tail       = thr->prev;
    thread_tail->next = NULL;
  } else {
    thr->prev->next = thr->next;
    thr->next->prev = thr->prev;
  }

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

  thread* curr  = NULL;   /* Pointer to current thread being examined */
  thread* parent;         /* Pointer to current parent thread being examined */
  thread* child;          /* Pointer to current child being examined */
  bool    found = FALSE;  /* Specifies if matching statement has been found yet */

  assert( stmt != NULL );

  /* First, find the thread (if any) that contains the specified statement */
  curr = thread_head;
  while( (curr != NULL) && !found ) {

    if( curr->head == stmt ) {

      curr->kill = TRUE;
      found      = TRUE;

    } else {

      /* Check parent(s) */
      parent = curr->parent;
      while( (parent != NULL) && (parent->head != stmt) ) {
        parent = parent->parent;
      }

      if( parent != NULL ) {

        /* Kill all children including ourselves */
        child = parent->child_head;
        while( child != NULL ) {
          child->kill = TRUE;
          child       = child->next_sib;
        }
        parent->kill = TRUE;
        found        = TRUE;

      }

    }

    if( !found ) {
      curr = curr->next;
    }

  }

}

/*!
 Kills all threads in the simulator.  This is used at the end of a simulation run.
*/
void sim_kill_all_threads() {

  while( thread_head != NULL ) {
    sim_kill_thread( thread_head );
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
    sim_expr_changed( curr->exp );
    curr = curr->next;
  }
  
  exp_link_delete_list( static_expr_head, FALSE );
  
}

/*!
 \param expr  Pointer to expression to simulate.
 \param thr   Pointer to current thread that is being simulated.

 \return Returns TRUE if this expression has changed value from previous sim; otherwise,
         returns FALSE.

 Recursively traverses specified expression tree, following the #ESUPPL_LSB_LEFT_CHANGED 
 and #ESUPPL_LSB_RIGHT_CHANGED bits in the supplemental field.  Once an expression is
 found that has neither bit set, perform the expression operation and move back up
 the tree.  Once both left and right children have calculated values, perform the
 expression operation for the current expression, clear both changed bits and
 return.
*/
bool sim_expression( expression* expr, thread* thr ) {

  bool retval        = FALSE;  /* Return value for this function                       */
  bool left_changed  = FALSE;  /* Signifies if left expression tree has changed value  */
  bool right_changed = FALSE;  /* Signifies if right expression tree has changed value */

  assert( expr != NULL );

#ifdef DEBUG_MODE
  snprintf( user_msg, USER_MSG_LENGTH, "    In sim_expression %d, left_changed %d, right_changed %d",
            expr->id, ESUPPL_IS_LEFT_CHANGED( expr->suppl ), ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) );
  print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

  /* Traverse left child expression if it has changed */
  if( (ESUPPL_IS_LEFT_CHANGED( expr->suppl ) == 1) ||
      (expr->op == EXP_OP_CASE)    ||
      (expr->op == EXP_OP_CASEX)                   ||
      (expr->op == EXP_OP_CASEZ) ) {

    /* EOR operations will be traversed by the expression operator */
    if( (expr->op != EXP_OP_EOR) && (expr->left != NULL) ) {
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
  if( ESUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 1 ) {

    /* EOR operations will be traversed by the expression operator */
    if( (expr->op != EXP_OP_EOR) && (expr->right != NULL) ) {
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
 \param head_stmt  Pointer to head statement to simulate.
 \param last_stmt  Pointer to last statement simulated during this run (if non-NULL, this
                   statement should be first statement to execute the next time this is run).

 \return Returns TRUE if this statement block has executed at least one statement; otherwise,
         returns FALSE to indicate that this statement block had no simulation effect.

 Performs statement simulation as described above.  Calls expression simulator if
 the associated root expression is specified that signals have changed value within
 it.  Continues to run for current statement tree until statement tree hits a
 wait-for-event condition (or we reach the end of a simulation tree).
*/
bool sim_thread( thread* thr ) {

  statement* stmt;                  /* Pointer to current statement to evaluate */
  sig_link*  sigl;                  /* Pointer to current signal in signal list */
  bool       first        = TRUE;   /* Specifies that this is the first time through the loop */
  bool       expr_changed = FALSE;  /* Specifies if expression tree was modified in any way */

  /* Set the value of stmt with the head_stmt */
  stmt = thr->curr;

  while( (stmt != NULL) && !thr->kill ) {

    /* Place expression in expression simulator and run */
    expr_changed = sim_expression( stmt->exp, thr );

    /* Indicate that this statement's expression has been executed */
    stmt->exp->exec_num++;

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "  Executed statement %d, expr changed %d", stmt->exp->id, expr_changed );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
      
    /* Clear wait event signal bits */
    if( first && expr_changed ) {
      sigl = stmt->wait_sig_head;
      while( sigl != NULL ) {
        vsignal_set_wait_bit( sigl->sig, 0 );
        sigl = sigl->next;
      }
      first = FALSE;
    }

    thr->curr = stmt;

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
      thr->kill ) {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Completed thread %x, executed %d, killing...\n", thr, !first );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif
 
    sim_kill_thread( thr );

  /* Otherwise, set wait bits on current statement */
  } else {

#ifdef DEBUG_MODE
    snprintf( user_msg, USER_MSG_LENGTH, "Switching context of thread %x, executed %d...\n", thr, !first );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
#endif

    sigl = curr_thread->curr->wait_sig_head;
    while( sigl != NULL ) {
      vsignal_set_wait_bit( sigl->sig, 1 );
      sigl = sigl->next;
    }

  }

  return( !first );

}

/*!
 This function is the heart of the simulation engine.  It is called by the
 db_do_timestep() function in db.c  and moves the statements and expressions into
 the appropriate simulation functions.  See above explanation on this procedure.
*/
void sim_simulate() {

  bool thread_executed = TRUE;  /* Specifies if the any statement had a simulation effect */
  bool curr_executed;           /* Specifies if the current statement had a simulation effect */
  
  while( thread_executed ) {

    thread_executed = FALSE;
    curr_thread     = thread_head;
  
#ifdef DEBUG_MODE
    print_output( "####  Iterating through thread queue  ####", DEBUG, __FILE__, __LINE__ );
#endif
    
    while( curr_thread != NULL ) {
    
      assert( curr_thread->curr != NULL );
      assert( curr_thread->curr->exp != NULL );
    
      thread_executed |= sim_thread( curr_thread );

      if( curr_thread != NULL ) {
        curr_thread = curr_thread->next;
      } else {
        curr_thread = thread_head;
      }

    }

  }
  
}


/*
 $Log$
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

