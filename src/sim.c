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
 the root, setting the #SUPPL_LSB_LEFT_CHANGED or #SUPPL_LSB_RIGHT_CHANGED as it 
 makes its way to the root.  When at the root expression, the #SUPPL_LSB_STMT_HEAD 
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
 When a statement is placed into the statement simulation engine, the #SUPPL_LSB_STMT_HEAD 
 bit is cleared in the root expression.  Additionally, the root expression pointed to by 
 the statement is interrogated to see if the #SUPPL_LSB_LEFT_CHANGED or #SUPPL_LSB_RIGHT_CHANGED 
 bits are set.  If one or both of the bits are found to be set, the root expression 
 is placed into the expression simulation engine for further processing.  When the 
 statement's expression has completed its simulation, the value of the root expression 
 is used to determine if the next_true or next_false path will be taken.  If the value 
 of the root expression is true, the next_true statement is loaded into the statement 
 simulation engine.  If the value of the root expression is false and the next_false 
 pointer is NULL, this signifies that the current statement tree has completed for this 
 timestep.  At this point, the current statement will set the #SUPPL_LSB_STMT_HEAD bit in 
 its root expression and is removed from the statement simulation engine.  The next statement 
 at the head of the pre-simulation statement queue is then loaded into the statement 
 simulation engine.  If next_false statement is not NULL, it is loaded into the statement 
 simulation  engine and work is done on that statement.

 \par
 When a root expression is placed into the expression simulation engine, the tree is
 traversed, following the paths that have set #SUPPL_LSB_LEFT_CHANGED or 
 #SUPPL_LSB_RIGHT_CHANGED bits set.  Each expression tree is traversed depth first.  When an 
 expression is reached that does not have either of these bits set, we have reached the expression
 whose value has changed.  When this expression is found, it is evaluated and the
 resulting value is stored into its value vector.  Once this has occurred, the parent
 expression checks to see if the other child expression has changed value.  If so,
 that child expression's tree is traversed.  Once both child expressions contain the
 current value for the current timestep, the parent expression evaluates its
 expression with the values of its children and clears both the #SUPPL_LSB_LEFT_CHANGED and
 #SUPPL_LSB_RIGHT_CHANGED bits to indicate that both children were evaluated.  The resulting
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

extern nibble or_optab[16];

stmt_link* presim_stmt_head;
stmt_link* presim_stmt_tail;


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
void sim_add_to_queue( expression* expr ) {

  expression* parent;    /* Pointer to parent expression of the current expression */

  /* If any of the changed bits are set, stop traversing now */
  if( (SUPPL_IS_LEFT_CHANGED( expr->suppl )  == 0) &&
      (SUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 0) ) {

    /* If we are not the root expression, do the following */
    if( SUPPL_IS_ROOT( expr->suppl ) == 0 ) {

      /* Set the appropriate CHANGED bit of the parent expression */
      if( (expr->parent->expr->left != NULL) && (expr->parent->expr->left->id == expr->id) ) {

        expr->parent->expr->suppl = expr->parent->expr->suppl | (0x1 << SUPPL_LSB_LEFT_CHANGED);

      } else if( (expr->parent->expr->right != NULL) && (expr->parent->expr->right->id == expr->id) ) {
        
        expr->parent->expr->suppl = expr->parent->expr->suppl | (0x1 << SUPPL_LSB_RIGHT_CHANGED);

      }

      /* Continue up the tree */
      sim_add_to_queue( expr->parent->expr );

    } else {

      /* We are the root expression so just add the statement to the queue */
      stmt_link_add( expr->parent->stmt, &(presim_stmt_head), &(presim_stmt_tail) );

    }

  }

}

/*!
 \param expr  Pointer to expression to simulate.

 Recursively traverses specified expression tree, following the #SUPPL_LSB_LEFT_CHANGED 
 and #SUPPL_LSB_RIGHT_CHANGED bits in the supplemental field.  Once an expression is
 found that has neither bit set, perform the expression operation and move back up
 the tree.  Once both left and right children have calculated values, perform the
 expression operation for the current expression, clear both changed bits and
 return.
*/
void sim_expr_changed( expression* expr ) {

  assert( expr != NULL );

  /* Traverse left child expression if it has changed */
  if( SUPPL_IS_LEFT_CHANGED( expr->suppl ) == 1 ) {

    sim_expr_changed( expr->left );

    /* Clear LEFT CHANGED bit */
    expr->suppl = expr->suppl & ~(0x1 << SUPPL_LSB_LEFT_CHANGED);

  }

  /* Traverse right child expression if it has changed */
  if( SUPPL_IS_RIGHT_CHANGED( expr->suppl ) == 1 ) {

    sim_expr_changed( expr->right );

    /* Clear RIGHT CHANGED bit */
    expr->suppl = expr->suppl & ~(0x1 << SUPPL_LSB_RIGHT_CHANGED);

  }

  /* Now perform expression operation for this expression */
  printf( "Performing expression operation: %d, id: %d\n", SUPPL_OP( expr->suppl ), expr->id );
  expression_operate( expr );

  /* Indicate that this expression has been executed */
  expr->suppl = expr->suppl | (0x1 << SUPPL_LSB_EXECUTED);

}

/*!
 \param expr  Pointer to expression to simulate.

 \return Returns 1 if the value of the root expression as non-zero; otherwise, returns
         a value of 0.

 Performs expression simulation as described above.  Returns the root expression
 value as a 1-bit value.
*/
int sim_expression( expression* expr ) {

  vector result;        /* Vector containing result of expression tree */
  nibble data;          /* Data for result vector                      */

  sim_expr_changed( expr );

  /* Evaluate the value of the root expression and return this value */
  vector_init( &result, &data, 1, 0 );
  vector_unary_op( &result, expr->value, or_optab );

  return( data & 0x1 );

}

/*!
 \param head_stmt  Pointer to head statement to simulate.

 Performs statement simulation as described above.  Calls expression simulator if
 the associated root expression is specified that signals have changed value within
 it.  Continues to run for current statement tree until statement tree hits a
 wait-for-event condition (or we reach the end of a simulation tree).
*/
void sim_statement( statement* head_stmt ) {

  statement* stmt;       /* Pointer to current statement to evaluate */

  /* Set the value of stmt with the head_stmt */
  stmt = head_stmt;

  /* Clear the STMT_HEAD bit in the root expression */
  stmt->exp->suppl = stmt->exp->suppl & ~(0x1 << SUPPL_LSB_STMT_HEAD);

  while( stmt != NULL ) {

    /* 
     If the root expression has changed, place root expression into expression
     simulation engine.
    */
    if( (SUPPL_IS_LEFT_CHANGED( stmt->exp->suppl )  == 1) ||
        (SUPPL_IS_RIGHT_CHANGED( stmt->exp->suppl ) == 1) ) {

      if( sim_expression( stmt->exp ) == 1 ) {

        stmt = stmt->next_true;

      } else {

        /* 
         If statement's next_false value is NULL, we need to wait.  Set STMT_HEAD
         bit in this statement.
        */
        if( stmt->next_false == NULL ) {
         
          stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);

        }

        stmt = stmt->next_false;

      }

    }

  }

}

/*!
 This function is the heart of the simulation engine.  It is called by the
 db_do_timestep() function in db.c  and moves the statements and expressions into
 the appropriate simulation functions.  See above explanation on this procedure.
*/
void sim_simulate() {

  stmt_link* curr_stmt;       /* Pointer to current statement to simulate */

  /* Get head statement from pre-simulation statement queue */
  curr_stmt = presim_stmt_head;

  while( curr_stmt != NULL ) {

    assert( curr_stmt->stmt != NULL );

    /* Place current statement into statement simulation engine and call it */
    sim_statement( curr_stmt->stmt );

    /* Remove current statement from head of queue */
    presim_stmt_head = presim_stmt_head->next;
    free_safe( curr_stmt );

  }

}

/* $Log$
/* Revision 1.3  2002/06/22 21:08:23  phase1geo
/* Added simulation engine and tied it to the db.c file.  Simulation engine is
/* currently untested and will remain so until the parser is updated correctly
/* for statements.  This will be the next step.
/* */
